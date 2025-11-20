# Drogon Server

로봇 워크스페이스 관리를 위한 Drogon 기반 REST API 서버

## 시스템 요구사항

- Ubuntu 20.04 이상
- CMake 3.5 이상
- C++17 이상

## 1. 시스템 패키지 설치

```bash
sudo apt-get update
sudo apt-get install -y \
    git gcc g++ cmake \
    libjsoncpp-dev uuid-dev zlib1g-dev \
    openssl libssl-dev \
    libssh2-1-dev \
    libyaml-cpp-dev \
    libarchive-dev \
    libicu-dev
```

## 2. Drogon 프레임워크 설치

```bash
git clone https://github.com/drogonframework/drogon
cd drogon
git submodule update --init
mkdir build
cd build
cmake ..
make && sudo make install
cd ../..
```

## 3. 프로젝트 클론 및 빌드

```bash
# 프로젝트 클론 후 Drogon 서버 루트로 이동

# Drogon submodule 초기화
git submodule update --init Drogon/

# 빌드
mkdir build
cd build
cmake ..
make
```

### 외부 라이브러리 (submodule)

- `external/tree-sitter` - 구문 분석 라이브러리
- `external/tree-sitter-python` - Python 파서
- `external/bcrypt` - 비밀번호 해싱

## 4. 설정

### config.json 수정

서버 실행 전 `config.json` 파일에서 필요한 설정을 수정합니다:

#### 서버 포트 설정 (기본: 80)

```json
"listeners": [
    {
        "address": "0.0.0.0",
        "port": 8080,
        "https": false
    }
]
```

#### 저장소 경로 설정

```json
"custom_config": {
    "storage": {
        "base_dir": "/path/to/storage/",
        "temp_apply_dir": "/path/to/temp/apply/",
        "temp_backup_dir": "/path/to/temp/backup/",
        "temp_upload_dir": "/path/to/temp/upload/",
        "temp_export_dir": "/path/to/temp/export/"
    }
}
```

## 5. 실행

```bash
# build 디렉토리에서 실행
./Drogon

# 또는 config.json이 있는 디렉토리에서 실행
cd ..
./build/Drogon
```

## 6. API 엔드포인트

### Device 관리

- `GET /api/v1/repo` - 디바이스 목록 조회
- `POST /api/v1/repo` - 디바이스 생성
- `GET /api/v1/repo/{deviceId}` - 디바이스 정보 조회
- `PUT /api/v1/repo/{deviceId}` - 디바이스 수정
- `DELETE /api/v1/repo/{deviceId}` - 디바이스 삭제

### Workspace 관리

- `POST /api/v1/repo/{deviceId}/workspace` - 워크스페이스 생성
- `GET /api/v1/repo/{deviceId}/workspace/{workspaceId}` - 워크스페이스 정보 조회
- `PUT /api/v1/repo/{deviceId}/workspace/{workspaceId}` - 워크스페이스 수정
- `DELETE /api/v1/repo/{deviceId}/workspace/{workspaceId}` - 워크스페이스 삭제

### 작업 수행

- `POST /api/v1/operation/device/{deviceId}/apply` - 워크스페이스 적용
- `GET /api/v1/operation/device/{deviceId}/backup` - 디바이스 백업
- `POST /api/v1/operation/workspace/import` - 워크스페이스 가져오기
- `POST /api/v1/operation/workspace/export` - 워크스페이스 내보내기

### 연결 검증

- `POST /api/v1/validation/device` - 디바이스 연결 검증

### 모니터링

- `GET /metrics` - Prometheus 메트릭

## 7. 디렉토리 구조

```
Drogon/
├── src/
│   ├── main.cc              # 진입점
│   ├── controllers/         # API 컨트롤러
│   ├── services/            # 비즈니스 로직
│   ├── utils/               # 유틸리티
│   │   ├── connection/      # 연결 검증
│   │   ├── device/          # 디바이스 메타데이터
│   │   ├── diff/            # 파일 diff
│   │   ├── robot/           # 로봇 HTTP 클라이언트
│   │   └── sftp/            # SFTP 클라이언트
│   └── models/              # 데이터 모델
├── external/                # 외부 라이브러리
│   ├── bcrypt/
│   ├── tree-sitter/
│   └── tree-sitter-python/
├── config.json              # 서버 설정
└── CMakeLists.txt           # 빌드 설정
```

# RoboEditor

## 프로젝트 개요

