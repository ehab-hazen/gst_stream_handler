#include "benchmark/benchmark.h"
#include "stream_handler.hpp"
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

template <typename T> using vec = std::vector<T>;
template <typename T> using fut = std::future<T>;
template <typename T> using sp = std::shared_ptr<T>;

constexpr int FRAME_COUNT = 200;

int ReadNFrames(StreamHandler &stream_handler, int frame_count) {
  ino64_t read_byte_count = 0;
  for (int i = 0; i < frame_count && stream_handler.IsStreamOpen(); ++i) {
    vec<uint8_t> bytes = stream_handler.PullSample();
    read_byte_count += bytes.size();
  }
  return read_byte_count;
}

void MultipleStreamsMultipleReaders(int frame_count, int stream_count) {
  vec<fut<void>> tasks;
  int base = frame_count / stream_count;
  int remainder = frame_count % stream_count;

  for (int i = 0; i < stream_count; ++i) {
    int task_frame_count = base + (i < remainder ? 1 : 0);
    tasks.push_back(
        std::async(std::launch::async, [i, frame_count = task_frame_count]() {
          StreamHandler stream_handler("rtsp://10.1.10.250:555/AlHajj01.mp4");
          std::cout << i << " Read " << ReadNFrames(stream_handler, frame_count)
                    << "/"
                    << frame_count * stream_handler.GetStreamWidth() *
                           stream_handler.GetStreamHeight() * 3
                    << " bytes\n";
        }));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  for (int i = 0; i < stream_count; ++i) {
    tasks[i].get();
  }
}

static void BM_MultipleStreamsMultipleReaders(benchmark::State &state) {
  int frame_count = state.range(0), stream_count = state.range(1);

  for (auto _ : state) {
    MultipleStreamsMultipleReaders(frame_count, stream_count);
  }
}

BENCHMARK(BM_MultipleStreamsMultipleReaders)
    ->Args({FRAME_COUNT, 1})
    ->Args({FRAME_COUNT, 2})
    ->Args({FRAME_COUNT, 4})
    ->Args({FRAME_COUNT, 8})
    ->Args({FRAME_COUNT, 12})
    ->Args({FRAME_COUNT, 16})
    ->Args({FRAME_COUNT, 20})
    ->Args({FRAME_COUNT, 24});

BENCHMARK_MAIN();
