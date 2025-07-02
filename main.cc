#include "iostream"
#include "stream_handler.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <chrono>
#include <cstdlib>
#include <cxxopts.hpp>
#include <future>
#include <thread>

/**
 * @returns The number of frames read
 */
u32 ReadNFrames(up<StreamHandler> stream_handler, u32 frame_count) {
    u32 read_count = 0;
    size_t width = stream_handler->GetStreamWidth(),
           height = stream_handler->GetStreamHeight();

    for (u32 i = 0; i < frame_count && stream_handler->IsStreamOpen(); ++i) {
        vec<u8> bytes = stream_handler->PullSample();
        if (bytes.size() == width * height * 3) {
            ++read_count;
        } else {
            std::cout << "Stream [" << stream_handler->GetId() << "]: Read "
                      << bytes.size() << "/" << width * height * 3
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

cxxopts::ParseResult ParseArgs(int argc, char **argv) {
    cxxopts::Options options("Gst stream handler");
    options.add_options()(
        "f,frame_count",
        "The number of frames each stream will try to read. Make sure that the "
        "remote rtsp stream has enough frames.",
        cxxopts::value<u32>())(
        "s,stream_count",
        "The number of concurrent streams to run, make sure there are atleast "
        "as many active rtsp servers running locally on different ports. The "
        "port numbers must start from 8554 and increment by 1 for each "
        "additional server.",
        cxxopts::value<u32>());
    cxxopts::ParseResult result = options.parse(argc, argv);
    if (!result.count("frame_count") || !result.count("stream_count")) {
        std::cout << options.help();
        std::exit(1);
    }
    return result;
}

int main(i32 argc, char **argv) {
    cxxopts::ParseResult args = ParseArgs(argc, argv);
    utils::IncreaseFileDescriptorLimit();

    u32 frame_count = args["frame_count"].as<u32>(),
        stream_count = args["stream_count"].as<u32>();
    vec<fut<void>> tasks;

    // Start N concurrent streams
    for (u32 id = 0; id < stream_count; ++id) {
        fut<void> f =
            std::async(std::launch::async, OpenAndReadStream, id, frame_count);
        tasks.push_back(std::move(f));

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Wait for all tasks to complete
    for (u32 i = 0; i < stream_count; ++i) {
        tasks[i].get();
    }

    return 0;
}
