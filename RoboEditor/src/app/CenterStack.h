#ifndef CENTERSTACK_H
#define CENTERSTACK_H

#include <QTabWidget>
#include <QTreeView>

#include <QFileSystemModel>
#include <QListView>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QWidget>

// Forward declarations
class QMainWindow;
class ComparePage;
class BackupPage;
class OpenFilePage;
class ModifyPage;

class CenterStack : public QWidget
{
    Q_OBJECT

      public:
        explicit CenterStack(QWidget *parent = nullptr);
        ~CenterStack();

        // 페이지 전환
        void showCompare();
        void showOpenFile();
        void showModify();
        void showModifyWithCompare();
        void openCompareResult(const QString &left, const QString &right);

        // 페이지 접근자
        ModifyPage *modifyPage() const
        {
            return mfp_;
        }
        ModifyPage *getModifyPage() const
        {
            return mfp_;
        }

        // 경로 관련
        QString getWorkspacePath() const;
        void    setBackupPath(const QString &path);

      signals:
        // 메인윈도우가 받을 시그널
        void compareRequested(const QString &left, const QString &right);
        void openFileRequested(const QString &target);
        void modifyRequested(const QString &target);
        void workspaceSelected(const QString &path);
        void fileOpened(const QString &filePath);

      public slots:
        void updateControllerList();

      private slots:
        void onControllerTreeClicked(const QModelIndex &index);
        void onEditController(const QString &serialNumber);
        void onRemoveController(const QString &serialNumber);

      private:
        void setupUI();

        // 첫 번째 구조 (Compare/OpenFile/Modify 페이지용)
        QStackedWidget *stack_;
        int             idxC_, idxO_, idxM_;
        ComparePage    *cmp_;
        OpenFilePage   *ofp_;
        ModifyPage     *mfp_;

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

        // 추가 ModifyPage (중복 제거 필요 시 위의 mfp_와 통합)
        ModifyPage *modifyPage_;

        // 경로
        QString workspacePath_;
        QString backupRootPath_;
};

#endif  // CENTERSTACK_H
