#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include "core/types/Container.h"
#include "features/containers/viewmodel/ContainerDetailViewModel.h"

namespace Features::Containers {

class ContainerLogsView : public QWidget {
    Q_OBJECT
public:
    explicit ContainerLogsView(ContainerDetailViewModel *vm, QWidget *parent = nullptr);
    void attachContainer(const Core::Container &container);

private:
    void setupUi();

    ContainerDetailViewModel *m_vm;
    QPlainTextEdit *m_logView{nullptr};
    Core::Container m_container;
};

} // namespace Features::Containers
