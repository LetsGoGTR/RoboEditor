#include "ControllerManager.h"

#include <QTemporaryDir>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QMutex>

#include "ControllerSetting.h"
#include "FileCompressor.h"
#include "LogManager.h"
#include "PasswordManager.h"
#include "SftpClient.h"

static ControllerManager *getinstance = nullptr;

ControllerManager::ControllerManager(QObject *parent) :
    QObject(parent),
    pm_(new PasswordManager(nullptr))
{
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
        newConInfo.birth       = QDateTime::currentDateTime().toString(Qt::ISODate);
        controllers_.append(newConInfo);

        qDebug() << "SN:" << newConInfo.serialNumber;
        qDebug() << "IP:" << newConInfo.ip;
        qDebug() << "SFTP:" << newConInfo.sftpPort;
        qDebug() << "Username:" << newConInfo.username;

        saveToFile(newConInfo.serialNumber);
        QString msg = QString("[%1] is registred").arg(newConInfo.serialNumber);
        LogManager::append(msg);

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
        QString msg = QString("[%1] is removed").arg(sn);
        LogManager::append(msg);
        saveToFile();
        emit controllerListChanged();
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
            QString msg = QString("[%1] is removed").arg(sn);
            LogManager::append(msg);
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

                    QString msg = QString("[%1] state has been updated").arg(c.serialNumber);
                    LogManager::append(msg);

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
    if (stateUpdatesPaused_) {
        return;
    }

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
            //updateConnectionState(c.serialNumber, false);  // 즉시 끊김 표시
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

            //updateConnectionState(c.serialNumber, false);  // 즉시 끊김 표시
            cleanupApiClient(c.serialNumber);
            setupApiClient(c.serialNumber);
            continue;
        }

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

void ControllerManager::saveToFile(const QString &serialNumber)
{
    if (serialNumber.isEmpty()) {
        // 1. 전체 저장 (프로그램 종료 시)
        QList<ControllerInfo> controllersCopy;
        {
            QMutexLocker locker(&mutex_);
            controllersCopy = controllers_;
        }

        int successCount = 0;
        int failCount    = 0;

        for (const auto &c : controllersCopy) {
            if (saveController(c)) {
                successCount++;
            } else {
                failCount++;
                qWarning() << "[saveToFile] Failed to save:" << c.serialNumber;
            }
        }

        saveControllerList();

        qDebug() << "[saveToFile] All saved - Success:" << successCount << "Failed:" << failCount;
    } else {
        // 2. 특정 제어기만 저장
        ControllerInfo info;

        {
            QMutexLocker locker(&mutex_);
            bool         found = false;
            for (const auto &c : controllers_) {
                if (c.serialNumber == serialNumber) {
                    info  = c;
                    found = true;
                    break;
                }
            }

            if (!found) {
                qWarning() << "[saveToFile] Controller not found:" << serialNumber;
                return;
            }
        }

        // 개별 저장
        if (!saveController(info)) {
            qWarning() << "[saveToFile] Failed to save:" << serialNumber;
            return;
        }

        saveControllerList();

        qDebug() << "[saveToFile] Saved:" << serialNumber;
    }
}

