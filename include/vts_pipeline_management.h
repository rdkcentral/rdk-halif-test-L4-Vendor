/** @file vts_pipeline_management.h
 * @brief Declares common pipeline assembly.
 */
#ifndef VTS_PIPELINE_MANAGEMENT_H
#define VTS_PIPELINE_MANAGEMENT_H

#include "vts_configuration.h"
#include "vts_pipeline_management_interface.h"
#include <gst/gst.h>
#include <string>



/** Coordinates codec detection, platform checks, and pipeline creation. */
class PipelineManagementLayer {
public:
    /** Creates the platform implementation selected during CMake configuration. */
    PipelineManagementLayer();

    /**
     * Builds the platform pipeline after checking the configured platform and codec.
     * @param configuration Runtime platform and element settings.
     * @param fileUri Input media path.
     * @param error Receives any GStreamer parse error.
     * @return The caller-owned pipeline element, or null on failure.
     */
                                    GstElement* createScenarioGstPipeline(const L4Configuration& configuration,
                                                                          const std::string& fileUri,
                                                                          GError** error) const;

private:

    std::unique_ptr<PipelineManagementInterface> platformPipelineManagement;
};

#endif