#include "ComparePage.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QToolButton>
#include <QTreeView>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QVBoxLayout>

#include "DropTextEdit.h"
#include "CodeEditor.h"

#include <json/json.h>
#include <sstream>

ComparePage::ComparePage(QWidget *parent) : QWidget(parent), currentFilter_("All")
{
    dock_  = buildDock();
    auto v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(dock_);
}

QWidget *ComparePage::buildDock()
{
    auto w = new QWidget(this);
    auto v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);

    // 내부 스플리터: (좌) 비교파일 탭 위젯 | (우) diff 패널
    rightSplit_ = new QSplitter(Qt::Horizontal, w);

    // 좌측: 비교 파일들을 탭으로 관리
    compareTabWidget_ = new QTabWidget(rightSplit_);
    compareTabWidget_->setTabsClosable(true);
    compareTabWidget_->setStyleSheet("QTabWidget::pane { "
                                     "  border: none; "
                                     "  background-color: white; "
                                     "} "
                                     "QTabBar::tab { "
                                     "  background-color: #f0f0f0; "
                                     "  border: 1px solid #ccc; "
                                     "  border-bottom: none; "
                                     "  border-top-left-radius: 3px; "
                                     "  border-top-right-radius: 3px; "
                                     "  padding: 4px 8px; "
                                     "  margin-right: 2px; "
                                     "  font-size: 9pt; "
                                     "} "
                                     "QTabBar::tab:selected { "
                                     "  background-color: white; "
                                     "  font-weight: bold; "
                                     "} "
                                     "QTabBar::tab:hover { "
                                     "  background-color: #e0e0e0; "
                                     "}");

    // 탭 닫기 시그널 연결 - 마지막 탭이 닫히면 Compare 전체 닫기
    connect(compareTabWidget_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        auto *widget = compareTabWidget_->widget(index);
        compareTabWidget_->removeTab(index);
        if (widget)
            widget->deleteLater();

        // 마지막 탭이 닫히면 Compare 전체를 닫음
        if (compareTabWidget_->count() == 0) {
            emit closed();
            return;
        }

        // 현재 활성 탭 업데이트
        if (compareTabWidget_->count() > 0) {
            int currentIndex = compareTabWidget_->currentIndex();
            if (currentIndex >= 0) {
                rightText_ = qobject_cast<CodeEditor *>(compareTabWidget_->currentWidget());
                // 남아있는 탭의 경로로 신호 발송 (diff 재계산)
                QString tabPath = compareTabWidget_->tabToolTip(currentIndex);
                if (!tabPath.isEmpty()) {
                    targetPath_ = tabPath;
                    emit targetPathChanged(tabPath);
                }
            }
        } else {
            rightText_ = nullptr;
        }
    });

    // 탭 변경 시 현재 활성 탭 업데이트
    connect(compareTabWidget_, &QTabWidget::currentChanged, this, [this](int index) {
        if (index >= 0) {
            rightText_ = qobject_cast<CodeEditor *>(compareTabWidget_->widget(index));
            // 탭이 변경될 때 targetPath 업데이트 및 신호 발송 (diff 재계산 트리거)
            QString tabPath = compareTabWidget_->tabToolTip(index);
            if (!tabPath.isEmpty()) {
                targetPath_ = tabPath;
                emit targetPathChanged(tabPath);
            }
        } else {
            rightText_ = nullptr;
        }
    });

    // 우측: diff 패널
    diffPanel_ = buildDiffPanel();

    rightSplit_->addWidget(compareTabWidget_);
    rightSplit_->addWidget(diffPanel_);
    rightSplit_->setStretchFactor(0, 4);  // compareTabWidget_ (파일 내용) - 2배 공간
    rightSplit_->setStretchFactor(1, 1);  // diffPanel_ (비교 테이블) - 1배 공간

    v->addWidget(rightSplit_);
    return w;
}

