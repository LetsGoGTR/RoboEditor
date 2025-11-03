#ifndef CENTER_H
#define CENTER_H

#include <QTreeView>

#include <QFileSystemModel>
#include <QListView>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QWidget>

#include "ModifyPage.h"

class Center : public QWidget
{
    Q_OBJECT

      public:
        explicit Center(QWidget *parent = nullptr);
        ~Center();

        // 루트 디렉토리 설정
        void    setBackupPath(const QString &path);
        QString getWorkspacePath() const;

        // ModifyPage 직접 접근
        ModifyPage *modifyPage() const
        {
            return modifyPage_;
        }
        void updateControllerList();
      signals:
        void workspaceSelected(const QString &path);
        void fileOpened(const QString &filePath);
      private slots:
        void onControllerTreeClicked(const QModelIndex &index);

      private:
        void setupUI();

        QSplitter          *splitter_;
        QTabWidget         *treeTabWidget_;
        QTreeView          *backupTree_;
        QListView          *controllerList_;
        QString             workspacePath_;
        QString             backupRootPath_;
        QTreeView          *workspaceTree_;
        QStandardItemModel *controllerModel_;
        QFileSystemModel   *backupModel_;
        QFileSystemModel   *workspaceModel_;

        ModifyPage     *modifyPage_;
        QStackedWidget *stack_;
      private slots:
        void onEditController(const QString &serialNumber);    // ✅ 추가
        void onRemoveController(const QString &serialNumber);  // ✅ 추가
};

#endif  // CENTER_H
