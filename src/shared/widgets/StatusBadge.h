#pragma once
#include <QLabel>

namespace Shared {

class StatusBadge : public QLabel {
    Q_OBJECT
public:
    explicit StatusBadge(QWidget *parent = nullptr);
    void setStatus(const QString &text, const QString &color);
};

} // namespace Shared
