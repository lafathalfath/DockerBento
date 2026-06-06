cmake_minimum_required(VERSION 3.16)
project(DockerBento VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
)

find_package(PkgConfig REQUIRED)

# Ensure pkg-config can find system packages regardless of how CMake is launched
# (VSCode CMake Extension, IDEs, etc. may not inherit PKG_CONFIG_PATH from the shell)
if(NOT ENV{PKG_CONFIG_PATH})
    set(ENV{PKG_CONFIG_PATH} "/usr/lib/pkgconfig:/usr/share/pkgconfig")
endif()

pkg_check_modules(QTERMWIDGET REQUIRED qtermwidget6)

set(SOURCES
    src/main.cpp

    # Core
    src/core/docker/DockerClient.cpp
    src/core/docker/DockerSocketClient.cpp
    src/core/types/Container.cpp
    src/core/types/Image.cpp
    src/core/types/Volume.cpp
    src/core/types/Network.cpp

    # Shell
    src/shell/MainWindow.cpp
    src/shell/SidebarWidget.cpp

    # Shared
    src/shared/widgets/StatusBadge.cpp
    src/shared/widgets/ConfirmDialog.cpp
    src/shared/widgets/SearchBar.cpp
    src/shared/utils/ByteFormatter.cpp

    # Containers feature
    src/features/containers/model/ContainerRepository.cpp
    src/features/containers/viewmodel/ContainerListViewModel.cpp
    src/features/containers/viewmodel/ContainerDetailViewModel.cpp
    src/features/containers/view/ContainerListPage.cpp
    src/features/containers/view/ContainerDetailPanel.cpp
    src/features/containers/view/ContainerLogsView.cpp
    src/features/containers/view/ContainerTerminalView.cpp

    # Images feature
    src/features/images/model/ImageRepository.cpp
    src/features/images/viewmodel/ImageListViewModel.cpp
    src/features/images/view/ImageListPage.cpp
    src/features/images/view/RunContainerDialog.cpp

    # Volumes feature
    src/features/volumes/model/VolumeRepository.cpp
    src/features/volumes/viewmodel/VolumeListViewModel.cpp
    src/features/volumes/view/VolumeListPage.cpp

    # Networks feature
    src/features/networks/model/NetworkRepository.cpp
    src/features/networks/viewmodel/NetworkListViewModel.cpp
    src/features/networks/view/NetworkListPage.cpp

    # Settings feature
    src/features/settings/model/ConnectionSettings.cpp
    src/features/settings/view/SettingsDialog.cpp

    # Hub Catalog feature
    src/features/hub/model/HubRepository.cpp
    src/features/hub/viewmodel/HubSearchViewModel.cpp
    src/features/hub/view/HubSearchPage.cpp
    src/features/hub/view/HubDetailPage.cpp
)

set(HEADERS
    src/core/docker/DockerClient.h
    src/core/docker/DockerSocketClient.h
    src/core/types/Container.h
    src/core/types/Image.h
    src/core/types/Volume.h
    src/core/types/Network.h

    src/shell/MainWindow.h
    src/shell/SidebarWidget.h

    src/shared/widgets/StatusBadge.h
    src/shared/widgets/ConfirmDialog.h
    src/shared/widgets/SearchBar.h
    src/shared/utils/ByteFormatter.h

    src/features/containers/model/ContainerRepository.h
    src/features/containers/viewmodel/ContainerListViewModel.h
    src/features/containers/viewmodel/ContainerDetailViewModel.h
    src/features/containers/view/ContainerListPage.h
    src/features/containers/view/ContainerDetailPanel.h
    src/features/containers/view/ContainerLogsView.h
    src/features/containers/view/ContainerTerminalView.h

    src/features/images/model/ImageRepository.h
    src/features/images/viewmodel/ImageListViewModel.h
    src/features/images/view/ImageListPage.h
    src/features/images/view/RunContainerDialog.h

    src/shared/widgets/ToggleTable.h

    src/features/volumes/model/VolumeRepository.h
    src/features/volumes/viewmodel/VolumeListViewModel.h
    src/features/volumes/view/VolumeListPage.h

    src/features/networks/model/NetworkRepository.h
    src/features/networks/viewmodel/NetworkListViewModel.h
    src/features/networks/view/NetworkListPage.h

    src/features/settings/model/ConnectionSettings.h
    src/features/settings/view/SettingsDialog.h

    src/features/hub/model/HubRepository.h
    src/features/hub/viewmodel/HubSearchViewModel.h
    src/features/hub/view/HubSearchPage.h
    src/features/hub/view/HubDetailPage.h
    src/features/hub/view/HubImageCard.h
)

add_executable(DockerBento ${SOURCES} ${HEADERS} resources/resources.qrc)

target_include_directories(DockerBento PRIVATE src ${QTERMWIDGET_INCLUDE_DIRS})

target_link_libraries(DockerBento PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    ${QTERMWIDGET_LIBRARIES}
)

install(TARGETS DockerBento DESTINATION bin)
install(FILES packaging/io.github.dockerbento.DockerBento.desktop
    DESTINATION share/applications)
install(FILES packaging/io.github.dockerbento.DockerBento.appdata.xml
    DESTINATION share/metainfo)
install(FILES resources/icons/dockerbento.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME io.github.dockerbento.DockerBento.png)
