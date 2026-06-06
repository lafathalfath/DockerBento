#pragma once
#include <QObject>
#include <QList>
#include <QString>
#include <functional>

class QNetworkAccessManager;

namespace Features::Hub {

struct HubImage {
    QString name;
    QString description;
    int     starCount{0};
    int     pullCount{0};
    bool    isOfficial{false};
    bool    isAutomated{false};

    // Derived display helpers — filled after construction
    QString avatarLetter() const {
        if (name.isEmpty()) return "?";
        // For "library/nginx" or "nginx" take first char of last segment
        QString seg = name.contains('/') ? name.section('/', -1) : name;
        return seg.left(1).toUpper();
    }
    // Deterministic color from name
    QString avatarColor() const {
        static const char *palette[] = {
            "#1565c0","#6a1b9a","#00695c","#c62828",
            "#e65100","#2e7d32","#4527a0","#00838f"
        };
        int h = 0;
        for (QChar c : name) h = (h * 31 + c.unicode()) & 0x7fffffff;
        return palette[h % 8];
    }
};

struct HubTag {
    QString name;
    QString digest;
    qint64  fullSize{0};
    QString lastUpdated;
};

class HubRepository : public QObject {
    Q_OBJECT
public:
    explicit HubRepository(QObject *parent = nullptr);

    void search(const QString &query, int page,
                std::function<void(bool, QList<HubImage>, int total, QString)> callback);

    void fetchPopular(std::function<void(bool, QList<HubImage>, QString)> callback);

    void fetchTags(const QString &image, int page,
                   std::function<void(bool, QList<HubTag>, int total, QString)> callback);

    void pull(const QString &image,
              std::function<void(bool, QString)> callback);

private:
    QNetworkAccessManager *m_nam;
};

} // namespace Features::Hub
