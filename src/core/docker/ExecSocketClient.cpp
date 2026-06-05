#include "ExecSocketClient.h"

namespace Core {

ExecSocketClient::ExecSocketClient(const QString &socketPath, QObject *parent)
    : QObject(parent), m_socketPath(socketPath)
{}

ExecSocketClient::~ExecSocketClient() {
    close();
}

void ExecSocketClient::start(const QString &execId) {
    m_execId = execId;
    m_headersParsed = false;
    m_connected = false;
    m_buffer.clear();

    m_socket = new QLocalSocket(this);

    connect(m_socket, &QLocalSocket::connected,
            this, &ExecSocketClient::onSocketConnected);
    connect(m_socket, &QLocalSocket::readyRead,
            this, &ExecSocketClient::onSocketReadyRead);
    connect(m_socket, &QLocalSocket::disconnected,
            this, &ExecSocketClient::onSocketDisconnected);
    connect(m_socket, &QLocalSocket::errorOccurred,
            this, &ExecSocketClient::onSocketError);

    m_socket->connectToServer(m_socketPath);
}

void ExecSocketClient::write(const QByteArray &data) {
    if (m_socket && m_connected)
        m_socket->write(data);
}

void ExecSocketClient::close() {
    m_connected = false;
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void ExecSocketClient::onSocketConnected() {
    QByteArray body = "{\"Detach\":false,\"Tty\":true}";
    QByteArray request;
    request += "POST /v1.41/exec/" + m_execId.toUtf8() + "/start HTTP/1.1\r\n";
    request += "Host: localhost\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    request += "Connection: Upgrade\r\n";
    request += "Upgrade: tcp\r\n";
    request += "\r\n";
    request += body;

    m_socket->write(request);
}

void ExecSocketClient::onSocketReadyRead() {
    m_buffer.append(m_socket->readAll());

    if (!m_headersParsed) {
        int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;

        QByteArray headers = m_buffer.left(headerEnd);
        m_buffer = m_buffer.mid(headerEnd + 4);

        // Check for HTTP 200 / 101 response
        if (!headers.startsWith("HTTP/1.1 200") && !headers.startsWith("HTTP/1.1 101")) {
            emit errorOccurred("Exec start failed: " + QString::fromUtf8(headers.left(40)));
            close();
            return;
        }

        m_headersParsed = true;
        m_connected = true;
        emit connected();
    }

    if (!m_buffer.isEmpty()) {
        QByteArray data = m_buffer;
        m_buffer.clear();
        emit dataReceived(data);
    }
}

void ExecSocketClient::onSocketDisconnected() {
    m_connected = false;
    emit finished();
}

void ExecSocketClient::onSocketError(QLocalSocket::LocalSocketError err) {
    if (err == QLocalSocket::PeerClosedError)
        return;
    emit errorOccurred(m_socket ? m_socket->errorString() : "Socket error");
}

} // namespace Core
