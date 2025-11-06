#libssh 없는 순정
cmake_minimum_required(VERSION 3.16)

project(RoboEditor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

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

FetchContent_Declare(
    libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive.git
    GIT_TAG        v3.7.4 # 안정적인 최신 태그
)
# libarchive가 zlib 등 다른 라이브러리를 찾지 못하도록 옵션을 켭니다.
set(ENABLE_TEST OFF CACHE BOOL "")
set(ENABLE_ZLIB ON CACHE BOOL "")
set(ENABLE_BZIP2 OFF CACHE BOOL "")
set(ENABLE_LZMA OFF CACHE BOOL "")
set(ENABLE_ZSTD OFF CACHE BOOL "")
set(ENABLE_ICONV OFF CACHE BOOL "")
set(ENABLE_CNG OFF CACHE BOOL "")

set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR} CACHE PATH "")
set(ZLIB_LIBRARY zlibstatic CACHE STRING "")

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
        src/core/utils/diff/treediff.h
        src/core/utils/diff/treediff.cpp
        src/core/utils/diff/line-diff-v6+.cpp

        src/ui/mainwindow.ui
        src/ui/ApplyPage.ui
        src/ui/confirmselection.ui
        src/ui/ModifyPage.ui
        src/ui/PasswordInput.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    qt_add_executable(RoboEditor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/app/CodeEditor.cpp
        src/app/CodeEditor.h
        src/ui/BackupPage.ui

        src/ui/ControllerSettings.ui
        src/core/FileCompressor.h src/core/FileCompressor.cpp

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET RoboEditor APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(RoboEditor SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(RoboEditor
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_include_directories(RoboEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)
target_link_libraries(RoboEditor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(RoboEditor PRIVATE Qt6::Widgets)
target_link_libraries(RoboEditor PRIVATE Qt6::Network)

# jsoncpp
target_link_libraries(RoboEditor PRIVATE jsoncpp_static)

# libbcrypt
target_link_libraries(RoboEditor PRIVATE bcrypt)

# libarchive (for WorkspaceService)
target_link_libraries(RoboEditor PRIVATE archive_static)

# yaml-cpp (for DiffYaml)
target_link_libraries(RoboEditor PRIVATE yaml-cpp)

target_link_libraries(RoboEditor PRIVATE zlibstatic)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
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
