# DockerBento

A desktop Linux application for managing Docker containers, images, volumes, and networks. Built with Qt6 and C++17, communicating directly with the Docker daemon via Unix socket.

![Platform](https://img.shields.io/badge/platform-Linux-blue)
![Qt](https://img.shields.io/badge/Qt-6-green)
![C++](https://img.shields.io/badge/C++-17-orange)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

## Features

- **Containers** — List, start, stop, restart, pause, unpause, kill, remove. Inline action buttons and context menus. Detail panel with streaming logs. Auto-refresh every 5 seconds with flicker-free smart diff updates
- **Images** — List, remove, force remove, prune unused. Run containers from images with full configuration (ports, env vars, volumes, restart policy)
- **Volumes** — List, remove, prune unused
- **Networks** — List, remove (except built-in bridge/host/none), prune unused
- **Settings** — Configurable connection (Unix socket or TCP), auto-detect socket path, test connection, persistent settings
- **Dark Theme** — Full dark UI with custom color palette

## Screenshots

*Coming soon*

## Prerequisites

- **Linux** with a desktop environment
- **Docker** daemon running (rootless or root mode)
- **CMake** 3.16+
- **Qt6** with modules: Core, Gui, Widgets, Network, Concurrent
- **C++17** compatible compiler (GCC 7+, Clang 5+)

### Installing Dependencies

**Arch Linux:**
```bash
sudo pacman -S cmake qt6-base
```

**Ubuntu / Debian:**
```bash
sudo apt install cmake qt6-base-dev libgl1-mesa-dev
```

**Fedora:**
```bash
sudo dnf install cmake qt6-qtbase-devel
```

## Building from Source

```bash
cmake -B build
cmake --build build -j$(nproc)
./build/DockerBento
```

Make sure the Docker daemon is running. DockerBento auto-detects the socket path in this order:
1. `DOCKER_HOST` environment variable
2. Rootless socket: `/run/user/<uid>/docker.sock`
3. Default: `/var/run/docker.sock`

If auto-detect fails, configure the connection manually via the Settings dialog (gear icon in the sidebar).

## Distribution

Build a portable AppImage (works on any x86_64 Linux with glibc 2.17+, Wayland and X11):

```bash
bash packaging/build-appimage.sh
chmod +x DockerBento-x86_64.AppImage
./DockerBento-x86_64.AppImage
```

See [docs/DISTRIBUTION.md](docs/DISTRIBUTION.md) for Flatpak, AUR, and deb packaging.

## Project Structure

```
src/
  core/               # Docker client and type definitions
    docker/           # DockerClient (high-level), DockerSocketClient (HTTP over Unix socket)
    types/            # Container, Image, Volume, Network data structs
  features/           # Feature modules (model / viewmodel / view)
    containers/       # Container management with detail panel and logs
    images/           # Image management with run container dialog
    volumes/          # Volume management
    networks/         # Network management
    settings/         # Connection settings
  shared/             # Reusable widgets and utilities
    widgets/          # ToggleTable, StatusBadge, ConfirmDialog, SearchBar
    utils/            # ByteFormatter
  shell/              # App shell (MainWindow, SidebarWidget)
packaging/            # Distribution packaging scripts and metadata
  build-appimage.sh   # AppImage build script
  AppRun              # AppImage entry point (handles Wayland/X11 selection)
  *.desktop           # XDG desktop entry
  *.appdata.xml       # AppStream metadata
  *.yml               # Flatpak manifest
docs/                 # Documentation
  ARCHITECTURE.md     # System architecture and design decisions
  CODING_STANDARDS.md # Coding conventions, security rules, patterns
  GETTING_STARTED.md  # Developer setup and walkthrough
  NEW_MODULE.md       # Guide for adding new feature modules
  DISTRIBUTION.md     # AppImage, Flatpak, AUR packaging guides
```

## Documentation

| Document | Description |
|----------|-------------|
| [Architecture](docs/ARCHITECTURE.md) | System overview, layer pattern, core components, data flow |
| [Coding Standards](docs/CODING_STANDARDS.md) | Naming, Qt patterns, async safety, security rules, styling |
| [Getting Started](docs/GETTING_STARTED.md) | Developer setup, project walkthrough, first contribution |
| [New Module Guide](docs/NEW_MODULE.md) | Step-by-step guide for adding a new feature module |
| [Distribution](docs/DISTRIBUTION.md) | AppImage, Flatpak, AUR packaging guides |
| [Contributing](docs/CONTRIBUTING.md) | Branch workflow, commit message format, PR checklist |

## Architecture Overview

DockerBento uses a **feature-driven architecture** with three layers per feature:

- **Model** (Repository) — Data access via Docker API
- **ViewModel** — State management, filtering, actions
- **View** — Qt widgets and user interaction

Communication with Docker uses raw HTTP over Unix socket (`QLocalSocket`), not Qt's HTTP stack. All API calls are asynchronous with callback-based error handling.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for full details.

## License

MIT
