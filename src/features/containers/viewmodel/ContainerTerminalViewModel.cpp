#include "ContainerTerminalViewModel.h"
#include "features/containers/model/ContainerRepository.h"
#include "core/docker/DockerClient.h"
#include "core/docker/ExecSocketClient.h"

namespace Features::Containers {

ContainerTerminalViewModel::ContainerTerminalViewModel(ContainerRepository *repo,
                                                       Core::DockerClient *docker,
                                                       QObject *parent)
    : QObject(parent), m_repo(repo), m_docker(docker)
{}

ContainerTerminalViewModel::~ContainerTerminalViewModel() {
    cleanup();
}

void ContainerTerminalViewModel::startSession(const QString &containerId, const QString &shell) {
    cleanup();

    m_repo->execCreate(containerId, shell,
        [this](bool ok, const QString &execId, const QString &error) {
            if (!ok) {
                emit sessionError(error);
                return;
            }

            m_exec = new Core::ExecSocketClient(m_docker->socketPath(), this);

            connect(m_exec, &Core::ExecSocketClient::connected, this, [this]() {
                emit sessionStarted();
            });

            connect(m_exec, &Core::ExecSocketClient::dataReceived, this,
                [this](const QByteArray &data) {
                    emit outputReceived(data);
                });

            connect(m_exec, &Core::ExecSocketClient::finished, this, [this]() {
                cleanup();
                emit sessionFinished();
            });

            connect(m_exec, &Core::ExecSocketClient::errorOccurred, this,
                [this](const QString &err) {
                    cleanup();
                    emit sessionError(err);
                });

            m_exec->start(execId);
        });
}

void ContainerTerminalViewModel::stopSession() {
    cleanup();
    emit sessionFinished();
}

void ContainerTerminalViewModel::sendInput(const QString &text) {
    if (m_exec && m_exec->isConnected())
        m_exec->write(text.toUtf8());
}

void ContainerTerminalViewModel::cleanup() {
    if (m_exec) {
        m_exec->close();
        m_exec->deleteLater();
        m_exec = nullptr;
    }
}

} // namespace Features::Containers
