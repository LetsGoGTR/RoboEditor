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

# jsoncpp
add_subdirectory(external/jsoncpp)

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
        src/app/OpenFilePage.cpp
        src/app/CenterStack.cpp
        src/app/ComparePage.cpp
        src/app/DropTextEdit.cpp
        src/app/LogManager.cpp
        src/app/mainwindow.cpp
        src/app/NavDock.cpp
        src/app/TopMenu.cpp
        src/app/ApplyPage.cpp
        src/app/ConfirmSelection.cpp
        src/app/ModifyPage.cpp
        src/app/PasswordManager.cpp
        src/app/SelectController.cpp
        src/app/SelectStorage.cpp

        src/app/BackupPage.h
        src/app/OpenFilePage.h
        src/app/CenterStack.h
        src/app/ComparePage.h
        src/app/DropTextEdit.h
        src/app/LogManager.h
        src/app/mainwindow.h
        src/app/NavDock.h
        src/app/TopMenu.h
        src/app/ApplyPage.h
        src/app/ConfirmSelection.h
        src/app/ModifyPage.h
        src/app/PasswordManager.h
        src/app/SelectController.h
        src/app/SelectStorage.h

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
        src/ui/wrongpassword.ui

        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

    qt_add_executable(RoboEditor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/app/ControllerManager.h src/app/ControllerManager.cpp
        src/app/CodeEditor.cpp
        src/app/CodeEditor.h
        src/app/ShortcutManager.h src/app/ShortcutManager.cpp
        src/app/ControllerSetting.h src/app/ControllerSetting.cpp
        src/ui/ControllerSettings.ui

        # src/app/OpenFilePage.h src/app/OpenFilePage.cpp
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

target_link_libraries(RoboEditor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(RoboEditor PRIVATE Qt6::Widgets)
target_link_libraries(RoboEditor PRIVATE Qt6::Widgets)
target_link_libraries(RoboEditor PRIVATE Qt6::Network)

# jsoncpp
target_link_libraries(RoboEditor PRIVATE jsoncpp_static)

# libbcrypt
target_link_libraries(RoboEditor PRIVATE bcrypt)

# libarchive (for WorkspaceService)
target_link_libraries(RoboEditor PRIVATE archive)

# yaml-cpp (for DiffYaml)
target_link_libraries(RoboEditor PRIVATE yaml-cpp)


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
