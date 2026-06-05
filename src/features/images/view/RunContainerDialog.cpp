#include "RunContainerDialog.h"
#include "core/docker/DockerClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QScrollArea>
#include <QUrl>

namespace Features::Images {

const QString RunContainerDialog::INPUT_STYLE =
    "QLineEdit, QComboBox, QSpinBox {"
    "  background-color: #2a2a2a; border: 1px solid #444; border-radius: 4px;"
    "  color: #e0e0e0; padding: 4px 8px; font-size: 13px; }"
    "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: #1976d2; }"
    "QComboBox::drop-down { border: none; }"
    "QComboBox QAbstractItemView { background-color: #2a2a2a; color: #e0e0e0;"
    "  selection-background-color: #1565c0; }";

const QString RunContainerDialog::BTN_STYLE_ADD =
    "QPushButton { background-color: #333; color: #aaa; border: 1px dashed #555;"
    "  border-radius: 4px; padding: 3px 10px; font-size: 12px; }"
    "QPushButton:hover { background-color: #3a3a3a; color: #ccc; }";

static const QString GROUP_STYLE =
    "QGroupBox { color: #aaa; font-size: 12px; border: 1px solid #333;"
    "  border-radius: 6px; margin-top: 8px; padding: 12px 12px 8px 12px; }"
    "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left;"
    "  padding: 0 6px; left: 12px; }";

static const QString TABLE_STYLE =
    "QTableWidget { background-color: #222; color: #e0e0e0; border: 1px solid #333;"
    "  border-radius: 4px; }"
    "QTableWidget::item { padding: 2px 4px; }"
    "QHeaderView::section { background-color: #2a2a2a; color: #888; border: none;"
    "  border-bottom: 1px solid #333; padding: 4px 6px; font-size: 11px; }";

RunContainerDialog::RunContainerDialog(const Core::Image &image,
                                       Core::DockerClient *docker,
                                       QWidget *parent)
    : QDialog(parent), m_image(image), m_docker(docker)
{
    setWindowTitle(QString("Run Container — %1").arg(image.displayName()));
    setMinimumSize(560, 500);
    resize(580, 620);
    setModal(true);
    setStyleSheet("QDialog { background-color: #1e1e1e; }" + INPUT_STYLE);
    setupUi();
}

void RunContainerDialog::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: #1e1e1e; }");

    auto *scrollContent = new QWidget;
    auto *form = new QVBoxLayout(scrollContent);
    form->setSpacing(12);
    form->setContentsMargins(20, 16, 20, 8);

    // ── Image info ───────────────────────────────────────────
    auto *imgLabel = new QLabel(QString("Image: <b>%1</b>").arg(m_image.displayName()));
    imgLabel->setStyleSheet("color: #888; font-size: 12px;");
    form->addWidget(imgLabel);

    // ── Basic ────────────────────────────────────────────────
    auto *basicGroup = new QGroupBox("Basic");
    basicGroup->setStyleSheet(GROUP_STYLE);
    auto *basicForm = new QFormLayout(basicGroup);
    basicForm->setSpacing(8);

