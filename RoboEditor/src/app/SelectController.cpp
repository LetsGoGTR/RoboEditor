#include "SelectController.h"

#include <QTableWidget>
#include <QDebug>
#include <QPainter>

selectcontroller::selectcontroller(QWidget *parent) :
    QWidget(parent),
    model(nullptr),
    table(nullptr)
{
    setupUI();
    getControllerState();

    // ControllerManager 시그널 연결
    ControllerManager *manager = ControllerManager::instance();
    connect(manager, &ControllerManager::controllerListChanged, 
            this, &selectcontroller::onControllerListChanged);
    connect(manager, &ControllerManager::controllerStateUpdated,
            this, &selectcontroller::onControllerStateUpdated);
}

//ControllerManager로부터 제어기의 정보를 받아오는 함수
void selectcontroller::getControllerState()
{
    qDebug() << "refreshed";
    controllerState.clear();

    // ControllerManager에서 실제 제어기 목록 가져오기
    ControllerManager *manager = ControllerManager::instance();
    QList<ControllerInfo> controllers = manager->getControllers();

    // QList를 QVector로 변환
    for (const auto &info : controllers) {
        controllerState.push_back(info);
    }

    // 상태 업데이트 요청
    manager->updateControllersStates();
    
    updateTable();  // 테이블 업데이트
}

void selectcontroller::setupUI()
{
    model = new QStandardItemModel(0, 3, this);
    model->setHorizontalHeaderLabels({"Serial Number", "State", "Select"});

    table = new QTableView(this);
    table->setModel(model);

    QHeaderView *header = table->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->resizeSection(2, 40);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->addWidget(table);
    setLayout(lay);
}
void selectcontroller::updateTable()
{
    model->removeRows(0, model->rowCount());  // 기존 행 제거
    checkBoxes.clear();

    for (const ControllerInfo &info : controllerState) {
        QList<QStandardItem *> rowItems;

        // Serial Number
        QStandardItem *snItem = new QStandardItem(info.serialNumber);
        snItem->setEditable(false);
        rowItems.append(snItem);

        // State 결정
        // 0: Offline (연결 안됨)
        // 1: Running (연결 + 실행 중, data: true)
        // 2: Online (연결 + 실행 안함, data: false)
        int state;
        if (!info.isConnected) {
            state = 0;  // Offline
        } else if (info.isRunning) {
            state = 1;  // Running (data: true)
        } else {
            state = 2;  // Online (data: false)
        }

        // State 표시 (switch case 사용)
        QString stateStr;
        QColor  statusColor;
        
        switch (state) {
        case 0:
            stateStr    = "Offline";
            statusColor = QColor(128, 128, 128);  // 회색
            break;
        case 1:
            stateStr    = "Running";
            statusColor = QColor(255, 0, 0);  // 빨강
            break;
        case 2:
            stateStr    = "Online";
            statusColor = QColor(0, 255, 0);  // 초록
            break;
        default:
            stateStr    = "Unknown";
            statusColor = QColor(128, 128, 128);  // 회색
            break;
        }

        QStandardItem *stateItem = new QStandardItem(stateStr);
        stateItem->setEditable(false);

        // 원형 아이콘 생성
        QPixmap  pixmap(16, 16);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(statusColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(2, 2, 12, 12);

        stateItem->setIcon(QIcon(pixmap));
        rowItems.append(stateItem);

        // Select (placeholder)
        QStandardItem *selectItem = new QStandardItem();
        selectItem->setEditable(false);
        rowItems.append(selectItem);

        model->appendRow(rowItems);

        // CheckBox 추가
        int          row       = model->rowCount() - 1;
        QCheckBox   *checkBox  = new QCheckBox();
        QWidget     *container = new QWidget();
        QHBoxLayout *layout    = new QHBoxLayout(container);
        layout->addWidget(checkBox);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);

        table->setIndexWidget(model->index(row, 2), container);
        checkBoxes.append(checkBox);

        connect(checkBox, &QCheckBox::stateChanged, this, [this]() {
            emit controllerSelectionChanged();
        });
    }
}

QVector<QString> selectcontroller::getSelectedControllers() const
{
    QVector<QString> selected;
    for (int i = 0; i < checkBoxes.size() && i < controllerState.size(); ++i) {
        if (checkBoxes[i]->isChecked()) {
            selected.append(controllerState[i].serialNumber);
        }
    }
    return selected;
}

void selectcontroller::onControllerListChanged()
{
    qDebug() << "[SelectController] Controller list changed, refreshing...";
    getControllerState();
}

void selectcontroller::onControllerStateUpdated(const QString &serialNumber, 
                                                bool isConnected, 
                                                bool isRunning)
{
    qDebug() << "[SelectController] State updated:" << serialNumber 
             << "Connected:" << isConnected << "Running:" << isRunning;
    
    // controllerState에서 해당 제어기 찾아서 업데이트
    for (auto &info : controllerState) {
        if (info.serialNumber == serialNumber) {
            info.isConnected = isConnected;
            info.isRunning = isRunning;
            break;
        }
    }
    
    // 테이블 업데이트
    updateTable();
}

selectcontroller::~selectcontroller() {}
