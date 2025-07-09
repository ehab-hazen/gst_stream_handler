#pragma once

#include "cstdlib"
#include "logging.hpp"
#include "resource_monitor.hpp"
#include "sys/resource.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cxxopts.hpp>
#include <glog/logging.h>
#include <iostream>
#include <type_traits>
#include <utility>

namespace utils {

inline int Rand(int min, int max) { return min + (rand() % (max - min + 1)); }

template <typename F, typename... Args>
inline double Timed(F &&f, Args &&...args) {
    auto start = std::chrono::steady_clock::now();
    f(std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    return elapsed;
}

template <typename F, typename... Args>
inline std::pair<double, std::invoke_result_t<F, Args...>>
TimedResult(F &&f, Args &&...args) {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto start = std::chrono::steady_clock::now();
    ReturnType ret = f(std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    return {elapsed, ret};
}

/**
 * @brief Increase system's file descriptor limit for the current processes (and
 * it's children if any). This is required because the default limit of 1024 on
 * most systems is not enough for 24 streams since a single stream opens
 * multiple descriptors (the exact number will depend on the pipeline).
 */
static inline void IncreaseFileDescriptorLimit() {
    rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim)) {
        perror("getrlimit failed");
        return;
    }

    const rlim_t min_desired = 1440;
    if (rlim.rlim_cur < min_desired) {
        rlim.rlim_cur = std::min(min_desired, rlim.rlim_max);
        if (setrlimit(RLIMIT_NOFILE, &rlim)) {
            perror("setrlimit failed");
        } else {
            INFO << "Raised file descriptor limit to " << rlim.rlim_cur;
        }
    }
}

static inline void
SaveResourceUsageMetricsCsv(const cxxopts::ParseResult &args,
                            const ResourceMonitor &resource_monitor,
                            const ResourceMonitor::Metrics &metrics) {
    const std::string &filepath = args["metrics_csv"].as<std::string>();
    LOG(INFO) << "Writing usage metrics to " << filepath;

    auto [cpu_ram_metrics, gpu_metrics] = metrics;
    u32 refresh_rate = args["refresh_rate"].as<u32>();
    std::ofstream out(filepath);

    // ===== Header =====
    out << "timestamp_ms,cpu_user_ms,cpu_sys_ms";
    if (!cpu_ram_metrics.empty()) {
        const auto &first = cpu_ram_metrics[0];
        for (u32 c = 0; c < first.hardware_threads_.size(); ++c) {
            out << ",cpu" << c << "_usage";
        }
    }
    out << ",ram_kb";
    for (u32 g = 0; g < resource_monitor.GpuDeviceCount(); ++g) {
        out << ",gpu" << g << "_util"
            << ",gpu" << g << "_mem"
            << ",gpu" << g << "_enc_util"
            << ",gpu" << g << "_dec_util"
            << ",gpu" << g << "_temp"
            << ",gpu" << g << "_power"
            << ",gpu" << g << "_gpu_clock"
            << ",gpu" << g << "_mem_clock"
            << ",gpu" << g << "_sm_clock"
            << ",gpu" << g << "_vid_clock";
    }
    out << "\n";

    // ===== Rows =====
    u32 count = std::min(cpu_ram_metrics.size(), gpu_metrics.size());
    for (u32 i = 0; i < count; ++i) {
        const auto &cr = cpu_ram_metrics[i];
        const auto &gpu_metric = gpu_metrics[i];

        u64 timestamp_ms = static_cast<u64>(1000.0 * i / refresh_rate);
        u64 cpu_user_ms = cr.usage_.ru_utime.tv_sec * 1000 +
                          cr.usage_.ru_utime.tv_usec / 1000;
        u64 cpu_sys_ms = cr.usage_.ru_stime.tv_sec * 1000 +
                         cr.usage_.ru_stime.tv_usec / 1000;

        out << timestamp_ms << "," << cpu_user_ms << "," << cpu_sys_ms;

        for (const auto &core : cr.hardware_threads_) {
            out << "," << core.usage;
        }

        u64 ram_kb = cr.usage_.ru_maxrss;
        out << "," << ram_kb;

        for (const auto &g : gpu_metric) {
            out << "," << g.gpu_ << "," << g.memory_ << ","
                << g.encoder_utilization_ << "," << g.decoder_utilization_
                << "," << g.temperature_ << "," << g.power_ << ","
                << g.gpu_clocks_ << "," << g.mem_clocks_ << "," << g.sm_clocks_
                << "," << g.vid_clocks_;
        }

        out << "\n";
    }

    out.close();
}

} // namespace utils
