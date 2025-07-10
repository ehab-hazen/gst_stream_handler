#pragma once

#include "glib.h"
#include "gst/app/gstappsink.h"
#include "gst/gst.h"
#include "gst/gstbin.h"
#include "gst/gstbuffer.h"
#include "gst/gstclock.h"
#include "gst/gstelement.h"
#include "gst/gstmemory.h"
#include "gst/gstobject.h"
#include "gst/gstsample.h"
#include "gst/video/video-info.h"
#include "logging.hpp"
#include "types.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

template <typename T> using vec = std::vector<T>;

constexpr int INITIALIZATION_TIMEOUT_SECONDS = 5;

class StreamHandler {
  public:
    StreamHandler() = delete;
    StreamHandler(const StreamHandler &) = delete;

    StreamHandler(int id, const std::string &stream_uri, int fps_limit)
        : id_(id), stream_uri_(stream_uri), is_stream_open_(true),
          fps_limit_(fps_limit), stream_width_(0), stream_height_(0),
          pipeline_(nullptr) {
        InitGStreamer();
        CreateNewPipeline();
        UpdateAppsink();
        Play();
        UpdateStreamResolution();
    }

    ~StreamHandler() {
        is_stream_open_ = false;
        if (pipeline_) {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            // Wait for state change to finish
            gst_element_get_state(pipeline_, NULL, NULL, GST_CLOCK_TIME_NONE);
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
        }
        if (appsink_) {
            gst_object_unref(appsink_);
            appsink_ = nullptr;
        }
    }

    bool IsStreamOpen() const { return is_stream_open_; }

    int GetId() const { return id_; }

    size_t GetStreamWidth() const { return stream_width_; }

    size_t GetStreamHeight() const { return stream_height_; }

    int GetFPSLimit() const { return fps_limit_; }

    vec<u8> PullSample() {
        vec<u8> bytes;
        if (!is_stream_open_) {
            return bytes;
        }

        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
        if (!sample) {
            ERROR << "[StreamHandler][PullSample] Unable to read next "
                     "frame -- Closing the stream";
            is_stream_open_ = false;
            return bytes;
        }

        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            bytes.assign(map.data, map.data + map.size);
            gst_buffer_unmap(buffer, &map);
            gst_sample_unref(sample);
            return bytes;
        } else {
            gst_sample_unref(sample);
            return bytes;
        }
    }

    static up<StreamHandler> OpenStream(i32 id, const std::string &uri,
                                        u32 retry_count = 3) {
        for (u32 i = 0; i < retry_count; ++i) {
            up<StreamHandler> stream_handler =
                std::make_unique<StreamHandler>(id, uri, 30);
            if (stream_handler->IsStreamOpen()) {
                return stream_handler;
            }
        }
        return nullptr;
    }

  private:
    int id_;
    std::string stream_uri_;
    bool is_stream_open_;

    int fps_limit_;
    int stream_width_;
    int stream_height_;

    GstElement *pipeline_;
    GstElement *appsink_;

    void CheckError(GError *&error) {
        if (error) {
            ERROR << error->message;
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

        const std::string pipeline_description =
            "rtspsrc location=" + stream_uri_ +
            " latency=0"
            " ! rtph264depay"
            " ! h264parse"
            " ! avdec_h264"
            " ! videorate max-rate=" +
            std::to_string(fps_limit_) +
            " drop-only=true"
            " ! videoscale"
            " ! videoconvert"
            " ! video/x-raw,format=RGB,framerate=" +
            std::to_string(fps_limit_) +
            "/1,pixel-aspect-ratio=1/1"
            " ! queue max-size-buffers=3 leaky=downstream"
            " ! appsink name=sink sync=false";

        /*const std::string kAppsinkCaps =*/
        /*    "video/x-raw,format=RGB,pixel-aspect-ratio=1/1";*/
        /*const std::string frame_rate_caps =*/
        /*    "max-rate=" + std::to_string(fps_limit_) + " drop-only=true";*/
        /*const std::string pipeline_description =*/
        /*    "uridecodebin uri=" + stream_uri_ +*/
        /*    " ! videoconvert ! videoscale !"*/
        /*    " videorate " +*/
        /*    frame_rate_caps +*/
        /*    " ! queue max-size-buffers=3 leaky=downstream ! " +*/
        /*    "appsink sync=false name=sink caps=\"" + kAppsinkCaps + "\"";*/
        pipeline_ = gst_parse_launch(pipeline_description.c_str(), &error);
        CheckError(error);

        if (!pipeline_) {
            ERROR << "Unable to create gstreamer pipeline";
            is_stream_open_ = false;
        }
    }

    void UpdateAppsink() {
        appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
        if (!appsink_) {
            ERROR << "Unable to get app sink";
            is_stream_open_ = false;
        }
    }

    void Play() {
        GstStateChangeReturn ret =
            gst_element_set_state(pipeline_, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            ERROR << "Unable to start playing";
            is_stream_open_ = false;
        }
    }

    void UpdateStreamResolution() {
        GstSample *sample = gst_app_sink_try_pull_sample(
            GST_APP_SINK(appsink_),
            INITIALIZATION_TIMEOUT_SECONDS * GST_SECOND);
        if (!sample) {
            GstState state;
            GstState pending;
            GstClockTime timeout = 5 * GST_SECOND;
            GstStateChangeReturn ret =
                gst_element_get_state(pipeline_, &state, &pending, timeout);

            ERROR << "Stream [" << id_
                  << "]: Pipeline state: " << gst_element_state_get_name(state)
                  << ", pending: " << gst_element_state_get_name(pending)
                  << ", return: " << ret;
            is_stream_open_ = false;
            return;
        }

        GstCaps *caps = gst_sample_get_caps(sample);
        GstVideoInfo info;
        if (!caps) {
            ERROR << "Sample has no caps";
            is_stream_open_ = false;
            return;
        } else if (!gst_video_info_from_caps(&info, caps)) {
            ERROR << "Failed to parse video info from caps";
            is_stream_open_ = false;
            return;
        } else {
            stream_width_ = info.width;
            stream_height_ = info.height;
        }
        gst_sample_unref(sample);
    }
};
