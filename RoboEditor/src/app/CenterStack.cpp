#include "CenterStack.h"

#include <QToolButton>

#include <QBrush>
#include <QDebug>
#include <QDir>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QStackedWidget>
#include <QStyleHints>
#include <QVBoxLayout>

#include "ApplyPage.h"
#include "BackupPage.h"
#include "ComparePage.h"
#include "ControllerManager.h"
#include "LogManager.h"
#include "ModifyPage.h"
#include "WorkspaceContextMenuController.h"

static QIcon makeCircleIcon(const QColor &color, int size = 12)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size - 1, size - 1);
    return QIcon(pixmap);
}

CenterStack::CenterStack(QWidget *parent) :
    QWidget(parent),
    m_pollingTimer(new QTimer(this)),
    splitter_(nullptr),
    treeTabWidget_(nullptr),
    controllerList_(nullptr),
    backupTree_(nullptr),
    workspaceTree_(nullptr),
    controllerModel_(nullptr),
    backupModel_(nullptr),
    workspaceModel_(nullptr),
    modifyPage_(nullptr)
{
    stack_ = new QStackedWidget;
    cmp_   = new ComparePage;

    idxC_ = stack_->addWidget(cmp_);

    // auto *layout = new QVBoxLayout(this);
    // layout->addWidget(stack_);
    // layout->setContentsMargins(0, 0, 0, 0);

    connect(cmp_, &ComparePage::uiCompareClicked, this, [=](const QString &L, const QString &R) {
        emit compareRequested(L, R);
        openCompareResult(L, R);
    });

    connect(m_pollingTimer, &QTimer::timeout, this, &CenterStack::onPollingTimeout);

    connect(ControllerManager::instance(),
            &ControllerManager::controllerStateUpdated,
            this,
            &CenterStack::updateControllerList);

    setupUI();
    startPolling(5000);
}

void CenterStack::openCompareResult(const QString &left, const QString &right)
{
    QWidget *result = new QWidget;
    auto    *ly     = new QVBoxLayout(result);
    ly->addWidget(new QLabel(QString("Result for:\nL=%1\nR=%2").arg(left, right)));
    ly->addStretch();

    int idx = stack_->addWidget(result);
    stack_->setCurrentIndex(idx);
}
void CenterStack::showCompare()
{
    stack_->setCurrentIndex(idxC_);
}

