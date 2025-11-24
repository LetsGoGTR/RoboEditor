#include "ApiClient.h"

#include <QTemporaryFile>
#include <QTimer>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonParseError>

#include "ControllerManager.h"
#include "FileCompressor.h"
#include "LogManager.h"

ApiClient::ApiClient(const ControllerInfo &info, QObject *parent) :
    QObject(parent),
    m_manager(new QNetworkAccessManager(this)),
    m_baseUrl(normalizeBaseUrlAPI(info)),
    m_robotRunning(false)
{
    // QNetworkAccessManager finished 시그널 연결
    connect(m_manager, &QNetworkAccessManager::finished, this, &ApiClient::onFinished);

    qDebug() << "[ApiClient] Initialized with base URL:" << m_baseUrl;
}

ApiClient::~ApiClient() {}
// url 정규화
QString ApiClient::normalizeBaseUrlSFTP(const ControllerInfo &info)
{
    QString host = info.host.trimmed();

    // 경로 제거 (example.com/api -> example.com)
    int slashIndex = host.indexOf('/');
    if (slashIndex != -1) {
        host = host.left(slashIndex);
    }

    if (host.isEmpty()) {
        qWarning() << "[normalizeBaseUrl] Empty host";
        return QString();
    }

    // protocol: 0=HTTP, 1=HTTPS (버튼그룹 ID에 따라)
    QString protocol = (info.protocol == 0) ? "http://" : "https://";

    QString result = QString("%1%2:%3").arg(protocol).arg(host).arg(info.sftpPort);
    qDebug() << "[normalizeBaseUrlSFTP]" << info.host << "->" << result;
    return result;
}
QString ApiClient::normalizeBaseUrlAPI(const ControllerInfo &info)
{
    QString host = info.host.trimmed();

    // 경로 제거 (example.com/api -> example.com)
    int slashIndex = host.indexOf('/');
    if (slashIndex != -1) {
        host = host.left(slashIndex);
    }

    if (host.isEmpty()) {
        qWarning() << "[normalizeBaseUrl] Empty host";
        return QString();
    }

    // protocol: 0=HTTP, 1=HTTPS (버튼그룹 ID에 따라)
    QString protocol = (info.protocol == 0) ? "http://" : "https://";
    QString result   = QString("%1%2:%3").arg(protocol).arg(host).arg(info.apiPort);
    qDebug() << "[normalizeBaseUrlAPI]" << info.host << "->" << result;
    return result;
}
//요청 만들기, 도메인 + 엔드포인트 url 반환
QNetworkRequest ApiClient::createRequest(const QString &endpoint)
{
    QNetworkRequest request;
    QString         fullUrl = m_baseUrl + endpoint;
    request.setUrl(QUrl(fullUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setRawHeader("Connection", "keep-alive");

    return request;
}

//서버사이드에서 데이터 받아오기
void ApiClient::get(const QString &endpoint)
{
    QNetworkRequest request = createRequest(endpoint);  //도메인 객체 생성
    QNetworkReply  *reply   = m_manager->get(request);  //실제로 요청 보내기
    QString         url     = request.url().toString();

    // 엔드포인트와 메서드 정보를 reply 속성으로 저장
    reply->setProperty("endpoint", endpoint);
    reply->setProperty("method", "GET");

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(10000);  // 10초 제한

    // 타임아웃 발생 : 요청 강제 중단
    connect(timeoutTimer, &QTimer::timeout, this, [=]() {
        if (reply->isRunning()) {
            reply->abort();  // 네트워크 요청 중단
            qWarning() << "[ApiClient] Timeout for" << endpoint;
            emit requestFailed(endpoint, "Timeout - no response", url);
        }
        timeoutTimer->deleteLater();
    });

    // 정상적으로 끝나면 타이머 중지
    connect(reply, &QNetworkReply::finished, timeoutTimer, [timeoutTimer]() {
        if (timeoutTimer->isActive())
            timeoutTimer->stop();
        timeoutTimer->deleteLater();
    });

    // 타이머 시작
    timeoutTimer->start();
}

//서버사이드에 데이터 전송
void ApiClient::upload(const QString &endpoint, const QString &filePath)
{
    QStringList filesToCompress = {filePath};

    QString baseName      = QFileInfo(filePath).completeBaseName();
    QString tempTarGzPath = QDir::temp().filePath(baseName + ".tar.gz");

    if (QFile::exists(tempTarGzPath))
        QFile::remove(tempTarGzPath);

    if (!FileCompressor::createTarGz(filesToCompress, tempTarGzPath)) {
        qWarning() << "[ApiClient] Failed to create tar.gz file for upload:" << tempTarGzPath;
        QString url = createRequest(endpoint).url().toString();
        emit    requestFailed(endpoint, "Failed to compress file before upload", url);
        return;
    }

    QFile *file = new QFile(tempTarGzPath);
    if (!file->open(QIODevice::ReadOnly)) {
        qWarning() << "[ApiClient] Failed to open compressed file:" << tempTarGzPath;
        QString url = createRequest(endpoint).url().toString();
        emit    requestFailed(endpoint, "Cannot open compressed file", url);
        delete file;
        QFile::remove(tempTarGzPath);
        return;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart       filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                                        .arg(QFileInfo(tempTarGzPath).fileName())));

    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    // *** 수정: Content-Type을 설정하지 않음 (QHttpMultiPart가 자동 설정) ***
    QNetworkRequest request;
    QString         fullUrl = m_baseUrl + endpoint;
    request.setUrl(QUrl(fullUrl));
    // application/json 헤더를 설정하지 않음!
    request.setRawHeader("Connection", "keep-alive");

    QNetworkReply *reply = m_manager->post(request, multiPart);
    multiPart->setParent(reply);

    reply->setProperty("endpoint", endpoint);
    reply->setProperty("method", "POST");
    reply->setProperty("tempTarGzPath", tempTarGzPath);

    qDebug() << "[ApiClient] File POST request sent to:" << endpoint << "->" << tempTarGzPath
             << "size =" << QFileInfo(tempTarGzPath).size() << "bytes";

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(30000);

    connect(timeoutTimer, &QTimer::timeout, this, [reply, endpoint]() {
        if (reply->isRunning()) {
            reply->abort();
            qWarning() << "[ApiClient] File upload timeout:" << endpoint;
        }
    });

    connect(reply, &QNetworkReply::finished, timeoutTimer, [timeoutTimer]() {
        if (timeoutTimer->isActive())
            timeoutTimer->stop();
        timeoutTimer->deleteLater();
    });

    timeoutTimer->start();
}
void ApiClient::download(const QString &endpoint,
                         const QString &destPath,
                         const QString &serialNumber)
{
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply  *reply   = m_manager->get(request);

    reply->setProperty("endpoint", endpoint);
    reply->setProperty("method", "GET");
    reply->setProperty("destPath", destPath);
    reply->setProperty("serialNumber", serialNumber);
    qDebug() << "[ApiClient] Download request sent to:" << endpoint;
    qDebug() << "[ApiClient] Destination path:" << destPath;

    // 다운로드 진행률 모니터링
    connect(reply,
            &QNetworkReply::downloadProgress,
            this,
            [endpoint](qint64 bytesReceived, qint64 bytesTotal) {
                if (bytesTotal > 0) {
                    int progress = (bytesReceived * 100) / bytesTotal;
                    qDebug() << "[ApiClient]" << endpoint << "downloading:" << progress << "%"
                             << "(" << bytesReceived << "/" << bytesTotal << "bytes)";
                } else {
                    // 서버가 Content-Length를 보내지 않는 경우
                    qDebug() << "[ApiClient]" << endpoint << "downloading:" << bytesReceived
                             << "bytes";
                }
            });
}

void ApiClient::onFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    QString endpoint     = reply->property("endpoint").toString();
    QString method       = reply->property("method").toString();
    QString url          = reply->url().toString();
    QString serialNumber = reply->property("serialNumber").toString();

    // 네트워크 에러 확인, 연결이 안됐을 때
    if (reply->error() != QNetworkReply::NoError) {
        QString errorString = reply->errorString();
        qWarning() << "[ApiClient1] Request failed:" << method << endpoint
                   << "Error:" << errorString;

        emit requestFailed(endpoint, errorString, url);
        return;
    }

    // HTTP 상태 코드 확인
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "[ApiClient5] Response received from" << endpoint << "Status code:" << statusCode;

    //서버 응답 실패
    if (statusCode >= 400) {
        QString errorMsg = QString("HTTP %1 error").arg(statusCode);
        qWarning() << "[ApiClient2]" << errorMsg << "for" << endpoint;
        emit requestFailed(endpoint, errorMsg, url);
        return;
    }

    // 업로드 응답 처리
    if (method == "POST" && reply->property("tempTarGzPath").isValid()) {
        QString tempTarGzPath = reply->property("tempTarGzPath").toString();
        QFile::remove(tempTarGzPath);
        qDebug() << "[ApiClient3] Removed temp tarGz file after upload:" << tempTarGzPath;
    }
    // 다운로드 응답 처리
    if (method == "GET" && reply->property("destPath").isValid()) {
        QString destPath = reply->property("destPath").toString();

        // 바이너리 데이터 읽기 (tar.gz 파일)
        QByteArray compressedData = reply->readAll();
        qDebug() << "[ApiClient4] Downloaded compressed file size:" << compressedData.size()
                 << "bytes";

        QString timestamp     = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
        QString tempFileName  = QString("%1_%2.tar.gz").arg(serialNumber).arg(timestamp);
        QString tempTarGzPath = QDir::temp().filePath(tempFileName);

        QFile tempFile(tempTarGzPath);
        if (!tempFile.open(QIODevice::WriteOnly)) {
            qWarning() << "[ApiClient] Failed to create temp file:" << tempTarGzPath
                       << "for robot:" << serialNumber;
            emit requestFailed(endpoint, "Cannot create temp file", url);
            return;
        }

        tempFile.write(compressedData);
        tempFile.close();

        // ---------------------- 압축 풀 폴더 설정 ----------------------
        // 압축 파일명(확장자 제외)을 폴더 이름으로 사용
        QString folderName  = QFileInfo(tempTarGzPath).baseName();  // 예: SN1234_2025-11-06_153022
        QString extractPath = QDir(destPath).filePath(folderName);

        // 같은 이름의 폴더가 이미 존재하면 고유한 이름으로 변경
        QString uniqueExtractPath = extractPath;
        int     counter           = 1;
        while (QDir(uniqueExtractPath).exists()) {
            uniqueExtractPath = QString("%1(%2)").arg(extractPath).arg(counter);
            counter++;
        }

        // 최종 폴더 생성
        QDir dir;
        if (!dir.mkpath(uniqueExtractPath)) {
            qWarning() << "[ApiClient] Failed to create extract directory:" << uniqueExtractPath;
            emit requestFailed(endpoint, "Cannot create extract directory", url);
            QFile::remove(tempTarGzPath);
            return;
        }

        // ---------------------- 압축 해제 ----------------------
        if (!FileCompressor::extractTarGz(tempTarGzPath, uniqueExtractPath)) {
            qWarning() << "[ApiClient] Failed to extract tar.gz file to:" << uniqueExtractPath;
            emit requestFailed(endpoint, "Failed to extract downloaded file", url);
            QFile::remove(tempTarGzPath);
            return;
        }

        // 임시 파일 삭제
        QFile::remove(tempTarGzPath);

        qDebug() << "[ApiClient] Download and extraction completed:" << uniqueExtractPath;
        emit requestSucceeded(endpoint, QJsonObject());
        return;
    }

    // 로봇 상태 응답 처리
    QByteArray responseData = reply->readAll();

    // 로봇 상태 파싱
    if (endpoint == "/api/robot/running" && method == "GET") {
        parseRunningStateResponse(responseData);
    }

    // JSON 파싱
    QJsonParseError parseError;
    QJsonDocument   jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[ApiClient] JSON parse error:" << parseError.errorString();
        emit requestFailed(endpoint, "Invalid JSON response", url);
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 성공 시그널 발생
    emit requestSucceeded(endpoint, jsonObj);
}

