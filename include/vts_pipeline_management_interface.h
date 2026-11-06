/** @file vts_pipeline_management_interface.h
 * @brief Defines the platform-specific pipeline construction contract.
 */
#ifndef VTS_PIPELINE_MANAGEMENT_INTERFACE_H
#define VTS_PIPELINE_MANAGEMENT_INTERFACE_H

#include "vts_configuration.h"
#include "vts_verification.h"

#include <gst/gst.h>
#include <memory>
#include <string>

/** Implemented by each supported hardware platform. */
class PipelineManagementInterface {
public:
    /** Releases the platform implementation through the interface. */
    virtual ~PipelineManagementInterface() = default;

    /** @return Lowercase platform identifier used for configuration matching. */
    virtual std::string platform_name() const = 0;

    /** @return Whether common code should probe the input for audio and video streams. */
    virtual bool requires_codec_detection() const
    {
        return false;
    }

    /**
     * Creates the GStreamer pipeline for one media input.
     * @param configuration Runtime element and sink settings.
     * @param fileUri Absolute or relative path to the media file.
     * @param videoInfo Audio and video stream presence detected from the file.
     * @param error Receives any GStreamer parse error.
     * @return Caller-owned pipeline element; throws on configuration errors.
     */
    virtual GstElement* pipeline_creation(
        const L4Configuration& configuration,
        const std::string& fileUri,
        const DetectedVideoInfo& videoInfo,
        GError** error) const = 0;
};

/** @return The implementation selected by the platform-specific build target. */
std::unique_ptr<PipelineManagementInterface> createPlatformPipelineManagement();

#endif