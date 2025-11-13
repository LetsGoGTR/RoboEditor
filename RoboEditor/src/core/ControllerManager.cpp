#include "ControllerManager.h"

#include <QTemporaryDir>

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "ControllerSetting.h"
#include "FileCompressor.h"
#include "SftpClient.h"

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
                    c.pswd     = updated.pswd;
                    c.wsPath   = updated.wsPath;

                    qDebug() << "[updateInfo] Controller updated:";
                    qDebug() << "SN:" << c.serialNumber;
                    qDebug() << "IP:" << c.ip;
                    qDebug() << "Username:" << c.username;
                    qDebug() << "workspace Path:" << c.wsPath;
                    qDebug() << "SFTP:" << c.sftpPort;

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

    QString    baseUrl = QString("%1").arg(info.ip);
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

        QString inputUrl    = QString("%1").arg(c.ip);
        QString expectedUrl = ApiClient::normalizeBaseUrl(inputUrl);
        QString currentUrl  = client->getBaseUrl();

        if (currentUrl != expectedUrl) {
            qDebug() << "[ControllerManager] URL mismatch for" << c.serialNumber;
            qDebug() << "exp : " << expectedUrl;
            qDebug() << "cur : " << currentUrl;

            updateConnectionState(c.serialNumber, false);  // 즉시 끊김 표시
            cleanupApiClient(c.serialNumber);
            setupApiClient(c.serialNumber);
            continue;
        }
        updateConnectionState(c.serialNumber, false);

        client->get("/api/robot/running");
        if(c.serialNumber =="SN1"){

        ControllerManager *manager = ControllerManager::instance();

        QStringList localFiles;
        localFiles << "C:/Users/SSAFY/workspace.tgz";  // 로컬 파일 (Windows 경로)

        QString remoteDir = "/workspace/";  // 원격 디렉토리 (리눅스 경로)

        // bool success = manager->send("SN1",       // 시리얼 번호
        // localFiles,  // 로컬 파일들 (압축할 파일 목록)
        // remoteDir    // 원격 저장 경로
        // );

        // bool success = manager->receive(
        // "SN1",                      // 시리얼 번호
        // "/workspace/test.tgz",   // 원격 파일 경로
        // "C:/Download"               // 로컬 폴더 경로
        // );

        }
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
        obj["username"]     = c.username;
        obj["pswd"]         = c.pswd;
        obj["wsPath"]       = c.wsPath;
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
        c.username     = obj["username"].toString();
        c.pswd         = obj["pswd"].toString();
        c.wsPath       = obj["wsPath"].toString();
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
// void ControllerManager::backupRequest(const QString &serialNumber)
// {
//     ApiClient *client = getApiClient(serialNumber);
//     if (!client) {
//         qWarning() << "[ControllerManager] No client for" << serialNumber;
//         return;
//     }

//     // 백업 저장 경로 설정
//     QString backupDir = QString("C:/backup/%1").arg(serialNumber);

//     //bool uploadFile(const QString &localPath, const QString &remotePath);
//     //bool downloadFile(const QString &remotePath, const QString &localPath);

//     //https용
//     //client->download("/api/robot/export", backupDir, serialNumber);
// }
// void ControllerManager::applyRequest(const QString &serialNumber, const QString &filePath)
// {
//     ApiClient *client = getApiClient(serialNumber);
//     if (!client) {
//         qWarning() << "[ControllerManager] No client for" << serialNumber;
//         return;
//     }

