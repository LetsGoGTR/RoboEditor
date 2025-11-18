#ifndef APPLYPAGE_H
#define APPLYPAGE_H
#pragma once
#include <QList>
#include <QWidget>
#include <QQueue>

#include "SelectController.h"
#include "SelectStorage.h"

namespace Ui
{
    class ApplyPage;
}

class ProgressDialog;

class ApplyPage : public QWidget
{
    Q_OBJECT

      public:
        explicit ApplyPage(QWidget *parent = nullptr);
        ~ApplyPage();
        QString        selectedBackupDir;
        QList<QString> selectedControllerList;

      private:
        Ui::ApplyPage *ui;

        selectTableWidget *selectbackupWidget;
        selectcontroller  *selectcontrollerWidget;

        void showPasswordUI();
        void refreshList();
        void confirmSelection();
        void importAnyspace();

        QQueue<QString> applyQueue_;
        QString apiPassword_;
        int totalApplyRequests_;
        int completedApplyRequests_;
        int failedApplyRequests_;

        ProgressDialog *applyProgressDialog_ = nullptr;
        bool applyInProgress_ = false;

        // 사전 백업/적용 대상 관리
        int  preBackupTotal_      = 0;
        int  preBackupCompleted_  = 0;
        int  preBackupFailed_     = 0;
        bool preBackupInProgress_ = false;

        // 사전 백업 성공/실패 제어기 목록
        QStringList preBackupSuccessControllers_;
        QStringList preBackupFailedControllers_;

        // 실제 Apply에 사용할 대상 목록 (사전 백업 성공한 제어기만)
        QList<QString> applyTargetControllers_;

        QString determineBackupRoot() const;  // 사전 백업 루트 계산

      signals:
        void uiApplyClicked(const QString &target);

      private slots:
        void onAllAppliesCompleted();
        // 큐 처리용 슬롯
        void startApplyQueue();
        void processNextApply();
        void onApplyCompleted(const QString &serialNumber);
        void onApplyFailed(const QString &serialNumber, const QString &error);

        // 사전 백업용 슬롯
        void startPreBackup();
        void onPreBackupCompleted(const QString &serialNumber);
        void onPreBackupFailed(const QString &serialNumber, const QString &error);
};

#endif  // APPLYPAGE_H
