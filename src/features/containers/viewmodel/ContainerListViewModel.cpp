#include "ContainerListViewModel.h"

namespace Features::Containers {

ContainerListViewModel::ContainerListViewModel(ContainerRepository *repo, QObject *parent)
    : QObject(parent), m_repo(repo)
{
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &ContainerListViewModel::refresh);
}

void ContainerListViewModel::refresh() {
    if (m_loading) return;
    setLoading(true);
    m_repo->fetchAll([this](bool ok, QList<Core::Container> containers, QString err) {
        setLoading(false);
        if (!ok) {
            m_error = err;
            emit errorOccurred(err);
            return;
        }
        m_error.clear();
        m_all = containers;
        applyFilter();
    });
}

void ContainerListViewModel::setFilter(const QString &text) {
    m_filterText = text.toLower();
    applyFilter();
}

void ContainerListViewModel::setAutoRefresh(bool enabled, int intervalMs) {
    if (enabled)
        m_refreshTimer->start(intervalMs);
    else
        m_refreshTimer->stop();
}

void ContainerListViewModel::applyFilter() {
    if (m_filterText.isEmpty()) {
        m_filtered = m_all;
    } else {
        m_filtered.clear();
        for (const auto &c : m_all) {
            if (c.name.toLower().contains(m_filterText) ||
                c.image.toLower().contains(m_filterText) ||
                c.shortId.contains(m_filterText) ||
                c.statusText.toLower().contains(m_filterText))
            {
                m_filtered.append(c);
            }
        }
    }
    emit containersChanged();
}

void ContainerListViewModel::setLoading(bool v) {
    if (m_loading == v) return;
    m_loading = v;
    emit loadingChanged(v);
}

void ContainerListViewModel::startContainer(const QString &id) {
    m_repo->start(id, [this, id](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Container started"); refresh(); }
        else    emit actionFailed("Failed to start: " + err);
    });
}

void ContainerListViewModel::stopContainer(const QString &id) {
    m_repo->stop(id, [this, id](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Container stopped"); refresh(); }
        else    emit actionFailed("Failed to stop: " + err);
    });
}

void ContainerListViewModel::restartContainer(const QString &id) {
    m_repo->restart(id, [this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Container restarted"); refresh(); }
        else    emit actionFailed("Failed to restart: " + err);
    });
}

void ContainerListViewModel::pauseContainer(const QString &id) {
    m_repo->pause(id, [this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Container paused"); refresh(); }
        else    emit actionFailed("Failed to pause: " + err);
    });
}

void ContainerListViewModel::unpauseContainer(const QString &id) {
    m_repo->unpause(id, [this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Container unpaused"); refresh(); }
        else    emit actionFailed("Failed to unpause: " + err);
    });
}

void ContainerListViewModel::removeContainer(const QString &id, bool force) {
    m_repo->remove(id, force, [this](bool ok, QString err) {
        if (ok) { emit actionSucceeded("Container removed"); refresh(); }
        else    emit actionFailed("Failed to remove: " + err);
    });
}

} // namespace Features::Containers
