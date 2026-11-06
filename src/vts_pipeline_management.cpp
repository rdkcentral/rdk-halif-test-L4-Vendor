/** @file vts_pipeline_management.cpp
 * @brief Coordinates shared codec checks and platform pipeline construction.
 */
#include "vts_pipeline_management.h"

#include <stdexcept>
#include <algorithm>
#include <cctype>


/** Binds the common layer to the implementation selected by the child build. */
PipelineManagementLayer::PipelineManagementLayer()
    : platformPipelineManagement(createPlatformPipelineManagement())
{
}

/** Validates platform identity and codec metadata, then creates the pipeline element. */
GstElement* PipelineManagementLayer::createScenarioGstPipeline(
    const L4Configuration& configuration,
    const std::string& fileUri,
    GError** error) const
{
    const std::string compiledPlatform = platformPipelineManagement->platform_name();
    std::string configuredPlatform = configuration.platform;
    std::transform(configuredPlatform.begin(), configuredPlatform.end(),
                   configuredPlatform.begin(), [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    if (configuredPlatform.find(compiledPlatform) == std::string::npos) {
        throw std::runtime_error("Configured platform '" + configuration.platform +
                                 "' does not match the platform selected at CMake configure time ('" +
                                 compiledPlatform + "')");
    }
    DetectedVideoInfo videoInfo{};
    if (platformPipelineManagement->requires_codec_detection()) {
        videoInfo = MediaStreamDetector::detect(fileUri);
    }
    return platformPipelineManagement->pipeline_creation(configuration, fileUri, videoInfo, error);
}

