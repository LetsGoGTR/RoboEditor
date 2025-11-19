cmake_minimum_required(VERSION 3.10)
project(workspace-controller)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 정적 빌드 옵션
option(BUILD_STATIC "Build static executable" OFF)

if(BUILD_STATIC)
  message(STATUS "Static Build Mode")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# Find libarchive
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)

add_executable(
  workspace-controller
  src/main.cc
  src/utils/utils.cc
  src/services/workspaceService.cc
  src/controllers/httpController.cc
  src/controllers/robotController.cc
  src/controllers/workspaceController.cc
)

target_include_directories(workspace-controller
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/libs
    ${LIBARCHIVE_INCLUDE_DIRS}
)

if(BUILD_STATIC)
  execute_process(
    COMMAND pkg-config --libs --static libarchive
    OUTPUT_VARIABLE LIBARCHIVE_STATIC_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  execute_process(
    COMMAND pkg-config --libs --static libxml2
    OUTPUT_VARIABLE LIBXML2_STATIC_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  target_link_libraries(workspace-controller
    PRIVATE
      ${LIBARCHIVE_STATIC_LIBS}
      ${LIBXML2_STATIC_LIBS}
      icuuc
      icudata
      icui18n
      z        
      lzma     
      pthread
  )
  
  target_link_options(workspace-controller
    PRIVATE
      -static-libgcc
      -static-libstdc++
  )
else()
  target_link_libraries(workspace-controller
    PRIVATE
      ${LIBARCHIVE_LIBRARIES}
  )
endif()
