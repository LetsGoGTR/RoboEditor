
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
