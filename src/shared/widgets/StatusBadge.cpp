#include "StatusBadge.h"

namespace Shared {

StatusBadge::StatusBadge(QWidget *parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setFixedHeight(22);
    setMinimumWidth(70);
}

void StatusBadge::setStatus(const QString &text, const QString &color) {
    setText(text);
    setStyleSheet(QString(
        "QLabel {"
        "  background-color: %1;"
        "  color: white;"
        "  border-radius: 11px;"
        "  padding: 0 10px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "}"
    ).arg(color));
}

} // namespace Shared