QWidget *ComparePage::buildDiffPanel()
{
    if (diffPanel_)
        return diffPanel_;
    diffPanel_ = new QWidget(this);
    diffPanel_->setStyleSheet("QWidget { "
                              "  background-color: white; "
                              "  border: none; "
                              "}");

    auto v = new QVBoxLayout(diffPanel_);
    v->setContentsMargins(8, 8, 8, 8);

    // 상단: 파일 선택 + 필터 버튼
    auto topBar = new QWidget(diffPanel_);
    topBar->setStyleSheet("QWidget { background-color: transparent; border: none; }");
    auto topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 4);
    topLayout->setSpacing(6);

    // 파일 선택 버튼 (두 파일 비교)
    auto btnSelectFile = new QPushButton(tr("Compare Files"), topBar);
    btnSelectFile->setFixedSize(110, 26);
    btnSelectFile->setStyleSheet("QPushButton { "
                                 "  background-color: #f0f0f0; "
                                 "  border: 1px solid #b0b0b0; "
                                 "  border-radius: 3px; "
                                 "  padding: 4px 8px; "
                                 "  font-size: 9pt; "
                                 "} "
                                 "QPushButton:hover { "
                                 "  background-color: #e0e0e0; "
                                 "  border: 1px solid #909090; "
                                 "} "
                                 "QPushButton:pressed { "
                                 "  background-color: #d0d0d0; "
                                 "}");
    topLayout->addWidget(btnSelectFile);

    connect(btnSelectFile, &QPushButton::clicked, this, [this]() {
        // 왼쪽 파일 선택 (비교 대상) - C:/backup 시작
        QString leftPath = QFileDialog::getOpenFileName(this, 
                                                        tr("Select left file (compare)"),
                                                        "C:/backup",
                                                        tr("All Files (*.*)"));
        if (leftPath.isEmpty()) {
            return;
        }

        // 오른쪽 파일 선택 (기준) - C:/backup 시작
        QString rightPath = QFileDialog::getOpenFileName(this, 
                                                         tr("Select right file (base)"),
                                                         "C:/backup",
                                                         tr("All Files (*.*)"));
        if (rightPath.isEmpty()) {
            return;
        }

        // 두 파일 비교 수행
        performDiff(leftPath, rightPath);
    });

    // 구분선
    auto separator1 = new QFrame(topBar);
    separator1->setFrameShape(QFrame::VLine);
    separator1->setFrameShadow(QFrame::Sunken);
    topLayout->addWidget(separator1);

    // 필터 레이블
    auto filterLabel = new QLabel(tr("Filter:"), topBar);
    topLayout->addWidget(filterLabel);

    // 필터 버튼들 - 좌측 정렬
    auto btnAll     = new QPushButton(tr("All"), topBar);
    auto btnAdded   = new QPushButton(tr("Added"), topBar);
    auto btnRemoved = new QPushButton(tr("Removed"), topBar);
    auto btnChanged = new QPushButton(tr("Changed"), topBar);

    btnAll->setCheckable(true);
    btnAdded->setCheckable(true);
    btnRemoved->setCheckable(true);
    btnChanged->setCheckable(true);
    btnAll->setChecked(true);

    // 버튼 크기 통일
    btnAll->setFixedSize(50, 26);
    btnAdded->setFixedSize(60, 26);
    btnRemoved->setFixedSize(70, 26);
    btnChanged->setFixedSize(70, 26);

    // 필터 버튼 스타일
    QString filterBtnStyle = "QPushButton { "
                             "  background-color: #ffffff; "
                             "  border: 1px solid #b0b0b0; "
                             "  border-radius: 3px; "
                             "  padding: 4px 8px; "
                             "  font-size: 9pt; "
                             "  color: #606060; "
                             "} "
                             "QPushButton:hover { "
                             "  background-color: #f5f5f5; "
                             "  border: 1px solid #909090; "
                             "} "
                             "QPushButton:checked { "
                             "  background-color: #007acc; "
                             "  border: 1px solid #005a9e; "
                             "  color: white; "
                             "  font-weight: bold; "
                             "} "
                             "QPushButton:checked:hover { "
                             "  background-color: #005a9e; "
                             "}";

    btnAll->setStyleSheet(filterBtnStyle);
    btnAdded->setStyleSheet(filterBtnStyle);
    btnRemoved->setStyleSheet(filterBtnStyle);
    btnChanged->setStyleSheet(filterBtnStyle);

    topLayout->addWidget(btnAll);
    topLayout->addWidget(btnAdded);
    topLayout->addWidget(btnRemoved);
    topLayout->addWidget(btnChanged);

    // 왼쪽으로 밀착
    topLayout->addStretch();

    // 구분선
    auto separator2 = new QFrame(topBar);
    separator2->setFrameShape(QFrame::VLine);
    separator2->setFrameShadow(QFrame::Sunken);
    topLayout->addWidget(separator2);

    // 통계 레이블 - 우측
    if (!statLabel_)
        statLabel_ = new QLabel("Changes: 0", topBar);
    statLabel_->setStyleSheet("QLabel { font-weight: bold; }");
    topLayout->addWidget(statLabel_);

    v->addWidget(topBar);

    // 필터 버튼 연결 (상호 배타적)
    connect(btnAll, &QPushButton::clicked, this, [=, this]() {
        btnAll->setChecked(true);
        btnAdded->setChecked(false);
        btnRemoved->setChecked(false);
        btnChanged->setChecked(false);
        applyFilter("All");
    });
    connect(btnAdded, &QPushButton::clicked, this, [=, this]() {
        btnAll->setChecked(false);
        btnAdded->setChecked(true);
        btnRemoved->setChecked(false);
        btnChanged->setChecked(false);
        applyFilter("Added");
    });
    connect(btnRemoved, &QPushButton::clicked, this, [=, this]() {
        btnAll->setChecked(false);
        btnAdded->setChecked(false);
        btnRemoved->setChecked(true);
        btnChanged->setChecked(false);
        applyFilter("Removed");
    });
    connect(btnChanged, &QPushButton::clicked, this, [=, this]() {
        btnAll->setChecked(false);
        btnAdded->setChecked(false);
        btnRemoved->setChecked(false);
        btnChanged->setChecked(true);
        applyFilter("Changed");
    });

    // ===== Diff 테이블 =====
    if (!diffTable_) {
        diffTable_ = new QTableWidget(diffPanel_);
    }

    // 컬럼 설정
    diffTable_->setColumnCount(4);
    diffTable_->setHorizontalHeaderLabels({"Key", "Left", "Right", "State"});

    // 가로 헤더 설정
    QHeaderView *hHeader = diffTable_->horizontalHeader();
    hHeader->setVisible(true);                                // 헤더 항상 표시
    hHeader->setStretchLastSection(false);                    // 마지막 컬럼 자동 늘림 OFF (테이블 크기 줄이기)
    hHeader->setSectionResizeMode(QHeaderView::Interactive);  // 사용자가 컬럼 크기 조절 가능
    hHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hHeader->setHighlightSections(false);
    hHeader->setStyleSheet("QHeaderView::section { "
                           "  background-color: #f5f5f5; "
                           "  border: 1px solid #d0d0d0; "
                           "  padding: 4px 8px; "
                           "  font-weight: bold; "
                           "  font-size: 9pt; "
                           "}");

    // 세로 헤더 설정
    QHeaderView *vHeader = diffTable_->verticalHeader();
    vHeader->setVisible(true);
    vHeader->setDefaultSectionSize(20);  // 행 높이 통일
    vHeader->setSectionResizeMode(QHeaderView::Fixed);

    // 테이블 동작 설정
    diffTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    diffTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    diffTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    diffTable_->setAlternatingRowColors(true);

    // 테이블 스타일 (스크롤바 포함)
    diffTable_->setStyleSheet("QTableWidget { "
                              "  background-color: white; "
                              "  gridline-color: #e0e0e0; "
                              "  border: none; "
                              "  font-family: 'Consolas', 'Monaco', 'Courier New', monospace; "
                              "  font-size: 9pt; "
                              "} "
                              "QTableWidget::item { "
                              "  padding: 4px 8px; "
                              "  border: none; "
                              "} "
                              "QTableWidget::item:selected { "
                              "  background-color: #cce8ff; "
                              "  color: black; "
                              "} "
                              "QTableWidget::item:alternate { "
                              "  background-color: #fafafa; "
                              "} "
                              /* 세로 스크롤바 */
                              "QScrollBar:vertical { "
                              "  background: #f0f0f0; "
                              "  width: 14px; "
                              "  border: none; "
                              "  margin: 0px; "
                              "} "
                              "QScrollBar::handle:vertical { "
                              "  background: #c0c0c0; "
                              "  min-height: 20px; "
                              "  border-radius: 7px; "
                              "  margin: 2px; "
                              "} "
                              "QScrollBar::handle:vertical:hover { "
                              "  background: #a0a0a0; "
                              "} "
                              "QScrollBar::handle:vertical:pressed { "
                              "  background: #808080; "
                              "} "
                              "QScrollBar::add-line:vertical, "
                              "QScrollBar::sub-line:vertical { "
                              "  height: 0px; "
                              "} "
                              "QScrollBar::add-page:vertical, "
                              "QScrollBar::sub-page:vertical { "
                              "  background: none; "
                              "} "
                              /* 가로 스크롤바 */
                              "QScrollBar:horizontal { "
                              "  background: #f0f0f0; "
                              "  height: 14px; "
                              "  border: none; "
                              "  margin: 0px; "
                              "} "
                              "QScrollBar::handle:horizontal { "
                              "  background: #c0c0c0; "
                              "  min-width: 20px; "
                              "  border-radius: 7px; "
                              "  margin: 2px; "
                              "} "
                              "QScrollBar::handle:horizontal:hover { "
                              "  background: #a0a0a0; "
                              "} "
                              "QScrollBar::handle:horizontal:pressed { "
                              "  background: #808080; "
                              "} "
                              "QScrollBar::add-line:horizontal, "
                              "QScrollBar::sub-line:horizontal { "
                              "  width: 0px; "
                              "} "
                              "QScrollBar::add-page:horizontal, "
                              "QScrollBar::sub-page:horizontal { "
                              "  background: none; "
                              "}");

    // 초기 컬럼 너비 설정 (최소 크기)
    diffTable_->setColumnWidth(0, 60);   // Key/Line
    diffTable_->setColumnWidth(1, 250);  // Left (Compare)
    diffTable_->setColumnWidth(2, 250);  // Right (Base)
    diffTable_->setColumnWidth(3, 100);  // State

    v->addWidget(diffTable_);

    return diffPanel_;
}

