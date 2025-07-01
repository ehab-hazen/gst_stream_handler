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
#include "gst/gstparse.h"
#include "gst/gstsample.h"
#include "gst/video/video-info.h"
#include "gst_map_proxy.hpp"
#include "types.hpp"
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

template <typename T> using vec = std::vector<T>;

constexpr int INITIALIZATION_TIMEOUT_SECONDS = 5;

class StreamHandler {
  public:
    StreamHandler() = delete;
    StreamHandler(const StreamHandler &) = delete;

    StreamHandler(int id, const std::string &stream_uri, int fps_limit)
        : id_(id), stream_uri_(stream_uri), fps_limit_(fps_limit),
          pipeline_(nullptr), is_stream_open_(true), stream_width_(0),
          stream_height_(0) {
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

    opt<GstMapProxy> PullSample() {
        if (!is_stream_open_) {
            return std::nullopt;
        }

        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
        if (!sample) {
            std::cerr << "[StreamHandler][PullSample] Unable to read next "
                         "frame -- Closing the stream\n";
            is_stream_open_ = false;
            return std::nullopt;
        }

        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            return std::make_optional<GstMapProxy>(sample, buffer, map);
        } else {
            gst_sample_unref(sample);
            return std::nullopt;
        }
    }

    static up<StreamHandler> OpenStream(i32 id, const std::string &uri,
                                        u32 retry_count = 3) {
        for (i32 i = 0; i < retry_count; ++i) {
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

        std::string pipeline_description =
            "uridecodebin uri=" + stream_uri_ +
            " name=src "
            "src. ! queue ! nvh264dec ! "
            "nvvidconv ! video/x-raw(memory:NVMM),format=RGBA ! "
            "videorate ! video/x-raw,framerate=" +
            std::to_string(fps_limit_) +
            "/1 ! "
            "queue max-size-buffers=3 leaky=downstream ! "
            "appsink sync=false name=sink caps=video/x-raw,format=RGBA";
        pipeline_ = gst_parse_launch(pipeline_description.c_str(), &error);

        if (error || !pipeline_) {
            if (error) {
                std::cerr << error->message << "\n";
                g_clear_error(&error);
            }
            std::cerr << "[StreamHandler][CreateNewPipeline] Unable to use "
                         "hardware acceleration, falling back to "
                         "software elements\n";

            const std::string kAppsinkCaps =
                "video/x-raw,format=RGB,pixel-aspect-ratio=1/1";
            const std::string frame_rate_caps =
                "max-rate=" + std::to_string(fps_limit_) + " drop-only=true";
            pipeline_description =
                "uridecodebin uri=" + stream_uri_ +
                " ! videoconvert ! videoscale !"
                " videorate " +
                frame_rate_caps +
                " ! queue max-size-buffers=3 leaky=downstream ! " +
                "appsink sync=false name=sink caps=\"" + kAppsinkCaps + "\"";
            pipeline_ = gst_parse_launch(pipeline_description.c_str(), &error);
        }
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

    void Play() {
        GstStateChangeReturn ret =
            gst_element_set_state(pipeline_, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            std::cerr << "Unable to start playing\n";
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
            GstClockTime timeout = 15 * GST_SECOND;
            GstStateChangeReturn ret =
                gst_element_get_state(pipeline_, &state, &pending, timeout);

            std::ostringstream msg;
            msg << "Stream [" << id_
                << "]: Pipeline state: " << gst_element_state_get_name(state)
                << ", pending: " << gst_element_state_get_name(pending)
                << ", return: " << ret << "\n";

            std::cerr << msg.str();
            is_stream_open_ = false;
            return;
        }

        GstCaps *caps = gst_sample_get_caps(sample);
        GstVideoInfo info;
        if (!caps) {
            std::cerr << "Sample has no caps\n";
            is_stream_open_ = false;
            return;
        } else if (!gst_video_info_from_caps(&info, caps)) {
            std::cerr << "Failed to parse video info from caps\n";
            is_stream_open_ = false;
            return;
        } else {
            stream_width_ = info.width;
            stream_height_ = info.height;
        }
        gst_sample_unref(sample);
    }
};
