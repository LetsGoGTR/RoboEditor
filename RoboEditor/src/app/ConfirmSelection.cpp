#include "ConfirmSelection.h"

#include <QTableView>

#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "ApplyPage.h"
#include "ControllerManager.h"
#include "ui_ConfirmSelection.h"

ConfirmSelection::ConfirmSelection(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfirmSelection),
    applyPage(nullptr)
{
    ui->setupUi(this);
    QObject::connect(
            ui->requestApplyBtn, &QPushButton::clicked, this, &ConfirmSelection::requestApply);
}

void ConfirmSelection::setApplyPage(ApplyPage *page)
{
    applyPage = page;

    if (!applyPage) {
        qWarning() << "ApplyPage is null!";
        return;
    }

    if (ui->selectedBackup) {
        ui->selectedBackup->setText(applyPage->selectedBackupDir);
    }

    QTableView         *table = new QTableView(this);
    QStandardItemModel *model = new QStandardItemModel(this);

    model->setColumnCount(2);
    model->setHorizontalHeaderLabels({"Index", "Serial Number"});

    QList<QString> selectedController = applyPage->selectedControllerList;
    for (int i = 0; i < selectedController.size(); ++i) {
        QList<QStandardItem *> row;
        row << new QStandardItem(QString::number(i + 1));
        row << new QStandardItem(selectedController[i]);
        model->appendRow(row);
    }

    table->setModel(model);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QHeaderView *header = table->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->resizeSection(0, 100);

    QVBoxLayout *layout = new QVBoxLayout(ui->controllerTable);
    layout->addWidget(table);
    layout->setContentsMargins(0, 0, 0, 0);
}

void ConfirmSelection::requestApply()
{
    if (!applyPage) {
        qWarning() << "ApplyPage is null!";
        return;
    }

    // 데이터 준비
    QString        backupDir   = applyPage->selectedBackupDir;
    QList<QString> controllers = applyPage->selectedControllerList;

    if (controllers.isEmpty()) {
        QMessageBox::warning(this, "Error", "No controllers selected!");
        return;
    }
    if (backupDir.isEmpty()) {
        QMessageBox::warning(this, "Error", "No backup folder selected!");
        return;
    }

    // Apply 요청을 큐에 추가
    emit applyRequested();

    QMessageBox::information(this,
                             "적용 시작",
                             QString("선택된 %1개 제어기에 백업을 적용합니다.\n백업 파일: %2")
                                     .arg(controllers.size())
                                     .arg(backupDir));

    // 다이얼로그 닫기
    accept();
}

ConfirmSelection::~ConfirmSelection()
{
    delete ui;
}
