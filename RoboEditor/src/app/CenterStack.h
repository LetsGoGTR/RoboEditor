#ifndef CENTERSTACK_H
#define CENTERSTACK_H
#pragma once

#include <QWidget>
class QMainWindow;
class QStackedWidget;
class ComparePage;
class BackupPage;
class OpenFilePage;
class ApplyPage;
class ModifyPage;

class CenterStack : public QWidget
{
    Q_OBJECT
      public:
        explicit CenterStack(QWidget *parent = nullptr);
        void showCompare();
        void showBackup();
        void showOpenFile();
        void showModify();
        void showApply();
        void openCompareResult(const QString &left, const QString &right);

        ModifyPage *getModifyPage() const
        {
            return mfp_;
        }

        // 메인윈도우가 받을 시그널 (재발행)
      signals:
        void compareRequested(const QString &left, const QString &right);
        void backupRequested(const QString &target);
        void openFileRequested(const QString &target);
        void modifyRequested(const QString &target);
        void applyRequested(const QString &target);

      private:
        QStackedWidget *stack_ = nullptr;
        int             idxC_ = -1, idxB_ = -1, idxO_ = -1, idxA_ = -1, idxM_ = -1;
        ComparePage    *cmp_ = nullptr;
        BackupPage     *bkp_ = nullptr;
        OpenFilePage   *ofp_ = nullptr;
        ApplyPage      *alp_ = nullptr;
        ModifyPage     *mfp_ = nullptr;
};

#endif  // CENTERSTACK_H
