cmake_minimum_required(VERSION 3.5)
project(Drogon CXX)

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
add_subdirectory(external/libbcrypt)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    Drogon::Drogon
    yaml-cpp
    LibArchive::LibArchive
    bcrypt
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
aux_source_directory(src/utils/diff UTILS_DIFF_SRC)

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
                           ${CMAKE_CURRENT_SOURCE_DIR}/external/libbcrypt/include
                           ${LibArchive_INCLUDE_DIRS})
target_sources(${PROJECT_NAME}
               PRIVATE
               ${SRC_DIR}
               ${CTL_SRC}
               ${FILTER_SRC}
               ${PLUGIN_SRC}
               ${MODEL_SRC}
               ${SERVICE_SRC}
               ${UTILS_DIFF_SRC}
               src/utils/RobotHttpClient.cc
)
# ##############################################################################
# uncomment the following line for dynamically loading views 
# set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS ON)

# ##############################################################################
#[[
if (TARGET JsonCpp::JsonCpp)
  set(JSONCPP_TARGET JsonCpp::JsonCpp)
elseif (TARGET jsoncpp_lib)
  set(JSONCPP_TARGET jsoncpp_lib)
elseif (TARGET jsoncpp)
  set(JSONCPP_TARGET jsoncpp)
elseif (JSONCPP_INCLUDE_DIRS AND JSONCPP_LIBRARIES)
  add_library(jsoncpp_imported INTERFACE)
  target_include_directories(jsoncpp_imported INTERFACE ${JSONCPP_INCLUDE_DIRS})
  target_link_libraries(jsoncpp_imported INTERFACE ${JSONCPP_LIBRARIES})
  set(JSONCPP_TARGET jsoncpp_imported)
else()
  # 임시(WSL/Ubuntu 흔한 경로)
  add_library(jsoncpp_imported INTERFACE)
  target_include_directories(jsoncpp_imported INTERFACE /usr/include/jsoncpp)
  target_link_libraries(jsoncpp_imported INTERFACE jsoncpp)
  set(JSONCPP_TARGET jsoncpp_imported)
endif()

# --- 1) 먼저 라이브러리를 '정의' ---
add_library(treediff_lib
  src/utils/diff/treediff.cpp
)
target_include_directories(treediff_lib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(treediff_lib PUBLIC cxx_std_17)

# Drogon 헤더를 treediff에서 include 하므로 둘 다 treediff_lib에 '먼저' 연결
target_link_libraries(treediff_lib
  PUBLIC Drogon::Drogon
  PUBLIC ${JSONCPP_TARGET}
)

# --- 2) 그 다음 실행파일을 '정의' ---
add_executable(tree_diff
  src/utils/diff/tree_main.cpp
)

# --- 3) 그리고 실행파일에 라이브러리를 '링크' ---
target_link_libraries(tree_diff PRIVATE treediff_lib)
]]