void CenterStack::showModifyWithCompare()
{
    // ModifyPage의 Compare 기능 활성화
    if (modifyPage_) {
        modifyPage_->showCompare();
    }
}
void CenterStack::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    splitter_               = new QSplitter(Qt::Horizontal, this);
    treeTabWidget_          = new QTabWidget(splitter_);

    // ===== Controller Model (QStandardItemModel) =====
    controllerModel_ = new QStandardItemModel(this);
    controllerModel_->setColumnCount(1);
    controllerModel_->setHeaderData(0, Qt::Horizontal, "Controller List");

    // ===== Tab1 구성 =====
    QWidget     *tab1       = new QWidget;
    QVBoxLayout *tab1Layout = new QVBoxLayout(tab1);

    // --- 상단 버튼 ---
    QHBoxLayout *topButtonLayout = new QHBoxLayout;

    QToolButton *backButton = new QToolButton;
    backButton->setText("← Back");
    backButton->setToolTip("Return to controller list");
    backButton->setVisible(false);

    QToolButton *refreshButton = new QToolButton;
    refreshButton->setText("⟳ Refresh");
    refreshButton->setToolTip("Reload controller list");
    refreshButton->setVisible(true);

    topButtonLayout->addWidget(backButton);
    topButtonLayout->addWidget(refreshButton);
    topButtonLayout->addStretch();

    // (1) Controller List
    controllerList_ = new QListView;
    controllerList_->setModel(controllerModel_);
    controllerList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    controllerList_->setSelectionMode(QAbstractItemView::SingleSelection);

    controllerList_->setContextMenuPolicy(Qt::CustomContextMenu);

    // (2) Backup Tree
    backupModel_ = new QFileSystemModel(this);
    backupModel_->setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    backupModel_->setRootPath("C:/backup");

    backupTree_ = new QTreeView;
    backupTree_->setModel(backupModel_);
    backupTree_->header()->hide();
    backupTree_->setColumnHidden(1, true);
    backupTree_->setColumnHidden(2, true);
    backupTree_->setColumnHidden(3, true);
    backupTree_->setExpandsOnDoubleClick(false);
    backupTree_->setStyleSheet("QTreeView::branch { image: none; border-image: none; }");

    // (3) Stack (ControllerList ↔ BackupTree 전환)
    internalStack_ = new QStackedWidget;
    internalStack_->addWidget(controllerList_);
    internalStack_->addWidget(backupTree_);

    tab1Layout->addLayout(topButtonLayout);
    tab1Layout->addWidget(internalStack_);
    tab1->setLayout(tab1Layout);

    // ===== Tab2 구성 =====
    QWidget     *tab2       = new QWidget;
    QVBoxLayout *tab2Layout = new QVBoxLayout(tab2);

    workspaceModel_ = new QFileSystemModel(this);
    workspaceModel_->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
    workspaceModel_->setRootPath("C:/backup");

    workspaceTree_ = new QTreeView;
    workspaceTree_->setModel(workspaceModel_);
    workspaceTree_->header()->hide();
    workspaceTree_->setColumnHidden(1, true);
    workspaceTree_->setColumnHidden(2, true);
    workspaceTree_->setColumnHidden(3, true);
    workspaceTree_->setDragEnabled(true);
    workspaceTree_->setAcceptDrops(true);
    workspaceTree_->setDropIndicatorShown(true);
    backupTree_->setExpandsOnDoubleClick(false);
    workspaceTree_->setDragDropMode(QAbstractItemView::DragDrop);
    workspaceTree_->setDefaultDropAction(Qt::MoveAction);
    workspaceTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    workspaceTree_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    tab2Layout->addWidget(workspaceTree_);
    tab2->setLayout(tab2Layout);

    workspaceMenuController_ = new WorkspaceContextMenuController(
            workspaceTree_,
            workspaceModel_,
            workspaceModel_->rootPath(),  // 초기 root는 "C:/backup"
            this);

    // ===== 탭 추가 =====
    treeTabWidget_->addTab(tab1, "Controller");
    treeTabWidget_->addTab(tab2, "Workspace");

    // ===== ModifyPage =====
    modifyPage_ = new ModifyPage(splitter_);
    splitter_->addWidget(treeTabWidget_);
    splitter_->addWidget(modifyPage_);
    splitter_->setStretchFactor(0, 2);
    splitter_->setStretchFactor(1, 15);
    mainLayout->addWidget(splitter_);
    setLayout(mainLayout);

    // ---------------------- 시그널 연결 ----------------------

    connect(controllerList_, &QListView::customContextMenuRequested, this, [=](const QPoint &pos) {
        QModelIndex index = controllerList_->indexAt(pos);
        if (!index.isValid())
            return;

        QString controllerName = index.data(Qt::DisplayRole).toString();
        qDebug() << "우클릭 발생:" << controllerName;

        QMenu    menu;
        QAction *editAction   = menu.addAction("제어기 수정");
        QAction *removeAction = menu.addAction("제어기 삭제");

        QAction *selected = menu.exec(controllerList_->viewport()->mapToGlobal(pos));
        if (selected == editAction)
            onEditController(controllerName);
        else if (selected == removeAction)
            onRemoveController(controllerName);
    });
    // [1] Controller 클릭 → BackupTree로 전환
    connect(controllerList_, &QListView::clicked, this, [=](const QModelIndex &index) {
        QString controllerPath = index.data(Qt::UserRole + 1).toString();
        qDebug() << "[DEBUG] controllerPath =" << controllerPath;
        if (!QDir(controllerPath).exists())
            return;

        backupTree_->setRootIndex(backupModel_->index(controllerPath));
        internalStack_->setCurrentIndex(1);

        backButton->setVisible(true);
        refreshButton->setVisible(false);

        qDebug() << "Switched to BackupTree for:" << controllerPath;
    });

    // [2] Back 버튼 클릭 → ControllerList로 복귀
    connect(backButton, &QToolButton::clicked, this, [=]() {
        internalStack_->setCurrentIndex(0);
        backButton->setVisible(false);
        refreshButton->setVisible(true);
        qDebug() << "Returned to controller list view";
    });

    // [3] BackupTree 클릭 → Workspace 탭으로 이동
    connect(backupTree_, &QTreeView::clicked, this, [=](const QModelIndex &index) {
        QString   selectedPath = backupModel_->filePath(index);
        QFileInfo info(selectedPath);
        if (!info.isDir())
            return;

        workspacePath_ = selectedPath;
        workspaceTree_->setRootIndex(workspaceModel_->index(workspacePath_));
        treeTabWidget_->setCurrentIndex(1);

        if (workspaceMenuController_) {
            workspaceMenuController_->setWorkspaceRoot(workspacePath_);
        }

        emit workspaceSelected(workspacePath_);
        qDebug() << "Backup selected, switching to workspace:" << workspacePath_;
    });

    // [4] WorkspaceTree 더블클릭 → ModifyPage 열기
    connect(workspaceTree_, &QTreeView::clicked, this, [=](const QModelIndex &index) {
        QString   path = workspaceModel_->filePath(index);
        QFileInfo info(path);

        if (info.isDir()) {
            bool expanded = workspaceTree_->isExpanded(index);
            workspaceTree_->setExpanded(index, !expanded);  // 한 번 클릭으로 토글
        } else if (info.isFile()) {
            modifyPage_->openDocument(path);
            qDebug() << "Opened file in ModifyPage:" << path;
        }
    });

    // [4-1] ModifyPage 시그널 연결
    connect(modifyPage_, &ModifyPage::uiModifyClicked, this, &CenterStack::modifyRequested);

    // [5] 제어기 등록 -> 제어기 리스트 업데이트
    connect(ControllerManager::instance(),
            &ControllerManager::controllerListChanged,
            this,
            &CenterStack::updateControllerList);

    // [6] refresh버튼 클릭 -> 제어기 리스트 업데이트
    connect(refreshButton, &QToolButton::clicked, this, [=]() {
        ControllerManager::instance()->updateControllersStates();
        this->updateControllerList();
        qDebug() << "[CenterStack] Controller list refreshed.";
    });

    updateControllerList();
}

