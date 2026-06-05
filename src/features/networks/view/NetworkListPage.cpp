#include "NetworkListPage.h"
#include "shared/widgets/SearchBar.h"
#include "shared/widgets/ConfirmDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>

namespace Features::Networks {

NetworkListPage::NetworkListPage(NetworkListViewModel *vm, QWidget *parent)
    : QWidget(parent), m_vm(vm)
{
    setupUi();
    connect(m_vm, &NetworkListViewModel::networksChanged, this, &NetworkListPage::onNetworksChanged);
    connect(m_vm, &NetworkListViewModel::loadingChanged, this, [this](bool loading) {
        m_stack->setCurrentIndex(loading ? 0 : 1);
    });
    connect(m_vm, &NetworkListViewModel::actionSucceeded, this, [this](const QString &msg) {
        m_statusLabel->setText(msg);
        m_statusLabel->setStyleSheet("color: #4caf50; font-size: 12px;");
    });
    connect(m_vm, &NetworkListViewModel::actionFailed, this, [](const QString &msg) {
        QMessageBox::warning(nullptr, "Error", msg);
    });

    m_vm->refresh();
}

void NetworkListPage::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(52);
    toolbar->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #333;");
    auto *tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(16, 8, 16, 8);

    auto *title = new QLabel("Networks");
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #e0e0e0;");
    m_search = new Shared::SearchBar("Search networks...");
    m_search->setMaximumWidth(280);

    auto *pruneBtn = new QPushButton("Prune Unused");
    pruneBtn->setStyleSheet(
        "QPushButton { background-color: #7b1fa2; color: white; border-radius: 4px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #9c27b0; }"
    );

    auto *refreshBtn = new QPushButton("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #e0e0e0; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #444; }"
    );

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");

    tl->addWidget(title);
    tl->addStretch();
    tl->addWidget(m_statusLabel);
    tl->addWidget(m_search);
    tl->addWidget(pruneBtn);
    tl->addWidget(refreshBtn);

    m_stack = new QStackedWidget;
    auto *loading = new QLabel("Loading...");
    loading->setAlignment(Qt::AlignCenter);
    loading->setStyleSheet("color: #888; font-size: 14px;");

    m_table = new Shared::ToggleTable;
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"Name", "Driver", "Scope", "Subnet", "Gateway", "Internal", "Containers"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #1a1a1a; color: #e0e0e0; border: none; }"
        "QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid #2a2a2a; }"
        "QTableWidget::item:selected          { background-color: #1565c0; }"
        "QTableWidget::item:selected:alternate { background-color: #1565c0; }"
        "QHeaderView::section { background-color: #252525; color: #aaa; border: none;"
        "  border-bottom: 1px solid #333; padding: 6px 8px; font-size: 12px; }"
        "QTableWidget::item:alternate { background-color: #1d1d1d; }"
    );

    m_stack->addWidget(loading);
    m_stack->addWidget(m_table);

    root->addWidget(toolbar);
    root->addWidget(m_stack);

    connect(m_search, &QLineEdit::textChanged, m_vm, &NetworkListViewModel::setFilter);
    connect(refreshBtn, &QPushButton::clicked, m_vm, &NetworkListViewModel::refresh);
    connect(pruneBtn, &QPushButton::clicked, this, [this]() {
        if (Shared::ConfirmDialog::confirm(this, "Prune Networks",
            "Remove all unused networks?", "Prune", true))
            m_vm->pruneNetworks();
    });
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &NetworkListPage::onContextMenu);
}

void NetworkListPage::onNetworksChanged() {
    const auto &networks = m_vm->networks();
    m_table->setRowCount(networks.size());

    for (int i = 0; i < networks.size(); ++i) {
        const auto &n = networks[i];
        m_table->setRowHeight(i, 36);

        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            return item;
        };

        QString subnet, gateway;
        if (!n.ipam.isEmpty()) {
            subnet = n.ipam[0].subnet;
            gateway = n.ipam[0].gateway;
        }

        m_table->setItem(i, 0, makeItem(n.name));
        m_table->setItem(i, 1, makeItem(n.driver));
        m_table->setItem(i, 2, makeItem(n.scope));
        m_table->setItem(i, 3, makeItem(subnet.isEmpty() ? "-" : subnet));
        m_table->setItem(i, 4, makeItem(gateway.isEmpty() ? "-" : gateway));
        m_table->setItem(i, 5, makeItem(n.internal ? "Yes" : "No"));
        m_table->setItem(i, 6, makeItem(QString::number(n.containerCount)));

        m_table->item(i, 0)->setData(Qt::UserRole, n.id);
        m_table->item(i, 0)->setData(Qt::UserRole + 1, n.name);
    }

    m_statusLabel->setText(QString("%1 network%2").arg(networks.size()).arg(networks.size() != 1 ? "s" : ""));
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");
    m_stack->setCurrentIndex(1);
}

void NetworkListPage::onContextMenu(const QPoint &pos) {
    int row = m_table->rowAt(pos.y());
    if (row < 0) return;
    auto *item = m_table->item(row, 0);
    if (!item) return;

    QString id = item->data(Qt::UserRole).toString();
    QString name = item->data(Qt::UserRole + 1).toString();

    // Don't allow removing built-in networks
    static const QStringList builtIn = {"bridge", "host", "none"};
    if (builtIn.contains(name)) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #444; }"
        "QMenu::item { padding: 6px 20px; }"
        "QMenu::item:selected { background-color: #1565c0; }"
    );
    menu.addAction("Remove", [this, id, name]() {
        if (Shared::ConfirmDialog::confirm(this, "Remove Network",
            QString("Remove network <b>%1</b>?").arg(name), "Remove", true))
            m_vm->removeNetwork(id);
    });

    menu.exec(m_table->viewport()->mapToGlobal(pos));
}

} // namespace Features::Networks
