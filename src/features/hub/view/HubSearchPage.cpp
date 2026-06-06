#include "HubSearchPage.h"
#include "HubImageCard.h"
#include "HubDetailPage.h"
#include "shared/widgets/SearchBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QScrollBar>

namespace Features::Hub {

HubSearchPage::HubSearchPage(HubSearchViewModel *vm, QWidget *parent)
    : QWidget(parent), m_vm(vm)
{
    m_logoNam = new QNetworkAccessManager(this);
    setupUi();

    connect(m_vm, &HubSearchViewModel::popularChanged, this, [this]() {
        populateGrid(m_vm->popular(), m_gridContainer, true);
    });
    connect(m_vm, &HubSearchViewModel::resultsChanged, this, &HubSearchPage::onResultsChanged);
    connect(m_vm, &HubSearchViewModel::loadingChanged, this, [this](bool loading) {
        if (loading) {
            m_statusLabel->setText("Searching...");
            m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");
        }
    });
    connect(m_vm, &HubSearchViewModel::pullProgress, this, [this](const QString &status) {
        if (!status.trimmed().isEmpty()) {
            m_pullLog->append(status);
            m_pullLog->verticalScrollBar()->setValue(m_pullLog->verticalScrollBar()->maximum());
        }
    });
    connect(m_vm, &HubSearchViewModel::pullFinished, this, [this](const QString &name) {
        m_pullLog->append(QString("\n✅ Pull complete: %1").arg(name));
        m_pullLog->verticalScrollBar()->setValue(m_pullLog->verticalScrollBar()->maximum());
        m_statusLabel->setText(QString("Pulled %1").arg(name));
        m_statusLabel->setStyleSheet("color: #4caf50; font-size: 12px;");
        emit imagePulled();
    });
    connect(m_vm, &HubSearchViewModel::actionFailed, this, [](const QString &err) {
        QMessageBox::warning(nullptr, "Error", err);
    });

    m_vm->loadPopular();
}

void HubSearchPage::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Shared toolbar
    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(52);
    toolbar->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #333;");
    auto *tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(16, 8, 16, 8);

    auto *title = new QLabel("Hub Catalog");
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #e0e0e0;");

    m_search = new Shared::SearchBar("Search Docker Hub...");
    m_search->setMaximumWidth(320);
    m_search->setMinimumWidth(220);

    auto *searchBtn = new QPushButton("Search");
    searchBtn->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; border-radius: 4px; padding: 4px 14px; }"
        "QPushButton:hover { background-color: #1976d2; }");

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");

    tl->addWidget(title);
    tl->addStretch();
    tl->addWidget(m_statusLabel);
    tl->addSpacing(12);
    tl->addWidget(m_search);
    tl->addWidget(searchBtn);

    // Root stack holds all pages
    m_rootStack = new QStackedWidget;

    buildGridPage();
    buildResultsPage();

    // Detail page
    m_detailPage = new HubDetailPage(m_vm);
    connect(m_detailPage, &HubDetailPage::backRequested, this, [this]() {
        if (m_search->text().trimmed().isEmpty())
            m_rootStack->setCurrentIndex(GridPage);
        else
            m_rootStack->setCurrentIndex(ResultsPage);
    });
    connect(m_detailPage, &HubDetailPage::pullRequested, this, &HubSearchPage::doPull);
    m_rootStack->addWidget(m_detailPage); // DetailPage=2

    buildPullLogPage(); // PullLogPage=3

    root->addWidget(toolbar);
    root->addWidget(m_rootStack, 1);

    auto doSearch = [this]() {
        QString q = m_search->text().trimmed();
        if (q.isEmpty()) {
            m_rootStack->setCurrentIndex(GridPage);
            return;
        }
        m_vm->search(q);
    };
    connect(searchBtn, &QPushButton::clicked, this, doSearch);
    connect(m_search,  &QLineEdit::returnPressed, this, doSearch);
    connect(m_search,  &QLineEdit::textChanged, this, [this](const QString &t) {
        if (t.trimmed().isEmpty() && m_rootStack->currentIndex() == ResultsPage)
            m_rootStack->setCurrentIndex(GridPage);
    });
    connect(m_prevBtn, &QPushButton::clicked, this, [this]() { m_vm->prevPage(); });
    connect(m_nextBtn, &QPushButton::clicked, this, [this]() { m_vm->nextPage(); });
}