- **주제**: Svelte 기반 로컬 로봇 설정 파일 편집·비교·관리 프로그램
- **설명**: 브라우저(Chromium 계열) 기반 환경에서 로컬 디렉토리를 선택해 파일을 탐색, 비교(diff), 편집하고 컨트롤러별 워크스페이스를 관리하는 Svelte App입니다. Monaco Editor를 이용한 코드 편집, 파일/폴더 트리, 파일 비교 기능 등이 포함되어 있습니다.

## 상세

- **주요 기능**: 제어기로부터 작업 폴더 백업 기능, 작업 폴더 편집기 기능, 파일 및 폴더 별 비교 기능, 제어기로 작업 폴더 적용 기능
- **기반 기능**: 파일 탐색(`FileExplorer`), 파일 편집(Monaco Editor), 파일 저장, 컨트롤러별 워크스페이스 관리

## 사용 방법

- 로컬 개발(빠른 시작):
- **로컬 개발 (빠른 시작)** — 주요 명령을 복사해 붙여 넣어 사용하세요:

```bash
# 의존성 설치
npm install

# 개발 서버 (기본)
npm run dev

# HTTPS로 실행이 필요한 경우
npm run dev -- --https

# 프로덕션 빌드
npm run build

# 빌드 결과 프리뷰
npm run preview

# 단위 테스트 실행
npm run test
```

- 브라우저 권장: Chromium 기반(Chrome, Edge) — HTTPS 또는 `localhost` 환경 권장

## 사용한 라이브러리(주요)

| 라이브러리                      | 용도                         |      버전 |
| ------------------------------- | ---------------------------- | --------: |
| `svelte`                        | UI 프레임워크                | `^5.41.0` |
| `@sveltejs/kit`                 | SvelteKit 앱 런타임 / 라우팅 | `^2.47.1` |
| `vite`                          | 번들러 / 개발 서버           | `^7.1.10` |
| `monaco-editor`                 | 코드 편집기(에디터)          | `^0.53.0` |
| `vite-plugin-monaco-editor-esm` | Monaco 번들링 최적화         |  `^2.0.2` |
| `@testing-library/svelte`       | 컴포넌트 테스트 유틸리티     |  `^5.2.8` |
| `playwright`                    | E2E / 브라우저 테스트        | `^1.56.1` |
| `typescript`                    | 정적 타입 검사 및 빌드       |  `^5.9.3` |
| `prettier`                      | 코드 포맷터                  |  `^3.6.2` |
| `eslint`                        | 린팅 도구                    | `^9.38.0` |

## 폴더 구조 (주요 파일/폴더)

- `src/` : 애플리케이션 소스
  **`src/` 폴더 구조 (주요 항목)**

| 경로            | 설명                                     |
| --------------- | ---------------------------------------- |
| `src/`          | 애플리케이션 소스 루트                   |
| `src/apis/`     | 파일, 폴더, 워크스페이스 등의 API 레이어 |
| `src/features/` | 페이지별 기능 컴포넌트 및 대화상자       |
| `src/lib/`      | 재사용 가능한 UI 컴포넌트 및 레이아웃    |
| `src/routes/`   | SvelteKit 라우트 및 페이지               |
| `src/stores/`   | 애플리케이션 상태 관리(Store)            |
| `src/utils/`    | 유틸리티 및 Helper                       |

## 기타 유의사항

- `monaco-editor` 관련 설정은 `vite-plugin-monaco-editor-esm`을 사용해 번들링 최적화를 적용했습니다.
- 코드 포맷은 `prettier`와 `prettier-plugin-svelte`를 따릅니다. `npm run format`로 자동 포맷하세요.

# 🚀 RoboEditor 빌드 및 배포 가이드

Windows(MinGW) & Linux(Ubuntu 22.04)

이 문서는 **RoboEditor** 프로젝트의 소스 코드를 다운로드하고,  
Windows(MinGW)와 Linux(Ubuntu) 환경에서 빌드하여 실행 가능한 상태로 배포하는 전체 과정을 설명합니다.

---

# 1. 개발 환경 설정 (Prerequisites)

운영체제에 따라 필요한 도구들을 먼저 설치해야 합니다.

---

## 🪟 Windows 설정 (64-bit)

Windows 환경에서는 **Qt(MinGW 컴파일러)** 와 **MSYS2(libssh2 설치)** 를 함께 사용합니다.

### 1) Qt 6.7.3 설치

