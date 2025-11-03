#include "openfilepage.h"

#include <QTreeWidgetItem>

#include <QDebug>
#include <QHeaderView>

OpenFilePage::OpenFilePage(QWidget *parent) : QWidget(parent)
{
    setupUi();
    populateDummyData();

    connect(workspaceRadioGroup, &QButtonGroup::buttonClicked, this, [=](QAbstractButton *button) {
        int id = workspaceRadioGroup->id(button);
        onStorageFolderSelected(id);
    });
    connect(robotRadioGroup, &QButtonGroup::buttonClicked, this, [=](QAbstractButton *button) {
        int id = robotRadioGroup->id(button);
        onRobotFolderSelected(id);
    });
    connect(loadButton, &QPushButton::clicked, this, &OpenFilePage::onLoadClicked);
}

void OpenFilePage::setupUi()
{
    mainLayout    = new QVBoxLayout(this);
    contentLayout = new QHBoxLayout();

    robotLayout     = new QVBoxLayout();
    workspaceLayout = new QVBoxLayout();
    infoLayout      = new QVBoxLayout();

    // ===== Robot Table =====
    robotTableTitle = new QLabel("Copy from robot controller");
    robotList       = new QTableWidget(this);
    robotList->setColumnCount(4);
    robotList->setHorizontalHeaderLabels({"Serial Number", "ip", "connection state", "select"});
    robotList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    robotList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    robotList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    robotList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    robotList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    robotList->setSelectionMode(QAbstractItemView::NoSelection);
    robotList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    robotList->verticalHeader()->setVisible(false);
    robotList->setShowGrid(false);

    robotRadioGroup = new QButtonGroup(this);

    robotLayout->addWidget(robotTableTitle);
    robotLayout->addWidget(robotList);
    robotLayout->setStretch(1, 1);

    // ===== Storage Table =====
    workspaceTableTitle = new QLabel("Copy from backup folder");
    workspaceList       = new QTableWidget(this);
    workspaceList->setColumnCount(5);
    workspaceList->setHorizontalHeaderLabels({"index", "date", "size", "comment", "select"});
    workspaceList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    workspaceList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    workspaceList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    workspaceList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    workspaceList->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    workspaceList->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    workspaceList->setSelectionMode(QAbstractItemView::NoSelection);
    workspaceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    workspaceList->verticalHeader()->setVisible(false);
    workspaceList->setShowGrid(false);

    workspaceRadioGroup = new QButtonGroup(this);

    workspaceLayout->addWidget(workspaceTableTitle);
    workspaceLayout->addWidget(workspaceList);
    workspaceLayout->setStretch(1, 1);

    // ===== Info Section =====
    detailTitle = new QLabel("detail");
    detailDate  = new QLabel("date : yyyy.MM.dd hh:mm:ss");
    detailSize  = new QLabel("size : x.x B");

    commentLabel = new QLabel("comment");
    commentValue = new QLabel();
    commentValue->setText("");

    previewLabel = new QLabel("preview");
    previewTree  = new QTreeWidget();
    previewTree->setHeaderHidden(true);
    previewTree->setMinimumWidth(200);

    loadButton = new QPushButton("LOAD");

    infoLayout->addWidget(detailTitle);
    infoLayout->addWidget(detailDate);
    infoLayout->addWidget(detailSize);
    infoLayout->addSpacing(10);
    infoLayout->addWidget(commentLabel);
    infoLayout->addWidget(commentValue);
    infoLayout->addSpacing(10);
    infoLayout->addWidget(previewLabel);
    infoLayout->addWidget(previewTree);
    infoLayout->addWidget(loadButton);

    // 전체 배치
    contentLayout->addLayout(robotLayout, 2);
    contentLayout->addSpacing(10);
    contentLayout->addLayout(workspaceLayout, 3);
    contentLayout->addSpacing(10);
    contentLayout->addLayout(infoLayout, 2);
    mainLayout->addLayout(contentLayout);
}

