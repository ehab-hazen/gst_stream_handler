#pragma once

#include "iostream"
#include "types.hpp"
#include <cxxopts.hpp>
#include <string>

static inline cxxopts::ParseResult ParseArgs(int argc, char **argv) {
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
        cxxopts::value<u32>())("r,refresh_rate",
                               "Number of (resource usage) "
                               "samples to collect per second.",
                               cxxopts::value<u32>()->default_value("2"))(
        "m,metrics_csv",
        "Path to csv file where usage metrics should be saved."
        "./metrics.csv",
        cxxopts::value<std::string>()->default_value("./metrics.csv"));
    cxxopts::ParseResult result = options.parse(argc, argv);
    if (!result.count("frame_count") || !result.count("stream_count")) {
        std::cout << options.help();
        std::exit(1);
    }
    return result;
}
