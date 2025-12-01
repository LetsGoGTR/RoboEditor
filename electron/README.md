# 폴더 구조
```
root/
├─ app/
│  ├─ index.html
│  ├─ roboeditor-icon.ico
│  └─ {프론트 빌드 파일}
│
├─ back/
│  ├─ Drogon.exe / ELF
│  ├─ config.json
│  └─ {라이브러리}
│
├─ node_modules/
│
├─ build-auto.js
├─ index.html
├─ index.js
├─ package.json
├─ package-lock.json
├─ preload.js
└─ README.md
```

## root/ 
Electron 설정 파일

- `build-auto.js`  
  - **Electron 패키징 스크립트**
  - `back/`의 실행파일 확인 후 `electron-builder` 실행
  - Windows portable EXE → dist/win
  - Linux AppImage → dist/linux

- `index.html`  
  - Electron이 로드하는 기본 HTML 셸
  - 현재 사용하지 않는 파일(테스트 용도)

- `index.js`  
  - **백엔드 → 프론트엔드 실행하는 파이프라인**
  - Electron 메인 프로세스 진입점 (가장 먼저 실행되는 파일)
  - 경로 계산하여 Drogon 백엔드를 실행
  - 포트에 접속하여 프론트엔드를 보여주는 창 생성
  - 창 종료 시 종료 코드 실행
  - `BrowserWindow` 생성, `app` 폴더의 프론트엔드 로딩, 앱 종료 처리 등 담당

- `package.json`  
  - **프로젝트 메타 정보 + Electron/Electron-builder 설정 + 스크립트 정의**
  - `"main": "index.js"`로 Electron 메인 진입점을 지정
  - `"scripts"`에 `npm run dev`, `npm run build` 등의 명령 정의
  - 빌드 시 폴더 구조 구성

- `package-lock.json`  
  - npm 의존성 버전 스냅샷 파일
  - 배포 환경에서 동일한 버전의 패키지를 설치하기 위한 용도
  - 패키지 변경 시 npm이 자동으로 갱신하며, 일반적으로 직접 수정하지 않습니다. 

- `preload.js`  
  - 현재 사용하지 않는 파일

- `README.md`  
  - 프로젝트 설명 문서
  

## app/

Svelte/SvelteKit로 빌드한 **프론트엔드(화면 UI)** 정적 파일들이 들어 있는 디렉터리입니다.
새 버전(프론트) 적용 시 폴더 내부 전부 교체 후 빌드합니다.

- `app/index.html`  
  - 프론트엔드 진입점 HTML 파일  
  - Drogon 서버에서 이 파일을 서빙하고 `BrowserWindow`가 Url 표시합니다.

- `app/roboeditor-icon.ico`  
  - 애플리케이션 아이콘 파일  
  - electron-builder로 EXE를 만들 때 사용되며, 작업 표시줄/시작 메뉴 아이콘으로 사용됩니다.

- `app/{프론트 빌드 파일}`  
  - 번들된 JS/CSS, 이미지, 폰트 등 프론트엔드 빌드 결과물  
  - 파일 이름은 빌드시 해시 값 등으로 자동 생성되며, 일반적으로 직접 수정하지 않습니다.


## back/

C++ Drogon으로 만든 **백엔드 서버 실행 파일**과 그에 필요한 **런타임 라이브러리**가 들어 있는 디렉터리입니다.  
프론트엔드는 이 백엔드와 HTTP/API로 통신합니다.
새 버전(백) 적용 시 폴더 실행 파일 교체 후 빌드합니다.

- `back/Drogon.exe` / `back/Drogon` (ELF)  
  - Drogon 기반 백엔드 서버 실행 파일  
  - diff/backup, SFTP, 파일 비교 등 주요 비즈니스 로직을 처리합니다.
  - Window, Linux 실행파일을 각각 빌드합니다.
  - Electron 앱 실행 시 함께 실행되어 API 서버 역할을 합니다.

- `back/config.json`  
  - Drogon 서버 설정 파일  
  - 사용하는 포트, 로그 설정, 정적 파일 경로 등 백엔드 동작에 필요한 설정을 정의합니다.

- `back/{라이브러리}`  
  - `Drogon.exe`가 실행되기 위해 필요한 DLL/so 파일 모음  
  - 예: `libcurl`, `libarchive`, `libssh2`, `jsoncpp`, `yaml-cpp`, `openssl` 등  
  - 일반적으로 직접 수정하지 않지만 라이브러리 변경이 있을 경우 `back`폴더에 직접 변경이 필요합니다.


## node_modules/

`npm init -y`, `npm install electron electron-builder cross-env` 시 자동으로 생성되며, 일반적으로 직접 수정하지 않습니다.