void ControllerManager::loadFromFile()
{
    // 1. 기존 데이터 초기화
    {
        QMutexLocker locker(&mutex_);
        controllers_.clear();
    }

    // 2. 전체 제어기 목록 로드
    loadControllerList();

    // 3. ApiClient 설정
    QList<ControllerInfo> controllersCopy;
    {
        QMutexLocker locker(&mutex_);
        controllersCopy = controllers_;
    }

    for (const auto &c : controllersCopy) {
        setupApiClient(c.serialNumber);
    }

    qDebug() << "[loadFromFile] Loaded" << controllersCopy.size() << "controllers";

    emit controllerListChanged();
}
bool ControllerManager::backupRequest(const QString &serialNumber, const QString &baseBackupDir)
{
    ControllerInfo info = getController(serialNumber);
    if (info.serialNumber.isEmpty())
        return false;
    ApiClient *client = getApiClient(serialNumber);
    if (!client)
        return false;

    QDir    base(baseBackupDir);
    QString sn = info.serialNumber;
    QString snDir;  // 항상 C:\backup\123 형태로 맞춤
    {
        QString tail = QFileInfo(base.path()).fileName();

        // 1) base가 "C:/backup" 같은 루트일 때만 SN 하위 폴더 생성
        if (tail.compare("backup", Qt::CaseInsensitive) == 0) {
            // 예: base = C:/backup → C:/backup/SN1
            snDir = base.filePath(sn);
        }
        // 2) base가 이미 해당 SN 폴더일 때는 있는 폴더 그대로 사용
        else if (tail == sn) {
            // 예: base = C:/backup/SN1 → C:/backup/SN1
            snDir = base.path();
        }
        // 3) 그 외는 호출자가 넘긴 baseBackupDir을 그대로 최상위로 사용
        else {
            // 예: base = C:/backup/test → C:/backup/test
            snDir = base.path();
        }
    }
    if (!QDir().mkpath(snDir)) {
        qWarning() << "[backupRequest] cannot mkpath:" << snDir;
        return false;
    }

    // 타임스탬프 이름 계산 (폴더명 & 내부 workspace rename 용)
    const QString ts            = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
    const QString targetDirName = sn + "_" + ts;  // 예: 123_2025-11-12_153723
    const QString remoteTarGz   = info.wsPath + "/output.tgz";

    QObject *context = new QObject(this);

    connect(
            client,
            &ApiClient::requestSucceeded,
            context,  // ← context 추가
            [this, serialNumber, remoteTarGz, snDir, targetDirName, context](
                    const QString &endpoint, const QJsonObject &) {
                if (endpoint != "/api/workspace/compress")
                    return;

                // context 삭제 (자동으로 모든 연결 해제)
                context->deleteLater();

                // receive는 snDir(부모 폴더) + targetDirName(원하는 최상위 폴더명)으로 호출
                bool ok = receive(serialNumber, remoteTarGz, snDir, targetDirName);
                if (!ok) {
                    qWarning() << "[backupRequest] receive failed for" << serialNumber;
                    emit backupFailed(serialNumber, QStringLiteral("SFTP 수신 실패"));
                } else {
                    qDebug() << "[backupRequest] completed at"
                             << QDir(snDir).filePath(targetDirName);
                    emit backupCompleted(serialNumber);
                }
            },
            Qt::QueuedConnection);

    connect(
            client,
            &ApiClient::requestFailed,
            context,  // ← context 추가
            [this, serialNumber, context](
                    const QString &endpoint, const QString &err, const QString &) {
                if (endpoint != "/api/workspace/compress")
                    return;

                // context 삭제 (자동으로 모든 연결 해제)
                context->deleteLater();

                qWarning() << "[backupRequest] compress failed:" << err;
                emit backupFailed(serialNumber, QStringLiteral("압축 요청 실패: ") + err);
            },
            Qt::QueuedConnection);

    if (!client->postWorkspaceCompress(info.username)) {
        // 요청 전송 실패 시 context 정리
        context->deleteLater();
        qWarning() << "[backupRequest] failed to send compress request";
        emit backupFailed(serialNumber, QStringLiteral("압축 요청 전송 실패"));
        return false;
    }

    return true;
}

