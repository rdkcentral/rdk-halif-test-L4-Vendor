/** @file vts_realtek_pipeline_management.cpp
 * @brief Builds scenario pipelines for Realtek devices.
 */
#include "vts_pipeline_management_interface.h"

#include <glib.h>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace {
/** Converts the media path to an absolute file URI for playbin. */
std::string mediaUri(const std::string& fileUri)
{
    const std::filesystem::path path(fileUri);
    const std::string absolutePath =
        std::filesystem::absolute(path).lexically_normal().string();
    gchar* uri = g_filename_to_uri(absolutePath.c_str(), nullptr, nullptr);
    if (!uri) {
        throw std::runtime_error("Cannot convert media path to URI: " + absolutePath);
    }
    const std::string result(uri);
    g_free(uri);
    return result;
}

/** Returns the configured sink element, failing when it is not set in the YAML. */
std::string requiredSink(const std::string& sinkName, const std::string& key)
{
    if (sinkName.empty()) {
        throw std::runtime_error(key + " must be set in platform.yaml for playbin");
    }
    return sinkName;
}

/** Creates the named sink and attaches it to playbin, which takes its own reference. */
void attachSink(GstElement* playbin, const char* property, const std::string& factoryName)
{
    GstElement* sink = gst_element_factory_make(factoryName.c_str(), nullptr);
    if (!sink) {
        gst_object_unref(playbin);
        throw std::runtime_error("Could not create sink element: " + factoryName);
    }
    g_object_set(playbin, property, sink, nullptr);
    gst_object_unref(sink);
}

GstElement* createPlaybin(const std::string& uri)
{
    GstElement* playbin = gst_element_factory_make("playbin", "player");
    if (!playbin) {
        throw std::runtime_error("Could not create playbin element");
    }
    g_object_set(playbin, "uri", uri.c_str(), nullptr);
    return playbin;
}

class RealtekPipelineManagement final : public PipelineManagementInterface {
public:
    /** Identifies this implementation for configuration/build consistency checks. */
    std::string platform_name() const override
    {
        return "realtek";
    }

    /** Enables stream probing so the sinks attached match the streams in the file. */
    bool requires_codec_detection() const override
    {
        return true;
    }

    /** Selects the playbin pipeline that matches the audio and video streams in the file. */
    GstElement* pipeline_creation(const L4Configuration& configuration,
                                  const std::string& fileUri,
                                  const DetectedVideoInfo& videoInfo,
                                  GError**) const override
    {
        const std::string uri = mediaUri(fileUri);
        if (videoInfo.hasAudio && !videoInfo.hasVideo) {
            return createAudioOnlyPipeline(configuration, uri);
        }
        if (!videoInfo.hasAudio && videoInfo.hasVideo) {
            return createVideoOnlyPipeline(configuration, uri);
        }
        if (videoInfo.hasAudio && videoInfo.hasVideo) {
            return createAudioVideoPipeline(configuration, uri);
        }
        throw std::runtime_error("No audio or video stream found in: " + fileUri);
    }

private:
    /** Builds a playbin that outputs only audio. */
    GstElement* createAudioOnlyPipeline(const L4Configuration& configuration,
                                        const std::string& uri) const
    {
        const std::string audioSink = requiredSink(configuration.audioSink, "elements.audio_sink");
        GstElement* playbin = createPlaybin(uri);
        attachSink(playbin, "audio-sink", audioSink);
        return playbin;
    }

    /** Builds a playbin that outputs only video. */
    GstElement* createVideoOnlyPipeline(const L4Configuration& configuration,
                                        const std::string& uri) const
    {
        const std::string videoSink = requiredSink(configuration.videoSink, "elements.video_sink");
        GstElement* playbin = createPlaybin(uri);
        attachSink(playbin, "video-sink", videoSink);
        return playbin;
    }

    /** Builds a playbin that outputs both audio and video. */
    GstElement* createAudioVideoPipeline(const L4Configuration& configuration,
                                         const std::string& uri) const
    {
        const std::string audioSink = requiredSink(configuration.audioSink, "elements.audio_sink");
        const std::string videoSink = requiredSink(configuration.videoSink, "elements.video_sink");
        GstElement* playbin = createPlaybin(uri);
        attachSink(playbin, "audio-sink", audioSink);
        attachSink(playbin, "video-sink", videoSink);
        return playbin;
    }
};
}

/** Creates the implementation linked by the platform-selected CMake target. */
std::unique_ptr<PipelineManagementInterface> createPlatformPipelineManagement()
{
    return std::make_unique<RealtekPipelineManagement>();
}