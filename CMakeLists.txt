cmake_minimum_required(VERSION 3.10)

project(gstreamer_projects)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(PkgConfig REQUIRED)
find_package(cxxopts REQUIRED)

# GStreamer packages
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GST_RTSP REQUIRED gstreamer-rtsp-server-1.0)

# OpenCV
find_package(OpenCV REQUIRED)

######################
# Stream Handler App #
######################

add_executable(stream_handler main.cc)

find_library(NVML_LIB NAMES nvidia-ml REQUIRED)
find_library(GLOG_LIB NAMES glog REQUIRED)

target_include_directories(stream_handler PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

set_target_properties(stream_handler PROPERTIES
    BUILD_RPATH "/usr/local/lib"
    INSTALL_RPATH "/usr/local/lib"
)

target_link_libraries(stream_handler
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${OpenCV_LIBS}
    ${NVML_LIB}
    ${GLOG_LIB}
    cxxopts::cxxopts
)

###################
# RTSP Server App #
###################

add_executable(rtsp_server rtsp_server.cc)

target_include_directories(rtsp_server PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_RTSP_INCLUDE_DIRS}
)

target_link_libraries(rtsp_server
    ${GST_LIBRARIES}
    ${GST_RTSP_LIBRARIES}
)

target_compile_options(rtsp_server PRIVATE
    ${GST_CFLAGS_OTHER}
    ${GST_RTSP_CFLAGS_OTHER}
)
