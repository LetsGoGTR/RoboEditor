#ifndef BACKUPPAGE_H
#define BACKUPPAGE_H
#pragma once

#include <QWidget>

#include "SelectController.h"

namespace Ui
{
    class BackupPage;
}

class ProgressDialog;

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

        // 백업 진행 상황 추적
        int totalBackupRequests_;
        int completedBackupRequests_;
        int failedBackupRequests_;

        ProgressDialog *backupProgressDialog_ = nullptr;

      signals:
        void uiBackupClicked(const QString &target);

      private slots:
        void onBackupCompleted(const QString &serialNumber);
        void onBackupFailed(const QString &serialNumber, const QString &error);
};

#endif  // BACKUPPAGE_H
