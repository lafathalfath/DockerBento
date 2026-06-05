#pragma once
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>

namespace Core {

struct Image {
    QString id;
    QString shortId;
    QStringList repoTags;
    QStringList repoDigests;
    qint64 size{0};
    qint64 sharedSize{0};
    QDateTime created;
    QString parentId;
    int containers{0};

    QString displayName() const;
    QString tag() const;
    QString repository() const;

    static Image fromJson(const QJsonObject &obj);
};

} // namespace Core