//     //https용
//     //client->upload("/api/robot/import", filePath);
// }
bool ControllerManager::backupRequest(const QString& serialNumber,
                                      const QString& baseBackupDir)
{
    ControllerInfo info = getController(serialNumber);
    if (info.serialNumber.isEmpty()) return false;
    ApiClient* client = getApiClient(serialNumber);
    if (!client) return false;

    QDir base(baseBackupDir);
    QString sn = info.serialNumber;
    QString snDir; // 항상 C:\backup\123 형태로 맞춤
    {
        QString tail = QFileInfo(base.path()).fileName();
        if (tail == sn) snDir = base.path();
        else            snDir = base.filePath(sn);
    }
    if (!QDir().mkpath(snDir)) {
        qWarning() << "[backupRequest] cannot mkpath:" << snDir;
        return false;
    }

    // 타임스탬프 이름 계산 (폴더명 & 내부 workspace rename 용)
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
    const QString targetDirName = sn + "_" + ts;      // 예: 123_2025-11-12_153723
    const QString remoteTarGz   = info.wsPath + "/workspace.tgz";

    // compress 완료 콜백에서만 받기
    QMetaObject::Connection okConn, failConn;
    okConn = connect(client, &ApiClient::requestSucceeded, this,
                     [=](const QString& endpoint, const QJsonObject&) {
                         if (endpoint != "/api/workspace/compress") return;
                         QObject::disconnect(okConn);
                         QObject::disconnect(failConn);

                         // receive는 snDir(부모 폴더) + targetDirName(원하는 최상위 폴더명)으로 호출
                         bool ok = receive(serialNumber, remoteTarGz, snDir, targetDirName);
                         if (!ok) qWarning() << "[backupRequest] receive failed for" << serialNumber;
                         else     qDebug()   << "[backupRequest] completed at"
                                      << QDir(snDir).filePath(targetDirName);
                     },
                     Qt::QueuedConnection);

    failConn = connect(client, &ApiClient::requestFailed, this,
                       [=](const QString& endpoint, const QString& err, const QString&) {
                           if (endpoint != "/api/workspace/compress") return;
                           QObject::disconnect(okConn);
                           QObject::disconnect(failConn);
                           qWarning() << "[backupRequest] compress failed:" << err;
                       },
                       Qt::QueuedConnection);

    if (!client->postWorkspaceCompress(info.username)) {
        QObject::disconnect(okConn);
        QObject::disconnect(failConn);
        qWarning() << "[backupRequest] failed to send compress request";
        return false;
    }
    return true;
}


bool ControllerManager::applyRequest(const QString &serialNumber,
                                     const QString &filePath,
                                     const QString& apiPassword)
{
    // 0) 입력 검증 먼저
    if (filePath.isEmpty()) {
        qWarning() << "[ControllerManager][applyRequest] filePath is empty";
        return false;
    }

    // 1) 컨트롤러 조회
    ControllerInfo info = getController(serialNumber);
    if (info.serialNumber.isEmpty()) {
        qWarning() << "[ControllerManager][applyRequest] Controller not found:" << serialNumber;
        return false;
    }

    // 2) ApiClient 확보
    ApiClient* client = getApiClient(serialNumber);
    if (!client) {
        qWarning() << "[ControllerManager][applyRequest] ApiClient not found for:" << serialNumber;
        return false;
    }

    // 3) SFTP 업로드 (remoteDir는 '디렉터리'만 넘김)
    const QString remoteDir = info.wsPath; // 예: "/home/samsung/workspace"
    const QStringList localPaths{ filePath }; // ex) C:/backup/1234/123_2025-11-07_150404.tgz
    if (!send(serialNumber, localPaths, remoteDir)) { // 선언은 dir-only (파일명 X)  :contentReference[oaicite:0]{index=0}
        qWarning() << "[ControllerManager][applyRequest] SFTP send failed for" << serialNumber;
        return false;
    }

    // 4) 서버에 압축 해제 요청
    if (!client->postWorkspaceExtract(info.username, apiPassword)) {
        qWarning() << "[ControllerManager][applyRequest] extract request failed for" << serialNumber;
        return false;
    }

    if (filePath.isEmpty()) {
        qWarning() << "[ControllerManager][applyRequest] filePath is empty";
        return false;
    }

    // QStringList localPaths;
    // localPaths << filePath;   // ex) C:/backup/1234/123_2025-11-07_150404

    // bool ok = send(serialNumber, localPaths, remoteDir);
    // if (!ok) {
    //     qWarning() << "[ControllerManager][applyRequest] send failed for" << serialNumber;
    //     return false;
    // }

    return true;
}

