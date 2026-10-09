/** @file verification_layer.h
 * @brief Declares codec metadata and media/sink verification operations.
 */
#ifndef VERIFICATION_LAYER_H
#define VERIFICATION_LAYER_H

#include "l4_configuration.h"
#include <gst/gst.h>
#include <string>
#include <vector>

/** Codec details used by platform pipeline builders. */
struct DetectedVideoInfo {
    /** GStreamer encoded-media caps name, for example `video/x-h264`. */
    std::string capsName;
    /** Parser factory for the codec, or empty when no parser is required. */
    std::string parserElement;
};

/** Detects the input codec and checks sink factory capabilities. */
class VideoCodecDetector {
public:
    /**
     * Uses libavformat to identify the best video stream in a media file.
     * @param videoPath Path to the input media.
     * @return GStreamer caps and parser information for its codec.
     * @throws std::runtime_error If the file or video stream is unsupported.
     */
    static DetectedVideoInfo detect(const std::string& videoPath);

    /**
     * Checks static sink-pad templates for caps intersecting the encoded codec caps.
     * @param sinkName GStreamer element factory name.
     * @param videoInfo Codec metadata returned by detect().
     * @throws std::runtime_error If the factory is missing, not a sink, or incompatible.
     */
    static void verifySinkSupportsCodec(const std::string& sinkName,
                                        const DetectedVideoInfo& videoInfo);
};





#endif
