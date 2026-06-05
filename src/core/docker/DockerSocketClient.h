#pragma once
#include <QObject>
#include <QLocalSocket>
#include <functional>

namespace Core {

// Low-level HTTP-over-Unix-socket request
class DockerSocketClient : public QObject {
    Q_OBJECT
public:
    explicit DockerSocketClient(const QString &socketPath, int timeoutMs = 10000, QObject *parent = nullptr);

    void sendRequest(const QString &method, const QString &path,
                     const QByteArray &body,
                     std::function<void(int statusCode, QByteArray response, QString error)> callback);

private:
    QString m_socketPath;
    int     m_timeoutMs;
};

} // namespace Core
