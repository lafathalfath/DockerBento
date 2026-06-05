#include "Network.h"
#include <QJsonArray>
#include <QJsonObject>

namespace Core {

Network Network::fromJson(const QJsonObject &obj) {
    Network n;
    n.id = obj["Id"].toString();
    n.shortId = n.id.left(12);
    n.name = obj["Name"].toString();
    n.driver = obj["Driver"].toString();
    n.scope = obj["Scope"].toString();
    n.internal = obj["Internal"].toBool();
    n.attachable = obj["Attachable"].toBool();
    n.ingress = obj["Ingress"].toBool();
    n.ipv6Enabled = obj["EnableIPv6"].toBool();
    n.created = QDateTime::fromString(obj["Created"].toString(), Qt::ISODateWithMs);

    // IPAM
    QJsonObject ipam = obj["IPAM"].toObject();
    QJsonArray configs = ipam["Config"].toArray();
    for (const auto &c : configs) {
        QJsonObject co = c.toObject();
        NetworkIPAM entry;
        entry.driver = ipam["Driver"].toString();
        entry.subnet = co["Subnet"].toString();
        entry.gateway = co["Gateway"].toString();
        entry.ipRange = co["IPRange"].toString();
        n.ipam.append(entry);
    }

    // Containers
    QJsonObject containers = obj["Containers"].toObject();
    n.containerCount = containers.size();
    for (auto it = containers.begin(); it != containers.end(); ++it)
        n.connectedContainers.append(it.key().left(12));

    return n;
}

} // namespace Core
