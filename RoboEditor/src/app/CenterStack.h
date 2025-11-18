#ifndef CENTERSTACK_H
#define CENTERSTACK_H
#pragma once
#include <QTabWidget>
#include <QTimer>
#include <QTreeView>

#include <QFileSystemModel>
#include <QListView>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QWidget>

#include "ControllerManager.h"

// Forward declarations
class QMainWindow;
class ComparePage;
class BackupPage;
class ModifyPage;
class WorkspaceContextMenuController;
class CenterStack : public QWidget
{
    Q_OBJECT

      public:
        explicit CenterStack(QWidget *parent = nullptr);
        ~CenterStack();

        // 페이지 전환
        void showCompare();
        void showModifyWithCompare();
        void openCompareResult(const QString &left, const QString &right);
        void startPolling(int intervalMs = 5000);
        void stopPolling();
        // 페이지 접근자
        ModifyPage *modifyPage() const
        {
            return modifyPage_;
        }
        ModifyPage *getModifyPage() const
        {
            return modifyPage_;
        }
        ComparePage *comparePage() const
        {
            return comparePage_;
        }
        ComparePage *getComparePage() const
        {
            return comparePage_;
        }
        // 경로 관련
        QString getWorkspacePath() const;
        void    setBackupPath(const QString &path);

      signals:
        // 메인윈도우가 받을 시그널
        void compareRequested(const QString &left, const QString &right);
        void modifyRequested(const QString &target);
        void workspaceSelected(const QString &path);
        void fileOpened(const QString &filePath);

      public slots:
        void updateControllerList();
        void onPollingTimeout();

      private slots:
        void onControllerTreeClicked(const QModelIndex &index);
        void onEditController(const QString &serialNumber);
        void onRemoveController(const QString &serialNumber);

      private:
        void setupUI();

        // 첫 번째 구조 (Compare/OpenFile/Modify 페이지용)
        QStackedWidget *stack_;
        int             idxC_;
        ComparePage    *cmp_;

        // 두 번째 구조 (Controller/Workspace 트리용)
        QStackedWidget *internalStack_;
        QSplitter      *splitter_;
        QTabWidget     *treeTabWidget_;
        QTreeView      *backupTree_;
        QListView      *controllerList_;
        QTreeView      *workspaceTree_;
        QWidget        *controllerWidget_;

        // 모델
        QStandardItemModel *controllerModel_;
        QFileSystemModel   *backupModel_;
        QFileSystemModel   *workspaceModel_;

        ModifyPage        *modifyPage_;
        ComparePage       *comparePage_;
        ControllerManager *controllerManager_;

        // 경로
        QString workspacePath_;
        QString backupRootPath_;

        WorkspaceContextMenuController *workspaceMenuController_ = nullptr;

        //타이머
        QTimer *m_pollingTimer;
};

#endif  // CENTERSTACK_H
