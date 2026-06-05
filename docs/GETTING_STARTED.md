# Getting Started

This guide walks through setting up the DockerBento development environment, understanding the project structure, and making your first change.

## Prerequisites

1. **Linux** desktop environment
2. **Docker** daemon running and accessible via Unix socket
3. **CMake** 3.16 or newer
4. **Qt6** development packages (Core, Gui, Widgets, Network, Concurrent)
5. **C++17** compiler (GCC 7+, Clang 5+)

### Verify Prerequisites

```bash
# Check CMake
cmake --version    # needs 3.16+

# Check Qt6
qmake6 --version   # or: pkg-config --modversion Qt6Core

# Check Docker socket
curl --unix-socket /var/run/docker.sock http://localhost/v1.41/version
# Or for rootless:
curl --unix-socket /run/user/$(id -u)/docker.sock http://localhost/v1.41/version
```

## Build & Run

```bash
# Configure
cmake -B build

# Build
cmake --build build -j$(nproc)

# Run
./build/DockerBento
```

For a debug build:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

## Project Layout

```
DockerBento/
├── CMakeLists.txt          # Build configuration, all source files listed here
├── CLAUDE.md               # AI assistant instructions
├── README.md               # Project overview
├── docs/                   # Documentation
├── resources/
│   ├── resources.qrc       # Qt resource file
│   └── styles/
│       └── dark.qss        # Global stylesheet (scrollbars, tooltips)
└── src/
    ├── main.cpp            # Entry point, dark palette setup
    ├── core/               # Docker communication layer
    │   ├── docker/         # DockerClient + DockerSocketClient
    │   └── types/          # Data structs (Container, Image, Volume, Network)
    ├── features/           # Feature modules
    │   ├── containers/     # model/ viewmodel/ view/
    │   ├── images/         # model/ viewmodel/ view/
    │   ├── volumes/        # model/ viewmodel/ view/
    │   ├── networks/       # model/ viewmodel/ view/
    │   └── settings/       # model/ view/
    ├── shared/             # Reusable components
    │   ├── widgets/        # ToggleTable, StatusBadge, ConfirmDialog, SearchBar
    │   └── utils/          # ByteFormatter
    └── shell/              # App shell
        ├── MainWindow.h/.cpp
        └── SidebarWidget.h/.cpp
```

## Understanding the Architecture

### How a Feature Works

Take the **Containers** feature as an example:

1. **ContainerRepository** (`model/`) calls the Docker API via `DockerClient`:
   ```
   GET /containers/json?all=true → QList<Container>
   POST /containers/{id}/stop   → success/failure
   ```

2. **ContainerListViewModel** (`viewmodel/`) manages the list state:
   - Stores the container list and current filter
   - Runs a 5-second auto-refresh timer
   - Emits `containersChanged()` when data updates
   - Emits `actionSucceeded(msg)` / `actionFailed(error)` after actions

3. **ContainerListPage** (`view/`) renders the UI:
   - Toolbar with search, refresh, filters
   - ToggleTable with columns: Name, Image, Status, Ports, Created, Actions
   - Inline action buttons (Start/Stop + dropdown menu)
   - Smart diff update to avoid flicker on auto-refresh

### Signal Flow

```
User Action → View → ViewModel → Repository → DockerClient → Docker Daemon
                                                                    │
User sees ← View ← ViewModel (signal) ← Repository (callback) ←───┘
```

### Key Files to Read First

| File | Why |
|------|-----|
| `src/main.cpp` | Entry point, dark palette setup |
| `src/core/docker/DockerClient.h` | API methods and callback types |
| `src/core/docker/DockerSocketClient.cpp` | How HTTP-over-Unix-socket works |
| `src/core/types/Container.h` | Example data struct with fromJson |
| `src/features/containers/model/ContainerRepository.cpp` | Example repository |
| `src/features/containers/viewmodel/ContainerListViewModel.cpp` | Example viewmodel |
| `src/features/containers/view/ContainerListPage.cpp` | Example view with smart diff |
| `src/shared/widgets/ToggleTable.h` | Toggle selection behavior |
| `src/shell/MainWindow.cpp` | How features are wired together |

## Making Changes

### Modifying an Existing Feature

1. Find the relevant file in `src/features/<name>/`
2. Model changes go in the repository, UI changes go in the view, logic changes go in the viewmodel
3. Build and test: `cmake --build build -j$(nproc) && ./build/DockerBento`

### Adding a New Docker API Call

1. Add the method to the appropriate Repository class
2. Use `m_docker->get()`, `post()`, `postJson()`, or `del()` with a callback
3. In the ViewModel, add a method that calls the repository and emits success/failure signals
4. In the View, add a button or menu item that calls the ViewModel method

Example (adding a "rename container" action):

```cpp
// ContainerRepository.h — add method
void rename(const QString &id, const QString &newName,
            std::function<void(bool, QString)> callback);

// ContainerRepository.cpp — implement
void ContainerRepository::rename(const QString &id, const QString &newName,
                                  std::function<void(bool, QString)> callback) {
    // Always percent-encode user-supplied strings in URL query parameters
    QString encodedName = QString::fromUtf8(QUrl::toPercentEncoding(newName));
    m_docker->post(
        QString("/containers/%1/rename?name=%2").arg(id, encodedName),
        QJsonDocument(),
        [callback](bool ok, const QString &err) { callback(ok, err); }
    );
}
```

### Adding a New Feature Module

See [NEW_MODULE.md](NEW_MODULE.md) for a complete step-by-step guide.

## Debugging

### Connection Issues

If DockerBento shows "Disconnected" in the sidebar:
1. Check Docker is running: `systemctl status docker` or `systemctl --user status docker`
2. Check socket exists: `ls -la /var/run/docker.sock` or `ls -la /run/user/$(id -u)/docker.sock`
3. Check permissions: `groups` should include `docker`, or use rootless Docker
4. Open Settings (gear icon) → Auto Detect → Test Connection

### Build Issues

```bash
# Clean build
rm -rf build
cmake -B build
cmake --build build -j$(nproc)

# Verbose build (see compiler commands)
cmake --build build -j$(nproc) -- VERBOSE=1
```

### Qt MOC Issues

If you see "undefined reference to vtable" or signal/slot errors:
- Ensure the class has `Q_OBJECT` macro
- Ensure the header is listed in `HEADERS` in CMakeLists.txt
- Clean and rebuild: `rm -rf build && cmake -B build && cmake --build build`
