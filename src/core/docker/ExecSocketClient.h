#pragma once
#include <QObject>
#include <QLocalSocket>

namespace Core {

class ExecSocketClient : public QObject {
    Q_OBJECT
public:
    explicit ExecSocketClient(const QString &socketPath, QObject *parent = nullptr);
    ~ExecSocketClient() override;

    void start(const QString &execId);
    void write(const QByteArray &data);
    void close();
    bool isConnected() const { return m_connected; }

signals:
    void dataReceived(const QByteArray &data);
    void connected();
    void finished();
    void errorOccurred(const QString &error);

private:
    void onSocketConnected();
    void onSocketReadyRead();
    void onSocketDisconnected();
    void onSocketError(QLocalSocket::LocalSocketError error);

    QString m_socketPath;
    QString m_execId;
    QLocalSocket *m_socket{nullptr};
    bool m_headersParsed{false};
    bool m_connected{false};
    QByteArray m_buffer;
};

} // namespace Core
