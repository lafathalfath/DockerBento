#pragma once
#include <QObject>
#include <QTimer>
#include "core/types/Container.h"
#include "features/containers/model/ContainerRepository.h"

namespace Features::Containers {

class ContainerListViewModel : public QObject {
    Q_OBJECT
public:
    explicit ContainerListViewModel(ContainerRepository *repo, QObject *parent = nullptr);

    void refresh();
    void setFilter(const QString &text);
    void setAutoRefresh(bool enabled, int intervalMs = 5000);

    const QList<Core::Container> &containers() const { return m_filtered; }
    bool isLoading() const { return m_loading; }
    QString errorMessage() const { return m_error; }

    void startContainer(const QString &id);
    void stopContainer(const QString &id);
    void restartContainer(const QString &id);
    void pauseContainer(const QString &id);
    void unpauseContainer(const QString &id);
    void removeContainer(const QString &id, bool force = false);

signals:
    void containersChanged();
    void loadingChanged(bool loading);
    void errorOccurred(const QString &message);
    void actionSucceeded(const QString &message);
    void actionFailed(const QString &message);

private:
    void applyFilter();
    void setLoading(bool v);

    ContainerRepository *m_repo;
    QList<Core::Container> m_all;
    QList<Core::Container> m_filtered;
    QString m_filterText;
    bool m_loading{false};
    QString m_error;
    QTimer *m_refreshTimer;
};

} // namespace Features::Containers
