/** @file platform_pipeline_management.cpp
 * @brief Builds scenario pipelines for Amlogic devices.
 */
#include "pipeline_management_interface.h"

#include <filesystem>

namespace {
class AmlogicPipelineManagement final : public PipelineManagementInterface {
public:
    /** Identifies this implementation for configuration/build consistency checks. */
    std::string platform_name() const override
    {
        return "amlogic";
    }

    /** Selects demuxing and codec handling, then appends the configured sink. */
    std::string pipeline_creation(const L4Configuration& configuration,
                                 const std::string& videoPath,
                                 const DetectedVideoInfo& videoInfo) const override
    {
        const std::filesystem::path path(videoPath);
        const std::string absolutePath =
            std::filesystem::absolute(path).lexically_normal().string();
        const std::string& demuxElement = path.extension() == ".ts"
                                                // The extension selects the demuxer for MPEG-TS versus ISO BMFF inputs.
                                                const std::string& demuxElement = path.extension() == ".ts"
                                              ? configuration.tsDemuxElement
                                              : configuration.mp4DemuxElement;
        std::string pipeline = configuration.scenarioSourceElement +
                               " location=\"" + absolutePath + "\"";
        if (!demuxElement.empty()) {
            pipeline += " ! " + demuxElement;
        }
        if (videoInfo.capsName == "video/x-vp8") {
            // Decode VP8 in software and expose raw frames at the probe point.
            pipeline += " ! avdec_vp8 ! " + configuration.videoConvertElement +
                        " name=timestamp_output";
        } else {
            pipeline += " ! " + videoInfo.parserElement + " name=timestamp_output";
        }
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
    return std::make_unique<AmlogicPipelineManagement>();
}