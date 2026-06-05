#include "ImageListPage.h"
#include "RunContainerDialog.h"
#include "shared/widgets/SearchBar.h"
#include "shared/widgets/ConfirmDialog.h"
#include "shared/utils/ByteFormatter.h"
#include "core/docker/DockerClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>

namespace Features::Images {

enum Col { Repository=0, Tag, Id, Size, Created, Containers, Actions };

ImageListPage::ImageListPage(ImageListViewModel *vm, Core::DockerClient *docker, QWidget *parent)
    : QWidget(parent), m_vm(vm), m_docker(docker)
{
    setupUi();
    connect(m_vm, &ImageListViewModel::imagesChanged, this, &ImageListPage::onImagesChanged);
    connect(m_vm, &ImageListViewModel::loadingChanged, this, [this](bool loading) {
        if (loading && m_stack->currentIndex() == 0)
            {}; // only show loading on first load
    });
    connect(m_vm, &ImageListViewModel::actionSucceeded, this, [this](const QString &msg) {
        m_statusLabel->setText(msg);
        m_statusLabel->setStyleSheet("color: #4caf50; font-size: 12px;");
    });
    connect(m_vm, &ImageListViewModel::actionFailed, this, [](const QString &msg) {
        QMessageBox::warning(nullptr, "Error", msg);
    });

    m_vm->refresh();
}

void ImageListPage::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(52);
    toolbar->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #333;");
    auto *tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(16, 8, 16, 8);

    auto *title = new QLabel("Images");
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #e0e0e0;");
    m_search = new Shared::SearchBar("Search images...");
    m_search->setMaximumWidth(280);

    auto *pruneBtn = new QPushButton("Prune Unused");
    pruneBtn->setStyleSheet(
        "QPushButton { background-color: #7b1fa2; color: white; border-radius: 4px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #9c27b0; }");

    auto *refreshBtn = new QPushButton("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #e0e0e0; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #444; }");

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
    m_table->setHorizontalHeaderLabels({"Repository", "Tag", "ID", "Size", "Created", "Containers", "Actions"});
    m_table->horizontalHeader()->setSectionResizeMode(Col::Repository, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Tag,        QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Id,         QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Size,       QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Created,    QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Containers, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Col::Actions,    QHeaderView::Fixed);
    m_table->setColumnWidth(Col::Actions, 100);
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
        "QTableWidget::item:alternate { background-color: #1d1d1d; }");

    m_stack->addWidget(loading);
    m_stack->addWidget(m_table);

    root->addWidget(toolbar);
    root->addWidget(m_stack);

    connect(m_search, &QLineEdit::textChanged, m_vm, &ImageListViewModel::setFilter);
    connect(refreshBtn, &QPushButton::clicked, m_vm, &ImageListViewModel::refresh);
    connect(pruneBtn, &QPushButton::clicked, this, [this]() {
        if (Shared::ConfirmDialog::confirm(this, "Prune Images",
            "Remove all dangling (unused) images?", "Prune", true))
            m_vm->pruneImages();
    });
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &ImageListPage::onContextMenu);
}

Core::Image ImageListPage::imageForRow(int row) const {
    auto *item = m_table->item(row, Col::Repository);
    if (!item) return {};
    QString id = item->data(Qt::UserRole).toString();
    for (const auto &img : m_vm->images())
        if (img.id == id) return img;
    return {};
}

void ImageListPage::runContainerFor(int row) {
    auto img = imageForRow(row);
    if (img.id.isEmpty()) return;

    auto *dlg = new RunContainerDialog(img, m_docker, this);
    connect(dlg, &QDialog::accepted, this, [this]() {
        emit containerCreated();
    });
    dlg->exec();
    dlg->deleteLater();
}

