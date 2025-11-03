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

ComparePage::ComparePage(QWidget *parent) : QWidget(parent)
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

    // 헤더 + [X] - 탭 스타일로 디자인
    auto header = new QWidget(w);
    header->setMaximumHeight(28);
    header->setStyleSheet("QWidget { "
                          "  background-color: #f0f0f0; "
                          "  border: 1px solid #ccc; "
                          "  border-bottom: none; "
                          "  border-top-left-radius: 3px; "
                          "  border-top-right-radius: 3px; "
                          "}");
    auto h = new QHBoxLayout(header);
    h->setContentsMargins(8, 4, 8, 4);
    h->setSpacing(4);

    auto  title = new QLabel(tr("Compare"), header);
    QFont font  = title->font();
    font.setPointSize(9);
    title->setFont(font);

    auto btnClose = new QToolButton(header);
    btnClose->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    btnClose->setAutoRaise(true);
    btnClose->setMaximumSize(18, 18);

    h->addWidget(title);
    h->addStretch();
    h->addWidget(btnClose);
    connect(btnClose, &QToolButton::clicked, this, &ComparePage::onCloseClicked);
    v->addWidget(header);

    // 내부 스플리터: (좌) 비교대상 텍스트(읽기 전용) | (우) diff 패널
    rightSplit_ = new QSplitter(Qt::Horizontal, w);

    // 좌측: 비교 대상 텍스트 - 흰색 배경, 테두리
    rightText_ = new DropTextEdit(rightSplit_);
    rightText_->setReadOnly(true);
    rightText_->setStyleSheet(
            "QPlainTextEdit { background-color: white; border: 1px solid #ccc; }");

    // 우측: diff 패널
    diffPanel_ = buildDiffPanel();

    rightSplit_->addWidget(rightText_);
    rightSplit_->addWidget(diffPanel_);
    rightSplit_->setStretchFactor(0, 1);
    rightSplit_->setStretchFactor(1, 1);

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
                              "  border: 1px solid #ccc; "
                              "  border-top: none; "
                              "}");

    auto v = new QVBoxLayout(diffPanel_);
    v->setContentsMargins(8, 8, 8, 8);

    // 상단: 파일 선택 + 필터 버튼
    auto topBar = new QWidget(diffPanel_);
    topBar->setStyleSheet("QWidget { background-color: transparent; border: none; }");
    auto topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 4);
    topLayout->setSpacing(6);

    // 파일 선택 버튼
    auto btnSelectFile = new QPushButton(tr("Select File"), topBar);
    btnSelectFile->setFixedSize(90, 26);
    topLayout->addWidget(btnSelectFile);

    connect(btnSelectFile, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Select file to compare"));
        if (!path.isEmpty()) {
            setTargetPath(path);
            emit targetPathChanged(path);
        }
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
    connect(btnAll, &QPushButton::clicked, this, [=]() {
        btnAll->setChecked(true);
        btnAdded->setChecked(false);
        btnRemoved->setChecked(false);
        btnChanged->setChecked(false);
        // TODO: 필터 적용
    });
    connect(btnAdded, &QPushButton::clicked, this, [=]() {
        btnAll->setChecked(false);
        btnAdded->setChecked(true);
        btnRemoved->setChecked(false);
        btnChanged->setChecked(false);
        // TODO: ADDED만 필터
    });
    connect(btnRemoved, &QPushButton::clicked, this, [=]() {
        btnAll->setChecked(false);
        btnAdded->setChecked(false);
        btnRemoved->setChecked(true);
        btnChanged->setChecked(false);
        // TODO: REMOVED만 필터
    });
    connect(btnChanged, &QPushButton::clicked, this, [=]() {
        btnAll->setChecked(false);
        btnAdded->setChecked(false);
        btnRemoved->setChecked(false);
        btnChanged->setChecked(true);
        // TODO: CHANGED만 필터
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
    hHeader->setStretchLastSection(true);                     // 마지막 컬럼 늘림
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
    vHeader->setDefaultSectionSize(28);  // 행 높이 통일
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
                              "  border: 1px solid #d0d0d0; "
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

    // 초기 컬럼 너비 설정
    diffTable_->setColumnWidth(0, 200);  // Key
    diffTable_->setColumnWidth(1, 200);  // Left
    diffTable_->setColumnWidth(2, 200);  // Right
    // State는 자동으로 늘어남 (stretchLastSection)

    v->addWidget(diffTable_);

    return diffPanel_;
}

void ComparePage::onCloseClicked()
{
    emit closed();
}

void ComparePage::setTargetPath(const QString &path)
{
    targetPath_ = path;
    loadRightText(targetPath_);
    emit targetPathChanged(path);
}

void ComparePage::recalcDiff(const QString &leftText)
{
    const QString rightText = rightText_ ? rightText_->toPlainText() : QString();

    // (임시) 빈 diff라도 테이블이 null이 아니도록 보장
    if (!diffTable_)
        buildDiffPanel();  // 방어적
    if (!diffTable_)
        return;

    // TODO: 이후 실제 diff 로직으로 교체
    QList<DiffRow> rows;
    if (!leftText.isEmpty() || !rightText.isEmpty()) {
        rows.push_back({"(preview)",
                        QString::number(leftText.size()),
                        QString::number(rightText.size()),
                        leftText == rightText ? "SAME" : "CHANGED"});
    }
    setDiffRows(rows);
}

// 내부 상태 업데이트용
void ComparePage::setRoots(const QString &leftRoot, const QString &rightRoot)
{
    currentLeftRoot_  = leftRoot;
    currentRightRoot_ = rightRoot;
    // fileInfoLabel_ 제거됨
}

// 좌/우 본문 텍스트 채우기
void ComparePage::setFileContents(const QString &leftText, const QString &rightText)
{
    // leftText는 이제 ModifyPage가 가진다. 여기서는 우측만 갱신.
    Q_UNUSED(leftText);
    if (rightText_)
        rightText_->setPlainText(rightText);
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

        auto *itemKey   = new QTableWidgetItem(r.key);
        auto *itemLeft  = new QTableWidgetItem(r.origin);
        auto *itemRight = new QTableWidgetItem(r.target);
        auto *itemState = new QTableWidgetItem(r.state);

        // State에 따라 명확한 색상과 아이콘 적용
        if (r.state == "CHANGED") {
            // 노란색 - 변경됨
            QColor bgColor(255, 250, 205);   // 레몬 크림색
            QColor textColor(184, 134, 11);  // 어두운 황금색

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

            itemKey->setBackground(bgColor);
            itemLeft->setBackground(bgColor);
            itemRight->setBackground(bgColor);
            itemState->setBackground(bgColor);

            itemState->setForeground(QBrush(textColor));
            itemState->setText("= SAME");
        }

        // 텍스트 정렬
        itemKey->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        itemLeft->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        itemRight->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        itemState->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);

        diffTable_->setItem(i, 0, itemKey);
        diffTable_->setItem(i, 1, itemLeft);
        diffTable_->setItem(i, 2, itemRight);
        diffTable_->setItem(i, 3, itemState);
    }

    // 통계 업데이트
    QString stats = QString("Changes: %1").arg(changeCount);
    if (addedCount > 0 || removedCount > 0 || changedCount > 0) {
        stats += QString(" (+%1 -%2 ~%3)").arg(addedCount).arg(removedCount).arg(changedCount);
    }
    statLabel_->setText(stats);
}

