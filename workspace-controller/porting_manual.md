### 1. 프로그램 복사
workspace-controller 바이너리 파일을 제어기에 복사합니다.

### 2. 데몬 작동을 위한 서비스 파일 작성
제어기 환경에 맞춰 데몬으로 서버사이드 프로그램을 구동하기 위해 서비스 파일에 대한 수정이 필요합니다.
```
[Unit]
Description=Workspace Controller Service
After=network.target

[Service]
User=
WorkingDirectory=
ExecStart=
Restart=always
LogsDirectory=workspace-controller

[Install]
WantedBy=multi-user.target
```

1. User :\
    유저 이름을 입력합니다. sftp 서버에서 사용할 유저와 동일하게 설정해주시면 됩니다.\
    (동일하게 설정하지 않을경우 압축/압축해제 후 파일 소유자가 달라져 sftp 파일 접근이 불가능할 수 있습니다.)\
    ex) test
2. WorkingDirectory :\ 
    workspace-controller 바이너리 파일이 들어있는 폴더의 절대 경로를 넣어줍니다.\
    ex) /home/test/daemon/
3. ExecStart :\
    workspace-controller 바이너리 파일의 절대 경로를 넣어줍니다.\
    경로 뒤에 1 ~ 65535 범위의 정수를 실행할 포트로 지정할 수 있습니다.\
    지정하지 않은 경우 포트는 기본값(8888)으로 지정 됩니다.\
    ex) /home/test/daemon/workspace-controller 9999
4. LogsDirectory :\
    로그 파일이 저장될 디렉토리 이름입니다.\
    systemd가 자동으로 /var/log/workspace-controller 폴더를 생성하고 User에게 권한을 부여합니다.\
    로그 파일 위치: /var/log/workspace-controller/server.log

### 3. 실행

바이너리 파일에 실행 권한 부여 :
```bash
chmod +x workspace-controller
```

서비스 파일 이동 :
```bash
sudo cp workspace-controller.service /etc/systemd/system/
```

데몬 실행 :
```bash
sudo systemctl daemon-reload
sudo systemctl enable workspace-controller.service
sudo systemctl start workspace-controller.service
```

### 5. 포트 개방
ExecStart에서 지정한 포트 또는 기본 포트(8888)를 외부 개방해주시면 됩니다.

### 6. 서비스 관리

바이너리 교체시:
```bash
sudo systemctl restart workspace-controller.service
```

서비스 파일 교체시:
```bash
sudo cp workspace-controller.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl restart workspace-controller.service
```

상태/로그 확인:
```bash
sudo systemctl status workspace-controller.service
tail -f /var/log/workspace-controller/server.log
```
