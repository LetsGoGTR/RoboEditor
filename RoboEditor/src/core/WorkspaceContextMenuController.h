#ifndef WORKSPACECONTEXTMENUCONTROLLER_H
#define WORKSPACECONTEXTMENUCONTROLLER_H
#pragma once

#include <QObject>
#include <QString>

class QTreeView;
class QFileSystemModel;
class QWidget;
class QLabel;
class QLineEdit;
class QPoint;
class QEvent;

class WorkspaceContextMenuController : public QObject
{
    Q_OBJECT
  public:
    explicit WorkspaceContextMenuController(QTreeView *treeView,
                                            QFileSystemModel *model,
                                            const QString &workspaceRoot,
                                            QObject *parent = nullptr);

    void setWorkspaceRoot(const QString &workspaceRoot);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

  private slots:
    void onContextMenuRequested(const QPoint &pos);
    void onInlineEditAccepted();

  private:
    enum class PendingAction {
        None,
        CreateFile,
        CreateFolder,
        Rename
    };

    QTreeView        *treeView_;
    QFileSystemModel *model_;
    QString           workspaceRoot_;

    // VS Code 스타일 상단 입력바
    QWidget   *inputBar_      = nullptr;
    QLabel    *inputLabel_    = nullptr;
    QLineEdit *nameEdit_      = nullptr;

    PendingAction pendingAction_   = PendingAction::None;
    QString       pendingTargetDir_;
    QString       pendingOldPath_;

    // 액션 함수들
    void onCreateFile(const QString &dirPath);
    void onCreateFolder(const QString &dirPath);
    void onRenamePath(const QString &oldPath);
    void onDeleteFile(const QString &filePath);
    void onDeleteFolder(const QString &folderPath);
    void onDuplicateFile(const QString &filePath);
    void onDuplicateFolder(const QString &folderPath);

    // inline editor
    void showInlineEditor(PendingAction action,
                          const QString &targetDirOrPath,
                          const QString &defaultText = QString());
    void hideInlineEditor(bool clearText = true);

    void refreshWorkspace(const QString &path);
};

#endif  // WORKSPACECONTEXTMENUCONTROLLER_H
