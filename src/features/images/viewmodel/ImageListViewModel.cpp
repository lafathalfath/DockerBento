#include "ImageListViewModel.h"

namespace Features::Images {

ImageListViewModel::ImageListViewModel(ImageRepository *repo, QObject *parent)
    : QObject(parent), m_repo(repo)
{}

void ImageListViewModel::refresh() {
    if (m_loading) return;
    m_loading = true;
    emit loadingChanged(true);
    m_repo->fetchAll([this](bool ok, QList<Core::Image> images, QString err) {
        m_loading = false;
        emit loadingChanged(false);
        if (!ok) { emit errorOccurred(err); return; }
        m_all = images;
        applyFilter();
    });
}

void ImageListViewModel::setFilter(const QString &text) {
    m_filterText = text.toLower();
    applyFilter();
}

void ImageListViewModel::applyFilter() {
    if (m_filterText.isEmpty()) {
        m_filtered = m_all;
    } else {
        m_filtered.clear();
        for (const auto &img : m_all) {
            bool match = img.shortId.contains(m_filterText);
            for (const auto &tag : img.repoTags)
                match |= tag.toLower().contains(m_filterText);
            if (match) m_filtered.append(img);
        }
    }
    emit imagesChanged();
}

void ImageListViewModel::removeImage(const QString &id, bool force) {
    m_repo->remove(id, force, [this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Image removed"); refresh(); }
        else    emit actionFailed("Failed to remove: " + err);
    });
}

void ImageListViewModel::pruneImages() {
    m_repo->prune([this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Unused images pruned"); refresh(); }
        else    emit actionFailed("Failed to prune: " + err);
    });
}

} // namespace Features::Images
