#pragma once
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>

namespace Core {

struct NetworkIPAM {
    QString driver;
    QString subnet;
    QString gateway;
    QString ipRange;
};

struct Network {
    QString id;
    QString shortId;
    QString name;
    QString driver;  // bridge/host/overlay/macvlan/none
    QString scope;   // local/swarm/global
    bool internal{false};
    bool attachable{false};
    bool ingress{false};
    bool ipv6Enabled{false};
    QDateTime created;
    QList<NetworkIPAM> ipam;
    QStringList connectedContainers;
    int containerCount{0};

    static Network fromJson(const QJsonObject &obj);
};

} // namespace Core
