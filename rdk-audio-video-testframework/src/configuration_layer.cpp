/** @file configuration_layer.cpp
 * @brief Parses the suite's required runtime settings from INI files.
 */
#include "l4_configuration.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <stdexcept>

namespace {
/** Removes leading and trailing whitespace without changing interior text. */
std::string trim(std::string value)
{
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char c) {
        return !std::isspace(c);
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), value.end());
    return value;
}

/** Reads a complete configuration file or throws with its path. */
std::string readText(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open configuration: " + path);
    }
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

/**
 * Parses supported INI key/value lines; sections and comment lines are ignored.
 * Repeated keys use the last value encountered.
 */
std::map<std::string, std::string> parseIni(const std::string& text)
{
    std::map<std::string, std::string> values;
    std::size_t lineStart = 0;
    while (lineStart < text.size()) {
        const std::size_t lineEnd = text.find('\n', lineStart);
        std::string line = text.substr(lineStart, lineEnd == std::string::npos
            ? std::string::npos : lineEnd - lineStart);
        lineStart = lineEnd == std::string::npos ? text.size() : lineEnd + 1;
        line = trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';' || line.front() == '[') {
            continue;
        }
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error("Invalid INI line: " + line);
        }
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (key.empty()) {
            throw std::runtime_error("Invalid INI key/value: " + line);
        }
        // Empty values represent optional GStreamer filters and are valid.
        values[key] = value;
    }
    return values;
}

/** Returns a required key or reports which configuration entry is missing. */
const std::string& valueFor(const std::map<std::string, std::string>& values,
                            const std::string& key)
{
    const auto value = values.find(key);
    if (value == values.end()) {
        throw std::runtime_error("Missing configuration key: " + key);
    }
    return value->second;
}

}

/** Loads required INI keys and converts numeric settings into runtime types. */
L4Configuration ConfigurationLayer::load(const std::string& path)
{
    const std::string extension = path.substr(path.find_last_of('.') + 1);
    if (extension != "ini") {
        throw std::runtime_error("Only INI configuration is supported");
    }

    const std::map<std::string, std::string> values = parseIni(readText(path));
    L4Configuration configuration;
    configuration.platform = valueFor(values, "platform");
    configuration.testUrl = valueFor(values, "test_url");
    configuration.sourceElement = valueFor(values, "source_element");
    configuration.decoderElement = valueFor(values, "decoder_element");
    configuration.scenarioSourceElement = valueFor(values, "scenario_source_element");
    configuration.mp4DemuxElement = valueFor(values, "mp4_demux_element");
    configuration.tsDemuxElement = valueFor(values, "ts_demux_element");
    configuration.mp4DemuxLink = valueFor(values, "mp4_demux_link");
    configuration.tsDemuxLink = valueFor(values, "ts_demux_link");
    configuration.videoConvertElement = valueFor(values, "video_convert_element");
    configuration.audioSink = valueFor(values, "audio_sink");
    configuration.videoSink = valueFor(values, "video_sink");
    configuration.pauseVideo = valueFor(values, "pause_video");
    configuration.forwardSeekVideo = valueFor(values, "forward_seek_video");
    configuration.timestampGapVideo = valueFor(values, "timestamp_gap_video");
    configuration.timestampNegativeJumpVideo = valueFor(values, "timestamp_negative_jump_video");
    configuration.audioFilter = valueFor(values, "audio_filter");
    configuration.videoFilter = valueFor(values, "video_filter");
    configuration.volume = std::stod(valueFor(values, "volume"));
    configuration.mute = valueFor(values, "mute") == "true";
    configuration.eosTimeoutSeconds = static_cast<unsigned int>(std::stoul(valueFor(values, "eos_timeout_seconds")));
    configuration.allowedFrameDrops = static_cast<unsigned int>(std::stoul(valueFor(values, "allowed_frame_drops")));
    configuration.avSyncToleranceMs = static_cast<unsigned int>(std::stoul(valueFor(values, "av_sync_tolerance_ms")));
    return configuration;
}