# Electron 빌드 방법

1. Electron 폴더 생성  (`root`, `electron`)
2. 패키징 실행 폴더 복사
3. node_modules 설치 (`npm install`)
4. Svelte => 정적 파일로 빌드 후 app/에 결과 복사
5. Drogon => Window/Linux 빌드 후 back/에 실행 파일 복사
6. npm run build

## 빌드 주의사항

- Node.js LTS 버전 이상이 설치되어 있어야 합니다.
- `npm install`: node_modules 설치에 필요합니다.
- `npm run build`: Electron 빌드 시 필요합니다.
- 실행 전 `app/`과 `back/`에 빌드된 결과가 필요합니다.
- 심볼릭 링크의 문제로 관리자 권한이 필요 할 수도 있습니다.


# 결과물

## 결과물 구조

### Window
```
dist/
└─ win/
   ├─ RoboEditor-1.0.0.exe          # 사용자에게 배포하는 실제 실행파일(Portable)
   ├─ win-unpacked/                 # 언팩된 앱 폴더
   └─ builder-effective-config.yaml # electron-builder가 사용한 최종 설정 (디버깅용)
```

### Linux
```
dist/
└─ linux/
   ├─ RoboEditor-1.0.0.AppImage    # 사용자에게 배포하는 파일
   └─ linux-unpacked/              # 언팩된 앱 폴더
```

## RoboEditor-1.0.0

- Electron + 프론트엔드 + 백엔드를 모두 포함한 최종 실행파일
- 임시 폴더(Temp)에 자기 내용을 풀고 Electron 바이너리를 실행해서 앱을 킵니다.
- **사용자는 실행파일만 실행하면 됩니다.**
## unpacked

- Electron Builder가 `target: "portable"`(Windows) 또는 `target: "AppImage"`(Linux)로 빌드할 때,
  최종 실행 파일과 함께 자동으로 생성하는 폴더입니다.
- 위치는 `build.directories.output` 설정과 `-c.directories.output=...` 옵션에 따라 결정되며,
  기본적으로는:
  - Windows: `dist/win/win-unpacked/`
  - Linux : `dist/linux/linux-unpacked/`
  로 생성됩니다.
- 이 폴더는 개발자가 “최종 패키지 안에 어떤 파일이 포함되었는지” 확인할 때 사용합니다.
- **일반 사용자에게 배포할 필요는 없습니다.**

### builder-effective-config.yaml

- Electron Builder가 빌드 시점에 사용한 **최종 설정값**을 덤프한 디버깅용 파일입니다.
- `package.json`의 `build` 설정과, CLI 옵션(`-c.xxx=...`)이 합쳐진 결과를 확인할 수 있습니다.
- 위치는 보통 `dist/win/` 아래에 생성되며, 빌드 설정 문제를 디버깅할 때에만 사용합니다.
- **일반 사용자에게 배포할 필요는 없습니다.**



# 설정 변경 시 주의사항

### 1. 내부 연결 포트를 변경하고 싶을 때

이 앱은 **Drogon 백엔드가 먼저 포트에 서버를 띄우고**, Electron이 그 포트로 접속해서 화면을 띄우는 구조입니다.  
따라서 포트를 바꾸려면 **백엔드 설정과 Electron 설정을 함께** 수정해야 합니다.

- 수정해야 하는 파일
  - `index.js`
    - 최상단에 있는 상수:
      ```js
      const BACKEND_PORT = 5555;
      ```
    - 이 값을 원하는 포트 번호로 변경합니다. (예: `8080`, `9000` 등)
  - `back/config.json`
    - Drogon의 `listeners` 설정에서 **같은 포트 번호**로 변경합니다.

- 주의사항
  - 두 파일의 포트가 다르면:
    - Drogon은 A번 포트에서 기다리고,
    - Electron은 B번 포트로 접속하려고 해서
    - 프론트 화면이 뜨지 않고 로딩 실패/에러 박스가 뜹니다.
  - 이미 다른 프로그램이 사용 중인 포트로 바꾸면 백엔드가 실행에 실패할 수 있습니다.

### 2. 백엔드 실행 파일 이름을 바꾸고 싶을 때 (`Drogon.exe`, `Drogon`)

현재 구조에서는 백엔드 실행 파일 이름을 아래처럼 **하드코딩**해서 사용합니다.

- `back/Drogon.exe`  (Windows)
- `back/Drogon`      (Linux, ELF)

이름을 바꾸고 싶다면 **아래 세 곳을 함께** 수정해야 합니다.