1. https://www.qt.io/download-qt-installer 에서 **Qt Online Installer** 다운로드
2. 설치 시 **Custom Installation** 선택
3. **Qt 6.7.3 → MinGW 11.2.0 64-bit** 체크
   - 설치 경로 예시: `C:\Qt\6.7.3\mingw_64`

---

### 2) MSYS2 설치 및 libssh2 설치

1. https://www.msys2.org 에서 설치
   - 권장 경로: `C:\msys64`
2. 시작 메뉴에서 **MSYS2 MinGW 64-bit 터미널 실행**
3. 아래 명령으로 libssh2 설치
   ```bash
   pacman -S mingw-w64-x86_64-libssh2
   ```

---

## 🐧 Linux 설정 (Ubuntu 22.04)

### 1) 필수 패키지 설치

```bash
sudo apt update
sudo apt install build-essential cmake git libgl1-mesa-dev libssh2-1-dev libssl-dev
```

### 2) Qt 6.7.3 설치

1. https://www.qt.io/download-qt-installer
2. 다운로드한 `.run` 파일 실행
   ```bash
   chmod +x qt-unified-linux-x64-*-online.run
   ./qt-unified-linux-x64-*-online.run
   ```
3. **Qt 6.7.3 → Desktop gcc 64-bit** 선택
   - 예시 경로: `/home/사용자명/Qt/6.7.3/gcc_64`

---

# 2. 소스 코드 클론 (Clone)

```bash
git clone https://gitlab.com/사용자명/RoboEditor.git
cd RoboEditor
```

---

# 3. 빌드하기 (Build)

---

## 🪟 Windows (MinGW)

프로젝트 루트에서 PowerShell을 열고 아래 순서대로 실행합니다.

```powershell
cd C:\Users\SSAFY\S13P31S205\RoboEditor

# MinGW 툴체인 PATH 추가 (Qt 6.7.3 기본 mingw1120_64)
$env:PATH = "C:\Qt\Tools\mingw1120_64\bin;" + $env:PATH

# CMake configure
cmake -S . -B build/test -G "MinGW Makefiles" `
  -D CMAKE_PREFIX_PATH="C:/Qt/6.7.3/mingw_64" `
  -D CMAKE_BUILD_TYPE=Debug

# Build
cmake --build build/test
```

---

## 🐧 Linux (Ubuntu)

### 1) 이전 빌드 삭제 + Configure

```bash
rm -rf build CMakeCache.txt
mkdir build && cd build

cmake .. \
  -DCMAKE_PREFIX_PATH=/home/ssafy/Qt/6.7.3/gcc_64 \
  -DCMAKE_BUILD_TYPE=Release
```

### 2) 빌드

```bash
make -j$(nproc)
```

---

# 4. 배포 및 실행 (Deploy & Run)

---

## 🪟 Windows 배포

### ✔ Step 1. Qt DLL 자동 복사 (+ 일부 DLL 수동 복사)

```powershell
cd build/test
C:/Qt/6.7.3/mingw_64/bin/windeployqt.exe --release ./RoboEditor.exe
```

windeployqt 실행 후, 아래 DLL들이 **같은 폴더(build/test)** 에 있는지 확인합니다.

- `build/test/external/libbcrypt/` 폴더에서:
  - `libbcrypt.dll` (또는 프로젝트에서 생성된 bcrypt 관련 DLL)
- `C:\Qt\Tools\mingw1120_64\bin` 폴더에서:
  - `libssp-0.dll`

위 두 DLL이 자동으로 복사되지 않으면, **직접 `build/test` 폴더로 복사**해야 실행됩니다.

---

### ✔ Step 2. 실행

```text
RoboEditor.exe
```

---

## 🐧 Linux 실행

빌드 후 바로 실행 가능:

```bash
cd build
./RoboEditor
```

---

# 5. Troubleshooting

---

## ❗ Linux Permission denied

백업 폴더 접근 권한 문제 해결:

```bash
sudo chown -R ssafy:ssafy ~/backup
sudo chmod -R 755 ~/backup
```

---

## ❗ libssh2 not found

### Windows

MSYS2 경로(`C:/msys64/mingw64`) 올바르게 설정됐는지 확인

### Linux

```bash
sudo apt install libssh2-1-dev
```

---

## ❗ Linux에서 ws2_32 오류

윈도우 전용 라이브러리가 리눅스에 링크되는 경우 →  
`if(WIN32)` 블록 안에 있는지 확인
