#include "Center.h"

#include <QToolButton>

#include <QDebug>
#include <QDir>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>

Center::Center(QWidget *parent) :
    QWidget(parent),
    splitter_(nullptr),
    treeTabWidget_(nullptr),
    controllerList_(nullptr),
    backupTree_(nullptr),
    workspaceTree_(nullptr),
    controllerModel_(nullptr),
    workspaceModel_(nullptr),
    modifyPage_(nullptr)
{
    setupUI();
}

Center::~Center() {}

void Center::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    splitter_               = new QSplitter(Qt::Horizontal, this);
    treeTabWidget_          = new QTabWidget(splitter_);

    // ===== Controller Model =====
    controllerModel_ = new QFileSystemModel(this);
    controllerModel_->setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    controllerModel_->setRootPath("C:/backup");

    // ===== Tab1 구성 =====
    QWidget     *tab1       = new QWidget;
    QVBoxLayout *tab1Layout = new QVBoxLayout(tab1);

    // --- 상단에 버튼 추가 ---
    QHBoxLayout *topButtonLayout = new QHBoxLayout;

    QToolButton *backButton = new QToolButton;
    backButton->setText("← Back");
    backButton->setToolTip("Return to controller list");
    backButton->setVisible(false);  // 기본적으로는 숨김 상태

    QToolButton *refreshButton = new QToolButton;
    refreshButton->setText("⟳ Refresh");
    refreshButton->setToolTip("Reload controller list");
    refreshButton->setVisible(true);  // 기본적으로는 보임

    // 버튼 가로 정렬
    topButtonLayout->addWidget(backButton);
    topButtonLayout->addWidget(refreshButton);
    topButtonLayout->addStretch();

    tab1Layout->addLayout(topButtonLayout);
    tab1Layout->addWidget(stack_);
    tab1->setLayout(tab1Layout);

    // (1) Controller List
    controllerList_ = new QListView;
    controllerList_->setModel(controllerModel_);
    controllerList_->setRootIndex(controllerModel_->index("C:/backup"));
    controllerList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    controllerList_->setSelectionMode(QAbstractItemView::SingleSelection);

    // (2) Backup Tree
    backupTree_ = new QTreeView;
    backupTree_->setModel(controllerModel_);
    backupTree_->setColumnHidden(1, true);
    backupTree_->setColumnHidden(2, true);
    backupTree_->setColumnHidden(3, true);
    backupTree_->setExpandsOnDoubleClick(false);
    backupTree_->setStyleSheet("QTreeView::branch { image: none; border-image: none; }");

    // (3) Stack: 리스트 <-> 트리 전환용
    stack_ = new QStackedWidget;
    stack_->addWidget(controllerList_);
    stack_->addWidget(backupTree_);

    tab1Layout->addWidget(backButton);
    tab1Layout->addWidget(stack_);
    tab1->setLayout(tab1Layout);

    // ===== Tab2 구성 =====
    QWidget     *tab2       = new QWidget;
    QVBoxLayout *tab2Layout = new QVBoxLayout(tab2);

    workspaceModel_ = new QFileSystemModel(this);
    workspaceModel_->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
    workspaceModel_->setRootPath("C:/backup");

    workspaceTree_ = new QTreeView;
    workspaceTree_->setModel(workspaceModel_);
    workspaceTree_->setColumnHidden(1, true);
    workspaceTree_->setColumnHidden(2, true);
    workspaceTree_->setColumnHidden(3, true);

    tab2Layout->addWidget(workspaceTree_);
    tab2->setLayout(tab2Layout);

    // ===== 탭 추가 =====
    treeTabWidget_->addTab(tab1, "Controller");
    treeTabWidget_->addTab(tab2, "Workspace");

    // ===== ModifyPage =====
    modifyPage_ = new ModifyPage(splitter_);
    splitter_->addWidget(treeTabWidget_);
    splitter_->addWidget(modifyPage_);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 10);
    mainLayout->addWidget(splitter_);
    setLayout(mainLayout);

    // ===== 시그널 연결 =====

    // [1] 리스트 클릭 → BackupTree로 전환
    connect(controllerList_, &QListView::clicked, this, [=](const QModelIndex &index) {
        QString controllerPath = controllerModel_->filePath(index);
        if (!QDir(controllerPath).exists())
            return;

        backupTree_->setRootIndex(controllerModel_->index(controllerPath));
        stack_->setCurrentIndex(1);  // BackupTree로 전환

        // ✅ 버튼 표시 상태 조정
        backButton->setVisible(true);
        refreshButton->setVisible(false);  // ← 백업 트리에서는 숨김

        qDebug() << "Switched to BackupTree for:" << controllerPath;
    });

    // [2] Back 버튼 클릭 → 리스트로 복귀
    connect(backButton, &QToolButton::clicked, this, [=]() {
        stack_->setCurrentIndex(0);  // Controller list로 복귀

        // ✅ 버튼 표시 상태 조정
        backButton->setVisible(false);
        refreshButton->setVisible(true);  // ← 리스트로 돌아오면 다시 표시

        qDebug() << "Returned to controller list view";
    });

    // [3] BackupTree 클릭 → workspace 탭으로 이동
    connect(backupTree_, &QTreeView::clicked, this, [=](const QModelIndex &index) {
        QString   selectedPath = controllerModel_->filePath(index);
        QFileInfo info(selectedPath);
        if (!info.isDir())
            return;

        workspacePath_ = selectedPath;
        qDebug() << "Backup selected, switching to workspace:" << workspacePath_;

        workspaceTree_->setRootIndex(workspaceModel_->index(workspacePath_));
        treeTabWidget_->setCurrentIndex(1);  // 탭2로 이동
        emit workspaceSelected(workspacePath_);
    });
}

QString Center::getWorkspacePath() const
{
    return workspacePath_;
}

void Center::setBackupPath(const QString &path)
{
    backupRootPath_ = path;
    QDir dir(backupRootPath_);
    if (!dir.exists())
        dir.mkpath(backupRootPath_);
    controllerModel_->setRootPath(backupRootPath_);
    controllerList_->setRootIndex(controllerModel_->index(backupRootPath_));
    workspaceModel_->setRootPath(backupRootPath_);
}
void Center::onControllerTreeClicked(const QModelIndex &index)
{
    // 안전하게 로그만 남기거나, 실제 동작을 정의해도 됨.
    QString selectedPath = controllerModel_->filePath(index);
    qDebug() << "onControllerTreeClicked called for:" << selectedPath;
}
