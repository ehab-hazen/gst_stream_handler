#pragma once

#include "glib.h"
#include "gst/app/gstappsink.h"
#include "gst/gst.h"
#include "gst/gstbin.h"
#include "gst/gstclock.h"
#include "gst/gstelement.h"
#include "gst/gstobject.h"
#include "gst/gstparse.h"
#include "gst/video/video-info.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

template <typename T> using vec = std::vector<T>;

constexpr int INITIALIZATION_TIMEOUT_SECONDS = 5;

class StreamHandler {
public:
  StreamHandler() = delete;
  StreamHandler(const StreamHandler &) = delete;

  StreamHandler(const std::string &stream_uri)
      : stream_uri_(stream_uri), pipeline_(nullptr), is_stream_open_(true),
        stream_width_(0), stream_height_(0) {
    InitGStreamer();
    CreateNewPipeline();
    UpdateAppsink();
    StartPlaying();
    UpdateStreamResolution();
  }

  ~StreamHandler() {
    is_stream_open_ = false;
    if (pipeline_) {
      gst_element_set_state(pipeline_, GST_STATE_NULL);
      gst_object_unref(pipeline_);
    }
    if (appsink_) {
      gst_object_unref(appsink_);
    }
  }

  bool IsStreamOpen() const { return is_stream_open_; }

  size_t GetStreamWidth() const { return stream_width_; }

  size_t GetStreamHeight() const { return stream_height_; }

  vec<uint8_t> PullSample() {
    vec<uint8_t> bytes;
    size_t size = 0;

    if (!is_stream_open_) {
      return bytes;
    }

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
    if (!sample) {
      std::cerr << "Unable to read next frame -- Closing the stream\n";
      is_stream_open_ = false;
      return bytes;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      bytes.assign(map.data, map.data + map.size);
      gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);

    return bytes;
  }

private:
  std::string stream_uri_;
  bool is_stream_open_;

  int stream_width_;
  int stream_height_;

  GstElement *pipeline_;
  GstElement *appsink_;

  void CheckError(GError *&error) {
    if (error) {
      std::cerr << error->message << std::endl;
      g_clear_error(&error);
      is_stream_open_ = false;
    }
  }

  void InitGStreamer() {
    GError *error = nullptr;
    gst_init_check(nullptr, nullptr, &error);
    CheckError(error);
  }

  void CreateNewPipeline() {
    GError *error = nullptr;
    std::string pipeline_str =
        "uridecodebin uri=" + stream_uri_ +
        " ! videoconvert ! video/x-raw,format=RGB"
        " ! appsink name=sink emit-signals=true sync=false";
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    CheckError(error);

    if (!pipeline_) {
      std::cerr << "Unable to create gstreamer pipeline\n";
      is_stream_open_ = false;
    }
  }

  void UpdateAppsink() {
    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
    if (!appsink_) {
      std::cerr << "Unable to get app sink\n";
      is_stream_open_ = false;
    }
  }

  void StartPlaying() {
    GstStateChangeReturn ret =
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      std::cerr << "Unable to start playing\n";
      is_stream_open_ = false;
    }
  }

  void UpdateStreamResolution() {
    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink_), INITIALIZATION_TIMEOUT_SECONDS * GST_SECOND);
    if (!sample) {
      std::cerr << "Failed to get initial sample for resolution\n";
      is_stream_open_ = false;
      return;
    }

    GstCaps *caps = gst_sample_get_caps(sample);
    if (!caps) {
      std::cerr << "Sample has no caps\n";
      gst_sample_unref(sample);
      is_stream_open_ = false;
      return;
    }

    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, caps)) {
      std::cerr << "Failed to parse video info from caps\n";
      gst_sample_unref(sample);
      is_stream_open_ = false;
      return;
    }

    stream_width_ = info.width;
    stream_height_ = info.height;
    gst_sample_unref(sample);
  }
};
