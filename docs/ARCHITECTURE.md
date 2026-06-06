# Architecture

DockerBento is a Qt6 C++17 desktop application for managing Docker resources locally. It communicates with the Docker daemon via HTTP-over-Unix-socket and provides a dark-themed UI for containers, images, volumes, and networks.

## System Overview

```
┌───────────────────────────────────────────────────────┐
│                    MainWindow (Shell)                 │
│  ┌────────────┐  ┌──────────────────────────────────┐ │
│  │ Sidebar    │  │  QStackedWidget (content area)   │ │
│  │            │  │  ┌───────────────────────────┐   │ │
│  │ Containers │  │  │ ContainerListPage         │   │ │
│  │ Images     │  │  │ ImageListPage             │   │ │
│  │ Volumes    │  │  │ VolumeListPage            │   │ │
│  │ Networks   │  │  │ NetworkListPage           │   │ │
│  │ Hub Catalog│  │  │ HubSearchPage             │   │ │
│  │            │  │  └───────────────────────────┘   │ │
│  │ ───────────│  │                                  │ │
│  │ ● Status   │  │  ContainerDetailPanel (splitter) │ │
│  │ ⚙ Settings│  │                                  │ │
│  └────────────┘  └──────────────────────────────────┘ │
└───────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────┐     ┌────────────────────┐
│  DockerClient    │────▶│ DockerSocketClient │
│  (high-level)    │     │ (raw HTTP/Unix)    │
└─────────────────┘     └────────────────────┘
         │
         ▼
    Docker Engine API v1.41
    /var/run/docker.sock
```

## Layer Architecture

Each feature follows a three-layer pattern:

### Model (Repository)
- **Location**: `src/features/<feature>/model/`
- **Role**: Data access layer. Makes Docker API calls through DockerClient and returns parsed results
- **Pattern**: Methods accept callbacks. On success, parse JSON into type structs. On failure, propagate error strings
- **Examples**: `ContainerRepository::fetchAll()`, `ImageRepository::pull()`, `VolumeRepository::prune()`

### ViewModel
- **Location**: `src/features/<feature>/viewmodel/`
- **Role**: Presentation logic. Manages list state, filtering, sorting, auto-refresh timers. Translates user actions into repository calls
- **Pattern**: QObject with signals for state changes (`containersChanged`, `loadingChanged`, `actionSucceeded`, `actionFailed`). Views connect to these signals
- **Examples**: `ContainerListViewModel` manages container list + 5-second auto-refresh timer

### View
- **Location**: `src/features/<feature>/view/`
- **Role**: Qt widgets. Builds UI, connects to ViewModel signals, handles user interaction
- **Pattern**: Each page is a QWidget with toolbar + ToggleTable. Action columns with inline buttons + dropdown menus. Context menus via right-click
- **Examples**: `ContainerListPage`, `ContainerDetailPanel`, `ContainerLogsView`

## Hub Catalog Feature

The Hub Catalog feature (`src/features/hub/`) is structurally different from other features because it communicates with the **Docker Hub REST API** (internet) rather than the local Docker daemon.

- **HubRepository** — uses `QNetworkAccessManager` to call `https://hub.docker.com/v2/search/repositories/`. No dependency on `DockerClient`.
- **HubSearchViewModel** — manages search state, pagination (25 results/page), and delegates pull operations to `Features::Images::ImageRepository` (which uses the local Docker daemon).
- **HubSearchPage** — search bar, paginated results table, live pull log terminal view.

Pull flow: Hub Catalog → `HubSearchViewModel::pullImage()` → `ImageRepository::pull()` → Docker daemon streams progress back → `HubSearchPage` displays it in a log view.

## Core Layer

### DockerClient (`src/core/docker/DockerClient.h/.cpp`)

High-level HTTP client. Provides typed methods:
- `get(path, JsonCallback)` — GET request, parse JSON response
- `post(path, body, PlainCallback)` — POST request, success/failure
- `postJson(path, body, JsonCallback)` — POST request, parse JSON response
- `del(path, PlainCallback)` — DELETE request
- `streamGet(path, StreamCallback)` — Streaming GET (used for logs)
- `testConnection(callback)` — Tests connectivity to Docker daemon
- `applySettings(cfg)` — Applies connection settings (socket path, timeout)

Auto-detects socket path: `DOCKER_HOST` env → rootless socket (`/run/user/<uid>/docker.sock`) → default (`/var/run/docker.sock`).

Emits `connectionChanged(bool)` signal when connectivity state changes.

### DockerSocketClient (`src/core/docker/DockerSocketClient.h/.cpp`)

Low-level HTTP-over-Unix-socket implementation using `QLocalSocket`.

**Critical design**: All async state is heap-allocated via `std::shared_ptr`:
- `shared_ptr<QByteArray> responseData` — accumulates response bytes across readyRead signals
- `shared_ptr<bool> done` — ensures callback fires exactly once

This prevents double-free and use-after-free when multiple Qt signals fire (readyRead, disconnected, errorOccurred, timeout).