void OpenFilePage::populateDummyData()
{
    // === Storage Dummy Data ===
    struct StorageData
    {
        QString date, size, comment;
    };
    QList<StorageData> storageList = {
            {"2025.10.15 10:22:27", "1.4 GB", "제어기 0번 로봇 1 @@ 설정 변경"},
            {"2025.10.14 13:22:24", "1.4 GB", "제어기 1번 로봇 2 @@ 설정 변경"},
            {"2025.10.13 10:21:44", "1.4 GB", "제어기 2번 로봇 3 ## 설정 변경"},
            {"2025.10.12 12:22:44", "1.4 GB", "제어기 1 !! 설정 변경"},
            {"2025.10.11 10:20:56", "1.4 GB", "제어기 0번 로봇 1 @@프로그램 변경"},
    };

    workspaceList->setRowCount(storageList.size());
    for (int i = 0; i < storageList.size(); ++i) {
        workspaceList->setItem(i, 0, new QTableWidgetItem(QString::number(storageList.size() - i)));
        workspaceList->setItem(i, 1, new QTableWidgetItem(storageList[i].date));
        workspaceList->setItem(i, 2, new QTableWidgetItem(storageList[i].size));
        workspaceList->setItem(i, 3, new QTableWidgetItem(storageList[i].comment));

        QRadioButton *radio = new QRadioButton();
        workspaceList->setCellWidget(i, 4, radio);
        workspaceRadioGroup->addButton(radio, i);
    }

    // === Robot Dummy Data ===
    struct RobotData
    {
        QString mac, ip, state;
    };
    QList<RobotData> robotListDummy = {
            {"A0:B1:C2:D3:E4:F5", "192.168.0.10", "ready"},
            {"12:34:56:78:9A:BC", "192.168.0.11", "ready"},
            {"DE:AD:BE:EF:00:11", "10.0.1.20", "busy"},
            {"F0:0D:AB:CD:EF:99", "172.16.5.30", "error"},
    };

    robotList->setRowCount(robotListDummy.size());
    for (int i = 0; i < robotListDummy.size(); ++i) {
        robotList->setItem(i, 0, new QTableWidgetItem(robotListDummy[i].mac));
        robotList->setItem(i, 1, new QTableWidgetItem(robotListDummy[i].ip));

        QTableWidgetItem *stateItem = new QTableWidgetItem(robotListDummy[i].state);
        if (robotListDummy[i].state == "ready")
            stateItem->setForeground(Qt::blue);
        else if (robotListDummy[i].state == "busy")
            stateItem->setForeground(Qt::darkMagenta);
        else if (robotListDummy[i].state == "error")
            stateItem->setForeground(Qt::red);
        robotList->setItem(i, 2, stateItem);

        QRadioButton *radio = new QRadioButton();
        robotList->setCellWidget(i, 3, radio);
        robotRadioGroup->addButton(radio, i);
    }

    // === Preview Tree ===
    QStringList paths = {"C:/OSDK",
                         "C:/OSDK/srcpy03",
                         "C:/OSDK/srcpy03/burger_cam.yaml",
                         "C:/OSDK/srcpy03/burger_cam_copy.yaml",
                         "C:/OSDK/vcompat.dll"};

    QTreeWidgetItem *root = new QTreeWidgetItem(previewTree, QStringList("C:"));
    QTreeWidgetItem *osdk = new QTreeWidgetItem(root, QStringList("OSDK"));
    QTreeWidgetItem *src  = new QTreeWidgetItem(osdk, QStringList("srcpy03"));
    src->addChild(new QTreeWidgetItem(QStringList("burger_cam.yaml")));
    src->addChild(new QTreeWidgetItem(QStringList("burger_cam_copy.yaml")));
    osdk->addChild(new QTreeWidgetItem(QStringList("vcompat.dll")));
    previewTree->addTopLevelItem(root);
    previewTree->expandAll();
}

void OpenFilePage::onStorageFolderSelected(int id)
{
    qDebug() << "Storage folder selected index:" << id;
    commentValue->setText(workspaceList->item(id, 3)->text());
}

void OpenFilePage::onRobotFolderSelected(int id)
{
    qDebug() << "Robot controller selected index:" << id;
}

void OpenFilePage::onLoadClicked()
{
    qDebug() << "LOAD button clicked";
}
