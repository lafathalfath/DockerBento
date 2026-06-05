#include "ContainerRepository.h"
#include "core/docker/DockerClient.h"
#include "core/types/Container.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

namespace Features::Containers {

ContainerRepository::ContainerRepository(Core::DockerClient *client, QObject *parent)
    : QObject(parent), m_client(client)
{}

void ContainerRepository::fetchAll(ContainerCallback callback) {
    m_client->get("/v1.41/containers/json?all=true&size=true", [callback](bool ok, const QJsonDocument &doc, const QString &err) {
        if (!ok) { callback(false, {}, err); return; }
        ContainerList list;
        for (const auto &val : doc.array())
            list.append(Core::Container::fromJson(val.toObject()));
        callback(true, list, {});
    });
}

void ContainerRepository::fetchInspect(const QString &id, std::function<void(bool, QJsonObject, QString)> callback) {
    m_client->get(QString("/v1.41/containers/%1/json").arg(id), [callback](bool ok, const QJsonDocument &doc, const QString &err) {
        if (!ok) { callback(false, {}, err); return; }
        callback(true, doc.object(), {});
    });
}

void ContainerRepository::start(const QString &id, ActionCallback callback) {
    m_client->post(QString("/v1.41/containers/%1/start").arg(id), {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ContainerRepository::stop(const QString &id, ActionCallback callback) {
    m_client->post(QString("/v1.41/containers/%1/stop").arg(id), {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ContainerRepository::restart(const QString &id, ActionCallback callback) {
    m_client->post(QString("/v1.41/containers/%1/restart").arg(id), {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ContainerRepository::pause(const QString &id, ActionCallback callback) {
    m_client->post(QString("/v1.41/containers/%1/pause").arg(id), {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ContainerRepository::unpause(const QString &id, ActionCallback callback) {
    m_client->post(QString("/v1.41/containers/%1/unpause").arg(id), {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ContainerRepository::remove(const QString &id, bool force, ActionCallback callback) {
    QString path = QString("/v1.41/containers/%1?v=true").arg(id);
    if (force) path += "&force=true";
    m_client->del(path, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ContainerRepository::kill(const QString &id, const QString &signal, ActionCallback callback) {
    // Whitelist valid Unix signal names to prevent query parameter injection
    static const QStringList allowed = {
        "SIGKILL", "SIGTERM", "SIGHUP", "SIGINT", "SIGQUIT",
        "SIGUSR1", "SIGUSR2", "SIGSTOP", "SIGCONT"
    };
    if (!allowed.contains(signal)) {
        callback(false, "Invalid signal: " + signal);
        return;
    }
    m_client->post(QString("/v1.41/containers/%1/kill?signal=%2").arg(id, signal), {}, [callback](bool ok, const QString &err) {
        callback(ok, err);
    });
}

void ContainerRepository::getLogs(const QString &id, int tail, LogsCallback callback) {
    QString path = QString("/v1.41/containers/%1/logs?stdout=true&stderr=true&tail=%2&timestamps=true")
                       .arg(id).arg(tail);
    m_client->get(path, [callback](bool ok, const QJsonDocument &, const QString &err) {
        if (!ok) { callback("Error: " + err, true); return; }
        callback({}, true);
    });
}

void ContainerRepository::streamLogs(const QString &id, LogsCallback callback) {
    QString path = QString("/v1.41/containers/%1/logs?stdout=true&stderr=true&follow=true&timestamps=true&tail=200")
                       .arg(id);
    m_client->streamGet(path, callback);
}

void ContainerRepository::execCreate(const QString &containerId, const QString &shell,
                                     std::function<void(bool, QString, QString)> callback)
{
    QJsonObject body;
    body["AttachStdin"] = true;
    body["AttachStdout"] = true;
    body["AttachStderr"] = true;
    body["Tty"] = true;
    body["Cmd"] = QJsonArray{shell};

    QString path = QString("/v1.41/containers/%1/exec").arg(containerId);
    m_client->postJson(path, QJsonDocument(body),
        [callback](bool ok, const QJsonDocument &doc, const QString &err) {
            if (!ok) { callback(false, {}, err); return; }
            QString execId = doc.object()["Id"].toString();
            if (execId.isEmpty()) { callback(false, {}, "No exec ID in response"); return; }
            callback(true, execId, {});
        });
}

} // namespace Features::Containers
