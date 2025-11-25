#include "WorkspaceContextMenuController.h"

#include <QTreeView>

#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QWidget>

#include "services/FileService.h"
#include "services/FolderService.h"

WorkspaceContextMenuController::WorkspaceContextMenuController(QTreeView        *treeView,
                                                               QFileSystemModel *model,
                                                               const QString    &workspaceRoot,
                                                               QObject          *parent) :
    QObject(parent),
    treeView_(treeView),
    model_(model),
    workspaceRoot_(workspaceRoot)
{
    Q_ASSERT(treeView_);
    Q_ASSERT(model_);

    ///////////////////////////////////////
    // 1) 트리 우클릭 컨텍스트 메뉴 연결
    ///////////////////////////////////////
    treeView_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView_,
            &QTreeView::customContextMenuRequested,
            this,
            &WorkspaceContextMenuController::onContextMenuRequested);

    ///////////////////////////////////////
    // 2) 상단 입력바 생성 (파일/폴더 생성 + 이름 변경에 사용)
    ///////////////////////////////////////
    QWidget *parentWidget = treeView_->parentWidget();
    if (!parentWidget)
        parentWidget = treeView_;

    inputBar_ = new QWidget(parentWidget);
    inputBar_->setAutoFillBackground(true);
    inputBar_->setVisible(false);

    auto *layout = new QHBoxLayout(inputBar_);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    inputLabel_ = new QLabel("이름:", inputBar_);
    nameEdit_   = new QLineEdit(inputBar_);

    layout->addWidget(inputLabel_);
    layout->addWidget(nameEdit_);

    connect(nameEdit_,
            &QLineEdit::returnPressed,
            this,
            &WorkspaceContextMenuController::onInlineEditAccepted);

    nameEdit_->installEventFilter(this);
    treeView_->installEventFilter(this);

    QRect g = treeView_->geometry();
    inputBar_->setFixedHeight(32);
    inputBar_->setGeometry(g.x(), g.y(), g.width(), inputBar_->height());
    inputBar_->raise();
}

void WorkspaceContextMenuController::setWorkspaceRoot(const QString &workspaceRoot)
{
    workspaceRoot_ = workspaceRoot;
    refreshWorkspace(workspaceRoot_);
}

