#include "Image.h"
#include <QJsonArray>

namespace Core {

Image Image::fromJson(const QJsonObject &obj) {
    Image img;
    img.id = obj["Id"].toString();
    if (img.id.startsWith("sha256:"))
        img.shortId = img.id.mid(7, 12);
    else
        img.shortId = img.id.left(12);

    QJsonArray tags = obj["RepoTags"].toArray();
    for (const auto &t : tags)
        img.repoTags.append(t.toString());

    QJsonArray digests = obj["RepoDigests"].toArray();
    for (const auto &d : digests)
        img.repoDigests.append(d.toString());

    img.size = static_cast<qint64>(obj["Size"].toDouble());
    img.sharedSize = static_cast<qint64>(obj["SharedSize"].toDouble());
    img.created = QDateTime::fromSecsSinceEpoch(
        static_cast<qint64>(obj["Created"].toDouble()));
    img.parentId = obj["ParentId"].toString();
    img.containers = obj["Containers"].toInt();

    return img;
}

QString Image::displayName() const {
    if (repoTags.isEmpty() || repoTags[0] == "<none>:<none>")
        return shortId;
    return repoTags[0];
}

QString Image::repository() const {
    if (repoTags.isEmpty()) return "<none>";
    QString tag = repoTags[0];
    int colon = tag.lastIndexOf(':');
    return colon >= 0 ? tag.left(colon) : tag;
}

QString Image::tag() const {
    if (repoTags.isEmpty()) return "<none>";
    QString t = repoTags[0];
    int colon = t.lastIndexOf(':');
    return colon >= 0 ? t.mid(colon + 1) : "latest";
}

} // namespace Core
