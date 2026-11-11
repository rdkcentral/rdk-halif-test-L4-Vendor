/** @file vts_configuration.cpp
 * @brief Parses the suite's required runtime settings from YAML files.
 */
#include "vts_configuration.h"



#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <libfyaml.h>

namespace {
/**
 * Collects scalar entries from nested mappings under dotted keys such as "elements.audio_sink".
 * Repeated keys use the last value encountered.
 */
void collectEntries(struct fy_node* mapping, const std::string& prefix,
                    std::map<std::string, std::string>& values, const std::string& path)
{
    void* iterator = nullptr;
    while (struct fy_node_pair* pair = fy_node_mapping_iterate(mapping, &iterator)) {
        struct fy_node* key = fy_node_pair_key(pair);
        struct fy_node* value = fy_node_pair_value(pair);
        if (!key || !value || !fy_node_is_scalar(key)) {
            throw std::runtime_error("Configuration keys must be scalars: " + path);
        }

        size_t keyLength = 0;
        const char* keyText = fy_node_get_scalar(key, &keyLength);
        if (!keyText) {
            throw std::runtime_error("Cannot read configuration key in: " + path);
        }
        const std::string fullKey = prefix + std::string(keyText, keyLength);

        if (fy_node_is_mapping(value)) {
            collectEntries(value, fullKey + ".", values, path);
            continue;
        }
        if (!fy_node_is_scalar(value)) {
            throw std::runtime_error("Configuration values must be scalars or mappings: " + path);
        }

        size_t valueLength = 0;
        const char* valueText = fy_node_get_scalar(value, &valueLength);
        if (!valueText) {
            throw std::runtime_error("Cannot read configuration entry in: " + path);
        }
        values[fullKey] = std::string(valueText, valueLength);
    }
}

/**
 * Parses a YAML file whose top-level mapping holds scalars and nested mappings.
 */
std::map<std::string, std::string> parseYaml(const std::string& path)
{
    std::unique_ptr<struct fy_document, decltype(&fy_document_destroy)> document(
        fy_document_build_from_file(nullptr, path.c_str()), &fy_document_destroy);
    if (!document) {
        throw std::runtime_error("Cannot parse configuration: " + path);
    }

    struct fy_node* root = fy_document_root(document.get());
    if (!root || !fy_node_is_mapping(root)) {
        throw std::runtime_error("Configuration root must be a YAML mapping: " + path);
    }

    std::map<std::string, std::string> values;
    collectEntries(root, "", values, path);
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

/** Loads required YAML keys and converts numeric settings into runtime types. */
L4Configuration ConfigurationLayer::load(const std::string& path)
{
    const std::string extension = path.substr(path.find_last_of('.') + 1);
    if (extension != "yaml" && extension != "yml") {
        throw std::runtime_error("Only YAML configuration is supported");
    }

    const std::map<std::string, std::string> values = parseYaml(path);
    L4Configuration configuration;
    configuration.platform = valueFor(values, "platform");
    configuration.testUrl = valueFor(values, "media.test_url");
    configuration.sourceElement = valueFor(values, "elements.source_element");
    configuration.decoderElement = valueFor(values, "elements.decoder_element");
    configuration.scenarioSourceElement = valueFor(values, "elements.scenario_source_element");
    configuration.mp4DemuxElement = valueFor(values, "elements.mp4_demux_element");
    configuration.tsDemuxElement = valueFor(values, "elements.ts_demux_element");
    configuration.mp4DemuxLink = valueFor(values, "elements.mp4_demux_link");
    configuration.tsDemuxLink = valueFor(values, "elements.ts_demux_link");
    configuration.videoConvertElement = valueFor(values, "elements.video_convert_element");
    configuration.audioSink = valueFor(values, "elements.audio_sink");
    configuration.videoSink = valueFor(values, "elements.video_sink");
    configuration.pauseVideo = valueFor(values, "media.pause_video");
    configuration.forwardSeekVideo = valueFor(values, "media.forward_seek_video");
    configuration.audioFilter = valueFor(values, "elements.audio_filter");
    configuration.videoFilter = valueFor(values, "elements.video_filter");
    configuration.volume = std::stod(valueFor(values, "playback.volume"));
    configuration.mute = valueFor(values, "playback.mute") == "true";
    configuration.allowedFrameDrops = static_cast<unsigned int>(std::stoul(valueFor(values, "verification.allowed_frame_drops")));
    configuration.avSyncToleranceMs = static_cast<unsigned int>(std::stoul(valueFor(values, "verification.av_sync_tolerance_ms")));
    return configuration;
}