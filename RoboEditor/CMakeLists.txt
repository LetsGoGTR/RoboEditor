cmake_minimum_required(VERSION 3.16)

project(RoboEditor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 모든 라이브러리(zlib 포함)를 PIC 모드로 빌드하여 링크 에러 해결
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# pkg-config 제거 - Windows에서는 직접 경로 지정이 더 안정적

include(FetchContent)
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG    0.8.0
)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(yaml-cpp)

FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG        v1.3.1
)
FetchContent_MakeAvailable(zlib)

# ZLIB::ZLIB alias 보장
if (NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

# --- libarchive ---
FetchContent_Declare(
    libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive.git
    GIT_TAG        v3.7.4
)

set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
set(ENABLE_ZLIB ON CACHE BOOL "" FORCE)
set(ENABLE_BZip2 OFF CACHE BOOL "" FORCE)
set(ENABLE_LZMA OFF CACHE BOOL "" FORCE)
set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
set(ENABLE_ICONV OFF CACHE BOOL "" FORCE)
set(ENABLE_CNG OFF CACHE BOOL "" FORCE)
set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)

# 🔥 핵심: libarchive가 zlib을 찾을 수 있도록 경로를 명시적으로 설정
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
set(ZLIB_LIBRARY zlibstatic CACHE FILEPATH "" FORCE)
set(ZLIB_LIBRARIES zlibstatic CACHE STRING "" FORCE)

FetchContent_MakeAvailable(libarchive)

# jsoncpp
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Disable jsoncpp tests")
add_subdirectory(external/jsoncpp)

set_target_properties(jsoncpp_static PROPERTIES
    CXX_STANDARD ${CMAKE_CXX_STANDARD}
)

# libbcrypt
add_subdirectory(external/libbcrypt)
include_directories(external/libbcrypt/include)

if(WIN32)
    # Windows (MSYS2) 설정
    set(MSYS2_ROOT "C:/msys64/mingw64")
    set(OPENSSL_ROOT_DIR "${MSYS2_ROOT}" CACHE PATH "" FORCE)
    set(OPENSSL_INCLUDE_DIR "${MSYS2_ROOT}/include" CACHE PATH "" FORCE)
    set(OPENSSL_CRYPTO_LIBRARY "${MSYS2_ROOT}/lib/libcrypto.a" CACHE FILEPATH "" FORCE)
    set(OPENSSL_SSL_LIBRARY "${MSYS2_ROOT}/lib/libssl.a" CACHE FILEPATH "" FORCE)

    set(LIBSSH2_INCLUDE_DIR "${MSYS2_ROOT}/include" CACHE PATH "")
    set(LIBSSH2_LIBRARY "${MSYS2_ROOT}/lib/libssh2.a" CACHE FILEPATH "")
else()
    # Linux/macOS 설정 (패키지 매니저로 설치된 경로 탐색)
    find_package(OpenSSL REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH2 REQUIRED libssh2)
    
    set(LIBSSH2_INCLUDE_DIR ${LIBSSH2_INCLUDE_DIRS})
    set(LIBSSH2_LIBRARY ${LIBSSH2_LIBRARIES})
endif()

# libssh2 라이브러리 존재 확인
if(WIN32 AND NOT EXISTS "${LIBSSH2_LIBRARY}")
    message(FATAL_ERROR
        "libssh2 not found at ${LIBSSH2_LIBRARY}\n"
        "Please install it with: pacman -S mingw-w64-x86_64-libssh2"
    )
endif()

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

set(TS_FILES RoboEditor_ko_KR.ts)

