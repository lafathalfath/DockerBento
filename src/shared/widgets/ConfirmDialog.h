#pragma once
#include <QDialog>

namespace Shared {

class ConfirmDialog : public QDialog {
    Q_OBJECT
public:
    static bool confirm(QWidget *parent, const QString &title,
                        const QString &message,
                        const QString &confirmText = "Confirm",
                        bool destructive = false);
};

} // namespace Shared
