#ifndef COMPAREPAGE_H
#define COMPAREPAGE_H
#pragma once

#include <QList>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QWidget>

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

class ComparePage : public QWidget
{
    Q_OBJECT
      public:
        explicit ComparePage(QWidget *parent = nullptr);
        void setTargetPath(const QString &path);  // 파일 선택 시 1회 호출

        QWidget *buildDiffPanel();
        struct DiffRow
        {
            QString key;
            QString origin;
            QString target;
            QString state;  // "SAME", "CHANGED", "ADDED", "REMOVED"
        };

      public slots:
        void onOpenLeftFolderClicked();
        void onOpenRightFolderClicked();
        void onLeftTreeDoubleClicked(const QModelIndex &idx);
        void onRightTreeDoubleClicked(const QModelIndex &idx);

        void refreshTreeView(const QString &side, const QString &path);
        void loadLocalFolder(const QString &side, const QString &path);

        void recalcDiff(const QString &leftText);  // 좌 텍스트 받아 재계산

      signals:
        // 메인/센터스택 쪽으로 전달할 로그용 이벤트
        void uiCompareClicked(const QString &leftPath, const QString &rightPath);
        void closed();                                   // [X] 클릭
        void targetPathChanged(const QString &newPath);  // 비교 대상 파일 변경됨

      private slots:
        void onCompareClicked();
        void onCloseClicked();

      private:
        QWidget      *dock_{nullptr};        // 헤더+[X]+rightSplit
        QSplitter    *rightSplit_{nullptr};  // (좌) rightText_ | (우) diffPanel_
        DropTextEdit *rightText_{nullptr};   // 읽기 전용 (비교대상)
        QWidget      *diffPanel_{nullptr};   // 기존 "테이블+상단 버튼" 위젯
        QTableWidget *diffTable_ = nullptr;
        QLabel       *statLabel_ = nullptr;
        QString       targetPath_;

        QLineEdit *leftFileSelect_{nullptr};
        QLineEdit *rightFileSelect_{nullptr};

        QWidget *buildDock();
        // 현재 선택된 루트 경로
        QString currentLeftRoot_;
        QString currentRightRoot_;

        // 현재 폴더에서 받은 파일 리스트 캐시
        QStringList leftFiles_;
        QStringList rightFiles_;

        // 좌측 사이드: 폴더 브라우저 탭
        QTabWidget *dirTabs_;
        QWidget    *leftTabPage_;
        QWidget    *rightTabPage_;

        // 본문 텍스트 뷰
        DropTextEdit *leftText_;
        //DropTextEdit *rightText_;

        // diff 영역
        QLineEdit *keySearchEdit_;
        QCheckBox *chkOnlyChanged_;
        QCheckBox *chkHideSame_;
        QCheckBox *chkOnlyAddDel_;

        // 내부 유틸
        void setRoots(const QString &leftRoot, const QString &rightRoot);
        void populateFileSelects(const QStringList &leftFiles, const QStringList &rightFiles);
        void updateFolderListing(const QString     &side,
                                 const QString     &basePath,
                                 const QStringList &folders,
                                 const QStringList &files);

        void setFileContents(const QString &leftText, const QString &rightText);
        void setDiffRows(const QList<DiffRow> &rows);
        void refreshDiffTable(const QList<DiffRow> &rows);

        void loadRightText(const QString &path);
        // 나중에 REST 붙일 자리 (지금은 더미)
        void fetchFolderFromApiDummy(const QString &side, const QString &basePathHint);
        void fetchCompareFromApiDummy(const QString &leftFilePath, const QString &rightFilePath);
        void loadFileIntoEditor(const QString &fullPath, bool isLeft);
};

#endif  // COMPAREPAGE_H