void ComparePage::onCloseClicked()
{
    emit closed();
}

void ComparePage::setTargetPath(const QString &path)
{
    if (path.isEmpty())
        return;

    // compareTabWidget_이 null인지 체크
    if (!compareTabWidget_) {
        qWarning() << "[ComparePage] compareTabWidget_ is null";
        return;
    }

    targetPath_ = path;
    QFileInfo fileInfo(path);
    QString   fileName     = fileInfo.fileName();
    QString   absolutePath = fileInfo.absoluteFilePath();

    // 기존 탭을 모두 닫음 (새 파일로 교체)
    while (compareTabWidget_->count() > 0) {
        auto *widget = compareTabWidget_->widget(0);
        compareTabWidget_->removeTab(0);
        if (widget)
            widget->deleteLater();
    }

    // 새 탭 추가 (CodeEditor 사용 - 라인 번호 포함)
    auto *newTextEdit = new CodeEditor(compareTabWidget_);
    newTextEdit->setReadOnly(true);
    newTextEdit->setStyleSheet("QPlainTextEdit { background-color: white; border: none; }");

    // 파일 내용 로드
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream   in(&f);
        const QString content = in.readAll();
        f.close();
        newTextEdit->setPlainText(content);
    } else {
        newTextEdit->setPlainText(tr("Failed to open: %1").arg(path));
    }

    // 탭 추가 및 활성화 (툴팁에 전체 경로 저장)
    int index = compareTabWidget_->addTab(newTextEdit, fileName);
    compareTabWidget_->setTabToolTip(index, absolutePath);
    compareTabWidget_->setCurrentIndex(index);
    rightText_ = newTextEdit;

    emit targetPathChanged(path);
}

