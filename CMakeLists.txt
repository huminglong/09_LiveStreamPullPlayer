cmake_minimum_required(VERSION 3.15)

project(09_LiveStreamPullPlayer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(Qt5 COMPONENTS Widgets Multimedia REQUIRED)

set(_default_ffmpeg_root "F:/Software/cpp_packages/ffmpeg-gpl")
set(FFMPEG_ROOT "${FFMPEG_ROOT}" CACHE PATH "Path to FFmpeg installation root")
if(NOT FFMPEG_ROOT AND EXISTS "${_default_ffmpeg_root}")
  set(FFMPEG_ROOT "${_default_ffmpeg_root}" CACHE PATH "Path to FFmpeg installation root" FORCE)
endif()

if(FFMPEG_ROOT)
  list(APPEND CMAKE_PREFIX_PATH "${FFMPEG_ROOT}")
endif()

find_path(FFMPEG_INCLUDE_DIR
  NAMES libavformat/avformat.h
  HINTS ${FFMPEG_ROOT}
  PATH_SUFFIXES include)

set(_ffmpeg_library_search_paths)
if(FFMPEG_ROOT)
  list(APPEND _ffmpeg_library_search_paths
    ${FFMPEG_ROOT}
    ${FFMPEG_ROOT}/lib
    ${FFMPEG_ROOT}/bin)
endif()

find_library(FFMPEG_avformat
  NAMES avformat avformat-61 avformat-60 avformat-59
  HINTS ${_ffmpeg_library_search_paths}
  PATH_SUFFIXES lib bin)

find_library(FFMPEG_avcodec
  NAMES avcodec avcodec-61 avcodec-60 avcodec-59
  HINTS ${_ffmpeg_library_search_paths}
  PATH_SUFFIXES lib bin)

find_library(FFMPEG_avutil
  NAMES avutil avutil-59 avutil-58
  HINTS ${_ffmpeg_library_search_paths}
  PATH_SUFFIXES lib bin)

find_library(FFMPEG_swscale
  NAMES swscale swscale-8 swscale-7
  HINTS ${_ffmpeg_library_search_paths}
  PATH_SUFFIXES lib bin)

find_library(FFMPEG_swresample
  NAMES swresample swresample-5 swresample-4
  HINTS ${_ffmpeg_library_search_paths}
  PATH_SUFFIXES lib bin)

if(NOT (FFMPEG_INCLUDE_DIR AND FFMPEG_avformat AND FFMPEG_avcodec AND FFMPEG_avutil))
  message(FATAL_ERROR "FFmpeg not found. Please set FFMPEG_ROOT to the root directory of your FFmpeg installation.")
endif()

if(NOT FFMPEG_swscale)
  message(FATAL_ERROR "FFmpeg swscale library not found. Please ensure libswscale is available.")
endif()

if(NOT FFMPEG_swresample)
  message(FATAL_ERROR "FFmpeg swresample library not found. Please ensure libswresample is available.")
endif()

set(FFMPEG_LIBRARIES ${FFMPEG_avformat} ${FFMPEG_avcodec} ${FFMPEG_avutil})
list(APPEND FFMPEG_LIBRARIES ${FFMPEG_swscale} ${FFMPEG_swresample})

message(STATUS "FFmpeg include dir: ${FFMPEG_INCLUDE_DIR}")
message(STATUS "FFmpeg libraries: ${FFMPEG_LIBRARIES}")

set(PROJECT_SOURCES
  main.cpp
  mainwindow.cpp
  mainwindow.h
  livestreamplayer.cpp
  livestreamplayer.h
  packetqueue.h
  packetqueue.cpp
  playerstats.h
  videowidget.cpp
  videowidget.h)

if(ANDROID)
  add_library(09_LiveStreamPullPlayer SHARED ${PROJECT_SOURCES})
else()
  add_executable(09_LiveStreamPullPlayer ${PROJECT_SOURCES})
endif()

target_include_directories(09_LiveStreamPullPlayer PRIVATE ${FFMPEG_INCLUDE_DIR})

target_link_libraries(09_LiveStreamPullPlayer PRIVATE
  Qt5::Widgets
  Qt5::Multimedia
  ${FFMPEG_LIBRARIES})

# Treat sources as UTF-8 in MSVC to avoid codepage warnings (e.g., from FFmpeg headers)
if(MSVC)
  target_compile_options(09_LiveStreamPullPlayer PRIVATE /utf-8)
endif()
