/** @file vts_test_config.h
 * @brief Parses the AV test configuration and the stream configuration provisioned on the device.
 *
 * Each test runs a GstValidate scenario file. The scenario comes from the suite (scenario key) or from the test
 * (scenario key, which overrides the suite). The platform configuration (vts_configuration.h) is a separate file.
 */
#ifndef VTS_TEST_CONFIG_H
#define VTS_TEST_CONFIG_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

/** One stream of the stream configuration (AVStreams). */
struct L4StreamInfo {
    std::string id;
    /** Relative path of the stream on the stream server (AVStreams.<id>.input). May contain folders. */
    std::string input;
    /** Absolute path of the stream file on the device: AVModule.input_streams_path + file name of input. */
    std::string path;
    uint32_t durationS;
    uint32_t audioFrames;
    uint32_t videoFrames;
    uint32_t audioErrorFrames;
    uint32_t videoErrorFrames;
};

/** One test of the test configuration. Suite and name select the UT Core suite and test. */
struct L4TestEntry {
    /** UT Core suite, for example "NormalPlayback". */
    std::string suite;
    /** UT Core test name inside the suite, for example "Test1". */
    std::string name;
    /** Absolute path of the GstValidate scenario file that this test runs. */
    std::string scenario;
    /** Pause between two streams of this test, in milliseconds. */
    uint32_t delayBetweenPlayMs;
    /** Stream ids (keys of L4TestConfig::streams) played in order. */
    std::vector<std::string> streams;
};

/** The parsed test configuration. */
struct L4TestConfig {
    /** AVModule.userChoice: ask the operator for a Y/N verdict after each stream. */
    bool userChoice;
    /** Absolute directory that holds the stream files on the DUT (AVModule.input_streams_path, resolved). */
    std::string inputStreamsPath;
    /** Stream id -> stream. */
    std::map<std::string, L4StreamInfo> streams;
    /** Tests in file order. */
    std::vector<L4TestEntry> tests;
};

class TestConfigLayer {
public:
    /**
     * Parses the test configuration file and every include_<n> stream configuration it names.
     *
     * Relative paths are resolved against the install base, which is the parent of the directory that holds
     * the test configuration file (device layout: <base>/av_test_configurations/vts_l4_av_test_config.yaml).
     * Scenario files are resolved against AVModule.scenario_dir (default "scenarios").
     *
     * @param path Path to the test configuration YAML.
     * @return The parsed configuration.
     * @throws std::runtime_error on a missing or malformed key, an unknown stream id, a test without a
     *         scenario, or a scenario name that is not a plain file name.
     */
    static L4TestConfig load(const std::string& path);

    /** Returns the test with the given suite and name, or nullptr when the configuration has no such test. */
    static const L4TestEntry* find(const L4TestConfig& config, const std::string& suite, const std::string& name);
};

#endif
