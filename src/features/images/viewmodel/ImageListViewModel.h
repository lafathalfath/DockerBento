#pragma once
#include <QObject>
#include "core/types/Image.h"
#include "features/images/model/ImageRepository.h"

namespace Features::Images {

class ImageListViewModel : public QObject {
    Q_OBJECT
public:
    explicit ImageListViewModel(ImageRepository *repo, QObject *parent = nullptr);

    void refresh();
    void setFilter(const QString &text);
    void removeImage(const QString &id, bool force = false);
    void pruneImages();

    const QList<Core::Image> &images() const { return m_filtered; }
    bool isLoading() const { return m_loading; }

signals:
    void imagesChanged();
    void loadingChanged(bool loading);
    void actionSucceeded(const QString &message);
    void actionFailed(const QString &message);
    void errorOccurred(const QString &message);

private:
    void applyFilter();

    ImageRepository *m_repo;
    QList<Core::Image> m_all;
    QList<Core::Image> m_filtered;
    QString m_filterText;
    bool m_loading{false};
};

} // namespace Features::Images
