#include "ControllerManager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "ControllerSetting.h"
#include "FileCompressor.h"

static ControllerManager *getinstance = nullptr;

ControllerManager::ControllerManager(QObject *parent) : QObject(parent)
{
    QDir dir("C:/backup/config");
    if (!dir.exists())
        dir.mkpath("C:/backup/config");
    configFilePath_ = "C:/backup/config/controllers.json";
    loadFromFile();
}

ControllerManager::~ControllerManager()
{
    // ApiClient들 정리
    for (auto it = apiClients_.begin(); it != apiClients_.end(); ++it) {
        if (it.value()) {
            it.value()->deleteLater();
        }
    }
    apiClients_.clear();

    saveToFile();
}

ControllerManager *ControllerManager::instance()
{
    static QMutex mutex;
    QMutexLocker  locker(&mutex);
    if (!getinstance)
        getinstance = new ControllerManager();
    return getinstance;
}

void ControllerManager::registerController()
{
    ControllerSetting dialog;

    if (dialog.exec() == QDialog::Accepted) {
        ControllerInfo newConInfo = dialog.getControllerInfo();

        // 동일한 SN이 있으면 에러
        if (isDuplicatedSN(newConInfo.serialNumber)) {
            QMessageBox::warning(nullptr, "등록 실패", "이미 동일한 제어기가 등록되어 있습니다.");
            return;
        }

        // SN 폴더 자동 생성
        QString folderPath = "C:/backup/" + newConInfo.serialNumber;
        QDir    dir;

        if (!dir.exists(folderPath)) {
            if (dir.mkpath(folderPath)) {
                qDebug() << "폴더 생성 완료:" << folderPath;
            } else {
                QMessageBox::warning(nullptr,
                                     "폴더 생성 실패",
                                     "작업 경로에 폴더를 생성할 수 없습니다:\n" + folderPath);
                return;
            }
        }

        // 초기 상태 설정
        newConInfo.isConnected = false;
        newConInfo.isRunning   = false;

        controllers_.append(newConInfo);

        qDebug() << "SN:" << newConInfo.serialNumber;
        qDebug() << "IP:" << newConInfo.ip;
        qDebug() << "SFTP:" << newConInfo.sftpPort;
        qDebug() << "API:" << newConInfo.apiPort;
        qDebug() << "Username:" << newConInfo.username;

        saveToFile();

        setupApiClient(newConInfo.serialNumber);

        emit controllerListChanged();
    }
}

void ControllerManager::removeController(int index)
{
    if (index >= 0 && index < controllers_.size()) {
        QString sn = controllers_[index].serialNumber;

        // ApiClient 정리
        cleanupApiClient(sn);

        controllers_.removeAt(index);
        saveToFile();
        emit controllerListChanged();

        qDebug() << "controllers[" << index << "] (" << sn << ") removed";
    }
}

void ControllerManager::removeController(const ControllerInfo *curCon)
{
    QMutexLocker locker(&mutex_);

    for (int idx = 0; idx < controllers_.size(); ++idx) {
        if (controllers_[idx].serialNumber == curCon->serialNumber) {
            QString sn = controllers_[idx].serialNumber;

            // ApiClient 정리
            locker.unlock();
            cleanupApiClient(sn);
            locker.relock();

            controllers_.removeAt(idx);
            qDebug() << "controllers[" << idx << "] removed";

            saveToFile();
            emit controllerListChanged();
            break;
        }
    }
}

void ControllerManager::updateInfo(const ControllerInfo &newInfo)
{
    ControllerSetting dialog;
    dialog.setControllerInfo(newInfo);

    if (dialog.exec() == QDialog::Accepted) {
        ControllerInfo updated = dialog.getControllerInfo();

        QString serialNumber;

        {
            QMutexLocker locker(&mutex_);
            for (auto &c : controllers_) {
                if (c.serialNumber == newInfo.serialNumber) {
                    serialNumber = c.serialNumber;

                    c.ip       = updated.ip;
                    c.username = updated.username;
                    c.sftpPort = updated.sftpPort;
                    c.apiPort  = updated.apiPort;

                    qDebug() << "[updateInfo] Controller updated:";
                    qDebug() << "SN:" << c.serialNumber;
                    qDebug() << "IP:" << c.ip;
                    qDebug() << "Username:" << c.username;
                    qDebug() << "SFTP:" << c.sftpPort;
                    qDebug() << "API:" << c.apiPort;

                    break;
                }
            }
        }

        saveToFile();

        cleanupApiClient(serialNumber);
        setupApiClient(serialNumber);

        emit controllerListChanged();
    }
}

