# Coding Standards

C++17 / Qt6 conventions used in DockerBento.

## Language & Build

- **Standard**: C++17 (`CMAKE_CXX_STANDARD 17`)
- **Build system**: CMake 3.16+ with `CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, `CMAKE_AUTOUIC` enabled
- **Qt modules**: Core, Gui, Widgets, Network, Concurrent

## Naming

| Element | Convention | Example |
|---------|-----------|---------|
| Classes | PascalCase | `ContainerListPage`, `DockerClient` |
| Methods/functions | camelCase | `fetchAll()`, `updateTable()` |
| Member variables | `m_` prefix + camelCase | `m_docker`, `m_socketPath` |
| Local variables | camelCase | `statusCode`, `responseBody` |
| Constants/enums | PascalCase values | `ConnectionType::UnixSocket` |
| Namespaces | PascalCase, nested | `Features::Containers::` |
| Signals | camelCase, past tense or state | `containersChanged`, `loadingChanged` |
| Slots | `on` prefix + camelCase | `onContainersChanged`, `onContextMenu` |
| Files | PascalCase matching class | `ContainerListPage.h/.cpp` |

## Namespaces

Mirror directory structure:

```
src/core/docker/     → Core::
src/core/types/      → Core::
src/features/containers/  → Features::Containers::
src/features/images/      → Features::Images::
src/features/volumes/     → Features::Volumes::
src/features/networks/    → Features::Networks::
src/features/settings/    → Features::Settings::
src/shared/widgets/  → Shared::
src/shared/utils/    → Shared::
src/shell/           → Shell::
```

## Headers

- Use `#pragma once` (not include guards)
- Forward-declare classes in headers when only pointer/reference is needed
- Minimal includes in headers; put remaining includes in .cpp files
- Group includes: own header first, then project headers, then Qt headers, then std headers

```cpp
#include "ContainerListPage.h"          // own header
#include "shared/widgets/SearchBar.h"   // project
#include <QVBoxLayout>                  // Qt
#include <memory>                       // std
```

## Qt Patterns

### Q_OBJECT Macro
Every QObject subclass that uses signals/slots must have Q_OBJECT in the class body.

### Signal/Slot Connections
Prefer lambda connections for conciseness:

```cpp
connect(m_vm, &ViewModel::dataChanged, this, [this]() {
    updateTable();
});
```

Use member function connections for public slots that may be connected from outside:

```cpp
connect(m_vm, &ViewModel::dataChanged, this, &View::onDataChanged);
```

### Parent-Child Ownership
- Qt widgets are owned by their parent. Pass `this` or a parent widget to constructors
- Heap-allocated widgets in layouts are owned by the layout's parent widget
- Use `deleteLater()` for objects that need deferred deletion (e.g., dialogs after exec)

### Async Callback Memory Safety
For async operations with multiple signal paths (readyRead, disconnected, errorOccurred, timeout), use `std::shared_ptr` for shared state:

```cpp
auto responseData = std::make_shared<QByteArray>();
auto done = std::make_shared<bool>(false);

auto finish = [done, callback](int code, QByteArray data, QString err) {
    if (*done) return;       // guard: fire exactly once
    *done = true;
    callback(code, data, err);
};
```

This prevents double-free and use-after-free when multiple lambdas capture the same state.

## Feature Structure

Every feature follows this directory layout:

```
src/features/<name>/
  model/
    <Name>Repository.h/.cpp    — Data access, Docker API calls
  viewmodel/
    <Name>ListViewModel.h/.cpp — State management, filtering, actions
  view/
    <Name>ListPage.h/.cpp      — Table-based list UI
```

### Repository (Model Layer)
- Takes `Core::DockerClient*` in constructor
- Methods are async with callback parameters
- Parses JSON responses into Core type structs
- Does not hold state — stateless data access

```cpp
void fetchAll(std::function<void(bool, QList<Core::Container>, QString)> callback);
```

### ViewModel
- QObject subclass with signals for state changes
- Holds the filtered list of items
- Manages auto-refresh timer (containers: 5 seconds)
- Action methods call repository, then refresh on success

Standard signals:
```cpp
signals:
    void containersChanged();
    void loadingChanged(bool loading);
    void actionSucceeded(const QString &message);
    void actionFailed(const QString &error);
```

### View (Page)
- QWidget subclass
- Creates UI in `setupUi()` method
- Uses `Shared::ToggleTable` for tables (not raw QTableWidget)
- Connects to ViewModel signals in constructor

## Table Conventions

### ToggleTable
Always use `Shared::ToggleTable` instead of `QTableWidget`. This provides click-to-toggle selection and emits `selectionCleared()`.

