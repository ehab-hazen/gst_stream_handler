#include "iostream"
#include "stream_handler.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <chrono>
#include <future>
#include <thread>

u32 ReadNFrames(up<StreamHandler> stream_handler, u32 frame_count) {
    u32 read_count = 0;
    size_t width = stream_handler->GetStreamWidth(),
           height = stream_handler->GetStreamHeight();

    for (i32 i = 0; i < frame_count && stream_handler->IsStreamOpen(); ++i) {
        opt<GstMapProxy> map_proxy = stream_handler->PullSample();
        if (map_proxy && map_proxy->size() == width * height * 3) {
            ++read_count;
        } else {
            std::cout << "Stream [" << stream_handler->GetId() << "]: Read "
                      << map_proxy->size() << "/" << width * height * 3
                      << " bytes\n";
        }
    }
    return read_count;
}

void OpenAndReadStream(i32 id, u32 frame_count) {
    std::ostringstream msg;

    std::string stream_uri =
        "rtsp://127.0.0.1:" + std::to_string(8554 + id) + "/stream";
    up<StreamHandler> stream_handler =
        StreamHandler::OpenStream(id, stream_uri);
    if (stream_handler == nullptr) {
        msg << "Stream [" << id << "]: Failed to open stream\n";
        std::cout << msg.str();
    } else {
        msg << "Stream [" << id << "]: Opened stream, uri = " << stream_uri
            << "\n";
        std::cout << msg.str();
        msg.str("");

        auto [elapsed, read_count] = utils::TimedResult(
            ReadNFrames, std::move(stream_handler), frame_count);

        msg << "Stream [" << id << "]: Read " << read_count << "/"
            << frame_count << " in " << elapsed
            << "s, FPS = " << read_count / elapsed << "\n";
        std::cout << msg.str();
    }
}

int main(int argc, char *argv[]) {
    utils::IncreaseFileDescriptorLimit();

    u32 frame_count = 2000, stream_count = 24;
    vec<fut<void>> tasks;

    for (i32 id = 0; id < stream_count; ++id) {
        fut<void> f =
            std::async(std::launch::async, OpenAndReadStream, id, frame_count);
        tasks.push_back(std::move(f));

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    for (i32 i = 0; i < stream_count; ++i) {
        tasks[i].get();
    }

    return 0;
}
