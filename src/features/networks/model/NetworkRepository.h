#pragma once
#include <QObject>
#include <functional>
#include "core/types/Network.h"

namespace Core { class DockerClient; }

namespace Features::Networks {

using NetworkList = QList<Core::Network>;
using NetworkListCallback = std::function<void(bool ok, NetworkList networks, QString error)>;
using ActionCallback = std::function<void(bool ok, QString error)>;

class NetworkRepository : public QObject {
    Q_OBJECT
public:
    explicit NetworkRepository(Core::DockerClient *client, QObject *parent = nullptr);

    void fetchAll(NetworkListCallback callback);
    void remove(const QString &id, ActionCallback callback);
    void prune(ActionCallback callback);

private:
    Core::DockerClient *m_client;
};

} // namespace Features::Networks
