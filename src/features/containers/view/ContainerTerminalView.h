#pragma once
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>
#include "core/types/Container.h"

class QTermWidget;

namespace Features::Containers {

class ContainerTerminalView : public QWidget {
    Q_OBJECT
public:
    explicit ContainerTerminalView(QWidget *parent = nullptr);
    void attachContainer(const Core::Container &container);
    void stopSession();

private:
    void setupUi();
    void startSession();

    QStackedWidget *m_stack{nullptr};
    QLabel *m_placeholder{nullptr};
    QTermWidget *m_term{nullptr};
    QPushButton *m_connectBtn{nullptr};
    QComboBox *m_shellCombo{nullptr};
    Core::Container m_container;
};

} // namespace Features::Containers
