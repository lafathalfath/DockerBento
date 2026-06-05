#include "ContainerListPage.h"
#include "ContainerDetailPanel.h"
#include "shared/widgets/SearchBar.h"
#include "shared/widgets/StatusBadge.h"
#include "shared/widgets/ConfirmDialog.h"
#include "shared/widgets/ToggleTable.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QMessageBox>

namespace Features::Containers {

// Column indices
enum Col { Name=0, Image, Status, Ports, Networks, Created, Id, Actions };

ContainerListPage::ContainerListPage(ContainerListViewModel *vm, QWidget *parent)
    : QWidget(parent), m_vm(vm)
{
    setupUi();

    connect(m_vm, &ContainerListViewModel::containersChanged, this, &ContainerListPage::onContainersChanged);
    // Don't switch to loading page on auto-refresh — only on first load (stack still on index 0)
    connect(m_vm, &ContainerListViewModel::loadingChanged, this, [this](bool loading) {
        if (loading && m_stack->currentIndex() == 0)
            m_loadingLabel->setText("Loading...");
    });
    connect(m_vm, &ContainerListViewModel::actionSucceeded, this, [this](const QString &msg) {
        m_statusLabel->setText(msg);
        m_statusLabel->setStyleSheet("color: #4caf50; font-size: 12px;");
    });
    connect(m_vm, &ContainerListViewModel::actionFailed, this, [this](const QString &msg) {
        QMessageBox::warning(this, "Error", msg);
    });

    m_vm->setAutoRefresh(true, 5000);
    m_vm->refresh();
}

void ContainerListPage::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Toolbar
    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(52);
    toolbar->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #333;");
    auto *tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(16, 8, 16, 8);

    auto *title = new QLabel("Containers");
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #e0e0e0;");

    m_search = new Shared::SearchBar("Search containers...");
    m_search->setMaximumWidth(280);

    auto *refreshBtn = new QPushButton("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #e0e0e0; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 12px; } "
        "QPushButton:hover { background-color: #444; }");

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");

    tbLayout->addWidget(title);
    tbLayout->addStretch();
    tbLayout->addWidget(m_statusLabel);
    tbLayout->addWidget(m_search);
    tbLayout->addWidget(refreshBtn);

    // Stack: loading / table
    m_stack = new QStackedWidget;

    m_loadingLabel = new QLabel("Loading...");
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setStyleSheet("color: #888; font-size: 14px;");

    m_table = new Shared::ToggleTable;
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({"Name", "Image", "Status", "Ports", "Networks", "Created", "ID", "Actions"});
    m_table->horizontalHeader()->setSectionResizeMode(Col::Name,     QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Image,    QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Status,   QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Ports,    QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Networks, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Created,  QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Id,       QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Actions,  QHeaderView::Fixed);
    m_table->setColumnWidth(Col::Status,  104);
    m_table->setColumnWidth(Col::Actions, 120);
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

    m_stack->addWidget(m_loadingLabel);
    m_stack->addWidget(m_table);
    m_stack->setCurrentIndex(0);

    root->addWidget(toolbar);
    root->addWidget(m_stack);

    connect(m_search, &QLineEdit::textChanged, m_vm, &ContainerListViewModel::setFilter);
    connect(refreshBtn, &QPushButton::clicked, m_vm, &ContainerListViewModel::refresh);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ContainerListPage::onRowDoubleClicked);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &ContainerListPage::onContextMenu);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ContainerListPage::onSelectionChanged);
    connect(m_table, &Shared::ToggleTable::selectionCleared,
            this, &ContainerListPage::selectionCleared);
}

// ── Smart diff update — no full rebuild ──────────────────────────────────────

void ContainerListPage::updateTable() {
    const auto &containers = m_vm->containers();
    const int newCount = containers.size();
    const int oldCount = m_table->rowCount();

    // Suspend layout/paint for the duration of the update
    m_table->setUpdatesEnabled(false);

    // Grow or shrink row count without touching existing rows
    if (newCount > oldCount)
        m_table->setRowCount(newCount);

    for (int i = 0; i < newCount; ++i) {
        const auto &c = containers[i];

        // Check if this row already shows the same container in the same state
        auto *nameItem = m_table->item(i, Col::Name);
        const QString existingId     = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
        const QString existingStatus = nameItem ? nameItem->data(Qt::UserRole + 1).toString() : QString();

        if (existingId == c.id && existingStatus == c.statusText) {
            // Row unchanged — skip repainting it entirely
            continue;
        }

        setRow(i, c);
    }

    // Remove surplus rows from the bottom
    if (newCount < oldCount)
        m_table->setRowCount(newCount);

    m_table->setUpdatesEnabled(true);

    m_statusLabel->setText(QString("%1 container%2")
        .arg(newCount).arg(newCount != 1 ? "s" : ""));
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");
}

