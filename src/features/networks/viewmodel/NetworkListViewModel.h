#pragma once
#include <QObject>
#include "core/types/Network.h"
#include "features/networks/model/NetworkRepository.h"

namespace Features::Networks {

class NetworkListViewModel : public QObject {
    Q_OBJECT
public:
    explicit NetworkListViewModel(NetworkRepository *repo, QObject *parent = nullptr);

    void refresh();
    void setFilter(const QString &text);
    void removeNetwork(const QString &id);
    void pruneNetworks();

    const QList<Core::Network> &networks() const { return m_filtered; }
    bool isLoading() const { return m_loading; }

signals:
    void networksChanged();
    void loadingChanged(bool loading);
    void actionSucceeded(const QString &message);
    void actionFailed(const QString &message);
    void errorOccurred(const QString &message);

private:
    void applyFilter();

    NetworkRepository *m_repo;
    QList<Core::Network> m_all;
    QList<Core::Network> m_filtered;
    QString m_filterText;
    bool m_loading{false};
};

} // namespace Features::Networks
