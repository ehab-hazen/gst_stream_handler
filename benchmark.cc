#include "benchmark/benchmark.h"
#include "stream_handler.hpp"
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <opencv2/core/base.hpp>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

template <typename T> using vec = std::vector<T>;
template <typename T> using fut = std::future<T>;
template <typename T> using up = std::unique_ptr<T>;
template <typename T> using sp = std::shared_ptr<T>;
using u64 = uint64_t;

constexpr int FRAME_COUNT = 2000;

// TODO: if a task fails, the others should take on its work

up<StreamHandler> OpenStream(const std::string &uri, int retry_count = 3) {
  for (int i = 0; i < retry_count; ++i) {
    up<StreamHandler> stream_handler = std::make_unique<StreamHandler>(uri);
    if (stream_handler->IsStreamOpen()) {
      return stream_handler;
    }
  }
  return nullptr;
}

u64 ReadNFrames(up<StreamHandler> stream_handler, int frame_count) {
  ino64_t read_byte_count = 0;
  for (int i = 0; i < frame_count && stream_handler->IsStreamOpen(); ++i) {
    vec<uint8_t> bytes = stream_handler->PullSample();
    read_byte_count += bytes.size();
  }
  return read_byte_count;
}

void MultipleStreamsMultipleReaders(int frame_count, int stream_count) {
  vec<fut<void>> tasks;
  int base = frame_count / stream_count, remainder = frame_count % stream_count;

  for (int i = 0; i < stream_count; ++i) {
    int task_frame_count = base + (i < remainder ? 1 : 0);
    auto task = [i, frame_count = task_frame_count]() {
      up<StreamHandler> stream_handler =
          OpenStream("rtsp://10.1.10.250:555/AlHajj01.mp4");
      if (stream_handler == nullptr) {
        std::cerr << i << "Failed to open stream\n";
        return;
      }

      int width = stream_handler->GetStreamWidth(),
          height = stream_handler->GetStreamHeight();
      u64 read_frame_count =
          ReadNFrames(std::move(stream_handler), frame_count);

      std::cout << i << " Read " << read_frame_count << "/"
                << u64(frame_count) * width * height * 3 << " bytes\n";
    };

    tasks.push_back(std::async(std::launch::async, task));
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
