#include "VolumeListPage.h"
#include "shared/widgets/SearchBar.h"
#include "shared/widgets/ConfirmDialog.h"
#include "shared/utils/ByteFormatter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>

namespace Features::Volumes {

VolumeListPage::VolumeListPage(VolumeListViewModel *vm, QWidget *parent)
    : QWidget(parent), m_vm(vm)
{
    setupUi();
    connect(m_vm, &VolumeListViewModel::volumesChanged, this, &VolumeListPage::onVolumesChanged);
    connect(m_vm, &VolumeListViewModel::loadingChanged, this, [this](bool loading) {
        m_stack->setCurrentIndex(loading ? 0 : 1);
    });
    connect(m_vm, &VolumeListViewModel::actionSucceeded, this, [this](const QString &msg) {
        m_statusLabel->setText(msg);
        m_statusLabel->setStyleSheet("color: #4caf50; font-size: 12px;");
    });
    connect(m_vm, &VolumeListViewModel::actionFailed, this, [](const QString &msg) {
        QMessageBox::warning(nullptr, "Error", msg);
    });

    m_vm->refresh();
}

void VolumeListPage::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(52);
    toolbar->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #333;");
    auto *tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(16, 8, 16, 8);

    auto *title = new QLabel("Volumes");
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #e0e0e0;");

    m_search = new Shared::SearchBar("Search volumes...");
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
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Name", "Driver", "Mountpoint", "Scope", "Created"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
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

    connect(m_search, &QLineEdit::textChanged, m_vm, &VolumeListViewModel::setFilter);
    connect(refreshBtn, &QPushButton::clicked, m_vm, &VolumeListViewModel::refresh);
    connect(pruneBtn, &QPushButton::clicked, this, [this]() {
        if (Shared::ConfirmDialog::confirm(this, "Prune Volumes",
            "Remove all unused local volumes? Data will be lost.", "Prune", true))
            m_vm->pruneVolumes();
    });
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &VolumeListPage::onContextMenu);
}

void VolumeListPage::onVolumesChanged() {
    const auto &volumes = m_vm->volumes();
    m_table->setRowCount(volumes.size());

    for (int i = 0; i < volumes.size(); ++i) {
        const auto &v = volumes[i];
        m_table->setRowHeight(i, 36);

        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            return item;
        };

        m_table->setItem(i, 0, makeItem(v.name));
        m_table->setItem(i, 1, makeItem(v.driver));
        m_table->setItem(i, 2, makeItem(v.mountpoint));
        m_table->setItem(i, 3, makeItem(v.scope));
        m_table->setItem(i, 4, makeItem(v.created.isValid()
            ? v.created.toString("yyyy-MM-dd hh:mm") : "-"));

        m_table->item(i, 0)->setData(Qt::UserRole, v.name);
    }

    m_statusLabel->setText(QString("%1 volume%2").arg(volumes.size()).arg(volumes.size() != 1 ? "s" : ""));
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");
    m_stack->setCurrentIndex(1);
}

void VolumeListPage::onContextMenu(const QPoint &pos) {
    int row = m_table->rowAt(pos.y());
    if (row < 0) return;
    auto *item = m_table->item(row, 0);
    if (!item) return;
    QString name = item->data(Qt::UserRole).toString();

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #444; }"
        "QMenu::item { padding: 6px 20px; }"
        "QMenu::item:selected { background-color: #1565c0; }"
    );
    menu.addAction("Remove", [this, name]() {
        if (Shared::ConfirmDialog::confirm(this, "Remove Volume",
            QString("Remove volume <b>%1</b>?<br><b>All data will be permanently lost.</b>").arg(name),
            "Remove", true))
            m_vm->removeVolume(name);
    });

    menu.exec(m_table->viewport()->mapToGlobal(pos));
}

} // namespace Features::Volumes
