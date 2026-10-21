/** @file l4-rdk-audio-video.cpp
 * @brief Registers the active scenarios with the UT Core suite.
 */
#include <ut.h>
#include "l4-rdk-audio-video.h"

/** Shares one scenario runner fixture across the registered UT-Core cases. */
class L4RdkAudioVideoTest : public UTCore {
protected:
    RDKAudioVideo audioVideo;
};

/** Smoke-checks that the UT suite and its decoder fixture are loadable. */
UT_ADD_TEST(L4RdkAudioVideoTest, hello_world)
{
    audioVideo.helloWorld();
}

/** Verifies the playback pause/resume scenario on the configured input. */
UT_ADD_TEST(L4RdkAudioVideoTest, video_pause_scenario)
{
    UT_ASSERT_TRUE(audioVideo.runPauseValidationScenario());
}

/** Verifies forward-seek behavior on the configured input. */
UT_ADD_TEST(L4RdkAudioVideoTest, video_forward_seek_scenario)
{
    UT_ASSERT_TRUE(audioVideo.runForwardSeekValidationScenario());
}

/** Checks the configured defective files for timestamp anomalies. */
UT_ADD_TEST(L4RdkAudioVideoTest, video_timestamp_continuity_scenario)
{
    UT_ASSERT_TRUE(audioVideo.runTimestampContinuityValidationScenario());
}


