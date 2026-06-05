#pragma once
#include <QWidget>
#include <QLabel>
#include <QTabWidget>
#include "core/types/Container.h"
#include "features/containers/viewmodel/ContainerDetailViewModel.h"

namespace Features::Containers {

class ContainerLogsView;

class ContainerDetailPanel : public QWidget {
    Q_OBJECT
public:
    explicit ContainerDetailPanel(ContainerDetailViewModel *vm, QWidget *parent = nullptr);

    void showContainer(const Core::Container &container);
    void clearAndHide();

private:
    void setupUi();
    void populateInfo();

    ContainerDetailViewModel *m_vm;
    QLabel *m_nameLabel{nullptr};
    QLabel *m_idLabel{nullptr};
    QLabel *m_imageLabel{nullptr};
    QLabel *m_statusLabel{nullptr};
    QLabel *m_createdLabel{nullptr};
    QLabel *m_commandLabel{nullptr};
    QLabel *m_networksLabel{nullptr};
    QLabel *m_portsLabel{nullptr};
    QTabWidget *m_tabs{nullptr};
    ContainerLogsView *m_logsView{nullptr};
    QWidget *m_infoPage{nullptr};
};

} // namespace Features::Containers
