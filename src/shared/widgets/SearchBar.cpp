#include "SearchBar.h"

namespace Shared {

SearchBar::SearchBar(const QString &placeholder, QWidget *parent)
    : QLineEdit(parent)
{
    setPlaceholderText(placeholder);
    setClearButtonEnabled(true);
    setFixedHeight(34);
    setStyleSheet(
        "QLineEdit {"
        "  background-color: #2d2d2d;"
        "  border: 1px solid #444;"
        "  border-radius: 17px;"
        "  padding: 0 12px 0 32px;"
        "  color: #e0e0e0;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #1976d2;"
        "}"
    );
}

} // namespace Shared
