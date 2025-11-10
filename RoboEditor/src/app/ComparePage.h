#ifndef COMPAREPAGE_H
#define COMPAREPAGE_H
#pragma once

#include <QList>
#include <QPair>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QWidget>

// Core services 포함
#include "./services/DiffService.h"
#include "DiffHighlighter.h"

class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QCheckBox;
class QTableWidget;
class QFileSystemModel;
class QTreeView;
class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class DropTextEdit;
class CodeEditor;

class ComparePage : public QWidget
{
    Q_OBJECT
      public:
        explicit ComparePage(QWidget *parent = nullptr);
        ~ComparePage();  // 소멸자 추가 (임시 폴더 정리)

        void setTargetPath(const QString &path);      // 파일 선택 시 1회 호출
        void setLeftEditor(CodeEditor * leftEditor);  // 좌측 편집기 설정
        void clearHighlights();                       // 하이라이트 제거

        QWidget *buildDiffPanel();
        struct DiffRow
        {
            int     line;             // 라인 번호 (YAML용, 호환성 유지)
            int     leftLineNumber;   // 좌측(Compare) 파일의 라인 번호
            int     rightLineNumber;  // 우측(Base) 파일의 라인 번호
            QString key;
            QString origin;
            QString target;
            QString state;  // "SAME", "CHANGED", "ADDED", "REMOVED"

            DiffRow() : line(-1), leftLineNumber(-1), rightLineNumber(-1) {}
        };

        // 폴더 비교 관련
        void performFolderDiff(const QString &leftPath, const QString &rightPath);

        // 설정 저장/복원
        QByteArray saveSplitterState() const;
        void       restoreSplitterState(const QByteArray &state);

      public slots:
        void recalcDiff(const QString &leftText,
                        const QString &leftPath = QString());  // 좌 텍스트 받아 재계산

      signals:
        // 메인/센터스택 쪽으로 전달할 로그용 이벤트
        void uiCompareClicked(const QString &leftPath, const QString &rightPath);
        void closed();                                   // [X] 클릭
        void targetPathChanged(const QString &newPath);  // 비교 대상 파일 변경됨

      private slots:
        void onCloseClicked();

      private:
        QWidget    *dock_{nullptr};              // 헤더+[X]+rightSplit
        QSplitter  *rightSplit_{nullptr};        // (좌) rightText_ | (우) diffPanel_
        QTabWidget *compareTabWidget_{nullptr};  // 비교 파일들을 탭으로 관리
        CodeEditor *rightText_{nullptr};  // 읽기 전용 (비교대상) - 현재 활성 탭 (라인 번호 포함)
        CodeEditor   *leftText_{nullptr};   // 좌측 편집기 (ModifyPage의 현재 탭)
        QWidget      *diffPanel_{nullptr};  // 기존 "테이블+상단 버튼" 위젯
        QTableWidget *diffTable_      = nullptr;
        QTreeWidget  *folderDiffTree_ = nullptr;  // 폴더 비교 결과 트리
        QLabel       *statLabel_      = nullptr;
        QString       targetPath_;
        QString       cachedLeftPath_;
        QString       cachedLeftText_;

        // DiffHighlighter 객체 (양쪽 편집기 공유)
        core::DiffHighlighter *leftDiffHighlighter_{nullptr};   // 좌측 편집기용
        core::DiffHighlighter *rightDiffHighlighter_{nullptr};  // 우측 편집기용

        // 폴더 비교 관련
        bool        isFolderMode_{false};  // 파일 비교 vs 폴더 비교 모드
        QStringList tempFolders_;  // 임시 폴더 목록 (압축 해제 시 생성, 정리용)

        QWidget *buildDock();

        // 현재 필터 상태
        QString        currentFilter_;  // "All", "Added", "Removed", "Changed"
        QList<DiffRow> allDiffRows_;    // 필터링 전 전체 데이터

        // 내부 유틸
        void setDiffRows(const QList<DiffRow> &rows);
        void refreshDiffTable(const QList<DiffRow> &rows);

        // Diff 수행 (DiffService 사용)
        void performDiff(const QString &leftPath, const QString &rightPath);

        // DiffService 결과를 DiffRow로 변환
        QList<DiffRow> parseDiffResult(const Json::Value &result, const QString &fileType);

        // 테이블 컬럼 조정
        void updateTableColumns(const QString &fileType,
                                const QString &leftHeaderOverride  = QString(),
                                const QString &rightHeaderOverride = QString());

        // 필터 관련
        void           applyFilter(const QString &filterType);
        QList<DiffRow> filterRows(const QList<DiffRow> &rows, const QString &filterType) const;

        void    displayFolderDiffResult(const Json::Value &result);
        QString extractArchiveToTemp(const QString &archivePath);
        void    cleanupTempFolders();
        QColor  getColorForDiffState(const QString &state) const;

        QString detectFileType(const QString &path) const;
        bool    areFileTypesCompatible(const QString &leftType, const QString &rightType) const;

        struct ControllerPathInfo
        {
            QString serial;
            QString detail;
        };

        ControllerPathInfo      extractControllerInfo(const QString &path) const;
        QPair<QString, QString> determineColumnHeaders(const QString &leftPath,
                                                       const QString &rightPath) const;
        QString                 formatPathForCompare(const QString &fullPath) const;
};

#endif  // COMPAREPAGE_H
