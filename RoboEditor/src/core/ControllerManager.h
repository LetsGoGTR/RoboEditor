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
    QString pswd;
    bool    isConnected;
    bool    isRunning;

    ControllerInfo() : sftpPort(22), apiPort(80), isConnected(false), isRunning(false) {}
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

        //그냥 호출해도 괜찮
        void backupRequest(const QString &serialNumber);

        //여러개 한번에 보내면 안돼서 보낼 제어기 리스트를 큐로 관리해서 하나씩 보내고 완료되면 다음꺼 보내야함
        void applyRequest(const QString &serialNumber, const QString &filePath);

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
        QMap<QString, ApiClient *> apiClients_;
        mutable QMutex             mutex_;
};

#endif  // CONTROLLERMANAGER_H
