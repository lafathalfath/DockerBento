#pragma once
#include <QObject>
#include "core/types/Container.h"
#include "features/containers/model/ContainerRepository.h"

namespace Features::Containers {

class ContainerDetailViewModel : public QObject {
    Q_OBJECT
public:
    explicit ContainerDetailViewModel(ContainerRepository *repo, QObject *parent = nullptr);

    void load(const Core::Container &container);
    void startLogStream();
    void stopLogStream();

    const Core::Container &container() const { return m_container; }
    const QJsonObject &inspectData() const { return m_inspectData; }

signals:
    void detailLoaded();
    void logLine(const QString &line);
    void logStreamFinished();

private:
    ContainerRepository *m_repo;
    Core::Container m_container;
    QJsonObject m_inspectData;
    bool m_streaming{false};
};

} // namespace Features::Containers