void ContainerListPage::setRow(int row, const Core::Container &c) {
    m_table->setRowHeight(row, 38);

    auto ensureItem = [&](int col) -> QTableWidgetItem * {
        auto *it = m_table->item(row, col);
        if (!it) {
            it = new QTableWidgetItem;
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, col, it);
        }
        return it;
    };

    auto *nameItem = ensureItem(Col::Name);
    nameItem->setText(c.name);
    nameItem->setData(Qt::UserRole,     c.id);
    nameItem->setData(Qt::UserRole + 1, c.statusText); // used by diff check

    ensureItem(Col::Image)->setText(c.image);
    ensureItem(Col::Ports)->setText(formatPorts(c));
    ensureItem(Col::Networks)->setText(c.networks.join(", "));
    ensureItem(Col::Created)->setText(c.created.toString("yyyy-MM-dd hh:mm"));
    ensureItem(Col::Id)->setText(c.shortId);

    // Status badge — reuse widget if already there
    auto *existingBadgeHost = qobject_cast<QWidget *>(m_table->cellWidget(row, Col::Status));
    if (!existingBadgeHost) {
        auto *host = new QWidget;
        auto *hl   = new QHBoxLayout(host);
        hl->setContentsMargins(4, 0, 4, 0);
        auto *badge = new Shared::StatusBadge;
        hl->addWidget(badge);
        hl->addStretch();
        m_table->setCellWidget(row, Col::Status, host);
    }
    auto *badge = m_table->cellWidget(row, Col::Status)->findChild<Shared::StatusBadge *>();
    if (badge)
        badge->setStatus(Core::Container::statusToString(c.status), c.statusColor());

    // Action buttons — always rebuild since state may have changed
    m_table->setCellWidget(row, Col::Actions, makeActionWidget(c));
}

QWidget *ContainerListPage::makeActionWidget(const Core::Container &c) {
    auto *host = new QWidget;
    host->setStyleSheet("background: transparent;");
    auto *hl = new QHBoxLayout(host);
    hl->setContentsMargins(4, 2, 4, 2);
    hl->setSpacing(4);

    // Primary action button (Start/Stop)
    auto *primaryBtn = new QToolButton;
    primaryBtn->setFixedSize(28, 28);

    if (c.isRunning() && !c.isPaused()) {
        primaryBtn->setText("■");
        primaryBtn->setToolTip("Stop");
        primaryBtn->setStyleSheet(
            "QToolButton { background-color: #c62828; color: white; border-radius: 4px; font-size: 11px; }"
            "QToolButton:hover { background-color: #e53935; }");
        connect(primaryBtn, &QToolButton::clicked, this, [this, id = c.id]() {
            m_vm->stopContainer(id);
        });
    } else {
        primaryBtn->setText("▶");
        primaryBtn->setToolTip("Start");
        primaryBtn->setStyleSheet(
            "QToolButton { background-color: #2e7d32; color: white; border-radius: 4px; font-size: 11px; }"
            "QToolButton:hover { background-color: #388e3c; }");
        connect(primaryBtn, &QToolButton::clicked, this, [this, id = c.id]() {
            m_vm->startContainer(id);
        });
    }

    // "⋮" more-actions dropdown
    auto *moreBtn = new QToolButton;
    moreBtn->setText("⋮");
    moreBtn->setFixedSize(28, 28);
    moreBtn->setToolTip("More actions");
    moreBtn->setStyleSheet(
        "QToolButton { background-color: #333; color: #ccc; border-radius: 4px; font-size: 14px; }"
        "QToolButton:hover { background-color: #444; }");
    moreBtn->setPopupMode(QToolButton::InstantPopup);

    // Build the dropdown menu
    auto *menu = new QMenu(moreBtn);
    menu->setStyleSheet(
        "QMenu { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #444; }"
        "QMenu::item { padding: 6px 20px; }"
        "QMenu::item:selected { background-color: #1565c0; }"
        "QMenu::separator { height: 1px; background-color: #444; margin: 2px 0; }");

    if (c.isRunning()) {
        menu->addAction("■  Stop",    this, [this, id = c.id]() { m_vm->stopContainer(id);    });
        menu->addAction("↺  Restart", this, [this, id = c.id]() { m_vm->restartContainer(id); });
        if (c.isPaused())
            menu->addAction("▶  Unpause", this, [this, id = c.id]() { m_vm->unpauseContainer(id); });
        else
            menu->addAction("⏸  Pause",   this, [this, id = c.id]() { m_vm->pauseContainer(id);   });
    } else {
        menu->addAction("▶  Start", this, [this, id = c.id]() { m_vm->startContainer(id); });
    }

    menu->addSeparator();
    auto *removeAct = menu->addAction("🗑  Remove", this, [this, c]() {
        if (Shared::ConfirmDialog::confirm(this, "Remove Container",
            QString("Remove container <b>%1</b>?<br>This cannot be undone.").arg(c.name),
            "Remove", true))
        {
            m_vm->removeContainer(c.id, c.isRunning());
        }
    });
    removeAct->setEnabled(true);

    moreBtn->setMenu(menu);

    hl->addWidget(primaryBtn);
    hl->addWidget(moreBtn);
    hl->addStretch();
    return host;
}

