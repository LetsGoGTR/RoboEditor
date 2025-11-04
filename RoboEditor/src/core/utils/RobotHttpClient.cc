#include "RobotHttpClient.h"

#include <QFile>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <sstream>

namespace utils
{
    RobotHttpClient::RobotHttpClient(QObject *parent) :
        QObject(parent),
        m_networkManager(new QNetworkAccessManager(this))
    {
    }

    RobotHttpClient::~RobotHttpClient() = default;

    void RobotHttpClient::checkRunning(const std::string &ip, BoolCallback callback)
    {
        std::string     url = "http://" + ip + RUNNING_ENDPOINT;
        QUrl            qUrl(QString::fromStdString(url));
        QNetworkRequest request(qUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = m_networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
            if (reply->error() != QNetworkReply::NoError) {
                callback(false, "Failed to connect to robot");
                reply->deleteLater();
                return;
            }

            QByteArray  responseData = reply->readAll();
            std::string jsonStr(responseData.constData(), responseData.size());

            Json::Value             jsonData;
            Json::CharReaderBuilder reader;
            std::string             errs;
            std::istringstream      s(jsonStr);

            if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
                callback(false, "Invalid JSON response");
                reply->deleteLater();
                return;
            }

            if (!jsonData.isMember("data")) {
                callback(false, "Invalid response from robot");
                reply->deleteLater();
                return;
            }

            bool isRunning = jsonData["data"].asBool();
            callback(isRunning, "");
            reply->deleteLater();
        });
    }

    void RobotHttpClient::uploadWorkspace(const std::string &ip,
                                          const std::string &filePath,
                                          BoolCallback       callback)
    {
        QFile file(QString::fromStdString(filePath));
        if (!file.open(QIODevice::ReadOnly)) {
            callback(false, "Failed to read file: " + filePath);
            return;
        }

        std::string     url = "http://" + ip + IMPORT_ENDPOINT;
        QUrl            qUrl(QString::fromStdString(url));
        QNetworkRequest request(qUrl);

        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);

        QHttpPart filePart;
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QVariant("form-data; name=\"file\"; filename=\"workspace.tar.gz\""));
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/gzip"));
        filePart.setBodyDevice(&file);
        file.setParent(multiPart);

        multiPart->append(filePart);

        QNetworkReply *reply = m_networkManager->post(request, multiPart);
        multiPart->setParent(reply);

        connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
            if (reply->error() != QNetworkReply::NoError) {
                callback(false, "Failed to upload to robot");
                reply->deleteLater();
                return;
            }

            QByteArray  responseData = reply->readAll();
            std::string jsonStr(responseData.constData(), responseData.size());

            Json::Value             jsonData;
            Json::CharReaderBuilder reader;
            std::string             errs;
            std::istringstream      s(jsonStr);

            if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
                callback(false, "Invalid JSON response");
                reply->deleteLater();
                return;
            }

            if (!jsonData.isMember("success")) {
                callback(false, "Invalid response from robot");
                reply->deleteLater();
                return;
            }

            bool success = jsonData["success"].asBool();
            if (!success) {
                std::string errorMsg = "Upload rejected by robot";
                if (jsonData.isMember("message")) {
                    errorMsg = jsonData["message"].asString();
                }
                callback(false, errorMsg);
                reply->deleteLater();
                return;
            }

            callback(true, "");
            reply->deleteLater();
        });
    }

    void RobotHttpClient::downloadWorkspace(const std::string &ip, StringCallback callback)
    {
        std::string     url = "http://" + ip + EXPORT_ENDPOINT;
        QUrl            qUrl(QString::fromStdString(url));
        QNetworkRequest request(qUrl);

        QNetworkReply *reply = m_networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
            if (reply->error() != QNetworkReply::NoError) {
                callback("", "Failed to download from robot");
                reply->deleteLater();
                return;
            }

            QByteArray content = reply->readAll();
            if (content.isEmpty()) {
                callback("", "Empty response from robot");
                reply->deleteLater();
                return;
            }

            callback(std::string(content.constData(), content.size()), "");
            reply->deleteLater();
        });
    }

    void RobotHttpClient::getJsonResponse(const std::string &ip,
                                          const std::string &endpoint,
                                          JsonCallback       callback)
    {
        std::string     url = "http://" + ip + endpoint;
        QUrl            qUrl(QString::fromStdString(url));
        QNetworkRequest request(qUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = m_networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
            if (reply->error() != QNetworkReply::NoError) {
                callback(Json::Value(), "Failed to connect to robot");
                reply->deleteLater();
                return;
            }

            QByteArray  responseData = reply->readAll();
            std::string jsonStr(responseData.constData(), responseData.size());

            Json::Value             jsonData;
            Json::CharReaderBuilder reader;
            std::string             errs;
            std::istringstream      s(jsonStr);

            if (!Json::parseFromStream(reader, s, &jsonData, &errs)) {
                callback(Json::Value(), "Invalid JSON response");
                reply->deleteLater();
                return;
            }

            callback(jsonData, "");
            reply->deleteLater();
        });
    }

}  // namespace utils
