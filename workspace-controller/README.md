# Workspace Controller

로봇 워크스페이스 관리를 위한 서버사이드 프로그램

## 시스템 요구사항

- Ubuntu 20.04 이상
- CMake 3.10 이상
- C++17 이상

## 1. 시스템 패키지 설치

```bash
sudo apt-get update
sudo apt-get install -y \
    git gcc g++ cmake \
    libarchive-dev \
    pkg-config
```

### 정적 빌드 시 추가 패키지

```bash
sudo apt-get install -y \
    libxml2-dev \
    libicu-dev \
    liblzma-dev \
    zlib1g-dev
```

## 2. 빌드

```bash
# 빌드 폴더 생성
mkdir build && cd build

# 동적 빌드
cmake ..
make

# 또는 정적 빌드
cmake -DBUILD_STATIC=ON ..
make
```

## 3. 실행

```bash
./workspace-controller
```

## 4. 콘솔 로그 활성화

`src/main.cc` 수정:

1. 기존 init 라인을 주석 처리
```cpp
plog::init(plog::info, &fileAppender);
```

2. 다음 주석을 해제 후 재빌드
```cpp
// static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
// plog::init(plog::info, &fileAppender).addAppender(&consoleAppender);
```

## 5. 디렉토리 구조

```
workspace-controller/
├── src/
│   ├── main.cc              # 진입점
│   ├── controllers/         # 컨트롤러
│   │   ├── httpController.cc
│   │   ├── robotController.cc
│   │   └── workspaceController.cc
│   ├── services/            # 비즈니스 로직
│   │   └── workspaceService.cc
│   ├── utils/               # 유틸리티
│   │   └── utils.cc
│   └── libs/                # 헤더 라이브러리
└── CMakeLists.txt           # 빌드 설정
```
