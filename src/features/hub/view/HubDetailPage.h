#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include "shared/widgets/ToggleTable.h"
#include "features/hub/model/HubRepository.h"
#include "features/hub/viewmodel/HubSearchViewModel.h"

namespace Features::Hub {

class HubDetailPage : public QWidget {
    Q_OBJECT
public:
    explicit HubDetailPage(HubSearchViewModel *vm, QWidget *parent = nullptr);

    void showImage(const HubImage &img);

signals:
    void backRequested();
    void pullRequested(const QString &imageWithTag);

private:
    void setupUi();
    void loadTags();
    void refreshTagsTable();

    HubSearchViewModel  *m_vm;
    HubImage             m_image;

    QLabel              *m_nameLabel{nullptr};
    QLabel              *m_descLabel{nullptr};
    QLabel              *m_statsLabel{nullptr};
    QLabel              *m_tagsStatusLabel{nullptr};
    QPushButton         *m_prevTagsBtn{nullptr};
    QPushButton         *m_nextTagsBtn{nullptr};
    QLabel              *m_tagsPagination{nullptr};
    Shared::ToggleTable *m_tagsTable{nullptr};
    QStackedWidget      *m_tagsStack{nullptr};
};

} // namespace Features::Hub
