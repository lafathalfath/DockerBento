#include "NetworkRepository.h"
#include "core/docker/DockerClient.h"
#include <QJsonArray>

namespace Features::Networks {

NetworkRepository::NetworkRepository(Core::DockerClient *client, QObject *parent)
    : QObject(parent), m_client(client)
{}

void NetworkRepository::fetchAll(NetworkListCallback callback) {
    m_client->get("/v1.41/networks", [callback](bool ok, const QJsonDocument &doc, const QString &err) {
        if (!ok) { callback(false, {}, err); return; }
        NetworkList list;
        for (const auto &val : doc.array())
            list.append(Core::Network::fromJson(val.toObject()));
        callback(true, list, {});
    });
}

void NetworkRepository::remove(const QString &id, ActionCallback callback) {
    m_client->del(QString("/v1.41/networks/%1").arg(id), [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void NetworkRepository::prune(ActionCallback callback) {
    m_client->post("/v1.41/networks/prune", {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

} // namespace Features::Networks