    auto makeLabel = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setStyleSheet("color: #aaa; font-size: 12px;");
        return l;
    };

    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("auto-generated if empty");
    m_cmdEdit = new QLineEdit;
    m_cmdEdit->setPlaceholderText("override CMD (e.g. /bin/bash)");
    m_entrypointEdit = new QLineEdit;
    m_entrypointEdit->setPlaceholderText("override ENTRYPOINT");

    basicForm->addRow(makeLabel("Name"), m_nameEdit);
    basicForm->addRow(makeLabel("Command"), m_cmdEdit);
    basicForm->addRow(makeLabel("Entrypoint"), m_entrypointEdit);
    form->addWidget(basicGroup);

    // ── Ports ────────────────────────────────────────────────
    auto *portsGroup = new QGroupBox("Port Mapping");
    portsGroup->setStyleSheet(GROUP_STYLE);
    auto *portsLay = new QVBoxLayout(portsGroup);
    portsLay->setSpacing(4);

    m_portsTable = new QTableWidget(0, 3);
    m_portsTable->setHorizontalHeaderLabels({"Host Port", "Container Port", "Protocol"});
    m_portsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_portsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_portsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_portsTable->verticalHeader()->setVisible(false);
    m_portsTable->setMaximumHeight(130);
    m_portsTable->setStyleSheet(TABLE_STYLE);

    auto *addPortBtn = new QPushButton("+ Add Port");
    addPortBtn->setStyleSheet(BTN_STYLE_ADD);
    connect(addPortBtn, &QPushButton::clicked, this, [this]() { addPortRow(); });

    portsLay->addWidget(m_portsTable);
    portsLay->addWidget(addPortBtn);
    form->addWidget(portsGroup);

    // ── Environment Variables ────────────────────────────────
    auto *envGroup = new QGroupBox("Environment Variables");
    envGroup->setStyleSheet(GROUP_STYLE);
    auto *envLay = new QVBoxLayout(envGroup);
    envLay->setSpacing(4);

    m_envTable = new QTableWidget(0, 2);
    m_envTable->setHorizontalHeaderLabels({"Key", "Value"});
    m_envTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_envTable->verticalHeader()->setVisible(false);
    m_envTable->setMaximumHeight(130);
    m_envTable->setStyleSheet(TABLE_STYLE);

    auto *addEnvBtn = new QPushButton("+ Add Variable");
    addEnvBtn->setStyleSheet(BTN_STYLE_ADD);
    connect(addEnvBtn, &QPushButton::clicked, this, [this]() { addEnvRow(); });

    envLay->addWidget(m_envTable);
    envLay->addWidget(addEnvBtn);
    form->addWidget(envGroup);

    // ── Volumes ──────────────────────────────────────────────
    auto *volGroup = new QGroupBox("Volume Mounts");
    volGroup->setStyleSheet(GROUP_STYLE);
    auto *volLay = new QVBoxLayout(volGroup);
    volLay->setSpacing(4);

    m_volumesTable = new QTableWidget(0, 2);
    m_volumesTable->setHorizontalHeaderLabels({"Host Path", "Container Path"});
    m_volumesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_volumesTable->verticalHeader()->setVisible(false);
    m_volumesTable->setMaximumHeight(130);
    m_volumesTable->setStyleSheet(TABLE_STYLE);

    auto *addVolBtn = new QPushButton("+ Add Mount");
    addVolBtn->setStyleSheet(BTN_STYLE_ADD);
    connect(addVolBtn, &QPushButton::clicked, this, [this]() { addVolumeRow(); });

    volLay->addWidget(m_volumesTable);
    volLay->addWidget(addVolBtn);
    form->addWidget(volGroup);

    // ── Options ──────────────────────────────────────────────
    auto *optGroup = new QGroupBox("Options");
    optGroup->setStyleSheet(GROUP_STYLE);
    auto *optLay = new QVBoxLayout(optGroup);
    optLay->setSpacing(6);

    auto *restartRow = new QHBoxLayout;
    auto *restartLabel = makeLabel("Restart policy");
    m_restartPolicy = new QComboBox;
    m_restartPolicy->addItems({"no", "always", "on-failure", "unless-stopped"});
    restartRow->addWidget(restartLabel);
    restartRow->addWidget(m_restartPolicy, 1);
    optLay->addLayout(restartRow);

    QString checkStyle =
        "QCheckBox { color: #ccc; font-size: 13px; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
        "QCheckBox::indicator:unchecked { border: 2px solid #555; border-radius: 3px; background: #2a2a2a; }"
        "QCheckBox::indicator:checked { border: 2px solid #1976d2; border-radius: 3px; background: #1976d2; }";

    m_detachCheck = new QCheckBox("Run in background (detached)");
    m_detachCheck->setChecked(true);
    m_detachCheck->setStyleSheet(checkStyle);

    m_removeCheck = new QCheckBox("Auto-remove when stopped");
    m_removeCheck->setStyleSheet(checkStyle);

    m_privilegedCheck = new QCheckBox("Privileged mode");
    m_privilegedCheck->setStyleSheet(checkStyle);

    m_privilegedWarning = new QLabel(
        "⚠ Privileged mode grants the container full access to the host kernel.\n"
        "Only enable this for trusted containers.");
    m_privilegedWarning->setStyleSheet(
        "color: #ff9800; font-size: 11px; padding: 4px 8px;"
        "background-color: #2a2000; border: 1px solid #5a4000; border-radius: 4px;");
    m_privilegedWarning->setWordWrap(true);
    m_privilegedWarning->setVisible(false);

    connect(m_privilegedCheck, &QCheckBox::toggled, m_privilegedWarning, &QLabel::setVisible);

    optLay->addWidget(m_detachCheck);
    optLay->addWidget(m_removeCheck);
    optLay->addWidget(m_privilegedCheck);
    optLay->addWidget(m_privilegedWarning);
    form->addWidget(optGroup);

    form->addStretch();

    scrollArea->setWidget(scrollContent);

    // ── Bottom bar (outside scroll) ──────────────────────────
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(scrollArea, 1);

    auto *bottomBar = new QWidget;
    bottomBar->setFixedHeight(56);
    bottomBar->setStyleSheet("background-color: #181818; border-top: 1px solid #333;");
    auto *bl = new QHBoxLayout(bottomBar);
    bl->setContentsMargins(20, 0, 20, 0);

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");

    auto *cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #ccc; border: 1px solid #555;"
        "  border-radius: 4px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #444; }");

    m_runBtn = new QPushButton("Run Container");
    m_runBtn->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; border-radius: 4px;"
        "  padding: 6px 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: #388e3c; }"
        "QPushButton:disabled { background-color: #333; color: #666; }");

    bl->addWidget(m_statusLabel, 1);
    bl->addWidget(cancelBtn);
    bl->addWidget(m_runBtn);
    root->addWidget(bottomBar);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_runBtn, &QPushButton::clicked, this, &RunContainerDialog::onRun);
}