void ComparePage::setDiffRows(const QList<DiffRow> &rows)
{
    refreshDiffTable(rows);
}

// ------------------------
// "API 자리 더미"
// ------------------------

// 더미 API: GET api/v1/folder?path=
void ComparePage::fetchFolderFromApiDummy(const QString &side, const QString &basePathHint)
{
    QString path =
            basePathHint.isEmpty() ? (side == "left" ? "/robot/A" : "/robot/B") : basePathHint;

    QStringList dummyFolders = {"cfg", "prog", "logs"};
    QStringList dummyFiles;

    if (side == "left") {
        dummyFiles = {"main.yaml", "task.srl", "io_map.yaml"};
    } else {
        dummyFiles = {"main.yaml", "task.srl", "io_map_v2.yaml"};
    }

    updateFolderListing(side, path, dummyFolders, dummyFiles);
}

// 더미 API: GET api/v1/file?path=... 2번 호출 + diff 결과
void ComparePage::fetchCompareFromApiDummy(const QString &leftFilePath,
                                           const QString &rightFilePath)
{
    Q_UNUSED(leftFilePath)
    Q_UNUSED(rightFilePath)

    QString leftContent = "paramA=10\n"
                          "paramB=20\n"
                          "paramC=OFF\n";

    QString rightContent = "paramA=10\n"
                           "paramB=22\n"
                           "paramC=OFF\n"
                           "paramD=EXTRA\n";

    setFileContents(leftContent, rightContent);

    QList<DiffRow> rows;
    rows.push_back({"paramA", "10", "10", ""});
    rows.push_back({"paramB", "20", "22", "CHANGED"});
    rows.push_back({"paramC", "OFF", "OFF", ""});
    rows.push_back({"paramD", "", "EXTRA", "ADDED"});

    setDiffRows(rows);
}

