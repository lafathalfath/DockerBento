#pragma once
#include <QWidget>
#include <QLabel>
#include <QStackedWidget>
#include "shared/widgets/ToggleTable.h"
#include "features/containers/viewmodel/ContainerListViewModel.h"

namespace Shared { class SearchBar; }

namespace Features::Containers {

class ContainerListPage : public QWidget {
    Q_OBJECT
public:
    explicit ContainerListPage(ContainerListViewModel *vm, QWidget *parent = nullptr);

signals:
    void containerSelected(const Core::Container &container);
    void selectionCleared();

private slots:
    void onContainersChanged();
    void onRowDoubleClicked(int row, int col);
    void onContextMenu(const QPoint &pos);
    void onSelectionChanged();

private:
    void setupUi();
    // Smart update: diff existing rows instead of full rebuild
    void updateTable();
    void setRow(int row, const Core::Container &c);
    QWidget *makeActionWidget(const Core::Container &c);
    void showContextMenuForId(const QString &containerId, const QPoint &globalPos);
    QString formatPorts(const Core::Container &c) const;
    Core::Container containerForRow(int row) const;

    ContainerListViewModel *m_vm;
    Shared::ToggleTable *m_table{nullptr};
    Shared::SearchBar *m_search{nullptr};
    QLabel        *m_statusLabel{nullptr};
    QLabel        *m_loadingLabel{nullptr};
    QStackedWidget *m_stack{nullptr};
};

} // namespace Features::Containers
