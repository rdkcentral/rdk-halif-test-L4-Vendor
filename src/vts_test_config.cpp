/** @file vts_test_config.cpp
 * @brief Parses the AV test configuration and the stream configuration with libfyaml.
 */
#include "vts_test_config.h"

#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include <libfyaml.h>

namespace {

namespace fs = std::filesystem;

using DocumentPtr = std::unique_ptr<struct fy_document, decltype(&fy_document_destroy)>;

DocumentPtr openDocument(const std::string& path)
{
    DocumentPtr document(fy_document_build_from_file(nullptr, path.c_str()), &fy_document_destroy);
    if (!document) {
        throw std::runtime_error("Cannot parse YAML: " + path);
    }
    return document;
}

std::string scalarOf(struct fy_node* node, const std::string& where)
{
    if (!node || !fy_node_is_scalar(node)) {
        throw std::runtime_error(where + " must be a scalar value");
    }
    size_t length = 0;
    const char* text = fy_node_get_scalar(node, &length);
    if (!text) {
        throw std::runtime_error("Cannot read value of " + where);
    }
    return std::string(text, length);
}

/** Returns the value node stored under key in a mapping, or nullptr when the key is absent. */
struct fy_node* childOf(struct fy_node* mapping, const std::string& key)
{
    if (!mapping || !fy_node_is_mapping(mapping)) {
        return nullptr;
    }
    void* iterator = nullptr;
    while (struct fy_node_pair* pair = fy_node_mapping_iterate(mapping, &iterator)) {
        struct fy_node* keyNode = fy_node_pair_key(pair);
        if (keyNode && fy_node_is_scalar(keyNode)) {
            size_t length = 0;
            const char* text = fy_node_get_scalar(keyNode, &length);
            if (text && std::string(text, length) == key) {
                return fy_node_pair_value(pair);
            }
        }
    }
    return nullptr;
}

struct fy_node* requiredMapping(struct fy_node* mapping, const std::string& key, const std::string& where)
{
    struct fy_node* value = childOf(mapping, key);
    if (!value) {
        throw std::runtime_error("Missing key " + where + key);
    }
    if (!fy_node_is_mapping(value)) {
        throw std::runtime_error(where + key + " must be a mapping");
    }
    return value;
}

int64_t parseInteger(const std::string& text, const std::string& where)
{
    try {
        size_t used = 0;
        const int64_t value = std::stoll(text, &used);
        if (used != text.size()) {
            throw std::invalid_argument(text);
        }
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error(where + " must be an integer, got '" + text + "'");
    }
}

uint32_t parseUnsigned(const std::string& text, const std::string& where)
{
    const int64_t value = parseInteger(text, where);
    if (value < 0) {
        throw std::runtime_error(where + " must not be negative");
    }
    return static_cast<uint32_t>(value);
}

bool parseBool(const std::string& text, const std::string& where)
{
    if (text == "true") {
        return true;
    }
    if (text == "false") {
        return false;
    }
    throw std::runtime_error(where + " must be true or false, got '" + text + "'");
}

/** A scenario name is a plain file name, so it cannot point outside scenario_dir. */
std::string scenarioPathOf(const fs::path& scenarioDir, const std::string& name, const std::string& where)
{
    if (name.empty() || name.find('/') != std::string::npos || name.find('\\') != std::string::npos ||
        name.find("..") != std::string::npos) {
        throw std::runtime_error(where + " must be a plain scenario file name, got '" + name + "'");
    }
    return (scenarioDir / name).lexically_normal().string();
}

/** Reads AVStreams entries. Stream paths are resolved against the stream input directory. */
void parseStreams(struct fy_node* streams, const fs::path& inputDir, std::map<std::string, L4StreamInfo>& out)
{
    if (!fy_node_is_mapping(streams)) {
        throw std::runtime_error("AVStreams must be a mapping");
    }
    void* iterator = nullptr;
    while (struct fy_node_pair* pair = fy_node_mapping_iterate(streams, &iterator)) {
        const std::string id = scalarOf(fy_node_pair_key(pair), "AVStreams key");
        struct fy_node* entry = fy_node_pair_value(pair);
        if (!entry || !fy_node_is_mapping(entry)) {
            throw std::runtime_error("AVStreams." + id + " must be a mapping");
        }
        const std::string where = "AVStreams." + id + ".";

        L4StreamInfo stream;
        stream.id = id;
        const std::string input = scalarOf(childOf(entry, "input"), where + "input");
        if (input.empty() || input[0] == '/' || input.find("..") != std::string::npos) {
            throw std::runtime_error(where + "input must be a relative path without '..', got '" + input + "'");
        }
        stream.input = input;
        stream.path = (inputDir / fs::path(input).filename()).lexically_normal().string();
        stream.durationS = parseUnsigned(scalarOf(childOf(entry, "duration"), where + "duration"), where + "duration");

        auto optionalCount = [&](const char* key) -> uint32_t {
            struct fy_node* value = childOf(entry, key);
            return value ? parseUnsigned(scalarOf(value, where + key), where + key) : 0;
        };
        stream.audioFrames = optionalCount("audio_frames");
        stream.videoFrames = optionalCount("video_frames");
        stream.audioErrorFrames = optionalCount("audio_error_frames");
        stream.videoErrorFrames = optionalCount("video_error_frames");

        out[id] = stream;
    }
}

} // namespace

