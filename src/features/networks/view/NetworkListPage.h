#pragma once
#include <QWidget>
#include <QLabel>
#include <QStackedWidget>
#include "shared/widgets/ToggleTable.h"
#include "features/networks/viewmodel/NetworkListViewModel.h"

namespace Shared { class SearchBar; }

namespace Features::Networks {

class NetworkListPage : public QWidget {
    Q_OBJECT
public:
    explicit NetworkListPage(NetworkListViewModel *vm, QWidget *parent = nullptr);

private:
    void setupUi();
    void onNetworksChanged();
    void onContextMenu(const QPoint &pos);

    NetworkListViewModel *m_vm;
    Shared::ToggleTable *m_table{nullptr};
    Shared::SearchBar *m_search{nullptr};
    QLabel *m_statusLabel{nullptr};
    QStackedWidget *m_stack{nullptr};
};

} // namespace Features::Networks