bool ControllerManager::applyRequest(const QString &serialNumber,
                                     const QString &filePath,
                                     const QString &apiPassword)
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
    ApiClient *client = getApiClient(serialNumber);
    if (!client) {
        qWarning() << "[ControllerManager][applyRequest] ApiClient not found for:" << serialNumber;
        return false;
    }

    // 3) SFTP 업로드 (remoteDir는 '디렉터리'만 넘김)
    const QString     remoteDir = info.wsPath;  // 예: "/home/samsung/workspace"
    const QStringList localPaths{filePath};     // ex) C:/backup/1234/123_2025-11-07_150404.tgz
    if (!send(serialNumber,
              localPaths,
              remoteDir)) {  // 선언은 dir-only (파일명 X)  :contentReference[oaicite:0]{index=0}
        qWarning() << "[ControllerManager][applyRequest] SFTP send failed for" << serialNumber;
        return false;
    }

    // 2) /api/workspace/extract 결과 비동기 감시
    QObject *context = new QObject(this);

    connect(
            client,
            &ApiClient::requestSucceeded,
            context,
            [this, serialNumber, context](const QString &endpoint, const QJsonObject &) {
                if (endpoint != "/api/workspace/extract")
                    return;

                context->deleteLater();  // 자동으로 모든 연결 해제

                qDebug() << "[applyRequest] extract completed for" << serialNumber;
                emit applyCompleted(serialNumber);
            },
            Qt::QueuedConnection);

    connect(
            client,
            &ApiClient::requestFailed,
            context,  // ← context 추가
            [this, serialNumber, context](
                    const QString &endpoint, const QString &err, const QString &) {
                if (endpoint != "/api/workspace/extract")
                    return;

                context->deleteLater();  // 자동으로 모든 연결 해제

                qWarning() << "[applyRequest] extract failed:" << err;
                emit applyFailed(serialNumber, QStringLiteral("압축 해제 요청 실패: ") + err);
            },
            Qt::QueuedConnection);

    if (!client->postWorkspaceExtract(info.username, "0000")) {
        context->deleteLater();
        qWarning() << "[ControllerManager][applyRequest] extract request send failed for"
                   << serialNumber;

        emit applyFailed(serialNumber, QStringLiteral("압축 해제 요청 전송 실패"));
        return false;
    }

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

    QString remotePath = remoteDir + "/input.tgz";
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

