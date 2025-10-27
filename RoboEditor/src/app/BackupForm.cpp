#include "BackupForm.h"

#include <QButtonGroup>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QRadioButton>
#include <QVBoxLayout>

BackupForm::BackupForm(QWidget *parent) : QWidget{parent}
{
    setupUi();
    populateDummyData();

    // Signal Connection
    connect(backupCreate, &QPushButton::clicked, this, &BackupForm::onBackupCreateClicked);

    connect(backupDelete, &QPushButton::clicked, this, &BackupForm::onBackupDeleteClicked);

    connect(radioGroup, &QButtonGroup::idClicked, this, &BackupForm::onRadioSelected);
}

void BackupForm::setupUi()
{
    // Backup List
    backupTableTitle = new QLabel("backup folder directory", this);

    backupList = new QTableWidget(this);
    backupList->setColumnCount(5);
    backupList->setHorizontalHeaderLabels({"index", "date", "size", "comment", ""});
    backupList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    backupList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    backupList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
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
    leftLayout = new QVBoxLayout();
    leftLayout->addWidget(backupTableTitle);
    leftLayout->addWidget(backupList);
    leftLayout->addWidget(backupDelete);

    // WorkDir Backup
    backupActionTitle = new QLabel("작업 폴더 백업하기", this);

    QGroupBox *workGroup = new QGroupBox("작업 폴더 정보", this);
    sizeLabel            = new QLabel("size : 1.5 GB", this);
    commentLabel         = new QLabel("comment", this);
    backupComment        = new QTextEdit(this);
    backupCreate         = new QPushButton("store", this);

    QVBoxLayout *workLayout = new QVBoxLayout();
    workLayout->addWidget(sizeLabel);
    workLayout->setContentsMargins(8, 8, 8, 8);
    workGroup->setLayout(workLayout);

    // ✅ 오른쪽 레이아웃 비율 1:2로 조정
    QVBoxLayout *rightTopLayout = new QVBoxLayout();
    rightTopLayout->addWidget(workGroup);
    rightTopLayout->setStretch(0, 1);  // 상단 영역

    QVBoxLayout *rightBottomLayout = new QVBoxLayout();
    rightBottomLayout->addWidget(commentLabel);
    rightBottomLayout->addWidget(backupComment);
    rightBottomLayout->addWidget(backupCreate);
    rightBottomLayout->setStretch(1, 2);  // comment 영역 비중 2

    rightLayout = new QVBoxLayout();
    rightLayout->addWidget(backupActionTitle);
    rightLayout->addLayout(rightTopLayout);
    rightLayout->addLayout(rightBottomLayout);
    rightLayout->setStretch(1, 1);
    rightLayout->setStretch(2, 2);
    rightLayout->setContentsMargins(5, 5, 5, 5);

    // Overall Layout
    contentLayout = new QHBoxLayout();
    contentLayout->addLayout(leftLayout, 2);
    contentLayout->addLayout(rightLayout, 1);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(contentLayout);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    setLayout(mainLayout);

    backupDelete->setEnabled(false);
}

// Dummy Data for test
void BackupForm::populateDummyData()
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
void BackupForm::onBackupCreateClicked()
{
    QString comment = backupComment->toPlainText();
    qDebug() << "백업 생성 요청:" << comment;
}

void BackupForm::onBackupDeleteClicked()
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

void BackupForm::onRadioSelected(int id)
{
    if (id >= 0) {
        backupDelete->setEnabled(true);
        qDebug() << "선택된 백업 인덱스:" << id;
    }
}

BackupForm::~BackupForm() = default;
