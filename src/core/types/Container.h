#pragma once
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>

namespace Core {

enum class ContainerStatus {
    Running,
    Paused,
    Exited,
    Created,
    Restarting,
    Dead,
    Unknown
};

struct PortBinding {
    QString hostIp;
    quint16 hostPort{0};
    quint16 containerPort{0};
    QString protocol; // tcp/udp
};

struct Container {
    QString id;
    QString shortId;
    QString name;
    QString image;
    QString imageId;
    ContainerStatus status{ContainerStatus::Unknown};
    QString statusText;
    QDateTime created;
    QString command;
    QList<PortBinding> ports;
    QStringList networks;
    QString networkMode;
    qint64 sizeRootFs{0};
    qint64 sizeRw{0};

    static Container fromJson(const QJsonObject &obj);
    static ContainerStatus parseStatus(const QString &s);
    static QString statusToString(ContainerStatus s);
    QString statusColor() const;
    bool isRunning() const { return status == ContainerStatus::Running; }
    bool isPaused() const { return status == ContainerStatus::Paused; }
};

} // namespace Core
