#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H
#include <QList>
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
        workspacePath("/home/robot_user/workspace"),
        isConnected(false),
        isRunning(false)
    {
    }
};

class ControllerManager
{
  public:
    ControllerManager();

    bool isConnected();         //제어기와 연결 되어있는지 확인하는 함수
    bool isRunning();           //제어기의 동작 확인 하는 함수
    int  sendFile();            //제어기에게 파일을 전송하는 함수
    void registerController();  //제어기 등록하는 함수
    void removeController();    //제어기 등록 해제하는 함수
    void updateInfo();          //제어기 정보 수정하기
    void updateState();         //전체 제어기 상태 갱신

    void loadState();

  private:
    QList<ControllerInfo> controllers;
};

#endif  // CONTROLLERMANAGER_H
