#include "BackupPage.h"

#include <QButtonGroup>
#include <QDir>
#include <QFileInfoList>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

BackupPage::BackupPage(QWidget *parent) : QWidget(parent), settings("RoboEditor")
{
    customHomePath = settings.value("customHomePath", QDir::homePath()).toString();

    setupUi();
    populateDummyData();
    setupConnections();
}

BackupPage::~BackupPage() = default;

void BackupPage::setupUi()
{
    // ===== Directory Explorer =====
    dirTitle = new QLabel("file directory", this);

    // --- Buttons ---
    upButton      = new QToolButton(this);
    homeButton    = new QToolButton(this);
    setHomeButton = new QToolButton(this);
    refreshButton = new QToolButton(this);

    upButton->setText("⬆");
    homeButton->setText("🏠");
    setHomeButton->setText("📌");
    refreshButton->setText("🔄");

    QList<QToolButton *> dirButtons = {upButton, homeButton, setHomeButton, refreshButton};
    for (auto *btn : dirButtons) {
        btn->setFixedWidth(30);
        btn->setToolTip(btn->text());
    }

    // --- Directory Layout ---
    QHBoxLayout *dirControlLayout = new QHBoxLayout();
    dirControlLayout->addWidget(upButton);
    dirControlLayout->addWidget(homeButton);
    dirControlLayout->addWidget(setHomeButton);
    dirControlLayout->addWidget(refreshButton);
    dirControlLayout->addStretch();

    dirModel = new QFileSystemModel(this);
    dirModel->setRootPath(QDir::homePath());
    dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);

    dirTree = new QTreeView(this);
    dirTree->setModel(dirModel);
    dirTree->setRootIndex(dirModel->index(QDir::homePath()));
    dirTree->setHeaderHidden(true);
    dirTree->setColumnHidden(1, true);  // Size
    dirTree->setColumnHidden(2, true);  // Type
    dirTree->setColumnHidden(3, true);  // Date Modified
    dirTree->setMinimumWidth(250);
    dirTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    dirLayout = new QVBoxLayout();
    dirLayout->addWidget(dirTitle);
    dirLayout->addLayout(dirControlLayout);
    dirLayout->addWidget(dirTree);

    // ===== Backup List =====
    backupTableTitle = new QLabel("backup folder directory", this);

    backupList = new QTableWidget(this);
    backupList->setColumnCount(5);
    backupList->setHorizontalHeaderLabels({"index", "date", "size", "comment", ""});
    backupList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    backupList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    backupList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    backupList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    backupList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    backupList->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    backupList->setSelectionMode(QAbstractItemView::NoSelection);
    backupList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    backupList->verticalHeader()->setVisible(false);
    backupList->setShowGrid(false);

    backupDelete = new QPushButton("delete", this);
    radioGroup   = new QButtonGroup(this);
    radioGroup->setExclusive(true);

    workspaceLayout = new QVBoxLayout();
    workspaceLayout->addWidget(backupTableTitle);
    workspaceLayout->addWidget(backupList);
    workspaceLayout->addWidget(backupDelete);

    // ===== WorkDir Backup =====
    backupActionTitle = new QLabel("작업 폴더 백업하기", this);
    sizeLabel         = new QLabel("size : 1.5 GB", this);
    commentLabel      = new QLabel("comment", this);
    backupComment     = new QTextEdit(this);
    backupCreate      = new QPushButton("Backup", this);

    QVBoxLayout *rightTopLayout = new QVBoxLayout();
    rightTopLayout->addWidget(backupActionTitle);
    rightTopLayout->addWidget(sizeLabel);
    rightTopLayout->addStretch();

    QVBoxLayout *rightBottomLayout = new QVBoxLayout();
    rightBottomLayout->addWidget(commentLabel);
    rightBottomLayout->addWidget(backupComment);
    rightBottomLayout->addWidget(backupCreate);
    rightBottomLayout->setStretch(1, 2);

    infoLayout = new QVBoxLayout();
    infoLayout->addLayout(rightTopLayout);
    infoLayout->addLayout(rightBottomLayout);
    infoLayout->setContentsMargins(5, 5, 5, 5);

    // ===== Overall Layout =====
    contentLayout = new QHBoxLayout();
    contentLayout->addLayout(dirLayout, 1);
    contentLayout->addLayout(workspaceLayout, 2);
    contentLayout->addLayout(infoLayout, 1);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(contentLayout);

    setLayout(mainLayout);
    backupDelete->setEnabled(false);
}

