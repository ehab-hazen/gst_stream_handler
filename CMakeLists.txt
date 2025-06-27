cmake_minimum_required(VERSION 3.10)

project(gstreamer_projects)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(PkgConfig REQUIRED)

# GStreamer packages
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GST_RTSP REQUIRED gstreamer-rtsp-server-1.0)

# Google Benchmark
find_package(benchmark REQUIRED)

# OpenCV
find_package(OpenCV REQUIRED)

######################
# Stream Handler App #
######################

add_executable(stream_handler main.cc)

target_include_directories(stream_handler PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(stream_handler
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${OpenCV_LIBS}
)

###########################
# Stream Benchmark Target #
###########################

add_executable(benchmarks benchmark.cc)

target_include_directories(benchmarks PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(benchmarks
    benchmark::benchmark
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${OpenCV_LIBS}
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
