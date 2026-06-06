#!/usr/bin/env bash
# Build a self-contained AppImage for DockerBento.
#
# Requirements:
#   - linuxdeploy + linuxdeploy-plugin-qt (downloaded automatically on first run)
#   - Qt6 development files installed
#   - CMake, make/ninja
#
# Usage:
#   cd /path/to/DockerBento
#   bash packaging/build-appimage.sh
#
# Output: DockerBento-x86_64.AppImage in the project root

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-release"
APPDIR="$PROJECT_DIR/AppDir"

cd "$PROJECT_DIR"

# ── Download linuxdeploy tools if missing ──────────────────────────────────────
LINUXDEPLOY="$PROJECT_DIR/packaging/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="$PROJECT_DIR/packaging/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="$PROJECT_DIR/packaging/appimagetool-x86_64.AppImage"

if [ ! -f "$LINUXDEPLOY" ]; then
    echo "Downloading linuxdeploy..."
    wget -q --show-progress -O "$LINUXDEPLOY" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY"
fi

if [ ! -f "$LINUXDEPLOY_QT" ]; then
    echo "Downloading linuxdeploy-plugin-qt..."
    wget -q --show-progress -O "$LINUXDEPLOY_QT" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY_QT"
fi

if [ ! -f "$APPIMAGETOOL" ]; then
    echo "Downloading appimagetool..."
    wget -q --show-progress -O "$APPIMAGETOOL" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "$APPIMAGETOOL"
fi

# ── Build release binary ───────────────────────────────────────────────────────
echo "Building release binary..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILD_DIR" -j"$(nproc)"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