- 수정해야 하는 파일

  1. **`build-auto.js`**
     ```js
     const winExe   = path.join(backDir, 'Drogon.exe');
     const linuxElf = path.join(backDir, 'Drogon');
     ```
     - 여기서 파일명을 새 이름으로 변경합니다.
     - 예: `RoboBackend.exe`, `RoboBackend` 등으로 바꾼다면:
       ```js
       const winExe   = path.join(backDir, 'RoboBackend.exe');
       const linuxElf = path.join(backDir, 'RoboBackend');
       ```


  2. **`index.js` (백엔드 경로 계산 부분)**
     ```js
     function getBackendPath() {
       if (app.isPackaged) {
         return path.join(
           process.resourcesPath,
           'back',
           process.platform === 'win32' ? 'Drogon.exe' : 'Drogon'
         );
       } else {
         return path.join(
           __dirname,
           'back',
           process.platform === 'win32' ? 'Drogon.exe' : 'Drogon'
         );
       }
     }
     ```
     - 여기서도 동일한 새 이름으로 변경해야 합니다.
     - 예:
       ```js
       process.platform === 'win32' ? 'RoboBackend.exe' : 'RoboBackend'
       ```

  3. **`back/` 폴더 실제 파일명**
     - 빌드된 실행 파일 자체 이름도 위에서 지정한 이름으로 맞춰야 합니다.
     - 예:
       - Windows: `back/RoboBackend.exe`
       - Linux: `back/RoboBackend`

- 주의사항
  - 세 곳 중 하나라도 이름이 다르면:
    - `build-auto.js`에서 실행 파일을 못 찾아 빌드가 실패하거나,
    - Electron이 런타임에 백엔드를 실행하지 못합니다.


### 3. 빌드 결과 경로를 바꾸고 싶을 때 (`dist/win`, `dist/linux`)

기본값은 다음과 같이 설정되어 있습니다.

- Windows: `dist/win`
- Linux: `dist/linux`

실제 지정은 `build-auto.js`에서 `electron-builder` 옵션으로 이루어집니다.

- 수정해야 하는 파일

  - **`build-auto.js`**
    ```js
    // Windows 빌드
    const resWin = spawnSync(
      'electron-builder',
      ['--win', 'portable', '-c.directories.output=dist/win'],
      commonOpts
    );

    // Linux 빌드
    const resLinux = spawnSync(
      'electron-builder',
      ['--linux', 'AppImage', '-c.directories.output=dist/linux'],
      commonOpts
    );
    ```

  - 출력 위치를 바꾸고 싶다면 `dist/win`, `dist/linux` 부분을 원하는 경로로 변경합니다.
    - 예:  
      - Windows → `out/win`  
      - Linux  → `out/linux`
    - 변경 예시:
      ```js
      '-c.directories.output=out/win'
      '-c.directories.output=out/linux'
      ```

- 참고
  - `package.json`의 `"build.directories.output": "dist"` 설정도 존재하지만,  
    `build-auto.js`에서 CLI 옵션으로 다시 지정해주기 때문에 최종 결과는 여기서 주는 값이 우선됩니다.
  - README에서 “빌드 결과 위치”를 안내할 때 이 경로와 맞춰 주면 됩니다.


### 4. 아이콘 이름을 바꾸고 싶을 때 (`roboeditor-icon.ico`)

아이콘 파일은 다음 위치/설정에서 사용됩니다.

- 실제 파일:  
  - `app/roboeditor-icon.ico`
- Electron 창 아이콘:
  - `index.js` → `getIconPath()`에서 사용
- 빌드 설정(설치/실행 파일 아이콘):
  - `package.json` → `"build.icon"`

아이콘 파일 이름을 바꾸고 싶다면 **아래 세 곳을 함께** 수정해야 합니다.

- 수정해야 하는 파일

  1. **`app/` 폴더 실제 아이콘 파일 이름**
     - 예: `app/my-icon.ico` 로 변경

  2. **`index.js` (`getIconPath` 함수)**
     ```js
     function getIconPath() {
       return path.join(process.resourcesPath, 'app', 'roboeditor-icon.ico');
     }
     ```
     - 여기서 파일명을 새 이름으로 변경합니다.
     - 예:
       ```js
       function getIconPath() {
         return path.join(process.resourcesPath, 'app', 'my-icon.ico');
       }
       ```

  3. **`package.json` (electron-builder 설정)**
     ```json
     "build": {
       "icon": "app/roboeditor-icon.ico",
     }
     ```
     - 여기도 동일한 경로로 변경합니다.
     - 예:
       ```json
       "icon": "app/my-icon.ico"
       ```

- 주의사항
  - 파일 위치(`app/`)를 바꾸면 `index.js`와 `package.json` 둘 다 경로를 맞춰줘야 합니다.