void RunContainerDialog::addPortRow(const QString &host, const QString &container, const QString &proto) {
    int row = m_portsTable->rowCount();
    m_portsTable->insertRow(row);
    auto *hostItem = new QTableWidgetItem(host);
    auto *contItem = new QTableWidgetItem(container);
    auto *protoItem = new QTableWidgetItem(proto);
    m_portsTable->setItem(row, 0, hostItem);
    m_portsTable->setItem(row, 1, contItem);
    m_portsTable->setItem(row, 2, protoItem);
    m_portsTable->setRowHeight(row, 28);
}

void RunContainerDialog::addEnvRow(const QString &key, const QString &val) {
    int row = m_envTable->rowCount();
    m_envTable->insertRow(row);
    m_envTable->setItem(row, 0, new QTableWidgetItem(key));
    m_envTable->setItem(row, 1, new QTableWidgetItem(val));
    m_envTable->setRowHeight(row, 28);
}

void RunContainerDialog::addVolumeRow(const QString &host, const QString &container) {
    int row = m_volumesTable->rowCount();
    m_volumesTable->insertRow(row);
    m_volumesTable->setItem(row, 0, new QTableWidgetItem(host));
    m_volumesTable->setItem(row, 1, new QTableWidgetItem(container));
    m_volumesTable->setRowHeight(row, 28);
}

