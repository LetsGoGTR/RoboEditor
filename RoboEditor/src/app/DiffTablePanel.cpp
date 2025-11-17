#include "DiffTablePanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

DiffTablePanel::DiffTablePanel(QWidget *parent)
    : QWidget(parent)
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(8, 8, 8, 8);
    mainLayout_->setSpacing(6);

    // 상단 바
    topBar_ = new QWidget(this);
    auto *topLayout = new QHBoxLayout(topBar_);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(8);

    typeInfoLabel_ = new QLabel(this);
    typeInfoLabel_->setText(tr("Ready"));
    typeInfoLabel_->setVisible(false);
    topLayout->addWidget(typeInfoLabel_, 1);

    statLabel_ = new QLabel(this);
    topLayout->addWidget(statLabel_);

    filterCombo_ = new QComboBox(this);
    filterCombo_->addItems({tr("All"), tr("Changed"), tr("Added"), tr("Removed")});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DiffTablePanel::onFilterChanged);
    topLayout->addWidget(filterCombo_);

    showOnlyChangedCheck_ = new QCheckBox(tr("Changed only"), this);
    connect(showOnlyChangedCheck_, &QCheckBox::toggled,
            this, &DiffTablePanel::onShowOnlyChangedToggled);
    topLayout->addWidget(showOnlyChangedCheck_);

    mainLayout_->addWidget(topBar_);

    // 파일 비교용 테이블
    table_ = new QTableWidget(this);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setShowGrid(true);
    table_->verticalHeader()->setVisible(false);

    connect(table_, &QTableWidget::cellDoubleClicked,
            this, &DiffTablePanel::onTableCellActivated);

    mainLayout_->addWidget(table_);
}

void DiffTablePanel::setupFileDiffMode(const QString &fileType, const QString &leftHeader, const QString &rightHeader)
{
    isFolderMode_ = false;
    if (folderSplit_) folderSplit_->hide();
    table_->show();

    filterCombo_->setVisible(true);
    showOnlyChangedCheck_->setVisible(true);

    if (fileType == "yaml") {
        table_->setColumnCount(5);
        table_->setHorizontalHeaderLabels({"Line", "Path", leftHeader, rightHeader, "State"});
        table_->setColumnWidth(0, 50);
        table_->setColumnWidth(1, 120);
        table_->setColumnWidth(2, 150);
        table_->setColumnWidth(3, 150);
        table_->setColumnWidth(4, 100);
    } else {
        table_->setColumnCount(4);
        table_->setHorizontalHeaderLabels({"Line", leftHeader, rightHeader, "State"});
        table_->setColumnWidth(0, 50);
        table_->setColumnWidth(1, 150);
        table_->setColumnWidth(2, 150);
        table_->setColumnWidth(3, 100);
    }
    table_->horizontalHeader()->setStretchLastSection(true);
}

void DiffTablePanel::setDiffRows(const QList<DiffRow> &rows)
{
    allRows_ = rows;
    rebuildTable();
}