void ComparePage::recalcDiff(const QString &leftText)
{
    // compareTabWidget_이 null이거나 탭이 없으면 리턴
    if (!compareTabWidget_ || compareTabWidget_->count() == 0) {
        return;
    }

    const QString rightText = rightText_ ? rightText_->toPlainText() : QString();

    // (임시) 빈 diff라도 테이블이 null이 아니도록 보장
    if (!diffTable_)
        buildDiffPanel();  // 방어적
    if (!diffTable_)
        return;

    // targetPath_가 없으면 미리보기만 표시
    if (targetPath_.isEmpty()) {
    QList<DiffRow> rows;
    if (!leftText.isEmpty() || !rightText.isEmpty()) {
        rows.push_back({-1,  // line
                        "(preview)",
                        QString::number(leftText.size()),
                        QString::number(rightText.size()),
                        leftText == rightText ? "SAME" : "CHANGED"});
    }
    setDiffRows(rows);
        return;
    }

    // DiffService를 사용하여 diff 수행
    // 주의: right가 기준(base), left가 비교 대상(compare)
    std::string rightContentStr = rightText.toStdString();
    std::string leftContentStr  = leftText.toStdString();
    std::string rightNameStr    = targetPath_.toStdString();
    std::string leftNameStr     = "ModifyPage"; // 임시 이름
    
    services::ServiceResult result = services::DiffService::diff(rightContentStr, 
                                                                  leftContentStr,
                                                                  rightNameStr,
                                                                  leftNameStr);
    
    if (!result.success) {
        QList<DiffRow> rows;
        rows.push_back({-1, "Error", QString::fromStdString(result.errorMessage), "", "ERROR"});
        setDiffRows(rows);
        return;
    }
    
    // 파일 타입 감지 (확장자 기반)
    QFileInfo fileInfo(targetPath_);
    QString ext = fileInfo.suffix().toLower();
    QString fileType;
    
    if (ext == "yaml" || ext == "yml" || ext == "pts") {
        fileType = "yaml";
    } else if (ext == "py" || ext == "srl" || ext == "sbp") {
        fileType = "python";
    } else {
        fileType = "text";
    }
    
    // 테이블 컬럼 업데이트
    updateTableColumns(fileType);
    
    // 결과 파싱 및 표시
    QList<DiffRow> diffRows = parseDiffResult(result.data, fileType);
    setDiffRows(diffRows);
}


