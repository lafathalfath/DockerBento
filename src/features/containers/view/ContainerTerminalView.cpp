#include "ContainerTerminalView.h"
#include <qtermwidget.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QTimer>

namespace Features::Containers {

ContainerTerminalView::ContainerTerminalView(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void ContainerTerminalView::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Toolbar
    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet("background-color: #212121; border-bottom: 1px solid #333;");
    auto *tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(12, 4, 12, 4);

    m_shellCombo = new QComboBox;
    m_shellCombo->addItems({"/bin/sh", "/bin/bash", "/bin/ash"});
    m_shellCombo->setStyleSheet(
        "QComboBox { background-color: #333; color: #e0e0e0; border: 1px solid #555;"
        "border-radius: 4px; padding: 3px 8px; min-width: 100px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #2a2a2a; color: #e0e0e0;"
        "selection-background-color: #1565c0; }");

    m_connectBtn = new QPushButton("Connect");
    m_connectBtn->setCheckable(true);
    m_connectBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #e0e0e0; border: 1px solid #555;"
        "border-radius: 4px; padding: 3px 10px; }"
        "QPushButton:checked { background-color: #2e7d32; border-color: #388e3c; }"
        "QPushButton:hover { background-color: #444; }");

    tl->addWidget(m_shellCombo);
    tl->addWidget(m_connectBtn);
    tl->addStretch();

    // Stack: page 0 = placeholder, page 1 = terminal (created later)
    m_stack = new QStackedWidget;

    m_placeholder = new QLabel("Select a shell and click Connect");
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setStyleSheet("color: #555; background-color: #0d1117;");
    m_stack->addWidget(m_placeholder); // index 0

    root->addWidget(toolbar);
    root->addWidget(m_stack, 1);

    connect(m_connectBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) {
            if (!m_container.isRunning()) {
                m_connectBtn->setChecked(false);
                return;
            }
            m_connectBtn->setText("Disconnect");
            m_shellCombo->setEnabled(false);
            startSession();
        } else {
            stopSession();
        }
    });
}

void ContainerTerminalView::startSession() {
    // Remove old terminal page if any
    if (m_term) {
        m_stack->removeWidget(m_term);
        m_term->deleteLater();
        m_term = nullptr;
    }

    m_term = new QTermWidget(0, this);
    m_term->setShellProgram("/usr/bin/docker");
    m_term->setArgs({"exec", "-it", m_container.id, m_shellCombo->currentText()});
    m_term->setColorScheme("Linux");

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    m_term->setTerminalFont(font);
    m_term->setScrollBarPosition(QTermWidget::ScrollBarRight);
    m_term->setHistorySize(5000);
    m_term->setBlinkingCursor(true);

    m_stack->addWidget(m_term);     // index 1
    m_stack->setCurrentWidget(m_term);

    // Defer start so the widget is fully laid out before PTY size is computed
    QTimer::singleShot(0, m_term, [this]() {
        if (m_term) m_term->startShellProgram();
    });

    connect(m_term, &QTermWidget::finished, this, [this]() {
        m_connectBtn->setChecked(false); // triggers toggled(false) → stopSession()
    });
}

void ContainerTerminalView::stopSession() {
    if (m_term) {
        m_stack->removeWidget(m_term);
        m_term->deleteLater();
        m_term = nullptr;
    }
    m_placeholder->setText("Session ended. Click Connect to start a new session.");
    m_stack->setCurrentWidget(m_placeholder);
    m_connectBtn->setText("Connect");
    m_shellCombo->setEnabled(true);
}

void ContainerTerminalView::attachContainer(const Core::Container &container) {
    stopSession();
    m_container = container;
    m_placeholder->setText("Select a shell and click Connect");
    m_connectBtn->setChecked(false);
    m_connectBtn->setText("Connect");
    m_connectBtn->setEnabled(container.isRunning());
    m_shellCombo->setEnabled(true);
}

} // namespace Features::Containers
