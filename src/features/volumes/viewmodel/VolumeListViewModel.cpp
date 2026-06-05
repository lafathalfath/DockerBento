#include "VolumeListViewModel.h"

namespace Features::Volumes {

VolumeListViewModel::VolumeListViewModel(VolumeRepository *repo, QObject *parent)
    : QObject(parent), m_repo(repo)
{}

void VolumeListViewModel::refresh() {
    if (m_loading) return;
    m_loading = true;
    emit loadingChanged(true);
    m_repo->fetchAll([this](bool ok, QList<Core::Volume> volumes, QString err) {
        m_loading = false;
        emit loadingChanged(false);
        if (!ok) { emit errorOccurred(err); return; }
        m_all = volumes;
        applyFilter();
    });
}

void VolumeListViewModel::setFilter(const QString &text) {
    m_filterText = text.toLower();
    applyFilter();
}

void VolumeListViewModel::applyFilter() {
    if (m_filterText.isEmpty()) {
        m_filtered = m_all;
    } else {
        m_filtered.clear();
        for (const auto &v : m_all) {
            if (v.name.toLower().contains(m_filterText) ||
                v.driver.toLower().contains(m_filterText))
                m_filtered.append(v);
        }
    }
    emit volumesChanged();
}

void VolumeListViewModel::removeVolume(const QString &name) {
    m_repo->remove(name, [this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Volume removed"); refresh(); }
        else    emit actionFailed("Failed to remove: " + err);
    });
}

void VolumeListViewModel::pruneVolumes() {
    m_repo->prune([this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Unused volumes pruned"); refresh(); }
        else    emit actionFailed("Failed to prune: " + err);
    });
}

} // namespace Features::Volumes
