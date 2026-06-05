#pragma once
#include <QObject>
#include <functional>
#include "core/types/Container.h"

namespace Core { class DockerClient; }

namespace Features::Containers {

using ContainerList = QList<Core::Container>;
using ContainerCallback = std::function<void(bool ok, ContainerList containers, QString error)>;
using ActionCallback = std::function<void(bool ok, QString error)>;
using LogsCallback = std::function<void(const QString &chunk, bool done)>;

class ContainerRepository : public QObject {
    Q_OBJECT
public:
    explicit ContainerRepository(Core::DockerClient *client, QObject *parent = nullptr);

    void fetchAll(ContainerCallback callback);
    void fetchInspect(const QString &id, std::function<void(bool, QJsonObject, QString)> callback);

    void start(const QString &id, ActionCallback callback);
    void stop(const QString &id, ActionCallback callback);
    void restart(const QString &id, ActionCallback callback);
    void pause(const QString &id, ActionCallback callback);
    void unpause(const QString &id, ActionCallback callback);
    void remove(const QString &id, bool force, ActionCallback callback);
    void kill(const QString &id, const QString &signal, ActionCallback callback);

    void getLogs(const QString &id, int tail, LogsCallback callback);
    void streamLogs(const QString &id, LogsCallback callback);

    void execCreate(const QString &containerId, const QString &shell,
                    std::function<void(bool ok, QString execId, QString error)> callback);

private:
    Core::DockerClient *m_client;
};

} // namespace Features::Containers
