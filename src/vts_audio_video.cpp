/** @file vts_audio_video.cpp
 * @brief UT Core suites of the AV L4 tests and the runner that executes one configured test.
 *
 * Suites are the use cases of docs/L4_Test_Cases.md. Each suite has Test1 to Test32. A test runs the entry
 * of the AV test configuration with the same suite and name; a name the configuration does not define fails
 * with a log message, and the host never selects it.
 */
#include "vts_audio_video.h"

#include <ut.h>
#include <ut_log.h>

#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <thread>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

namespace fs = std::filesystem;

bool runInChildProcess(const std::function<bool()>& scenario)
{
    std::fflush(nullptr);
    const pid_t pid = fork();
    if (pid < 0) {
        UT_LOG_ERROR("Could not fork scenario process.");
        return false;
    }
    if (pid == 0) {
        const bool passed = scenario();
        std::fflush(nullptr);
        _exit(passed ? 0 : 1);
    }

    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited < 0) {
        UT_LOG_ERROR("Could not wait for scenario process.");
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/** Question printed after each stream when userChoice is set. The Python host looks for this exact text. */
constexpr const char* kQualityQuestion = "Is the stream playback good (Y/N)?";

/** Prints the question and reads the operator answer from stdin. Only the first non-blank character counts. */
bool askOperator(const char* question)
{
    std::printf("\n%s\n", question);
    std::fflush(stdout);

    char line[32] = {0};
    while (std::fgets(line, sizeof(line), stdin)) {
        for (const char* character = line; *character != '\0'; ++character) {
            if (!std::isspace(static_cast<unsigned char>(*character))) {
                return *character == 'Y' || *character == 'y';
            }
        }
    }
    UT_LOG_ERROR("No operator answer on stdin");
    return false;
}

/** Platform configuration: L4_CONFIG when set, otherwise the path compiled into the binary. */
std::string platformConfigPath()
{
    const char* override = std::getenv("L4_CONFIG");
    return override ? override : L4_DEFAULT_CONFIG_PATH;
}

/** Test configuration: L4_TEST_CONFIG when set, otherwise the default location in the install base. */
std::string testConfigPath()
{
    const char* override = std::getenv("L4_TEST_CONFIG");
    return override ? override : "av_test_configurations/vts_l4_av_test_config.yaml";
}

} // namespace

bool runConfiguredTest(const std::string& suite, const std::string& name)
{
    L4Configuration configuration;
    try {
        configuration = ConfigurationLayer::load(platformConfigPath());
    } catch (const std::exception& error) {
        UT_LOG_ERROR("%s.%s: platform configuration failed: %s", suite.c_str(), name.c_str(), error.what());
        return false;
    }

    L4TestConfig config;
    try {
        config = TestConfigLayer::load(testConfigPath());
    } catch (const std::exception& error) {
        UT_LOG_ERROR("%s.%s: test configuration failed (%s): %s", suite.c_str(), name.c_str(),
                     testConfigPath().c_str(), error.what());
        return false;
    }

    const L4TestEntry* test = TestConfigLayer::find(config, suite, name);
    if (!test) {
        UT_LOG_ERROR("%s.%s is not defined in %s", suite.c_str(), name.c_str(), testConfigPath().c_str());
        return false;
    }

    UT_LOG("%s.%s: %zu stream(s), scenario %s", suite.c_str(), name.c_str(), test->streams.size(),
           test->scenario.c_str());

    if (!fs::is_regular_file(test->scenario)) {
        UT_LOG_ERROR("%s.%s: scenario file not found: %s", suite.c_str(), name.c_str(), test->scenario.c_str());
        return false;
    }

    bool passed = true;
    for (const std::string& streamId : test->streams) {
        const L4StreamInfo& stream = config.streams.at(streamId);
        if (!fs::is_regular_file(stream.path)) {
            UT_LOG_ERROR("%s.%s: stream file not found: %s", suite.c_str(), name.c_str(), stream.path.c_str());
            passed = false;
            continue;
        }

        UT_LOG("%s.%s: stream %s (%s), expected %u s, frames audio=%u video=%u, error frames audio=%u video=%u",
               suite.c_str(), name.c_str(), streamId.c_str(), stream.path.c_str(), stream.durationS,
               stream.audioFrames, stream.videoFrames, stream.audioErrorFrames, stream.videoErrorFrames);

        // Time limit of the scenario: the stream length plus a fixed margin for setup and teardown.
        constexpr uint32_t kScenarioMarginS = 120;
        if (!runInChildProcess([&] {
                return runScenarioFile(configuration, test->scenario, stream.path,
                                       stream.durationS + kScenarioMarginS);
            })) {
            UT_LOG_ERROR("%s.%s: playback of %s failed", suite.c_str(), name.c_str(), streamId.c_str());
            passed = false;
        } else if (config.userChoice && !askOperator(kQualityQuestion)) {
            UT_LOG_ERROR("%s.%s: operator reported a problem with %s", suite.c_str(), name.c_str(), streamId.c_str());
            passed = false;
        }

        if (test->delayBetweenPlayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(test->delayBetweenPlayMs));
        }
    }

    UT_LOG("%s.%s: %s", suite.c_str(), name.c_str(), passed ? "PASS" : "FAIL");
    return passed;
}

