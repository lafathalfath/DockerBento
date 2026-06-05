# Adding a New Feature Module

This guide walks through adding a new feature module to DockerBento, using a hypothetical "Compose" feature as an example.

## Overview

Every feature module follows the same structure:

```
src/features/<name>/
  model/
    <Name>Repository.h/.cpp     # Data access (Docker API calls)
  viewmodel/
    <Name>ListViewModel.h/.cpp  # State management, filtering, actions
  view/
    <Name>ListPage.h/.cpp       # Table-based list UI
```

## Step 1: Create Directory Structure

```bash
mkdir -p src/features/compose/{model,viewmodel,view}
```

## Step 2: Create the Repository (Model Layer)

The repository handles Docker API calls and returns parsed data.

**`src/features/compose/model/ComposeRepository.h`**

```cpp
#pragma once
#include <QObject>
#include <QList>
#include <functional>

namespace Core { class DockerClient; }

namespace Features::Compose {

struct ComposeProject {
    QString name;
    QString status;
    QString configFile;
};

class ComposeRepository : public QObject {
    Q_OBJECT
public:
    explicit ComposeRepository(Core::DockerClient *docker, QObject *parent = nullptr);

    void fetchAll(std::function<void(bool, QList<ComposeProject>, QString)> callback);
    void remove(const QString &name, std::function<void(bool, QString)> callback);

private:
    Core::DockerClient *m_docker;
};

} // namespace Features::Compose
```

**`src/features/compose/model/ComposeRepository.cpp`**

```cpp
#include "ComposeRepository.h"
#include "core/docker/DockerClient.h"
#include <QJsonArray>
#include <QJsonObject>

namespace Features::Compose {

ComposeRepository::ComposeRepository(Core::DockerClient *docker, QObject *parent)
    : QObject(parent), m_docker(docker)
{}

void ComposeRepository::fetchAll(
    std::function<void(bool, QList<ComposeProject>, QString)> callback)
{
    m_docker->get("/containers/json?all=true",
        [callback](bool ok, const QJsonDocument &doc, const QString &err) {
            if (!ok) { callback(false, {}, err); return; }
            // Parse response into ComposeProject list...
            QList<ComposeProject> projects;
            callback(true, projects, {});
        });
}

void ComposeRepository::remove(const QString &name,
    std::function<void(bool, QString)> callback)
{
    m_docker->del(QString("/compose/%1").arg(name), callback);
}

} // namespace Features::Compose
```

## Step 3: Create the ViewModel

The viewmodel manages state and translates user actions into repository calls.

**`src/features/compose/viewmodel/ComposeListViewModel.h`**

```cpp
#pragma once
#include <QObject>
#include <QList>
#include "features/compose/model/ComposeRepository.h"

namespace Features::Compose {

class ComposeListViewModel : public QObject {
    Q_OBJECT
public:
    explicit ComposeListViewModel(ComposeRepository *repo, QObject *parent = nullptr);

    const QList<ComposeProject> &projects() const { return m_filtered; }
    void refresh();
    void setFilter(const QString &text);
    void removeProject(const QString &name);

signals:
    void projectsChanged();
    void loadingChanged(bool loading);
    void actionSucceeded(const QString &message);
    void actionFailed(const QString &error);

private:
    void applyFilter();

    ComposeRepository *m_repo;
    QList<ComposeProject> m_all;
    QList<ComposeProject> m_filtered;
    QString m_filter;
};

} // namespace Features::Compose
```

**`src/features/compose/viewmodel/ComposeListViewModel.cpp`**

```cpp
#include "ComposeListViewModel.h"

namespace Features::Compose {

ComposeListViewModel::ComposeListViewModel(ComposeRepository *repo, QObject *parent)
    : QObject(parent), m_repo(repo)
{}

void ComposeListViewModel::refresh() {
    emit loadingChanged(true);
    m_repo->fetchAll([this](bool ok, QList<ComposeProject> list, const QString &err) {
        emit loadingChanged(false);
        if (!ok) { emit actionFailed(err); return; }
        m_all = list;
        applyFilter();
    });
}

void ComposeListViewModel::setFilter(const QString &text) {
    m_filter = text;
    applyFilter();
}

void ComposeListViewModel::removeProject(const QString &name) {
    m_repo->remove(name, [this, name](bool ok, const QString &err) {
        if (ok) {
            emit actionSucceeded(QString("Removed %1").arg(name));
            refresh();
        } else {
            emit actionFailed(err);
        }
    });
}

void ComposeListViewModel::applyFilter() {
    if (m_filter.isEmpty()) {
        m_filtered = m_all;
    } else {
        m_filtered.clear();
        for (const auto &p : m_all)
            if (p.name.contains(m_filter, Qt::CaseInsensitive))
                m_filtered.append(p);
    }
    emit projectsChanged();
}

} // namespace Features::Compose
```