void BackupPage::populateDummyData()
{
    for (int i = 0; i < 5; ++i) {
        backupList->insertRow(i);
        backupList->setItem(i, 0, new QTableWidgetItem(QString::number(5 - i)));
        backupList->setItem(i, 1, new QTableWidgetItem(QString("2025.10.%1").arg(11 + i)));
        backupList->setItem(i, 2, new QTableWidgetItem("1.4 GB"));
        backupList->setItem(i, 3, new QTableWidgetItem(QString("로봇 %1 설정 변경").arg(i + 1)));

        QWidget      *radioWidget = new QWidget();
        QHBoxLayout  *radioLay    = new QHBoxLayout(radioWidget);
        QRadioButton *radio       = new QRadioButton(radioWidget);
        radioLay->setContentsMargins(6, 0, 6, 0);
        radioLay->addWidget(radio);
        radioLay->setAlignment(Qt::AlignCenter);

        backupList->setCellWidget(i, 4, radioWidget);
        radioGroup->addButton(radio, i);
    }
}

void BackupPage::setupConnections()
{
    // Directory controls
    connect(dirTree, &QTreeView::doubleClicked, this, &BackupPage::onDirectorySelected);
    connect(upButton, &QToolButton::clicked, this, &BackupPage::onDirUpClicked);
    connect(homeButton, &QToolButton::clicked, this, &BackupPage::onDirHomeClicked);
    connect(setHomeButton, &QToolButton::clicked, this, &BackupPage::onSetHomeClicked);
    connect(refreshButton, &QToolButton::clicked, this, &BackupPage::onDirRefreshClicked);

    // Backup actions
    connect(backupCreate, &QPushButton::clicked, this, &BackupPage::onBackupCreateClicked);
    connect(backupDelete, &QPushButton::clicked, this, &BackupPage::onBackupDeleteClicked);
    connect(radioGroup, &QButtonGroup::idClicked, this, &BackupPage::onRadioSelected);
}

// ===== Slots =====
void BackupPage::onDirUpClicked()
{
    QModelIndex parentIndex = dirTree->rootIndex().parent();
    if (parentIndex.isValid())
        dirTree->setRootIndex(parentIndex);
}

void BackupPage::onDirHomeClicked()
{
    QString     targetHome = customHomePath.isEmpty() ? QDir::homePath() : customHomePath;
    QModelIndex homeIndex  = dirModel->index(targetHome);
    dirTree->setRootIndex(homeIndex);
}

void BackupPage::onSetHomeClicked()
{
    QModelIndex currentIndex = dirTree->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, "Set Home", "먼저 폴더를 선택하세요.");
        return;
    }

    QString newHomePath = dirModel->filePath(currentIndex);
    if (!QFileInfo(newHomePath).isDir()) {
        QMessageBox::warning(this, "Invalid Folder", "유효한 폴더가 아닙니다.");
        return;
    }

    customHomePath = newHomePath;
    settings.setValue("customHomePath", customHomePath);
    QMessageBox::information(
            this,
            "홈 경로 변경됨",
            QString("'%1' 폴더가 새로운 홈으로 설정되었습니다.").arg(customHomePath));
}

void BackupPage::onDirRefreshClicked()
{
    QModelIndex currentIndex = dirTree->rootIndex();
    QString     currentPath  = dirModel->filePath(currentIndex);

    // ✅ 루트 경로를 다시 설정하면 내부 캐시가 초기화되고 새로 로드됩니다.
    dirModel->setRootPath(QString());         // reset
    dirModel->setRootPath(QDir::homePath());  // optional full reload
    dirTree->setRootIndex(dirModel->index(currentPath));
}

void BackupPage::onDirectorySelected(const QModelIndex &index)
{
    QString path = dirModel->filePath(index);
    if (QFileInfo(path).isDir()) {
        dirTree->setRootIndex(index);
        qDebug() << "📂 Entered folder:" << path;
    }

    qint64        size = 0;
    QDir          dir(path);
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : files)
        size += info.size();

    double sizeGB = size / (1024.0 * 1024 * 1024);
    sizeLabel->setText(
            QString("선택된 폴더: %1\nsize: %2 GB").arg(path).arg(QString::number(sizeGB, 'f', 2)));
}

void BackupPage::onBackupCreateClicked()
{
    QString comment = backupComment->toPlainText();
    qDebug() << "백업 생성 요청:" << comment;
    emit uiBackupClicked(comment);
}

void BackupPage::onBackupDeleteClicked()
{
    int selectedId = radioGroup->checkedId();
    if (selectedId == -1) {
        backupDelete->setEnabled(false);
        return;
    }

    QAbstractButton *selectedButton = radioGroup->button(selectedId);
    if (!selectedButton)
        return;

    int targetRow = -1;
    for (int row = 0; row < backupList->rowCount(); ++row) {
        QWidget *cellWidget = backupList->cellWidget(row, 4);
        if (cellWidget && cellWidget->findChild<QRadioButton *>() == selectedButton) {
            targetRow = row;
            break;
        }
    }

    if (targetRow != -1)
        backupList->removeRow(targetRow);

    backupDelete->setEnabled(false);
    qDebug() << "🗑 백업 삭제 완료";
}

void BackupPage::onRadioSelected(int id)
{
    if (id >= 0)
        backupDelete->setEnabled(true);
}
