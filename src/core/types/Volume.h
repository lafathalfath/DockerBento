#pragma once
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QVariantMap>

namespace Core {

struct Volume {
    QString name;
    QString driver;
    QString mountpoint;
    QDateTime created;
    QString scope; // local/global
    QVariantMap options;
    QVariantMap labels;
    qint64 usageSize{-1}; // -1 = unknown

    static Volume fromJson(const QJsonObject &obj);
};

} // namespace Core
