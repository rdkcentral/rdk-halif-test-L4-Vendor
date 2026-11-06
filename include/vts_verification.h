/** @file vts_verification.h
 * @brief Declares audio and video stream detection for media files.
 */
#ifndef VTS_VERIFICATION_H
#define VTS_VERIFICATION_H

#include "vts_configuration.h"
#include <gst/gst.h>
#include <string>
#include <vector>

/** Audio and video stream presence detected in a media file. */
struct DetectedVideoInfo {
    /** True when the file contains at least one audio stream. */
    bool hasAudio = false;
    /** True when the file contains at least one video stream. */
    bool hasVideo = false;
};

/** Detects which audio and video streams a media file contains. */
class MediaStreamDetector {
public:
    /**
     * Uses libavformat to check the streams in a media file.
     * @param mediaPath Path to the input media.
     * @return Audio and video presence for the file.
     * @throws std::runtime_error If the file cannot be read or has no audio or video.
     */
    static DetectedVideoInfo detect(const std::string& mediaPath);
};





#endif
