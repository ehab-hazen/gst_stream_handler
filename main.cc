#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <unistd.h>
#include "iostream"
#include <chrono>

#include "stream_handler.hpp"

int main(int argc, char *argv[]) {
  StreamHandler stream_handler("rtsp://10.1.10.250:555/AlHajj01.mp4");
  constexpr int frame_count = 100;
  int read_count = 0;

  size_t width = stream_handler.GetStreamWidth();
  size_t height = stream_handler.GetStreamHeight();

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < frame_count && stream_handler.IsStreamOpen(); ++i) {
    (void)stream_handler.PullSample();
    ++read_count;

    /*std::cout << "Frame[" << i << "] " << "Read " << size << " bytes\n";*/
    /*std::string frame_name = "/tmp/Frame_" + std::to_string(i) + ".png";*/
    /*cv::Mat frame(height, width, CV_8UC3, data);*/
    /*cv::imwrite(frame_name, frame);*/
  }
  auto end = std::chrono::steady_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();
  std::cout << "Read " << read_count << " frames in " << elapsed << "s\n";

  return 0;
}
