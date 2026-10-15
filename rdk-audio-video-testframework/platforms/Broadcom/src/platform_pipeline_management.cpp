/** @file platform_pipeline_management.cpp
 * @brief Builds scenario pipelines for Broadcom devices.
 */
#include "pipeline_management_interface.h"

#include <filesystem>
#include <stdexcept>

namespace {
class BroadcomPipelineManagement final : public PipelineManagementInterface {
public:
    /** Identifies this implementation for configuration/build consistency checks. */
    std::string platform_name() const override
    {
        return "broadcom";
    }

    /** Broadcom's video filter handles codec-specific input before the sink. */
    bool requires_codec_detection() const override
    {
        return false;
    }

    /** Selects the demuxer, then appends the configured video filter and sink. */
    std::string pipeline_creation(const L4Configuration& configuration,
                                 const std::string& videoPath,
                                 const DetectedVideoInfo&) const override
    {
        const std::filesystem::path path(videoPath);
        const std::string absolutePath =
            std::filesystem::absolute(path).lexically_normal().string();
        const std::string& demuxElement = path.extension() == ".ts"
                                              ? configuration.tsDemuxElement
                                              : configuration.mp4DemuxElement;
        if (configuration.videoFilter.empty()) {
            throw std::runtime_error("Broadcom video_filter is required");
        }
        std::string pipeline = configuration.scenarioSourceElement +
                               " location=\"" + absolutePath + "\"";
        if (!demuxElement.empty()) {
            pipeline += " ! " + demuxElement;
        }
        pipeline += " ! " + configuration.videoFilter + " name=timestamp_output";
        if (!configuration.videoSink.empty()) {
            pipeline += " ! " + configuration.videoSink;
        }
        return pipeline;
    }
};
}

/** Creates the implementation linked by the platform-selected CMake target. */
std::unique_ptr<PipelineManagementInterface> createPlatformPipelineManagement()
{
    return std::make_unique<BroadcomPipelineManagement>();
}