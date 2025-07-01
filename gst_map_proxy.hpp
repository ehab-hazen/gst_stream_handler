#pragma once

#include "gst/gstbuffer.h"
#include "gst/gstsample.h"
#include "types.hpp"

/**
 * @brief Proxy class that manages the lifetime of a sample pulled from a gst
 * stream. This method avoids creating a new buffer for every frame
 */
struct GstMapProxy {
  public:
    GstMapProxy() = delete;
    GstMapProxy(const GstMapProxy &) = delete;
    GstMapProxy(GstSample *sample, GstBuffer *buffer, GstMapInfo map)
        : sample_(sample), buffer_(buffer), map_(map) {}

    ~GstMapProxy() {
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
