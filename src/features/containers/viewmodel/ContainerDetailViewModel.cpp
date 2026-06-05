#include "ContainerDetailViewModel.h"

namespace Features::Containers {

ContainerDetailViewModel::ContainerDetailViewModel(ContainerRepository *repo, QObject *parent)
    : QObject(parent), m_repo(repo)
{}

void ContainerDetailViewModel::load(const Core::Container &container) {
    m_container = container;
    m_repo->fetchInspect(container.id, [this](bool ok, QJsonObject obj, QString) {
        if (ok) {
            m_inspectData = obj;
            emit detailLoaded();
        }
    });
}

void ContainerDetailViewModel::startLogStream() {
    if (m_streaming) return;
    m_streaming = true;
    m_repo->streamLogs(m_container.id, [this](const QString &chunk, bool done) {
        if (done) {
            m_streaming = false;
            emit logStreamFinished();
            return;
        }
        // Strip Docker multiplexing header (8 bytes: stream type + 3 null + 4-byte length)
        QString line = chunk;
        if (line.length() > 8) {
            QByteArray raw = line.toUtf8();
            if (raw[0] == 0x01 || raw[0] == 0x02)
                line = QString::fromUtf8(raw.mid(8));
        }
        emit logLine(line.trimmed());
    });
}

void ContainerDetailViewModel::stopLogStream() {
    m_streaming = false;
}

} // namespace Features::Containers
