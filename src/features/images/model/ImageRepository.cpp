#include "ImageRepository.h"
#include "core/docker/DockerClient.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

namespace Features::Images {

ImageRepository::ImageRepository(Core::DockerClient *client, QObject *parent)
    : QObject(parent), m_client(client)
{}

void ImageRepository::fetchAll(ImageListCallback callback) {
    m_client->get("/v1.41/images/json?all=false", [callback](bool ok, const QJsonDocument &doc, const QString &err) {
        if (!ok) { callback(false, {}, err); return; }
        ImageList list;
        for (const auto &val : doc.array())
            list.append(Core::Image::fromJson(val.toObject()));
        callback(true, list, {});
    });
}

void ImageRepository::remove(const QString &id, bool force, ActionCallback callback) {
    QString path = QString("/v1.41/images/%1").arg(id);
    if (force) path += "?force=true";
    m_client->del(path, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ImageRepository::prune(ActionCallback callback) {
    m_client->post("/v1.41/images/prune", {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ImageRepository::pull(const QString &image, std::function<void(const QString &, bool)> callback) {
    // Split "name:tag" into separate query params; percent-encode both.
    QString name = image;
    QString tag  = "latest";
    int colon = image.lastIndexOf(':');
    if (colon > 0) {
        name = image.left(colon);
        tag  = image.mid(colon + 1);
    }
    QString path = QString("/v1.41/images/create?fromImage=%1&tag=%2")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(name)))
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(tag)));
    m_client->streamPost(path, {}, callback);
}

} // namespace Features::Images
