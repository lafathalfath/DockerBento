#include <QApplication>
#include <QFont>
#include "shell/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DockerBento");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("DockerBento");

    // Dark theme base palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText,      QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Base,            QColor(26, 26, 26));
    darkPalette.setColor(QPalette::AlternateBase,   QColor(29, 29, 29));
    darkPalette.setColor(QPalette::ToolTipBase,     QColor(42, 42, 42));
    darkPalette.setColor(QPalette::ToolTipText,     QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Text,            QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Button,          QColor(51, 51, 51));
    darkPalette.setColor(QPalette::ButtonText,      QColor(224, 224, 224));
    darkPalette.setColor(QPalette::BrightText,      Qt::white);
    darkPalette.setColor(QPalette::Highlight,       QColor(21, 101, 192));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::Link,            QColor(66, 165, 245));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text,       QColor(96, 96, 96));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(96, 96, 96));
    app.setPalette(darkPalette);

    QFont appFont("Inter", 10);
    appFont.setHintingPreference(QFont::PreferFullHinting);
    app.setFont(appFont);

    Shell::MainWindow window;
    window.show();

    return app.exec();
}