bool ControllerManager::send(const QString     &serialNumber,
                             const QStringList &localPaths,
                             const QString     &remoteDir)
{
    // 1. 제어기 정보 가져오기
    ControllerInfo controller = getController(serialNumber);
    if (controller.serialNumber.isEmpty()) {
        qWarning() << "Controller not found:" << serialNumber;
        return false;
    }

    // 2. 임시 디렉토리에서 tgz 생성
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        qWarning() << "Cannot create temporary directory";
        return false;
    }

    QString tarGzPath = tempDir.path() + "/upload.tgz";

    qDebug() << "Creating tgz:" << tarGzPath;
    if (!FileCompressor::createTarGz(localPaths, tarGzPath)) {
        qWarning() << "Failed to create tgz";
        return false;
    }

    // 3. SFTP 연결 및 업로드
    SFTPClient client(controller.ip, controller.sftpPort, controller.username, controller.pswd);

    if (!client.connectToServer()) {
        qWarning() << "SFTP connection failed:" << controller.ip;
        return false;
    }

    QString remotePath = remoteDir + "/workspace.tgz";
    qDebug() << "Uploading to:" << remotePath;

    bool uploadSuccess = client.uploadFile(tarGzPath, remotePath);
    client.disconnect();

    if (!uploadSuccess) {
        qWarning() << "Upload failed";
        return false;
    }

    qDebug() << "Send completed successfully";
    return true;
}

bool ControllerManager::receive(const QString& serialNumber,
                                const QString& remoteTarGz,
                                const QString& parentDir,
                                const QString& targetDirName)
{
    // 1. 제어기 정보 가져오기
    ControllerInfo controller = getController(serialNumber);
    if (controller.serialNumber.isEmpty()) {
        qWarning() << "Controller not found:" << serialNumber;
        return false;
    }

    // 2. SFTP 연결 및 다운로드
    SFTPClient client(controller.ip, controller.sftpPort, controller.username, controller.pswd);
    if (!client.connectToServer()) {
        qWarning() << "SFTP connection failed:" << controller.ip;
        return false;
    }

    QTemporaryDir tempDir; // 세션 임시폴더
    if (!tempDir.isValid()) {
        qWarning() << "Cannot create temporary directory";
        client.disconnect();
        return false;
    }

    const QString localTarPath = tempDir.path() + "/download.tgz";
    qDebug() << "Downloading from:" << remoteTarGz << "to:" << localTarPath;

    const bool downloadSuccess = client.downloadFile(remoteTarGz, localTarPath);
    client.disconnect();
    if (!downloadSuccess) {
        qWarning() << "Download failed";
        return false;
    }

    // 3. 임시 추출
    const QString tempExtractRoot = tempDir.path() + "/extract";
    qDebug() << "Extracting to:" << tempExtractRoot;
    if (!FileCompressor::extractTarGz(localTarPath, tempExtractRoot)) {
        qWarning() << "Failed to extract tgz";
        return false;
    }

    // 4. 압축 내부 최상위가 'workspace'인지 확인
    const QString srcWorkspace = QDir(tempExtractRoot).filePath("workspace");
    if (!QDir(srcWorkspace).exists()) {
        qWarning() << "Missing 'workspace' root in archive";
        return false;
    }

    // 5. 최종 경로: C:\backup\<SN>\<SN>_YYYY-MM-DD_HHMMSS
    if (!QDir().mkpath(parentDir)) {
        qWarning() << "Cannot mkpath parentDir:" << parentDir;
        return false;
    }
    const QString finalPath = QDir(parentDir).filePath(targetDirName);

    // 동일 드라이브면 rename이 가장 안전/빠름
    if (!QDir().rename(srcWorkspace, finalPath)) {
        // rename 실패 시 간단 복사 fallback (최소 구현)
        auto copyDirRecursive = [](const QString& src, const QString& dst, auto&& self) -> bool {
            QDir s(src);
            if (!s.exists()) return false;
            if (!QDir().mkpath(dst)) return false;
            const auto entries = s.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
            for (const QFileInfo& fi : entries) {
                const QString from = fi.absoluteFilePath();
                const QString to   = QDir(dst).filePath(fi.fileName());
                if (fi.isDir()) {
                    if (!self(from, to, self)) return false;
                } else {
                    if (QFile::exists(to)) QFile::remove(to);
                    if (!QFile::copy(from, to)) return false;
                }
            }
            return true;
        };

        if (!copyDirRecursive(srcWorkspace, finalPath, copyDirRecursive)) {
            qWarning() << "Failed to place extracted content to final dest:" << finalPath;
            // 실패 시 최종 폴더 생성되지 않거나 내용 없음 → 빈 폴더 남지 않음
            return false;
        }
    }

    qDebug() << "Receive completed successfully ->" << finalPath;
    return true;
}
