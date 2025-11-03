#include "BackupPage.h"

#include <QFile>
#include <QFileDialog>

#include "ui_BackupPage.h"

BackupPage::BackupPage(QWidget *parent) : QWidget(parent), ui(new Ui::BackupPage)
{
    ui->setupUi(this);
    ui->ConfirmBtn->setEnabled(false);

    selectcontrollerWidget = new selectcontroller(this);
    ui->controllerWidget->setLayout(new QVBoxLayout());
    ui->controllerWidget->layout()->addWidget(selectcontrollerWidget);

    connect(ui->refreshBtn, &QPushButton::clicked, this, [=]() {
        selectcontrollerWidget->getControllerState();
    });
    connect(ui->BrowseBtn, &QPushButton::clicked, this, &BackupPage::importAnySpace);
    connect(ui->ConfirmBtn, &QPushButton::clicked, this, &BackupPage::confirmSelection);
    connect(ui->CancelBtn, &QPushButton::clicked, this, [this]() { this->window()->close(); });

    connect(selectcontrollerWidget, &selectcontroller::controllerSelectionChanged, this, [this]() {
        updateConfirmState();
    });
}

BackupPage::~BackupPage() = default;

void BackupPage::confirmSelection()
{
    qDebug() << "selected : " << selectedBackupDir;

    for (auto cur : selectedControllerList) {
        qDebug() << "selected : " << cur;
    }
    selectedControllerList = selectcontrollerWidget->getSelectedControllers();
    qDebug() << "confirm clicked";
}

void BackupPage::importAnySpace()
{
    QString dirName = QFileDialog::getExistingDirectory(
            this,                // 부모 위젯 포인터 (nullptr 대신 this 권장)
            "Select Workspace",  // 대화상자 제목
            QDir::homePath(),    // 초기 디렉토리 경로
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks  // 폴더만 표시
    );

    if (dirName.isEmpty())  // 사용자가 취소한 경우
        return;

    selectedBackupDir = dirName;
    ui->selectedDir->setText(selectedBackupDir);
}

void BackupPage::updateConfirmState()
{
    bool hasControllers = !selectcontrollerWidget->getSelectedControllers().isEmpty();
    bool hasDirectory   = !selectedBackupDir.isEmpty();
    ui->ConfirmBtn->setEnabled(hasControllers && hasDirectory);
}
