#include "ConfirmDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace Shared {

bool ConfirmDialog::confirm(QWidget *parent, const QString &title,
                             const QString &message,
                             const QString &confirmText,
                             bool destructive)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(360);
    dlg.setModal(true);

    auto *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(16);
    layout->setContentsMargins(24, 20, 24, 20);

    auto *icon = new QLabel(destructive ? "⚠" : "?", &dlg);
    icon->setStyleSheet("font-size: 32px;");
    icon->setAlignment(Qt::AlignCenter);

    auto *msg = new QLabel(message, &dlg);
    msg->setWordWrap(true);
    msg->setAlignment(Qt::AlignCenter);

    auto *btnRow = new QHBoxLayout;
    auto *cancel = new QPushButton("Cancel", &dlg);
    auto *confirm = new QPushButton(confirmText, &dlg);

    if (destructive)
        confirm->setStyleSheet("QPushButton { background-color: #f44336; color: white; border-radius: 4px; padding: 6px 16px; }");
    else
        confirm->setStyleSheet("QPushButton { background-color: #2196f3; color: white; border-radius: 4px; padding: 6px 16px; }");

    cancel->setStyleSheet("QPushButton { background-color: #424242; color: white; border-radius: 4px; padding: 6px 16px; }");

    btnRow->addStretch();
    btnRow->addWidget(cancel);
    btnRow->addWidget(confirm);

    layout->addWidget(icon);
    layout->addWidget(msg);
    layout->addLayout(btnRow);

    QObject::connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(confirm, &QPushButton::clicked, &dlg, &QDialog::accept);

    return dlg.exec() == QDialog::Accepted;
}

} // namespace Shared