// ------------------------
// 버튼 핸들러
// ------------------------

void ComparePage::onCompareClicked()
{
    // QLineEdit에서 파일명 읽기
    const QString leftFileName  = leftFileSelect_ ? leftFileSelect_->text().trimmed() : QString();
    const QString rightFileName = rightFileSelect_ ? rightFileSelect_->text().trimmed() : QString();

    if (leftFileName.isEmpty() || rightFileName.isEmpty()) {
        qWarning() << "[Compare] file name is empty" << leftFileName << rightFileName;
        return;
    }
    if (currentLeftRoot_.isEmpty() || currentRightRoot_.isEmpty()) {
        qWarning() << "[Compare] root path not set" << currentLeftRoot_ << currentRightRoot_;
        return;
    }

    // full path 조립
    const QString leftFullPath  = QDir(currentLeftRoot_).filePath(leftFileName);
    const QString rightFullPath = QDir(currentRightRoot_).filePath(rightFileName);

    // 로그/신호
    emit uiCompareClicked(leftFullPath, rightFullPath);

    // 좌/우 에디터 로딩
    loadFileIntoEditor(leftFullPath, /*isLeft=*/true);
    loadFileIntoEditor(rightFullPath, /*isLeft=*/false);

    // 임시 diff
    fetchCompareFromApiDummy(leftFullPath, rightFullPath);
}

// 실제 로컬 디렉토리를 스캔해서 현재 콤보박스 갱신용으로 넘겨주는 헬퍼
void ComparePage::loadLocalFolder(const QString &side, const QString &path)
{
    // path 안의 파일 목록 가져오기
    QDir dir(path);

    // 안전장치: 폴더가 유효하지 않으면 그냥 리턴
    if (!dir.exists()) {
        qWarning() << "[ComparePage] Directory does not exist:" << path;
        return;
    }

    // 폴더 내부의 "파일들"만 뽑자 (지금은 1차 버전: 재귀 안 하고 바로 아래 것만)
    // 추후에는 YAML, SRL 등 필터링 가능
    QStringList   fileNames;
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    for (const QFileInfo &info : entries) {
        fileNames << info.fileName();
    }

    // 서브폴더 목록도 원하면 뽑을 수 있어 (지금은 UI 왼쪽 탭쪽에 붙일 예정)
    QStringList   folderNames;
    QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : subdirs) {
        folderNames << info.fileName();
    }

    // 기존 updateFolderListing() 재사용해서
    // - currentLeftRoot_ / currentRightRoot_ 갱신
    // - leftFileSelect_ / rightFileSelect_ 갱신
    updateFolderListing(side, path, folderNames, fileNames);

    refreshTreeView(side, path);
}

void ComparePage::loadRightText(const QString &path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream   in(&f);
        const QString content = in.readAll();
        f.close();
        if (rightText_)
            rightText_->setPlainText(content);
    } else {
        if (rightText_)
            rightText_->setPlainText(tr("Failed to open: %1").arg(path));
    }
}

void ComparePage::loadFileIntoEditor(const QString &fullPath, bool isLeft)
{
    QFile f(fullPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream   ts(&f);
    const QString content = ts.readAll();
    f.close();

    if (isLeft) {
        // 좌측은 ModifyPage 영역. 필요하면 시그널로 위임하도록 변경 가능.
        // 현재는 무시.
        return;
    } else {
        if (rightText_)
            rightText_->setPlainText(content);
    }
}
// ----- 선언만 있던 슬롯/헬퍼의 최소 구현 -----
void ComparePage::onOpenLeftFolderClicked()
{ /* TODO: hook later */
}
void ComparePage::onOpenRightFolderClicked()
{ /* TODO: hook later */
}
void ComparePage::onLeftTreeDoubleClicked(const QModelIndex &)
{ /* TODO */
}
void ComparePage::onRightTreeDoubleClicked(const QModelIndex &)
{ /* TODO */
}
void ComparePage::refreshTreeView(const QString &side, const QString &path)
{
    Q_UNUSED(side);
    Q_UNUSED(path);
}

void ComparePage::updateFolderListing(const QString &side,
                                      const QString &basePath,
                                      const QStringList & /*folders*/,
                                      const QStringList & /*files*/)
{
    if (side == "left")
        currentLeftRoot_ = basePath;
    else
        currentRightRoot_ = basePath;
    // fileInfoLabel_ 제거됨
}
