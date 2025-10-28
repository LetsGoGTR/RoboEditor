#include "BackupPage.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

BackupPage::BackupPage(QWidget* parent): QWidget(parent) {
    auto* target = new QLineEdit; target->setPlaceholderText("Controller IP or Robot ID");
    auto* btn = new QPushButton("Backup (UI only)");
    connect(btn, &QPushButton::clicked, this, [=]{
        emit requestBackup(target->text());
    });

    auto* ly = new QVBoxLayout(this);
    ly->addWidget(new QLabel("백업 실행 (Stub)"));
    ly->addWidget(target);
    ly->addWidget(btn);
    ly->addStretch();
    setLayout(ly);
}
