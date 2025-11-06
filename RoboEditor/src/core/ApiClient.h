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
        void upload(const QString &endpoint, const QString &filePath);
        void download(
                const QString &endpoint, const QString &destPath, const QString &serialNumber);

        static QString normalizeBaseUrl(const QString &baseUrl);
        QString        getBaseUrl() const
        {
            return m_baseUrl;
        }

      signals:
        void robotStateChanged(bool isRunning);  //로봇의 상태가 변화했을 때 신호
        void requestSucceeded(const QString     &endpoint,
                              const QJsonObject &response);  //요청이 성공했을 때 신호
        void requestFailed(const QString &endpoint,
                           const QString &errorString,
                           const QString &url);  //요청이 실패했을 때 신호

      private slots:
        void onFinished(QNetworkReply * reply);

      private:
        QNetworkRequest createRequest(const QString &endpoint);
        void            parseRunningStateResponse(const QByteArray &responseData);

        QNetworkAccessManager *m_manager;
        QString                m_baseUrl;
        bool                   m_robotRunning;

        // 메모리 최적화: 재사용 가능한 객체들
        QNetworkRequest m_reusableRequest;
};

#endif  // API_CLIENT_H
