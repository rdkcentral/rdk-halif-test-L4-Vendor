/** @file vts_verification.h
 * @brief Declares audio and video stream detection for media files.
 */
#ifndef VTS_VERIFICATION_H
#define VTS_VERIFICATION_H

#include "vts_configuration.h"
#include <gst/gst.h>
#include <cstdint>
#include <string>
#include <vector>

/** Audio and video stream presence, and the first video stream's codec and size, of a media file. */
struct DetectedVideoInfo {
    /** True when the file contains at least one audio stream. */
    bool hasAudio = false;
    /** True when the file contains at least one video stream. */
    bool hasVideo = false;
    /** GStreamer encoded caps name of the first video stream; empty when the codec is unsupported. */
    std::string capsName;
    /** Coded width of the first video stream in pixels. */
    int width = 0;
    /** Coded height of the first video stream in pixels. */
    int height = 0;
    /** Frame rate numerator of the first video stream. */
    int frameRateNumerator = 0;
    /** Frame rate denominator of the first video stream. */
    int frameRateDenominator = 1;
};

/** Media details passed to platform pipeline builders and to the normal-playback checks. */
using MediaInfo = DetectedVideoInfo;

/** Detects which audio and video streams a media file contains. */
class MediaStreamDetector {
public:
    /**
     * Uses libavformat to check the streams in a media file.
     * @param mediaPath Path to the input media.
     * @return Audio and video presence, and the first video stream's details, for the file.
     * @throws std::runtime_error If FFmpeg cannot open or read the file.
     */
    static DetectedVideoInfo detect(const std::string& mediaPath);
};

/**
 * Checks the sink's static pad templates accept the codec caps of the media.
 * @throws std::runtime_error If the sink is missing, not a sink, or does not accept the caps.
 */
void verifySinkSupportsCodec(const std::string& sinkName, const MediaInfo& media);

/** Outcome and detail of one normal-playback check. */
struct NormalPlaybackCheck {
    bool passed = false;
    std::string detail;
};

/** The six normal-playback checks of one run. */
struct NormalPlaybackResult {
    NormalPlaybackCheck decoderLifecycle;
    NormalPlaybackCheck decoderDimensions;
    NormalPlaybackCheck decoderDropCount;
    NormalPlaybackCheck decoderFrameErrorCount;
    NormalPlaybackCheck decoderHardwareErrorCount;
    NormalPlaybackCheck caps;
    NormalPlaybackCheck bufferFlow;
    NormalPlaybackCheck bufferStatus;
    NormalPlaybackCheck avSync;
    NormalPlaybackCheck completion;

    /** @return True when all six checks passed. */
    bool overallPass() const
    {
         return decoderLifecycle.passed && decoderDimensions.passed && decoderDropCount.passed &&
             decoderFrameErrorCount.passed && decoderHardwareErrorCount.passed && caps.passed && bufferFlow.passed &&
               bufferStatus.passed && avSync.passed && completion.passed;
    }
};

/** Values observed while one scenario run plays. */
struct PlaybackObservations {
    bool failed = false;
    bool decoderDimensionsObserved = false;
    NormalPlaybackCheck decoderDimensions{false, "Decoder frame dimensions were not obtained during playback"};
    bool decoderDropCountObserved = false;
    NormalPlaybackCheck decoderDropCount{false, "Decoder drop count was not obtained during playback"};
    bool decoderFrameErrorCountObserved = false;
    NormalPlaybackCheck decoderFrameErrorCount{false, "Decoder frame error count was not obtained during playback"};
    bool decoderHardwareErrorCountObserved = false;
    NormalPlaybackCheck decoderHardwareErrorCount{false, "Decoder hardware error count was not obtained during playback"};
    bool reachedEos = false;
    bool pipelineReachedPlaying = false;
    std::vector<GstState> sinkStateHistory;
    bool firstFrameSeen = false;
    bool sinkDecodeErrorSignalConnected = false;
    bool sinkDecodeErrorDetected = false;
    unsigned int sinkDecodeErrorCode = 0;
    bool sinkPtsErrorSignalConnected = false;
    bool sinkPtsErrorDetected = false;
    unsigned int sinkPtsErrorCode = 0;
    std::uint64_t sinkBufferUnderflowCount = 0;
    bool sinkStatsAvailable = false;
    std::uint64_t sinkStatsDroppedFrames = 0;
    std::uint64_t sinkStatsRenderedFrames = 0;
    std::uint64_t qosDroppedFrames = 0;
    std::string qosDroppedSource;
    bool capsSeen = false;
    std::string negotiatedCaps;
    int capsWidth = 0;
    int capsHeight = 0;
    int capsFrameRateNumerator = 0;
    int capsFrameRateDenominator = 0;
    std::uint64_t playbackBuffers = 0;
    std::uint64_t validateReportCount = 0;
};

/** Evaluates the playback checks from one run's observations. */
class NormalPlaybackChecks {
public:
    /**
     * Evaluates all six checks. Checks that need a video sink pass as not applicable when the file has no video.
     * @param observations Values observed during the run.
     * @param configuration Platform configuration with the allowed frame drops.
     * @param expected Stream details of the media file.
     */
    static NormalPlaybackResult evaluate(const PlaybackObservations& observations,
                                         const L4Configuration& configuration,
                                         const MediaInfo& expected);

    /** Checks the video sink went PAUSED to PLAYING, the pipeline reached PLAYING, and no decode error was reported. */
    static NormalPlaybackCheck checkDecoderLifecycle(const PlaybackObservations& observations);

    /** Checks negotiated caps, H.264 alignment, and resolution and frame rate against the file. */
    static NormalPlaybackCheck checkCaps(const PlaybackObservations& observations, const MediaInfo& expected);

    /** Checks buffers flowed into the video sink pad. */
    static NormalPlaybackCheck checkBufferFlow(const PlaybackObservations& observations);

    /** Checks dropped frames and underflows against the configured limits. */
    static NormalPlaybackCheck checkBufferStatus(const PlaybackObservations& observations,
                                                 const L4Configuration& configuration);

    /** Checks the sink's pts-error-callback for A/V sync errors. */
    static NormalPlaybackCheck checkAvSync(const PlaybackObservations& observations);

    /** Checks the run reached EOS without an error or a GstValidate report. */
    static NormalPlaybackCheck checkCompletion(const PlaybackObservations& observations);
};

/** Bus, pad probe, signal and timer callbacks of a scenario run, defined in vts_media_framework.cpp. */
gboolean handle_bus_message(GstBus* bus, GstMessage* message, gpointer data);
gboolean handle_watchdog(gpointer data);
GstPadProbeReturn inspect_sink_pad(GstPad* pad, GstPadProbeInfo* info, gpointer data);
void on_first_video_frame(GstElement* sink, guint frame, gpointer unused, gpointer data);
void on_pts_error(GstElement* sink, guint errorCode, gpointer unused, gpointer data);
void on_decode_error(GstElement* sink, guint errorCode, gpointer unused, gpointer data);
void on_buffer_underflow(GstElement* sink, guint count, gpointer unused, gpointer data);
gboolean sample_sink_stats(gpointer data);

#endif
