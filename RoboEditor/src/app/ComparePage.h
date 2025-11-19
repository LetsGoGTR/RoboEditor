#ifndef COMPAREPAGE_H
#define COMPAREPAGE_H
#pragma once

#include <QSplitter>
#include <QWidget>

#include "../core/FileTypeHelper.h"
#include "CodeEditor.h"
#include "DiffTablePanel.h"

class QTabWidget;

class ComparePage : public QWidget
{
    Q_OBJECT
      public:
        explicit ComparePage(QWidget *parent = nullptr);
        ~ComparePage();

        void setTargetPath(const QString &path);
        void setLeftEditor(CodeEditor * leftEditor);
        void clearHighlights();

        void performDiff(const QString &leftPath, const QString &rightPath);
        void performFolderDiff(const QString &leftPath, const QString &rightPath);
        void applyTheme(bool dark);

      signals:
        void closed();
        void targetPathChanged(const QString &newPath);
        void requestOpenFile(const QString &filePath);
        void uiCompareClicked(const QString &leftPath, const QString &rightPath);
        void themeChangeRequested(bool dark);

      public slots:
        void recalcDiff(const QString &leftText, const QString &leftPath = QString());

      private slots:
        void onFolderFileClicked(const QString &path);
        void onDiffRowClicked(const DiffRow &row);  // 스크롤 동기화용 슬롯

      private:
        QWidget    *dock_{nullptr};
        QSplitter  *rightSplit_{nullptr};
        QTabWidget *compareTabWidget_{nullptr};

        // 로직 처리를 위해 포인터는 유지하되, UI 구성시에는 컨테이너 안에 넣습니다.
        DiffTablePanel *diffPanel_{nullptr};

        CodeEditor *rightText_{nullptr};
        CodeEditor *leftText_{nullptr};

        core::DiffHighlighter *leftDiffHighlighter_{nullptr};
        core::DiffHighlighter *rightDiffHighlighter_{nullptr};

        QString     targetPath_;
        QString     cachedLeftPath_;
        QString     cachedLeftText_;
        QStringList tempFolders_;

        QString lastLeftFolderPath_;
        QString lastRightFolderPath_;

        bool isDarkMode_ = false;

        QWidget *buildDock();

        // 버튼 + 패널을 포함한 우측 영역 전체를 생성하는 함수
        QWidget *buildRightPanel();

        QList<DiffRow> parseDiffResult(const Json::Value &result, const QString &fileType);
        QString        extractArchiveToTemp(const QString &archivePath);
        void           cleanupTempFolders();
        QString        formatPathForCompare(const QString &fullPath) const;

        // 하이라이터 갱신 및 테이블 업데이트 통합 함수
        void setDiffRows(const QList<DiffRow> &rows);

        struct ControllerPathInfo
        {
            QString serial;
            QString detail;
        };
        ControllerPathInfo      extractControllerInfo(const QString &path) const;
        QPair<QString, QString> determineColumnHeaders(const QString &leftPath,
                                                       const QString &rightPath) const;
};
#endif  // COMPAREPAGE_H
