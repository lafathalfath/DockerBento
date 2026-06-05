#include "DockerClient.h"
#include "DockerSocketClient.h"
#include "features/settings/model/ConnectionSettings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QLocalSocket>
#include <unistd.h>

namespace Core {

DockerClient::DockerClient(QObject *parent)
    : QObject(parent)
{
    // Load persisted settings, which auto-detects socket if not set
    auto cfg = Features::Settings::ConnectionSettings::load();
    applySettings(cfg);
}

void DockerClient::applySettings(const Features::Settings::ConnectionSettings &cfg) {
    m_socketPath = cfg.socketPath;
    m_timeoutMs  = cfg.timeoutMs;
    // Reset connection state — caller should re-test
    if (m_connected) {
        m_connected = false;
        emit connectionChanged(false);
    }
}

void DockerClient::testConnection(std::function<void(bool, const QString &, const QString &)> callback) {
    get("/v1.41/info", [callback](bool ok, const QJsonDocument &doc, const QString &err) {
        if (!ok) { callback(false, {}, err); return; }
        QString info = QString("Docker %1 — %2 containers running")
            .arg(doc.object()["ServerVersion"].toString())
            .arg(doc.object()["ContainersRunning"].toInt());
        callback(true, info, {});
    });
}

void DockerClient::updateConnected(bool v) {
    if (m_connected == v) return;
    m_connected = v;
    emit connectionChanged(v);
}

void DockerClient::sendRequest(const QString &method, const QString &path,
                               const QByteArray &body,
                               std::function<void(int, QByteArray, QString)> callback)
{
    auto *client = new DockerSocketClient(m_socketPath, m_timeoutMs, this);
    client->sendRequest(method, path, body, [this, callback, client](int code, QByteArray data, QString err) {
        updateConnected(code != -1);
        callback(code, data, err);
        client->deleteLater();
    });
}

void DockerClient::get(const QString &path, JsonCallback callback)
{
    sendRequest("GET", path, {}, [callback](int code, QByteArray data, QString err) {
        if (!err.isEmpty() || code < 0) {
            callback(false, {}, err.isEmpty() ? "Connection failed" : err);
            return;
        }
        QJsonParseError pe;
        auto doc = QJsonDocument::fromJson(data, &pe);
        if (pe.error != QJsonParseError::NoError) {
            callback(false, {}, "JSON parse error: " + pe.errorString());
            return;
        }
        callback(code >= 200 && code < 300, doc,
                 code >= 400 ? QString("HTTP %1").arg(code) : QString{});
    });
}

void DockerClient::post(const QString &path, const QJsonDocument &body, PlainCallback callback)
{
    sendRequest("POST", path, body.toJson(QJsonDocument::Compact), [callback](int code, QByteArray, QString err) {
        if (!err.isEmpty() || code < 0) {
            callback(false, err.isEmpty() ? "Connection failed" : err);
            return;
        }
        callback(code >= 200 && code < 300,
                 code >= 400 ? QString("HTTP %1").arg(code) : QString{});
    });
}

void DockerClient::postJson(const QString &path, const QJsonDocument &body, JsonCallback callback)
{
    sendRequest("POST", path, body.toJson(QJsonDocument::Compact), [callback](int code, QByteArray data, QString err) {
        if (!err.isEmpty() || code < 0) {
            callback(false, {}, err.isEmpty() ? "Connection failed" : err);
            return;
        }
        QJsonParseError pe;
        auto doc = QJsonDocument::fromJson(data, &pe);
        callback(code >= 200 && code < 300, doc,
                 code >= 400 ? QString("HTTP %1").arg(code) : QString{});
    });
}

void DockerClient::del(const QString &path, PlainCallback callback)
{
    sendRequest("DELETE", path, {}, [callback](int code, QByteArray, QString err) {
        if (!err.isEmpty() || code < 0) {
            callback(false, err.isEmpty() ? "Connection failed" : err);
            return;
        }
        callback(code >= 200 && code < 300,
                 code >= 400 ? QString("HTTP %1").arg(code) : QString{});
    });
}

void DockerClient::streamGet(const QString &path, StreamCallback callback)
{
    auto *socket = new QLocalSocket(this);
    auto *buffer = new QByteArray();
    auto *headersParsed = new bool(false);

    connect(socket, &QLocalSocket::connected, this, [socket, path]() {
        QByteArray req;
        req += "GET " + path.toUtf8() + " HTTP/1.1\r\n";
        req += "Host: localhost\r\n";
        req += "Accept: application/json\r\n";
        req += "Connection: close\r\n\r\n";
        socket->write(req);
    });

    connect(socket, &QLocalSocket::readyRead, this, [socket, buffer, headersParsed, callback]() {
        buffer->append(socket->readAll());
        if (!*headersParsed) {
            int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0) return;
            *buffer = buffer->mid(headerEnd + 4);
            *headersParsed = true;
        }
        while (true) {
            int nl = buffer->indexOf('\n');
            if (nl < 0) break;
            QString line = QString::fromUtf8(buffer->left(nl)).trimmed();
            *buffer = buffer->mid(nl + 1);
            if (!line.isEmpty()) callback(line, false);
        }
    });

    connect(socket, &QLocalSocket::disconnected, this, [socket, buffer, headersParsed, callback]() {
        if (!buffer->isEmpty())
            callback(QString::fromUtf8(*buffer), false);
        callback({}, true);
        delete buffer;
        delete headersParsed;
        socket->deleteLater();
    });

    socket->connectToServer(m_socketPath);
}

} // namespace Core
