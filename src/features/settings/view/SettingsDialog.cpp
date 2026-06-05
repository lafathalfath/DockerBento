#include "SettingsDialog.h"
#include "core/docker/DockerClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileInfo>
#include <QDir>
#include <unistd.h>

namespace Features::Settings {

static const QString STYLE_GROUPBOX =
    "QGroupBox {"
    "  color: #aaa;"
    "  font-size: 12px;"
    "  border: 1px solid #333;"
    "  border-radius: 6px;"
    "  margin-top: 8px;"
    "  padding: 12px 12px 8px 12px;"
    "}"
    "QGroupBox::title {"
    "  subcontrol-origin: margin;"
    "  subcontrol-position: top left;"
    "  padding: 0 6px;"
    "  left: 12px;"
    "}";

static const QString STYLE_INPUT =
    "QLineEdit, QSpinBox {"
    "  background-color: #2a2a2a;"
    "  border: 1px solid #444;"
    "  border-radius: 4px;"
    "  color: #e0e0e0;"
    "  padding: 4px 8px;"
    "  font-size: 13px;"
    "}"
    "QLineEdit:focus, QSpinBox:focus { border-color: #1976d2; }";

static const QString STYLE_RADIO =
    "QRadioButton { color: #ccc; font-size: 13px; spacing: 6px; }"
    "QRadioButton::indicator { width: 16px; height: 16px; }"
    "QRadioButton::indicator:unchecked {"
    "  border: 2px solid #555; border-radius: 8px; background: #2a2a2a; }"
    "QRadioButton::indicator:checked {"
    "  border: 2px solid #1976d2; border-radius: 8px; background: #1976d2; }";

SettingsDialog::SettingsDialog(Core::DockerClient *docker, QWidget *parent)
    : QDialog(parent), m_docker(docker)
{
    setWindowTitle("Docker Connection Settings");
    setMinimumWidth(500);
    setModal(true);
    setStyleSheet("QDialog { background-color: #1e1e1e; }");
    setupUi();
    loadCurrent();
}

void SettingsDialog::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setSpacing(16);
    root->setContentsMargins(20, 20, 20, 20);

    // ── Header ───────────────────────────────────────────────
    auto *titleLabel = new QLabel("Docker Connection");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #e0e0e0;");
    auto *subtitleLabel = new QLabel("Configure how DockerBento connects to the Docker daemon.");
    subtitleLabel->setStyleSheet("color: #888; font-size: 12px;");
    root->addWidget(titleLabel);
    root->addWidget(subtitleLabel);

    // ── Connection type ───────────────────────────────────────
    auto *typeGroup = new QGroupBox("Connection type");
    typeGroup->setStyleSheet(STYLE_GROUPBOX);
    auto *typeLayout = new QVBoxLayout(typeGroup);
    typeLayout->setSpacing(8);

    m_rbSocket = new QRadioButton("Unix Socket  (local Docker daemon)");
    m_rbTcp    = new QRadioButton("TCP  (remote / Docker Desktop / Podman)");
    m_rbSocket->setStyleSheet(STYLE_RADIO);
    m_rbTcp->setStyleSheet(STYLE_RADIO);
    m_rbSocket->setChecked(true);

    typeLayout->addWidget(m_rbSocket);
    typeLayout->addWidget(m_rbTcp);
    root->addWidget(typeGroup);

    // ── Type-specific fields (stacked) ─────────────────────────
    m_typeStack = new QStackedWidget;

    // Page 0: Unix socket
    auto *socketPage = new QWidget;
    auto *socketForm = new QFormLayout(socketPage);
    socketForm->setSpacing(8);
    socketForm->setContentsMargins(0, 0, 0, 0);

    m_socketPathEdit = new QLineEdit;
    m_socketPathEdit->setPlaceholderText("/var/run/docker.sock");
    m_socketPathEdit->setStyleSheet(STYLE_INPUT);

