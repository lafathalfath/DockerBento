#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPixmap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include "features/hub/model/HubRepository.h"

namespace Features::Hub {

// Circular avatar that renders a colored disc with a single letter (fallback).
class AvatarLabel : public QWidget {
public:
    explicit AvatarLabel(const QString &letter, const QString &color, QWidget *parent = nullptr)
        : QWidget(parent), m_letter(letter), m_color(color)
    {
        setFixedSize(40, 40);
    }

    void setPixmap(const QPixmap &px) {
        m_pixmap = px.scaled(40, 40, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QPainterPath circle;
        circle.addEllipse(0, 0, 40, 40);
        p.setClipPath(circle);

        if (!m_pixmap.isNull()) {
            // Centre the scaled pixmap
            int x = (40 - m_pixmap.width())  / 2;
            int y = (40 - m_pixmap.height()) / 2;
            p.drawPixmap(x, y, m_pixmap);
        } else {
            p.fillPath(circle, QColor(m_color));
            p.setPen(QColor(255, 255, 255, 220));
            QFont f = p.font();
            f.setPixelSize(18);
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRect(0, 0, 40, 40), Qt::AlignCenter, m_letter);
        }
    }
private:
    QString  m_letter;
    QString  m_color;
    QPixmap  m_pixmap;
};

class HubImageCard : public QWidget {
    Q_OBJECT
public:
    explicit HubImageCard(const HubImage &img, QNetworkAccessManager *nam, QWidget *parent = nullptr)
        : QWidget(parent), m_image(img)
    {
        setFixedSize(220, 130);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet(
            "HubImageCard {"
            "  background-color: #1e1e1e;"
            "  border: 1px solid #2e2e2e;"
            "  border-radius: 8px;"
            "}"
            "HubImageCard:hover {"
            "  background-color: #252525;"
            "  border: 1px solid #1565c0;"
            "}");

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(12, 10, 12, 10);
        root->setSpacing(6);

        // Top row: avatar + name/badge
        auto *topRow = new QHBoxLayout;
        topRow->setSpacing(10);

        m_avatar = new AvatarLabel(img.avatarLetter(), img.avatarColor());
        topRow->addWidget(m_avatar);

        auto *nameCol = new QVBoxLayout;
        nameCol->setSpacing(2);

        auto *nameLabel = new QLabel(img.name);
        nameLabel->setStyleSheet("color: #e0e0e0; font-size: 12px; font-weight: bold;");
        nameLabel->setWordWrap(false);
        nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        nameCol->addWidget(nameLabel);

        if (img.isOfficial) {
            auto *badge = new QLabel("✔ Official");
            badge->setStyleSheet("color: #4caf50; font-size: 10px;");
            nameCol->addWidget(badge);
        }
        topRow->addLayout(nameCol, 1);
        root->addLayout(topRow);

        // Description
        auto *desc = new QLabel(img.description.isEmpty()
            ? "No description available." : img.description);
        desc->setWordWrap(true);
        desc->setStyleSheet("color: #777; font-size: 10px;");
        desc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        root->addWidget(desc, 1);

        // Stats row
        auto *statsRow = new QHBoxLayout;
        statsRow->setSpacing(12);

        auto makeStat = [](const QString &icon, const QString &val) {
            auto *lbl = new QLabel(icon + " " + val);
            lbl->setStyleSheet("color: #555; font-size: 10px;");
            return lbl;
        };

        QString pulls = img.pullCount > 1000000
            ? QString("%1M").arg(img.pullCount / 1000000)
            : img.pullCount > 1000
                ? QString("%1K").arg(img.pullCount / 1000)
                : QString::number(img.pullCount);

        statsRow->addWidget(makeStat("★", QString::number(img.starCount)));
        statsRow->addWidget(makeStat("⬇", pulls));
        statsRow->addStretch();
        root->addLayout(statsRow);

        // Async logo fetch: try /v2/orgs/{name}/ then /v2/users/{name}/
        if (nam) fetchLogo(nam);
    }

    const HubImage &image() const { return m_image; }

signals:
    void clicked(const HubImage &img);

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) emit clicked(m_image);
        QWidget::mousePressEvent(e);
    }

private:
    void fetchLogo(QNetworkAccessManager *nam) {
        // Derive org/user name from image name ("library/nginx" → "nginx", "nginx" → "nginx")
        QString slug = m_image.name.contains('/') ? m_image.name.section('/', -1) : m_image.name;
        // For official library images, publisher org often has a matching org page
        QUrl url(QString("https://hub.docker.com/v2/orgs/%1/").arg(slug));
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, "DockerBento/1.0");
        auto *reply = nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, nam, slug]() {
            reply->deleteLater();
            QString gravatarUrl;
            if (reply->error() == QNetworkReply::NoError) {
                auto doc = QJsonDocument::fromJson(reply->readAll());
                gravatarUrl = doc.object()["gravatar_url"].toString();
            }
            if (gravatarUrl.isEmpty()) {
                // Try /v2/users/{name}/ as fallback (for non-org publishers)
                QUrl url2(QString("https://hub.docker.com/v2/users/%1/").arg(slug));
                QNetworkRequest req2(url2);
                req2.setHeader(QNetworkRequest::UserAgentHeader, "DockerBento/1.0");
                auto *reply2 = nam->get(req2);
                connect(reply2, &QNetworkReply::finished, this, [this, reply2, nam]() {
                    reply2->deleteLater();
                    if (reply2->error() == QNetworkReply::NoError) {
                        auto doc = QJsonDocument::fromJson(reply2->readAll());
                        QString url = doc.object()["gravatar_url"].toString();
                        if (!url.isEmpty()) downloadLogo(nam, url);
                    }
                });
            } else {
                downloadLogo(nam, gravatarUrl);
            }
        });
    }

    void downloadLogo(QNetworkAccessManager *nam, const QString &url) {
        // Strip existing query, request 80px; d=404 means "return HTTP 404 if no custom avatar"
        // so we can detect missing logos and keep the letter fallback
        QString sized = url;
        sized.replace(QRegularExpression("\\?.*$"), "").append("?s=80&r=g&d=404");
        QNetworkRequest dlReq{QUrl{sized}};
        dlReq.setHeader(QNetworkRequest::UserAgentHeader, "DockerBento/1.0");
        auto *reply = nam->get(dlReq);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) return;
            QPixmap px;
            if (px.loadFromData(reply->readAll()) && !px.isNull())
                m_avatar->setPixmap(px);
        });
    }

    HubImage    m_image;
    AvatarLabel *m_avatar{nullptr};
};

} // namespace Features::Hub
