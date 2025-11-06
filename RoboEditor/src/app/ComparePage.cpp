#include "ComparePage.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QToolButton>
#include <QTreeView>
#include <QTreeWidget>

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
#include <QUuid>
#include <QListView>
#include <QList>
#include <QSet>

#include "DropTextEdit.h"
#include "CodeEditor.h"
#include "FlowLayout.h"

#include <json/json.h>
#include <sstream>
#include <yaml-cpp/yaml.h>

// Core services
#include "../core/services/WorkspaceService.h"

ComparePage::ComparePage(QWidget *parent) : QWidget(parent), currentFilter_("All")
{
    // DiffHighlighter 객체 생성 (좌측/우측 각각)
    leftDiffHighlighter_ = new core::DiffHighlighter(this);
    rightDiffHighlighter_ = new core::DiffHighlighter(this);
    
    dock_  = buildDock();
    auto v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(dock_);
}

ComparePage::~ComparePage()
{
    // 임시 폴더 정리
    cleanupTempFolders();
}

QByteArray ComparePage::saveSplitterState() const
{
    if (rightSplit_) {
        return rightSplit_->saveState();
    }
    return QByteArray();
}

void ComparePage::restoreSplitterState(const QByteArray &state)
{
    if (rightSplit_ && !state.isEmpty()) {
        rightSplit_->restoreState(state);
    }
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

    // 상단: 파일 선택 + 필터 버튼 (FlowLayout 사용 - 자동 줄바꿈)
    auto topBar = new QWidget(diffPanel_);
    topBar->setStyleSheet("QWidget { background-color: transparent; border: none; }");
    auto topLayout = new FlowLayout(topBar, 0, 6, 6);  // margin=0, hSpacing=6, vSpacing=6

    // 파일 선택 버튼 (두 파일 비교)
    auto btnSelectFile = new QPushButton(tr("Compare Files"), topBar);
    btnSelectFile->setFixedSize(100, 26);
    
    QString buttonStyle = "QPushButton { "
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
                          "}";
    
    btnSelectFile->setStyleSheet(buttonStyle);
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
    
    // 폴더 비교 버튼 추가
    auto btnSelectFolder = new QPushButton(tr("Compare Folders"), topBar);
    btnSelectFolder->setFixedSize(120, 26);
    btnSelectFolder->setStyleSheet(buttonStyle);
    topLayout->addWidget(btnSelectFolder);
    
    connect(btnSelectFolder, &QPushButton::clicked, this, [this]() {
        // 폴더 또는 압축 파일 선택 다이얼로그 (커스텀)
        QFileDialog dialog(this, tr("Select folder or archive (compare)"), "C:/backup");
        dialog.setFileMode(QFileDialog::Directory);
        dialog.setOption(QFileDialog::ShowDirsOnly, false);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setNameFilter(tr("Folders and Archives (*.zip *.tar *.tar.gz *.tgz)"));
        
        // 폴더와 파일 모두 선택 가능하도록 설정
        QListView *listView = dialog.findChild<QListView*>("listView");
        if (listView) {
            listView->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        
        QString leftPath;
        if (dialog.exec() == QDialog::Accepted) {
            QStringList paths = dialog.selectedFiles();
            if (!paths.isEmpty()) {
                leftPath = paths.first();
            }
        }
        
        if (leftPath.isEmpty()) {
            return;
        }
        
        // 두 번째 선택
        QFileDialog dialog2(this, tr("Select folder or archive (base)"), "C:/backup");
        dialog2.setFileMode(QFileDialog::Directory);
        dialog2.setOption(QFileDialog::ShowDirsOnly, false);
        dialog2.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog2.setNameFilter(tr("Folders and Archives (*.zip *.tar *.tar.gz *.tgz)"));
        
        QString rightPath;
        if (dialog2.exec() == QDialog::Accepted) {
            QStringList paths = dialog2.selectedFiles();
            if (!paths.isEmpty()) {
                rightPath = paths.first();
            }
        }
        
        if (rightPath.isEmpty()) {
            return;
        }
        
        // 폴더 비교 수행
        performFolderDiff(leftPath, rightPath);
    });

    // 필터 레이블
    auto filterLabel = new QLabel(tr("Filter:"), topBar);
    filterLabel->setStyleSheet("QLabel { font-weight: bold; }");
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

    v->addWidget(topBar);
    
    // 통계 레이블 - 별도 위젯으로 분리 (좌측 정렬)
    auto statsBar = new QWidget(diffPanel_);
    statsBar->setStyleSheet("QWidget { background-color: transparent; border: none; }");
    auto statsLayout = new QHBoxLayout(statsBar);
    statsLayout->setContentsMargins(0, 4, 0, 4);
    statsLayout->setSpacing(6);
    
    if (!statLabel_)
        statLabel_ = new QLabel("Changes: 0", statsBar);
    statLabel_->setStyleSheet("QLabel { font-weight: bold; font-size: 9pt; }");
    statsLayout->addWidget(statLabel_);
    
    statsLayout->addStretch();
    
    v->addWidget(statsBar);

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
    
    // 테이블 행 클릭 시 해당 라인으로 스크롤 (양쪽 모두)
    connect(diffTable_, &QTableWidget::cellClicked, this, [this](int row, int column) {
        if (row < 0 || row >= diffTable_->rowCount())
            return;
        
        // 첫 번째 컬럼(Line 또는 Key)에서 라인 번호 추출
        QTableWidgetItem *lineItem = diffTable_->item(row, 0);
        if (lineItem) {
            bool ok;
            int lineNumber = lineItem->text().toInt(&ok);
            if (ok && lineNumber > 0) {
                // 양쪽 편집기 모두 스크롤
                if (rightText_) {
                    rightText_->scrollToLine(lineNumber);
                }
                if (leftText_) {
                    leftText_->scrollToLine(lineNumber);
                }
            }
        }
    });

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

void ComparePage::setLeftEditor(CodeEditor *leftEditor)
{
    leftText_ = leftEditor;
    
    // 좌측 편집기에 좌측 DiffHighlighter 설정
    if (leftText_) {
        leftText_->setDiffHighlighter(leftDiffHighlighter_);
        cachedLeftPath_ = leftText_->lastLoadedPath();
        cachedLeftText_ = leftText_->toPlainText();

        if (!targetPath_.isEmpty()) {
            recalcDiff(cachedLeftText_, cachedLeftPath_);
        }
    }
}

void ComparePage::clearHighlights()
{
    // 좌측 편집기의 하이라이트 제거 및 DiffHighlighter 연결 해제
    // leftText_는 ModifyPage의 CodeEditor이므로 DiffHighlighter 연결을 완전히 제거해야 함
    if (leftText_) {
        leftText_->clearDiffHighlights();
        
        // CodeEditor에서 DiffHighlighter 연결 완전히 제거 (삭제된 객체 참조 방지)
        leftText_->setDiffHighlighter(nullptr);
    }
    
    // 우측 편집기의 하이라이트 제거
    if (rightText_) {
        rightText_->clearDiffHighlights();
        
        // 우측 편집기에서도 DiffHighlighter 연결 제거
        rightText_->setDiffHighlighter(nullptr);
    }
    
    // DiffHighlighter의 상태도 초기화
    if (leftDiffHighlighter_) {
        leftDiffHighlighter_->clearLineStates();
    }
    if (rightDiffHighlighter_) {
        rightDiffHighlighter_->clearLineStates();
    }
    
    // leftText_ 포인터는 ComparePage가 삭제될 때 자동으로 무효화되므로
    // 여기서는 nullptr로 설정하지 않음
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

    QString leftContent = leftText_ ? leftText_->toPlainText() : cachedLeftText_;
    if (!cachedLeftPath_.isEmpty()) {
        recalcDiff(leftContent, cachedLeftPath_);
    }
}

void ComparePage::recalcDiff(const QString &leftText, const QString &leftPath)
{
    // compareTabWidget_이 null이거나 탭이 없으면 리턴
    if (!compareTabWidget_ || compareTabWidget_->count() == 0) {
        return;
    }

    const QString rightText = rightText_ ? rightText_->toPlainText() : QString();

    QString effectiveLeftPath = leftPath;
    if (effectiveLeftPath.isEmpty() && leftText_) {
        effectiveLeftPath = leftText_->lastLoadedPath();
    }

    cachedLeftText_ = leftText;
    cachedLeftPath_ = effectiveLeftPath;

    // (임시) 빈 diff라도 테이블이 null이 아니도록 보장
    if (!diffTable_)
        buildDiffPanel();  // 방어적
    if (!diffTable_)
        return;

    // targetPath_가 없으면 미리보기만 표시
    if (targetPath_.isEmpty()) {
    QList<DiffRow> rows;
    if (!leftText.isEmpty() || !rightText.isEmpty()) {
        DiffRow previewRow;
        previewRow.line = -1;
        previewRow.leftLineNumber = -1;
        previewRow.rightLineNumber = -1;
        previewRow.key = "(preview)";
        previewRow.origin = QString::number(rightText.size());
        previewRow.target = QString::number(leftText.size());
        previewRow.state = leftText == rightText ? "SAME" : "CHANGED";
        rows.append(previewRow);
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
    
    // 파일 타입 감지 (확장자 기반)
    QString fileType = detectFileType(targetPath_);
    QPair<QString, QString> columnHeaders = determineColumnHeaders(effectiveLeftPath, targetPath_);

    if (!effectiveLeftPath.isEmpty()) {
        QString leftFileType = detectFileType(effectiveLeftPath);
        QString leftSuffix   = QFileInfo(effectiveLeftPath).suffix();
        QString rightSuffix  = QFileInfo(targetPath_).suffix();

        bool suffixMismatch = !leftSuffix.isEmpty() && !rightSuffix.isEmpty() &&
                              leftSuffix.compare(rightSuffix, Qt::CaseInsensitive) != 0;

        if (suffixMismatch || !areFileTypesCompatible(leftFileType, fileType)) {
            updateTableColumns(fileType, columnHeaders.first, columnHeaders.second);

            QString normalizedLeftSuffix = leftSuffix.toUpper();
            if (normalizedLeftSuffix.isEmpty()) {
                normalizedLeftSuffix = leftFileType.isEmpty() ? tr("없음") : leftFileType.toUpper();
            }

            QString normalizedRightSuffix = rightSuffix.toUpper();
            if (normalizedRightSuffix.isEmpty()) {
                normalizedRightSuffix = fileType.isEmpty() ? tr("없음") : fileType.toUpper();
            }

            DiffRow errorRow;
            errorRow.key   = tr("확장자 불일치");
            errorRow.origin = tr("%1").arg(normalizedRightSuffix);
            errorRow.target = tr("%1").arg(normalizedLeftSuffix);
            errorRow.state  = "MISMATCH";

    QList<DiffRow> mismatchRows;
            mismatchRows.append(errorRow);
            setDiffRows(mismatchRows);
            return;
        }
    }
    
    services::ServiceResult result = services::DiffService::diff(rightContentStr, 
                                                                  leftContentStr,
                                                                  rightNameStr,
                                                                  leftNameStr);
    
    if (!result.success) {
        QList<DiffRow> rows;
        
        // YAML 파싱 에러인 경우, 에러 라인을 찾아서 표시
        if (fileType == "yaml") {
            bool leftError = false;
            bool rightError = false;
            int errorLine = -1;
            QString errorMsg = "YAML 형식 오류";
            
            // 좌측 파일 파싱 시도
            try {
                YAML::Load(leftText.toStdString());
            } catch (const YAML::Exception &e) {
                leftError = true;
                errorLine = e.mark.line + 1;  // 0-based -> 1-based
                errorMsg = QString("라인 %1: %2").arg(errorLine).arg(QString::fromStdString(e.msg));
            }
            
            // 우측 파일 파싱 시도
            try {
                YAML::Load(rightText.toStdString());
            } catch (const YAML::Exception &e) {
                rightError = true;
                // 좌측에 에러가 없으면 우측 에러 정보 사용
                if (!leftError) {
                    errorLine = e.mark.line + 1;
                    errorMsg = QString("비교 파일 라인 %1: %2").arg(errorLine).arg(QString::fromStdString(e.msg));
                }
            }
            
            // 에러가 있으면 에러 행 추가
            if (leftError || rightError) {
                DiffRow errorRow;
                errorRow.line = errorLine;
                errorRow.leftLineNumber = leftError ? errorLine : -1;
                errorRow.rightLineNumber = rightError ? errorLine : -1;
                errorRow.key = errorLine > 0 ? QString::number(errorLine) : "Parse Error";
                errorRow.origin = rightError ? "형식을 수정해주세요" : "";
                errorRow.target = leftError ? "형식을 수정해주세요" : "";
                errorRow.state = "ERROR";
                rows.append(errorRow);
                setDiffRows(rows);
                return;
            }
        }
        
        // 일반 에러 처리
        DiffRow errorRow;
        errorRow.line = -1;
        errorRow.leftLineNumber = -1;
        errorRow.rightLineNumber = -1;
        errorRow.key = "Error";
        errorRow.origin = "";
        errorRow.target = QString::fromStdString(result.errorMessage);
        errorRow.state = "ERROR";
        rows.append(errorRow);
        setDiffRows(rows);
        return;
    }
    
    // 테이블 컬럼 업데이트
            updateTableColumns(fileType, columnHeaders.first, columnHeaders.second);
    
    // 결과 파싱 및 표시 (core의 DiffService가 이미 모든 비교와 에러 처리를 수행함)
    QList<DiffRow> diffRows = parseDiffResult(result.data, fileType);
    
    // YAML 파싱 에러 체크 (빈 결과 + YAML 타입)
    // DiffYaml은 에러 발생 시 throw하지 않고 빈 벡터를 반환하므로
    // result.success는 true지만 결과가 비어있을 수 있음
    if (diffRows.isEmpty() && fileType == "yaml") {
        bool leftError = false;
        bool rightError = false;
        int errorLine = -1;
        QString errorMsg = "YAML 형식 오류";
        
        // 좌측 파일 파싱 시도
        try {
            YAML::Load(leftText.toStdString());
        } catch (const YAML::Exception &e) {
            leftError = true;
            errorLine = e.mark.line + 1;  // 0-based -> 1-based
            errorMsg = QString("라인 %1: %2").arg(errorLine).arg(QString::fromStdString(e.msg));
        }
        
        // 우측 파일 파싱 시도
        try {
            YAML::Load(rightText.toStdString());
        } catch (const YAML::Exception &e) {
            rightError = true;
            // 좌측에 에러가 없으면 우측 에러 정보 사용
            if (!leftError) {
                errorLine = e.mark.line + 1;
                errorMsg = QString("비교 파일 라인 %1: %2").arg(errorLine).arg(QString::fromStdString(e.msg));
            }
        }
        
        // 둘 중 하나라도 에러가 있으면 에러 행 추가
        if (leftError || rightError) {
            DiffRow errorRow;
            errorRow.line = errorLine;
            errorRow.leftLineNumber = leftError ? errorLine : -1;
            errorRow.rightLineNumber = rightError ? errorLine : -1;
            errorRow.key = errorLine > 0 ? QString::number(errorLine) : "Parse Error";
            errorRow.origin = rightError ? "형식을 수정해주세요" : "";
            errorRow.target = leftError ? "형식을 수정해주세요" : "";
            errorRow.state = "ERROR";
            diffRows.append(errorRow);
        }
        // 둘 다 정상이면 진짜 동일한 것이므로 에러 추가하지 않음
    }
    
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
            
        } else if (r.state == "ERROR") {
            // 주황색 - 에러 (YAML 형식 오류)
            QColor bgColor(255, 237, 213);   // 연한 주황색
            QColor textColor(194, 65, 12);   // 진한 주황색

            if (itemLine) itemLine->setBackground(bgColor);
            itemKey->setBackground(bgColor);
            itemLeft->setBackground(bgColor);
            itemRight->setBackground(bgColor);
            itemState->setBackground(bgColor);

            itemState->setForeground(QBrush(textColor));
            QFont boldFont = itemState->font();
            boldFont.setBold(true);
            itemState->setFont(boldFont);
            itemState->setText("⚠ 형식 오류");
            
            // Left 컬럼에 "수정 필요" 메시지 표시
            itemLeft->setText("형식을 수정해주세요");
            itemLeft->setForeground(QBrush(textColor));
            QFont italicFont = itemLeft->font();
            italicFont.setItalic(true);
            itemLeft->setFont(italicFont);
            
            // Right 컬럼에 상세 에러 메시지 표시
            if (!r.target.isEmpty()) {
                itemRight->setText("");
                itemRight->setForeground(QBrush(textColor));
            }
        } else if (r.state == "MISMATCH") {
            // 주황색 - 확장자 불일치(형식 오류와 구분)
            QColor bgColor(255, 237, 213);
            QColor textColor(194, 65, 12);

            if (itemLine) itemLine->setBackground(bgColor);
            itemKey->setBackground(bgColor);
            itemLeft->setBackground(bgColor);
            itemRight->setBackground(bgColor);
            itemState->setBackground(bgColor);

            itemState->setForeground(QBrush(textColor));
            QFont boldFont = itemState->font();
            boldFont.setBold(true);
            itemState->setFont(boldFont);
            itemState->setText("⚠ 확장자 불일치");
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
    
    // 좌측 편집기용 하이라이트 정보 (leftLineNumber 사용)
    QMap<int, QString> leftLineStates;
    for (const DiffRow &row : rows) {
        if (row.leftLineNumber > 0 && row.state != "SAME") {
            leftLineStates[row.leftLineNumber] = row.state;
        }
    }
    
    // 우측 편집기용 하이라이트 정보 (rightLineNumber 사용)
    QMap<int, QString> rightLineStates;
    for (const DiffRow &row : rows) {
        if (row.rightLineNumber > 0 && row.state != "SAME") {
            rightLineStates[row.rightLineNumber] = row.state;
        }
    }
    
    // 좌측 편집기에 하이라이트 설정
    if (leftDiffHighlighter_) {
        leftDiffHighlighter_->setLineStates(leftLineStates);
    }
    
    // 우측 편집기에 하이라이트 설정
    if (rightDiffHighlighter_) {
        rightDiffHighlighter_->setLineStates(rightLineStates);
    }
    
    // 우측 편집기에 DiffHighlighter 설정 (아직 설정되지 않은 경우)
    if (rightText_ && rightText_->getDiffHighlighter() != rightDiffHighlighter_) {
        rightText_->setDiffHighlighter(rightDiffHighlighter_);
    }
    
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
    
    // 파일 타입 감지
    QString leftFileType = detectFileType(leftPath);
    QString rightFileType = detectFileType(rightPath);

    if (!areFileTypesCompatible(leftFileType, rightFileType)) {
        QString leftSuffix = QFileInfo(leftPath).suffix();
        QString rightSuffix = QFileInfo(rightPath).suffix();

        if (leftSuffix.isEmpty()) {
            leftSuffix = leftFileType.toUpper();
        } else {
            leftSuffix = leftSuffix.toUpper();
        }

        if (rightSuffix.isEmpty()) {
            rightSuffix = rightFileType.toUpper();
        } else {
            rightSuffix = rightSuffix.toUpper();
        }

        QMessageBox::warning(this,
                             tr("파일 형식 불일치"),
                             tr("서로 다른 형식의 파일은 비교할 수 없습니다.\n왼쪽: %1\n오른쪽: %2")
                                 .arg(leftSuffix)
                                 .arg(rightSuffix));
        return;
    }

    QString fileType = rightFileType;
    QPair<QString, QString> columnHeaders = determineColumnHeaders(leftPath, rightPath);
    
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
        // YAML 파싱 에러인 경우, 자세한 에러 메시지 표시
        if (fileType == "yaml") {
            QString detailedError = QString::fromStdString(result.errorMessage);
            
            // 좌측 파일 파싱 시도
            try {
                YAML::Load(leftContent.toStdString());
            } catch (const YAML::Exception &e) {
                int errorLine = e.mark.line + 1;
                detailedError = QString("좌측 파일 라인 %1:\n%2")
                    .arg(errorLine)
                    .arg(QString::fromStdString(e.msg));
            }
            
            // 우측 파일 파싱 시도
            try {
                YAML::Load(rightContent.toStdString());
            } catch (const YAML::Exception &e) {
                int errorLine = e.mark.line + 1;
                detailedError += QString("\n\n우측 파일 라인 %1:\n%2")
                    .arg(errorLine)
                    .arg(QString::fromStdString(e.msg));
            }
            
            QMessageBox::critical(this, tr("YAML 형식 오류"), detailedError);
        } else {
            QMessageBox::critical(this, 
                                tr("Diff 오류"), 
                                QString::fromStdString(result.errorMessage));
        }
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
    
    // 테이블 컬럼 업데이트
    updateTableColumns(fileType, columnHeaders.first, columnHeaders.second);
    
    // 결과 파싱 및 표시 (core의 DiffService가 이미 모든 비교와 에러 처리를 수행함)
    QList<DiffRow> diffRows = parseDiffResult(result.data, fileType);
    
    // YAML 파싱 에러 체크 (빈 결과 + YAML 타입)
    // DiffYaml은 에러 발생 시 throw하지 않고 빈 벡터를 반환하므로
    // result.success는 true지만 결과가 비어있을 수 있음
    if (diffRows.isEmpty() && fileType == "yaml") {
        bool leftError = false;
        bool rightError = false;
        int errorLine = -1;
        QString errorMsg = "YAML 형식 오류";
        
        // 좌측 파일 파싱 시도
        try {
            YAML::Load(leftContent.toStdString());
        } catch (const YAML::Exception &e) {
            leftError = true;
            errorLine = e.mark.line + 1;  // 0-based -> 1-based
            errorMsg = QString("라인 %1: %2").arg(errorLine).arg(QString::fromStdString(e.msg));
        }
        
        // 우측 파일 파싱 시도
        try {
            YAML::Load(rightContent.toStdString());
        } catch (const YAML::Exception &e) {
            rightError = true;
            // 좌측에 에러가 없으면 우측 에러 정보 사용
            if (!leftError) {
                errorLine = e.mark.line + 1;
                errorMsg = QString("비교 파일 라인 %1: %2").arg(errorLine).arg(QString::fromStdString(e.msg));
            }
        }
        
        // 둘 중 하나라도 에러가 있으면 에러 행 추가
        if (leftError || rightError) {
            DiffRow errorRow;
            errorRow.line = errorLine;
            errorRow.leftLineNumber = leftError ? errorLine : -1;
            errorRow.rightLineNumber = rightError ? errorLine : -1;
            errorRow.key = errorLine > 0 ? QString::number(errorLine) : "Parse Error";
            errorRow.origin = rightError ? "형식을 수정해주세요" : "";
            errorRow.target = leftError ? "형식을 수정해주세요" : "";
            errorRow.state = "ERROR";
            diffRows.append(errorRow);
        }
        // 둘 다 정상이면 진짜 동일한 것이므로 에러 추가하지 않음
    }
    
    setDiffRows(diffRows);
    
    // 왼쪽 파일도 표시하고 싶다면 신호를 발생시킴
    emit uiCompareClicked(leftPath, rightPath);
}

void ComparePage::updateTableColumns(const QString &fileType,
                                     const QString &leftHeaderOverride,
                                     const QString &rightHeaderOverride)
{
    if (!diffTable_) return;
    
    auto resolveHeader = [](const QString &overrideText, const QString &fallback) {
        return overrideText.isEmpty() ? fallback : overrideText;
    };

    if (fileType == "yaml") {
        // YAML: Line, Path, Left Value, Right Value, State
        diffTable_->setColumnCount(5);
        QString leftHeader  = resolveHeader(leftHeaderOverride, tr("Left Value (Compare)"));
        QString rightHeader = resolveHeader(rightHeaderOverride, tr("Right Value (Base)"));
        diffTable_->setHorizontalHeaderLabels({"Line", "Path", leftHeader, rightHeader, "State"});
        diffTable_->setColumnWidth(0, 50);   // Line
        diffTable_->setColumnWidth(1, 120);  // Path
        diffTable_->setColumnWidth(2, 150);  // Left Value
        diffTable_->setColumnWidth(3, 150);  // Right Value
        diffTable_->setColumnWidth(4, 100);  // State
    } else if (fileType == "python" || fileType == "text") {
        // Python/Text: Line, Left Content, Right Content, State (순서 변경)
        diffTable_->setColumnCount(4);
        QString leftHeader  = resolveHeader(leftHeaderOverride, tr("Left Content (Compare)"));
        QString rightHeader = resolveHeader(rightHeaderOverride, tr("Right Content (Base)"));
        diffTable_->setHorizontalHeaderLabels({"Line", leftHeader, rightHeader, "State"});
        diffTable_->setColumnWidth(0, 50);   // Line (줄임)
        diffTable_->setColumnWidth(1, 150);  // Left Content (줄임)
        diffTable_->setColumnWidth(2, 150);  // Right Content (줄임)
        diffTable_->setColumnWidth(3, 100);  // State
    } else {
        // 기본값
        diffTable_->setColumnCount(4);
        QString leftHeader  = resolveHeader(leftHeaderOverride, tr("Left"));
        QString rightHeader = resolveHeader(rightHeaderOverride, tr("Right"));
        diffTable_->setHorizontalHeaderLabels({"Key", leftHeader, rightHeader, "State"});
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
                    
                    // 라인 번호 추출 (oldLineNumber = 우측/Base, newLineNumber = 좌측/Compare)
                    row.line = -1;
                    row.leftLineNumber = -1;
                    row.rightLineNumber = -1;
                    
                    if (change.isMember("newLineNumber") && !change["newLineNumber"].isNull()) {
                        row.leftLineNumber = change["newLineNumber"].asInt();
                        row.line = row.leftLineNumber;  // 호환성 유지
                    }
                    
                    if (change.isMember("oldLineNumber") && !change["oldLineNumber"].isNull()) {
                        row.rightLineNumber = change["oldLineNumber"].asInt();
                        if (row.line == -1) {
                            row.line = row.rightLineNumber;  // 호환성 유지
                        }
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
                    
                    // baseLineNumber (right) 와 compareLineNumber (left) 추출
                    row.line = -1;
                    row.leftLineNumber = -1;
                    row.rightLineNumber = -1;
                    
                    if (change.isMember("compareLineNumber") && !change["compareLineNumber"].isNull()) {
                        row.leftLineNumber = change["compareLineNumber"].asInt();
                        row.line = row.leftLineNumber;
                        row.key = QString::number(row.leftLineNumber);
                    }
                    
                    if (change.isMember("baseLineNumber") && !change["baseLineNumber"].isNull()) {
                        row.rightLineNumber = change["baseLineNumber"].asInt();
                        if (row.line == -1) {
                            row.line = row.rightLineNumber;
                            row.key = QString::number(row.rightLineNumber);
                        }
                    }
                    
                    if (row.line == -1) {
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
                    row.leftLineNumber = -1;
                    row.rightLineNumber = -1;
                    
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
                    
                    // type을 state로 변환하고, 타입에 따라 라인 번호 할당
                    if (change.isMember("type")) {
                        std::string type = change["type"].asString();
                        if (type == "added") {
                            row.state = "ADDED";
                            row.leftLineNumber = row.line;  // added는 좌측(새 파일)에 추가
                        } else if (type == "removed") {
                            row.state = "REMOVED";
                            row.rightLineNumber = row.line;  // removed는 우측(원본 파일)에서 삭제
                        } else if (type == "modified") {
                            row.state = "CHANGED";
                            row.leftLineNumber = row.line;   // modified는 양쪽 모두 표시
                            row.rightLineNumber = row.line;
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
        errorRow.line = -1;
        errorRow.leftLineNumber = -1;
        errorRow.rightLineNumber = -1;
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

// ========================================
// 폴더 비교 관련 함수 구현
// ========================================

void ComparePage::performFolderDiff(const QString &leftPath, const QString &rightPath)
{
    QString finalLeftPath = leftPath;
    QString finalRightPath = rightPath;
    
    // 압축 파일 여부 확인 및 자동 해제
    QFileInfo leftInfo(leftPath);
    QFileInfo rightInfo(rightPath);
    
    bool leftIsArchive = services::WorkspaceService::isSupportedArchive(leftPath.toStdString());
    bool rightIsArchive = services::WorkspaceService::isSupportedArchive(rightPath.toStdString());
    
    // 좌측 압축 파일 해제
    if (leftIsArchive) {
        qDebug() << "[ComparePage] Left is archive, extracting...";
        finalLeftPath = extractArchiveToTemp(leftPath);
        if (finalLeftPath.isEmpty()) {
            QMessageBox::critical(this, tr("오류"), 
                tr("좌측 압축 파일 해제 실패:\n%1").arg(leftPath));
            return;
        }
    }
    
    // 우측 압축 파일 해제
    if (rightIsArchive) {
        qDebug() << "[ComparePage] Right is archive, extracting...";
        finalRightPath = extractArchiveToTemp(rightPath);
        if (finalRightPath.isEmpty()) {
            // 좌측 임시 폴더 정리
            if (leftIsArchive && !finalLeftPath.isEmpty()) {
                QDir(finalLeftPath).removeRecursively();
                tempFolders_.removeOne(finalLeftPath);
            }
            
            QMessageBox::critical(this, tr("오류"), 
                tr("우측 압축 파일 해제 실패:\n%1").arg(rightPath));
            return;
        }
    }
    
    // 폴더 비교 수행 (DiffService::diffDirectories)
    qDebug() << "[ComparePage] Comparing directories:" << finalLeftPath << "vs" << finalRightPath;
    
    services::ServiceResult result = services::DiffService::diffDirectories(
        finalRightPath.toStdString(),  // base (right)
        finalLeftPath.toStdString()    // compare (left)
    );
    
    if (!result.success) {
        QMessageBox::critical(this, tr("폴더 비교 오류"), 
            QString::fromStdString(result.errorMessage));
        return;
    }
    
    // 결과 표시
    isFolderMode_ = true;
    displayFolderDiffResult(result.data);
}

QString ComparePage::extractArchiveToTemp(const QString &archivePath)
{
    // 임시 디렉토리 생성
    QString tempBase = QDir::tempPath() + "/RoboEditor_Compare/";
    QDir().mkpath(tempBase);
    
    // UUID 기반 고유 ID 생성
    QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // 메타데이터 생성
    services::WorkspaceMetadata metadata;
    metadata.id = uniqueId.toStdString();
    metadata.name = QFileInfo(archivePath).baseName().toStdString();
    metadata.description = "Temporary extraction for comparison";
    metadata.target = "";
    
    // WorkspaceService::importWorkspace 호출
    services::ServiceResult result = services::WorkspaceService::importWorkspace(
        archivePath.toStdString(),
        tempBase.toStdString(),
        metadata
    );
    
    if (!result.success) {
        qWarning() << "[ComparePage] Archive extraction failed:" 
                   << QString::fromStdString(result.errorMessage);
        return QString();
    }
    
    QString extractedPath = tempBase + uniqueId;
    tempFolders_.append(extractedPath);
    
    qDebug() << "[ComparePage] Archive extracted to:" << extractedPath;
    return extractedPath;
}

void ComparePage::cleanupTempFolders()
{
    for (const QString &folder : tempFolders_) {
        if (QDir(folder).exists()) {
            qDebug() << "[ComparePage] Cleaning up temp folder:" << folder;
            QDir(folder).removeRecursively();
        }
    }
    tempFolders_.clear();
}

QColor ComparePage::getColorForDiffState(const QString &state) const
{
    if (state == "added") {
        return QColor(220, 252, 231);  // 연한 초록색
    } else if (state == "removed") {
        return QColor(254, 226, 226);  // 연한 빨강색
    } else if (state == "modified") {
        return QColor(255, 250, 205);  // 연한 노란색
    }
    return QColor(255, 255, 255);  // 흰색 (기본)
}

QString ComparePage::detectFileType(const QString &path) const
{
    QString ext = QFileInfo(path).suffix().toLower();

    static const QSet<QString> yamlExts   = {"yaml", "yml", "pts"};
    static const QSet<QString> pythonExts = {"py", "srl", "sbp"};

    if (yamlExts.contains(ext)) {
        return "yaml";
    }

    if (pythonExts.contains(ext)) {
        return "python";
    }

    return "text";
}

bool ComparePage::areFileTypesCompatible(const QString &leftType, const QString &rightType) const
{
    if (leftType == rightType) {
        return true;
    }

    if (leftType == "text" && rightType == "text") {
        return true;
    }

    return false;
}

ComparePage::ControllerPathInfo ComparePage::extractControllerInfo(const QString &path) const
{
    ControllerPathInfo info;
    if (path.isEmpty())
        return info;

    QString normalized = QDir::fromNativeSeparators(QDir::cleanPath(path));
    QStringList parts = normalized.split('/', Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return info;

    int backupIndex = -1;
    for (int i = 0; i < parts.size(); ++i) {
        if (parts[i].compare("backup", Qt::CaseInsensitive) == 0) {
            backupIndex = i;
            break;
        }
    }

    if (backupIndex < 0)
        return info;

    int serialIndex = backupIndex + 1;
    if (serialIndex >= parts.size())
        return info;

    QString serialCandidate = parts[serialIndex];
    if (serialCandidate.compare("temp", Qt::CaseInsensitive) == 0) {
        serialIndex++;
        if (serialIndex >= parts.size())
            return info;
        serialCandidate = parts[serialIndex];
    }

    info.serial = serialCandidate;

    int detailIndex = serialIndex + 1;
    if (detailIndex >= 0 && detailIndex < parts.size() - 1) {
        info.detail = parts[detailIndex];
    }

    return info;
}

QPair<QString, QString> ComparePage::determineColumnHeaders(const QString &leftPath,
                                                           const QString &rightPath) const
{
    ControllerPathInfo leftInfo  = extractControllerInfo(leftPath);
    ControllerPathInfo rightInfo = extractControllerInfo(rightPath);

    QString leftHeader;
    QString rightHeader;

    bool leftHasSerial  = !leftInfo.serial.isEmpty();
    bool rightHasSerial = !rightInfo.serial.isEmpty();
    bool sameController = leftHasSerial && rightHasSerial &&
                          leftInfo.serial.compare(rightInfo.serial, Qt::CaseInsensitive) == 0;

    if (sameController) {
        if (!leftInfo.detail.isEmpty())
            leftHeader = leftInfo.detail;
        else if (leftHasSerial)
            leftHeader = leftInfo.serial;

        if (!rightInfo.detail.isEmpty())
            rightHeader = rightInfo.detail;
        else if (rightHasSerial)
            rightHeader = rightInfo.serial;
    } else {
        if (leftHasSerial)
            leftHeader = leftInfo.serial;
        if (rightHasSerial)
            rightHeader = rightInfo.serial;
    }

    if (leftHeader.isEmpty() && !leftPath.isEmpty()) {
        leftHeader = QFileInfo(leftPath).fileName();
    }

    if (rightHeader.isEmpty() && !rightPath.isEmpty()) {
        rightHeader = QFileInfo(rightPath).fileName();
    }

    return {leftHeader, rightHeader};
}

void ComparePage::displayFolderDiffResult(const Json::Value &result)
{
    // 테이블 숨기고 트리 위젯 생성/표시
    if (diffTable_) {
        diffTable_->hide();
    }
    
    if (!folderDiffTree_) {
        folderDiffTree_ = new QTreeWidget(diffPanel_);
        folderDiffTree_->setHeaderLabels({tr("Path"), tr("Status"), tr("Type")});
        folderDiffTree_->setAlternatingRowColors(true);
        folderDiffTree_->setStyleSheet("QTreeWidget { "
                                        "  background-color: white; "
                                        "  border: none; "
                                        "  font-family: 'Consolas', 'Monaco', 'Courier New', monospace; "
                                        "  font-size: 9pt; "
                                        "}");
        
        // diffPanel_의 레이아웃에 추가
        diffPanel_->layout()->addWidget(folderDiffTree_);
    }
    
    folderDiffTree_->clear();
    folderDiffTree_->show();
    
    // statistics 파싱
    const Json::Value &stats = result["statistics"];
    int added = stats["added"].asInt();
    int removed = stats["removed"].asInt();
    int modified = stats["modified"].asInt();
    int total = stats["totalChanges"].asInt();
    
    // 통계 업데이트
    if (statLabel_) {
        statLabel_->setText(QString("Changes: %1 (+%2 -%3 ~%4)")
                            .arg(total).arg(added).arg(removed).arg(modified));
    }
    
    // 통계 헤더 아이템
    QTreeWidgetItem *statsItem = new QTreeWidgetItem(folderDiffTree_);
    statsItem->setText(0, QString("📊 Total Changes: %1").arg(total));
    statsItem->setText(1, QString("+%1 -%2 ~%3").arg(added).arg(removed).arg(modified));
    statsItem->setBackground(0, QColor(240, 240, 240));
    statsItem->setBackground(1, QColor(240, 240, 240));
    statsItem->setBackground(2, QColor(240, 240, 240));
    QFont boldFont = statsItem->font(0);
    boldFont.setBold(true);
    statsItem->setFont(0, boldFont);
    
    // changes 파싱
    const Json::Value &changes = result["changes"];
    
    // 폴더별로 그룹화
    QMap<QString, QList<Json::Value>> folderGroups;
    
    for (const auto &change : changes) {
        QString path = QString::fromStdString(change["path"].asString());
        QString folder = QFileInfo(path).dir().path();
        
        if (folder == ".") {
            folder = "(Root)";
        }
        
        folderGroups[folder].append(change);
    }
    
    // 폴더별로 트리 아이템 생성
    QStringList folderKeys = folderGroups.keys();
    folderKeys.sort();
    
    for (const QString &folderPath : folderKeys) {
        QList<Json::Value> files = folderGroups[folderPath];
        
        // 폴더 아이템
        QTreeWidgetItem *folderItem = new QTreeWidgetItem(folderDiffTree_);
        folderItem->setText(0, QString("📁 %1").arg(folderPath));
        folderItem->setText(1, QString("%1 file(s)").arg(files.size()));
        folderItem->setExpanded(true);
        
        QFont folderFont = folderItem->font(0);
        folderFont.setBold(true);
        folderItem->setFont(0, folderFont);
        
        // 파일 아이템들
        for (const auto &change : files) {
            QString path = QString::fromStdString(change["path"].asString());
            QString type = QString::fromStdString(change["type"].asString());
            QString fileName = QFileInfo(path).fileName();
            
            QTreeWidgetItem *fileItem = new QTreeWidgetItem(folderItem);
            fileItem->setText(0, fileName);
            fileItem->setData(0, Qt::UserRole, path);  // 전체 경로 저장
            
            // 상태별 색상 및 텍스트
            QColor bgColor = getColorForDiffState(type);
            
            if (type == "added") {
                fileItem->setText(1, tr("Added"));
                fileItem->setText(2, "+ ADDED");
                fileItem->setForeground(2, QBrush(QColor(22, 163, 74)));  // 진한 초록
            } else if (type == "removed") {
                fileItem->setText(1, tr("Removed"));
                fileItem->setText(2, "− REMOVED");
                fileItem->setForeground(2, QBrush(QColor(220, 38, 38)));  // 진한 빨강
            } else if (type == "modified") {
                fileItem->setText(1, tr("Modified"));
                fileItem->setText(2, "● CHANGED");
                fileItem->setForeground(2, QBrush(QColor(184, 134, 11)));  // 어두운 황금색
            }
            
            fileItem->setBackground(0, bgColor);
            fileItem->setBackground(1, bgColor);
            fileItem->setBackground(2, bgColor);
        }
    }
    
    // 컬럼 너비 조정
    folderDiffTree_->setColumnWidth(0, 400);
    folderDiffTree_->setColumnWidth(1, 150);
    folderDiffTree_->setColumnWidth(2, 150);
    
    qDebug() << "[ComparePage] Folder diff result displayed:" << total << "changes";
}

