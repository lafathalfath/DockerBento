#include "HubDetailPage.h"
#include "shared/utils/ByteFormatter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QToolButton>

namespace Features::Hub {

enum TagCol { TName=0, TDigest, TSize, TUpdated, TActions };

HubDetailPage::HubDetailPage(HubSearchViewModel *vm, QWidget *parent)
    : QWidget(parent), m_vm(vm)
{
    setupUi();

    connect(m_vm, &HubSearchViewModel::tagsChanged,       this, &HubDetailPage::refreshTagsTable);
    connect(m_vm, &HubSearchViewModel::tagsLoadingChanged, this, [this](bool loading) {
        m_tagsStatusLabel->setText(loading ? "Loading tags..." : "");
        if (loading) m_tagsStack->setCurrentIndex(0);
    });
}

void HubDetailPage::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Toolbar
    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(52);
    toolbar->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #333;");
    auto *tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(16, 8, 16, 8);

    auto *backBtn = new QPushButton("← Back");
    backBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #ccc; border-radius: 4px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #444; }");

    m_nameLabel = new QLabel;
    m_nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #e0e0e0;");

    m_tagsStatusLabel = new QLabel;
    m_tagsStatusLabel->setStyleSheet("color: #888; font-size: 12px;");

    tl->addWidget(backBtn);
    tl->addSpacing(12);
    tl->addWidget(m_nameLabel);
    tl->addStretch();
    tl->addWidget(m_tagsStatusLabel);

    // Info strip
    auto *infoStrip = new QWidget;
    infoStrip->setStyleSheet("background-color: #161616; border-bottom: 1px solid #2a2a2a;");
    auto *il = new QVBoxLayout(infoStrip);
    il->setContentsMargins(20, 12, 20, 12);
    il->setSpacing(6);

    m_descLabel = new QLabel;
    m_descLabel->setWordWrap(true);
    m_descLabel->setStyleSheet("color: #aaa; font-size: 12px;");

    m_statsLabel = new QLabel;
    m_statsLabel->setStyleSheet("color: #666; font-size: 11px;");

    il->addWidget(m_descLabel);
    il->addWidget(m_statsLabel);

    // Tags section header
    auto *tagsHeader = new QWidget;
    tagsHeader->setFixedHeight(36);
    tagsHeader->setStyleSheet("background-color: #1a1a1a; border-bottom: 1px solid #2a2a2a;");
    auto *thl = new QHBoxLayout(tagsHeader);
    thl->setContentsMargins(16, 0, 16, 0);

    auto *tagsTitle = new QLabel("Tags");
    tagsTitle->setStyleSheet("color: #aaa; font-size: 12px; font-weight: bold;");

    m_prevTagsBtn = new QPushButton("◀ Prev");
    m_nextTagsBtn = new QPushButton("Next ▶");
    for (auto *btn : {m_prevTagsBtn, m_nextTagsBtn}) {
        btn->setFixedHeight(24);
        btn->setStyleSheet(
            "QPushButton { background-color: #2a2a2a; color: #ccc; border-radius: 3px; padding: 0 8px; font-size: 11px; }"
            "QPushButton:hover { background-color: #333; }"
            "QPushButton:disabled { color: #444; background-color: #1a1a1a; }");
        btn->setEnabled(false);
    }
    m_tagsPagination = new QLabel;
    m_tagsPagination->setStyleSheet("color: #555; font-size: 10px;");

    thl->addWidget(tagsTitle);
    thl->addSpacing(12);
    thl->addWidget(m_tagsPagination);
    thl->addStretch();
    thl->addWidget(m_prevTagsBtn);
    thl->addWidget(m_nextTagsBtn);

    // Tags stack: loading label / table
    m_tagsStack = new QStackedWidget;

    auto *loadingLbl = new QLabel("Loading tags...");
    loadingLbl->setAlignment(Qt::AlignCenter);
    loadingLbl->setStyleSheet("color: #555; font-size: 13px;");
    m_tagsStack->addWidget(loadingLbl); // 0

    m_tagsTable = new Shared::ToggleTable;
    m_tagsTable->setColumnCount(5);
    m_tagsTable->setHorizontalHeaderLabels({"Tag", "Digest", "Size", "Updated", "Actions"});
    m_tagsTable->horizontalHeader()->setSectionResizeMode(TName,    QHeaderView::ResizeToContents);
    m_tagsTable->horizontalHeader()->setSectionResizeMode(TDigest,  QHeaderView::ResizeToContents);
    m_tagsTable->horizontalHeader()->setSectionResizeMode(TSize,    QHeaderView::ResizeToContents);
    m_tagsTable->horizontalHeader()->setSectionResizeMode(TUpdated, QHeaderView::ResizeToContents);
    m_tagsTable->horizontalHeader()->setSectionResizeMode(TActions, QHeaderView::Fixed);
    m_tagsTable->setColumnWidth(TActions, 110);
    m_tagsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tagsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tagsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tagsTable->setAlternatingRowColors(true);
    m_tagsTable->setShowGrid(false);
    m_tagsTable->verticalHeader()->setVisible(false);
    m_tagsTable->setStyleSheet(
        "QTableWidget { background-color: #1a1a1a; color: #e0e0e0; border: none; }"
        "QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid #2a2a2a; }"
        "QTableWidget::item:selected          { background-color: #1565c0; }"
        "QTableWidget::item:selected:alternate { background-color: #1565c0; }"
        "QHeaderView::section { background-color: #252525; color: #aaa; border: none;"
        "  border-bottom: 1px solid #333; padding: 6px 8px; font-size: 12px; }"
        "QTableWidget::item:alternate { background-color: #1d1d1d; }");
    m_tagsStack->addWidget(m_tagsTable); // 1

