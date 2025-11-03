#include "ControllerManager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>

#include "ControllerSetting.h"

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

        //동일한 SN이 있으면 에러
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
        } else {
            qDebug() << "폴더 이미 존재:" << folderPath;
        }

        controllers_.append(newConInfo);
        qDebug() << "SN:" << newConInfo.serialNumber;
        qDebug() << "IP:" << newConInfo.ip;
        qDebug() << "SFTP:" << newConInfo.sftpPort;
        qDebug() << "API:" << newConInfo.apiPort;
        qDebug() << "Username:" << newConInfo.username;
        qDebug() << "Workspace:" << newConInfo.workspacePath;
    } else {
        qDebug() << "Controller registration canceled";
    }
    saveToFile();
    emit controllerListChanged();
}
void ControllerManager::removeController(int index)
{
    if (index >= 0 && index < controllers_.size()) {
        QString sn = controllers_[index].serialNumber;
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
    //ip, username, sftp port, api port, workspacePath 수정

    ControllerSetting dialog;
    dialog.setControllerInfo(newInfo);

    if (dialog.exec() == QDialog::Accepted) {
        ControllerInfo updated = dialog.getControllerInfo();

        QMutexLocker locker(&mutex_);
        for (auto &c : controllers_) {
            if (c.serialNumber == newInfo.serialNumber) {
                c.ip            = updated.ip;
                c.username      = updated.username;
                c.sftpPort      = updated.sftpPort;
                c.apiPort       = updated.apiPort;
                c.workspacePath = updated.workspacePath;

                qDebug() << "[updateInfo] Controller updated:";
                qDebug() << "SN:" << c.serialNumber;
                qDebug() << "IP:" << c.ip;
                qDebug() << "Username:" << c.username;
                qDebug() << "SFTP:" << c.sftpPort;
                qDebug() << "API:" << c.apiPort;
                qDebug() << "Workspace:" << c.workspacePath;

                saveToFile();
                emit controllerListChanged();
                break;
            }
        }
    }
}
void ControllerManager::updateState()
{
    // isRunning, isConnected 확인
}
// bool ControllerManager::isConnected() {}
// bool ControllerManager::isRunning() {}
bool ControllerManager::isDuplicatedSN(const QString &SN)
{
    for (int idx = 0; idx < controllers_.size(); idx++) {
        if (controllers_[idx].serialNumber == SN) {
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
        obj["serialNumber"]  = c.serialNumber;
        obj["ip"]            = c.ip;
        obj["sftpPort"]      = c.sftpPort;
        obj["apiPort"]       = c.apiPort;
        obj["username"]      = c.username;
        obj["workspacePath"] = c.workspacePath;
        obj["isConnected"]   = c.isConnected;
        obj["isRunning"]     = c.isRunning;

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

// ✅ JSON 파일에서 로드
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
        c.serialNumber  = obj["serialNumber"].toString();
        c.ip            = obj["ip"].toString();
        c.sftpPort      = obj["sftpPort"].toInt();
        c.apiPort       = obj["apiPort"].toInt();
        c.username      = obj["username"].toString();
        c.workspacePath = obj["workspacePath"].toString();
        c.isConnected   = obj["isConnected"].toBool(false);
        c.isRunning     = obj["isRunning"].toBool(false);

        controllers_.append(c);
    }

    qDebug() << "[ControllerManager] Loaded" << controllers_.size() << "controllers from:" << path;

    // ✅ 로드 후 UI 업데이트를 위한 signal 발생
    emit controllerListChanged();
}
