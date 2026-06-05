#include "ConnectionSettings.h"
#include <QFile>
#include <QSettings>
#include <unistd.h>

namespace Features::Settings {

ConnectionSettings ConnectionSettings::load() {
    QSettings s("DockerBento", "DockerBento");
    ConnectionSettings cfg;

    cfg.type = static_cast<ConnectionType>(
        s.value("connection/type", static_cast<int>(ConnectionType::UnixSocket)).toInt());
    cfg.timeoutMs = s.value("connection/timeoutMs", 10000).toInt();

    // Socket path: persisted value, or auto-detect default
    QString defaultSocket;
    QString rootless = QString("/run/user/%1/docker.sock").arg(getuid());
    if (QFile::exists(rootless))
        defaultSocket = rootless;
    else
        defaultSocket = "/var/run/docker.sock";

    cfg.socketPath = s.value("connection/socketPath", defaultSocket).toString();
    cfg.tcpHost    = s.value("connection/tcpHost", "localhost").toString();
    cfg.tcpPort    = s.value("connection/tcpPort", 2375).toInt();

    return cfg;
}

void ConnectionSettings::save() const {
    QSettings s("DockerBento", "DockerBento");
    s.setValue("connection/type",      static_cast<int>(type));
    s.setValue("connection/socketPath", socketPath);
    s.setValue("connection/tcpHost",   tcpHost);
    s.setValue("connection/tcpPort",   tcpPort);
    s.setValue("connection/timeoutMs", timeoutMs);
}

QString ConnectionSettings::effectiveSocketPath() const {
    if (type == ConnectionType::UnixSocket) return socketPath;
    return {}; // TCP handled separately in DockerClient
}

} // namespace Features::Settings
