#pragma once

#include <QNetworkAccessManager>
#include <QObject>

#include <functional>
#include <json/json.h>
#include <string>

namespace utils
{
    class RobotHttpClient : public QObject
    {
        Q_OBJECT

          public:
            explicit RobotHttpClient(QObject *parent = nullptr);
            ~RobotHttpClient();

            using BoolCallback   = std::function<void(bool, const std::string &)>;
            using StringCallback = std::function<void(const std::string &, const std::string &)>;
            using JsonCallback   = std::function<void(const Json::Value &, const std::string &)>;

            void checkRunning(const std::string &ip, BoolCallback callback);
            void uploadWorkspace(
                    const std::string &ip, const std::string &filePath, BoolCallback callback);
            void downloadWorkspace(const std::string &ip, StringCallback callback);
            void getJsonResponse(
                    const std::string &ip, const std::string &endpoint, JsonCallback callback);

          private:
            QNetworkAccessManager *m_networkManager;

            static constexpr const char *RUNNING_ENDPOINT = "/api/v1/robot/running";
            static constexpr const char *IMPORT_ENDPOINT  = "/api/v1/robot/import";
            static constexpr const char *EXPORT_ENDPOINT  = "/api/v1/robot/export";
    };

}  // namespace utils
