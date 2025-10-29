#ifndef CENTERSTACK_H
#define CENTERSTACK_H
#pragma once

#include <QWidget>
class QMainWindow;
class QStackedWidget;
class ComparePage;
class BackupPage;
class OpenFilePage;

class CenterStack : public QWidget
{
    Q_OBJECT
      public:
        explicit CenterStack(QWidget *parent = nullptr);
        void showCompare();
        void showBackup();
        void showOpenFile();
        void openCompareResult(const QString &left, const QString &right);

        // 메인윈도우가 받을 시그널 (재발행)
      signals:
        void compareRequested(const QString &left, const QString &right);
        void backupRequested(const QString &target);
        void openFileRequested(const QString &target);

      private:
        QStackedWidget *stack_ = nullptr;
        ComparePage    *cmp_   = nullptr;
        BackupPage     *bkp_   = nullptr;
        OpenFilePage   *ofp_   = nullptr;
        int             idxC_ = -1, idxB_ = -1, idxO_ = -1;
};

#endif  // CENTERSTACK_H
