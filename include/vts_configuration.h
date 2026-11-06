/** @file vts_configuration.h
 * @brief Defines the runtime settings loaded from the platform YAML file.
 */
#ifndef VTS_CONFIGURATION_H
#define VTS_CONFIGURATION_H

#include <string>

/** Values used to construct platform pipelines and run media scenarios. */
struct L4Configuration {
    // Platform identity and generic playback elements.
    std::string platform;
    std::string testUrl;
    std::string sourceElement;
    std::string decoderElement;

    // Scenario pipeline elements and dynamic-pad link hints.
    std::string scenarioSourceElement;
    std::string mp4DemuxElement;
    std::string tsDemuxElement;
    std::string mp4DemuxLink;
    std::string tsDemuxLink;
    std::string h264ParserElement;
    std::string videoConvertElement;

    // Playback sinks, filters, and scenario media paths.
    std::string audioSink;
    std::string videoSink;
    std::string pauseVideo;
    std::string forwardSeekVideo;
    std::string audioFilter;
    std::string videoFilter;

    // Runtime limits use seconds, frames, and milliseconds respectively.
    double volume;
    bool mute;
    unsigned int allowedFrameDrops;
    unsigned int avSyncToleranceMs;
};

class ConfigurationLayer {
public:
    /**
     * Loads and validates required key/value entries from a YAML file.
     * @param path Path to the `.yaml` or `.yml` file.
     * @return A populated configuration structure.
     * @throws std::runtime_error If the file, format, or a required key is invalid.
     */
    static L4Configuration load(const std::string& path);
};

#endif