void ControllerManager::setupApiClient(const QString &serialNumber)
{
    // 이미 존재하면 리턴
    {
        QMutexLocker locker(&mutex_);
        if (apiClients_.contains(serialNumber)) {
            qWarning() << "[setupApiClient]" << serialNumber << "already has ApiClient";
            return;
        }
    }

    // 제어기 정보 가져오기
    ControllerInfo info = getController(serialNumber);
    if (info.serialNumber.isEmpty()) {
        qWarning() << "[setupApiClient] Controller not found:" << serialNumber;
        return;
    }

    QString    baseUrl = QString("%1:%2").arg(info.ip).arg(info.apiPort);
    ApiClient *client  = new ApiClient(baseUrl, this);

    // serialNumber를 값으로 캡처
    //running 값이 바뀌었을 때
    connect(client, &ApiClient::robotStateChanged, this, [this, serialNumber](bool isRunning) {
        updateRunningState(serialNumber, isRunning);
    });

    //연결 실패
    connect(client,
            &ApiClient::requestFailed,
            this,
            [this,
             serialNumber](const QString &endpoint, const QString &error, const QString &url) {
                // 제어기 정보 가져오기
                ControllerInfo info = getController(serialNumber);

                qWarning() << "[ControllerManager]" << serialNumber << " " << url << " "
                           << "request failed:" << error;

                updateConnectionState(serialNumber, false);
                updateRunningState(serialNumber, false);
            });

    //연결 정상
    connect(client,
            &ApiClient::requestSucceeded,
            this,
            [this, serialNumber](const QString &endpoint, const QJsonObject &response) {
                bool running = response.value("data").toBool();
                updateConnectionState(serialNumber, true);
                updateRunningState(serialNumber, running);
            });

    // ApiClient 저장
    {
        QMutexLocker locker(&mutex_);
        apiClients_[serialNumber] = client;
    }

    qDebug() << "[setupApiClient] ApiClient created and polling started for" << serialNumber;
}

void ControllerManager::cleanupApiClient(const QString &serialNumber)
{
    QMutexLocker locker(&mutex_);

    if (apiClients_.contains(serialNumber)) {
        ApiClient *client = apiClients_.take(serialNumber);
        if (client) {
            client->deleteLater();
            qDebug() << "[cleanupApiClient] ApiClient removed for" << serialNumber;
        }
    }
}

// 모든 제어기 상태 업데이트
void ControllerManager::updateControllersStates()
{
    QList<ControllerInfo>      controllersCopy;
    QMap<QString, ApiClient *> clientsCopy;

    {
        QMutexLocker locker(&mutex_);
        controllersCopy = controllers_;
        clientsCopy     = apiClients_;
    }

    qDebug() << "[ControllerManager] Processing" << controllersCopy.size() << "controllers";

    // mutex 없이 순회

    for (const auto &c : controllersCopy) {
        ApiClient *client = clientsCopy.value(c.serialNumber, nullptr);

        // URL이 틀리거나, ApiClient가 없으면 재생성
        if (!client) {
            updateConnectionState(c.serialNumber, false);  // 즉시 끊김 표시
            setupApiClient(c.serialNumber);
            continue;
        }

        QString inputUrl    = QString("%1:%2").arg(c.ip).arg(c.apiPort);
        QString expectedUrl = ApiClient::normalizeBaseUrl(inputUrl);
        QString currentUrl  = client->getBaseUrl();

        if (currentUrl != expectedUrl) {
            qDebug() << "[ControllerManager] URL mismatch for" << c.serialNumber;
            updateConnectionState(c.serialNumber, false);  // 즉시 끊김 표시
            cleanupApiClient(c.serialNumber);
            setupApiClient(c.serialNumber);
            continue;
        }
        updateConnectionState(c.serialNumber, false);

        client->get("/api/robot/running");
    }
}

// 상태 업데이트 메서드들
void ControllerManager::updateRunningState(const QString &serialNumber, bool running)
{
    QMutexLocker locker(&mutex_);

    for (auto &c : controllers_) {
        if (c.serialNumber == serialNumber) {
            c.isRunning = running;
            qDebug() << "[ControllerManager]" << serialNumber << "running state changed to"
                     << (running ? "RUNNING" : "IDLE");

            locker.unlock();
            emit controllerStateUpdated(serialNumber, c.isConnected, c.isRunning);
            return;
        }
    }
}

