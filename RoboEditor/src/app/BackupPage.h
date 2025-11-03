#ifndef BACKUPPAGE_H
#define BACKUPPAGE_H
#pragma once

#include <QWidget>

#include "SelectController.h"

namespace Ui
{
    class BackupPage;
}

class BackupPage : public QWidget
{
    Q_OBJECT

      public:
        explicit BackupPage(QWidget *parent = nullptr);
        ~BackupPage() override;
        QString        selectedBackupDir;
        QList<QString> selectedControllerList;

      private:
        Ui::BackupPage *ui;

        selectcontroller *selectcontrollerWidget;

        void confirmSelection();
        void importAnySpace();
        void updateConfirmState();

      signals:
        void uiBackupClicked(const QString &target);

      private slots:
};

#endif  // BACKUPPAGE_H