void ApiClient::parseRunningStateResponse(const QByteArray &responseData)
{
    QJsonParseError parseError;
    QJsonDocument   jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[ApiClient] Failed to parse robot state response";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // API 응답 형식: { "data": false, "result": 0, "result_msg": "" }
    if (!jsonObj.contains("data")) {
        qWarning() << "[ApiClient] Invalid robot state response: 'data' field missing";
        return;
    }

    bool isRunning = jsonObj.value("data").toBool();

    //“data”가 true 일 경우 전송 불가
    //“data”가 false 일 경우 전송 가능
    //“data”가 null이거나 에러 발생하여 예외 처리 된 경우 전송 불가

    // 상태가 변경된 경우에만 시그널 발생
    if (m_robotRunning != isRunning) {
        m_robotRunning = isRunning;
        qDebug() << "[ApiClient] Robot state changed:" << (m_robotRunning ? "RUNNING" : "IDLE");
        emit robotStateChanged(m_robotRunning);
    }
}

bool ApiClient::postJson(const QString &endpoint, const QJsonObject &body, int timeoutMs)
{
    QNetworkRequest request = createRequest(
            endpoint);  // baseUrl + endpoint, JSON 헤더 설정됨 :contentReference[oaicite:1]{index=1}
    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson());
    reply->setProperty("endpoint", endpoint);
    reply->setProperty("method", "POST");

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(timeoutMs);
    connect(timeoutTimer, &QTimer::timeout, this, [=]() {
        if (reply->isRunning()) {
            reply->abort();
            qWarning() << "[ApiClient] Timeout for" << endpoint;
            emit requestFailed(endpoint, "Timeout - no response", request.url().toString());
        }
        timeoutTimer->deleteLater();
    });
    connect(reply, &QNetworkReply::finished, timeoutTimer, [timeoutTimer]() {
        if (timeoutTimer->isActive())
            timeoutTimer->stop();
        timeoutTimer->deleteLater();
    });
    timeoutTimer->start();
    return true;  // 비동기. 성공/실패는 onFinished에서 emit됨 :contentReference[oaicite:2]{index=2}
}

bool ApiClient::postWorkspaceCompress(const QString &user)
{
    QJsonObject j;
    j["user"] = user;
    return postJson("/api/workspace/compress", j);
}

//패스워드도 빼도 된다.
bool ApiClient::postWorkspaceExtract(const QString &user, const QString &password)
{
    QJsonObject j;
    j["user"]     = user;
    j["password"] = password;
    return postJson("/api/workspace/extract", j);
}
