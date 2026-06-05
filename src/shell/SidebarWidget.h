#pragma once
#include <QWidget>
#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>

namespace Shell {

enum class Section { Containers, Images, Volumes, Networks };

class SidebarWidget : public QWidget {
    Q_OBJECT
public:
    explicit SidebarWidget(QWidget *parent = nullptr);

    void setDockerConnected(bool connected);
    void showConnectionError();

signals:
    void sectionChanged(Section section);
    void settingsRequested();

private:
    void setupUi();
    QPushButton *makeNavButton(const QString &icon, const QString &label);

    QButtonGroup *m_btnGroup{nullptr};
    QWidget      *m_connectionDot{nullptr};
    QLabel       *m_connectionLabel{nullptr};
    QPushButton  *m_settingsBtn{nullptr};
    QLabel       *m_errorHint{nullptr};
};

} // namespace Shell
