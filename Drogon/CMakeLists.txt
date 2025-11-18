cmake_minimum_required(VERSION 3.5)
project(Drogon C CXX)

include(CheckIncludeFileCXX)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
if (NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
    # Do nothing
elseif (HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
    set(CMAKE_CXX_STANDARD 20)
elseif (HAS_ANY AND HAS_STRING_VIEW)
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 14)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} ./src/main.cc)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# add_subdirectory(drogon) 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines
find_package(Drogon CONFIG REQUIRED)
find_package(LibArchive REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(jsoncpp REQUIRED)
add_subdirectory(external/bcrypt)

# ##############################################################################
add_library(tree_sitter STATIC
    external/tree-sitter/lib/src/lib.c
)

target_include_directories(tree_sitter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/tree-sitter/lib/include
)

add_library(tree_sitter_python STATIC
    external/tree-sitter-python/src/parser.c
    external/tree-sitter-python/src/scanner.c
)

target_include_directories(tree_sitter_python
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/tree-sitter-python/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external/tree-sitter/lib/include
)

target_link_libraries(tree_sitter_python
    PRIVATE
        tree_sitter
)
# ##############################################################################

# Find libssh2 using pkg-config
find_path(LIBSSH2_INCLUDE_DIR
    NAMES libssh2.h
)
find_library(LIBSSH2_LIBRARY
    NAMES ssh2 libssh2
)
if (NOT LIBSSH2_INCLUDE_DIR OR NOT LIBSSH2_LIBRARY)
    message(FATAL_ERROR "libssh2 not found. Please install libssh2-1-dev")
endif()

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    Drogon::Drogon
    yaml-cpp
    LibArchive::LibArchive
    bcrypt
    ${LIBSSH2_LIBRARY}
    tree_sitter_python
)

# ##############################################################################

if (CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "c++17 or higher is required")
elseif (CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "use c++17")
else ()
    message(STATUS "use c++20")
endif ()

aux_source_directory(src/controllers CTL_SRC)
aux_source_directory(src/filters FILTER_SRC)
aux_source_directory(src/plugins PLUGIN_SRC)
aux_source_directory(src/models MODEL_SRC)
aux_source_directory(src/services SERVICE_SRC)
aux_source_directory(src/utils UTILS_SRC)
aux_source_directory(src/utils/diff UTILS_DIFF_SRC)
aux_source_directory(src/utils/logging UTILS_LOGGING_SRC)
aux_source_directory(src/utils/robot UTILS_ROBOT_SRC)
aux_source_directory(src/utils/sftp UTILS_SFTP_SRC)

drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
                    ${CMAKE_CURRENT_BINARY_DIR})
# use the following line to create views with namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE)
# use the following line to create views with namespace CHANGE_ME prefixed
# and path namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE CHANGE_ME)

target_include_directories(${PROJECT_NAME}
                           PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/src
                           ${CMAKE_CURRENT_SOURCE_DIR}/models
                           ${CMAKE_CURRENT_SOURCE_DIR}/external/bcrypt
                           ${CMAKE_CURRENT_SOURCE_DIR}/external/tree-sitter/lib/include
                           ${LibArchive_INCLUDE_DIRS}
                           ${LIBSSH2_INCLUDE_DIR})
target_sources(${PROJECT_NAME}
               PRIVATE
               ${SRC_DIR}
               ${CTL_SRC}
               ${FILTER_SRC}
               ${PLUGIN_SRC}
               ${MODEL_SRC}
               ${SERVICE_SRC}
               ${UTILS_SRC}
               ${UTILS_DIFF_SRC}
               ${UTILS_LOGGING_SRC}
               ${UTILS_ROBOT_SRC}
               ${UTILS_SFTP_SRC}
)


#[[
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(test_diff_v4
    test/test_diff_v4.cpp
    src/utils/diff/DiffPython.cc
    src/utils/diff/DiffYaml.cc
    src/utils/diff/DiffTree.cc
)

target_link_libraries(test_diff_v4
    PRIVATE
    GTest::gtest                    # ✅ gtest 심볼
    GTest::gtest_main               # ✅ main 제공
    Threads::Threads                # ✅ (리눅스) gtest가 pthread 의존
    Drogon::Drogon
    yaml-cpp
    LibArchive::LibArchive
    bcrypt
)

if (TARGET JsonCpp::JsonCpp)
    target_link_libraries(test_diff_v4 PRIVATE JsonCpp::JsonCpp)
elseif (DEFINED jsoncpp_LIBRARIES)
    target_include_directories(test_diff_v4 PRIVATE ${jsoncpp_INCLUDE_DIRS})
    target_link_libraries(test_diff_v4 PRIVATE ${jsoncpp_LIBRARIES})
endif()

target_include_directories(test_diff_v4
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bcrypt
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/diff
    ${LibArchive_INCLUDE_DIRS}
)
]]