bool ControllerManager::ControllerManager::receive(const QString &serialNumber,
                                                   const QString &remoteTarGz,
                                                   const QString &parentDir,
                                                   const QString &targetDirName)
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

    QTemporaryDir tempDir;  // 세션 임시폴더
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
        auto copyDirRecursive = [](const QString &src, const QString &dst, auto &&self) -> bool {
            QDir s(src);
            if (!s.exists())
                return false;
            if (!QDir().mkpath(dst))
                return false;
            const auto entries = s.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
            for (const QFileInfo &fi : entries) {
                const QString from = fi.absoluteFilePath();
                const QString to   = QDir(dst).filePath(fi.fileName());
                if (fi.isDir()) {
                    if (!self(from, to, self))
                        return false;
                } else {
                    if (QFile::exists(to))
                        QFile::remove(to);
                    if (!QFile::copy(from, to))
                        return false;
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

// 컨트롤러 정보를 해당하는 제어기에 json파일로 저장
bool ControllerManager::saveController(const ControllerInfo &controller)
{
    // 1. 경로 생성
    QString folderPath = QString("C:/backup/%1/config").arg(controller.serialNumber);
    QString filePath   = folderPath + "/controller.json";

    // 2. config 디렉토리 생성
    QDir dir;
    if (!dir.exists(folderPath)) {
        if (!dir.mkpath(folderPath)) {
            qWarning() << "[saveController] Failed to create directory:" << folderPath;
            return false;
        }
    }

    // 3. JSON 객체 생성
    QJsonObject obj;
    obj["serialNumber"] = controller.serialNumber;
    obj["ip"]           = controller.ip;
    obj["sftpPort"]     = controller.sftpPort;
    obj["username"]     = controller.username;
    obj["pswd"]         = pm_->encrypt(controller.pswd);
    obj["wsPath"]       = controller.wsPath;
    obj["createdAt"]    = controller.birth;
    obj["lastModified"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    QJsonDocument doc(obj);

    // 4. 파일 쓰기
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[saveController] Failed to open file:" << filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "[saveController] Saved:" << filePath;
    return true;
}
//시리얼 넘버에 해당하는 제어기에서 불러오기
bool ControllerManager::loadController(const QString &serialNumber)
{
    // 1. 파일 경로 생성
    QString folderPath = QString("C:/backup/%1/config").arg(serialNumber);
    QString filePath   = folderPath + "/controller.json";

    // 2. 파일 존재 확인
    QFile file(filePath);
    if (!file.exists()) {
        qWarning() << "[loadController] Config not found:" << filePath;
        return false;
    }

    // 3. 파일 열기
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[loadController] Failed to open:" << filePath;
        return false;
    }

    // 4. 파일 읽기
    QByteArray data = file.readAll();
    file.close();

    // 5. JSON 파싱
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[loadController] Invalid JSON format in:" << filePath;
        return false;
    }

    QJsonObject obj = doc.object();
    // 6. ControllerInfo 생성
    ControllerInfo c;
    c.serialNumber = obj["serialNumber"].toString();
    c.ip           = obj["ip"].toString();
    c.sftpPort     = obj["sftpPort"].toInt();
    c.username     = obj["username"].toString();
    c.pswd         = pm_->decrypt(obj["pswd"].toString());
    c.birth        = obj["createdAt"].toString();
    c.wsPath       = obj["wsPath"].toString();
    c.isConnected  = false;  // 시작 시 연결 안됨
    c.isRunning    = false;  // 시작 시 실행 안됨

    // 기존 데이터 찾기
    bool found = false;
    for (int i = 0; i < controllers_.size(); ++i) {
        if (controllers_[i].serialNumber == serialNumber) {
            // 기존 데이터 업데이트
            controllers_[i] = c;
            found           = true;
            qDebug() << "[loadController] Updated:" << serialNumber;
            break;
        }
    }

    // 없으면 추가
    if (!found) {
        controllers_.append(c);
        qDebug() << "[loadController] Loaded:" << serialNumber;
    }

    return true;
}

// 전체 제어기 목록 관리 (경량 메타데이터)
void ControllerManager::saveControllerList()
{
    //전체 제어기 목록 업데이트

    // 1. Mutex로 보호된 영역에서 복사
    QList<ControllerInfo> controllersCopy;
    {
        QMutexLocker locker(&mutex_);
        controllersCopy = controllers_;
    }

    // 2. JSON 배열 생성 (SN과 IP만)
    QJsonArray snArray;
    for (const auto &c : controllersCopy) {
        QJsonObject obj;
        obj["serialNumber"] = c.serialNumber;
        obj["ip"]           = c.ip;
        snArray.append(obj);
    }

    // 3. 루트 객체 생성
    QJsonObject root;
    root["version"]     = "1.0";
    root["lastUpdated"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["controllers"] = snArray;

    QJsonDocument doc(root);

    // 4. 파일 저장
    QString listPath = QString("C:/backup/config/controller_list.json");
    QFile   file(listPath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[saveControllerList] Failed to open:" << listPath;
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}
void ControllerManager::loadControllerList()
{
    //전체 제어기 목록 가져오기
    QString listPath = QString("C:/backup/config/controller_list.json");

    QFile file(listPath);
    if (!file.exists()) {
        qDebug() << "[loadControllerList] No list file found";
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[loadControllerList] Failed to open:" << listPath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[loadControllerList] Invalid JSON format";
        return;
    }

    QJsonObject root    = doc.object();
    QJsonArray  snArray = root["controllers"].toArray();

    qDebug() << "[loadControllerList] Found" << snArray.size() << "controllers in list";

    // 각 제어기의 상세 정보 로드
    int successCount = 0;
    int failCount    = 0;

    for (const QJsonValue &val : snArray) {
        QJsonObject obj = val.toObject();
        QString     sn  = obj["serialNumber"].toString();

        if (!sn.isEmpty()) {
            if (loadController(sn)) {
                successCount++;
            } else {
                failCount++;
            }
        }
    }

    qDebug() << "[loadControllerList] Loaded successfully:" << successCount
             << "Failed:" << failCount;
}
void ControllerManager::onMasterPasswordChanged()
{
    if (!pm_)
        return;

    QString msg = QString("Master Key is changed");
    LogManager::append(msg);
    pm_->loadPasswordFromConfig();

    saveToFile();  // 모든 컨트롤러 정보 재저장
}

void ControllerManager::pauseStateUpdates()
{
    stateUpdatesPaused_ = true;
    qDebug() << "[ControllerManager] State updates paused";
}

void ControllerManager::resumeStateUpdates()
{
    stateUpdatesPaused_ = false;
    qDebug() << "[ControllerManager] State updates resumed";

    // 즉시 한 번 업데이트
    QTimer::singleShot(0, this, &ControllerManager::updateControllersStates);
}
