#pragma once
#include <QObject>
#include <QList>
#include "features/hub/model/HubRepository.h"

namespace Core { class DockerClient; }
namespace Features::Images { class ImageRepository; }

namespace Features::Hub {

class HubSearchViewModel : public QObject {
    Q_OBJECT
public:
    explicit HubSearchViewModel(HubRepository *repo,
                                Features::Images::ImageRepository *imageRepo,
                                QObject *parent = nullptr);

    const QList<HubImage> &results()  const { return m_results; }
    const QList<HubImage> &popular()  const { return m_popular; }
    const QList<HubTag>   &tags()     const { return m_tags; }
    int  totalResults() const { return m_total; }
    int  totalTags()    const { return m_totalTags; }
    int  currentPage()  const { return m_page; }
    int  tagsPage()     const { return m_tagsPage; }
    bool hasNextPage()  const { return m_page * 25 < m_total; }
    bool hasPrevPage()  const { return m_page > 1; }
    bool hasNextTagsPage() const { return m_tagsPage * 25 < m_totalTags; }
    bool hasPrevTagsPage() const { return m_tagsPage > 1; }

    void loadPopular();
    void search(const QString &query);
    void nextPage();
    void prevPage();
    void fetchTags(const QString &imageName, int page = 1);
    void nextTagsPage(const QString &imageName);
    void prevTagsPage(const QString &imageName);
    void pullImage(const QString &name);

signals:
    void popularChanged();
    void resultsChanged();
    void tagsChanged();
    void loadingChanged(bool loading);
    void tagsLoadingChanged(bool loading);
    void pullProgress(const QString &status);
    void pullFinished(const QString &name);
    void actionFailed(const QString &error);

private:
    void doSearch(int page);

    HubRepository                    *m_repo;
    Features::Images::ImageRepository *m_imageRepo;
    QList<HubImage>                   m_popular;
    QList<HubImage>                   m_results;
    QList<HubTag>                     m_tags;
    QString                           m_query;
    int                               m_page{1};
    int                               m_total{0};
    int                               m_tagsPage{1};
    int                               m_totalTags{0};
};

} // namespace Features::Hub
