#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <QList>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>

#include "ApiClient.h"

struct ControllerInfo
{
    QString serialNumber;
    QString ip;
    int     sftpPort;
    int     apiPort;
    QString username;
    QString workspacePath;
    bool    isConnected;
    bool    isRunning;

    ControllerInfo() : sftpPort(22), apiPort(8080), isConnected(false), isRunning(false) {}
};

class ControllerManager : public QObject
{
    Q_OBJECT

      public:
        static ControllerManager *instance();

        // 제어기 관리
        void registerController();
        void removeController(int index);
        void removeController(const ControllerInfo *curCon);
        void updateInfo(const ControllerInfo &newInfo);

        // 상태 업데이트
        void updateControllersStates();  // 모든 제어기 즉시 상태 체크

        // 제어기 정보 조회
        QList<ControllerInfo> getControllers() const;
        ControllerInfo        getController(const QString &serialNumber) const;
        ApiClient            *getApiClient(const QString &serialNumber);

        // 파일 저장/로드
        void saveToFile(const QString &filePath = "");
        void loadFromFile(const QString &filePath = "");

      signals:
        void controllerListChanged();
        void controllerStateUpdated(const QString &serialNumber, bool isConnected, bool isRunning);

      private:
        explicit ControllerManager(QObject *parent = nullptr);
        ~ControllerManager();

        ControllerManager(const ControllerManager &)            = delete;
        ControllerManager &operator=(const ControllerManager &) = delete;

        bool isDuplicatedSN(const QString &SN);

        // ApiClient 관리
        void setupApiClient(const QString &serialNumber);
        void cleanupApiClient(const QString &serialNumber);

        // 상태 업데이트 (thread-safe)
        void updateRunningState(const QString &serialNumber, bool running);
        void updateConnectionState(const QString &serialNumber, bool connected);

        QString                    configFilePath_;
        QList<ControllerInfo>      controllers_;
        QMap<QString, ApiClient *> apiClients_;  // serialNumber -> ApiClient
        mutable QMutex             mutex_;
};

#endif  // CONTROLLERMANAGER_H