void ControllerManager::updateConnectionState(const QString &serialNumber, bool connected)
{
    QMutexLocker locker(&mutex_);

    for (auto &c : controllers_) {
        if (c.serialNumber == serialNumber) {
            c.isConnected = connected;

            // 연결 끊기면 running도 false로
            if (!connected) {
                c.isRunning = false;
            }

            qDebug() << "[ControllerManager]" << serialNumber << "connection state changed to"
                     << (connected ? "CONNECTED" : "DISCONNECTED");

            locker.unlock();
            emit controllerStateUpdated(serialNumber, c.isConnected, c.isRunning);
            return;
        }
    }
}

// 제어기 정보 가져오기
ControllerInfo ControllerManager::getController(const QString &serialNumber) const
{
    QMutexLocker locker(&mutex_);

    for (const auto &c : controllers_) {
        if (c.serialNumber == serialNumber) {
            return c;
        }
    }

    return ControllerInfo();  // 빈 구조체 반환
}

// ApiClient 가져오기
ApiClient *ControllerManager::getApiClient(const QString &serialNumber)
{
    QMutexLocker locker(&mutex_);
    return apiClients_.value(serialNumber, nullptr);
}

QList<ControllerInfo> ControllerManager::getControllers() const
{
    QMutexLocker locker(&mutex_);
    return controllers_;  // 복사본 반환
}

bool ControllerManager::isDuplicatedSN(const QString &SN)
{
    for (const auto &c : controllers_) {
        if (c.serialNumber == SN) {
            return true;
        }
    }
    return false;
}

void ControllerManager::saveToFile(const QString &filePath)
{
    QString path = filePath.isEmpty() ? configFilePath_ : filePath;

    QMutexLocker locker(&mutex_);

    QJsonArray controllersArray;
    for (const auto &c : controllers_) {
        QJsonObject obj;
        obj["serialNumber"] = c.serialNumber;
        obj["ip"]           = c.ip;
        obj["sftpPort"]     = c.sftpPort;
        obj["apiPort"]      = c.apiPort;
        obj["username"]     = c.username;
        obj["isConnected"]  = c.isConnected;
        obj["isRunning"]    = c.isRunning;

        controllersArray.append(obj);
    }

    QJsonObject root;
    root["version"]     = "1.0";
    root["controllers"] = controllersArray;

    QJsonDocument doc(root);

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        qDebug() << "[ControllerManager] Saved to:" << path;
    } else {
        qWarning() << "[ControllerManager] Failed to save:" << path;
    }
}

void ControllerManager::loadFromFile(const QString &filePath)
{
    QString path = filePath.isEmpty() ? configFilePath_ : filePath;

    QFile file(path);
    if (!file.exists()) {
        qDebug() << "[ControllerManager] No config file found, starting fresh";
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[ControllerManager] Failed to open:" << path;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[ControllerManager] Invalid JSON format";
        return;
    }

    QJsonObject root             = doc.object();
    QJsonArray  controllersArray = root["controllers"].toArray();

    QMutexLocker locker(&mutex_);
    controllers_.clear();

    for (const QJsonValue &val : controllersArray) {
        QJsonObject obj = val.toObject();

        ControllerInfo c;
        c.serialNumber = obj["serialNumber"].toString();
        c.ip           = obj["ip"].toString();
        c.sftpPort     = obj["sftpPort"].toInt();
        c.apiPort      = obj["apiPort"].toInt();
        c.username     = obj["username"].toString();
        c.isConnected  = obj["isConnected"].toBool(false);
        c.isRunning    = obj["isRunning"].toBool(false);

        controllers_.append(c);
    }

    qDebug() << "[ControllerManager] Loaded" << controllers_.size() << "controllers from:" << path;

    locker.unlock();

    for (const auto &c : controllers_) {
        setupApiClient(c.serialNumber);
    }

    emit controllerListChanged();
}
void ControllerManager::backupRequest(const QString &serialNumber)
{
    ApiClient *client = getApiClient(serialNumber);
    if (!client) {
        qWarning() << "[ControllerManager] No client for" << serialNumber;
        return;
    }

    // 백업 저장 경로 설정
    QString backupDir = QString("C:/backup/%1").arg(serialNumber);
    client->download("/api/robot/export", backupDir, serialNumber);
}
void ControllerManager::applyRequest(const QString &serialNumber, const QString &filePath)
{
    ApiClient *client = getApiClient(serialNumber);
    if (!client) {
        qWarning() << "[ControllerManager] No client for" << serialNumber;
        return;
    }
    client->upload("/api/robot/import", filePath);
}
