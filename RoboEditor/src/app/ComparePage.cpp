#include "ComparePage.h"
#include "DropTextEdit.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileSystemModel>
#include <QTreeView>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QDebug>

ComparePage::ComparePage(QWidget* parent)
    : QWidget(parent)
{
    currentLeftRoot_  = "/robot/A";
    currentRightRoot_ = "/robot/B";

    // 상단 바
    viewModeCombo_ = new QComboBox(this);
    viewModeCombo_->addItems({"Text", "YAML Key", "JSON Key"});

    openLeftBtn_      = new QPushButton("Left Folder", this);
    leftFileSelect_   = new QComboBox(this);

    openRightBtn_     = new QPushButton("Right Folder", this);
    rightFileSelect_  = new QComboBox(this);

    compareBtn_       = new QPushButton("Compare", this);

    fileInfoLabel_ = new QLabel(
            QString("Left: %1 | Right: %2").arg(currentLeftRoot_, currentRightRoot_),
            this
            );

    auto* topBar = new QHBoxLayout;
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

    // 왼쪽 탭 페이지
    leftTabPage_ = new QWidget(this);
    {
        auto* v = new QVBoxLayout;
        // TODO: 실제로는 여기 fsTreeLeft_ 같은 트리뷰 붙일 예정
        // 지금은 placeholder 라벨로 대체 가능
        QLabel* leftLabel = new QLabel("Left folder browser (TODO: file tree)\ncurrent=" + currentLeftRoot_, this);
        v->addWidget(leftLabel);
        v->addStretch();
        leftTabPage_->setLayout(v);
    }

    // 오른쪽 탭 페이지
    rightTabPage_ = new QWidget(this);
    {
        auto* v = new QVBoxLayout;
        QLabel* rightLabel = new QLabel("Right folder browser (TODO: file tree)\ncurrent=" + currentRightRoot_, this);
        v->addWidget(rightLabel);
        v->addStretch();
        rightTabPage_->setLayout(v);
    }

    dirTabs_->addTab(leftTabPage_,  "Left");
    dirTabs_->addTab(rightTabPage_, "Right");

    // 본문 영역
    leftText_  = new DropTextEdit(this);
    rightText_ = new DropTextEdit(this);

    auto* textAreaLayout = new QHBoxLayout;
    textAreaLayout->addWidget(leftText_, 1);
    textAreaLayout->addWidget(rightText_, 1);

    auto* centerLayout = new QVBoxLayout;

    centerLayout->addLayout(textAreaLayout, 1);

    // diff 영역
    keySearchEdit_   = new QLineEdit(this);
    chkOnlyChanged_  = new QCheckBox("Only Changed", this);
    chkHideSame_     = new QCheckBox("Hide Same", this);
    chkOnlyAddDel_   = new QCheckBox("Only Add/Del", this);
    statLabel_       = new QLabel("Diff: 0 changes", this);

    diffTable_ = new QTableWidget(this);
    diffTable_->setColumnCount(4);
    diffTable_->setHorizontalHeaderLabels({"Key/Line", "Left", "Right", "State"});
    diffTable_->horizontalHeader()->setStretchLastSection(true);
    diffTable_->setSelectionMode(QAbstractItemView::NoSelection);
    diffTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* diffCtrlLayout = new QHBoxLayout;
    diffCtrlLayout->addWidget(keySearchEdit_);
    diffCtrlLayout->addWidget(chkOnlyChanged_);
    diffCtrlLayout->addWidget(chkHideSame_);
    diffCtrlLayout->addWidget(chkOnlyAddDel_);
    diffCtrlLayout->addStretch();
    diffCtrlLayout->addWidget(statLabel_);

    auto* diffLayout = new QVBoxLayout;
    diffLayout->addLayout(diffCtrlLayout);
    diffLayout->addWidget(diffTable_, 1);

    // 전체 body 3분할 (좌: dirTabs_, 중: centerLayout, 우: diffLayout)
    auto* bodyLayout = new QHBoxLayout;
    bodyLayout->addWidget(dirTabs_, 0);          // 왼쪽 고정폭 느낌
    bodyLayout->addLayout(centerLayout, 2);      // 가운데 크게
    bodyLayout->addLayout(diffLayout,   1);      // 오른쪽 비교 결과

    // 최종 레이아웃
    auto* rootLayout = new QVBoxLayout;
    rootLayout->addLayout(topBar);
    rootLayout->addLayout(bodyLayout, 1);
    setLayout(rootLayout);

    // fs model (일단 보관)
    fsModelLeft_  = new QFileSystemModel(this);
    fsModelRight_ = new QFileSystemModel(this);
    fsModelLeft_->setRootPath("");
    fsModelRight_->setRootPath("");
    fsTreeLeft_   = nullptr;
    fsTreeRight_  = nullptr;

    // 초기 파일 목록(더미)
    leftFiles_  = {"main.yaml","task.srl","io_map.yaml"};
    rightFiles_ = {"main.yaml","task.srl","io_map_v2.yaml"};
    leftFileSelect_->addItems(leftFiles_);
    rightFileSelect_->addItems(rightFiles_);

    connect(openLeftBtn_,  &QPushButton::clicked,
            this,          &ComparePage::onOpenLeftFolderClicked);
    connect(openRightBtn_, &QPushButton::clicked,
            this,          &ComparePage::onOpenRightFolderClicked);
    connect(compareBtn_,   &QPushButton::clicked,
            this,          &ComparePage::onCompareClicked);
}

