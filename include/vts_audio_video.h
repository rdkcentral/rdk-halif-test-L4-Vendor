/** @file vts_audio_video.h
 * @brief Declares the common L4 audio/video scenario test interface.
 */
#ifndef VTS_AUDIO_VIDEO_H
#define VTS_AUDIO_VIDEO_H

#include "vts_pipeline_management.h"
#include "vts_verification.h"

#include <string>

/** Runs the shared GstValidate scenarios used by the L4 test suite. */
class RDKAudioVideo {
public:
    /** Constructs the runner and its platform pipeline-management layer. */
    RDKAudioVideo();

    /** Prints the UT smoke-test message. */
    void helloWorld();

    /** Runs the configured pause/resume playback scenario. */
    bool runPauseValidationScenario();

    /** Runs the configured forward-seek playback scenario. */
    bool runForwardSeekValidationScenario();

    /**
     * Builds and executes one named GstValidate scenario.
     * @param configuration Runtime platform and pipeline configuration.
     * @param scenarioName Supported scenario identifier.
     * @param fileUri Media input used by the scenario.
     * @return True when pipeline execution passes.
     */
    bool runScenario(const L4Configuration& configuration,
                          const std::string& scenarioName,
                          const std::string& fileUri = "");
//    bool testAudioFile(const std::string& filePath);

private:
    PipelineManagementLayer pipelineManagementLayer;

};



#endif // VTS_AUDIO_VIDEO_H
