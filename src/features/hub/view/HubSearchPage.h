#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QScrollArea>
#include <QTextEdit>
#include <QNetworkAccessManager>
#include "features/hub/model/HubRepository.h"
#include "features/hub/viewmodel/HubSearchViewModel.h"

namespace Shared { class SearchBar; }

namespace Features::Hub {

class HubDetailPage;

class HubSearchPage : public QWidget {
    Q_OBJECT
public:
    explicit HubSearchPage(HubSearchViewModel *vm, QWidget *parent = nullptr);

signals:
    void imagePulled();

private:
    void setupUi();
    void buildGridPage();
    void buildResultsPage();
    void buildPullLogPage();

    void populateGrid(const QList<HubImage> &items, QWidget *container, bool isPopular);
    void onResultsChanged();
    void showDetail(const HubImage &img);
    void doPull(const QString &imageWithTag);

    // page indices in m_rootStack
    enum Page { GridPage=0, ResultsPage=1, DetailPage=2, PullLogPage=3 };

    HubSearchViewModel  *m_vm;
    Shared::SearchBar   *m_search{nullptr};
    QLabel              *m_statusLabel{nullptr};
    QStackedWidget      *m_rootStack{nullptr};

    // Grid page
    QWidget             *m_gridContainer{nullptr};
    QLabel              *m_gridTitle{nullptr};

    // Results page
    QWidget             *m_resultsWidget{nullptr};
    QWidget             *m_resultsGridContainer{nullptr};
    QPushButton         *m_prevBtn{nullptr};
    QPushButton         *m_nextBtn{nullptr};
    QLabel              *m_paginationLabel{nullptr};

    // Detail page
    HubDetailPage          *m_detailPage{nullptr};

    // Pull log page
    QTextEdit              *m_pullLog{nullptr};

    // Shared NAM for logo downloads — owned by this widget
    QNetworkAccessManager  *m_logoNam{nullptr};
};

} // namespace Features::Hub