// diff 테이블 갱신
void ComparePage::refreshDiffTable(const QList<DiffRow> &rows)
{
    diffTable_->setRowCount(rows.size());
    int changeCount  = 0;
    int addedCount   = 0;
    int removedCount = 0;
    int changedCount = 0;

    for (int i = 0; i < rows.size(); i++) {
        const auto &r = rows[i];

        // YAML(5컬럼)과 Python/Text(4컬럼)를 구분
        bool isYaml = (diffTable_->columnCount() == 5);
        
        QTableWidgetItem *itemLine = nullptr;
        if (isYaml) {
            // Line 컬럼 생성 (YAML만)
            itemLine = new QTableWidgetItem(r.line >= 0 ? QString::number(r.line) : "");
        }
        
        auto *itemKey   = new QTableWidgetItem(r.key);
        auto *itemLeft  = new QTableWidgetItem(r.target);  // target = Left (Compare)
        auto *itemRight = new QTableWidgetItem(r.origin);  // origin = Right (Base)
        auto *itemState = new QTableWidgetItem(r.state);

        // State에 따라 명확한 색상과 아이콘 적용
        if (r.state == "CHANGED") {
            // 노란색 - 변경됨
            QColor bgColor(255, 250, 205);   // 레몬 크림색
            QColor textColor(184, 134, 11);  // 어두운 황금색

            if (itemLine) itemLine->setBackground(bgColor);
            itemKey->setBackground(bgColor);
            itemLeft->setBackground(bgColor);
            itemRight->setBackground(bgColor);
            itemState->setBackground(bgColor);

            itemState->setForeground(QBrush(textColor));
            QFont boldFont = itemState->font();
            boldFont.setBold(true);
            itemState->setFont(boldFont);
            itemState->setText("● CHANGED");

            changedCount++;
            changeCount++;

        } else if (r.state == "ADDED") {
            // 초록색 - 추가됨
            QColor bgColor(220, 252, 231);  // 연한 민트색
            QColor textColor(22, 163, 74);  // 진한 초록색

            if (itemLine) itemLine->setBackground(bgColor);
            itemKey->setBackground(bgColor);
            itemLeft->setBackground(bgColor);
            itemRight->setBackground(bgColor);
            itemState->setBackground(bgColor);

            itemState->setForeground(QBrush(textColor));
            QFont boldFont = itemState->font();
            boldFont.setBold(true);
            itemState->setFont(boldFont);
            itemState->setText("+ ADDED");

            addedCount++;
            changeCount++;

        } else if (r.state == "REMOVED") {
            // 빨간색 - 삭제됨
            QColor bgColor(254, 226, 226);  // 연한 핑크색
            QColor textColor(220, 38, 38);  // 진한 빨강색

            if (itemLine) itemLine->setBackground(bgColor);
            itemKey->setBackground(bgColor);
            itemLeft->setBackground(bgColor);
            itemRight->setBackground(bgColor);
            itemState->setBackground(bgColor);

            itemState->setForeground(QBrush(textColor));
            QFont boldFont = itemState->font();
            boldFont.setBold(true);
            itemState->setFont(boldFont);
            itemState->setText("− REMOVED");

            removedCount++;
            changeCount++;

        } else if (r.state == "SAME") {
            // 회색 - 동일함
            QColor bgColor(249, 250, 251);    // 아주 연한 회색
            QColor textColor(107, 114, 128);  // 중간 회색

            if (itemLine) itemLine->setBackground(bgColor);
            itemKey->setBackground(bgColor);
            itemLeft->setBackground(bgColor);
            itemRight->setBackground(bgColor);
            itemState->setBackground(bgColor);

            itemState->setForeground(QBrush(textColor));
            itemState->setText("= SAME");
        }

        // 텍스트 정렬
        if (itemLine) itemLine->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        itemKey->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        itemLeft->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        itemRight->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        itemState->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);

        // 테이블에 아이템 설정
        if (isYaml) {
            // YAML: Line, Path, Left Value, Right Value, State
            diffTable_->setItem(i, 0, itemLine);
            diffTable_->setItem(i, 1, itemKey);
            diffTable_->setItem(i, 2, itemLeft);   // Column 2: Left Content (Compare)
            diffTable_->setItem(i, 3, itemRight);  // Column 3: Right Content (Base)
            diffTable_->setItem(i, 4, itemState);
        } else {
            // Python/Text: Line, Left Content, Right Content, State
            diffTable_->setItem(i, 0, itemKey);
            diffTable_->setItem(i, 1, itemLeft);   // Column 1: Left Content (Compare)
            diffTable_->setItem(i, 2, itemRight);  // Column 2: Right Content (Base)
            diffTable_->setItem(i, 3, itemState);
        }
    }

    // 통계 업데이트
    if (statLabel_) {
        QString stats = QString("Changes: %1").arg(changeCount);
        if (addedCount > 0 || removedCount > 0 || changedCount > 0) {
            stats += QString(" (+%1 -%2 ~%3)").arg(addedCount).arg(removedCount).arg(changedCount);
        }
        statLabel_->setText(stats);
    }
}