// 내부 상태 업데이트용
void ComparePage::setRoots(const QString& leftRoot, const QString& rightRoot)
{
    currentLeftRoot_  = leftRoot;
    currentRightRoot_ = rightRoot;
    fileInfoLabel_->setText(
            QString("Left: %1 | Right: %2").arg(currentLeftRoot_, currentRightRoot_)
            );
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
void ComparePage::updateFolderListing(const QString& side,
                                      const QString& basePath,
                                      const QStringList& folders,
                                      const QStringList& files)
{
    Q_UNUSED(folders)

    if (side == "left") {
        currentLeftRoot_ = basePath;
        leftFiles_ = files;
        leftFileSelect_->clear();
        leftFileSelect_->addItems(leftFiles_);
    } else {
        currentRightRoot_ = basePath;
        rightFiles_ = files;
        rightFileSelect_->clear();
        rightFileSelect_->addItems(rightFiles_);
    }

    fileInfoLabel_->setText(
            QString("Left: %1 | Right: %2").arg(currentLeftRoot_, currentRightRoot_)
            );
}

// 좌/우 본문 텍스트 채우기
void ComparePage::setFileContents(const QString& leftText,
                                  const QString& rightText)
{
    leftText_->setPlainText(leftText);
    rightText_->setPlainText(rightText);
}

// diff 테이블 갱신
void ComparePage::refreshDiffTable(const QList<DiffRow>& rows)
{
    diffTable_->setRowCount(rows.size());
    int changeCount = 0;

    for (int i = 0; i < rows.size(); i++) {
        const auto& r = rows[i];

        auto* itemKey   = new QTableWidgetItem(r.key);
        auto* itemLeft  = new QTableWidgetItem(r.origin);
        auto* itemRight = new QTableWidgetItem(r.target);
        auto* itemState = new QTableWidgetItem(r.state);

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


void ComparePage::setDiffRows(const QList<DiffRow>& rows)
{
    refreshDiffTable(rows);
}

// ------------------------
// "API 자리 더미"
// ------------------------

// 더미 API: GET api/v1/folder?path=
void ComparePage::fetchFolderFromApiDummy(const QString& side,
                                          const QString& basePathHint)
{
    QString path = basePathHint.isEmpty()
    ? (side == "left" ? "/robot/A" : "/robot/B")
    : basePathHint;

    QStringList dummyFolders = {"cfg","prog","logs"};
    QStringList dummyFiles;

    if (side == "left") {
        dummyFiles = {"main.yaml","task.srl","io_map.yaml"};
    } else {
        dummyFiles = {"main.yaml","task.srl","io_map_v2.yaml"};
    }

    updateFolderListing(side, path, dummyFolders, dummyFiles);
}

// 더미 API: GET api/v1/file?path=... 2번 호출 + diff 결과
void ComparePage::fetchCompareFromApiDummy(const QString& leftFilePath,
                                           const QString& rightFilePath)
{
    Q_UNUSED(leftFilePath)
    Q_UNUSED(rightFilePath)

    QString leftContent =
            "paramA=10\n"
            "paramB=20\n"
            "paramC=OFF\n";

    QString rightContent =
            "paramA=10\n"
            "paramB=22\n"
            "paramC=OFF\n"
            "paramD=EXTRA\n";

    setFileContents(leftContent, rightContent);

    QList<DiffRow> rows;
    rows.push_back({"paramA","10","10",""});
    rows.push_back({"paramB","20","22","CHANGED"});
    rows.push_back({"paramC","OFF","OFF",""});
    rows.push_back({"paramD","","EXTRA","ADDED"});

    setDiffRows(rows);
}

// ------------------------
// 버튼 핸들러
// ------------------------

void ComparePage::onOpenLeftFolderClicked()
{
    // 나중엔 서버에 basePathHint(currentLeftRoot_) 넘겨서
    // api/v1/folder?path=currentLeftRoot_
    fetchFolderFromApiDummy("left", currentLeftRoot_);
}

void ComparePage::onOpenRightFolderClicked()
{
    fetchFolderFromApiDummy("right", currentRightRoot_);
}

void ComparePage::onCompareClicked()
{
    const QString leftPath  = leftFileSelect_->currentText();
    const QString rightPath = rightFileSelect_->currentText();

    // 1) 상위(MainWindow까지 올라갈 로그용 이벤트)
    emit requestCompare(leftPath, rightPath);

    // 2) 실제 데이터는 지금은 내부 더미 호출로 UI 갱신
    fetchCompareFromApiDummy(leftPath, rightPath);
}