QJsonDocument RunContainerDialog::buildCreateBody() const {
    QJsonObject body;

    // Image
    body["Image"] = m_image.displayName();

    // Cmd
    QString cmd = m_cmdEdit->text().trimmed();
    if (!cmd.isEmpty()) {
        QJsonArray cmdArr;
        for (const auto &part : cmd.split(' ', Qt::SkipEmptyParts))
            cmdArr.append(part);
        body["Cmd"] = cmdArr;
    }

    // Entrypoint
    QString ep = m_entrypointEdit->text().trimmed();
    if (!ep.isEmpty()) {
        QJsonArray epArr;
        for (const auto &part : ep.split(' ', Qt::SkipEmptyParts))
            epArr.append(part);
        body["Entrypoint"] = epArr;
    }

    // Env
    QJsonArray envArr;
    for (int i = 0; i < m_envTable->rowCount(); ++i) {
        QString k = m_envTable->item(i, 0) ? m_envTable->item(i, 0)->text().trimmed() : "";
        QString v = m_envTable->item(i, 1) ? m_envTable->item(i, 1)->text().trimmed() : "";
        if (!k.isEmpty())
            envArr.append(k + "=" + v);
    }
    if (!envArr.isEmpty())
        body["Env"] = envArr;

    // ExposedPorts + PortBindings
    QJsonObject exposedPorts;
    QJsonObject portBindings;
    for (int i = 0; i < m_portsTable->rowCount(); ++i) {
        QString hp   = m_portsTable->item(i, 0) ? m_portsTable->item(i, 0)->text().trimmed() : "";
        QString cp   = m_portsTable->item(i, 1) ? m_portsTable->item(i, 1)->text().trimmed() : "";
        QString proto = m_portsTable->item(i, 2) ? m_portsTable->item(i, 2)->text().trimmed() : "tcp";
        if (cp.isEmpty()) continue;
        if (proto.isEmpty()) proto = "tcp";

        QString key = cp + "/" + proto;
        exposedPorts[key] = QJsonObject{};

        QJsonArray bindings;
        QJsonObject binding;
        binding["HostPort"] = hp;
        bindings.append(binding);
        portBindings[key] = bindings;
    }
    if (!exposedPorts.isEmpty())
        body["ExposedPorts"] = exposedPorts;

    // HostConfig
    QJsonObject hostConfig;

    if (!portBindings.isEmpty())
        hostConfig["PortBindings"] = portBindings;

    // Binds (volumes)
    QJsonArray binds;
    for (int i = 0; i < m_volumesTable->rowCount(); ++i) {
        QString hpath = m_volumesTable->item(i, 0) ? m_volumesTable->item(i, 0)->text().trimmed() : "";
        QString cpath = m_volumesTable->item(i, 1) ? m_volumesTable->item(i, 1)->text().trimmed() : "";
        if (!hpath.isEmpty() && !cpath.isEmpty())
            binds.append(hpath + ":" + cpath);
    }
    if (!binds.isEmpty())
        hostConfig["Binds"] = binds;

    // Restart policy
    QString policy = m_restartPolicy->currentText();
    QJsonObject restartObj;
    if (policy == "on-failure") {
        restartObj["Name"] = "on-failure";
        restartObj["MaximumRetryCount"] = 5;
    } else {
        restartObj["Name"] = policy;
    }
    hostConfig["RestartPolicy"] = restartObj;

    // Auto-remove
    if (m_removeCheck->isChecked())
        hostConfig["AutoRemove"] = true;

    // Privileged
    if (m_privilegedCheck->isChecked())
        hostConfig["Privileged"] = true;

    body["HostConfig"] = hostConfig;

    return QJsonDocument(body);
}

void RunContainerDialog::onRun() {
    m_runBtn->setEnabled(false);
    m_statusLabel->setText("Creating container...");
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");

    QString name = m_nameEdit->text().trimmed();
    QString path = "/v1.41/containers/create";
    if (!name.isEmpty())
        path += "?name=" + QString::fromUtf8(QUrl::toPercentEncoding(name));

    QJsonDocument body = buildCreateBody();

    m_docker->postJson(path, body, [this](bool ok, const QJsonDocument &doc, const QString &err) {
        if (!ok) {
            m_runBtn->setEnabled(true);
            m_statusLabel->setText("Create failed: " + err);
            m_statusLabel->setStyleSheet("color: #f44336; font-size: 12px;");
            return;
        }

        QString containerId = doc.object()["Id"].toString();
        m_statusLabel->setText("Starting container...");

        // Start the container
        m_docker->post(QString("/v1.41/containers/%1/start").arg(containerId), {},
            [this, containerId](bool ok2, const QString &err2) {
                m_runBtn->setEnabled(true);
                if (!ok2) {
                    m_statusLabel->setText("Created but start failed: " + err2);
                    m_statusLabel->setStyleSheet("color: #ff9800; font-size: 12px;");
                    return;
                }
                m_statusLabel->setText("Container started: " + containerId.left(12));
                m_statusLabel->setStyleSheet("color: #4caf50; font-size: 12px;");
                accept();
            });
    });
}

} // namespace Features::Images
