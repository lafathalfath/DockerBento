#pragma once
#include <QObject>
#include "core/types/Volume.h"
#include "features/volumes/model/VolumeRepository.h"

namespace Features::Volumes {

class VolumeListViewModel : public QObject {
    Q_OBJECT
public:
    explicit VolumeListViewModel(VolumeRepository *repo, QObject *parent = nullptr);

    void refresh();
    void setFilter(const QString &text);
    void removeVolume(const QString &name);
    void pruneVolumes();

    const QList<Core::Volume> &volumes() const { return m_filtered; }
    bool isLoading() const { return m_loading; }

signals:
    void volumesChanged();
    void loadingChanged(bool loading);
    void actionSucceeded(const QString &message);
    void actionFailed(const QString &message);
    void errorOccurred(const QString &message);

private:
    void applyFilter();

    VolumeRepository *m_repo;
    QList<Core::Volume> m_all;
    QList<Core::Volume> m_filtered;
    QString m_filterText;
    bool m_loading{false};
};

} // namespace Features::Volumes