**HTTP handling**:
- Builds raw HTTP/1.1 request with `Connection: close`
- Decodes chunked transfer encoding manually
- Skips `PeerClosedError` (normal for Docker — server closes connection after response)
- Timeout guard via QTimer

### Type Structs (`src/core/types/`)

Plain data structs with `fromJson(QJsonObject)` static factory methods:
- `Container` — id, name, image, status, state, ports, created
- `Image` — id, repoTags, size, created, containers count
- `Volume` — name, driver, mountpoint, scope, created
- `Network` — id, name, driver, scope, IPAM config, container count

## Shell Layer

### MainWindow (`src/shell/MainWindow.h/.cpp`)

Root QMainWindow. Responsibilities:
- Creates all repositories, view models, and views
- Wires cross-feature signals (e.g., `ImageListPage::containerCreated` → refresh containers)
- Manages content stack switching
- Hosts container splitter (list + detail panel)
- Detail panel starts hidden, shown on selection, hidden on selectionCleared

### SidebarWidget (`src/shell/SidebarWidget.h/.cpp`)

Navigation sidebar with:
- Section buttons (Containers, Images, Volumes, Networks)
- Connection status indicator (green dot = connected, red = disconnected)
- Settings button
- Error hint text when disconnected

## Shared Widgets

### ToggleTable (`src/shared/widgets/ToggleTable.h`)

Header-only QTableWidget subclass. Clicking a selected row deselects it (toggle behavior). Emits `selectionCleared()` signal. Implemented by tracking `m_lastSelectedRow` and swallowing the mouse event when deselecting (prevents Qt from re-selecting).

Used by all four list pages (containers, images, volumes, networks).

### Other Shared Widgets
- **StatusBadge** — Colored pill label for container status (running=green, exited=red, etc.)
- **ConfirmDialog** — Static `confirm()` method with destructive action styling
- **SearchBar** — Styled QLineEdit with placeholder text
- **ByteFormatter** — Formats bytes to human-readable (KB, MB, GB)

## Settings & Connection

`ConnectionSettings` struct supports Unix socket and TCP connection types. Persisted via `QSettings`. `SettingsDialog` provides:
- Radio buttons: Unix Socket / TCP
- Socket path input with auto-detect button
- TCP host/port fields
- Timeout spinbox
- Test connection button
- Apply & reconnect

## Styling

Dark theme applied in two layers:
1. **QPalette** in `main.cpp` — sets global colors (window, text, highlight, etc.)
2. **Inline stylesheets** on widgets — per-component colors, borders, hover states

Color palette:
| Purpose | Color |
|---------|-------|
| Background | `#1a1a1a` |
| Surface | `#1e1e1e` |
| Alternate row | `#1d1d1d` |
| Header bg | `#252525` |
| Border | `#333` |
| Text primary | `#e0e0e0` |
| Text secondary | `#aaa`, `#888` |
| Accent/Selection | `#1565c0` |
| Success | `#4caf50`, `#2e7d32` |
| Danger | `#c62828`, `#d32f2f` |
| Warning | `#f57f17` |
| Prune button | `#7b1fa2` |

## Auto-Refresh

Container list auto-refreshes every 5 seconds using a QTimer in `ContainerListViewModel`. Uses smart diff update in `ContainerListPage::updateTable()`:
1. Compare existing rows by container ID + status text
2. Skip rows that haven't changed
3. Wrap in `setUpdatesEnabled(false/true)` to prevent flicker
4. Skip loading page on subsequent refreshes (only show on first load)

## Security Model

All Docker communication goes through `DockerSocketClient` over a Unix socket. There is no shell execution — `QProcess`, `system()`, and `popen()` are never used.

**Input handling at API boundaries:**
- User-supplied strings inserted into URL query parameters are percent-encoded via `QUrl::toPercentEncoding()` before use (e.g., container name in `RunContainerDialog`)
- Signal names passed to `ContainerRepository::kill()` are validated against a static whitelist (`SIGKILL`, `SIGTERM`, `SIGHUP`, `SIGINT`, `SIGQUIT`, `SIGUSR1`, `SIGUSR2`, `SIGSTOP`, `SIGCONT`)
- Container/image IDs that come from the Docker API are hex strings and are safe to interpolate directly

**Dangerous operations:**
- Force remove, prune, and privileged container mode require both a `ConfirmDialog` confirmation and a visible inline warning label in the UI

## Data Flow Example

```
User clicks "Stop" button on container row
  → ContainerListPage calls m_vm->stopContainer(id)
    → ContainerListViewModel calls m_repo->stop(id, callback)
      → ContainerRepository calls m_docker->post("/containers/{id}/stop", ...)
        → DockerClient calls sendRequest("POST", ...)
          → DockerSocketClient opens QLocalSocket → docker.sock
            → Docker daemon stops container
          ← HTTP response parsed
        ← PlainCallback fires (success/error)
      ← Repository callback fires
    ← ViewModel emits actionSucceeded, calls refresh()
      → Repository fetches updated list
      ← ViewModel emits containersChanged
        → ContainerListPage::updateTable() diffs and updates rows
```
