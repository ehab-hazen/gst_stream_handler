cmake_minimum_required(VERSION 3.10)
project(stream_handler)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(PkgConfig REQUIRED)

# GStreamer packages
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)

# Google Benchmark
find_package(benchmark REQUIRED)
# OpenCV
find_package(OpenCV REQUIRED)

# Main executable
add_executable(${PROJECT_NAME} main.cc)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${OpenCV_LIBS}
)

# Benchmark executable (optional, if you have benchmark.cc)
add_executable(benchmark_tests benchmark.cc)
target_include_directories(benchmark_tests PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(benchmark_tests
    benchmark::benchmark
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${OpenCV_LIBS}
)
