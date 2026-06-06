#include "HubRepository.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

namespace Features::Hub {

static const QString HUB_API = "https://hub.docker.com/v2";

HubRepository::HubRepository(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

void HubRepository::search(const QString &query, int page,
    std::function<void(bool, QList<HubImage>, int, QString)> callback)
{
    QUrl url(HUB_API + "/search/repositories/");
    QUrlQuery q;
    q.addQueryItem("query", query);
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("page_size", "25");
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "DockerBento/1.0");

    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, {}, 0, reply->errorString());
            return;
        }

        QJsonParseError pe;
        auto doc = QJsonDocument::fromJson(reply->readAll(), &pe);
        if (pe.error != QJsonParseError::NoError) {
            callback(false, {}, 0, "JSON parse error: " + pe.errorString());
            return;
        }

        auto root = doc.object();
        int total = root["count"].toInt();
        QList<HubImage> results;

        for (const auto &val : root["results"].toArray()) {
            auto obj = val.toObject();
            HubImage img;
            img.name        = obj["repo_name"].toString();
            img.description = obj["short_description"].toString();
            img.starCount   = obj["star_count"].toInt();
            img.pullCount   = obj["pull_count"].toInt();
            img.isOfficial  = obj["is_official"].toBool();
            img.isAutomated = obj["is_automated"].toBool();
            results.append(img);
        }

        callback(true, results, total, {});
    });
}

void HubRepository::fetchPopular(
    std::function<void(bool, QList<HubImage>, QString)> callback)
{
    // /repositories/library/ returns official images ordered by pull count.
    // Field names differ from /search/: "name" (not "repo_name"),
    // "description" (not "short_description"), no "is_official" (all are official).
    QUrl url(HUB_API + "/repositories/library/");
    QUrlQuery q;
    q.addQueryItem("page",      "1");
    q.addQueryItem("page_size", "24");
    q.addQueryItem("ordering",  "-pull_count");
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "DockerBento/1.0");

    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, {}, reply->errorString());
            return;
        }
        QJsonParseError pe;
        auto doc = QJsonDocument::fromJson(reply->readAll(), &pe);
        if (pe.error != QJsonParseError::NoError) {
            callback(false, {}, "JSON parse error: " + pe.errorString());
            return;
        }
        QList<HubImage> results;
        for (const auto &val : doc.object()["results"].toArray()) {
            auto obj = val.toObject();
            HubImage img;
            img.name        = obj["name"].toString();         // "nginx", not "library/nginx"
            img.description = obj["description"].toString();
            img.starCount   = obj["star_count"].toInt();
            img.pullCount   = obj["pull_count"].toInt();
            img.isOfficial  = true;                           // all library/ images are official
            results.append(img);
        }
        callback(true, results, {});
    });
}

void HubRepository::fetchTags(const QString &image, int page,
    std::function<void(bool, QList<HubTag>, int, QString)> callback)
{
    // image may be "library/nginx" or "nginx" — normalise
    QString repo = image.contains('/') ? image : "library/" + image;
    QUrl url(HUB_API + "/repositories/" + repo + "/tags");
    QUrlQuery q;
    q.addQueryItem("page",      QString::number(page));
    q.addQueryItem("page_size", "25");
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "DockerBento/1.0");

    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, {}, 0, reply->errorString());
            return;
        }
        QJsonParseError pe;
        auto doc = QJsonDocument::fromJson(reply->readAll(), &pe);
        if (pe.error != QJsonParseError::NoError) {
            callback(false, {}, 0, "JSON parse error: " + pe.errorString());
            return;
        }
        auto root  = doc.object();
        int  total = root["count"].toInt();
        QList<HubTag> tags;
        for (const auto &val : root["results"].toArray()) {
            auto obj = val.toObject();
            HubTag t;
            t.name        = obj["name"].toString();
            t.lastUpdated = obj["last_updated"].toString().left(10);
            // sum compressed sizes of all images in this tag
            for (const auto &img : obj["images"].toArray())
                t.fullSize += img.toObject()["size"].toVariant().toLongLong();
            if (!obj["images"].toArray().isEmpty())
                t.digest = obj["images"].toArray().first().toObject()["digest"].toString().left(19);
            tags.append(t);
        }
        callback(true, tags, total, {});
    });
}

void HubRepository::pull(const QString &image,
    std::function<void(bool, QString)> callback)
{
    // Pull is triggered via the local Docker daemon — signal back to the caller
    // with the image name so the DockerClient can execute the actual pull.
    // This keeps HubRepository free of DockerClient dependency.
    callback(true, image);
}

} // namespace Features::Hub