void CenterStack::updateControllerList()
{
    controllerModel_->clear();
    controllerModel_->setHorizontalHeaderLabels({"Controller List"});

    // 1. ControllerManager의 싱글톤 인스턴스 가져오기
    ControllerManager *manager = ControllerManager::instance();

    // 2. controllers_ 리스트 가져오기
    QList<ControllerInfo> controllers = manager->getControllers();

    // 3. 리스트가 비어있는 경우
    if (controllers.isEmpty()) {
        QStandardItem *emptyItem = new QStandardItem("등록된 제어기가 없습니다.");
        emptyItem->setFlags(Qt::NoItemFlags);
        controllerModel_->appendRow(emptyItem);
        return;
    }

    // 4. 제어기 상태 표시
    for (const auto &c : controllers) {
        QStandardItem *item = new QStandardItem(c.serialNumber);
        item->setEditable(false);
        item->setData(QString("C:/backup/%1").arg(c.serialNumber), Qt::UserRole + 1);
        item->setToolTip(QString("IP: %1\nSFTP: %2\nUser: %3\nWorkspace: %4")
                                 .arg(c.ip)
                                 .arg(c.sftpPort)
                                 .arg(c.username)
                                 .arg(c.wsPath));
        bool isDark = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);

        if (!isDark) {
            QColor iconColor;
            if (!c.isConnected) {
                iconColor = Qt::gray;
                item->setForeground(QBrush(Qt::gray));
            } else {
                iconColor = c.isRunning ? Qt::red : Qt::green;
                item->setForeground(QBrush(Qt::black));
            }
            item->setIcon(makeCircleIcon(iconColor, 10));
        } else {
            QColor iconColor;
            if (!c.isConnected) {
                iconColor = Qt::gray;
                item->setForeground(QBrush(Qt::gray));
            } else {
                iconColor = c.isRunning ? Qt::red : Qt::green;
                item->setForeground(QBrush(Qt::white));
            }
            item->setIcon(makeCircleIcon(iconColor, 10));
        }

        controllerModel_->appendRow(item);
    }

    controllerList_->setModel(controllerModel_);
    controllerList_->update();
}

