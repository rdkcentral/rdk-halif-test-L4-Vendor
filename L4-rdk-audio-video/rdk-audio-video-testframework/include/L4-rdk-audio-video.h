/** @file L4-rdk-audio-video.h
 * @brief Declares the common L4 audio/video scenario test interface.
 */
#ifndef L4_RDK_AUDIO_VIDEO_H
#define L4_RDK_AUDIO_VIDEO_H

#include "pipeline_management_layer.h"
#include "verification_layer.h"

#include <string>

/** Runs the shared GstValidate scenarios used by the L4 test suite. */
class RDKAudioDecoderL4 {
public:
    /** Constructs the runner and its platform pipeline-management layer. */
    RDKAudioDecoderL4();

    /** Prints the UT smoke-test message. */
    void helloWorld();

    /** Runs the configured pause/resume playback scenario. */
    bool runPauseValidationScenario();

    /** Runs the configured forward-seek playback scenario. */
    bool runForwardSeekValidationScenario();

    /** Runs timestamp-continuity checks against both configured defective assets. */
    bool runTimestampContinuityValidationScenario();

    /**
     * Builds and executes one named GstValidate scenario.
     * @param configuration Runtime platform and pipeline configuration.
     * @param scenarioName Supported scenario identifier.
     * @param videoPath Media input used by the scenario.
     * @param videoSink Compatibility argument; the configured sink is used by the platform builder.
     * @param decoderElement Compatibility argument; decoder selection is handled by the platform builder.
     * @param expectTimestampAnomaly Whether an anomaly is expected for this input.
     * @param eosTimeoutSeconds Maximum wait before the run is failed.
     * @return True when pipeline execution and timestamp expectations pass.
     */
    bool runVideoScenario(const L4Configuration& configuration,
                          const std::string& scenarioName,
                          const std::string& videoPath = "",
                          const std::string& videoSink = "",
                          const std::string& decoderElement = "",
                          bool expectTimestampAnomaly = false,
                          unsigned int eosTimeoutSeconds = 30);
//    bool testAudioFile(const std::string& filePath);

private:
    PipelineManagementLayer pipelineManagementLayer;

};



#endif // L4_RDK_AUDIO_VIDEO_H
