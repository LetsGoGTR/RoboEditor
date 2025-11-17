#ifndef DIFFTABLEPANEL_H
#define DIFFTABLEPANEL_H
#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QSplitter>
#include <QList>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <json/json.h>

// DiffRow 구조체 정의
struct DiffRow {
    int     line = -1;
    int     leftLineNumber = -1;
    int     rightLineNumber = -1;
    QString key;    // Path 또는 Key
    QString origin; // Right Value (Base)
    QString target; // Left Value (Compare)
    QString state;  // "ADDED", "REMOVED", "CHANGED", "SAME", "ERROR", "MISMATCH"
};

class DiffTablePanel : public QWidget {
    Q_OBJECT
  public:
    explicit DiffTablePanel(QWidget *parent = nullptr);

    // 파일 비교 모드 설정
    void setupFileDiffMode(const QString &fileType,
                           const QString &leftHeader,
                           const QString &rightHeader);

    // 데이터 주입
    void setDiffRows(const QList<DiffRow> &rows);

    // 폴더 비교 모드 설정
    void setupFolderDiffMode(const Json::Value &result,
                             const QString &leftRootPath,
                             const QString &rightRootPath);

    // 공통 기능
    void clearAll();
    void setFileTypeInfo(const QString &leftType, const QString &rightType, bool compatible, const QString &message);

    // 내부 위젯 접근자
    QTableWidget* table() const { return table_; }

  signals:
    void rowActivated(const DiffRow &row);           // 테이블 더블클릭
    void fileDoubleClicked(const QString &filePath); // 폴더 트리 더블클릭

  private slots:
    void onTableCellActivated(int row, int column);
    void onFilterChanged(int index);
    void onShowOnlyChangedToggled(bool checked);
    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);

  private:
    // UI Components
    QVBoxLayout* mainLayout_           = nullptr;
    QTableWidget* table_               = nullptr;
    QSplitter* folderSplit_            = nullptr;
    QTreeWidget* leftFolderTree_       = nullptr;
    QTreeWidget* rightFolderTree_      = nullptr;

    // Controls
    QWidget* topBar_                 = nullptr;
    QComboBox* filterCombo_          = nullptr;
    QCheckBox* showOnlyChangedCheck_ = nullptr;
    QLabel* typeInfoLabel_           = nullptr;
    QLabel* statLabel_               = nullptr;

    // Data
    QList<DiffRow> allRows_;
    QString        currentFilter_ = "All";
    bool           isFolderMode_ = false;

    // Helpers
    void rebuildTable();
    bool matchFilter(const DiffRow &row) const;
    QColor getColorForDiffState(const QString &state) const;
};
#endif  // DIFFTABLEPANEL_H
