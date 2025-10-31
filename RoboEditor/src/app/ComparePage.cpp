#include "ComparePage.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTreeView>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#include "DropTextEdit.h"

ComparePage::ComparePage(QWidget *parent) : QWidget(parent)
{
    currentLeftRoot_  = "/robot/A";
    currentRightRoot_ = "/robot/B";

    // 상단 바
    viewModeCombo_ = new QComboBox(this);
    viewModeCombo_->addItems({"Text", "YAML Key", "JSON Key"});

    openLeftBtn_    = new QPushButton("Left Folder", this);
    leftFileSelect_ = new QComboBox(this);

    openRightBtn_    = new QPushButton("Right Folder", this);
    rightFileSelect_ = new QComboBox(this);

    compareBtn_ = new QPushButton("Compare", this);

    fileInfoLabel_ = new QLabel(
            QString("Left: %1 | Right: %2").arg(currentLeftRoot_, currentRightRoot_), this);

    auto *topBar = new QHBoxLayout;
    topBar->addWidget(viewModeCombo_);
    topBar->addWidget(openLeftBtn_);
    topBar->addWidget(leftFileSelect_);
    topBar->addWidget(openRightBtn_);
    topBar->addWidget(rightFileSelect_);
    topBar->addWidget(compareBtn_);
    topBar->addStretch();
    topBar->addWidget(fileInfoLabel_);

    // 좌측 탭 (dirTabs_)
    dirTabs_ = new QTabWidget(this);

    leftText_  = new DropTextEdit(this);
    rightText_ = new DropTextEdit(this);

    // 왼쪽 탭 페이지
    leftTabPage_  = new QWidget(this);
    rightTabPage_ = new QWidget(this);

    // QFileSystemModel 준비
    fsModelLeft_  = new QFileSystemModel(this);
    fsModelRight_ = new QFileSystemModel(this);

    // 모델 옵션 (숨겨야 하는 항목 최소화: . 등의 시스템거 제외)
    fsModelLeft_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    fsModelRight_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    // 아직 루트 경로 확정 전이라 빈 세팅
    fsModelLeft_->setRootPath("");
    fsModelRight_->setRootPath("");

    // 트리뷰 생성
    fsTreeLeft_  = new QTreeView(this);
    fsTreeRight_ = new QTreeView(this);

    fsTreeLeft_->setModel(fsModelLeft_);
    fsTreeRight_->setModel(fsModelRight_);

    // 헤더 약간 정리 (사이즈 자동, 수평 스크롤 등)
    fsTreeLeft_->setUniformRowHeights(true);
    fsTreeLeft_->setHeaderHidden(false);  // 필요하면 true로 바꿔도 됨
    fsTreeLeft_->setAnimated(true);
    fsTreeLeft_->setIndentation(16);
    fsTreeLeft_->setExpandsOnDoubleClick(true);
    fsTreeLeft_->setDragEnabled(true);
    fsTreeLeft_->setSelectionMode(QAbstractItemView::SingleSelection);
    fsTreeLeft_->setDragDropMode(QAbstractItemView::DragOnly);

    fsTreeRight_->setUniformRowHeights(true);
    fsTreeRight_->setHeaderHidden(false);
    fsTreeRight_->setAnimated(true);
    fsTreeRight_->setIndentation(16);
    fsTreeRight_->setExpandsOnDoubleClick(true);
    fsTreeRight_->setDragEnabled(true);
    fsTreeRight_->setSelectionMode(QAbstractItemView::SingleSelection);
    fsTreeRight_->setDragDropMode(QAbstractItemView::DragOnly);

    leftText_->setAcceptDrops(true);
    rightText_->setAcceptDrops(true);

    leftText_->setAcceptAsLeft(true);
    leftText_->setAcceptAsRight(false);

    rightText_->setAcceptAsLeft(false);
    rightText_->setAcceptAsRight(true);

    // 왼쪽 탭 layout : 제어기
    {
        auto *v = new QVBoxLayout;
        v->setContentsMargins(0, 0, 0, 0);
        v->addWidget(new QLabel("LEFT folder browser", this));
        v->addWidget(fsTreeLeft_, 1);
        leftTabPage_->setLayout(v);
    }

    // 오른쪽 탭 layout : 워크스페이스
    {
        auto *v = new QVBoxLayout;
        v->setContentsMargins(0, 0, 0, 0);
        v->addWidget(new QLabel("RIGHT folder browser", this));
        v->addWidget(fsTreeRight_, 1);
        rightTabPage_->setLayout(v);
    }

    dirTabs_ = new QTabWidget(this);
    dirTabs_->addTab(leftTabPage_, "Left");
    dirTabs_->addTab(rightTabPage_, "Right");

    // 1. 가운데(좌/우 텍스트)용 내부 splitter
    QSplitter *textSplit = new QSplitter(Qt::Horizontal, this);

    // 왼쪽 텍스트 영역
    QWidget *leftTextWrapper = new QWidget(this);
    {
        auto *v = new QVBoxLayout;
        v->setContentsMargins(0, 0, 0, 0);
        v->addWidget(leftText_);
        leftTextWrapper->setLayout(v);
    }

    // 오른쪽 텍스트 영역
    QWidget *rightTextWrapper = new QWidget(this);
    {
        auto *v = new QVBoxLayout;
        v->setContentsMargins(0, 0, 0, 0);
        v->addWidget(rightText_);
        rightTextWrapper->setLayout(v);
    }

    // splitter에 양쪽 텍스트 추가
    textSplit->addWidget(leftTextWrapper);
    textSplit->addWidget(rightTextWrapper);

    // 초깃값 비율: 왼쪽:오른쪽 비율을 결정
    textSplit->setStretchFactor(0, 1);
    textSplit->setStretchFactor(1, 1);

    // center 쪽 전체를 감쌀 QWidget
    QWidget *centerPanel = new QWidget(this);
    {
        auto *v = new QVBoxLayout;
        v->setContentsMargins(0, 0, 0, 0);
        // 원래 centerLayout에 들어가던 애들(지금은 사실 textSplit밖에 없었지?)
        v->addWidget(textSplit);
        centerPanel->setLayout(v);
    }

    // diff 영역
    keySearchEdit_  = new QLineEdit(this);
    chkOnlyChanged_ = new QCheckBox("Only Changed", this);
    chkHideSame_    = new QCheckBox("Hide Same", this);
    chkOnlyAddDel_  = new QCheckBox("Only Add/Del", this);
    statLabel_      = new QLabel("Diff: 0 changes", this);

    diffTable_ = new QTableWidget(this);
    diffTable_->setColumnCount(4);
    diffTable_->setHorizontalHeaderLabels({"Line", "Left", "Right", "State"});
    diffTable_->horizontalHeader()->setStretchLastSection(false);
    diffTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    diffTable_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    diffTable_->setMinimumWidth(50);
    diffTable_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    diffTable_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);

    diffTable_->setColumnWidth(0, 50);   // Key/Line
    diffTable_->setColumnWidth(1, 100);  // Left
    diffTable_->setColumnWidth(2, 100);  // Right
    diffTable_->setColumnWidth(3, 70);   // State

    diffTable_->setSelectionMode(QAbstractItemView::NoSelection);
    diffTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *diffCtrlLayout = new QGridLayout;
    diffCtrlLayout->setContentsMargins(0, 0, 0, 0);
    diffCtrlLayout->setHorizontalSpacing(6);
    diffCtrlLayout->setVerticalSpacing(3);

    diffCtrlLayout->addWidget(keySearchEdit_, 0, 0, 1, 1);
    diffCtrlLayout->addWidget(chkOnlyChanged_, 0, 1, 1, 1);
    diffCtrlLayout->addWidget(chkHideSame_, 0, 2, 1, 1);
    diffCtrlLayout->addWidget(chkOnlyAddDel_, 0, 3, 1, 1);

    diffCtrlLayout->addWidget(statLabel_, 1, 0, 1, 4);

    // 왼쪽 폴더 탭 영역 래퍼
    QWidget *leftPanel = new QWidget(this);
    {
        auto *v = new QVBoxLayout;
        v->setContentsMargins(0, 0, 0, 0);
        v->addWidget(dirTabs_);
        leftPanel->setLayout(v);
    }

    // 오른쪽 diff 영역 래퍼
    QWidget *rightPanel = new QWidget(this);
    {
        auto *v = new QVBoxLayout;
        v->setContentsMargins(0, 0, 0, 0);
        v->addLayout(diffCtrlLayout);  // 검색/필터 줄
        v->addWidget(diffTable_, 1);   // diff 테이블 본체
        rightPanel->setLayout(v);
    }

    rightPanel->setMinimumWidth(80);

    rightPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    QSplitter *mainSplit = new QSplitter(Qt::Horizontal, this);

    // 왼쪽 패널(폴더 탭)
    mainSplit->addWidget(leftPanel);

    // 가운데 패널(텍스트 비교 splitter 포함)
    mainSplit->addWidget(centerPanel);

    // 오른쪽 패널(diff 테이블)
    mainSplit->addWidget(rightPanel);

    // 각 영역 초기 비율 설정 (왼:중:오 비슷하게)
    mainSplit->setStretchFactor(0, 0);  // 폴더 브라우저는 기본적으로 작게
    mainSplit->setStretchFactor(1, 2);  // 가운데는 크게
    mainSplit->setStretchFactor(2, 1);  // diff는 중간

    // 최종 레이아웃
    auto *rootLayout = new QVBoxLayout;
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addLayout(topBar);        // 위쪽 컨트롤 바는 그대로 레이아웃
    rootLayout->addWidget(mainSplit, 1);  // 아래쪽은 이제 splitter 하나
    setLayout(rootLayout);

    // 초기 파일 목록(더미)
    leftFiles_  = {"main.yaml", "task.srl", "io_map.yaml"};
    rightFiles_ = {"main.yaml", "task.srl", "io_map_v2.yaml"};
    leftFileSelect_->addItems(leftFiles_);
    rightFileSelect_->addItems(rightFiles_);

    connect(openLeftBtn_, &QPushButton::clicked, this, &ComparePage::onOpenLeftFolderClicked);
    connect(openRightBtn_, &QPushButton::clicked, this, &ComparePage::onOpenRightFolderClicked);
    connect(compareBtn_, &QPushButton::clicked, this, &ComparePage::onCompareClicked);
    if (fsTreeLeft_) {
        connect(fsTreeLeft_,
                &QTreeView::doubleClicked,
                this,
                &ComparePage::onLeftTreeDoubleClicked);
        connect(leftText_, &DropTextEdit::fileDroppedToLeft, this, [this](const QString &path) {
            loadFileIntoEditor(path, /*isLeft=*/true);

            QFileInfo info(path);
            int       found = leftFileSelect_->findText(info.fileName());
            if (found == -1) {
                leftFileSelect_->addItem(info.fileName());
                found = leftFileSelect_->findText(info.fileName());
            }
            leftFileSelect_->setCurrentIndex(found);

            // 현재 루트 갱신까지 하고 싶으면:
            // currentLeftRoot_ = QFileInfo(path).absolutePath();
        });
    }
    if (fsTreeRight_) {
        connect(fsTreeRight_,
                &QTreeView::doubleClicked,
                this,
                &ComparePage::onRightTreeDoubleClicked);
        connect(rightText_, &DropTextEdit::fileDroppedToRight, this, [this](const QString &path) {
            loadFileIntoEditor(path, /*isLeft=*/false);

            QFileInfo info(path);
            int       found = rightFileSelect_->findText(info.fileName());
            if (found == -1) {
                rightFileSelect_->addItem(info.fileName());
                found = rightFileSelect_->findText(info.fileName());
            }
            rightFileSelect_->setCurrentIndex(found);

            // currentRightRoot_ = QFileInfo(path).absolutePath();
        });
    }
}

