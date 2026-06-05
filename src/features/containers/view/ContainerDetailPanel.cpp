#include "ContainerDetailPanel.h"
#include "ContainerLogsView.h"
#include "ContainerTerminalView.h"
#include "shared/widgets/StatusBadge.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>

namespace Features::Containers {

ContainerDetailPanel::ContainerDetailPanel(ContainerDetailViewModel *vm, QWidget *parent)
    : QWidget(parent), m_vm(vm)
{
    setupUi();
    connect(m_vm, &ContainerDetailViewModel::detailLoaded, this, &ContainerDetailPanel::populateInfo);
}

void ContainerDetailPanel::setupUi() {
    setStyleSheet("background-color: #1a1a1a;");
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Header
    auto *header = new QWidget;
    header->setStyleSheet("background-color: #212121; border-bottom: 1px solid #333;");
    header->setFixedHeight(52);
    auto *hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(16, 8, 16, 8);

    m_nameLabel = new QLabel("Select a container");
    m_nameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #e0e0e0;");
    m_idLabel = new QLabel;
    m_idLabel->setStyleSheet("color: #888; font-size: 11px; font-family: monospace;");
    m_statusLabel = new QLabel;

    hLayout->addWidget(m_nameLabel);
    hLayout->addWidget(m_idLabel);
    hLayout->addStretch();
    hLayout->addWidget(m_statusLabel);

    // Tabs
    m_tabs = new QTabWidget;
    m_tabs->setStyleSheet(
        "QTabWidget::pane { border: none; background-color: #1a1a1a; }"
        "QTabBar::tab { background-color: #252525; color: #aaa; padding: 8px 16px;"
        "  border: none; border-right: 1px solid #333; }"
        "QTabBar::tab:selected { color: #e0e0e0; border-bottom: 2px solid #1976d2;"
        "  background-color: #1a1a1a; }"
        "QTabBar::tab:hover { background-color: #2a2a2a; }"
    );

    // Info tab
    m_infoPage = new QWidget;
    m_infoPage->setStyleSheet("background-color: #1a1a1a;");
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidget(m_infoPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #1a1a1a; }");

    auto *formLayout = new QFormLayout(m_infoPage);
    formLayout->setContentsMargins(20, 16, 20, 16);
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);

    auto makeLabelKey = [](const QString &text) {
        auto *l = new QLabel(text);
        l->setStyleSheet("color: #888; font-size: 12px;");
        return l;
    };
    auto makeLabelVal = [this]() {
        auto *l = new QLabel;
        l->setStyleSheet("color: #e0e0e0; font-size: 12px;");
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        l->setWordWrap(true);
        return l;
    };

    m_imageLabel = makeLabelVal();
    m_createdLabel = makeLabelVal();
    m_commandLabel = makeLabelVal();
    m_commandLabel->setStyleSheet("color: #e0e0e0; font-size: 12px; font-family: monospace; background-color: #252525; padding: 4px 8px; border-radius: 4px;");
    m_networksLabel = makeLabelVal();
    m_portsLabel = makeLabelVal();

    formLayout->addRow(makeLabelKey("Image"), m_imageLabel);
    formLayout->addRow(makeLabelKey("Created"), m_createdLabel);
    formLayout->addRow(makeLabelKey("Command"), m_commandLabel);
    formLayout->addRow(makeLabelKey("Networks"), m_networksLabel);
    formLayout->addRow(makeLabelKey("Ports"), m_portsLabel);

    // Logs tab
    m_logsView = new ContainerLogsView(m_vm);

    // Terminal tab
    m_terminalView = new ContainerTerminalView;

    m_tabs->addTab(scrollArea, "Info");
    m_tabs->addTab(m_logsView, "Logs");
    m_tabs->addTab(m_terminalView, "Terminal");

    m_tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    root->addWidget(header);
    root->addWidget(m_tabs, 1);
}

void ContainerDetailPanel::showContainer(const Core::Container &container) {
    setVisible(true);
    m_nameLabel->setText(container.name);
    m_idLabel->setText(container.shortId);
    m_vm->load(container);
    m_logsView->attachContainer(container);
    m_terminalView->attachContainer(container);
}

void ContainerDetailPanel::clearAndHide() {
    m_vm->stopLogStream();
    m_terminalView->stopSession();
    setVisible(false);
}

void ContainerDetailPanel::populateInfo() {
    const auto &c = m_vm->container();

    m_imageLabel->setText(c.image);
    m_createdLabel->setText(c.created.toString("yyyy-MM-dd hh:mm:ss"));
    m_commandLabel->setText(c.command);
    m_networksLabel->setText(c.networks.join(", "));

    QStringList ports;
    for (const auto &p : c.ports) {
        if (p.hostPort > 0)
            ports.append(QString("%1:%2->%3/%4").arg(p.hostIp.isEmpty() ? "0.0.0.0" : p.hostIp)
                                                 .arg(p.hostPort).arg(p.containerPort).arg(p.protocol));
        else
            ports.append(QString("%1/%2").arg(p.containerPort).arg(p.protocol));
    }
    m_portsLabel->setText(ports.isEmpty() ? "-" : ports.join("\n"));
}

} // namespace Features::Containers