void ImageListPage::onImagesChanged() {
    const auto &images = m_vm->images();
    m_table->setUpdatesEnabled(false);
    m_table->setRowCount(images.size());

    for (int i = 0; i < images.size(); ++i) {
        const auto &img = images[i];
        m_table->setRowHeight(i, 36);

        auto ensureItem = [&](int col) -> QTableWidgetItem * {
            auto *it = m_table->item(i, col);
            if (!it) {
                it = new QTableWidgetItem;
                it->setFlags(it->flags() & ~Qt::ItemIsEditable);
                m_table->setItem(i, col, it);
            }
            return it;
        };

        auto *repoItem = ensureItem(Col::Repository);
        repoItem->setText(img.repository());
        repoItem->setData(Qt::UserRole, img.id);

        ensureItem(Col::Tag)->setText(img.tag());
        ensureItem(Col::Id)->setText(img.shortId);
        ensureItem(Col::Size)->setText(Shared::ByteFormatter::format(img.size));
        ensureItem(Col::Created)->setText(img.created.toString("yyyy-MM-dd"));
        ensureItem(Col::Containers)->setText(img.containers >= 0 ? QString::number(img.containers) : "-");

        // Action buttons
        auto *host = new QWidget;
        host->setStyleSheet("background: transparent;");
        auto *hl = new QHBoxLayout(host);
        hl->setContentsMargins(4, 2, 4, 2);
        hl->setSpacing(4);

        auto *runBtn = new QToolButton;
        runBtn->setText("▶ Run");
        runBtn->setFixedHeight(26);
        runBtn->setToolTip("Run container from this image");
        runBtn->setStyleSheet(
            "QToolButton { background-color: #2e7d32; color: white; border-radius: 4px;"
            "  font-size: 11px; padding: 0 8px; }"
            "QToolButton:hover { background-color: #388e3c; }");
        connect(runBtn, &QToolButton::clicked, this, [this, i]() { runContainerFor(i); });

        auto *moreBtn = new QToolButton;
        moreBtn->setText("⋮");
        moreBtn->setFixedSize(26, 26);
        moreBtn->setStyleSheet(
            "QToolButton { background-color: #333; color: #ccc; border-radius: 4px; font-size: 14px; }"
            "QToolButton:hover { background-color: #444; }");
        moreBtn->setPopupMode(QToolButton::InstantPopup);

        auto *menu = new QMenu(moreBtn);
        menu->setStyleSheet(
            "QMenu { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #444; }"
            "QMenu::item { padding: 6px 20px; }"
            "QMenu::item:selected { background-color: #1565c0; }"
            "QMenu::separator { height: 1px; background-color: #444; margin: 2px 0; }");

        menu->addAction("▶  Run Container", this, [this, i]() { runContainerFor(i); });
        menu->addSeparator();
        menu->addAction("🗑  Remove", this, [this, i]() {
            auto img = imageForRow(i);
            if (img.id.isEmpty()) return;
            if (Shared::ConfirmDialog::confirm(this, "Remove Image",
                QString("Remove image <b>%1</b>?").arg(img.displayName()), "Remove", true))
                m_vm->removeImage(img.id);
        });
        menu->addAction("🗑  Force Remove", this, [this, i]() {
            auto img = imageForRow(i);
            if (img.id.isEmpty()) return;
            if (Shared::ConfirmDialog::confirm(this, "Force Remove Image",
                QString("Force remove <b>%1</b>? Containers using it may break.").arg(img.displayName()),
                "Force Remove", true))
                m_vm->removeImage(img.id, true);
        });
        moreBtn->setMenu(menu);

        hl->addWidget(runBtn);
        hl->addWidget(moreBtn);
        hl->addStretch();
        m_table->setCellWidget(i, Col::Actions, host);
    }

    m_table->setUpdatesEnabled(true);

    qint64 total = 0;
    for (const auto &img : images) total += img.size;
    m_statusLabel->setText(QString("%1 image%2 · %3 total")
        .arg(images.size()).arg(images.size() != 1 ? "s" : "")
        .arg(Shared::ByteFormatter::format(total)));
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");
    m_stack->setCurrentIndex(1);
}

void ImageListPage::onContextMenu(const QPoint &pos) {
    int row = m_table->rowAt(pos.y());
    if (row < 0) return;
    auto img = imageForRow(row);
    if (img.id.isEmpty()) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #2a2a2a; color: #e0e0e0; border: 1px solid #444; }"
        "QMenu::item { padding: 6px 20px; }"
        "QMenu::item:selected { background-color: #1565c0; }"
        "QMenu::separator { height: 1px; background-color: #444; margin: 2px 0; }");

    menu.addAction("▶  Run Container", this, [this, row]() { runContainerFor(row); });
    menu.addSeparator();
    menu.addAction("🗑  Remove", this, [this, img]() {
        if (Shared::ConfirmDialog::confirm(this, "Remove Image",
            QString("Remove image <b>%1</b>?").arg(img.displayName()), "Remove", true))
            m_vm->removeImage(img.id);
    });
    menu.addAction("🗑  Force Remove", this, [this, img]() {
        if (Shared::ConfirmDialog::confirm(this, "Force Remove Image",
            QString("Force remove <b>%1</b>? Containers using it may break.").arg(img.displayName()),
            "Force Remove", true))
            m_vm->removeImage(img.id, true);
    });

    menu.exec(m_table->viewport()->mapToGlobal(pos));
}

} // namespace Features::Images
