/** @file vts_audio_video.h
 * @brief UT Core test runner for the AV L4 suites.
 */
#ifndef VTS_AUDIO_VIDEO_H
#define VTS_AUDIO_VIDEO_H

#include <cstdint>
#include <string>

#include "vts_configuration.h"
#include "vts_test_config.h"

void registerAvTestGroups();

/**
 * Runs one test of the AV test configuration, selected by suite and name.
 *
 * Every stream of the test is played with the test's GstValidate scenario. When the configuration sets
 * userChoice, the operator is asked for a Y/N verdict after each stream. UT Core logs the outcome, and the
 * test passes only if every stream passes.
 *
 * @param suite UT Core suite, for example "NormalPlayback".
 * @param name UT Core test name, for example "Test1".
 * @return true when the test passes.
 */
bool runConfiguredTest(const std::string& suite, const std::string& name);

/**
 * Plays one media file with a GstValidate scenario on the platform pipeline and evaluates the run.
 *
 * @param configuration Platform configuration (pipeline elements and sinks).
 * @param scenarioPath Absolute path of the .scenario file, read by GstValidate.
 * @param mediaPath Absolute path of the media file.
 * @param timeoutS Time limit of the run in seconds. The run is failed when it expires.
 * @return true when all six normal-playback checks pass (see NormalPlaybackChecks in vts_verification.h).
 */
bool runScenarioFile(const L4Configuration& configuration,
                     const std::string& scenarioPath,
                     const std::string& mediaPath,
                     uint32_t timeoutS);

#endif // VTS_AUDIO_VIDEO_H
