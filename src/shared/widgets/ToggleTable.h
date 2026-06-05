#pragma once
#include <QTableWidget>
#include <QMouseEvent>

namespace Shared {

class ToggleTable : public QTableWidget {
    Q_OBJECT
public:
    explicit ToggleTable(QWidget *parent = nullptr) : QTableWidget(parent) {}

signals:
    void selectionCleared();

protected:
    void mousePressEvent(QMouseEvent *e) override {
        const int clickedRow = rowAt(e->pos().y());

        if (clickedRow >= 0 && clickedRow == m_lastSelectedRow && !selectedItems().isEmpty()) {
            // Second click on same row — deselect and swallow the event
            clearSelection();
            setCurrentIndex(QModelIndex());
            m_lastSelectedRow = -1;
            emit selectionCleared();
            return; // don't call base — prevents Qt from re-selecting
        }

        QTableWidget::mousePressEvent(e);

        // Track which row is now selected
        m_lastSelectedRow = currentRow();
    }

private:
    int m_lastSelectedRow{-1};
};

} // namespace Shared
