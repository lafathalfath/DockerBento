# Distribution Guide

How to build DockerBento for distribution to end users.

## Release Build (bare binary)

A release build strips debug symbols and enables full optimization:

```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j$(nproc)
```

The resulting binary at `build-release/DockerBento` is statically linked against your system Qt. It will only run on systems with a **compatible Qt6** installed.

To install system-wide:
```bash
sudo cmake --install build-release --prefix /usr
```

This installs:
- Binary → `/usr/bin/DockerBento`
- Desktop entry → `/usr/share/applications/`
- AppStream metadata → `/usr/share/metainfo/`
- Icon → `/usr/share/icons/hicolor/256x256/apps/`

---

## Option 1: AppImage (Recommended for GitHub Releases)

AppImage bundles the binary + all Qt libraries into a single portable `.AppImage` file. Users download one file, `chmod +x`, and run it — no installation needed, works on any x86_64 Linux with glibc 2.17+.

### Build an AppImage

```bash
bash packaging/build-appimage.sh
```

This script:
1. Downloads `linuxdeploy`, `linuxdeploy-plugin-qt`, and `appimagetool` on first run (into `packaging/`)
2. Builds a release binary with CMake
3. Installs into a staging `AppDir/`
4. Calls `linuxdeploy --plugin qt` to bundle Qt plugins and libraries
5. Manually copies `libqwayland.so` and all Wayland sub-plugins (linuxdeploy-plugin-qt doesn't do this automatically)
6. Replaces the linuxdeploy-generated `AppRun` symlink with `packaging/AppRun`, a shell script that sets `QT_QPA_PLATFORM` to prefer Wayland when available, falling back to xcb (X11)
7. Calls `appimagetool --no-appstream` to produce `DockerBento-x86_64.AppImage` in the project root

The resulting AppImage runs on both Wayland and X11 sessions.

### Manual steps (if the script doesn't work)

```bash
# 1. Build release
cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-release -j$(nproc)
DESTDIR=AppDir cmake --install build-release

# 2. Bundle Qt
export QMAKE=$(which qmake6)
./packaging/linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --executable AppDir/usr/bin/DockerBento \
    --desktop-file packaging/io.github.dockerbento.DockerBento.desktop \
    --icon-file resources/icons/dockerbento.png \
    --plugin qt \
    --output appimage
```

### Known issues on Arch Linux (and other rolling distros)

**ELF `.relr.dyn` relocations**: Modern libraries use a newer ELF relocation format that the `strip` bundled inside linuxdeploy cannot process. The script already sets `NO_STRIP=1` to skip stripping — the release binary is already optimized by CMake (`-O2`) so runtime performance is unaffected.

**AppStream URL checks**: The script calls `appimagetool` directly (instead of `--output appimage`) to pass `--no-appstream`, which bypasses URL reachability checks for the AppStream metadata during local builds.

**Wayland platform plugin**: `linuxdeploy-plugin-qt` only deploys the platform plugin matching the current session (xcb on X11, wayland on Wayland). The script manually copies `libqwayland.so` and all wayland sub-plugins (shell integrations, decorations, graphics) so the AppImage works on both X11 and Wayland sessions.

### What to upload to GitHub Releases

Upload `DockerBento-x86_64.AppImage`. In the release notes, tell users:

```bash
chmod +x DockerBento-x86_64.AppImage
./DockerBento-x86_64.AppImage
```

---

## Option 2: Flatpak

Flatpak runs in a sandbox and can be distributed via [Flathub](https://flathub.org). This is the recommended path for a wide audience on modern distros.

### Build locally

```bash
# Install flatpak-builder if not present
sudo pacman -S flatpak-builder        # Arch
sudo apt install flatpak-builder      # Debian/Ubuntu

# Add KDE runtime (needed once)
flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.kde.Platform//6.9 org.kde.Sdk//6.9

# Build and install locally for testing
flatpak-builder --user --install --force-clean build-flatpak \
    packaging/io.github.dockerbento.DockerBento.yml

# Run
flatpak run io.github.dockerbento.DockerBento
```

### Docker socket access in Flatpak

Flatpak sandboxes filesystem access. The manifest pre-grants access to the two common socket paths, but users with non-standard paths need to grant access manually:

```bash
# Rootless Docker (most common on modern Linux)
flatpak override --user \
    --filesystem=/run/user/$UID/docker.sock \
    io.github.dockerbento.DockerBento

# Root Docker daemon
flatpak override --user \
    --filesystem=/var/run/docker.sock \
    io.github.dockerbento.DockerBento
```

### Submitting to Flathub

1. Fork [flathub/flathub](https://github.com/flathub/flathub)
2. Create `io.github.dockerbento.DockerBento/` directory
3. Copy the manifest from `packaging/io.github.dockerbento.DockerBento.yml`
4. Update the `sources` entry to point to a released tarball or git tag (not `type: dir`)
5. Open a pull request on the Flathub repo

---

## Option 3: Distro Packages (AUR, deb, rpm)

### Arch Linux (AUR)

Create a `PKGBUILD`:

```bash
pkgname=dockerbento
pkgver=1.0.0
pkgrel=1
pkgdesc="Desktop manager for Docker containers, images, volumes and networks"
arch=('x86_64')
url="https://github.com/dockerbento/DockerBento"
license=('MIT')
depends=('qt6-base')
makedepends=('cmake' 'git')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")

build() {
    cmake -B build -S "DockerBento-$pkgver" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build -j$(nproc)
}

package() {
    DESTDIR="$pkgdir" cmake --install build
    install -Dm644 "DockerBento-$pkgver/LICENSE" "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
```

### Debian/Ubuntu (.deb)

Use `cmake --install` with a `DESTDIR` and then `dpkg-deb`, or use `CPack`:

Add to `CMakeLists.txt`:
```cmake
include(CPack)
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <you@example.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6widgets6, libqt6network6")
set(CPACK_PACKAGE_VERSION "1.0.0")
```

Then:
```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-release -j$(nproc)
cd build-release && cpack -G DEB
```

---

## Packaging Files

```
packaging/
  build-appimage.sh                              # AppImage build script (downloads tools on first run)
  AppRun                                         # AppImage entry point — handles Wayland/X11 selection
  io.github.dockerbento.DockerBento.desktop      # XDG desktop entry
  io.github.dockerbento.DockerBento.appdata.xml  # AppStream metadata (Flathub/GNOME Software)
  io.github.dockerbento.DockerBento.yml          # Flatpak manifest
resources/
  icons/
    dockerbento.png  # 256x256 icon (replace with actual icon before publishing)
```

The packaging tools (`linuxdeploy-x86_64.AppImage`, `linuxdeploy-plugin-qt-x86_64.AppImage`, `appimagetool-x86_64.AppImage`) are downloaded automatically on first run and are excluded from git via `.gitignore`.

## Checklist Before Publishing a Release

- [ ] Update version in `CMakeLists.txt` (`project(DockerBento VERSION x.y.z)`)
- [ ] Update version in `packaging/io.github.dockerbento.DockerBento.appdata.xml`
- [ ] Replace `resources/icons/dockerbento.png` with a proper 256×256 icon
- [ ] Update `packaging/io.github.dockerbento.DockerBento.appdata.xml` URL fields with the real repository URL
- [ ] Build release binary and test on a clean system
- [ ] Build and test AppImage
- [ ] Tag the release in git: `git tag v1.0.0 && git push --tags`
- [ ] Upload AppImage to GitHub Releases