set(PROJECT_SOURCES
        src/main.cpp
        src/app/BackupPage.cpp
        src/app/CenterStack.cpp
        src/app/ComparePage.cpp
        src/app/FlowLayout.cpp
        src/app/LogManager.cpp
        src/app/mainwindow.cpp
        src/app/NavDock.cpp
        src/app/TopMenu.cpp
        src/app/ApplyPage.cpp
        src/app/ConfirmSelection.cpp
        src/app/ModifyPage.cpp
        src/app/SelectController.cpp
        src/app/SelectStorage.cpp

        src/app/BackupPage.h
        src/app/CenterStack.h
        src/app/ComparePage.h
        src/app/FlowLayout.h
        src/app/LogManager.h
        src/app/mainwindow.h
        src/app/NavDock.h
        src/app/TopMenu.h
        src/app/ApplyPage.h
        src/app/ConfirmSelection.h
        src/app/ModifyPage.h
        src/app/SelectController.h
        src/app/SelectStorage.h

        src/core/ApiClient.cpp
        src/core/ApiClient.h
        src/core/ControllerManager.cpp
        src/core/ControllerManager.h
        src/core/ControllerSetting.cpp
        src/core/ControllerSetting.h
        src/core/DiffHighlighter.cpp
        src/core/DiffHighlighter.h
        src/core/DropTextEdit.cpp
        src/core/DropTextEdit.h
        src/core/PasswordManager.cpp
        src/core/PasswordManager.h
        src/core/SftpClient.cpp
        src/core/SftpClient.h
        src/core/ShortcutManager.cpp
        src/core/ShortcutManager.h

        # Core Controllers
        src/core/controllers/ControllerHelper.h
        src/core/controllers/DeviceController.h
        src/core/controllers/DeviceController.cc
        src/core/controllers/FileController.h
        src/core/controllers/FileController.cc
        src/core/controllers/FolderController.h
        src/core/controllers/FolderController.cc
        src/core/controllers/WorkspaceController.h
        src/core/controllers/WorkspaceController.cc
        src/core/controllers/DiffController.h
        src/core/controllers/DiffController.cc

        # Core Services
        src/core/services/ServiceResult.h
        src/core/services/DeviceService.h
        src/core/services/DeviceService.cc
        src/core/services/FileService.h
        src/core/services/FileService.cc
        src/core/services/FolderService.h
        src/core/services/FolderService.cc
        src/core/services/WorkspaceService.h
        src/core/services/WorkspaceService.cc
        src/core/services/DiffService.h
        src/core/services/DiffService.cc
        src/core/services/AuthService.h
        src/core/services/AuthService.cc

        # Core Utils
        src/core/utils/ConfigUtils.h
        src/core/utils/TimeUtils.h
        src/core/utils/PathValidator.h
        src/core/utils/JsonFileUtils.h
        src/core/utils/RobotHttpClient.h
        src/core/utils/RobotHttpClient.cc
        src/core/utils/logging/Logger.h
        src/core/utils/logging/Logger.cc
        src/core/utils/diff/DiffText.h
        src/core/utils/diff/DiffText.cc
        src/core/utils/diff/DiffPython.h
        src/core/utils/diff/DiffPython.cc
        src/core/utils/diff/DiffYaml.h
        src/core/utils/diff/DiffYaml.cc
        src/core/utils/diff/DiffTree.h
        src/core/utils/diff/DiffTree.cc


        src/ui/mainwindow.ui
        src/ui/ApplyPage.ui
        src/ui/ConfirmSelection.ui
        src/ui/ModifyPage.ui
        src/ui/PasswordInput.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(RoboEditor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/app/CodeEditor.cpp
        src/app/CodeEditor.h
        src/ui/BackupPage.ui
        src/ui/ControllerSettings.ui
        src/core/FileCompressor.h
        src/core/FileCompressor.cpp
        RoboEditor.qrc
        src/core/WorkspaceContextMenuController.h
        src/core/WorkspaceContextMenuController.cpp
        src/app/ProgressDialog.cpp
        src/app/ProgressDialog.h
        src/app/DiffTablePanel.cpp
        src/app/DiffTablePanel.h
        src/core/FileTypeHelper.cpp
        src/core/FileTypeHelper.h
        src/core/AppConfig.h
        src/core/LogTextEdit.cpp
        src/core/LogTextEdit.h
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(RoboEditor SHARED ${PROJECT_SOURCES})
    else()
        add_executable(RoboEditor ${PROJECT_SOURCES})
    endif()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_include_directories(RoboEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${LIBSSH2_INCLUDE_DIR}
)

target_link_libraries(RoboEditor PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt6::Network
    jsoncpp_static
    bcrypt
    archive_static
    yaml-cpp
    ${LIBSSH2_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    zlibstatic
)

# 2. Windows일 때만 추가 라이브러리 링크
if(WIN32)
    target_link_libraries(RoboEditor PRIVATE
        ws2_32
        crypt32
        ssp
    )
endif()

# 빌드 후 src/styles 폴더를 실행 파일 옆으로 자동 복사
add_custom_command(TARGET RoboEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    $<TARGET_FILE_DIR:RoboEditor>/styles
    COMMENT "Copying styles directory to build directory..."
)
# make install 시에도 styles 폴더가 같이 설치되도록 설정
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT resources
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.RoboEditor)
endif()

set_target_properties(RoboEditor PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS RoboEditor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RoboEditor)
endif()