void DiffTablePanel::setupFolderDiffMode(const Json::Value &result, const QString &leftRootPath, const QString &rightRootPath)
{
    isFolderMode_ = true;
    table_->hide();

    filterCombo_->setVisible(false);
    showOnlyChangedCheck_->setVisible(false);

    // 폴더 UI 생성 (Lazy Loading)
    if (!folderSplit_) {
        folderSplit_ = new QSplitter(Qt::Horizontal, this);

        leftFolderTree_ = new QTreeWidget(folderSplit_);
        rightFolderTree_ = new QTreeWidget(folderSplit_);

        leftFolderTree_->setAlternatingRowColors(true);
        rightFolderTree_->setAlternatingRowColors(true);
        leftFolderTree_->setFrameShape(QFrame::NoFrame);
        rightFolderTree_->setFrameShape(QFrame::NoFrame);

        connect(leftFolderTree_, &QTreeWidget::itemDoubleClicked, this, &DiffTablePanel::onTreeItemDoubleClicked);
        connect(rightFolderTree_, &QTreeWidget::itemDoubleClicked, this, &DiffTablePanel::onTreeItemDoubleClicked);

        mainLayout_->insertWidget(1, folderSplit_);
        mainLayout_->setStretchFactor(folderSplit_, 1);
    }
    folderSplit_->show();
    leftFolderTree_->clear();
    rightFolderTree_->clear();

    // 헤더 설정
    QString lName = QFileInfo(leftRootPath).fileName();
    QString rName = QFileInfo(rightRootPath).fileName();
    if (lName.isEmpty()) lName = leftRootPath;
    if (rName.isEmpty()) rName = rightRootPath;

    leftFolderTree_->setHeaderLabels({lName, tr("Status"), tr("Type")});
    rightFolderTree_->setHeaderLabels({rName, tr("Status"), tr("Type")});

    // 통계 파싱
    const Json::Value &stats = result["statistics"];
    int added    = stats["added"].asInt();
    int removed  = stats["removed"].asInt();
    int modified = stats["modified"].asInt();
    int total    = stats["totalChanges"].asInt();

    if (statLabel_) {
        statLabel_->setText(QString("Folder Changes: %1 (+%2 -%3 ~%4)")
                                    .arg(total).arg(added).arg(removed).arg(modified));
    }

    // 트리 아이템 생성 로직
    const Json::Value &changes = result["changes"];
    QMap<QString, QList<Json::Value>> folderGroups;

    for (const auto &change : changes) {
        QString path = QString::fromStdString(change["path"].asString());
        QString folder = QFileInfo(path).dir().path();

        // "." 경로를 "(Root)"로 바꾸지 않고 빈 문자열로 처리하여 구분
        if (folder == ".") folder = "";

        folderGroups[folder].append(change);
    }

    QStringList folderKeys = folderGroups.keys();
    folderKeys.sort();

    for (const QString &folderPath : folderKeys) {
        const QList<Json::Value> &files = folderGroups[folderPath];

        // 폴더 경로가 비어있으면(최상위 파일들) 폴더 노드를 만들지 않고 invisibleRootItem 사용
        QTreeWidgetItem *lFolder = nullptr;
        QTreeWidgetItem *rFolder = nullptr;

        if (folderPath.isEmpty()) {
            // 최상위 루트에 직접 붙임
            lFolder = leftFolderTree_->invisibleRootItem();
            rFolder = rightFolderTree_->invisibleRootItem();
        } else {
            // 하위 폴더인 경우에만 폴더 노드 생성
            lFolder = new QTreeWidgetItem(leftFolderTree_);
            rFolder = new QTreeWidgetItem(rightFolderTree_);

            QString folderText = QString("📁 %1").arg(folderPath);
            QString countText = QString("%1 file(s)").arg(files.size());

            lFolder->setText(0, folderText); lFolder->setText(1, countText);
            rFolder->setText(0, folderText); rFolder->setText(1, countText);

            lFolder->setExpanded(true);
            rFolder->setExpanded(true);
        }

        // 파일 노드 생성
        for (const auto &change : files) {
            QString path = QString::fromStdString(change["path"].asString());
            QString type = QString::fromStdString(change["type"].asString());
            QString fileName = QFileInfo(path).fileName();
            QColor bgColor = getColorForDiffState(type);

            // Left (Added / Modified)
            if (type != "removed") {
                auto *item = new QTreeWidgetItem(lFolder);
                item->setText(0, fileName);
                item->setData(0, Qt::UserRole, leftRootPath + "/" + path);

                if (type == "added") {
                    item->setText(1, "Added");
                    item->setText(2, "+ ADDED");
                    item->setForeground(2, QBrush(QColor(22, 163, 74)));
                } else {
                    item->setText(1, "Modified");
                    item->setText(2, "● CHANGED");
                    item->setForeground(2, QBrush(QColor(184, 134, 11)));
                }
                for(int c=0; c<3; c++) item->setBackground(c, bgColor);
            }

            // Right (Removed / Modified)
            if (type != "added") {
                auto *item = new QTreeWidgetItem(rFolder);
                item->setText(0, fileName);
                item->setData(0, Qt::UserRole, rightRootPath + "/" + path);

                if (type == "removed") {
                    item->setText(1, "Removed");
                    item->setText(2, "− REMOVED");
                    item->setForeground(2, QBrush(QColor(220, 38, 38)));
                } else {
                    item->setText(1, "Modified");
                    item->setText(2, "● CHANGED");
                    item->setForeground(2, QBrush(QColor(184, 134, 11)));
                }
                for(int c=0; c<3; c++) item->setBackground(c, bgColor);
            }
        }
    }

    // 컬럼 너비 자동 조절
    for(int i=0; i<3; i++) {
        leftFolderTree_->resizeColumnToContents(i);
        rightFolderTree_->resizeColumnToContents(i);
    }
}

