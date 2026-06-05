#include "Container.h"
#include <QJsonArray>

namespace Core {

ContainerStatus Container::parseStatus(const QString &s) {
    QString lower = s.toLower();
    if (lower.startsWith("up")) return ContainerStatus::Running;
    if (lower.startsWith("paused")) return ContainerStatus::Paused;
    if (lower.startsWith("exited") || lower.startsWith("stopped")) return ContainerStatus::Exited;
    if (lower.startsWith("created")) return ContainerStatus::Created;
    if (lower.startsWith("restarting")) return ContainerStatus::Restarting;
    if (lower.startsWith("dead")) return ContainerStatus::Dead;
    return ContainerStatus::Unknown;
}

QString Container::statusToString(ContainerStatus s) {
    switch (s) {
        case ContainerStatus::Running:    return "Running";
        case ContainerStatus::Paused:     return "Paused";
        case ContainerStatus::Exited:     return "Exited";
        case ContainerStatus::Created:    return "Created";
        case ContainerStatus::Restarting: return "Restarting";
        case ContainerStatus::Dead:       return "Dead";
        default:                          return "Unknown";
    }
}

QString Container::statusColor() const {
    switch (status) {
        case ContainerStatus::Running:    return "#4caf50";
        case ContainerStatus::Paused:     return "#ff9800";
        case ContainerStatus::Exited:     return "#f44336";
        case ContainerStatus::Created:    return "#2196f3";
        case ContainerStatus::Restarting: return "#9c27b0";
        case ContainerStatus::Dead:       return "#795548";
        default:                          return "#9e9e9e";
    }
}

Container Container::fromJson(const QJsonObject &obj) {
    Container c;
    c.id = obj["Id"].toString();
    c.shortId = c.id.left(12);

    // Names come as ["/name"]
    QJsonArray names = obj["Names"].toArray();
    if (!names.isEmpty())
        c.name = names[0].toString().remove(QChar('/'));

    c.image = obj["Image"].toString();
    c.imageId = obj["ImageID"].toString();
    c.statusText = obj["Status"].toString();
    c.status = parseStatus(c.statusText);
    c.created = QDateTime::fromSecsSinceEpoch(
        static_cast<qint64>(obj["Created"].toDouble()));
    c.command = obj["Command"].toString();

    // Ports
    QJsonArray ports = obj["Ports"].toArray();
    for (const auto &p : ports) {
        QJsonObject po = p.toObject();
        PortBinding pb;
        pb.hostIp = po["IP"].toString();
        pb.hostPort = static_cast<quint16>(po["PublicPort"].toInt());
        pb.containerPort = static_cast<quint16>(po["PrivatePort"].toInt());
        pb.protocol = po["Type"].toString("tcp");
        c.ports.append(pb);
    }

    // Networks
    QJsonObject netSettings = obj["NetworkSettings"].toObject();
    QJsonObject nets = netSettings["Networks"].toObject();
    for (auto it = nets.begin(); it != nets.end(); ++it)
        c.networks.append(it.key());

    // Host config
    QJsonObject hostConfig = obj["HostConfig"].toObject();
    c.networkMode = hostConfig["NetworkMode"].toString();

    c.sizeRootFs = static_cast<qint64>(obj["SizeRootFs"].toDouble());
    c.sizeRw = static_cast<qint64>(obj["SizeRw"].toDouble());

    return c;
}

} // namespace Core
