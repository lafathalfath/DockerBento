#include "MainWindow.h"
#include "core/docker/DockerClient.h"
#include "features/containers/model/ContainerRepository.h"
#include "features/containers/viewmodel/ContainerListViewModel.h"
#include "features/containers/viewmodel/ContainerDetailViewModel.h"
#include "features/containers/view/ContainerListPage.h"
#include "features/containers/view/ContainerDetailPanel.h"
#include "features/images/model/ImageRepository.h"
#include "features/images/viewmodel/ImageListViewModel.h"
#include "features/images/view/ImageListPage.h"
#include "features/volumes/model/VolumeRepository.h"
#include "features/volumes/viewmodel/VolumeListViewModel.h"
#include "features/volumes/view/VolumeListPage.h"
#include "features/networks/model/NetworkRepository.h"
#include "features/networks/viewmodel/NetworkListViewModel.h"
#include "features/networks/view/NetworkListPage.h"
#include "features/settings/view/SettingsDialog.h"
#include "features/hub/model/HubRepository.h"
#include "features/hub/viewmodel/HubSearchViewModel.h"
#include "features/hub/view/HubSearchPage.h"
#include <QHBoxLayout>
#include <QWidget>
#include <QJsonDocument>

namespace Shell {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("DockerBento");
    setMinimumSize(1100, 680);
    resize(1280, 760);

    m_docker = new Core::DockerClient(this);

    setupUi();
    wireFeatures();

    // Initial connection probe
    m_docker->testConnection([this](bool ok, const QString &, const QString &) {
        m_sidebar->setDockerConnected(ok);
        if (!ok) m_sidebar->showConnectionError();
    });

    connect(m_docker, &Core::DockerClient::connectionChanged,
            m_sidebar, &SidebarWidget::setDockerConnected);
    connect(m_sidebar, &SidebarWidget::settingsRequested,
            this, &MainWindow::openSettings);
}

void MainWindow::setupUi() {
    setStyleSheet("QMainWindow { background-color: #121212; }");

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_sidebar = new SidebarWidget(this);
    m_contentStack = new QStackedWidget;
    m_contentStack->setStyleSheet("background-color: #1a1a1a;");

    rootLayout->addWidget(m_sidebar);
    rootLayout->addWidget(m_contentStack, 1);
}

void MainWindow::wireFeatures() {
    // Containers
    m_containerRepo       = new Features::Containers::ContainerRepository(m_docker, this);
    m_containerListVm     = new Features::Containers::ContainerListViewModel(m_containerRepo, this);
    m_containerDetailVm   = new Features::Containers::ContainerDetailViewModel(m_containerRepo, this);
    m_containerListPage   = new Features::Containers::ContainerListPage(m_containerListVm);
    m_containerDetailPanel= new Features::Containers::ContainerDetailPanel(m_containerDetailVm);

    m_containerSplitter = new QSplitter(Qt::Horizontal);
    m_containerSplitter->setHandleWidth(1);
    m_containerSplitter->setStyleSheet("QSplitter::handle { background-color: #333; }");
    m_containerSplitter->addWidget(m_containerListPage);
    m_containerSplitter->addWidget(m_containerDetailPanel);
    m_containerSplitter->setSizes({700, 400});
    m_containerSplitter->setCollapsible(1, true);

    connect(m_containerListPage, &Features::Containers::ContainerListPage::containerSelected,
            m_containerDetailPanel, &Features::Containers::ContainerDetailPanel::showContainer);
    connect(m_containerListPage, &Features::Containers::ContainerListPage::selectionCleared,
            m_containerDetailPanel, &Features::Containers::ContainerDetailPanel::clearAndHide);
    m_containerDetailPanel->setVisible(false); // hidden until a row is selected

    // Images
    m_imageRepo   = new Features::Images::ImageRepository(m_docker, this);
    m_imageListVm = new Features::Images::ImageListViewModel(m_imageRepo, this);
    m_imagePage   = new Features::Images::ImageListPage(m_imageListVm, m_docker);

    // When a container is created from an image, refresh containers list
    connect(m_imagePage, &Features::Images::ImageListPage::containerCreated,
            this, [this]() { m_containerListVm->refresh(); });

    // Volumes
    m_volumeRepo   = new Features::Volumes::VolumeRepository(m_docker, this);
    m_volumeListVm = new Features::Volumes::VolumeListViewModel(m_volumeRepo, this);
    m_volumePage   = new Features::Volumes::VolumeListPage(m_volumeListVm);

    // Networks
    m_networkRepo   = new Features::Networks::NetworkRepository(m_docker, this);
    m_networkListVm = new Features::Networks::NetworkListViewModel(m_networkRepo, this);
    m_networkPage   = new Features::Networks::NetworkListPage(m_networkListVm);

    // Hub Catalog
    m_hubRepo  = new Features::Hub::HubRepository(this);
    m_hubVm    = new Features::Hub::HubSearchViewModel(m_hubRepo, m_imageRepo, this);
    m_hubPage  = new Features::Hub::HubSearchPage(m_hubVm);

    connect(m_hubPage, &Features::Hub::HubSearchPage::imagePulled,
            this, [this]() { m_imageListVm->refresh(); });

    m_contentStack->addWidget(m_containerSplitter); // 0
    m_contentStack->addWidget(m_imagePage);          // 1
    m_contentStack->addWidget(m_volumePage);         // 2
    m_contentStack->addWidget(m_networkPage);        // 3
    m_contentStack->addWidget(m_hubPage);            // 4

    connect(m_sidebar, &SidebarWidget::sectionChanged, this, &MainWindow::switchSection);
}

void MainWindow::switchSection(Section section) {
    switch (section) {
        case Section::Containers: m_contentStack->setCurrentIndex(0); break;
        case Section::Images:     m_contentStack->setCurrentIndex(1); break;
        case Section::Volumes:    m_contentStack->setCurrentIndex(2); break;
        case Section::Networks:   m_contentStack->setCurrentIndex(3); break;
        case Section::Hub:        m_contentStack->setCurrentIndex(4); break;
    }
}

void MainWindow::openSettings() {
    auto *dlg = new Features::Settings::SettingsDialog(m_docker, this);
    connect(dlg, &Features::Settings::SettingsDialog::settingsApplied,
            this, [this](const Features::Settings::ConnectionSettings &) {
                onConnectionRestored();
            });
    dlg->exec();
    dlg->deleteLater();
}

void MainWindow::onConnectionRestored() {
    // Refresh all feature data after reconnect
    m_containerListVm->refresh();
    m_imageListVm->refresh();
    m_volumeListVm->refresh();
    m_networkListVm->refresh();

    m_docker->testConnection([this](bool ok, const QString &, const QString &) {
        m_sidebar->setDockerConnected(ok);
    });
}

} // namespace Shell
