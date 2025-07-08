#pragma once

#include "gst/gstbuffer.h"
#include "gst/gstsample.h"
#include "types.hpp"
#include <iostream>
#include <utility>

/**
 * @brief Proxy class that manages the lifetime of a sample pulled from a gst
 * stream. This method avoids creating a new buffer for every frame
 */
struct FrameProxy {
  public:
    FrameProxy() = delete;
    FrameProxy(const FrameProxy &) = delete;
    FrameProxy(FrameProxy &&rhs) {
        std::swap(map_, rhs.map_);
        std::swap(buffer_, rhs.buffer_);
        std::swap(sample_, rhs.sample_);

        std::cout << "[FrameProxy] move constructor called\n";
    }
    FrameProxy(GstSample *sample, GstBuffer *buffer, GstMapInfo map)
        : map_(map), buffer_(buffer), sample_(sample) {}

    ~FrameProxy() {
        gst_buffer_unmap(buffer_, &map_);
        gst_sample_unref(sample_);
    }

    size_t size() const { return map_.size; }

    u8 *data() const { return map_.data; }

  private:
    GstMapInfo map_;
    GstBuffer *buffer_;
    GstSample *sample_;
};
