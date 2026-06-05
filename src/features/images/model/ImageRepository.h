#pragma once
#include <QObject>
#include <functional>
#include "core/types/Image.h"

namespace Core { class DockerClient; }

namespace Features::Images {

using ImageList = QList<Core::Image>;
using ImageListCallback = std::function<void(bool ok, ImageList images, QString error)>;
using ActionCallback = std::function<void(bool ok, QString error)>;

class ImageRepository : public QObject {
    Q_OBJECT
public:
    explicit ImageRepository(Core::DockerClient *client, QObject *parent = nullptr);

    void fetchAll(ImageListCallback callback);
    void remove(const QString &id, bool force, ActionCallback callback);
    void prune(ActionCallback callback);
    void pull(const QString &image, std::function<void(const QString &status, bool done)> callback);

private:
    Core::DockerClient *m_client;
};

} // namespace Features::Images