# ── Deploy Qt libraries and plugins ───────────────────────────────────────────
echo "Deploying Qt dependencies..."
rm -f "$APPDIR/AppRun" "$APPDIR"/*.desktop "$APPDIR"/*.png

export QMAKE="$(which qmake6 2>/dev/null || which qmake)"
# NO_STRIP: linuxdeploy bundles an old strip that cannot handle .relr.dyn ELF
# sections used by modern Arch/Fedora/Ubuntu libraries. Release binary is
# already optimized by CMake (-O2).
export NO_STRIP=1

# ── Stub out missing Qt plugin dependencies ───────────────────────────────────
# linuxdeploy-plugin-qt fails hard when any Qt plugin has an unresolvable
# transitive dep (e.g. kimg_jxr.so needs libjxrglue.so.0 which isn't installed).
# Strategy:
#   1. Scan ALL Qt plugins for missing libs and create minimal ELF stubs.
#   2. Prepend stub dir to LD_LIBRARY_PATH so linuxdeploy's ldd checks pass.
#   3. After deployment, remove every AppDir plugin that still has unresolved
#      deps (those that pulled in stubs) and remove the stubs themselves.
STUB_DIR="$(mktemp -d)"
QTPLUG_SCAN="$("${QMAKE}" -query QT_INSTALL_PLUGINS)"

echo "Scanning Qt plugins for unresolvable dependencies..."
declare -A _SEEN_STUBS
while IFS= read -r so; do
    while IFS= read -r libname; do
        [ -z "$libname" ] && continue
        if [ -z "${_SEEN_STUBS[$libname]+x}" ]; then
            _SEEN_STUBS["$libname"]=1
            # Minimal ELF shared-object stub: satisfies ldd without real symbols
            printf 'void __stub_init(void) {}\n' \
                | gcc -x c - -shared -fPIC -Wl,-soname,"${libname}" \
                      -o "${STUB_DIR}/${libname}" 2>/dev/null \
                && echo "  Stub: ${libname}"
        fi
    done < <(ldd "${so}" 2>&1 | awk '/not found/{print $1}')
done < <(find "${QTPLUG_SCAN}" -name "*.so" 2>/dev/null)

export LD_LIBRARY_PATH="${STUB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/DockerBento" \
    --plugin qt

# ── Remove stub-dependent plugins from AppDir ─────────────────────────────────
# After deployment, any plugin that depends on one of our stubs (and nothing
# else provides that library on the target system) would be broken at runtime.
# Remove them — they are optional KDE/Plasma extras, not needed by DockerBento.
echo "Removing AppDir plugins with unresolvable dependencies..."
while IFS= read -r so; do
    # Check without our stub LD_LIBRARY_PATH
    missing=$(LD_LIBRARY_PATH="" ldd "${so}" 2>&1 | awk '/not found/{print $1}' | tr '\n' ' ')
    if [ -n "$missing" ]; then
        echo "  Removing $(basename "${so}"): needs ${missing}"
        rm "${so}"
    fi
done < <(find "${APPDIR}/usr/plugins" -name "*.so" 2>/dev/null)

# Remove stub .so files that linuxdeploy copied into AppDir/usr/lib
for stub in "${STUB_DIR}"/*.so* "${STUB_DIR}"/*.so; do
    [ -f "$stub" ] || continue
    name="$(basename "${stub}")"
    [ -f "${APPDIR}/usr/lib/${name}" ] && rm "${APPDIR}/usr/lib/${name}" && echo "  Removed stub copy: ${name}"
done

# ── Add Wayland platform plugin (linuxdeploy-plugin-qt skips it) ──────────────
# linuxdeploy-plugin-qt only deploys the platform plugin that matches the
# current session (xcb on X11, wayland on Wayland). Since builds often happen
# on X11 sessions, the Wayland plugin is frequently missing. Add it manually.
add_wayland_support() {
    local WAYLAND_PLUGIN="/usr/lib/qt6/plugins/platforms/libqwayland.so"
    [ -f "$WAYLAND_PLUGIN" ] || { echo "No libqwayland.so found, skipping"; return; }

    echo "Adding Wayland platform plugin..."
    cp "$WAYLAND_PLUGIN" "$APPDIR/usr/plugins/platforms/"

    # Copy all Qt6 wayland sub-plugins (shell integrations, decorations, graphics)
    for subdir in wayland-shell-integration wayland-decoration-client \
                  wayland-graphics-integration-client; do
        src_dir="/usr/lib/qt6/plugins/$subdir"
        if [ -d "$src_dir" ]; then
            dst_dir="$APPDIR/usr/plugins/$subdir"
            mkdir -p "$dst_dir"
            cp "$src_dir"/*.so "$dst_dir/" 2>/dev/null && echo "  Added $subdir plugins"
        fi
    done

    # Copy dependencies of libqwayland.so and its sub-plugins that aren't yet in AppDir
    local APPLIB="$APPDIR/usr/lib"
    while IFS= read -r plugin; do
        ldd "$plugin" 2>/dev/null | awk '/=> \// { print $3 }' | while read -r lib; do
            name=$(basename "$lib")
            [ "$name" = "linux-vdso.so.1" ] && continue
            [ -f "$APPLIB/$name" ] && continue
            case "$name" in
                libc.so*|libm.so*|libdl.so*|libpthread.so*|librt.so*) continue ;;
                libGL.so*|libEGL.so*|libGLdispatch.so*) continue ;;
                ld-linux*.so*) continue ;;
            esac
            [ -f "$lib" ] && cp "$lib" "$APPLIB/" && echo "  Added $name"
        done
    done < <(find "$APPDIR/usr/plugins" -name "*.so" 2>/dev/null)
}
add_wayland_support

# ── Package into AppImage (manual appimagetool call) ──────────────────────────
# We call appimagetool directly instead of using --output appimage so we can
# pass --no-appstream to skip URL reachability checks during local builds.
echo "Creating AppImage..."

# Copy desktop file and icon to AppDir root (normally done by linuxdeploy output step)
cp "$PROJECT_DIR/packaging/io.github.dockerbento.DockerBento.desktop" "$APPDIR/"
cp "$PROJECT_DIR/resources/icons/dockerbento.png" "$APPDIR/"
ln -sf "io.github.dockerbento.DockerBento.desktop" "$APPDIR/.DirIcon" 2>/dev/null || true

# linuxdeploy creates AppDir/AppRun as a symlink → usr/bin/DockerBento.
# cp through a symlink overwrites the target (the ELF binary), not the symlink.
# Remove the symlink first so we write a real file.
rm -f "$APPDIR/AppRun"
cp "$PROJECT_DIR/packaging/AppRun" "$APPDIR/AppRun"
chmod +x "$APPDIR/AppRun"

OUTPUT="$PROJECT_DIR/DockerBento-x86_64.AppImage"
ARCH=x86_64 "$APPIMAGETOOL" --no-appstream "$APPDIR" "$OUTPUT"

echo ""
echo "Done!"
echo "AppImage: $OUTPUT ($(du -sh "$OUTPUT" | cut -f1))"
