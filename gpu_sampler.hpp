#pragma once

#include "types.hpp"
#include <glog/logging.h>
#include <nvml.h>
#include <utility>

class GpuSampler {

  public:
    struct Metric {
        /**
         * @brief GPU utilization
         */
        u32 gpu_;
        /**
         * @brief GPU memory usage
         */
        u32 memory_;
        u32 power_;
        u32 temperature_;

        u32 gpu_clocks_;
        u32 mem_clocks_;
        u32 sm_clocks_;
        u32 vid_clocks_;
    };

    struct Metadata {
        std::string name;
        u32 memory_total;
        u32 power_limit;
        u32 max_gpu_clock;
        u32 max_mem_clock;
    };

    using Metrics = vec<Metric>;

    GpuSampler() { Init(); }
    GpuSampler(const GpuSampler &) = delete;
    GpuSampler(GpuSampler &&rhs) {
        std::swap(device_count_, rhs.device_count_);
        std::swap(devices_, rhs.devices_);
        std::swap(metadata_, rhs.metadata_);
    }

    ~GpuSampler() { Shutdown(); }

    u32 DeviceCount() const { return device_count_; }

    const Metadata &GetDeviceMetadataByIndex(u32 index) const {
        return metadata_[index];
    }

    Metrics Sample() const {
        Metrics metrics;

        for (u32 i = 0; i < device_count_; ++i) {
            Metric metric;
            nvmlUtilization_t utilization;

            nvmlDeviceGetTemperature(devices_[i], NVML_TEMPERATURE_GPU,
                                     &metric.temperature_);
            nvmlDeviceGetUtilizationRates(devices_[i], &utilization);
            nvmlDeviceGetPowerUsage(devices_[i], &metric.power_);

            nvmlDeviceGetClockInfo(devices_[i], NVML_CLOCK_GRAPHICS,
                                   &metric.gpu_clocks_);
            nvmlDeviceGetClockInfo(devices_[i], NVML_CLOCK_MEM,
                                   &metric.mem_clocks_);
            nvmlDeviceGetClockInfo(devices_[i], NVML_CLOCK_VIDEO,
                                   &metric.vid_clocks_);
            nvmlDeviceGetClockInfo(devices_[i], NVML_CLOCK_SM,
                                   &metric.sm_clocks_);

            metric.gpu_ = utilization.gpu;
            metric.memory_ = utilization.memory;

            metrics.push_back(metric);
        }

        return metrics;
    }

  private:
    u32 device_count_;
    vec<nvmlDevice_t> devices_;
    vec<Metadata> metadata_;

    void Init() {
        nvmlReturn_t result;

        // Initialize nvml
        result = nvmlInit();
        if (result != NVML_SUCCESS) {
            LOG(ERROR) << "Failed to initialize NVML: "
                       << nvmlErrorString(result) << "\n";
            return Shutdown();
        }

        // Get GPU count
        result = nvmlDeviceGetCount(&device_count_);
        if (result != NVML_SUCCESS) {
            printf("Failed to query device count: %s\n",
                   nvmlErrorString(result));
            return Shutdown();
        }

        // Get handles and metadata for all devices
        for (u32 i = 0; i < device_count_; ++i) {
            nvmlDevice_t device;
            result = nvmlDeviceGetHandleByIndex(i, &device);
            if (result != NVML_SUCCESS) {
                printf("Failed to get handle for device %i: %s\n", i,
                       nvmlErrorString(result));
                return Shutdown();
            }

            devices_.push_back(device);
            metadata_.push_back(GetDeviceMetadata(device));
        }
    }

    Metadata GetDeviceMetadata(nvmlDevice_t device) {
        Metadata meta{};
        char name_buf[64];
        nvmlDeviceGetName(device, name_buf, sizeof(name_buf));
        meta.name = name_buf;

        nvmlMemory_t mem;
        nvmlDeviceGetMemoryInfo(device, &mem);
        meta.memory_total = mem.total / 1024 / 1024;

        nvmlDeviceGetPowerManagementLimit(device, &meta.power_limit);
        nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_GRAPHICS,
                                  &meta.max_gpu_clock);
        nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_MEM, &meta.max_mem_clock);

        return meta;
    }

    void Shutdown() {
        nvmlReturn_t result = nvmlShutdown();
        if (result != NVML_SUCCESS) {
            printf("Failed to shutdown NVML: %s\n", nvmlErrorString(result));
        }
    }
};
