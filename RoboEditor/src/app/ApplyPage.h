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
        int totalApplyRequests_;
        int completedApplyRequests_;
        int failedApplyRequests_;

      signals:
        void uiApplyClicked(const QString &target);

      private slots:
        void onAllAppliesCompleted();
        // 큐 처리용 슬롯
        void startApplyQueue();
        void processNextApply();
        void onApplyCompleted(const QString &serialNumber);
        void onApplyFailed(const QString &serialNumber, const QString &error);
};

#endif  // APPLYPAGE_H
