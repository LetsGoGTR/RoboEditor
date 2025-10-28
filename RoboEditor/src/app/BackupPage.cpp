#include "BackupPage.h"

#include <QButtonGroup>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

BackupPage::BackupPage(QWidget *parent) : QWidget(parent)
{
    setupUi();
    populateDummyData();

    // Signal Connection
    connect(backupCreate, &QPushButton::clicked, this, &BackupPage::onBackupCreateClicked);
    connect(backupDelete, &QPushButton::clicked, this, &BackupPage::onBackupDeleteClicked);
    connect(radioGroup, &QButtonGroup::idClicked, this, &BackupPage::onRadioSelected);

    // 디렉토리 선택 시 동작 연결
    connect(dirTree, &QTreeView::doubleClicked, this, [=](const QModelIndex &index) {
        QString path = dirModel->filePath(index);
        if (QFileInfo(path).isDir()) {
            dirTree->setRootIndex(index);  // 실제 탐색 위치를 이 폴더로 전환
            qDebug() << "📂 Entered folder:" << path;
        }
    });

    connect(backupCreate, &QPushButton::clicked, this, [=] {
        emit uiBackupClicked(backupComment->toPlainText());
    });
}

void BackupPage::setupUi()
{
    // Directory Explorer
    dirTitle = new QLabel("file directory", this);

    dirModel = new QFileSystemModel(this);
    dirModel->setRootPath(QDir::homePath());
    dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);

    dirTree = new QTreeView(this);
    dirTree->setModel(dirModel);
    dirTree->setRootIndex(dirModel->index(QDir::homePath()));
    dirTree->setHeaderHidden(true);
    dirTree->setMinimumWidth(250);  // 좌측 탐색기 폭 확보
    dirTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    dirLayout = new QVBoxLayout();
    dirLayout->addWidget(dirTitle);
    dirLayout->addWidget(dirTree);

    // Backup List
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

    // Radio Btn Group
    radioGroup = new QButtonGroup(this);
    radioGroup->setExclusive(true);

    // Left Layout
    workspaceLayout = new QVBoxLayout();
    workspaceLayout->addWidget(backupTableTitle);
    workspaceLayout->addWidget(backupList);
    workspaceLayout->addWidget(backupDelete);

    // WorkDir Backup
    backupActionTitle = new QLabel("작업 폴더 백업하기", this);
    sizeLabel         = new QLabel("size : 1.5 GB", this);
    commentLabel      = new QLabel("comment", this);
    backupComment     = new QTextEdit(this);
    backupCreate      = new QPushButton("Backup", this);

    QVBoxLayout *workLayout = new QVBoxLayout();
    workLayout->addWidget(sizeLabel);
    workLayout->setContentsMargins(8, 8, 8, 8);

    // ✅ 오른쪽 레이아웃 비율 1:2로 조정
    QVBoxLayout *rightTopLayout = new QVBoxLayout();
    rightTopLayout->addWidget(backupActionTitle);
    rightTopLayout->addWidget(sizeLabel);
    rightTopLayout->setStretch(1, 1);  // 상단 영역

    QVBoxLayout *rightBottomLayout = new QVBoxLayout();
    rightBottomLayout->addWidget(commentLabel);
    rightBottomLayout->addWidget(backupComment);
    rightBottomLayout->addWidget(backupCreate);
    rightBottomLayout->setStretch(1, 2);  // comment 영역 비중 2

    infoLayout = new QVBoxLayout();
    infoLayout->addWidget(backupActionTitle);
    infoLayout->addLayout(rightTopLayout);
    infoLayout->addLayout(rightBottomLayout);
    infoLayout->setStretch(1, 1);
    infoLayout->setStretch(2, 2);
    infoLayout->setContentsMargins(5, 5, 5, 5);

    // Overall Layout
    contentLayout = new QHBoxLayout();
    contentLayout->addLayout(dirLayout, 1);
    contentLayout->addLayout(workspaceLayout, 2);
    contentLayout->addLayout(infoLayout, 1);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(contentLayout);
    // mainLayout->setContentsMargins(10, 10, 10, 10);

    setLayout(mainLayout);

    backupDelete->setEnabled(false);
}