    m_detectBtn = new QPushButton("Auto-detect");
    m_detectBtn->setFixedWidth(110);
    m_detectBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #ccc; border: 1px solid #555;"
        "border-radius: 4px; padding: 4px 10px; }"
        "QPushButton:hover { background-color: #444; }");

    auto *socketRow = new QHBoxLayout;
    socketRow->setSpacing(8);
    socketRow->addWidget(m_socketPathEdit);
    socketRow->addWidget(m_detectBtn);

    auto *pathLabel = new QLabel("Socket path");
    pathLabel->setStyleSheet("color: #aaa; font-size: 12px;");
    socketForm->addRow(pathLabel, socketRow);

    // Help text for common paths
    auto *helpLabel = new QLabel(
        "<span style='color:#666; font-size:11px;'>"
        "Common paths: &nbsp;"
        "<code style='color:#888'>/var/run/docker.sock</code> &nbsp;·&nbsp; "
        "<code style='color:#888'>/run/user/1000/docker.sock</code> (rootless) &nbsp;·&nbsp; "
        "<code style='color:#888'>$DOCKER_HOST</code>"
        "</span>");
    helpLabel->setWordWrap(true);
    socketForm->addRow("", helpLabel);

    // Page 1: TCP
    auto *tcpPage = new QWidget;
    auto *tcpForm = new QFormLayout(tcpPage);
    tcpForm->setSpacing(8);
    tcpForm->setContentsMargins(0, 0, 0, 0);

    m_tcpHostEdit = new QLineEdit;
    m_tcpHostEdit->setPlaceholderText("localhost");
    m_tcpHostEdit->setStyleSheet(STYLE_INPUT);

    m_tcpPortSpin = new QSpinBox;
    m_tcpPortSpin->setRange(1, 65535);
    m_tcpPortSpin->setValue(2375);
    m_tcpPortSpin->setStyleSheet(STYLE_INPUT);

    auto *tcpHelpLabel = new QLabel(
        "<span style='color:#666; font-size:11px;'>"
        "Default port 2375 (unencrypted). Enable TCP in Docker daemon with "
        "<code style='color:#888'>-H tcp://0.0.0.0:2375</code>."
        "</span>");
    tcpHelpLabel->setWordWrap(true);

    auto *hostLabel = new QLabel("Host");
    hostLabel->setStyleSheet("color: #aaa; font-size: 12px;");
    auto *portLabel = new QLabel("Port");
    portLabel->setStyleSheet("color: #aaa; font-size: 12px;");
    tcpForm->addRow(hostLabel, m_tcpHostEdit);
    tcpForm->addRow(portLabel, m_tcpPortSpin);
    tcpForm->addRow("", tcpHelpLabel);

    m_typeStack->addWidget(socketPage); // index 0
    m_typeStack->addWidget(tcpPage);    // index 1
    root->addWidget(m_typeStack);

    // ── Advanced ──────────────────────────────────────────────
    auto *advGroup = new QGroupBox("Advanced");
    advGroup->setStyleSheet(STYLE_GROUPBOX);
    auto *advForm = new QFormLayout(advGroup);
    advForm->setSpacing(8);

    m_timeoutSpin = new QSpinBox;
    m_timeoutSpin->setRange(1000, 60000);
    m_timeoutSpin->setSingleStep(1000);
    m_timeoutSpin->setSuffix(" ms");
    m_timeoutSpin->setValue(10000);
    m_timeoutSpin->setStyleSheet(STYLE_INPUT);

    auto *toLabel = new QLabel("Request timeout");
    toLabel->setStyleSheet("color: #aaa; font-size: 12px;");
    advForm->addRow(toLabel, m_timeoutSpin);
    root->addWidget(advGroup);

    // ── Test + status ─────────────────────────────────────────
    auto *testRow = new QHBoxLayout;
    m_testBtn = new QPushButton("Test Connection");
    m_testBtn->setFixedWidth(140);
    m_testBtn->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; border-radius: 4px; padding: 6px 14px; }"
        "QPushButton:hover { background-color: #1976d2; }"
        "QPushButton:disabled { background-color: #333; color: #666; }");

    m_testStatusLabel = new QLabel;
    m_testStatusLabel->setWordWrap(true);
    m_testStatusLabel->setStyleSheet("font-size: 12px; color: #888;");

    testRow->addWidget(m_testBtn);
    testRow->addWidget(m_testStatusLabel, 1);
    root->addLayout(testRow);

    // ── Buttons ───────────────────────────────────────────────
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #333;");
    root->addWidget(sep);

    auto *btnRow = new QHBoxLayout;
    auto *cancelBtn = new QPushButton("Cancel");
    m_applyBtn = new QPushButton("Apply & Reconnect");
    cancelBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #ccc; border: 1px solid #555;"
        "border-radius: 4px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #444; }");
    m_applyBtn->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; border-radius: 4px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #388e3c; }");

    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(m_applyBtn);
    root->addLayout(btnRow);

    // ── Connections ───────────────────────────────────────────
    connect(m_rbSocket, &QRadioButton::toggled, this, &SettingsDialog::onTypeChanged);
    connect(m_detectBtn, &QPushButton::clicked, this, [this]() {
        // Try common socket paths in order
        QStringList candidates = {
            qEnvironmentVariable("DOCKER_HOST").remove("unix://"),
            QString("/run/user/%1/docker.sock").arg(getuid()),
            "/var/run/docker.sock",
            "/run/docker.sock",
        };
        for (const QString &p : candidates) {
            if (!p.isEmpty() && QFileInfo::exists(p)) {
                m_socketPathEdit->setText(p);
                setTestStatus(QString("Found: %1").arg(p), "#4caf50");
                return;
            }
        }
        setTestStatus("No Docker socket found. Is Docker running?", "#f44336");
    });
    connect(m_testBtn, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);
    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::loadCurrent() {
    auto cfg = ConnectionSettings::load();

    if (cfg.type == ConnectionType::UnixSocket) {
        m_rbSocket->setChecked(true);
    } else {
        m_rbTcp->setChecked(true);
    }

    m_socketPathEdit->setText(cfg.socketPath);
    m_tcpHostEdit->setText(cfg.tcpHost);
    m_tcpPortSpin->setValue(cfg.tcpPort);
    m_timeoutSpin->setValue(cfg.timeoutMs);
    onTypeChanged();
}

