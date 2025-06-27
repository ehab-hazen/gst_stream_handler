#include "benchmark/benchmark.h"
#include "stream_handler.hpp"
#include "sys/resource.h"
#include "utils.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <opencv2/core/base.hpp>
#include <sstream>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using str = std::string;

template <typename T, int N> using arr = std::array<T, N>;
template <typename T> using vec = std::vector<T>;
template <typename T> using fut = std::future<T>;
template <typename T> using up = std::unique_ptr<T>;
template <typename T> using sp = std::shared_ptr<T>;
template <typename... Ts> using tup = std::tuple<Ts...>;

constexpr u32 FRAME_COUNT = 2000;

const arr<str, 24> stream_uris = {
    "rtsp://127.0.0.1:8554/stream", "rtsp://127.0.0.1:8555/stream",
    "rtsp://127.0.0.1:8556/stream", "rtsp://127.0.0.1:8557/stream",
    "rtsp://127.0.0.1:8558/stream", "rtsp://127.0.0.1:8559/stream",
    "rtsp://127.0.0.1:8560/stream", "rtsp://127.0.0.1:8561/stream",
    "rtsp://127.0.0.1:8562/stream", "rtsp://127.0.0.1:8563/stream",
    "rtsp://127.0.0.1:8564/stream", "rtsp://127.0.0.1:8565/stream",
    "rtsp://127.0.0.1:8566/stream", "rtsp://127.0.0.1:8567/stream",
    "rtsp://127.0.0.1:8568/stream", "rtsp://127.0.0.1:8569/stream",
    "rtsp://127.0.0.1:8570/stream", "rtsp://127.0.0.1:8571/stream",
    "rtsp://127.0.0.1:8572/stream", "rtsp://127.0.0.1:8573/stream",
    "rtsp://127.0.0.1:8574/stream", "rtsp://127.0.0.1:8575/stream",
    "rtsp://127.0.0.1:8576/stream", "rtsp://127.0.0.1:8577/stream"};

up<StreamHandler> OpenStream(i32 id, const std::string &uri,
                             u32 retry_count = 3) {
  for (i32 i = 0; i < retry_count; ++i) {
    up<StreamHandler> stream_handler =
        std::make_unique<StreamHandler>(id, uri, 30);
    if (stream_handler->IsStreamOpen()) {
      return stream_handler;
    }
  }
  return nullptr;
}

tup<u64, u32, double> ReadNFrames(up<StreamHandler> stream_handler,
                                  u32 frame_count) {
  u64 read_byte_count = 0;
  u32 read_frame_count = 0;
  size_t width = stream_handler->GetStreamWidth();
  size_t height = stream_handler->GetStreamHeight();

  double elapsed = utils::Timed([&] {
    for (i32 i = 0; i < frame_count; ++i) {
      vec<u8> bytes = stream_handler->PullSample();
      read_byte_count += bytes.size();
      if (bytes.size() == width * height * 3) {
        ++read_frame_count;
      }
    }
  });

  return {read_byte_count, read_frame_count, elapsed};
}

/**
 * @brief Open `stream_count` concurrent streams and read `frame_count` frames
 * from each
 */
void ConcurrentStreams(u32 frame_count, u32 stream_count) {
  vec<fut<void>> tasks;

  // Open a new stream and read N frames from it
  auto task = [](i32 id, u32 frame_count) {
    const std::string &stream_uri = stream_uris[id];
    up<StreamHandler> stream_handler = OpenStream(id, stream_uri);
    if (stream_handler == nullptr) {
      std::cerr << "Failed to open stream_" << id << "\n";
    } else {
      std::ostringstream msg;
      msg << "Stream [" << id << "]: Opened connnection, uri = " << stream_uri
          << "\n";
      std::cout << msg.str();

      size_t width = stream_handler->GetStreamWidth(),
             height = stream_handler->GetStreamHeight();
      auto [read_bytes, read_frames, elapsed] =
          ReadNFrames(std::move(stream_handler), frame_count);

      msg.str(""); // reset string
      msg << "Stream [" << id << "]: " << " Read " << read_frames << "/"
          << frame_count << " frames (" << read_bytes << " bytes) in "
          << elapsed << "s, Frame rate = " << read_frames / elapsed << "\n";
      std::cout << msg.str();
    }
  };

  // Start N concurrent streams
  for (i32 i = 0; i < stream_count; ++i) {
    tasks.push_back(std::async(std::launch::async, task, i, frame_count));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  // Wait for all tasks to finish
  for (i32 i = 0; i < stream_count; ++i) {
    tasks[i].get();
  }
}

static void BM_ConcurrentStreams(benchmark::State &state) {
  u32 frame_count = state.range(0), stream_count = state.range(1);

  for (auto _ : state) {
    ConcurrentStreams(frame_count, stream_count);
  }
}

BENCHMARK(BM_ConcurrentStreams)
    ->Args({FRAME_COUNT, 1})
    ->Args({FRAME_COUNT, 2})
    ->Args({FRAME_COUNT, 4})
    ->Args({FRAME_COUNT, 8})
    ->Args({FRAME_COUNT, 12})
    ->Args({FRAME_COUNT, 16})
    ->Args({FRAME_COUNT, 20})
    ->Args({FRAME_COUNT, 24});

/**
 * @brief Increase system's file descriptor limit for the current processes (and
 * it's children if any). This is required because the default limit of 1024 on
 * most systems is not enough for 24 streams since a single stream opens
 * multiple descriptors (the exact number will depend on the pipeline).
 */
static void IncreaseFileDescriptorLimit() {
  rlimit rlim;
  if (getrlimit(RLIMIT_NOFILE, &rlim)) {
    perror("getrlimit failed");
    return;
  }

  const rlim_t min_desired = 4096;
  if (rlim.rlim_cur < min_desired) {
    rlim.rlim_cur = std::min(min_desired, rlim.rlim_max);
    if (setrlimit(RLIMIT_NOFILE, &rlim)) {
      perror("setrlimit failed");
    } else {
      std::cout << "Raised file descriptor limit to " << rlim.rlim_cur << "\n";
    }
  }
}

int main(int argc, char **argv) {
  IncreaseFileDescriptorLimit();
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  return 0;
}