    root->addWidget(toolbar);
    root->addWidget(infoStrip);
    root->addWidget(tagsHeader);
    root->addWidget(m_tagsStack, 1);

    connect(backBtn, &QPushButton::clicked, this, &HubDetailPage::backRequested);
    connect(m_prevTagsBtn, &QPushButton::clicked, this, [this]() {
        m_vm->prevTagsPage(m_image.name);
    });
    connect(m_nextTagsBtn, &QPushButton::clicked, this, [this]() {
        m_vm->nextTagsPage(m_image.name);
    });
}

void HubDetailPage::showImage(const HubImage &img) {
    m_image = img;
    m_nameLabel->setText(img.isOfficial ? img.name + "  ✔ Official" : img.name);

    QString desc = img.description.isEmpty() ? "No description available." : img.description;
    m_descLabel->setText(desc);

    QString pulls = img.pullCount > 1000000
        ? QString("%1M pulls").arg(img.pullCount / 1000000)
        : img.pullCount > 1000 ? QString("%1K pulls").arg(img.pullCount / 1000)
        : QString("%1 pulls").arg(img.pullCount);
    m_statsLabel->setText(QString("★ %1 stars  ·  ⬇ %2").arg(img.starCount).arg(pulls));

    m_tagsTable->setRowCount(0);
    m_tagsStack->setCurrentIndex(0);
    loadTags();
}

void HubDetailPage::loadTags() {
    m_vm->fetchTags(m_image.name, 1);
}

void HubDetailPage::refreshTagsTable() {
    const auto &tags = m_vm->tags();
    m_tagsTable->setUpdatesEnabled(false);
    m_tagsTable->setRowCount(tags.size());

    for (int i = 0; i < tags.size(); ++i) {
        const auto &t = tags[i];
        m_tagsTable->setRowHeight(i, 36);

        auto ensureItem = [&](int col) -> QTableWidgetItem * {
            auto *it = m_tagsTable->item(i, col);
            if (!it) {
                it = new QTableWidgetItem;
                it->setFlags(it->flags() & ~Qt::ItemIsEditable);
                m_tagsTable->setItem(i, col, it);
            }
            return it;
        };

        ensureItem(TName)->setText(t.name);
        ensureItem(TDigest)->setText(t.digest.isEmpty() ? "—" : t.digest + "...");
        ensureItem(TSize)->setText(t.fullSize > 0 ? Shared::ByteFormatter::format(t.fullSize) : "—");
        ensureItem(TUpdated)->setText(t.lastUpdated);

        // Pull button
        auto *host = new QWidget;
        host->setStyleSheet("background: transparent;");
        auto *hl = new QHBoxLayout(host);
        hl->setContentsMargins(4, 2, 4, 2);

        auto *pullBtn = new QToolButton;
        pullBtn->setText("⬇ Pull");
        pullBtn->setFixedHeight(26);
        pullBtn->setStyleSheet(
            "QToolButton { background-color: #1565c0; color: white; border-radius: 4px;"
            "  font-size: 11px; padding: 0 8px; }"
            "QToolButton:hover { background-color: #1976d2; }");
        connect(pullBtn, &QToolButton::clicked, this, [this, tag = t.name]() {
            emit pullRequested(m_image.name + ":" + tag);
        });

        hl->addWidget(pullBtn);
        hl->addStretch();
        m_tagsTable->setCellWidget(i, TActions, host);
    }

    m_tagsTable->setUpdatesEnabled(true);

    // Pagination
    int page  = m_vm->tagsPage();
    int total = m_vm->totalTags();
    int start = (page - 1) * 25 + 1;
    int end   = qMin(page * 25, total);
    m_tagsPagination->setText(total > 0
        ? QString("%1–%2 of %3").arg(start).arg(end).arg(total)
        : "");
    m_prevTagsBtn->setEnabled(m_vm->hasPrevTagsPage());
    m_nextTagsBtn->setEnabled(m_vm->hasNextTagsPage());

    m_tagsStack->setCurrentIndex(1);
}

} // namespace Features::Hub
