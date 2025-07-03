#pragma once

#include "types.hpp"
#include <fstream>
#include <sstream>

/**
 * @brief Implementation class for sampling usage of a single hardware thread
 * (HART)
 */
class HartSampler {

  public:
    struct Metric {
        std::string id;
        u64 user, nice, system, idle, iowait, irq, softirq, steal;
        double usage;

        u64 Active() const {
            return user + nice + system + irq + softirq + steal;
        }

        u64 Total() const { return Active() + idle + iowait; }
    };

    using Metrics = vec<Metric>;

    HartSampler(const HartSampler &) = delete;
    HartSampler(HartSampler &&) = delete;
    HartSampler()
        : cpu_count_(std::thread::hardware_concurrency()),
          last_sample_(ReadCpuStats()) {}

    Metrics Sample() const {
        Metrics current = ReadCpuStats();
        vec<double> usages = ComputeCoreUsage(last_sample_, current);
        for (size_t i = 0; i < usages.size(); ++i) {
            current[i].usage = usages[i];
        }
        last_sample_ = std::move(current);
        return last_sample_;
    }

    u32 CpuCount() const { return cpu_count_; }

  private:
    u32 cpu_count_;
    mutable Metrics last_sample_;

    vec<double> ComputeCoreUsage(const vec<Metric> &a,
                                 const vec<Metric> &b) const {
        vec<double> usage;
        for (size_t i = 0; i < a.size(); ++i) {
            u64 total_diff = b[i].Total() - a[i].Total();
            u64 active_diff = b[i].Active() - a[i].Active();
            double percent =
                total_diff == 0 ? 0.0 : 100.0 * active_diff / total_diff;
            usage.push_back(percent);
        }
        return usage;
    }

    Metrics ReadCpuStats() const {
        std::ifstream stat("/proc/stat");
        std::string line;
        vec<Metric> cores;

        while (std::getline(stat, line)) {
            if (line.rfind("cpu", 0) == 0 && line[3] != ' ') { // cpu0, cpu1...
                std::istringstream ss(line);
                Metric snap;
                ss >> snap.id >> snap.user >> snap.nice >> snap.system >>
                    snap.idle >> snap.iowait >> snap.irq >> snap.softirq >>
                    snap.steal;
                cores.push_back(snap);
            }
        }
        return cores;
    }
};