void ComparePage::setDiffRows(const QList<DiffRow> &rows)
{
    allDiffRows_ = rows;  // 전체 데이터 저장
    
    // 현재 필터 적용
    QList<DiffRow> filteredRows = filterRows(rows, currentFilter_);
    refreshDiffTable(filteredRows);
}


// ========================================
// DiffService를 사용한 diff 수행 구현
// ========================================

void ComparePage::performDiff(const QString &leftPath, const QString &rightPath)
{
    // 파일 내용 읽기
    QFile leftFile(leftPath);
    QFile rightFile(rightPath);
    
    if (!leftFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("오류"), tr("왼쪽 파일을 열 수 없습니다: %1").arg(leftPath));
        return;
    }
    
    if (!rightFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("오류"), tr("오른쪽 파일을 열 수 없습니다: %1").arg(rightPath));
        leftFile.close();
        return;
    }
    
    QTextStream leftStream(&leftFile);
    QTextStream rightStream(&rightFile);
    
    QString leftContent  = leftStream.readAll();
    QString rightContent = rightStream.readAll();
    
    leftFile.close();
    rightFile.close();
    
    // DiffService를 사용하여 diff 수행
    // 주의: right가 기준(base), left가 비교 대상(compare)
    std::string rightContentStr = rightContent.toStdString();
    std::string leftContentStr  = leftContent.toStdString();
    std::string rightNameStr    = rightPath.toStdString();
    std::string leftNameStr     = leftPath.toStdString();
    
    services::ServiceResult result = services::DiffService::diff(rightContentStr, 
                                                                  leftContentStr,
                                                                  rightNameStr,
                                                                  leftNameStr);
    
    // 오류 처리
    if (!result.success) {
        QMessageBox::critical(this, 
                            tr("Diff 오류"), 
                            QString::fromStdString(result.errorMessage));
        return;
    }
    
    // 오른쪽 파일을 탭에 표시
    if (compareTabWidget_) {
        // 기존 탭을 모두 닫음
        while (compareTabWidget_->count() > 0) {
            auto *widget = compareTabWidget_->widget(0);
            compareTabWidget_->removeTab(0);
            if (widget)
                widget->deleteLater();
        }
        
        // 오른쪽 파일을 새 탭으로 추가 (CodeEditor 사용 - 라인 번호 포함)
        auto *newTextEdit = new CodeEditor(compareTabWidget_);
        newTextEdit->setReadOnly(true);
        newTextEdit->setStyleSheet("QPlainTextEdit { background-color: white; border: none; }");
        newTextEdit->setPlainText(rightContent);
        
        QFileInfo rightFileInfo(rightPath);
        QString   rightFileName     = rightFileInfo.fileName();
        QString   rightAbsolutePath = rightFileInfo.absoluteFilePath();
        
        int index = compareTabWidget_->addTab(newTextEdit, rightFileName);
        compareTabWidget_->setTabToolTip(index, rightAbsolutePath);
        compareTabWidget_->setCurrentIndex(index);
        rightText_ = newTextEdit;
        
        targetPath_ = rightAbsolutePath;
    }
    
    // 파일 타입 감지
    QFileInfo fileInfo(rightPath);
    QString ext = fileInfo.suffix().toLower();
    QString fileType;
    
    if (ext == "yaml" || ext == "yml" || ext == "pts") {
        fileType = "yaml";
    } else if (ext == "py" || ext == "srl" || ext == "sbp") {
        fileType = "python";
    } else {
        fileType = "text";
    }
    
    // 테이블 컬럼 업데이트
    updateTableColumns(fileType);
    
    // 결과 파싱 및 표시
    QList<DiffRow> diffRows = parseDiffResult(result.data, fileType);
    setDiffRows(diffRows);
    
    // 왼쪽 파일도 표시하고 싶다면 신호를 발생시킴
    emit uiCompareClicked(leftPath, rightPath);
}

