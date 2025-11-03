#include "SelectController.h"

#include <QDebug>

selectcontroller::selectcontroller(QWidget *parent) :
    QWidget(parent),
    model(nullptr),
    table(nullptr)
{
    setupUI();
    getControllerState();
}

//로컬 PC drogon 서버로부터 제어기의 SN, State를 받아오는 함수
void selectcontroller::getControllerState()
{
    qDebug() << "refreshed";
    controllerState.clear();

    // 테스트용 더미 데이터
    QByteArray dummyResponse = R"(
    {
        "status": "ok",
        "data": [
            { "serial_number": "LSI-CTRL-001", "state": 1 },
            { "serial_number": "LSI-CTRL-002", "state": 0 },
            { "serial_number": "LSI-CTRL-003", "state": 1 },
            { "serial_number": "LSI-CTRL-004", "state": 2 }
        ]
    }
    )";

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest        request(QUrl("http://192.xxx.x.xx/api/v1/controllers"));

    connect(manager,
            &QNetworkAccessManager::finished,
            this,
            [this, manager, dummyResponse](QNetworkReply *reply) {  // dummyResponse 캡처
                QByteArray response;

                // 네트워크 에러 시 더미 데이터 사용
                if (reply->error() != QNetworkReply::NoError) {
                    qWarning() << "Network error, using dummy data:" << reply->errorString();
                    response = dummyResponse;  // 더미 데이터 사용
                } else {
                    response = reply->readAll();  // 실제 응답 사용
                }

                // JSON 파싱 (더미든 실제든 동일하게 처리)
                QJsonDocument jsonDoc = QJsonDocument::fromJson(response);

                if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                    QJsonObject jsonObj = jsonDoc.object();

                    if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
                        QJsonArray controllers = jsonObj["data"].toArray();

                        // 각 제어기 정보 추출
                        for (const QJsonValue &value : controllers) {
                            if (!value.isObject())
                                continue;

                            QJsonObject ctrl = value.toObject();
                            cstate      cs;
                            cs.SN    = ctrl["serial_number"].toString();
                            cs.state = ctrl["state"].toInt();

                            if (!cs.SN.isEmpty()) {
                                controllerState.push_back(cs);
                            }
                        }

                        updateTable();  // 데이터 수신 후 테이블 업데이트
                    }
                }

                reply->deleteLater();
                manager->deleteLater();
            });

    manager->get(request);
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

    for (const cstate &cs : controllerState) {
        QList<QStandardItem *> rowItems;

        // Serial Number
        QStandardItem *snItem = new QStandardItem(cs.SN);
        snItem->setEditable(false);
        rowItems.append(snItem);

        // State
        QString stateStr;
        switch (cs.state) {
        case 0:
            stateStr = "Offline";
            break;
        case 1:
            stateStr = "Online";
            break;
        case 2:
            stateStr = "Error";
            break;
        default:
            stateStr = "Unknown";
            break;
        }
        QStandardItem *stateItem = new QStandardItem(stateStr);
        stateItem->setEditable(false);
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
    }
}
QVector<QString> selectcontroller::getSelectedControllers() const
{
    QVector<QString> selected;
    for (int i = 0; i < checkBoxes.size() && i < controllerState.size(); ++i) {
        if (checkBoxes[i]->isChecked()) {
            selected.append(controllerState[i].SN);
        }
    }
    return selected;
}
selectcontroller::~selectcontroller() {}
