#pragma once
#include <QObject>
#include <functional>
#include "core/types/Volume.h"

namespace Core { class DockerClient; }

namespace Features::Volumes {

using VolumeList = QList<Core::Volume>;
using VolumeListCallback = std::function<void(bool ok, VolumeList volumes, QString error)>;
using ActionCallback = std::function<void(bool ok, QString error)>;

class VolumeRepository : public QObject {
    Q_OBJECT
public:
    explicit VolumeRepository(Core::DockerClient *client, QObject *parent = nullptr);

    void fetchAll(VolumeListCallback callback);
    void remove(const QString &name, ActionCallback callback);
    void prune(ActionCallback callback);

private:
    Core::DockerClient *m_client;
};

} // namespace Features::Volumes
