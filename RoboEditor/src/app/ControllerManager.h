#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>
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

    ControllerInfo() :
        sftpPort(22),
        apiPort(8080),
        workspacePath("/home/samsung/workspace3"),
        isConnected(false),
        isRunning(false)
    {
    }
};

class ControllerManager : public QObject
{
    Q_OBJECT
      public:
        static ControllerManager *instance();
        explicit ControllerManager(QObject *parent = nullptr);
        ~ControllerManager();

        //bool isConnected();                              // 제어기 연결 여부 확인
        //bool isRunning();                                // 제어기 동작 상태 확인
        bool isDuplicatedSN(const QString &SN);          // 중복 SN 검사
        int  sendFile();                                 // 제어기로 파일 전송
        void registerController();                       // 제어기 등록
        void removeController(int index);                // 인덱스로 제어기 삭제
        void removeController(const ControllerInfo *c);  // 포인터로 제어기 삭제
        void updateInfo(const ControllerInfo &newInfo);  // 정보 수정
        void updateState();                              // 상태 갱신
        void saveToFile(const QString &filePath = "");
        void loadFromFile(const QString &filePath = "");

        const QList<ControllerInfo> &getControllers() const
        {
            return controllers_;
        }

      private:
        ControllerManager(const ControllerManager &)            = delete;  // 복사 금지
        ControllerManager &operator=(const ControllerManager &) = delete;  // 대입 금지
        QString            configFilePath_;
      signals:
        void controllerListChanged();

      private:
        QList<ControllerInfo> controllers_;
        mutable QMutex        mutex_;
};

#endif  // CONTROLLERMANAGER_H