## Step 4: Create the View (Page)

The view builds the UI and connects to viewmodel signals.

**`src/features/compose/view/ComposeListPage.h`**

```cpp
#pragma once
#include <QWidget>
#include <QLabel>
#include <QStackedWidget>
#include "shared/widgets/ToggleTable.h"
#include "features/compose/viewmodel/ComposeListViewModel.h"

namespace Shared { class SearchBar; }

namespace Features::Compose {

class ComposeListPage : public QWidget {
    Q_OBJECT
public:
    explicit ComposeListPage(ComposeListViewModel *vm, QWidget *parent = nullptr);

private:
    void setupUi();
    void onProjectsChanged();
    void onContextMenu(const QPoint &pos);

    ComposeListViewModel *m_vm;
    Shared::ToggleTable *m_table{nullptr};
    Shared::SearchBar *m_search{nullptr};
    QLabel *m_statusLabel{nullptr};
    QStackedWidget *m_stack{nullptr};
};

} // namespace Features::Compose
```

**Important view conventions:**
- Use `Shared::ToggleTable` (not QTableWidget)
- Include `QTableWidget::item:selected:alternate { background-color: #1565c0; }` in the table stylesheet
- Follow the toolbar layout pattern: title, stretch, status label, search bar, action buttons
- Fixed toolbar height: 52px

## Step 5: Add Files to CMakeLists.txt

Add source files under the appropriate section:

```cmake
set(SOURCES
    # ... existing sources ...

    # Compose feature
    src/features/compose/model/ComposeRepository.cpp
    src/features/compose/viewmodel/ComposeListViewModel.cpp
    src/features/compose/view/ComposeListPage.cpp
)

set(HEADERS
    # ... existing headers ...

    src/features/compose/model/ComposeRepository.h
    src/features/compose/viewmodel/ComposeListViewModel.h
    src/features/compose/view/ComposeListPage.h
)
```

## Step 6: Wire Up in MainWindow

**`src/shell/MainWindow.h`** — Add forward declarations and member variables:

```cpp
namespace Features::Compose {
    class ComposeRepository;
    class ComposeListViewModel;
    class ComposeListPage;
}

// In the class body:
Features::Compose::ComposeRepository *m_composeRepo{nullptr};
Features::Compose::ComposeListViewModel *m_composeListVm{nullptr};
Features::Compose::ComposeListPage *m_composePage{nullptr};
```

**`src/shell/MainWindow.cpp`** — Create instances and wire signals:

```cpp
#include "features/compose/model/ComposeRepository.h"
#include "features/compose/viewmodel/ComposeListViewModel.h"
#include "features/compose/view/ComposeListPage.h"

// In setupUi() or wireFeatures():
m_composeRepo = new Features::Compose::ComposeRepository(m_docker, this);
m_composeListVm = new Features::Compose::ComposeListViewModel(m_composeRepo, this);
m_composePage = new Features::Compose::ComposeListPage(m_composeListVm);
m_contentStack->addWidget(m_composePage);
```

## Step 7: Add Sidebar Button

**`src/shell/SidebarWidget.h`** — Add the new section to the `Section` enum:

```cpp
enum class Section { Containers, Images, Volumes, Networks, Compose };
```

**`src/shell/SidebarWidget.cpp`** — Add a navigation button in `setupUi()`:

```cpp
auto *composeBtn = makeSidebarButton("Compose");
connect(composeBtn, &QPushButton::clicked, this, [this]() {
    emit sectionChanged(Section::Compose);
});
```

## Step 8: Handle Section Switching

In `MainWindow::switchSection()`, add the new case:

```cpp
case Section::Compose:
    m_contentStack->setCurrentWidget(m_composePage);
    break;
```

## Step 9: Build and Test

```bash
cmake --build build -j$(nproc)
./build/DockerBento
```

Verify:
- New button appears in sidebar
- Clicking it shows the new page
- Data loads from Docker API
- Search/filter works
- Context menu actions work
- Toggle selection (click to select, click again to deselect) works via ToggleTable

## Checklist

- [ ] Directory structure: `model/`, `viewmodel/`, `view/`
- [ ] Repository with async callbacks
- [ ] ViewModel with standard signals (`Changed`, `loadingChanged`, `actionSucceeded`, `actionFailed`)
- [ ] View using `Shared::ToggleTable` with correct stylesheet
- [ ] Files added to CMakeLists.txt (SOURCES and HEADERS)
- [ ] Wired in MainWindow (instances created, signals connected, added to content stack)
- [ ] Sidebar button added in SidebarWidget
- [ ] Section switching handled in MainWindow
- [ ] Builds without errors
- [ ] Update documentation (ARCHITECTURE.md, CLAUDE.md if needed)
