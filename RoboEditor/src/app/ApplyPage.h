#ifndef APPLYPAGE_H
#define APPLYPAGE_H
#pragma once
#include <QList>
#include <QQueue>
#include <QWidget>

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

    // 공통 UI/로직
    void   showPasswordUI();
    void   refreshList();
    void   confirmSelection();
    void   importAnyspace();
    void   startPreBackup();                 // 사전 백업 진입
    void   startApplyQueue();                // 사전 백업 후 Apply 시작
    void   processNextApply();               // Apply 큐 처리
    void   updateApplyProgressStatus();      // 라벨(사전 백업/적용) 업데이트
    QString determineBackupRoot() const;     // 백업 루트 계산

    // Apply 관련
    QQueue<QString> applyQueue_;
    QString         apiPassword_;
    int             totalApplyRequests_;
    int             completedApplyRequests_;
    int             failedApplyRequests_;

    ProgressDialog *applyProgressDialog_ = nullptr;
    bool            applyInProgress_     = false;

    // 사전 백업 관련 상태
    int  preBackupTotal_       = 0;
    int  preBackupCompleted_   = 0;
    int  preBackupFailed_      = 0;
    bool preBackupInProgress_  = false;
    QStringList preBackupSuccessControllers_;
    QStringList preBackupFailedControllers_;

    // 실제 Apply 대상 (사전 백업 성공한 제어기만)
    QList<QString> applyTargetControllers_;

  signals:
    void uiApplyClicked(const QString &target);

  private slots:
    void onAllAppliesCompleted();

    // Apply 큐 처리용 슬롯
    void onApplyCompleted(const QString &serialNumber);
    void onApplyFailed(const QString &serialNumber, const QString &error);

    // 사전 백업용 슬롯
    void onPreBackupCompleted(const QString &serialNumber);
    void onPreBackupFailed(const QString &serialNumber, const QString &error);
};

#endif  // APPLYPAGE_H
