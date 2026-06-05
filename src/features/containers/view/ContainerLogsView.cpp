#include "ContainerLogsView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QScrollBar>

namespace Features::Containers {

ContainerLogsView::ContainerLogsView(ContainerDetailViewModel *vm, QWidget *parent)
    : QWidget(parent), m_vm(vm)
{
    setupUi();
    connect(m_vm, &ContainerDetailViewModel::logLine, this, [this](const QString &line) {
        m_logView->appendPlainText(line);
        auto *sb = m_logView->verticalScrollBar();
        sb->setValue(sb->maximum());
    });
    connect(m_vm, &ContainerDetailViewModel::logStreamFinished, this, [this]() {
        m_logView->appendPlainText("\n--- stream ended ---");
    });
}

void ContainerLogsView::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet("background-color: #212121; border-bottom: 1px solid #333;");
    auto *tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(12, 4, 12, 4);

    auto *streamBtn = new QPushButton("Stream Logs");
    streamBtn->setCheckable(true);
    streamBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #e0e0e0; border: 1px solid #555;"
        "border-radius: 4px; padding: 3px 10px; }"
        "QPushButton:checked { background-color: #1565c0; border-color: #1976d2; }"
    );

    auto *clearBtn = new QPushButton("Clear");
    clearBtn->setStyleSheet(
        "QPushButton { background-color: #333; color: #e0e0e0; border: 1px solid #555;"
        "border-radius: 4px; padding: 3px 10px; }"
        "QPushButton:hover { background-color: #444; }"
    );

    tl->addWidget(streamBtn);
    tl->addWidget(clearBtn);
    tl->addStretch();

    m_logView = new QPlainTextEdit;
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(5000);
    m_logView->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #0d1117;"
        "  color: #58a6ff;"
        "  font-family: 'Fira Code', 'Monospace', monospace;"
        "  font-size: 12px;"
        "  border: none;"
        "  padding: 8px;"
        "}"
    );

    root->addWidget(toolbar);
    root->addWidget(m_logView);

    connect(streamBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) m_vm->startLogStream();
        else         m_vm->stopLogStream();
    });
    connect(clearBtn, &QPushButton::clicked, m_logView, &QPlainTextEdit::clear);
}

void ContainerLogsView::attachContainer(const Core::Container &container) {
    m_container = container;
    m_logView->clear();
    m_logView->setPlaceholderText(QString("Logs for %1\nClick 'Stream Logs' to start").arg(container.name));
}

} // namespace Features::Containers
