#include "SelectStorage.h"

#include <QButtonGroup>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QHeaderView>
#include <QRadioButton>

#include "ApplyPage.h"

selectTableWidget::selectTableWidget(QWidget *parent) : QWidget(parent)
{
    resize(1000, 600);
    rootPath    = "C:/backup";
    currentPath = rootPath;

    QSplitter   *splitter = new QSplitter(this);
    QVBoxLayout *layout   = new QVBoxLayout(this);
    layout->addWidget(splitter);
    setLayout(layout);

    // 왼쪽 리스트
    leftList  = new QListView(splitter);
    leftModel = new QFileSystemModel(this);
    leftModel->setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    leftModel->setRootPath(rootPath);
    leftList->setModel(leftModel);
    leftList->setRootIndex(leftModel->index(rootPath));
    leftList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 오른쪽 테이블
    rightTable = new QTableView(splitter);
    rightModel = new QStandardItemModel(this);
    rightModel->setHorizontalHeaderLabels({"Folder Name", "Last Modified", "Select"});
    rightTable->setModel(rightModel);
    rightTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QHeaderView *header = rightTable->horizontalHeader();

    // ✅ 0열(선택): 고정 크기
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->resizeSection(2, 50);

    header->setStretchLastSection(false);
    splitter->setStretchFactor(0, 2);  // 왼쪽 영역 비율 2
    splitter->setStretchFactor(1, 4);  // 오른쪽 영역 비율 3

    radioGroup = new QButtonGroup(this);
    radioGroup->setExclusive(true);

    connect(leftList, &QListView::clicked, this, &selectTableWidget::onLeftListClicked);
    connect(radioGroup, &QButtonGroup::idClicked, this, &selectTableWidget::onRadioButtonChanged);

    rightModel->setRowCount(0);
}

void selectTableWidget::onLeftListClicked(const QModelIndex &index)
{
    QString path = leftModel->filePath(index);
    currentPath  = path;
    showSubFolders(path);
}

void selectTableWidget::showSubFolders(const QString &path)
{
    // ✅ 모델의 행을 제거하면 QTableView가 자동으로 IndexWidget 정리
    rightModel->removeRows(0, rightModel->rowCount());

    QDir          dir(path);
    QFileInfoList subFolders = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    int row = 0;
    for (const QFileInfo &info : subFolders) {
        QList<QStandardItem *> items;
        items << new QStandardItem(info.fileName());
        items << new QStandardItem(info.lastModified().toString("yyyy-MM-dd hh:mm"));
        items << new QStandardItem();
        rightModel->appendRow(items);

        QRadioButton *radio = new QRadioButton();
        radioGroup->addButton(radio, row);
        QWidget     *container = new QWidget();
        QHBoxLayout *layout    = new QHBoxLayout(container);
        layout->addWidget(radio);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);
        rightTable->setIndexWidget(rightModel->index(row, 2), container);
        row++;
    }

    if (!subFolders.isEmpty() && radioGroup->button(0)) {
        radioGroup->button(0)->setChecked(true);
    }

    if (subFolders.isEmpty()) {
        QList<QStandardItem *> rowItems;
        rowItems << new QStandardItem();
        rowItems << new QStandardItem("(하위 폴더 없음)");
        rowItems << new QStandardItem("-");
        rightModel->appendRow(rowItems);
    }
}
void selectTableWidget::onRadioButtonChanged(int id)
{
    QString folder = getSelectedFolder();
    emit    folderSelected(folder);
}

QString selectTableWidget::getSelectedFolder() const
{
    int checkedId = radioGroup->checkedId();
    qDebug() << checkedId;
    if (checkedId < 0) {
        return QString("");
    }

    QStandardItem *item = rightModel->item(checkedId, 0);  // Folder Name 열
    if (item) {
        return currentPath + "/" + item->text();
    }

    return QString();
}
void selectTableWidget::clearRadioSelection()
{
    QAbstractButton *checked = radioGroup->checkedButton();
    if (checked) {
        radioGroup->setExclusive(false);
        checked->setChecked(false);
        radioGroup->setExclusive(true);
    }
}
