#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H
#pragma once
#include <QList>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>

#include "ApiClient.h"
#include "PasswordManager.h"

struct ControllerInfo
{
    QString serialNumber;
    QString ip;
    int     sftpPort;
    QString username;
    QString pswd;
    QString wsPath;
    QString birth;
    bool    isConnected;
    bool    isRunning;

    ControllerInfo() : sftpPort(22), isConnected(false), isRunning(false) {}
};

class ControllerManager : public QObject
{
    Q_OBJECT

      public:
        static ControllerManager *instance();
        PasswordManager          *passwordManager();
      public slots:
        void onMasterPasswordChanged();

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
        void saveToFile(const QString &serialNumber = "");
        void loadFromFile();

        // 개별 제어기 저장/로드
        bool saveController(const ControllerInfo &controller);
        bool loadController(const QString &serialNumber);

        // 전체 제어기 목록 관리
        void saveControllerList();
        void loadControllerList();

        bool backupRequest(const QString &serialNumber, const QString &baseBackupDir);
        bool applyRequest(
                const QString &serialNumber, const QString &filePath, const QString &apiPassword);

        bool receive(const QString &serialNumber,
                     const QString &remoteTarGz,
                     const QString &parentDir,
                     const QString &localDestDir);
        bool send(const QString     &serialNumber,
                  const QStringList &localPaths,
                  const QString     &remoteDir);

      signals:
        void controllerListChanged();
        void controllerStateUpdated(const QString &serialNumber, bool isConnected, bool isRunning);
        void backupCompleted(const QString &serialNumber);
        void backupFailed(const QString &serialNumber, const QString &error);

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
        PasswordManager           *pm_;
        mutable QMutex             mutex_;
};

#endif  // CONTROLLERMANAGER_H
