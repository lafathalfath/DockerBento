#pragma once
#include <QWidget>
#include <QLabel>
#include <QStackedWidget>
#include "shared/widgets/ToggleTable.h"
#include "features/volumes/viewmodel/VolumeListViewModel.h"

namespace Shared { class SearchBar; }

namespace Features::Volumes {

class VolumeListPage : public QWidget {
    Q_OBJECT
public:
    explicit VolumeListPage(VolumeListViewModel *vm, QWidget *parent = nullptr);

private:
    void setupUi();
    void onVolumesChanged();
    void onContextMenu(const QPoint &pos);

    VolumeListViewModel *m_vm;
    Shared::ToggleTable *m_table{nullptr};
    Shared::SearchBar *m_search{nullptr};
    QLabel *m_statusLabel{nullptr};
    QStackedWidget *m_stack{nullptr};
};

} // namespace Features::Volumes