bool WorkspaceContextMenuController::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == treeView_ && event->type() == QEvent::Resize) {
        if (inputBar_) {
            QRect g = treeView_->geometry();
            inputBar_->setGeometry(g.x(), g.y(), g.width(), inputBar_->height());
        }
    } else if (obj == nameEdit_ && event->type() == QEvent::KeyPress) {
        auto *e = static_cast<QKeyEvent *>(event);
        if (e->key() == Qt::Key_Escape) {
            hideInlineEditor();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

// --------------------------------------------------
// 컨텍스트 메뉴
// --------------------------------------------------

void WorkspaceContextMenuController::onContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = treeView_->indexAt(pos);

    QString   targetPath;
    QFileInfo info;
    bool      hasItem = index.isValid();

    if (hasItem) {
        targetPath = model_->filePath(index);
        info       = QFileInfo(targetPath);
    } else {
        targetPath = workspaceRoot_;
        info       = QFileInfo(targetPath);
    }

    if (workspaceRoot_.isEmpty())
        return;

    QMenu menu(treeView_);

    if (!hasItem || info.isDir()) {
        QAction *newFileAct = menu.addAction("새 파일 생성");
        QAction *newDirAct  = menu.addAction("새 디렉토리 생성");
        QAction *renameAct  = nullptr;
        QAction *deleteAct  = nullptr;
        QAction *dupAct     = nullptr;

        if (hasItem && info.isDir()) {
            menu.addSeparator();
            renameAct = menu.addAction("이름 변경");
            deleteAct = menu.addAction("폴더 삭제");
            dupAct    = menu.addAction("복사 하기");
        }

        QAction *chosen = menu.exec(treeView_->viewport()->mapToGlobal(pos));
        if (!chosen)
            return;

        if (chosen == newFileAct) {
            onCreateFile(targetPath);
        } else if (chosen == newDirAct) {
            onCreateFolder(targetPath);
        } else if (renameAct && chosen == renameAct) {
            onRenamePath(targetPath);
        } else if (deleteAct && chosen == deleteAct) {
            onDeleteFolder(targetPath);
        } else if (dupAct && chosen == dupAct) {
            onDuplicateFolder(targetPath);
        }
    } else if (info.isFile()) {
        QAction *renameAct = menu.addAction("이름 변경");
        QAction *deleteAct = menu.addAction("삭제");
        QAction *dupAct    = menu.addAction("복사");

        QAction *chosen = menu.exec(treeView_->viewport()->mapToGlobal(pos));
        if (!chosen)
            return;

        if (chosen == renameAct) {
            onRenamePath(targetPath);
        } else if (chosen == deleteAct) {
            onDeleteFile(targetPath);
        } else if (chosen == dupAct) {
            onDuplicateFile(targetPath);
        }
    }
}

// --------------------------------------------------
// 상단 입력바 제어
// --------------------------------------------------

void WorkspaceContextMenuController::showInlineEditor(PendingAction  action,
                                                      const QString &target,
                                                      const QString &defaultText)
{
    pendingAction_ = action;

    switch (action) {
    case PendingAction::CreateFile:
        inputLabel_->setText("새 파일 이름:");
        pendingTargetDir_ = target;
        pendingOldPath_.clear();
        break;

    case PendingAction::CreateFolder:
        inputLabel_->setText("새 폴더 이름:");
        pendingTargetDir_ = target;
        pendingOldPath_.clear();
        break;

    case PendingAction::Rename:
        inputLabel_->setText("이름 변경:");
        pendingOldPath_ = target;
        pendingTargetDir_.clear();
        break;

    default:
        return;
    }

    nameEdit_->setText(defaultText);
    nameEdit_->selectAll();

    QRect g = treeView_->geometry();
    inputBar_->setGeometry(g.x(), g.y(), g.width(), inputBar_->height());
    inputBar_->show();
    nameEdit_->setFocus();
}

void WorkspaceContextMenuController::hideInlineEditor(bool clearText)
{
    pendingAction_ = PendingAction::None;
    pendingOldPath_.clear();
    pendingTargetDir_.clear();

    if (clearText)
        nameEdit_->clear();

    if (inputBar_)
        inputBar_->hide();
}

// --------------------------------------------------
// 액션 처리
// --------------------------------------------------

// 파일 생성
void WorkspaceContextMenuController::onCreateFile(const QString &dirPath)
{
    showInlineEditor(PendingAction::CreateFile, dirPath, "new_file.txt");
}

// 폴더 생성
void WorkspaceContextMenuController::onCreateFolder(const QString &dirPath)
{
    showInlineEditor(PendingAction::CreateFolder, dirPath, "NewFolder");
}

// 이름 변경
void WorkspaceContextMenuController::onRenamePath(const QString &oldPath)
{
    QFileInfo info(oldPath);
    showInlineEditor(PendingAction::Rename, oldPath, info.fileName());
}

// 파일 삭제
void WorkspaceContextMenuController::onDeleteFile(const QString &filePath)
{
    QFileInfo info(filePath);
    if (!info.isFile())
        return;

    auto reply = QMessageBox::question(
            treeView_, tr("Delete File"), tr("Are you sure you want to delete '%1'?").arg(info.fileName()));
    if (reply != QMessageBox::Yes)
        return;

    services::ServiceResult result = services::FileService::deleteFile(filePath.toStdString());

    if (!result.success) {
        QMessageBox::warning(treeView_, tr("Delete Failed"), QString::fromStdString(result.errorMessage));
        return;
    }

    refreshWorkspace(info.dir().absolutePath());
}

// 폴더 삭제
void WorkspaceContextMenuController::onDeleteFolder(const QString &folderPath)
{
    QFileInfo info(folderPath);
    if (!info.isDir())
        return;

    if (folderPath == workspaceRoot_) {
        QMessageBox::warning(treeView_, tr("Cannot Delete"), tr("Workspace root cannot be deleted."));
        return;
    }

    auto reply = QMessageBox::question(
            treeView_,
            tr("Delete Folder"),
            tr("Folder '%1' and all its contents will be deleted.\nContinue?").arg(info.fileName()));

    if (reply != QMessageBox::Yes)
        return;

    services::ServiceResult result =
            services::FolderService::deleteFolder(folderPath.toStdString());

    if (!result.success) {
        QMessageBox::warning(treeView_, tr("Delete Failed"), QString::fromStdString(result.errorMessage));
        return;
    }

    refreshWorkspace(info.dir().absolutePath());
}

// --------------------------------------------------
// Enter 눌렀을 때 실행됨
// --------------------------------------------------

void WorkspaceContextMenuController::onInlineEditAccepted()
{
    QString text = nameEdit_->text().trimmed();
    if (text.isEmpty()) {
        hideInlineEditor();
        return;
    }

    bool ok = true;

    // =============================
    // 1) 새 파일 생성
    // =============================
    if (pendingAction_ == PendingAction::CreateFile) {
        QDir dir(pendingTargetDir_);
        if (!dir.exists()) {
            ok = false;
        } else {
            // 사용자가 입력한 원래 이름
            QString   originalName = text;
            QFileInfo fi(originalName);
            QString   baseName = fi.completeBaseName();  // "new_file"
            QString   suffix   = fi.suffix();            // "txt" (없으면 "")

            // 처음엔 사용자가 입력한 그대로를 시도
            QString uniqueName  = originalName;
            QString newFilePath = dir.filePath(uniqueName);

            // 같은 이름이 이미 존재하면 baseName(1).ext, baseName(2).ext ... 로 증가
            auto makeName = [&](int c) {
                if (suffix.isEmpty()) {
                    // 확장자 없을 때: name(1)
                    return QString("%1(%2)").arg(baseName).arg(c);
                } else {
                    // 확장자 있을 때: name(1).ext
                    return QString("%1(%2).%3").arg(baseName).arg(c).arg(suffix);
                }
            };

            int counter = 1;
            while (QFileInfo::exists(newFilePath)) {
                uniqueName  = makeName(counter++);
                newFilePath = dir.filePath(uniqueName);
            }

            services::ServiceResult r =
                    services::FileService::createFile(newFilePath.toStdString(), "");

            if (!r.success) {
                QMessageBox::warning(
                        treeView_, tr("File Creation Failed"), QString::fromStdString(r.errorMessage));
                ok = false;
            } else {
                // 성공 시 워크스페이스 갱신
                refreshWorkspace(pendingTargetDir_);
            }
        }
    }

    // =============================
    // 2) 새 폴더 생성
    // =============================
    else if (pendingAction_ == PendingAction::CreateFolder) {
        QDir dir(pendingTargetDir_);
        if (!dir.exists()) {
            ok = false;
        } else {
            QString baseName      = text;  // "NewFolder"
            QString uniqueName    = baseName;
            QString newFolderPath = dir.filePath(uniqueName);

            int counter = 1;
            while (QDir(newFolderPath).exists()) {
                // NewFolder(1), NewFolder(2) ...
                uniqueName    = QString("%1(%2)").arg(baseName).arg(counter++);
                newFolderPath = dir.filePath(uniqueName);
            }

            services::ServiceResult r =
                    services::FolderService::createFolder(newFolderPath.toStdString());

            if (!r.success) {
                QMessageBox::warning(
                        treeView_, tr("Folder Creation Failed"), QString::fromStdString(r.errorMessage));
                ok = false;
            } else {
                refreshWorkspace(pendingTargetDir_);
            }
        }
    }

    // =============================
    // 3) 이름 변경 (파일/폴더)
    //   → 이 부분은 기존 방식 그대로 유지
    // =============================
    else if (pendingAction_ == PendingAction::Rename) {
        QFileInfo info(pendingOldPath_);
        QDir      parentDir = info.dir();
        QString   newPath   = parentDir.filePath(text);

        // 이미 존재하는 이름이면 에러 (원하면 여기에도 (1) 붙이는 로직 넣을 수 있음)
        if (QFileInfo::exists(newPath) && newPath != pendingOldPath_) {
            QMessageBox::warning(treeView_, tr("Rename Failed"), tr("Name already exists."));
            ok = false;
        } else {
            services::ServiceResult r;
            if (info.isDir()) {
                r = services::FolderService::updateFolder(pendingOldPath_.toStdString(),
                                                          newPath.toStdString());
            } else {
                r = services::FileService::moveFile(pendingOldPath_.toStdString(),
                                                    newPath.toStdString());
            }

            if (!r.success) {
                QMessageBox::warning(
                        treeView_, tr("Rename Failed"), QString::fromStdString(r.errorMessage));
                ok = false;
            } else {
                refreshWorkspace(parentDir.absolutePath());
            }
        }
    }

    hideInlineEditor(/*clearText=*/ok);
}

// --------------------------------------------------
// 워크스페이스 새로고침
// --------------------------------------------------

void WorkspaceContextMenuController::refreshWorkspace(const QString &path)
{
    Q_UNUSED(path);

    if (!model_ || !treeView_)
        return;
    if (workspaceRoot_.isEmpty())
        return;

    QModelIndex rootIdx = model_->index(workspaceRoot_);
    if (rootIdx.isValid())
        treeView_->setRootIndex(rootIdx);
}

void WorkspaceContextMenuController::onDuplicateFile(const QString &filePath)
{
    QFileInfo info(filePath);
    if (!info.isFile())
        return;

    QDir parentDir = info.dir();

    // 원래 이름과 확장자 분리
    QString originalName = info.fileName();          // ex: "new_file.txt"
    QString baseName     = info.completeBaseName();  // ex: "new_file"
    QString suffix       = info.suffix();            // ex: "txt" (없으면 "")

    // 첫 후보는 원래 이름 뒤에 "(1)" 붙이기
    auto makeName = [&](int c) {
        if (suffix.isEmpty()) {
            // 확장자 없는 경우: name(1)
            return QString("%1(%2)").arg(baseName).arg(c);
        } else {
            // 확장자 있는 경우: name(1).ext
            return QString("%1(%2).%3").arg(baseName).arg(c).arg(suffix);
        }
    };

    int     counter    = 1;
    QString uniqueName = makeName(counter);
    QString newPath    = parentDir.filePath(uniqueName);

    // 이미 같은 이름 있으면 (2), (3) ... 로 증가
    while (QFileInfo::exists(newPath)) {
        counter++;
        uniqueName = makeName(counter);
        newPath    = parentDir.filePath(uniqueName);
    }

    // 원본 파일 읽기 (서비스 사용)
    services::ServiceResult readRes = services::FileService::readFile(filePath.toStdString());
    if (!readRes.success) {
        QMessageBox::warning(
                treeView_, tr("Duplicate Failed"), QString::fromStdString(readRes.errorMessage));
        return;
    }

    std::string content;
    if (readRes.data.isMember("content")) {
        content = readRes.data["content"].asString();
    }

    // 새 파일 생성
    services::ServiceResult createRes =
            services::FileService::createFile(newPath.toStdString(), content);

    if (!createRes.success) {
        QMessageBox::warning(
                treeView_, tr("Duplicate Failed"), QString::fromStdString(readRes.errorMessage));
        return;
    }

    // 트리 갱신
    refreshWorkspace(parentDir.absolutePath());
}

void WorkspaceContextMenuController::onDuplicateFolder(const QString &folderPath)
{
    QFileInfo info(folderPath);
    if (!info.isDir())
        return;

    QDir    parentDir     = info.dir();
    QString baseName      = info.fileName();  // ex: "NewFolder"
    QString uniqueName    = baseName;
    QString newFolderPath = parentDir.filePath(uniqueName);

    // NewFolder, NewFolder(1), NewFolder(2) ... 형식으로 고유 이름 찾기
    int counter = 1;
    while (QDir(newFolderPath).exists()) {
        uniqueName    = QString("%1(%2)").arg(baseName).arg(counter++);
        newFolderPath = parentDir.filePath(uniqueName);
    }

    // 재귀 복사 함수
    std::function<bool(const QString &, const QString &)> copyDirRecursively;
    copyDirRecursively = [&](const QString &src, const QString &dst) -> bool {
        QDir srcDir(src);
        if (!srcDir.exists())
            return false;

        QDir dstDir;
        if (!dstDir.mkpath(dst)) {
            return false;
        }

        QFileInfoList entries = srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo &entry : entries) {
            QString srcPath = entry.absoluteFilePath();
            QString dstPath = QDir(dst).filePath(entry.fileName());

            if (entry.isDir()) {
                if (!copyDirRecursively(srcPath, dstPath))
                    return false;
            } else if (entry.isFile()) {
                if (!QFile::copy(srcPath, dstPath))
                    return false;
            }
        }
        return true;
    };

    if (!copyDirRecursively(folderPath, newFolderPath)) {
        QMessageBox::warning(treeView_, tr("Duplicate Failed"), tr("Error occurred while duplicating folder."));
        return;
    }

    refreshWorkspace(parentDir.absolutePath());
}
