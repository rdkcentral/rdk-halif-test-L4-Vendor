/** @file pipeline_management_interface.h
 * @brief Defines the platform-specific pipeline construction contract.
 */
#ifndef PIPELINE_MANAGEMENT_INTERFACE_H
#define PIPELINE_MANAGEMENT_INTERFACE_H

#include "l4_configuration.h"
#include "verification_layer.h"

#include <memory>
#include <string>

/** Implemented by each supported hardware platform. */
class PipelineManagementInterface {
public:
    /** Releases the platform implementation through the interface. */
    virtual ~PipelineManagementInterface() = default;

    /** @return Lowercase platform identifier used for configuration matching. */
    virtual std::string platform_name() const = 0;

    /**
     * Creates a GStreamer launch description for one media input.
     * @param configuration Runtime element and sink settings.
     * @param videoPath Absolute or relative path to the media file.
     * @param videoInfo Codec and parser information detected from the file.
     * @return A launch string for the selected platform pipeline.
     */
    virtual std::string pipeline_creation(
        const L4Configuration& configuration,
        const std::string& videoPath,
        const DetectedVideoInfo& videoInfo) const = 0;
};

/** @return The implementation selected by the platform-specific build target. */
std::unique_ptr<PipelineManagementInterface> createPlatformPipelineManagement();

#endif