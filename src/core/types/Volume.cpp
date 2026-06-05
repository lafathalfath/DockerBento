#include "Volume.h"
#include <QJsonObject>

namespace Core {

Volume Volume::fromJson(const QJsonObject &obj) {
    Volume v;
    v.name = obj["Name"].toString();
    v.driver = obj["Driver"].toString();
    v.mountpoint = obj["Mountpoint"].toString();
    v.created = QDateTime::fromString(obj["CreatedAt"].toString(), Qt::ISODate);
    v.scope = obj["Scope"].toString();

    QJsonObject opts = obj["Options"].toObject();
    for (auto it = opts.begin(); it != opts.end(); ++it)
        v.options.insert(it.key(), it.value().toVariant());

    QJsonObject labels = obj["Labels"].toObject();
    for (auto it = labels.begin(); it != labels.end(); ++it)
        v.labels.insert(it.key(), it.value().toVariant());

    if (obj.contains("UsageData")) {
        QJsonObject usage = obj["UsageData"].toObject();
        v.usageSize = static_cast<qint64>(usage["Size"].toDouble(-1));
    }

    return v;
}

} // namespace Core
