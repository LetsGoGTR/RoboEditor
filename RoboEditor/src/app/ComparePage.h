#ifndef COMPAREPAGE_H
#define COMPAREPAGE_H
#pragma once

#include <QWidget>
#include <QList>
#include <QString>
#include <QStringList>

class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QCheckBox;
class QTableWidget;
class QFileSystemModel;
class QTreeView;
class QTabWidget;
class DropTextEdit;

class ComparePage : public QWidget {
    Q_OBJECT
  public:
    explicit ComparePage(QWidget* parent=nullptr);

    struct DiffRow {
        QString key;
        QString origin;
        QString target;
        QString state; // "SAME", "CHANGED", "ADDED", "REMOVED"
    };

  signals:
    // 메인/센터스택 쪽으로 전달할 로그용 이벤트
    void requestCompare(const QString& leftPath,
                        const QString& rightPath);

  private slots:
    void onOpenLeftFolderClicked();
    void onOpenRightFolderClicked();
    void onCompareClicked();
    void onLeftTreeDoubleClicked(const QModelIndex& idx);
    void onRightTreeDoubleClicked(const QModelIndex& idx);

  private:
    // 현재 선택된 루트 경로
    QString currentLeftRoot_;
    QString currentRightRoot_;

    // 현재 폴더에서 받은 파일 리스트 캐시
    QStringList leftFiles_;
    QStringList rightFiles_;

    // 좌측 사이드: 폴더 브라우저 탭
    QTabWidget* dirTabs_;
    QWidget*    leftTabPage_;
    QWidget*    rightTabPage_;

    // 상단 컨트롤
    QComboBox*   viewModeCombo_;
    QPushButton* openLeftBtn_;
    QComboBox*   leftFileSelect_;
    QPushButton* openRightBtn_;
    QComboBox*   rightFileSelect_;
    QPushButton* compareBtn_;
    QLabel*      fileInfoLabel_;

    // 본문 텍스트 뷰
    DropTextEdit* leftText_;
    DropTextEdit* rightText_;

    // diff 영역
    QLineEdit*     keySearchEdit_;
    QCheckBox*     chkOnlyChanged_;
    QCheckBox*     chkHideSame_;
    QCheckBox*     chkOnlyAddDel_;
    QLabel*        statLabel_;
    QTableWidget*  diffTable_;

    // (지금은 안 보이는 로컬/미래용 트리 모델)
    QFileSystemModel* fsModelLeft_;
    QFileSystemModel* fsModelRight_;
    QTreeView*        fsTreeLeft_;
    QTreeView*        fsTreeRight_;

    // 내부 유틸
    void setRoots(const QString& leftRoot, const QString& rightRoot);
    void populateFileSelects(const QStringList& leftFiles,
                             const QStringList& rightFiles);
    void updateFolderListing(const QString& side,
                             const QString& basePath,
                             const QStringList& folders,
                             const QStringList& files);

    void setFileContents(const QString& leftText,
                         const QString& rightText);
    void setDiffRows(const QList<DiffRow>& rows);
    void refreshDiffTable(const QList<DiffRow>& rows);

    // 나중에 REST 붙일 자리 (지금은 더미)
    void fetchFolderFromApiDummy(const QString& side,
                                 const QString& basePathHint);
    void fetchCompareFromApiDummy(const QString& leftFilePath,
                                  const QString& rightFilePath);
    void loadLocalFolder(const QString& side, const QString& path);
    void refreshTreeView(const QString& side, const QString& path);
    void loadFileIntoEditor(const QString& fullPath, bool isLeft);
};

#endif // COMPAREPAGE_H
