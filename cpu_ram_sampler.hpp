#pragma once

#include "hart_sampler.hpp"
#include "logging.hpp"
#include "types.hpp"
#include <fstream>
#include <set>
#include <sys/resource.h>

class CpuRamSampler {

  public:
    struct Metrics {
        rusage usage_;
        HartSampler::Metrics hardware_threads_;
    };

    CpuRamSampler(const CpuRamSampler &) = delete;
    CpuRamSampler(CpuRamSampler &&) = delete;
    CpuRamSampler() : hart_sampler_() {}

    Metrics Sample() const {
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);

        HartSampler::Metrics hart_metrics = hart_sampler_.Sample();
        return {usage, hart_metrics};
    }

    u32 CpuCount() const { return hart_sampler_.CpuCount(); }

    void LogMetadata() const {
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        std::set<std::string> seen;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos ||
                line.find("cpu cores") != std::string::npos ||
                line.find("cache size") != std::string::npos) {
                if (seen.insert(line).second) {
                    INFO << line;
                }
            }
        }

        std::ifstream meminfo("/proc/meminfo");
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal") != std::string::npos ||
                line.find("MemFree") != std::string::npos ||
                line.find("MemAvailable") != std::string::npos) {
                INFO << line;
            }
        }
    }

  private:
    HartSampler hart_sampler_;
};