// 내부 상태 업데이트용
void ComparePage::setRoots(const QString &leftRoot, const QString &rightRoot)
{
    currentLeftRoot_  = leftRoot;
    currentRightRoot_ = rightRoot;
    fileInfoLabel_->setText(
            QString("Left: %1 | Right: %2").arg(currentLeftRoot_, currentRightRoot_));
}

// 콤보박스 갱신
// void ComparePage::populateFileSelects(const QStringList& leftFiles,
//                                       const QStringList& rightFiles)
// {
//     leftFileSelect_->clear();
//     leftFileSelect_->addItems(leftFiles);

//     rightFileSelect_->clear();
//     rightFileSelect_->addItems(rightFiles);
// }

// API로 받은 폴더 목록을 UI에 반영하는 자리
void ComparePage::updateFolderListing(const QString     &side,
                                      const QString     &basePath,
                                      const QStringList &folders,
                                      const QStringList &files)
{
    Q_UNUSED(folders)

    if (side == "left") {
        currentLeftRoot_ = basePath;
        leftFiles_       = files;
        leftFileSelect_->clear();
        leftFileSelect_->addItems(leftFiles_);
    } else {
        currentRightRoot_ = basePath;
        rightFiles_       = files;
        rightFileSelect_->clear();
        rightFileSelect_->addItems(rightFiles_);
    }

    fileInfoLabel_->setText(
            QString("Left: %1 | Right: %2").arg(currentLeftRoot_, currentRightRoot_));
}

