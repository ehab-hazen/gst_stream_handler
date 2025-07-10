#pragma once

#include "cpu_ram_sampler.hpp"
#include "gpu_sampler.hpp"
#include "types.hpp"
#include <chrono>
#include <nvml.h>
#include <sys/resource.h>
#include <thread>

class ResourceMonitor {

  public:
    using CpuRamMetrics = vec<CpuRamSampler::Metrics>;
    using GpuMetrics = vec<GpuSampler::Metrics>;
    using Metrics = tup<CpuRamMetrics, GpuMetrics>;

    ResourceMonitor() = delete;
    ResourceMonitor(const ResourceMonitor &) = delete;
    /**
     * @brief Moving an instance will not move the already taken measurements
     * had that instance been already running
     */
    ResourceMonitor(ResourceMonitor &&rhs) {
        refresh_rate_.store(rhs.refresh_rate_.load());
    }
    ResourceMonitor(u32 refresh_rate) : refresh_rate_(refresh_rate) {}

    void SetRefreshRate(u32 refresh_rate) { refresh_rate_.store(refresh_rate); }

    Metrics Run(const atm<bool> &stop) const {
        CpuRamMetrics cpu_ram_measurements;
        GpuMetrics gpu_measurements;

        auto InternalMemUsage = [&cpu_ram_measurements, &gpu_measurements]() {
            u32 internal_mem_usage = sizeof(ResourceMonitor);
            internal_mem_usage += cpu_ram_measurements.capacity() *
                                  sizeof(CpuRamSampler::Metrics);
            for (const auto &gpus : gpu_measurements) {
                internal_mem_usage +=
                    gpus.capacity() * sizeof(GpuSampler::Metric);
            }
            return internal_mem_usage;
        };

        while (!stop.load()) {
            // Wait before the first measurement to warm the caches.
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000 / refresh_rate_));

            CpuRamSampler::Metrics cpu_ram_sample = cpu_ram_sampler_.Sample();
            // subtract the memory consumed by ResourceMonitor itself
            cpu_ram_sample.usage_.ru_maxrss -= InternalMemUsage();

            cpu_ram_measurements.push_back(cpu_ram_sample);
            gpu_measurements.push_back(gpu_sampler_.Sample());
        }

        return {cpu_ram_measurements, gpu_measurements};
    }

    void LogCpuRamMetadata() const { cpu_ram_sampler_.LogMetadata(); }

    u32 GpuDeviceCount() const { return gpu_sampler_.DeviceCount(); }

    const GpuSampler::Metadata &GetGpuMetadata(u32 index) const {
        return gpu_sampler_.GetDeviceMetadataByIndex(index);
    }

  private:
    /**
     * @brief number of measurements per second
     */
    atm<u32> refresh_rate_;

    CpuRamSampler cpu_ram_sampler_;
    GpuSampler gpu_sampler_;
};