/* ---- UT Core suites: one per use case of docs/L4_Test_Cases.md. Test names are looked up in the configuration. ---- */

#define AVL4_TEST(SUITE, NAME) \
    UT_ADD_TEST(SUITE, NAME) { UT_ASSERT_TRUE(runConfiguredTest(#SUITE, #NAME)); }

void registerAvTestGroups()
{
    UTCore::UT_add_suite_withGroupID("NormalPlayback", UT_TESTS_L4);
    UTCore::UT_add_suite_withGroupID("PauseResume", UT_TESTS_L4);
    UTCore::UT_add_suite_withGroupID("TrickModes", UT_TESTS_L4);
    UTCore::UT_add_suite_withGroupID("Seek", UT_TESTS_L4);
    UTCore::UT_add_suite_withGroupID("AdaptiveBitrate", UT_TESTS_L4);
    UTCore::UT_add_suite_withGroupID("MetadataPropagation", UT_TESTS_L4);
}

    /* Section 1.0: normal playback (AV, audio only, video only). */
class NormalPlayback : public UTCore {};
AVL4_TEST(NormalPlayback, Test1)
AVL4_TEST(NormalPlayback, Test2)
AVL4_TEST(NormalPlayback, Test3)
AVL4_TEST(NormalPlayback, Test4)
AVL4_TEST(NormalPlayback, Test5)
AVL4_TEST(NormalPlayback, Test6)
AVL4_TEST(NormalPlayback, Test7)
AVL4_TEST(NormalPlayback, Test8)
AVL4_TEST(NormalPlayback, Test9)
AVL4_TEST(NormalPlayback, Test10)
AVL4_TEST(NormalPlayback, Test11)
AVL4_TEST(NormalPlayback, Test12)
AVL4_TEST(NormalPlayback, Test13)
AVL4_TEST(NormalPlayback, Test14)
AVL4_TEST(NormalPlayback, Test15)
AVL4_TEST(NormalPlayback, Test16)
AVL4_TEST(NormalPlayback, Test17)
AVL4_TEST(NormalPlayback, Test18)
AVL4_TEST(NormalPlayback, Test19)
AVL4_TEST(NormalPlayback, Test20)
AVL4_TEST(NormalPlayback, Test21)
AVL4_TEST(NormalPlayback, Test22)
AVL4_TEST(NormalPlayback, Test23)
AVL4_TEST(NormalPlayback, Test24)
AVL4_TEST(NormalPlayback, Test25)
AVL4_TEST(NormalPlayback, Test26)
AVL4_TEST(NormalPlayback, Test27)
AVL4_TEST(NormalPlayback, Test28)
AVL4_TEST(NormalPlayback, Test29)
AVL4_TEST(NormalPlayback, Test30)
AVL4_TEST(NormalPlayback, Test31)
AVL4_TEST(NormalPlayback, Test32)

/* Section 2.0: pause and resume. */
class PauseResume : public UTCore {};
AVL4_TEST(PauseResume, Test1)
AVL4_TEST(PauseResume, Test2)
AVL4_TEST(PauseResume, Test3)
AVL4_TEST(PauseResume, Test4)
AVL4_TEST(PauseResume, Test5)
AVL4_TEST(PauseResume, Test6)
AVL4_TEST(PauseResume, Test7)
AVL4_TEST(PauseResume, Test8)
AVL4_TEST(PauseResume, Test9)
AVL4_TEST(PauseResume, Test10)
AVL4_TEST(PauseResume, Test11)
AVL4_TEST(PauseResume, Test12)
AVL4_TEST(PauseResume, Test13)
AVL4_TEST(PauseResume, Test14)
AVL4_TEST(PauseResume, Test15)
AVL4_TEST(PauseResume, Test16)
AVL4_TEST(PauseResume, Test17)
AVL4_TEST(PauseResume, Test18)
AVL4_TEST(PauseResume, Test19)
AVL4_TEST(PauseResume, Test20)
AVL4_TEST(PauseResume, Test21)
AVL4_TEST(PauseResume, Test22)
AVL4_TEST(PauseResume, Test23)
AVL4_TEST(PauseResume, Test24)
AVL4_TEST(PauseResume, Test25)
AVL4_TEST(PauseResume, Test26)
AVL4_TEST(PauseResume, Test27)
AVL4_TEST(PauseResume, Test28)
AVL4_TEST(PauseResume, Test29)
AVL4_TEST(PauseResume, Test30)
AVL4_TEST(PauseResume, Test31)
AVL4_TEST(PauseResume, Test32)

/* Section 3.0: fast-forward and rewind (trick modes). */
class TrickModes : public UTCore {};
AVL4_TEST(TrickModes, Test1)
AVL4_TEST(TrickModes, Test2)
AVL4_TEST(TrickModes, Test3)
AVL4_TEST(TrickModes, Test4)
AVL4_TEST(TrickModes, Test5)
AVL4_TEST(TrickModes, Test6)
AVL4_TEST(TrickModes, Test7)
AVL4_TEST(TrickModes, Test8)
AVL4_TEST(TrickModes, Test9)
AVL4_TEST(TrickModes, Test10)
AVL4_TEST(TrickModes, Test11)
AVL4_TEST(TrickModes, Test12)
AVL4_TEST(TrickModes, Test13)
AVL4_TEST(TrickModes, Test14)
AVL4_TEST(TrickModes, Test15)
AVL4_TEST(TrickModes, Test16)
AVL4_TEST(TrickModes, Test17)
AVL4_TEST(TrickModes, Test18)
AVL4_TEST(TrickModes, Test19)
AVL4_TEST(TrickModes, Test20)
AVL4_TEST(TrickModes, Test21)
AVL4_TEST(TrickModes, Test22)
AVL4_TEST(TrickModes, Test23)
AVL4_TEST(TrickModes, Test24)
AVL4_TEST(TrickModes, Test25)
AVL4_TEST(TrickModes, Test26)
AVL4_TEST(TrickModes, Test27)
AVL4_TEST(TrickModes, Test28)
AVL4_TEST(TrickModes, Test29)
AVL4_TEST(TrickModes, Test30)
AVL4_TEST(TrickModes, Test31)
AVL4_TEST(TrickModes, Test32)

/* Sections 4.0 and 5.0: forward and backward seek. */
class Seek : public UTCore {};
AVL4_TEST(Seek, Test1)
AVL4_TEST(Seek, Test2)
AVL4_TEST(Seek, Test3)
AVL4_TEST(Seek, Test4)
AVL4_TEST(Seek, Test5)
AVL4_TEST(Seek, Test6)
AVL4_TEST(Seek, Test7)
AVL4_TEST(Seek, Test8)
AVL4_TEST(Seek, Test9)
AVL4_TEST(Seek, Test10)
AVL4_TEST(Seek, Test11)
AVL4_TEST(Seek, Test12)
AVL4_TEST(Seek, Test13)
AVL4_TEST(Seek, Test14)
AVL4_TEST(Seek, Test15)
AVL4_TEST(Seek, Test16)
AVL4_TEST(Seek, Test17)
AVL4_TEST(Seek, Test18)
AVL4_TEST(Seek, Test19)
AVL4_TEST(Seek, Test20)
AVL4_TEST(Seek, Test21)
AVL4_TEST(Seek, Test22)
AVL4_TEST(Seek, Test23)
AVL4_TEST(Seek, Test24)
AVL4_TEST(Seek, Test25)
AVL4_TEST(Seek, Test26)
AVL4_TEST(Seek, Test27)
AVL4_TEST(Seek, Test28)
AVL4_TEST(Seek, Test29)
AVL4_TEST(Seek, Test30)
AVL4_TEST(Seek, Test31)
AVL4_TEST(Seek, Test32)

/* Section 6.0: adaptive bitrate resolution switching. */
class AdaptiveBitrate : public UTCore {};
AVL4_TEST(AdaptiveBitrate, Test1)
AVL4_TEST(AdaptiveBitrate, Test2)
AVL4_TEST(AdaptiveBitrate, Test3)
AVL4_TEST(AdaptiveBitrate, Test4)
AVL4_TEST(AdaptiveBitrate, Test5)
AVL4_TEST(AdaptiveBitrate, Test6)
AVL4_TEST(AdaptiveBitrate, Test7)
AVL4_TEST(AdaptiveBitrate, Test8)
AVL4_TEST(AdaptiveBitrate, Test9)
AVL4_TEST(AdaptiveBitrate, Test10)
AVL4_TEST(AdaptiveBitrate, Test11)
AVL4_TEST(AdaptiveBitrate, Test12)
AVL4_TEST(AdaptiveBitrate, Test13)
AVL4_TEST(AdaptiveBitrate, Test14)
AVL4_TEST(AdaptiveBitrate, Test15)
AVL4_TEST(AdaptiveBitrate, Test16)
AVL4_TEST(AdaptiveBitrate, Test17)
AVL4_TEST(AdaptiveBitrate, Test18)
AVL4_TEST(AdaptiveBitrate, Test19)
AVL4_TEST(AdaptiveBitrate, Test20)
AVL4_TEST(AdaptiveBitrate, Test21)
AVL4_TEST(AdaptiveBitrate, Test22)
AVL4_TEST(AdaptiveBitrate, Test23)
AVL4_TEST(AdaptiveBitrate, Test24)
AVL4_TEST(AdaptiveBitrate, Test25)
AVL4_TEST(AdaptiveBitrate, Test26)
AVL4_TEST(AdaptiveBitrate, Test27)
AVL4_TEST(AdaptiveBitrate, Test28)
AVL4_TEST(AdaptiveBitrate, Test29)
AVL4_TEST(AdaptiveBitrate, Test30)
AVL4_TEST(AdaptiveBitrate, Test31)
AVL4_TEST(AdaptiveBitrate, Test32)

/* Section 7.0: metadata propagation (playback and operator verdict; the caps validator is not implemented). */
class MetadataPropagation : public UTCore {};
AVL4_TEST(MetadataPropagation, Test1)
AVL4_TEST(MetadataPropagation, Test2)
AVL4_TEST(MetadataPropagation, Test3)
AVL4_TEST(MetadataPropagation, Test4)
AVL4_TEST(MetadataPropagation, Test5)
AVL4_TEST(MetadataPropagation, Test6)
AVL4_TEST(MetadataPropagation, Test7)
AVL4_TEST(MetadataPropagation, Test8)
AVL4_TEST(MetadataPropagation, Test9)
AVL4_TEST(MetadataPropagation, Test10)
AVL4_TEST(MetadataPropagation, Test11)
AVL4_TEST(MetadataPropagation, Test12)
AVL4_TEST(MetadataPropagation, Test13)
AVL4_TEST(MetadataPropagation, Test14)
AVL4_TEST(MetadataPropagation, Test15)
AVL4_TEST(MetadataPropagation, Test16)
AVL4_TEST(MetadataPropagation, Test17)
AVL4_TEST(MetadataPropagation, Test18)
AVL4_TEST(MetadataPropagation, Test19)
AVL4_TEST(MetadataPropagation, Test20)
AVL4_TEST(MetadataPropagation, Test21)
AVL4_TEST(MetadataPropagation, Test22)
AVL4_TEST(MetadataPropagation, Test23)
AVL4_TEST(MetadataPropagation, Test24)
AVL4_TEST(MetadataPropagation, Test25)
AVL4_TEST(MetadataPropagation, Test26)
AVL4_TEST(MetadataPropagation, Test27)
AVL4_TEST(MetadataPropagation, Test28)
AVL4_TEST(MetadataPropagation, Test29)
AVL4_TEST(MetadataPropagation, Test30)
AVL4_TEST(MetadataPropagation, Test31)
AVL4_TEST(MetadataPropagation, Test32)

