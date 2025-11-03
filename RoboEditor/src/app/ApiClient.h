#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <QTimer>

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

// 로봇의 상태를 주기적으로 확인하는 역할

class ApiClient : public QObject
{
    Q_OBJECT

      public:
        explicit ApiClient(const QString &baseUrl, QObject *parent = nullptr);
        ~ApiClient();

        //api요청
        void get(const QString &endpoint);
        void post(const QString &endpoint, const QJsonObject &data);

        //polling   :   5초, 5초마다 상태 확인
        void checkRobotRunning();
        void startPolling(int intervalMs = 5000);
        void stopPolling();

      signals:
        void robotStateChanged(bool isRunning);  //로봇의 상태가 변화했을 때 신호
        void requestSucceeded(const QString     &endpoint,
                              const QJsonObject &response);  //요청이 성공했을 때 신호
        void requestFailed(const QString &endpoint,
                           const QString &errorString,
                           const QString &url);  //요청이 실패했을 때 신호

      private slots:
        void onFinished(QNetworkReply * reply);
        void onPollingTimeout();

      private:
        QNetworkRequest createRequest(const QString &endpoint);
        void            parseRunningStateResponse(const QByteArray &responseData);

        QNetworkAccessManager *m_manager;
        QString                m_baseUrl;
        QTimer                *m_pollingTimer;
        bool                   m_robotRunning;

        // 메모리 최적화: 재사용 가능한 객체들
        QNetworkRequest m_reusableRequest;
};

#endif  // API_CLIENT_H
