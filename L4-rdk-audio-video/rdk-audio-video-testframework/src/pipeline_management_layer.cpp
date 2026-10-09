/** @file pipeline_management_layer.cpp
 * @brief Coordinates shared codec checks and platform pipeline construction.
 */
#include "pipeline_management_layer.h"

#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>


/** Binds the common layer to the implementation selected by the child build. */
PipelineManagementLayer::PipelineManagementLayer()
    : platformPipelineManagement(createPlatformPipelineManagement())
{
}

/** Validates platform identity and codec metadata before delegating pipeline assembly. */
std::string PipelineManagementLayer::createScenarioPipeline(
    const L4Configuration& configuration,
    const std::string& videoPath) const
{
    const std::string compiledPlatform = platformPipelineManagement->platform_name();
    std::string configuredPlatform = configuration.platform;
    std::transform(configuredPlatform.begin(), configuredPlatform.end(),
                    // Normalize configuration input so casing does not affect platform matching.
                    std::transform(configuredPlatform.begin(), configuredPlatform.end(),
                   configuredPlatform.begin(), [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
        if (configuredPlatform.find(compiledPlatform) == std::string::npos) {
        throw std::runtime_error("Configured platform '" + configuration.platform +
                                 "' does not match the platform selected at CMake configure time ('" +
                                 compiledPlatform + "')");
    }
    const DetectedVideoInfo videoInfo = VideoCodecDetector::detect(videoPath);
    if (!configuration.videoSink.empty()) {
        VideoCodecDetector::verifySinkSupportsCodec(configuration.videoSink, videoInfo);
    }
    return platformPipelineManagement->pipeline_creation(configuration, videoPath, videoInfo);
}

/** Parses the generated launch string; ownership of the returned element passes to the caller. */
GstElement* PipelineManagementLayer::createScenarioGstPipeline(
    const L4Configuration& configuration,
    const std::string& videoPath,
    GError** error) const
{
    const std::string pipeline = createScenarioPipeline(configuration, videoPath);
    return gst_parse_launch(pipeline.c_str(), error);
}

// --- GstValidateLauncherBuilder ---

/** Sets the pipeline description appended to the launcher command. */
GstValidateLauncherBuilder& GstValidateLauncherBuilder::setPipelineDescription(const std::string& pipeline)
{
    mPipelineDescription = pipeline;
    return *this;
}

/** Sets the scenario file used by gst-validate-launcher. */
GstValidateLauncherBuilder& GstValidateLauncherBuilder::setScenario(const std::string& scenarioFile)
{
    mScenarioFile = scenarioFile;
    return *this;
}

/** Controls the launcher's expected-issues mode. */
GstValidateLauncherBuilder& GstValidateLauncherBuilder::setExpectIssues(bool expect)
{
    mExpectIssues = expect;
    return *this;
}

/** Appends one launcher argument in insertion order. */
GstValidateLauncherBuilder& GstValidateLauncherBuilder::addExtraArg(const std::string& arg)
{
    mExtraArgs.push_back(arg);
    return *this;
}

/** Builds the command string and rejects an unset pipeline description. */
std::string GstValidateLauncherBuilder::build() const
{
    if (mPipelineDescription.empty()) {
        throw std::runtime_error("GstValidateLauncherBuilder: pipeline description must be set");
    }
    std::ostringstream cmd;
    cmd << "gst-validate-launcher";
    cmd << " --set-scenario " << (mScenarioFile.empty() ? "none" : mScenarioFile);
    if (mExpectIssues) {
        cmd << " --expect-issues";
    }
    for (const auto& arg : mExtraArgs) {
        cmd << " " << arg;
    }
    cmd << " -- " << mPipelineDescription;
    return cmd.str();
}