### Stylesheet
Every table must include the alternate-selected rule to prevent CSS specificity issues:

```cpp
m_table->setStyleSheet(
    "QTableWidget { background-color: #1a1a1a; color: #e0e0e0; border: none; }"
    "QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid #2a2a2a; }"
    "QTableWidget::item:selected          { background-color: #1565c0; }"
    "QTableWidget::item:selected:alternate { background-color: #1565c0; }"
    "QHeaderView::section { background-color: #252525; color: #aaa; border: none;"
    "  border-bottom: 1px solid #333; padding: 6px 8px; font-size: 12px; }"
    "QTableWidget::item:alternate { background-color: #1d1d1d; }"
);
```

### Smart Diff Updates
For tables that auto-refresh, use smart diff instead of full rebuild:
1. Compare each row's ID + display state with current data
2. Skip rows that haven't changed
3. Wrap update in `setUpdatesEnabled(false)` / `setUpdatesEnabled(true)`
4. Only show loading page on first load, not subsequent refreshes

### Action Columns
Inline action column with primary button + "more" dropdown:

```cpp
auto *runBtn = new QToolButton;    // primary action
auto *moreBtn = new QToolButton;   // "⋮" dropdown
moreBtn->setPopupMode(QToolButton::InstantPopup);
auto *menu = new QMenu(moreBtn);
// ... add menu actions
moreBtn->setMenu(menu);
```

## Styling

### Approach
- Global dark palette set in `main.cpp` via `QPalette`
- Per-widget styling via inline `setStyleSheet()` — not external .qss for component styles
- `resources/styles/dark.qss` only for global overrides (scrollbars, tooltips)

### Button Styles

| Type | Background | Hover |
|------|-----------|-------|
| Primary action (start/run) | `#2e7d32` | `#388e3c` |
| Danger action (stop/remove) | `#c62828` | `#d32f2f` |
| Prune | `#7b1fa2` | `#9c27b0` |
| Neutral | `#333` | `#444` |

### Context Menus

```cpp
menu.setStyleSheet(
    "QMenu { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #444; }"
    "QMenu::item { padding: 6px 20px; }"
    "QMenu::item:selected { background-color: #1565c0; }"
);
```

## CMakeLists.txt

When adding new files:
1. Add `.cpp` files to the `SOURCES` list under the appropriate feature comment
2. Add `.h` files to the `HEADERS` list
3. Header-only files (like ToggleTable.h) go in HEADERS only

## Error Handling

- Repository methods pass errors as `QString` in callbacks
- ViewModels emit `actionFailed(QString)` which views display as `QMessageBox::warning`
- Success messages are shown in the status label (green `#4caf50`)
- Connection failures are shown in the sidebar (red dot + hint text)

## Docker API

- Target API version: v1.41
- All paths are relative: `/containers/json`, `/images/json`, etc.
- Use `?all=true` for listing all containers (including stopped)
- Container actions: POST `/containers/{id}/start`, `/stop`, `/restart`, `/pause`, `/unpause`, `/kill`
- Destructive actions: DELETE `/containers/{id}`, `/images/{id}`, `/volumes/{name}`, `/networks/{id}`
- Built-in networks (bridge, host, none) cannot be removed — check name before offering remove action

## Security

### URL Parameter Encoding
Any user-supplied string inserted into a Docker API URL query parameter **must** be percent-encoded:

```cpp
#include <QUrl>

// WRONG — user input directly in URL
path += "?name=" + name;

// CORRECT — percent-encode before inserting
path += "?name=" + QString::fromUtf8(QUrl::toPercentEncoding(name));
```

Container IDs and image IDs from the Docker API itself are safe to interpolate directly (they are hex strings).

### Signal Whitelist
`ContainerRepository::kill()` whitelists valid Unix signal names before passing to the API. Any new code that accepts a signal name from the UI must validate against this list: `SIGKILL, SIGTERM, SIGHUP, SIGINT, SIGQUIT, SIGUSR1, SIGUSR2, SIGSTOP, SIGCONT`.

### No Shell Execution
Never use `QProcess`, `system()`, `popen()`, or any shell execution. All Docker operations go through DockerClient → DockerSocketClient → Unix socket. This is a hard rule — it eliminates an entire class of injection vulnerabilities.

### Dangerous Operations — Warn the User
For operations with large blast radius (privileged containers, force remove, prune), show a visible warning label in the UI in addition to the confirm dialog. Do not rely solely on checkbox labels. See `RunContainerDialog` for the privileged mode warning pattern (`m_privilegedWarning`).
