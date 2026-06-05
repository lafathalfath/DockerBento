#pragma once
#include <QLineEdit>

namespace Shared {

class SearchBar : public QLineEdit {
    Q_OBJECT
public:
    explicit SearchBar(const QString &placeholder = "Search...", QWidget *parent = nullptr);
};

} // namespace Shared