### 5. 산출물의 파일명을 바꾸고 싶을 때 (`RoboEditor-1.0.0.exe`)

Electron Builder가 기본적으로 파일명을 `${productName}-${version}.${ext}`로 사용합니다.
- productName → `package.json`의 build.productName
- version → `package.json`의 version
- ext → 타겟 포맷에 따라 달라짐
  - Windows portable → exe
  - Linux AppImage → AppImage
현재 설정 기준 기본 파일명은 다음과 같은 형태로 생성됩니다.
- Windows: `RoboEditor-1.0.0.exe`
- Linux : `RoboEditor-1.0.0.AppImage`
  
산출물(EXE/AppImage)의 파일명을 바꾸고 싶다면,  
`package.json`의 `build` 설정에서 `artifactName`을 사용합니다.

- 수정해야 하는 파일

    - **`package.json`**

        결과물을 `RoboEditor_1.0.0_win.exe`, `RoboEditor_1.0.0_linux.AppImage` 형태로 만들고 싶다면:

        ```jsonc
        "build": {
        "productName": "RoboEditor",
        "win": {
            "target": ["portable"],
            "artifactName": "${productName}_${version}_win.${ext}"
        },
        "linux": {
            "target": ["AppImage"],
            "artifactName": "${productName}_${version}_linux.${ext}"
        }
        }
        ```

    - 사용할 수 있는 대표 변수:
       - `${productName}` : `build.productName`
       - `${version}`     : `package.json`의 `version`
       - `${ext}`         : 타겟에 따라 자동으로 `exe`, `AppImage` 등으로 치환

- 주의사항
  - `artifactName`은 **산출물 파일명만 바꾸는 설정**이고,  
    내부 앱 이름(`productName`)이나 동작에는 영향을 주지 않습니다.
  - `build-auto.js`에서 지정한 출력 경로(`dist/win`, `dist/linux`)는 그대로 유지되고,  
    그 안에 생성되는 실제 파일 이름만 바뀝니다.
  - 배포할 때 사용자가 받는 파일이 바로 이 `artifactName` 패턴으로 만들어진 EXE/AppImage 입니다.




# Svelte 빌드 방법
1. npm install
2. npm run build
3. 빌드 폴더의 내용을 `app/`으로 복사
   
# Drogon 빌드 방법
- **Docker(개발용)**
   1. `docker-compose.yml` svelte 빌드 파일 경로 지정
   2. main 수정
        ```
        #include <drogon/drogon.h>
        int main()
        {
            // Set HTTP listener address and port
            drogon::app().addListener("0.0.0.0", 5555);
            // Load config file
            drogon::app().loadConfigFile("../config.json");
            // Run HTTP framework,the method will block in the internal event loop
            drogon::app().run();
        }
        ```
   3. utils/ConfigUtils.h 수정
        ```
        inline std::string getBaseDir()
        {
            return drogon::app().getCustomConfig()["storage"]["base_dir"].asString();
        }
        inline std::string getTempUploadDir()
        {
            return drogon::app().getCustomConfig()["storage"]["temp_upload_dir"].asString();
        }
        inline std::string getTempExportDir()
        {
            return drogon::app().getCustomConfig()["storage"]["temp_export_dir"].asString();
        }
        inline std::string getTempApplyDir()
        {
            return drogon::app().getCustomConfig()["storage"]["temp_apply_dir"].asString();
        }
        inline std::string getTempBackupDir()
        {
            return drogon::app().getCustomConfig()["storage"]["temp_backup_dir"].asString();
        }
        ```
   4. docker compose build
   5. docker compose up

- **Window**
   1. sys2 설치
   2. MSYS2 MINGW64 실행
   3. 라이브러리 설치
        ```
        pacman -S --needed mingw-w64-x86_64-libssh2
        pacman -S --needed mingw-w64-x86_64-drogon
        pacman -S --needed mingw-w64-x86_64-libarchive
        pacman -S --needed mingw-w64-x86_64-jsoncpp
        pacman -S --needed mingw-w64-x86_64-yaml-cpp
        ```
   4. 백엔드의 파일 루트에서 명령어 실행
        ```
        mkdir -p build
        cd build
        cmake -G "MinGW Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        ..
        mingw32-make -j8
        ```
   5. `build/`의 실행 파일을 `back/`으로 복사
   
- **Linux**
   1. cmake 설정
        ```
        cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DDROGON_DIR="Drogon CMake 패키지 위치"
        ```
   2. cmake 빌드
        ```
        ninja -C build
        ```
   3. `build/`의 실행 파일을 `back/`으로 복사
   
