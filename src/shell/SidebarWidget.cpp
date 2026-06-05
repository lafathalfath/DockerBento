#include "SidebarWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace Shell {

SidebarWidget::SidebarWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void SidebarWidget::setupUi() {
    setFixedWidth(180);
    setStyleSheet("SidebarWidget { background-color: #141414; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // App header
    auto *header = new QWidget;
    header->setFixedHeight(56);
    header->setStyleSheet("background-color: #0d0d0d; border-bottom: 1px solid #222;");
    auto *hl = new QHBoxLayout(header);
    hl->setContentsMargins(16, 0, 16, 0);
    auto *logoLabel = new QLabel("🐋 DockerBento");
    logoLabel->setStyleSheet("color: #e0e0e0; font-size: 13px; font-weight: bold;");
    hl->addWidget(logoLabel);

    // Nav buttons
    auto *nav = new QWidget;
    auto *navLayout = new QVBoxLayout(nav);
    navLayout->setContentsMargins(8, 8, 8, 8);
    navLayout->setSpacing(2);

    m_btnGroup = new QButtonGroup(this);

    struct NavItem { QString icon; QString label; Section section; };
    const QList<NavItem> items = {
        {"▣", "Containers", Section::Containers},
        {"◈", "Images",     Section::Images},
        {"⬡", "Volumes",    Section::Volumes},
        {"⬢", "Networks",   Section::Networks},
    };

    int id = 0;
    for (const auto &item : items) {
        auto *btn = makeNavButton(item.icon, item.label);
        m_btnGroup->addButton(btn, id++);
        navLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, section = item.section]() {
            emit sectionChanged(section);
        });
    }

    if (auto *first = m_btnGroup->button(0))
        first->setChecked(true);

    navLayout->addStretch();

    // Connection error hint (hidden by default)
    m_errorHint = new QLabel("Click ⚙ Settings to\nconfigure connection");
    m_errorHint->setAlignment(Qt::AlignCenter);
    m_errorHint->setWordWrap(true);
    m_errorHint->setStyleSheet(
        "color: #f44336; font-size: 10px; padding: 6px 8px;"
        "background-color: #2a1010; border-radius: 4px; margin: 0 8px;");
    m_errorHint->setVisible(false);
    navLayout->addWidget(m_errorHint);

    // Footer: connection dot + settings button
    auto *footer = new QWidget;
    footer->setStyleSheet("background-color: #0d0d0d; border-top: 1px solid #222;");
    footer->setFixedHeight(52);
    auto *fl = new QVBoxLayout(footer);
    fl->setContentsMargins(12, 6, 12, 6);
    fl->setSpacing(4);

    // Status row
    auto *statusRow = new QHBoxLayout;
    statusRow->setSpacing(6);

    m_connectionDot = new QWidget;
    m_connectionDot->setFixedSize(8, 8);
    m_connectionDot->setStyleSheet("background-color: #f44336; border-radius: 4px;");

    m_connectionLabel = new QLabel("Disconnected");
    m_connectionLabel->setStyleSheet("color: #888; font-size: 11px;");

    m_settingsBtn = new QPushButton("⚙ Settings");
    m_settingsBtn->setFixedHeight(22);
    m_settingsBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #666; border: none;"
        "font-size: 11px; text-align: right; padding-right: 0; }"
        "QPushButton:hover { color: #aaa; }");

    statusRow->addWidget(m_connectionDot);
    statusRow->addWidget(m_connectionLabel, 1);
    statusRow->addWidget(m_settingsBtn);

    fl->addLayout(statusRow);

    root->addWidget(header);
    root->addWidget(nav, 1);
    root->addWidget(footer);

    connect(m_settingsBtn, &QPushButton::clicked, this, &SidebarWidget::settingsRequested);
}

QPushButton *SidebarWidget::makeNavButton(const QString &icon, const QString &label) {
    auto *btn = new QPushButton(icon + "  " + label);
    btn->setCheckable(true);
    btn->setFixedHeight(40);
    btn->setStyleSheet(
        "QPushButton { text-align: left; padding: 0 12px; border: none; border-radius: 6px;"
        "  color: #888; font-size: 13px; background: transparent; }"
        "QPushButton:hover  { background-color: #1e1e1e; color: #ccc; }"
        "QPushButton:checked { background-color: #1565c0; color: white; font-weight: bold; }"
    );
    return btn;
}

void SidebarWidget::setDockerConnected(bool connected) {
    m_connectionDot->setStyleSheet(
        connected ? "background-color: #4caf50; border-radius: 4px;"
                  : "background-color: #f44336; border-radius: 4px;");
    m_connectionLabel->setText(connected ? "Connected" : "Disconnected");
    m_connectionLabel->setStyleSheet(
        connected ? "color: #4caf50; font-size: 11px;" : "color: #888; font-size: 11px;");

    if (connected) m_errorHint->setVisible(false);
}

void SidebarWidget::showConnectionError() {
    m_errorHint->setVisible(true);
}

} // namespace Shell
