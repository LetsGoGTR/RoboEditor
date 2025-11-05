#ifndef COMPAREPAGE_H
#define COMPAREPAGE_H
#pragma once

#include <QList>
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
class QTabWidget;
class DropTextEdit;
class CodeEditor;

class ComparePage : public QWidget
{
    Q_OBJECT
      public:
        explicit ComparePage(QWidget *parent = nullptr);
        void setTargetPath(const QString &path);  // 파일 선택 시 1회 호출
        void setLeftEditor(CodeEditor *leftEditor);  // 좌측 편집기 설정

        QWidget *buildDiffPanel();
        struct DiffRow
        {
            int     line;   // 라인 번호 (YAML용)
            QString key;
            QString origin;
            QString target;
            QString state;  // "SAME", "CHANGED", "ADDED", "REMOVED"
        };


      public slots:
        void recalcDiff(const QString &leftText);  // 좌 텍스트 받아 재계산

      signals:
        // 메인/센터스택 쪽으로 전달할 로그용 이벤트
        void uiCompareClicked(const QString &leftPath, const QString &rightPath);
        void closed();                                   // [X] 클릭
        void targetPathChanged(const QString &newPath);  // 비교 대상 파일 변경됨

      private slots:
        void onCloseClicked();

      private:
        QWidget      *dock_{nullptr};              // 헤더+[X]+rightSplit
        QSplitter    *rightSplit_{nullptr};        // (좌) rightText_ | (우) diffPanel_
        QTabWidget   *compareTabWidget_{nullptr};  // 비교 파일들을 탭으로 관리
        CodeEditor   *rightText_{nullptr};  // 읽기 전용 (비교대상) - 현재 활성 탭 (라인 번호 포함)
        CodeEditor   *leftText_{nullptr};   // 좌측 편집기 (ModifyPage의 현재 탭)
        QWidget      *diffPanel_{nullptr};  // 기존 "테이블+상단 버튼" 위젯
        QTableWidget *diffTable_ = nullptr;
        QLabel       *statLabel_ = nullptr;
        QString       targetPath_;
        
        // DiffHighlighter 객체 (양쪽 편집기 공유)
        core::DiffHighlighter *diffHighlighter_{nullptr};

        QWidget *buildDock();

        // 현재 필터 상태
        QString currentFilter_;  // "All", "Added", "Removed", "Changed"
        QList<DiffRow> allDiffRows_;  // 필터링 전 전체 데이터

        // 내부 유틸
        void setDiffRows(const QList<DiffRow> &rows);
        void refreshDiffTable(const QList<DiffRow> &rows);

        // Diff 수행 (DiffService 사용)
        void performDiff(const QString &leftPath, const QString &rightPath);
        
        // DiffService 결과를 DiffRow로 변환
        QList<DiffRow> parseDiffResult(const Json::Value &result, const QString &fileType);
        
        // 테이블 컬럼 조정
        void updateTableColumns(const QString &fileType);

        // 필터 관련
        void applyFilter(const QString &filterType);
        QList<DiffRow> filterRows(const QList<DiffRow> &rows, const QString &filterType) const;
};

#endif  // COMPAREPAGE_H