// 좌/우 본문 텍스트 채우기
void ComparePage::setFileContents(const QString &leftText, const QString &rightText)
{
    leftText_->setPlainText(leftText);
    rightText_->setPlainText(rightText);
}

// diff 테이블 갱신
void ComparePage::refreshDiffTable(const QList<DiffRow> &rows)
{
    diffTable_->setRowCount(rows.size());
    int changeCount = 0;

    for (int i = 0; i < rows.size(); i++) {
        const auto &r = rows[i];

        auto *itemKey   = new QTableWidgetItem(r.key);
        auto *itemLeft  = new QTableWidgetItem(r.origin);
        auto *itemRight = new QTableWidgetItem(r.target);
        auto *itemState = new QTableWidgetItem(r.state);

        if (r.state == "CHANGED") {
            itemKey->setBackground(Qt::yellow);
            itemLeft->setBackground(Qt::yellow);
            itemRight->setBackground(Qt::yellow);
            itemState->setBackground(Qt::yellow);
            changeCount++;
        } else if (r.state == "ADDED") {
            itemKey->setBackground(Qt::green);
            itemLeft->setBackground(Qt::green);
            itemRight->setBackground(Qt::green);
            itemState->setBackground(Qt::green);
            changeCount++;
        } else if (r.state == "REMOVED") {
            itemKey->setBackground(Qt::red);
            itemLeft->setBackground(Qt::red);
            itemRight->setBackground(Qt::red);
            itemState->setBackground(Qt::red);
            changeCount++;
        }

        diffTable_->setItem(i, 0, itemKey);
        diffTable_->setItem(i, 1, itemLeft);
        diffTable_->setItem(i, 2, itemRight);
        diffTable_->setItem(i, 3, itemState);
    }

    statLabel_->setText(QString("Diff: %1 changes").arg(changeCount));
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

void ComparePage::onOpenLeftFolderClicked()
{
    // 나중엔 서버에 basePathHint(currentLeftRoot_) 넘겨서
    // api/v1/folder?path=currentLeftRoot_
    //fetchFolderFromApiDummy("left", currentLeftRoot_);

    // 1) 폴더 선택 다이얼로그 오픈
    QString dirPath = QFileDialog::getExistingDirectory(
            this,
            tr("Select LEFT folder"),
            currentLeftRoot_.isEmpty() ? QDir::homePath() : currentLeftRoot_,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    // 사용자가 취소 눌렀으면 빈 문자열이므로 그냥 종료
    if (dirPath.isEmpty())
        return;

    // 2) 실제 디렉토리 내용 읽어서 UI 반영
    loadLocalFolder("left", dirPath);
}

void ComparePage::onOpenRightFolderClicked()
{
    //fetchFolderFromApiDummy("right", currentRightRoot_);

    QString dirPath = QFileDialog::getExistingDirectory(
            this,
            tr("Select RIGHT folder"),
            currentRightRoot_.isEmpty() ? QDir::homePath() : currentRightRoot_,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty())
        return;

    loadLocalFolder("right", dirPath);
}

void ComparePage::onCompareClicked()
{
    // const QString leftPath  = leftFileSelect_->currentText();
    // const QString rightPath = rightFileSelect_->currentText();

    // // 1) 상위(MainWindow까지 올라갈 로그용 이벤트)
    // emit requestCompare(leftPath, rightPath);

    // // 2) 실제 데이터는 지금은 내부 더미 호출로 UI 갱신
    // fetchCompareFromApiDummy(leftPath, rightPath);

    const QString leftFileName  = leftFileSelect_->currentText();
    const QString rightFileName = rightFileSelect_->currentText();

    // full path 조립
    const QString leftFullPath  = QDir(currentLeftRoot_).filePath(leftFileName);
    const QString rightFullPath = QDir(currentRightRoot_).filePath(rightFileName);

    // 로그/신호
    emit uiCompareClicked(leftFullPath, rightFullPath);

    loadFileIntoEditor(leftFullPath, /*isLeft=*/true);
    loadFileIntoEditor(rightFullPath, /*isLeft=*/false);

    // 지금은 더미 diff로 렌더만 해줌
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

void ComparePage::refreshTreeView(const QString &side, const QString &path)
{
    if (side == "left") {
        // 모델 루트 세팅
        QModelIndex idx = fsModelLeft_->setRootPath(path);
        // 트리뷰에 루트 인덱스 지정
        fsTreeLeft_->setRootIndex(idx);

        // 보기 편하게 첫 번째 컬럼만 보이도록 하려면:
        // 숨기고 싶은 컬럼(예: size, type 등) 조정 가능
        // 예: 1:Size, 2:Type, 3:DateModified (플랫폼/모델에 따라 다를 수도 있음)
        for (int c = 1; c < fsModelLeft_->columnCount(); ++c) {
            fsTreeLeft_->setColumnHidden(c, true);
        }
    } else {
        QModelIndex idx = fsModelRight_->setRootPath(path);
        fsTreeRight_->setRootIndex(idx);
        for (int c = 1; c < fsModelRight_->columnCount(); ++c) {
            fsTreeRight_->setColumnHidden(c, true);
        }
    }
}

void ComparePage::loadFileIntoEditor(const QString &fullPath, bool isLeft)
{
    QFile f(fullPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ComparePage] Cannot open file:" << fullPath;
        // 선택사항: QMessageBox로 사용자에게 알려줄 수도 있음
        // QMessageBox::warning(this, "Open failed", "파일을 열 수 없습니다:\n" + fullPath);
        return;
    }
    QTextStream   ts(&f);
    const QString content = ts.readAll();
    f.close();

    if (isLeft) {
        leftText_->setPlainText(content);
    } else {
        rightText_->setPlainText(content);
    }
}

void ComparePage::onLeftTreeDoubleClicked(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    // 전체 경로
    QString path = fsModelLeft_->filePath(idx);

    QFileInfo info(path);
    if (info.isFile()) {
        // 왼쪽 에디터에 로드
        loadFileIntoEditor(path, /*isLeft=*/true);

        // 콤보박스도 이 파일을 가리키게 업데이트해주면 UX 좋아짐
        // (옵션)
        int found = leftFileSelect_->findText(info.fileName());
        if (found == -1) {
            leftFileSelect_->addItem(info.fileName());
            found = leftFileSelect_->findText(info.fileName());
        }
        leftFileSelect_->setCurrentIndex(found);
    }
}

void ComparePage::onRightTreeDoubleClicked(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;
    QString path = fsModelRight_->filePath(idx);

    QFileInfo info(path);
    if (info.isFile()) {
        // 오른쪽 에디터에 로드
        loadFileIntoEditor(path, /*isLeft=*/false);

        // 우측 콤보박스도 동기화 (옵션)
        int found = rightFileSelect_->findText(info.fileName());
        if (found == -1) {
            rightFileSelect_->addItem(info.fileName());
            found = rightFileSelect_->findText(info.fileName());
        }
        rightFileSelect_->setCurrentIndex(found);
    }
}
