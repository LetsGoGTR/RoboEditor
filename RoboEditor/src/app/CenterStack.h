#ifndef CENTERSTACK_H
#define CENTERSTACK_H
#pragma once
#include <QObject>
class QMainWindow;
class QStackedWidget;
class ComparePage;
class BackupPage;
class OpenFilePage;
class ApplyPage;
class ModifyPage;

class CenterStack : public QObject
{
    Q_OBJECT
      public:
        explicit CenterStack(QMainWindow * mw);
        QStackedWidget *widget() const
        {
            return stack_;
        }
        void showCompare();
        void showBackup();
        void showOpenFile();
        void showModify();
        void showApply();

        // 메인윈도우가 받을 시그널 (재발행)
      signals:
        void compareRequested(const QString &left, const QString &right);
        void backupRequested(const QString &target);
        void openFileRequested(const QString &target);
        void modifyRequested(const QString &target);
        void applyRequested(const QString &target);

      public:
        void openCompareResult(const QString &left, const QString &right);

      private:
        void            build();
        QStackedWidget *stack_ = nullptr;
        int             idxC_ = -1, idxB_ = -1, idxO_ = -1, idxA_ = -1, idxM_ = -1;
        ComparePage    *cmp_ = nullptr;
        BackupPage     *bkp_ = nullptr;
        OpenFilePage   *ofp_ = nullptr;
        ApplyPage      *alp_ = nullptr;
        ModifyPage     *mfp_ = nullptr;
};

#endif  // CENTERSTACK_H
