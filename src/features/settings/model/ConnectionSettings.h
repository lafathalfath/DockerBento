#pragma once
#include <QString>
#include <QSettings>

namespace Features::Settings {

enum class ConnectionType {
    UnixSocket,  // /var/run/docker.sock or custom path
    Tcp          // tcp://host:port (e.g. remote Docker or Docker Desktop)
};

struct ConnectionSettings {
    ConnectionType type{ConnectionType::UnixSocket};
    QString socketPath{"/var/run/docker.sock"};
    QString tcpHost{"localhost"};
    int     tcpPort{2375};
    int     timeoutMs{10000};

    static ConnectionSettings load();
    void save() const;

    QString effectiveSocketPath() const;
    bool isTcp() const { return type == ConnectionType::Tcp; }
};

} // namespace Features::Settings