L4TestConfig TestConfigLayer::load(const std::string& path)
{
    const std::string extension = path.substr(path.find_last_of('.') + 1);
    if (extension != "yaml" && extension != "yml") {
        throw std::runtime_error("Only YAML test configuration is supported: " + path);
    }

    const fs::path base = fs::absolute(path).lexically_normal().parent_path().parent_path();
    DocumentPtr document = openDocument(path);
    struct fy_node* root = fy_document_root(document.get());
    if (!root || !fy_node_is_mapping(root)) {
        throw std::runtime_error("Test configuration root must be a mapping: " + path);
    }

    L4TestConfig config;

    struct fy_node* module = requiredMapping(root, "AVModule", "");
    const std::string inputStreams = scalarOf(childOf(module, "input_streams_path"), "AVModule.input_streams_path");
    const fs::path inputDir = (base / inputStreams).lexically_normal();
    config.inputStreamsPath = inputDir.string();

    std::string scenarioDirName = "scenarios";
    if (struct fy_node* dirNode = childOf(module, "scenario_dir")) {
        scenarioDirName = scalarOf(dirNode, "AVModule.scenario_dir");
    }
    const fs::path scenarioDir = (base / scenarioDirName).lexically_normal();

    config.userChoice = true;
    if (struct fy_node* userChoice = childOf(module, "userChoice")) {
        config.userChoice = parseBool(scalarOf(userChoice, "AVModule.userChoice"), "AVModule.userChoice");
    }

    // Stream configurations named by include_<n> keys, then the AVStreams of this file.
    void* iterator = nullptr;
    while (struct fy_node_pair* pair = fy_node_mapping_iterate(root, &iterator)) {
        const std::string key = scalarOf(fy_node_pair_key(pair), "top-level key");
        if (key.rfind("include_", 0) != 0) {
            continue;
        }
        const std::string includePath = scalarOf(fy_node_pair_value(pair), key);
        DocumentPtr include = openDocument((base / includePath).lexically_normal().string());
        struct fy_node* includeRoot = fy_document_root(include.get());
        if (struct fy_node* streams = childOf(includeRoot, "AVStreams")) {
            parseStreams(streams, inputDir, config.streams);
        }
    }
    if (struct fy_node* streams = childOf(root, "AVStreams")) {
        parseStreams(streams, inputDir, config.streams);
    }

    // AVModule.L4.<suite>: an optional scenario for the suite, then one mapping per test.
    struct fy_node* l4 = requiredMapping(module, "L4", "AVModule.");
    void* suiteIt = nullptr;
    while (struct fy_node_pair* suitePair = fy_node_mapping_iterate(l4, &suiteIt)) {
        const std::string suite = scalarOf(fy_node_pair_key(suitePair), "AVModule.L4 key");
        struct fy_node* suiteNode = fy_node_pair_value(suitePair);
        if (!suiteNode || !fy_node_is_mapping(suiteNode)) {
            throw std::runtime_error("AVModule.L4." + suite + " must be a mapping");
        }

        std::string suiteScenario;
        if (struct fy_node* scenarioNode = childOf(suiteNode, "scenario")) {
            suiteScenario = scalarOf(scenarioNode, "AVModule.L4." + suite + ".scenario");
        }

        void* testIt = nullptr;
        while (struct fy_node_pair* testPair = fy_node_mapping_iterate(suiteNode, &testIt)) {
            const std::string name = scalarOf(fy_node_pair_key(testPair), "test name in " + suite);
            if (name == "scenario") {
                continue;  // suite-level scenario, read above
            }
            struct fy_node* testNode = fy_node_pair_value(testPair);
            const std::string where = "AVModule.L4." + suite + "." + name;
            if (!testNode || !fy_node_is_mapping(testNode)) {
                throw std::runtime_error(where + " must be a mapping");
            }

            L4TestEntry entry;
            entry.suite = suite;
            entry.name = name;

            // The test's own scenario overrides the suite's scenario.
            std::string scenarioName = suiteScenario;
            if (struct fy_node* scenarioNode = childOf(testNode, "scenario")) {
                scenarioName = scalarOf(scenarioNode, where + ".scenario");
            }
            if (scenarioName.empty()) {
                throw std::runtime_error(where + " has no scenario: set scenario on the test or on the suite");
            }
            entry.scenario = scenarioPathOf(scenarioDir, scenarioName, where + ".scenario");

            entry.delayBetweenPlayMs = 0;
            if (struct fy_node* controls = childOf(testNode, "controls")) {
                if (!fy_node_is_mapping(controls)) {
                    throw std::runtime_error(where + ".controls must be a mapping");
                }
                if (struct fy_node* delay = childOf(controls, "delay_between_play")) {
                    entry.delayBetweenPlayMs = parseUnsigned(scalarOf(delay, where + ".delay_between_play"),
                                                             where + ".delay_between_play");
                }
            }

            struct fy_node* streams = childOf(testNode, "streams");
            if (!streams || !fy_node_is_sequence(streams)) {
                throw std::runtime_error(where + ".streams must be a sequence of stream ids");
            }
            void* streamIt = nullptr;
            while (struct fy_node* item = fy_node_sequence_iterate(streams, &streamIt)) {
                const std::string streamId = scalarOf(item, where + ".streams[]");
                if (config.streams.find(streamId) == config.streams.end()) {
                    throw std::runtime_error(where + ".streams: '" + streamId + "' is not in AVStreams");
                }
                entry.streams.push_back(streamId);
            }

            config.tests.push_back(entry);
        }
    }

    return config;
}

const L4TestEntry* TestConfigLayer::find(const L4TestConfig& config, const std::string& suite, const std::string& name)
{
    for (const L4TestEntry& entry : config.tests) {
        if (entry.suite == suite && entry.name == name) {
            return &entry;
        }
    }
    return nullptr;
}
