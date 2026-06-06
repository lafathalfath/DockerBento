# DockerBento

Desktop Linux application for managing Docker containers, images, volumes, and networks via Docker Unix socket. Built with Qt6 C++17.

## Build

```bash
cmake -B build
cmake --build build -j$(nproc)
./build/DockerBento
```

### Dependencies

- CMake 3.16+
- Qt6: Core, Gui, Widgets, Network, Concurrent
- C++17 compiler (GCC 7+, Clang 5+)
- Docker daemon running (unix socket)

## Architecture

Feature-driven with model/viewmodel/view layers. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for details.

```
src/
  core/           # Docker client, type definitions
    docker/       # DockerClient (high-level), DockerSocketClient (raw HTTP over unix socket)
    types/        # Container, Image, Volume, Network structs
  features/       # Feature modules, each with model/viewmodel/view
    containers/   # Container list, detail panel, logs, actions
    images/       # Image list, run container dialog
    volumes/      # Volume list, prune
    networks/     # Network list, prune
    settings/     # Connection settings dialog
    hub/          # Docker Hub catalog: search, browse, pull (uses QNetworkAccessManager)
  shared/         # Reusable widgets (ToggleTable, StatusBadge, SearchBar, ConfirmDialog)
    widgets/
    utils/        # ByteFormatter
  shell/          # MainWindow, SidebarWidget (app shell)
```

## Key Conventions

- **Namespaces**: `Core::`, `Features::Containers::`, `Features::Images::`, `Features::Hub::`, `Shared::`, `Shell::` — mirror directory structure
- **Naming**: PascalCase classes, camelCase methods/variables, `m_` prefix for member variables, UPPER_CASE constants
- **Headers**: `#pragma once`, forward-declare where possible, minimal includes in headers
- **Qt patterns**: Q_OBJECT macro on all QObject subclasses, signals/slots for communication, lambda connections preferred
- **Memory**: Qt parent-child ownership. Use `std::shared_ptr` for shared state in async lambdas (see DockerSocketClient)
- **Styling**: Dark theme via QPalette in main.cpp + per-widget inline stylesheets. Color constants: bg `#1a1a1a`, surface `#1e1e1e`, border `#333`, text `#e0e0e0`, accent `#1565c0`
- **Docker API**: v1.41 via HTTP over Unix socket. All calls are async with callbacks
- **Security**: User-supplied strings in URL params must use `QUrl::toPercentEncoding()`. Signal names must pass through a whitelist in `ContainerRepository::kill()`. No shell execution (`QProcess`, `system()`) ever

## File Patterns

- Each feature has: `model/XRepository.h/.cpp`, `viewmodel/XListViewModel.h/.cpp`, `view/XListPage.h/.cpp`
- New source files must be added to `CMakeLists.txt` in both SOURCES and HEADERS sections
- Header-only widgets go in HEADERS only (e.g., ToggleTable.h)

## Common Tasks

### Adding a new feature
1. Create directory under `src/features/<name>/` with model/, viewmodel/, view/ subdirs
2. Create Repository (model), ViewModel, and Page (view) classes
3. Add all files to CMakeLists.txt
4. Wire up in MainWindow: create instances, connect signals, add to content stack
5. Add sidebar button in SidebarWidget

### Adding a Docker API call
1. Add method to the relevant Repository class
2. Call via `m_docker->get/post/del()` with callback
3. Emit success/failure signals from ViewModel

### Table pages
- Use `Shared::ToggleTable` (not QTableWidget) for click-to-toggle selection
- Smart diff updates preferred: compare row data before replacing (see ContainerListPage::updateTable)
- Always include `QTableWidget::item:selected:alternate { background-color: #1565c0; }` in table stylesheet

### Security rules
- Percent-encode any user-supplied value inserted into a URL: `QUrl::toPercentEncoding(name)`
- Signal names must be validated against the whitelist in `ContainerRepository::kill()`
- Never use `QProcess`, `system()`, or `popen()` — all Docker calls go through DockerClient
- Dangerous operations (privileged containers, force remove, prune) require both a ConfirmDialog and a visible warning label in the UI

### Distribution
- AppImage: `bash packaging/build-appimage.sh` → produces `DockerBento-x86_64.AppImage`
- Flatpak: `flatpak-builder --user --install --force-clean build-flatpak packaging/io.github.dockerbento.DockerBento.yml`
- See [docs/DISTRIBUTION.md](docs/DISTRIBUTION.md) for full details and known issues

## Documentation

Keep documentation updated with every change. See:
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — system architecture and design decisions
- [docs/CODING_STANDARDS.md](docs/CODING_STANDARDS.md) — coding conventions and patterns
- [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) — developer setup and project walkthrough
- [docs/NEW_MODULE.md](docs/NEW_MODULE.md) — step-by-step guide for adding a new feature module
- [docs/DISTRIBUTION.md](docs/DISTRIBUTION.md) — how to build and package for distribution (AppImage, Flatpak, deb)
- [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) — branch strategy, commit message format, PR checklist