void ComparePage::updateTableColumns(const QString &fileType)
{
    if (!diffTable_) return;
    
    if (fileType == "yaml") {
        // YAML: Line, Path, Left Value, Right Value, State
        diffTable_->setColumnCount(5);
        diffTable_->setHorizontalHeaderLabels({"Line", "Path", "Left Value (Compare)", "Right Value (Base)", "State"});
        diffTable_->setColumnWidth(0, 50);   // Line
        diffTable_->setColumnWidth(1, 120);  // Path
        diffTable_->setColumnWidth(2, 150);  // Left Value
        diffTable_->setColumnWidth(3, 150);  // Right Value
        diffTable_->setColumnWidth(4, 100);  // State
    } else if (fileType == "python" || fileType == "text") {
        // Python/Text: Line, Left Content, Right Content, State (순서 변경)
        diffTable_->setColumnCount(4);
        diffTable_->setHorizontalHeaderLabels({"Line", "Left Content (Compare)", "Right Content (Base)", "State"});
        diffTable_->setColumnWidth(0, 50);   // Line (줄임)
        diffTable_->setColumnWidth(1, 150);  // Left Content (줄임)
        diffTable_->setColumnWidth(2, 150);  // Right Content (줄임)
        diffTable_->setColumnWidth(3, 100);  // State
    } else {
        // 기본값
        diffTable_->setColumnCount(4);
        diffTable_->setHorizontalHeaderLabels({"Key", "Left", "Right", "State"});
        diffTable_->setColumnWidth(0, 50);
        diffTable_->setColumnWidth(1, 150);
        diffTable_->setColumnWidth(2, 150);
        diffTable_->setColumnWidth(3, 100);
    }
}

