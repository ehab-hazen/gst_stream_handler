#include "benchmark/benchmark.h"
#include "stream_handler.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <future>
#include <memory>
#include <opencv2/core/base.hpp>
#include <sstream>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

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

tup<u32, double> ReadNFrames(up<StreamHandler> stream_handler,
                             u32 frame_count) {
    u32 read_frame_count = 0;
    size_t width = stream_handler->GetStreamWidth();
    size_t height = stream_handler->GetStreamHeight();

    double elapsed = utils::Timed([&] {
        for (i32 i = 0; i < frame_count; ++i) {
            opt<GstMapProxy> map_proxy = stream_handler->PullSample();
            if (map_proxy.has_value() &&
                map_proxy.value().size() == width * height * 3) {
                ++read_frame_count;
            }
        }
    });

    return {read_frame_count, elapsed};
}

/**
 * @brief Open a new stream and read N frames from it
 */
void OpenStreamAndReadNFrames(i32 id, u32 frame_count) {
    const std::string &stream_uri = stream_uris[id];
    up<StreamHandler> stream_handler =
        StreamHandler::OpenStream(id, stream_uri);
    if (stream_handler == nullptr) {
        std::cerr << "Failed to open stream_" << id << "\n";
    } else {
        std::ostringstream msg;
        msg << "Stream [" << id << "]: Opened connnection, uri = " << stream_uri
            << "\n";
        std::cout << msg.str();

        auto [read_frames, elapsed] =
            ReadNFrames(std::move(stream_handler), frame_count);

        msg.str(""); // reset string
        msg << "Stream [" << id << "]: " << " Read " << read_frames << "/"
            << frame_count << " frames in " << elapsed
            << "s, Frame rate = " << read_frames / elapsed << "\n";
        std::cout << msg.str();
    }
}

/**
 * @brief Open `stream_count` concurrent streams and read `frame_count` frames
 * from each
 */
void ConcurrentStreams(u32 frame_count, u32 stream_count) {
    vec<fut<void>> tasks;

    // Start N concurrent streams
    for (i32 i = 0; i < stream_count; ++i) {
        tasks.push_back(std::async(std::launch::async, OpenStreamAndReadNFrames,
                                   i, frame_count));
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
    // ->Args({FRAME_COUNT, 1})
    // ->Args({FRAME_COUNT, 2})
    // ->Args({FRAME_COUNT, 4})
    // ->Args({FRAME_COUNT, 8})
    // ->Args({FRAME_COUNT, 12})
    // ->Args({FRAME_COUNT, 16})
    // ->Args({FRAME_COUNT, 20})
    ->Args({FRAME_COUNT, 24});

int main(int argc, char **argv) {
    utils::IncreaseFileDescriptorLimit();
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    return 0;
}
