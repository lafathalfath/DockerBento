#include "NetworkListViewModel.h"

namespace Features::Networks {

NetworkListViewModel::NetworkListViewModel(NetworkRepository *repo, QObject *parent)
    : QObject(parent), m_repo(repo)
{}

void NetworkListViewModel::refresh() {
    if (m_loading) return;
    m_loading = true;
    emit loadingChanged(true);
    m_repo->fetchAll([this](bool ok, QList<Core::Network> networks, QString err) {
        m_loading = false;
        emit loadingChanged(false);
        if (!ok) { emit errorOccurred(err); return; }
        m_all = networks;
        applyFilter();
    });
}

void NetworkListViewModel::setFilter(const QString &text) {
    m_filterText = text.toLower();
    applyFilter();
}

void NetworkListViewModel::applyFilter() {
    if (m_filterText.isEmpty()) {
        m_filtered = m_all;
    } else {
        m_filtered.clear();
        for (const auto &n : m_all) {
            if (n.name.toLower().contains(m_filterText) ||
                n.driver.toLower().contains(m_filterText) ||
                n.shortId.contains(m_filterText))
                m_filtered.append(n);
        }
    }
    emit networksChanged();
}

void NetworkListViewModel::removeNetwork(const QString &id) {
    m_repo->remove(id, [this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Network removed"); refresh(); }
        else    emit actionFailed("Failed to remove: " + err);
    });
}

void NetworkListViewModel::pruneNetworks() {
    m_repo->prune([this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Unused networks pruned"); refresh(); }
        else    emit actionFailed("Failed to prune: " + err);
    });
}

} // namespace Features::Networks