// DiffService 결과를 DiffRow로 변환
QList<ComparePage::DiffRow> ComparePage::parseDiffResult(const Json::Value &result, 
                                                          const QString &fileType)
{
    QList<DiffRow> rows;
    
    try {
        if (fileType == "yaml") {
            // YAML diff 결과 파싱
            if (result.isMember("changes") && result["changes"].isArray()) {
                const Json::Value &changes = result["changes"];
                
                for (Json::ArrayIndex i = 0; i < changes.size(); ++i) {
                    const Json::Value &change = changes[i];
                    
                    DiffRow row;
                    
                    // 라인 번호 추출 (oldLineNumber 또는 newLineNumber)
                    row.line = -1;
                    if (change.isMember("oldLineNumber") && !change["oldLineNumber"].isNull()) {
                        row.line = change["oldLineNumber"].asInt();
                    } else if (change.isMember("newLineNumber") && !change["newLineNumber"].isNull()) {
                        row.line = change["newLineNumber"].asInt();
                    }
                    
                    // path 추출
                    if (change.isMember("path")) {
                        row.key = QString::fromStdString(change["path"].asString());
                    }
                    
                    // oldValue (right - base)
                    if (change.isMember("oldValue") && !change["oldValue"].isNull()) {
                        Json::StreamWriterBuilder builder;
                        builder["indentation"] = "";
                        row.origin = QString::fromStdString(Json::writeString(builder, change["oldValue"]));
                        // JSON 따옴표 제거
                        if (row.origin.startsWith('"') && row.origin.endsWith('"')) {
                            row.origin = row.origin.mid(1, row.origin.length() - 2);
                        }
                    }
                    
                    // newValue (left - compare)
                    if (change.isMember("newValue") && !change["newValue"].isNull()) {
                        Json::StreamWriterBuilder builder;
                        builder["indentation"] = "";
                        row.target = QString::fromStdString(Json::writeString(builder, change["newValue"]));
                        // JSON 따옴표 제거
                        if (row.target.startsWith('"') && row.target.endsWith('"')) {
                            row.target = row.target.mid(1, row.target.length() - 2);
                        }
                    }
                    
                    // type을 state로 변환
                    if (change.isMember("type")) {
                        std::string type = change["type"].asString();
                        if (type == "added") {
                            row.state = "ADDED";
                        } else if (type == "removed") {
                            row.state = "REMOVED";
                        } else if (type == "modified") {
                            row.state = "CHANGED";
                        } else {
                            row.state = "SAME";
                        }
                    }
                    
                    rows.append(row);
                }
                
                // 라인 번호로 정렬
                std::sort(rows.begin(), rows.end(), [](const DiffRow &a, const DiffRow &b) {
                    return a.line < b.line;
                });
            }
        } else if (fileType == "python") {
            // Python diff 결과 파싱
            if (result.isMember("changes") && result["changes"].isArray()) {
                const Json::Value &changes = result["changes"];
                
                for (Json::ArrayIndex i = 0; i < changes.size(); ++i) {
                    const Json::Value &change = changes[i];
                    
                    DiffRow row;
                    
                    // baseLineNumber 또는 compareLineNumber를 Line과 Key로 사용
                    row.line = -1;
                    if (change.isMember("baseLineNumber") && !change["baseLineNumber"].isNull()) {
                        row.line = change["baseLineNumber"].asInt();
                        row.key = QString::number(row.line);
                    } else if (change.isMember("compareLineNumber") && !change["compareLineNumber"].isNull()) {
                        row.line = change["compareLineNumber"].asInt();
                        row.key = QString::number(row.line);
                    } else {
                        row.line = i + 1;
                        row.key = QString::number(row.line);
                    }
                    
                    // baseValue (right - base)
                    if (change.isMember("baseValue") && !change["baseValue"].isNull()) {
                        row.origin = QString::fromStdString(change["baseValue"].asString());
                    }
                    
                    // compareValue (left - compare)
                    if (change.isMember("compareValue") && !change["compareValue"].isNull()) {
                        row.target = QString::fromStdString(change["compareValue"].asString());
                    }
                    
                    // type을 state로 변환
                    if (change.isMember("type")) {
                        std::string type = change["type"].asString();
                        if (type == "added") {
                            row.state = "ADDED";
                        } else if (type == "deleted") {
                            row.state = "REMOVED";
                        } else if (type == "modified") {
                            row.state = "CHANGED";
    } else {
                            row.state = "SAME";
                        }
                    }
                    
                    rows.append(row);
                }
            }
        } else if (fileType == "text") {
            // Text diff 결과 파싱
            if (result.isMember("changes") && result["changes"].isArray()) {
                const Json::Value &changes = result["changes"];
                
                for (Json::ArrayIndex i = 0; i < changes.size(); ++i) {
                    const Json::Value &change = changes[i];
                    
                    DiffRow row;
                    
                    // lineNumber를 Line과 Key로 사용
                    row.line = -1;
                    if (change.isMember("lineNumber")) {
                        row.line = change["lineNumber"].asInt();
                        row.key = QString::number(row.line);
                    }
                    
                    // oldLine (right - base)
                    if (change.isMember("oldLine")) {
                        row.origin = QString::fromStdString(change["oldLine"].asString());
                    }
                    
                    // newLine (left - compare)
                    if (change.isMember("newLine")) {
                        row.target = QString::fromStdString(change["newLine"].asString());
                    }
                    
                    // type을 state로 변환
                    if (change.isMember("type")) {
                        std::string type = change["type"].asString();
                        if (type == "added") {
                            row.state = "ADDED";
                        } else if (type == "removed") {
                            row.state = "REMOVED";
                        } else if (type == "modified") {
                            row.state = "CHANGED";
    } else {
                            row.state = "SAME";
                        }
                    }
                    
                    rows.append(row);
                }
            }
        }
        
    } catch (const std::exception &e) {
        qWarning() << "Parse diff result error:" << e.what();
        DiffRow errorRow;
        errorRow.key    = "Error";
        errorRow.origin = QString::fromUtf8(e.what());
        errorRow.target = "";
        errorRow.state  = "ERROR";
        rows.append(errorRow);
    }
    
    return rows;
}

// ========================================
// 필터 기능 구현
// ========================================

void ComparePage::applyFilter(const QString &filterType)
{
    currentFilter_ = filterType;
    
    // 전체 데이터에 필터 적용
    QList<DiffRow> filteredRows = filterRows(allDiffRows_, filterType);
    refreshDiffTable(filteredRows);
}

QList<ComparePage::DiffRow> ComparePage::filterRows(const QList<DiffRow> &rows, 
                                                     const QString &filterType) const
{
    if (filterType == "All") {
        return rows;
    }
    
    QList<DiffRow> filtered;
    
    for (const auto &row : rows) {
        if (filterType == "Added" && row.state == "ADDED") {
            filtered.append(row);
        } else if (filterType == "Removed" && row.state == "REMOVED") {
            filtered.append(row);
        } else if (filterType == "Changed" && row.state == "CHANGED") {
            filtered.append(row);
        }
    }
    
    return filtered;
}
