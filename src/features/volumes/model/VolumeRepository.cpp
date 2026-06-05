#include "VolumeRepository.h"
#include "core/docker/DockerClient.h"
#include <QJsonObject>
#include <QJsonArray>

namespace Features::Volumes {

VolumeRepository::VolumeRepository(Core::DockerClient *client, QObject *parent)
    : QObject(parent), m_client(client)
{}

void VolumeRepository::fetchAll(VolumeListCallback callback) {
    m_client->get("/v1.41/volumes", [callback](bool ok, const QJsonDocument &doc, const QString &err) {
        if (!ok) { callback(false, {}, err); return; }
        VolumeList list;
        QJsonArray arr = doc.object()["Volumes"].toArray();
        for (const auto &v : arr)
            list.append(Core::Volume::fromJson(v.toObject()));
        callback(true, list, {});
    });
}

void VolumeRepository::remove(const QString &name, ActionCallback callback) {
    m_client->del(QString("/v1.41/volumes/%1").arg(name), [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void VolumeRepository::prune(ActionCallback callback) {
    m_client->post("/v1.41/volumes/prune", {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

} // namespace Features::Volumes
