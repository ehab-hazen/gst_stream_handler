#include "argparse.hpp"
#include "iostream"
#include "logging.hpp"
#include "resource_monitor.hpp"
#include "stream_handler.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <chrono>
#include <cstdlib>
#include <cxxopts.hpp>
#include <future>
#include <glog/logging.h>
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
            INFO << "Stream [" << stream_handler->GetId() << "]: Read "
                 << bytes.size() << "/" << width * height * 3 << " bytes";
        }
    }
    return read_count;
}

void OpenAndReadStream(i32 id, u32 frame_count) {
    std::string stream_uri =
        "rtsp://127.0.0.1:" + std::to_string(8554 + id) + "/stream";
    up<StreamHandler> stream_handler =
        StreamHandler::OpenStream(id, stream_uri);
    if (stream_handler == nullptr) {
        ERROR << "Stream [" << id << "]: Failed to open stream";
    } else {
        INFO << "Stream [" << id << "]: Opened stream, uri = " << stream_uri;

        auto [elapsed, read_count] = utils::TimedResult(
            ReadNFrames, std::move(stream_handler), frame_count);

        double fps = read_count / elapsed;
        INFO << "Stream [" << id << "]: Read " << read_count << "/"
             << frame_count << " in " << elapsed << "s, FPS = " << fps;
    }
}

auto StartResourceMonitor(const ResourceMonitor &resource_monitor,
                          const atm<bool> &stop) {
    return std::async(std::launch::async, [&stop, &resource_monitor]() {
        // Log GPU metadata
        for (u32 i = 0; i < resource_monitor.GpuDeviceCount(); ++i) {
            const auto &meta = resource_monitor.GetGpuMetadata(i);
            INFO << "GPU " << i << ": " << meta.name
                 << ", Memory: " << meta.memory_total << " MB"
                 << ", Power Limit: " << meta.power_limit << " mW"
                 << ", Max GPU Clock: " << meta.max_gpu_clock << " MHz"
                 << ", Max Mem Clock: " << meta.max_mem_clock << " MHz";
        }

        resource_monitor.LogCpuRamMetadata();
        return resource_monitor.Run(stop);
    });
}

cxxopts::ParseResult Init(int argc, char **argv) {
    utils::IncreaseFileDescriptorLimit();
    google::InitGoogleLogging(argv[0]);
    FLAGS_minloglevel = 0;
    fLB::FLAGS_logtostderr = true;
    return ParseArgs(argc, argv);
}

int main(i32 argc, char **argv) {
    cxxopts::ParseResult args = Init(argc, argv);

    u32 frame_count = args["frame_count"].as<u32>(),
        stream_count = args["stream_count"].as<u32>();
    vec<fut<void>> tasks;
    atm<bool> stop = false;

    ResourceMonitor resource_monitor(2);
    auto metrics = StartResourceMonitor(resource_monitor, stop);

    // Start N concurrent streams
    INFO << "Starting " << stream_count << " concurrent streams";
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

    // Stop resource monitor
    stop.store(true);
    utils::SaveResourceUsageMetricsCsv(args, resource_monitor, metrics.get());

    return 0;
}