void HubSearchPage::buildGridPage() {
    auto *page = new QWidget;
    auto *vl   = new QVBoxLayout(page);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    // Section title bar
    auto *titleBar = new QWidget;
    titleBar->setFixedHeight(36);
    titleBar->setStyleSheet("background-color: #161616; border-bottom: 1px solid #2a2a2a;");
    auto *tbl = new QHBoxLayout(titleBar);
    tbl->setContentsMargins(20, 0, 20, 0);
    m_gridTitle = new QLabel("Popular Images");
    m_gridTitle->setStyleSheet("color: #888; font-size: 11px; font-weight: bold; letter-spacing: 1px;");
    tbl->addWidget(m_gridTitle);

    // Scroll area for the card grid
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet(
        "QScrollArea { border: none; background-color: #1a1a1a; }"
        "QScrollBar:vertical { background: #1a1a1a; width: 8px; }"
        "QScrollBar::handle:vertical { background: #333; border-radius: 4px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    m_gridContainer = new QWidget;
    m_gridContainer->setStyleSheet("background-color: #1a1a1a;");
    scroll->setWidget(m_gridContainer);

    vl->addWidget(titleBar);
    vl->addWidget(scroll, 1);

    m_rootStack->addWidget(page); // GridPage=0
}

void HubSearchPage::buildResultsPage() {
    auto *page = new QWidget;
    auto *vl   = new QVBoxLayout(page);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    // Pagination bar
    auto *paginationBar = new QWidget;
    paginationBar->setFixedHeight(36);
    paginationBar->setStyleSheet("background-color: #161616; border-bottom: 1px solid #2a2a2a;");
    auto *pl = new QHBoxLayout(paginationBar);
    pl->setContentsMargins(16, 0, 16, 0);

    m_prevBtn = new QPushButton("◀ Prev");
    m_nextBtn = new QPushButton("Next ▶");
    for (auto *btn : {m_prevBtn, m_nextBtn}) {
        btn->setFixedHeight(26);
        btn->setStyleSheet(
            "QPushButton { background-color: #2a2a2a; color: #ccc; border-radius: 4px; padding: 0 10px; font-size: 11px; }"
            "QPushButton:hover { background-color: #333; }"
            "QPushButton:disabled { color: #444; background-color: #1a1a1a; }");
        btn->setEnabled(false);
    }
    m_paginationLabel = new QLabel;
    m_paginationLabel->setStyleSheet("color: #666; font-size: 11px;");
    pl->addWidget(m_prevBtn);
    pl->addWidget(m_paginationLabel);
    pl->addStretch();
    pl->addWidget(m_nextBtn);

    // Scrollable grid for search results
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet(
        "QScrollArea { border: none; background-color: #1a1a1a; }"
        "QScrollBar:vertical { background: #1a1a1a; width: 8px; }"
        "QScrollBar::handle:vertical { background: #333; border-radius: 4px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

    m_resultsGridContainer = new QWidget;
    m_resultsGridContainer->setStyleSheet("background-color: #1a1a1a;");
    scroll->setWidget(m_resultsGridContainer);

    vl->addWidget(paginationBar);
    vl->addWidget(scroll, 1);

    m_rootStack->addWidget(page); // ResultsPage=1
}

void HubSearchPage::buildPullLogPage() {
    auto *page = new QWidget;
    auto *vl   = new QVBoxLayout(page);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    auto *header = new QWidget;
    header->setFixedHeight(36);
    header->setStyleSheet("background-color: #1e1e1e; border-bottom: 1px solid #333;");
    auto *hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 0, 12, 0);
    auto *hTitle = new QLabel("Pull Progress");
    hTitle->setStyleSheet("color: #e0e0e0; font-size: 13px; font-weight: bold;");
    auto *backBtn = new QPushButton("← Back to detail");
    backBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #888; border: none; font-size: 11px; }"
        "QPushButton:hover { color: #ccc; }");
    hl->addWidget(hTitle);
    hl->addStretch();
    hl->addWidget(backBtn);

    m_pullLog = new QTextEdit;
    m_pullLog->setReadOnly(true);
    m_pullLog->setStyleSheet(
        "QTextEdit { background-color: #0d0d0d; color: #aaffaa; font-family: monospace; "
        "font-size: 12px; border: none; padding: 12px; }");

    vl->addWidget(header);
    vl->addWidget(m_pullLog, 1);

    m_rootStack->addWidget(page); // PullLogPage=3

    connect(backBtn, &QPushButton::clicked, this, [this]() {
        m_rootStack->setCurrentIndex(DetailPage);
    });
}

void HubSearchPage::populateGrid(const QList<HubImage> &items, QWidget *container, bool isPopular) {
    // Clear old cards
    delete container->layout();
    const auto oldCards = container->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (auto *w : oldCards) w->deleteLater();

    m_gridTitle->setText(isPopular ? "Popular Images" : "Results");

    auto *grid = new QGridLayout(container);
    grid->setContentsMargins(20, 20, 20, 20);
    grid->setSpacing(12);

    const int cols = 4;
    for (int i = 0; i < items.size(); ++i) {
        auto *card = new HubImageCard(items[i], m_logoNam, container);
        connect(card, &HubImageCard::clicked, this, &HubSearchPage::showDetail);
        grid->addWidget(card, i / cols, i % cols);
    }

    // Fill remaining columns so cards stay left-aligned
    int lastRowItems = items.size() % cols;
    if (lastRowItems > 0) {
        for (int c = lastRowItems; c < cols; ++c) {
            auto *spacer = new QWidget;
            spacer->setFixedSize(220, 130);
            grid->addWidget(spacer, items.size() / cols, c);
        }
    }
    grid->setRowStretch(grid->rowCount(), 1);
}

void HubSearchPage::onResultsChanged() {
    const auto &results = m_vm->results();
    populateGrid(results, m_resultsGridContainer, false);

    int page  = m_vm->currentPage();
    int total = m_vm->totalResults();
    int start = (page - 1) * 25 + 1;
    int end   = qMin(page * 25, total);
    m_paginationLabel->setText(
        QString("%1–%2 of %3 results").arg(start).arg(end).arg(total));
    m_prevBtn->setEnabled(m_vm->hasPrevPage());
    m_nextBtn->setEnabled(m_vm->hasNextPage());

    m_statusLabel->setText(
        QString("%1 results for \"%2\"").arg(total).arg(m_search->text().trimmed()));
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");

    m_rootStack->setCurrentIndex(ResultsPage);
}

void HubSearchPage::showDetail(const HubImage &img) {
    m_detailPage->showImage(img);
    m_rootStack->setCurrentIndex(DetailPage);
}

void HubSearchPage::doPull(const QString &imageWithTag) {
    m_pullLog->clear();
    m_pullLog->append(QString("Pulling %1 ...\n").arg(imageWithTag));
    m_rootStack->setCurrentIndex(PullLogPage);
    m_vm->pullImage(imageWithTag);
}

} // namespace Features::Hub
