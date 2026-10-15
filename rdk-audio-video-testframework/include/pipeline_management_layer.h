/** @file pipeline_management_layer.h
 * @brief Declares common pipeline assembly and GstValidate command helpers.
 */
#ifndef PIPELINE_MANAGEMENT_LAYER_H
#define PIPELINE_MANAGEMENT_LAYER_H

#include "l4_configuration.h"
#include "pipeline_management_interface.h"
#include <gst/gst.h>
#include <string>
#include <vector>



/** Coordinates codec detection, platform checks, and launch-string creation. */
class PipelineManagementLayer {
public:
    /** Creates the platform implementation selected during CMake configuration. */
    PipelineManagementLayer();

    /**
     * Builds a platform pipeline after checking the configured platform and codec.
     * @param configuration Runtime platform and element settings.
     * @param videoPath Input media path used for codec detection and pipeline creation.
     * @return GStreamer launch description.
     */
    std::string createScenarioPipeline(const L4Configuration& configuration,
                                       const std::string& videoPath) const;

    /**
     * Parses the platform launch description into a GStreamer element.
     * @param configuration Runtime platform and element settings.
     * @param videoPath Input media path.
     * @param error Receives any GStreamer parse error.
     * @return The caller-owned parsed element, or null on parse failure.
     */
                                    GstElement* createScenarioGstPipeline(const L4Configuration& configuration,
                                                                          const std::string& videoPath,
                                                                          GError** error) const;

private:

    std::unique_ptr<PipelineManagementInterface> platformPipelineManagement;
};

/** Builds a gst-validate-launcher command from pipeline and scenario options. */
class GstValidateLauncherBuilder {
public:
    /** Sets the launch description passed after the command separator. */
    GstValidateLauncherBuilder& setPipelineDescription(const std::string& pipeline);

    /** Sets the scenario file, or `none` when no scenario is required. */
    GstValidateLauncherBuilder& setScenario(const std::string& scenarioFile);

    /** Enables the launcher's expected-issues mode. */
    GstValidateLauncherBuilder& setExpectIssues(bool expect);

    /** Appends one launcher argument without shell quoting or tokenization. */
    GstValidateLauncherBuilder& addExtraArg(const std::string& arg);

    /** @return The assembled command line; throws if no pipeline was configured. */
    std::string build() const;


private:
    std::string mPipelineDescription;
    std::string mScenarioFile;
    bool mExpectIssues{ false };
    std::vector<std::string> mExtraArgs;
};

#endif