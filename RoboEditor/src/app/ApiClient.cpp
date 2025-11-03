#include "ApiClient.h"

#include <QDebug>
#include <QJsonParseError>

ApiClient::ApiClient(const QString &baseUrl, QObject *parent) :
    QObject(parent),
    m_manager(new QNetworkAccessManager(this)),
    m_baseUrl(baseUrl),
    m_pollingTimer(new QTimer(this)),
    m_robotRunning(false)
{
    // QNetworkAccessManager finished 시그널 연결
    connect(m_manager, &QNetworkAccessManager::finished, this, &ApiClient::onFinished);

    // 폴링 타이머 연결
    connect(m_pollingTimer, &QTimer::timeout, this, &ApiClient::onPollingTimeout);

    qDebug() << "[ApiClient] Initialized with base URL:" << m_baseUrl;
}

ApiClient::~ApiClient()
{
    stopPolling();
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

//제어기의 connected, running 업데이트
void ApiClient::checkRobotRunning()
{
    get("/api/robot/running");
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

//서버사이드에서 데이터 전송
void ApiClient::post(const QString &endpoint, const QJsonObject &data)
{
    QNetworkRequest request  = createRequest(endpoint);
    QByteArray      jsonData = QJsonDocument(data).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_manager->post(request, jsonData);
    reply->setProperty("endpoint", endpoint);
    reply->setProperty("method", "POST");

    qDebug() << "[ApiClient] POST request sent to:" << endpoint;
}

void ApiClient::startPolling(int intervalMs)
{
    if (m_pollingTimer->isActive()) {
        qWarning() << "[ApiClient] Polling already started";
        return;
    }

    qDebug() << "[ApiClient] Starting polling with interval:" << intervalMs << "ms";

    // 즉시 한 번 상태 확인
    checkRobotRunning();

    // 주기적 폴링 시작
    m_pollingTimer->start(intervalMs);
}

void ApiClient::stopPolling()
{
    if (m_pollingTimer->isActive()) {
        m_pollingTimer->stop();
        qDebug() << "[ApiClient] Polling stopped";
    }
}

void ApiClient::onPollingTimeout()
{
    // 타이머 주기마다 로봇 상태 확인
    checkRobotRunning();
}

void ApiClient::onFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    QString endpoint = reply->property("endpoint").toString();
    QString method   = reply->property("method").toString();
    QString url      = reply->url().toString();

    // 네트워크 에러 확인, 연결이 안됐을 때, 아이콘 x
    if (reply->error() != QNetworkReply::NoError) {
        QString errorString = reply->errorString();
        qWarning() << "[ApiClient] Request failed:" << method << endpoint
                   << "Error:" << errorString;

        emit requestFailed(endpoint, errorString, url);
        return;
    }

    // HTTP 상태 코드 확인
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "[ApiClient] Response received from" << endpoint << "Status code:" << statusCode;

    //연결은 됐지만 서버 응답 실패 :  아이콘 : 주황
    if (statusCode >= 400) {
        QString errorMsg = QString("HTTP %1 error").arg(statusCode);
        qWarning() << "[ApiClient]" << errorMsg << "for" << endpoint;
        emit requestFailed(endpoint, errorMsg, url);
        return;
    }

    // 응답 본문 읽기
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