void DiffTablePanel::clearAll()
{
    allRows_.clear();
    table_->setRowCount(0);
    if (leftFolderTree_) leftFolderTree_->clear();
    if (rightFolderTree_) rightFolderTree_->clear();
    if (statLabel_) statLabel_->clear();
    if (typeInfoLabel_) typeInfoLabel_->setText(tr(""));
}

void DiffTablePanel::setFileTypeInfo(const QString &leftType, const QString &rightType, bool compatible, const QString &message)
{
    Q_UNUSED(leftType)
    Q_UNUSED(rightType)
    Q_UNUSED(compatible)
    Q_UNUSED(message)
}

void DiffTablePanel::onTableCellActivated(int row, int /*column*/)
{
    if (row >= 0 && row < allRows_.size()) {
        emit rowActivated(allRows_.at(row));
    }
}

void DiffTablePanel::onTreeItemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    if (item->childCount() == 0) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty()) {
            emit fileDoubleClicked(path);
        }
    }
}

void DiffTablePanel::onFilterChanged(int index)
{
    switch (index) {
    case 1: currentFilter_ = "Changed"; break;
    case 2: currentFilter_ = "Added"; break;
    case 3: currentFilter_ = "Removed"; break;
    default: currentFilter_ = "All"; break;
    }
    rebuildTable();
}

void DiffTablePanel::onShowOnlyChangedToggled(bool checked)
{
    Q_UNUSED(checked);
    rebuildTable();
}

bool DiffTablePanel::matchFilter(const DiffRow &row) const
{
    const bool isChangedLike = (row.state != "SAME");

    if (showOnlyChangedCheck_->isChecked() && !isChangedLike)
        return false;

    // 콤보박스 필터
    if (currentFilter_ == "All")
        return true;

    if (currentFilter_ == "Changed")
        return isChangedLike;

    if (currentFilter_ == "Added")
        return (row.state == "ADDED");

    if (currentFilter_ == "Removed")
        return (row.state == "REMOVED");

    return true;
}

void DiffTablePanel::rebuildTable()
{
    if (isFolderMode_) return;

    table_->setRowCount(0);

    int changeCount = 0, added = 0, removed = 0, modified = 0;
    bool isYaml = (table_->columnCount() == 5);

    for (const auto &r : allRows_) {
        if (!matchFilter(r)) continue;

        int row = table_->rowCount();
        table_->insertRow(row);

        QColor bgColor = getColorForDiffState(r.state.toLower());

        if (r.state == "ADDED") added++;
        else if (r.state == "REMOVED") removed++;
        else if (r.state == "CHANGED") modified++;
        if (r.state != "SAME") changeCount++;

        // DiffRow 멤버 이름 변경 반영 (row.leftValue -> row.target 등)
        auto createItem = [&](const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setBackground(bgColor);
            return item;
        };

        QTableWidgetItem *itemLine = isYaml ? createItem(r.line > 0 ? QString::number(r.line) : "") : nullptr;
        QTableWidgetItem *itemKey = createItem(r.key);
        QTableWidgetItem *itemLeft = createItem(r.target); // target = Left Value
        QTableWidgetItem *itemRight = createItem(r.origin); // origin = Right Value
        QTableWidgetItem *itemState = createItem(r.state);

        if(itemLine) itemLine->setTextAlignment(Qt::AlignCenter);
        itemState->setTextAlignment(Qt::AlignCenter);

        if (isYaml) {
            table_->setItem(row, 0, itemLine);
            table_->setItem(row, 1, itemKey);
            table_->setItem(row, 2, itemLeft);
            table_->setItem(row, 3, itemRight);
            table_->setItem(row, 4, itemState);
        } else {
            table_->setItem(row, 0, itemKey);
            table_->setItem(row, 1, itemLeft);
            table_->setItem(row, 2, itemRight);
            table_->setItem(row, 3, itemState);
        }
    }

    if (statLabel_) {
        statLabel_->setText(QString("Changes: %1 (+%2 -%3 ~%4)")
                                    .arg(changeCount).arg(added).arg(removed).arg(modified));
    }
}

QColor DiffTablePanel::getColorForDiffState(const QString &state) const
{
    QString s = state.toLower();
    if (s == "added") return QColor(220, 252, 231);
    if (s == "removed") return QColor(254, 226, 226);
    if (s == "changed" || s == "modified") return QColor(255, 250, 205);
    if (s == "error" || s == "mismatch") return QColor(255, 237, 213);
    return QColor(255, 255, 255);
}
