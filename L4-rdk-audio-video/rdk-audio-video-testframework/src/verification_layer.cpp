/** @file verification_layer.cpp
 * @brief Maps FFmpeg codec metadata to GStreamer caps and checks sink factories.
 */
#include "verification_layer.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

#include <memory>
#include <stdexcept>
#include <sstream>

namespace {
/** Converts an FFmpeg error code into its diagnostic text. */
std::string ffmpegError(int code)
{
    char message[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(code, message, sizeof(message));
    return message;
}

/** Maps supported FFmpeg codec names to encoded caps and parser factories. */
DetectedVideoInfo infoForCodec(const std::string& codec)
{
    if (codec == "h264") {
        return {"video/x-h264", "h264parse"};
    }
    if (codec == "h265" || codec == "hevc") {
        return {"video/x-h265", "h265parse"};
    }
    if (codec == "vp8") {
        return {"video/x-vp8", ""};
    }
    if (codec == "vp9") {
        return {"video/x-vp9", "vp9parse"};
    }
    if (codec == "av1") {
        return {"video/x-av1", "av1parse"};
    }
    throw std::runtime_error("Unsupported video codec: " + codec);
}
}

/** Opens the input with libavformat and identifies its best video stream. */
DetectedVideoInfo VideoCodecDetector::detect(const std::string& videoPath)
{
    AVFormatContext* formatContext = nullptr;
    const int openResult = avformat_open_input(&formatContext, videoPath.c_str(), nullptr, nullptr);
    if (openResult < 0) {
        avformat_close_input(&formatContext);
        throw std::runtime_error("FFmpeg could not open video: " + videoPath +
                                 " (" + ffmpegError(openResult) + ", code " +
                                 std::to_string(openResult) + ")");
    }
    const auto closeInput = [](AVFormatContext* context) { avformat_close_input(&context); };
        // Scope the FFmpeg context so every later error path closes the input.
        const auto closeInput = [](AVFormatContext* context) { avformat_close_input(&context); };
    std::unique_ptr<AVFormatContext, decltype(closeInput)> input(formatContext, closeInput);
    const int streamResult = avformat_find_stream_info(input.get(), nullptr);
    if (streamResult < 0) {
        throw std::runtime_error("FFmpeg could not read video streams: " + videoPath +
                                 " (" + ffmpegError(streamResult) + ", code " +
                                 std::to_string(streamResult) + ")");
    }
    const int videoStream = av_find_best_stream(input.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStream < 0) {
        throw std::runtime_error("No video stream found in: " + videoPath);
    }
    return infoForCodec(avcodec_get_name(input->streams[videoStream]->codecpar->codec_id));
}

/** Checks advertised static sink pads against the detected encoded caps. */
void VideoCodecDetector::verifySinkSupportsCodec(const std::string& sinkName,
                                                 const DetectedVideoInfo& videoInfo)
{
    GstElementFactory* factory = gst_element_factory_find(sinkName.c_str());
    if (!factory) {
        throw std::runtime_error("Video sink factory not found: " + sinkName);
    }
    if (!gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_SINK)) {
        gst_object_unref(factory);
        throw std::runtime_error("Configured video sink is not a sink: " + sinkName);
    }
    GstCaps* codecCaps = gst_caps_from_string(videoInfo.capsName.c_str());
    bool supported = false;
    for (const GList* pad = gst_element_factory_get_static_pad_templates(factory);
         pad; pad = pad->next) {
        auto* padTemplate = static_cast<GstStaticPadTemplate*>(pad->data);
        if (padTemplate->direction != GST_PAD_SINK) {
            continue;
        }
        GstCaps* sinkCaps = gst_static_pad_template_get_caps(padTemplate);
        supported = gst_caps_can_intersect(codecCaps, sinkCaps);
        gst_caps_unref(sinkCaps);
        if (supported) {
            break;
        }
    }
    gst_caps_unref(codecCaps);
    gst_object_unref(factory);
    if (!supported) {
        throw std::runtime_error("Video sink '" + sinkName + "' does not advertise support for " +
                                 videoInfo.capsName);
    }
}