// ── Slots ────────────────────────────────────────────────────────────────────

void ContainerListPage::onContainersChanged() {
    updateTable();
    m_stack->setCurrentIndex(1); // show table (no-op if already visible)
}

void ContainerListPage::onRowDoubleClicked(int row, int) {
    auto c = containerForRow(row);
    if (!c.id.isEmpty()) emit containerSelected(c);
}

void ContainerListPage::onSelectionChanged() {
    int row = m_table->currentRow();
    if (row < 0) return;
    auto c = containerForRow(row);
    if (!c.id.isEmpty()) emit containerSelected(c);
}

void ContainerListPage::onContextMenu(const QPoint &pos) {
    int row = m_table->rowAt(pos.y());
    if (row < 0) return;
    auto c = containerForRow(row);
    if (c.id.isEmpty()) return;
    showContextMenuForId(c.id, m_table->viewport()->mapToGlobal(pos));
}

void ContainerListPage::showContextMenuForId(const QString &containerId, const QPoint &globalPos) {
    const auto &containers = m_vm->containers();
    Core::Container c;
    for (const auto &ct : containers) {
        if (ct.id == containerId) { c = ct; break; }
    }
    if (c.id.isEmpty()) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #444; }"
        "QMenu::item { padding: 6px 20px; }"
        "QMenu::item:selected { background-color: #1565c0; }"
        "QMenu::separator { height: 1px; background-color: #444; margin: 2px 0; }");

    if (c.isRunning()) {
        menu.addAction("■  Stop",    this, [this, id = c.id]() { m_vm->stopContainer(id);    });
        menu.addAction("↺  Restart", this, [this, id = c.id]() { m_vm->restartContainer(id); });
        if (c.isPaused())
            menu.addAction("▶  Unpause", this, [this, id = c.id]() { m_vm->unpauseContainer(id); });
        else
            menu.addAction("⏸  Pause",   this, [this, id = c.id]() { m_vm->pauseContainer(id);   });
    } else {
        menu.addAction("▶  Start", this, [this, id = c.id]() { m_vm->startContainer(id); });
    }

    menu.addSeparator();
    menu.addAction("🗑  Remove", this, [this, c]() {
        if (Shared::ConfirmDialog::confirm(this, "Remove Container",
            QString("Remove container <b>%1</b>?<br>This cannot be undone.").arg(c.name),
            "Remove", true))
            m_vm->removeContainer(c.id, c.isRunning());
    });

    menu.exec(globalPos);
}

// ── Helpers ──────────────────────────────────────────────────────────────────

Core::Container ContainerListPage::containerForRow(int row) const {
    auto *item = m_table->item(row, Col::Name);
    if (!item) return {};
    const QString id = item->data(Qt::UserRole).toString();
    for (const auto &c : m_vm->containers())
        if (c.id == id) return c;
    return {};
}

QString ContainerListPage::formatPorts(const Core::Container &c) const {
    QStringList parts;
    for (const auto &p : c.ports) {
        if (p.hostPort > 0)
            parts.append(QString("%1→%2/%3").arg(p.hostPort).arg(p.containerPort).arg(p.protocol));
        else
            parts.append(QString("%1/%2").arg(p.containerPort).arg(p.protocol));
    }
    return parts.join(", ");
}

} // namespace Features::Containers