void SettingsDialog::onTypeChanged() {
    m_typeStack->setCurrentIndex(m_rbSocket->isChecked() ? 0 : 1);
}

void SettingsDialog::onTestConnection() {
    m_testBtn->setEnabled(false);
    setTestStatus("Testing...", "#888");

    // Temporarily apply settings to test
    ConnectionSettings cfg;
    cfg.type      = m_rbSocket->isChecked() ? ConnectionType::UnixSocket : ConnectionType::Tcp;
    cfg.socketPath = m_socketPathEdit->text().trimmed();
    cfg.tcpHost   = m_tcpHostEdit->text().trimmed();
    cfg.tcpPort   = m_tcpPortSpin->value();
    cfg.timeoutMs = m_timeoutSpin->value();

    m_docker->applySettings(cfg);
    m_docker->testConnection([this](bool ok, const QString &info, const QString &err) {
        m_testBtn->setEnabled(true);
        if (ok)
            setTestStatus("Connected — " + info, "#4caf50");
        else
            setTestStatus("Failed: " + err, "#f44336");
    });
}

void SettingsDialog::onApply() {
    ConnectionSettings cfg;
    cfg.type       = m_rbSocket->isChecked() ? ConnectionType::UnixSocket : ConnectionType::Tcp;
    cfg.socketPath = m_socketPathEdit->text().trimmed();
    cfg.tcpHost    = m_tcpHostEdit->text().trimmed();
    cfg.tcpPort    = m_tcpPortSpin->value();
    cfg.timeoutMs  = m_timeoutSpin->value();

    cfg.save();
    m_docker->applySettings(cfg);
    emit settingsApplied(cfg);
    accept();
}

void SettingsDialog::setTestStatus(const QString &msg, const QString &color) {
    m_testStatusLabel->setText(msg);
    m_testStatusLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(color));
}

} // namespace Features::Settings
