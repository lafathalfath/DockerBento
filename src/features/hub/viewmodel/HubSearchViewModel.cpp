#include "HubSearchViewModel.h"
#include "features/images/model/ImageRepository.h"

namespace Features::Hub {

HubSearchViewModel::HubSearchViewModel(HubRepository *repo,
                                       Features::Images::ImageRepository *imageRepo,
                                       QObject *parent)
    : QObject(parent), m_repo(repo), m_imageRepo(imageRepo)
{}

void HubSearchViewModel::loadPopular() {
    emit loadingChanged(true);
    m_repo->fetchPopular([this](bool ok, QList<HubImage> list, const QString &err) {
        emit loadingChanged(false);
        if (!ok) { emit actionFailed(err); return; }
        m_popular = list;
        emit popularChanged();
    });
}

void HubSearchViewModel::search(const QString &query) {
    if (query.trimmed().isEmpty()) return;
    m_query = query.trimmed();
    m_page  = 1;
    doSearch(1);
}

void HubSearchViewModel::nextPage() {
    if (!hasNextPage()) return;
    doSearch(m_page + 1);
}

void HubSearchViewModel::prevPage() {
    if (!hasPrevPage()) return;
    doSearch(m_page - 1);
}

void HubSearchViewModel::fetchTags(const QString &imageName, int page) {
    m_tagsPage = page;
    emit tagsLoadingChanged(true);
    m_repo->fetchTags(imageName, page, [this](bool ok, QList<HubTag> tags, int total, const QString &err) {
        emit tagsLoadingChanged(false);
        if (!ok) { emit actionFailed(err); return; }
        m_tags      = tags;
        m_totalTags = total;
        emit tagsChanged();
    });
}

void HubSearchViewModel::nextTagsPage(const QString &imageName) {
    if (!hasNextTagsPage()) return;
    fetchTags(imageName, m_tagsPage + 1);
}

void HubSearchViewModel::prevTagsPage(const QString &imageName) {
    if (!hasPrevTagsPage()) return;
    fetchTags(imageName, m_tagsPage - 1);
}

void HubSearchViewModel::doSearch(int page) {
    emit loadingChanged(true);
    m_repo->search(m_query, page, [this, page](bool ok, QList<HubImage> results, int total, const QString &err) {
        emit loadingChanged(false);
        if (!ok) { emit actionFailed(err); return; }
        m_results = results;
        m_total   = total;
        m_page    = page;
        emit resultsChanged();
    });
}

void HubSearchViewModel::pullImage(const QString &name) {
    m_imageRepo->pull(name, [this, name](const QString &status, bool done) {
        emit pullProgress(status);
        if (done) emit pullFinished(name);
    });
}

} // namespace Features::Hub
