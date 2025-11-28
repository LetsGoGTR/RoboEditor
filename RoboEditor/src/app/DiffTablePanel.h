#ifndef DIFFTABLEPANEL_H
#define DIFFTABLEPANEL_H
#pragma once

#include <QTableWidget>
#include <QTreeWidget>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QList>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

#include <json/json.h>

// DiffRow 구조체 정의
struct DiffRow
{
    int     line            = -1;
    int     leftLineNumber  = -1;
    int     rightLineNumber = -1;
    QString key;     // Path 또는 Key
    QString origin;  // Right Value (Base)
    QString target;  // Left Value (Compare)
    QString state;   // "ADDED", "REMOVED", "CHANGED", "SAME", "ERROR", "MISMATCH"
};

class DiffTablePanel : public QWidget
{
    Q_OBJECT

      public:
        explicit DiffTablePanel(QWidget *parent = nullptr);

        // 파일 비교 모드 설정
        void setupFileDiffMode(
                const QString &fileType, const QString &leftHeader, const QString &rightHeader);

        // 데이터 주입
        void setDiffRows(const QList<DiffRow> &rows);

        // 폴더 비교 모드 설정
        void setupFolderDiffMode(const Json::Value &result,
                                 const QString     &leftRootPath,
                                 const QString     &rightRootPath);

        // 공통 기능
        void clearAll();
        void setFileTypeInfo(const QString &leftType,
                             const QString &rightType,
                             bool           compatible,
                             const QString &message);

        void applyTheme(bool isDark);

        // 내부 위젯 접근자
        QTableWidget *table() const
        {
            return table_;
        }

      signals:
        void rowActivated(const DiffRow &row);            // 테이블 더블클릭
        void fileDoubleClicked(const QString &filePath);  // 폴더 트리 더블클릭

      private slots:
        void onTableCellActivated(int row, int column);
        void onFilterChanged(int index);
        void onShowOnlyChangedToggled(bool checked);
        void onTreeItemDoubleClicked(QTreeWidgetItem * item, int column);

      private:
        struct DiffColors
        {
            QColor added;
            QColor removed;
            QColor changed;
            QColor error;
            QColor normal;

            // 라이트 모드
            static DiffColors forLightMode()
            {
                DiffColors c;
                c.added   = QColor(220, 252, 231);  // 연한 초록
                c.removed = QColor(254, 226, 226);  // 연한 빨강
                c.changed = QColor(255, 250, 205);  // 연한 노랑
                c.error   = QColor(255, 237, 213);  // 연한 주황
                c.normal  = QColor(255, 255, 255);  // 흰색
                return c;
            }

            // 다크 모드
            static DiffColors forDarkMode()
            {
                DiffColors c;
                c.added   = QColor(34, 84, 61);  // 어두운 초록
                c.removed = QColor(88, 28, 36);  // 어두운 빨강
                c.changed = QColor(82, 82, 40);  // 어두운 노랑
                c.error   = QColor(88, 59, 40);  // 어두운 주황
                c.normal  = QColor(30, 30, 30);  // 어두운 회색
                return c;
            }
        };

        // UI Components
        QVBoxLayout  *mainLayout_      = nullptr;
        QTableWidget *table_           = nullptr;
        QSplitter    *folderSplit_     = nullptr;
        QTreeWidget  *leftFolderTree_  = nullptr;
        QTreeWidget  *rightFolderTree_ = nullptr;

        // Controls
        QWidget   *topBar_               = nullptr;
        QComboBox *filterCombo_          = nullptr;
        QCheckBox *showOnlyChangedCheck_ = nullptr;
        QLabel    *typeInfoLabel_        = nullptr;
        QLabel    *statLabel_            = nullptr;

        // Data
        QList<DiffRow> allRows_;
        QString        currentFilter_ = "All";
        bool           isFolderMode_  = false;

        DiffColors colors_;
        bool       isDarkMode_ = false;

        // Helpers
        void   rebuildTable();
        bool   matchFilter(const DiffRow &row) const;
        QColor getColorForDiffState(const QString &state) const;

        void updateTreeColors(QTreeWidget * tree);
};

#endif  // DIFFTABLEPANEL_H
