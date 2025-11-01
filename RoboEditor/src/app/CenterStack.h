#ifndef CENTERSTACK_H
#define CENTERSTACK_H
#pragma once

#include <QWidget>
class QMainWindow;
class QStackedWidget;
class ComparePage;
class BackupPage;
class OpenFilePage;
class ModifyPage;

class CenterStack : public QWidget
{
    Q_OBJECT
      public:
        explicit CenterStack(QWidget *parent = nullptr);
        void showCompare();
        void showOpenFile();
        void showModify();
        void openCompareResult(const QString &left, const QString &right);

        // 페이지 접근자
        ModifyPage *modifyPage() const
        {
            return mfp_;
        }

      public slots:
        void showModifyWithCompare();  // ModifyPage로 전환 + Compare 패널 표시

        ModifyPage *getModifyPage() const
        {
            return mfp_;
        }

        // 메인윈도우가 받을 시그널 (재발행)
      signals:
        void compareRequested(const QString &left, const QString &right);
        void openFileRequested(const QString &target);
        void modifyRequested(const QString &target);

      private:
        QStackedWidget *stack_ = nullptr;
        int             idxC_ = -1, idxO_ = -1, idxM_ = -1;
        ComparePage    *cmp_ = nullptr;
        OpenFilePage   *ofp_ = nullptr;
        ModifyPage     *mfp_ = nullptr;
};

#endif  // CENTERSTACK_H