// Dummy Data for test
void BackupPage::populateDummyData()
{
    for (int i = 0; i < 5; ++i) {
        backupList->insertRow(i);
        backupList->setItem(i, 0, new QTableWidgetItem(QString::number(5 - i)));
        backupList->setItem(
                i, 1, new QTableWidgetItem(QString("2025.10.%1 10:2%2:27").arg(11 + i).arg(i)));
        backupList->setItem(i, 2, new QTableWidgetItem("1.4 GB"));
        backupList->setItem(i,
                            3,
                            new QTableWidgetItem(
                                    QString("제어기 %1번 로봇 %2 @@ 설정 변경").arg(i).arg(i + 1)));

        QWidget      *radioWidget = new QWidget();
        QHBoxLayout  *radioLayout = new QHBoxLayout(radioWidget);
        QRadioButton *radio       = new QRadioButton(radioWidget);

        // ✅ 여백 추가
        radioLayout->setContentsMargins(6, 0, 6, 0);
        radioLayout->addWidget(radio);
        radioLayout->setAlignment(Qt::AlignCenter);

        backupList->setCellWidget(i, 4, radioWidget);
        radioGroup->addButton(radio, i);
    }
}

// slots

// backup 생성 slot
void BackupPage::onBackupCreateClicked()
{
    QString comment = backupComment->toPlainText();
    qDebug() << "백업 생성 요청:" << comment;
}

// backup 삭제 slot
void BackupPage::onBackupDeleteClicked()
{
    int selectedId = radioGroup->checkedId();

    if (selectedId == -1) {
        qDebug() << "⚠️ 삭제할 백업이 선택되지 않았습니다.";
        backupDelete->setEnabled(false);
        return;
    }

    QAbstractButton *selectedButton = radioGroup->button(selectedId);
    if (!selectedButton) {
        qDebug() << "⚠️ 선택된 버튼 객체를 찾을 수 없습니다.";
        backupDelete->setEnabled(false);
        return;
    }

    // 선택된 라디오버튼이 위치한 행(row) 찾기
    int targetRow = -1;
    for (int row = 0; row < backupList->rowCount(); ++row) {
        QWidget *cellWidget = backupList->cellWidget(row, 4);
        if (cellWidget && cellWidget->findChild<QRadioButton *>() == selectedButton) {
            targetRow = row;
            break;
        }
    }

    if (targetRow == -1) {
        qDebug() << "⚠️ 선택된 버튼에 해당하는 행을 찾지 못했습니다.";
        backupDelete->setEnabled(false);
        return;
    }

    // 삭제 대상 정보 출력
    QString index   = backupList->item(targetRow, 0)->text();
    QString date    = backupList->item(targetRow, 1)->text();
    QString comment = backupList->item(targetRow, 3)->text();
    qDebug() << QString("🗑 백업 삭제 요청: index=%1, date=%2, comment=%3")
                        .arg(index, date, comment);

    // 라디오버튼 그룹에서 제거
    radioGroup->removeButton(selectedButton);

    // 테이블에서 행 제거
    backupList->removeRow(targetRow);

    // 인덱스 재정렬
    for (int i = 0; i < backupList->rowCount(); ++i)
        backupList->setItem(
                i, 0, new QTableWidgetItem(QString::number(backupList->rowCount() - i)));

    // ✅ 삭제 후 버튼 비활성화
    backupDelete->setEnabled(false);

    qDebug() << "✅ 삭제 완료. 남은 행 수:" << backupList->rowCount();
}

// backup 선택 slot
void BackupPage::onRadioSelected(int id)
{
    if (id >= 0) {
        backupDelete->setEnabled(true);
        qDebug() << "선택된 백업 인덱스:" << id;
    }
}

// Directory 선택 slot
void BackupPage::onDirectorySelected(const QModelIndex &index)
{
    QString path = dirModel->filePath(index);
    qint64  size = 0;

    QDir          dir(path);
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : files)
        size += info.size();

    double sizeGB = size / (1024.0 * 1024 * 1024);
    sizeLabel->setText(
            QString("선택된 폴더: %1\nsize: %2 GB").arg(path).arg(QString::number(sizeGB, 'f', 2)));

    qDebug() << "폴더 선택됨:" << path;
}

BackupPage::~BackupPage() = default;
