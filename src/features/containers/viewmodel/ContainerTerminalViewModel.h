#pragma once
#include <QObject>
#include "core/types/Container.h"

namespace Core { class DockerClient; class ExecSocketClient; }

namespace Features::Containers {

class ContainerRepository;

class ContainerTerminalViewModel : public QObject {
    Q_OBJECT
public:
    explicit ContainerTerminalViewModel(ContainerRepository *repo,
                                        Core::DockerClient *docker,
                                        QObject *parent = nullptr);
    ~ContainerTerminalViewModel() override;

    void startSession(const QString &containerId, const QString &shell);
    void stopSession();
    void sendInput(const QString &text);
    bool isConnected() const { return m_exec != nullptr; }

signals:
    void outputReceived(const QByteArray &data);
    void sessionStarted();
    void sessionFinished();
    void sessionError(const QString &error);

private:
    void cleanup();

    ContainerRepository *m_repo;
    Core::DockerClient *m_docker;
    Core::ExecSocketClient *m_exec{nullptr};
};

} // namespace Features::Containers