QString CenterStack::getWorkspacePath() const
{
    return workspacePath_;
}

void CenterStack::setBackupPath(const QString &path)
{
    backupRootPath_ = path;
    QDir dir(backupRootPath_);
    if (!dir.exists())
        dir.mkpath(backupRootPath_);

    backupModel_->setRootPath(backupRootPath_);
    workspaceModel_->setRootPath(backupRootPath_);

    if (workspaceMenuController_) {
        workspaceMenuController_->setWorkspaceRoot(backupRootPath_);
    }
}

void CenterStack::onControllerTreeClicked(const QModelIndex &index)
{
    QString selectedPath = backupModel_->filePath(index);
    qDebug() << "onControllerTreeClicked called for:" << selectedPath;
}

//제어기 info 수정
void CenterStack::onEditController(const QString &serialNumber)
{
    ControllerManager    *manager     = ControllerManager::instance();
    QList<ControllerInfo> controllers = manager->getControllers();

    // 해당 제어기 찾기
    for (const auto &c : controllers) {
        if (c.serialNumber == serialNumber) {
            manager->updateInfo(c);
            break;
        }
    }
}

// 제어기 삭제
void CenterStack::onRemoveController(const QString &serialNumber)
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,
                                  "제어기 삭제",
                                  QString("정말로 '%1'을(를) 삭제하시겠습니까?\n\n"
                                          "※ 백업 폴더는 삭제되지 않습니다.")
                                          .arg(serialNumber),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        qDebug() << "[CenterStack] 삭제 취소됨";
        return;
    }

    ControllerManager    *manager     = ControllerManager::instance();
    QList<ControllerInfo> controllers = manager->getControllers();

    // 해당 인덱스 찾아서 삭제
    for (int i = 0; i < controllers.size(); ++i) {
        if (controllers[i].serialNumber == serialNumber) {
            manager->removeController(i);
            updateControllerList();

            QMessageBox::information(
                    this, "삭제 완료", QString("'%1'이(가) 삭제되었습니다.").arg(serialNumber));

            QString msg = QString("Controller removed: %1").arg(serialNumber);
            LogManager::append(msg);
            break;
        }
    }
}
void CenterStack::startPolling(int intervalMs)
{
    if (m_pollingTimer->isActive()) {
        qWarning() << "[CenterStack] Polling already started";
        return;
    }

    // 즉시 한 번 실행
    updateControllerList();

    // 주기적으로 실행
    m_pollingTimer->start(intervalMs);
}

void CenterStack::stopPolling()
{
    if (m_pollingTimer->isActive()) {
        m_pollingTimer->stop();
    }
}

void CenterStack::onPollingTimeout()
{
    qDebug() << "timeout";
    ControllerManager::instance()->updateControllersStates();
}

CenterStack::~CenterStack()
{
    stopPolling();
}
