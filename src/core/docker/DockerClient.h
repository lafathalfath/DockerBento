#pragma once
#include <QObject>
#include <QJsonDocument>
#include <functional>

namespace Features::Settings { struct ConnectionSettings; }

namespace Core {

using JsonCallback   = std::function<void(bool success, const QJsonDocument &doc, const QString &error)>;
using PlainCallback  = std::function<void(bool success, const QString &error)>;
using StreamCallback = std::function<void(const QString &chunk, bool done)>;

class DockerClient : public QObject {
    Q_OBJECT
public:
    explicit DockerClient(QObject *parent = nullptr);

    void applySettings(const Features::Settings::ConnectionSettings &cfg);
    void testConnection(std::function<void(bool ok, const QString &info, const QString &error)> callback);

    void get(const QString &path, JsonCallback callback);
    void post(const QString &path, const QJsonDocument &body, PlainCallback callback);
    void postJson(const QString &path, const QJsonDocument &body, JsonCallback callback);
    void del(const QString &path, PlainCallback callback);
    void streamGet(const QString &path, StreamCallback callback);

    bool isConnected() const { return m_connected; }
    QString socketPath() const { return m_socketPath; }

signals:
    void connectionChanged(bool connected);

private:
    void sendRequest(const QString &method, const QString &path,
                     const QByteArray &body, std::function<void(int, QByteArray, QString)> callback);
    void updateConnected(bool v);

    bool    m_connected{false};
    QString m_socketPath;
    int     m_timeoutMs{10000};
};

} // namespace Core
