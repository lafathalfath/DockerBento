#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QSplitter>
#include "shell/SidebarWidget.h"

// Forward declarations
namespace Core { class DockerClient; }

namespace Features::Containers {
    class ContainerRepository;
    class ContainerListViewModel;
    class ContainerDetailViewModel;
    class ContainerListPage;
    class ContainerDetailPanel;
}
namespace Features::Images {
    class ImageRepository;
    class ImageListViewModel;
    class ImageListPage;
}
namespace Features::Volumes {
    class VolumeRepository;
    class VolumeListViewModel;
    class VolumeListPage;
}
namespace Features::Networks {
    class NetworkRepository;
    class NetworkListViewModel;
    class NetworkListPage;
}

namespace Shell {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void wireFeatures();
    void switchSection(Section section);
    void openSettings();
    void onConnectionRestored();

    // Core
    Core::DockerClient *m_docker{nullptr};

    // Shell
    SidebarWidget *m_sidebar{nullptr};
    QStackedWidget *m_contentStack{nullptr};
    QSplitter *m_containerSplitter{nullptr};

    // Containers
    Features::Containers::ContainerRepository *m_containerRepo{nullptr};
    Features::Containers::ContainerListViewModel *m_containerListVm{nullptr};
    Features::Containers::ContainerDetailViewModel *m_containerDetailVm{nullptr};
    Features::Containers::ContainerListPage *m_containerListPage{nullptr};
    Features::Containers::ContainerDetailPanel *m_containerDetailPanel{nullptr};

    // Images
    Features::Images::ImageRepository *m_imageRepo{nullptr};
    Features::Images::ImageListViewModel *m_imageListVm{nullptr};
    Features::Images::ImageListPage *m_imagePage{nullptr};

    // Volumes
    Features::Volumes::VolumeRepository *m_volumeRepo{nullptr};
    Features::Volumes::VolumeListViewModel *m_volumeListVm{nullptr};
    Features::Volumes::VolumeListPage *m_volumePage{nullptr};

    // Networks
    Features::Networks::NetworkRepository *m_networkRepo{nullptr};
    Features::Networks::NetworkListViewModel *m_networkListVm{nullptr};
    Features::Networks::NetworkListPage *m_networkPage{nullptr};
};

} // namespace Shell
