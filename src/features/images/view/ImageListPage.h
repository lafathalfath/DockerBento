#pragma once
#include <QWidget>
#include <QLabel>
#include <QStackedWidget>
#include "shared/widgets/ToggleTable.h"
#include "features/images/viewmodel/ImageListViewModel.h"

namespace Core { class DockerClient; }
namespace Shared { class SearchBar; }

namespace Features::Images {

class ImageListPage : public QWidget {
    Q_OBJECT
public:
    explicit ImageListPage(ImageListViewModel *vm, Core::DockerClient *docker,
                           QWidget *parent = nullptr);

signals:
    void containerCreated();

private slots:
    void onImagesChanged();
    void onContextMenu(const QPoint &pos);

private:
    void setupUi();
    void runContainerFor(int row);
    Core::Image imageForRow(int row) const;

    ImageListViewModel *m_vm;
    Core::DockerClient *m_docker;
    Shared::ToggleTable *m_table{nullptr};
    Shared::SearchBar *m_search{nullptr};
    QLabel *m_statusLabel{nullptr};
    QStackedWidget *m_stack{nullptr};
};

} // namespace Features::Images
