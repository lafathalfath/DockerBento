#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QLabel>
#include "core/types/Image.h"

namespace Core { class DockerClient; }

namespace Features::Images {

class RunContainerDialog : public QDialog {
    Q_OBJECT
public:
    explicit RunContainerDialog(const Core::Image &image,
                                Core::DockerClient *docker,
                                QWidget *parent = nullptr);

private:
    void setupUi();
    void onRun();
    void addPortRow(const QString &host = {}, const QString &container = {}, const QString &proto = "tcp");
    void addEnvRow(const QString &key = {}, const QString &val = {});
    void addVolumeRow(const QString &host = {}, const QString &container = {});
    QJsonDocument buildCreateBody() const;

    static const QString BTN_STYLE_ADD;
    static const QString INPUT_STYLE;

    Core::Image       m_image;
    Core::DockerClient *m_docker;

    QLineEdit  *m_nameEdit{nullptr};
    QLineEdit  *m_cmdEdit{nullptr};
    QLineEdit  *m_entrypointEdit{nullptr};
    QComboBox  *m_restartPolicy{nullptr};
    QCheckBox  *m_detachCheck{nullptr};
    QCheckBox  *m_removeCheck{nullptr};
    QCheckBox  *m_privilegedCheck{nullptr};
    QTableWidget *m_portsTable{nullptr};
    QTableWidget *m_envTable{nullptr};
    QTableWidget *m_volumesTable{nullptr};
    QLabel     *m_statusLabel{nullptr};
    QLabel     *m_privilegedWarning{nullptr};
    QPushButton *m_runBtn{nullptr};
};

} // namespace Features::Images
