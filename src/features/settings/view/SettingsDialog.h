#pragma once
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QStackedWidget>
#include "features/settings/model/ConnectionSettings.h"

namespace Core { class DockerClient; }

namespace Features::Settings {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(Core::DockerClient *docker, QWidget *parent = nullptr);

signals:
    void settingsApplied(const ConnectionSettings &cfg);

private:
    void setupUi();
    void loadCurrent();
    void onTestConnection();
    void onApply();
    void onTypeChanged();
    void setTestStatus(const QString &msg, const QString &color);

    Core::DockerClient *m_docker;

    // Connection type
    QRadioButton *m_rbSocket{nullptr};
    QRadioButton *m_rbTcp{nullptr};

    // Unix socket
    QLineEdit *m_socketPathEdit{nullptr};
    QPushButton *m_detectBtn{nullptr};

    // TCP
    QLineEdit *m_tcpHostEdit{nullptr};
    QSpinBox  *m_tcpPortSpin{nullptr};

    // Stack shows socket/TCP fields
    QStackedWidget *m_typeStack{nullptr};

    // Timeout
    QSpinBox *m_timeoutSpin{nullptr};

    // Status
    QLabel *m_testStatusLabel{nullptr};
    QPushButton *m_testBtn{nullptr};
    QPushButton *m_applyBtn{nullptr};
};

} // namespace Features::Settings
