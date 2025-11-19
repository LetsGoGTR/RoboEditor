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
git submodule update --init Drogon/external/bcrypt Drogon/external/tree-sitter Drogon/external/tree-sitter-python

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

