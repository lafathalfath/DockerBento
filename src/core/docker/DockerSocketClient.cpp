#include "DockerSocketClient.h"
#include <QLocalSocket>
#include <QTimer>
#include <memory>

namespace Core {

DockerSocketClient::DockerSocketClient(const QString &socketPath, int timeoutMs, QObject *parent)
    : QObject(parent), m_socketPath(socketPath), m_timeoutMs(timeoutMs)
{}

void DockerSocketClient::sendRequest(const QString &method, const QString &path,
                                     const QByteArray &body,
                                     std::function<void(int, QByteArray, QString)> callback)
{
    // All shared state lives in these shared_ptrs so any lambda can be the
    // last one to run without double-free or use-after-free.
    auto responseData = std::make_shared<QByteArray>();
    auto done         = std::make_shared<bool>(false); // guard: fire callback exactly once

    QByteArray request;
    request += method.toUtf8() + " " + path.toUtf8() + " HTTP/1.1\r\n";
    request += "Host: localhost\r\n";
    request += "Accept: application/json\r\n";
    if (!body.isEmpty()) {
        request += "Content-Type: application/json\r\n";
        request += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    }
    request += "Connection: close\r\n\r\n";
    if (!body.isEmpty()) request += body;

    auto *socket = new QLocalSocket(this);

    auto finish = [done, socket, callback](int code, QByteArray data, QString err) {
        if (*done) return;
        *done = true;
        socket->abort();
        socket->deleteLater();
        callback(code, data, err);
    };

    connect(socket, &QLocalSocket::connected, this, [socket, request]() {
        socket->write(request);
    });

    connect(socket, &QLocalSocket::readyRead, this, [socket, responseData]() {
        responseData->append(socket->readAll());
    });

    connect(socket, &QLocalSocket::disconnected, this, [responseData, finish]() {
        int headerEnd = responseData->indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            finish(-1, {}, "Invalid HTTP response (no header terminator)");
            return;
        }

        QString headerStr = QString::fromUtf8(responseData->left(headerEnd));
        QByteArray responseBody = responseData->mid(headerEnd + 4);

        // Extract status code: "HTTP/1.1 200 OK"
        int statusCode = -1;
        int firstSpace = headerStr.indexOf(' ');
        if (firstSpace > 0) {
            int secondSpace = headerStr.indexOf(' ', firstSpace + 1);
            statusCode = headerStr.mid(firstSpace + 1, secondSpace - firstSpace - 1).toInt();
        }

        // Decode chunked transfer encoding
        if (headerStr.contains("Transfer-Encoding: chunked", Qt::CaseInsensitive)) {
            QByteArray decoded;
            QByteArray rem = responseBody;
            while (!rem.isEmpty()) {
                int crlf = rem.indexOf("\r\n");
                if (crlf < 0) break;
                bool ok;
                int chunkSize = rem.left(crlf).trimmed().toInt(&ok, 16);
                if (!ok || chunkSize == 0) break;
                decoded += rem.mid(crlf + 2, chunkSize);
                rem = rem.mid(crlf + 2 + chunkSize + 2);
            }
            responseBody = decoded;
        }

        finish(statusCode, responseBody, {});
    });

    connect(socket, &QLocalSocket::errorOccurred, this,
            [socket, finish](QLocalSocket::LocalSocketError err) {
                // RemoteClosedError = Docker closed connection after sending response — normal.
                // The disconnected signal fires right after and will parse the response.
                if (err == QLocalSocket::PeerClosedError)
                    return;
                finish(-1, {}, socket->errorString());
            });

    // Timeout guard — parent-owned so it is cleaned up even if socket outlives us
    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(m_timeoutMs);
    connect(timer, &QTimer::timeout, this, [finish, timer]() {
        timer->deleteLater();
        finish(-1, {}, "Connection timed out");
    });
    // Stop timer when socket finishes normally (either path)
    connect(socket, &QLocalSocket::disconnected,  timer, &QTimer::stop);
    connect(socket, &QLocalSocket::errorOccurred, timer, &QTimer::stop);
    timer->start();

    socket->connectToServer(m_socketPath);
}

} // namespace Core
