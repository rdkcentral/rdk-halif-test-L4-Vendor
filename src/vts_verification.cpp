/** @file vts_verification.cpp
 * @brief Uses libavformat to detect the audio and video streams in a media file.
 */
#include "vts_verification.h"
#include <ut_log.h>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace {

/** Converts an FFmpeg error code into its diagnostic text. */
std::string ffmpegError(int code)
{
    char message[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(code, message, sizeof(message));
    return message;
}
}

namespace {
/** Maps libavcodec codec names to the encoded caps GStreamer negotiates; empty when unsupported. */
std::string capsForCodec(const std::string& codec)
{
    if (codec == "h264") {
        return "video/x-h264";
    }
    if (codec == "hevc" || codec == "h265") {
        return "video/x-h265";
    }
    if (codec == "mpeg2video") {
        return "video/mpeg";
    }
    if (codec == "vp9") {
        return "video/x-vp9";
    }
    if (codec == "av1") {
        return "video/x-av1";
    }
    return "";
}

/** Returns "N/D" for a frame-rate fraction. */
std::string framerateText(int numerator, int denominator)
{
    return std::to_string(numerator) + "/" + std::to_string(denominator);
}
} // namespace

/** Opens the input with libavformat and records stream presence and the first video stream's codec and size. */
DetectedVideoInfo MediaStreamDetector::detect(const std::string& mediaPath)
{
    AVFormatContext* formatContext = nullptr;
    const int openResult = avformat_open_input(&formatContext, mediaPath.c_str(), nullptr, nullptr);
    if (openResult < 0) {
        throw std::runtime_error("FFmpeg could not open media: " + mediaPath +
                                 " (" + ffmpegError(openResult) + ", code " +
                                 std::to_string(openResult) + ")");
    }
    const auto closeInput = [](AVFormatContext* context) { avformat_close_input(&context); };
    std::unique_ptr<AVFormatContext, decltype(closeInput)> input(formatContext, closeInput);

    const int streamResult = avformat_find_stream_info(input.get(), nullptr);
    if (streamResult < 0) {
        throw std::runtime_error("FFmpeg could not read media streams: " + mediaPath +
                                 " (" + ffmpegError(streamResult) + ", code " +
                                 std::to_string(streamResult) + ")");
    }

    DetectedVideoInfo info;
    for (unsigned int index = 0; index < input->nb_streams; ++index) {
        const AVStream* stream = input->streams[index];
        // Cover art is reported as a video stream but is not playable video.
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            continue;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            info.hasAudio = true;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!info.hasVideo) {
                info.capsName = capsForCodec(avcodec_get_name(stream->codecpar->codec_id));
                info.width = stream->codecpar->width;
                info.height = stream->codecpar->height;
                const AVRational rate = stream->avg_frame_rate.num > 0 ? stream->avg_frame_rate : stream->r_frame_rate;
                info.frameRateNumerator = rate.num;
                info.frameRateDenominator = rate.den > 0 ? rate.den : 1;
            }
            info.hasVideo = true;
        }
    }
    return info;
}

/** Checks the sink's static pad templates accept the codec caps of the media. */
void verifySinkSupportsCodec(const std::string& sinkName, const MediaInfo& media)
{
    UT_LOG("DEBUG verifySinkSupportsCodec invoked: sink=%s caps=%s", sinkName.c_str(), media.capsName.c_str());
    GstElementFactory* factory = gst_element_factory_find(sinkName.c_str());
    if (!factory) {
        throw std::runtime_error("Video sink factory not found: " + sinkName);
    }
    if (!gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_SINK)) {
        gst_object_unref(factory);
        throw std::runtime_error("Configured video sink is not a sink: " + sinkName);
    }
    if (media.capsName.empty()) {
        gst_object_unref(factory);
        throw std::runtime_error("Video codec is not supported by the checks for sink " + sinkName);
    }
    GstCaps* codecCaps = gst_caps_from_string(media.capsName.c_str());
    if (!codecCaps) {
        gst_object_unref(factory);
        throw std::runtime_error("Cannot build caps " + media.capsName + " for sink " + sinkName);
    }
    bool supported = false;
    for (const GList* pad = gst_element_factory_get_static_pad_templates(factory); pad; pad = pad->next) {
        auto* padTemplate = static_cast<GstStaticPadTemplate*>(pad->data);
        if (padTemplate->direction != GST_PAD_SINK) {
            continue;
        }
        GstCaps* sinkCaps = gst_static_pad_template_get_caps(padTemplate);
        supported = gst_caps_can_intersect(codecCaps, sinkCaps);
        gst_caps_unref(sinkCaps);
        if (supported) {
            break;
        }
    }
    gst_caps_unref(codecCaps);
    gst_object_unref(factory);
    if (!supported) {
        throw std::runtime_error("Video sink '" + sinkName + "' does not advertise support for " + media.capsName);
    }
}

/** Evaluates the six normal-playback checks; video checks are not applicable to audio-only files. */
NormalPlaybackResult NormalPlaybackChecks::evaluate(const PlaybackObservations& observations,
                                                    const L4Configuration& configuration,
                                                    const MediaInfo& expected)
{
    UT_LOG("DEBUG evaluate invoked: hasVideo=%d", expected.hasVideo ? 1 : 0);
    NormalPlaybackResult result;
    result.completion = checkCompletion(observations);
    if (!expected.hasVideo) {
        const NormalPlaybackCheck notApplicable{true, "Not applicable: file has no video stream"};
        result.decoderDimensions = notApplicable;
        result.decoderDropCount = notApplicable;
        result.decoderFrameErrorCount = notApplicable;
        result.decoderHardwareErrorCount = notApplicable;
        result.decoderLifecycle = notApplicable;
        result.caps = notApplicable;
        result.bufferFlow = notApplicable;
        result.bufferStatus = notApplicable;
        result.avSync = notApplicable;
        return result;
    }
#ifdef L4_AMLOGIC_PLATFORM
    result.decoderDimensions = observations.decoderDimensions;
    result.decoderDropCount = observations.decoderDropCount;
    result.decoderFrameErrorCount = observations.decoderFrameErrorCount;
    result.decoderHardwareErrorCount = observations.decoderHardwareErrorCount;
#else
    result.decoderDimensions = {true, "Not applicable: decoder dimensions check is not implemented for this platform"};
    result.decoderDropCount = {true, "Not applicable: decoder drop count check is not implemented for this platform"};
    result.decoderFrameErrorCount = {true, "Not applicable: decoder frame error count check is not implemented for this platform"};
    result.decoderHardwareErrorCount = {true, "Not applicable: decoder hardware error count check is not implemented for this platform"};
#endif
    result.decoderLifecycle = checkDecoderLifecycle(observations);
    result.caps = checkCaps(observations, expected);
    result.bufferFlow = checkBufferFlow(observations);
    result.bufferStatus = checkBufferStatus(observations, configuration);
    result.avSync = checkAvSync(observations);
    return result;
}

/** Checks the video sink went PAUSED to PLAYING, the pipeline reached PLAYING, and a first frame was rendered. */
NormalPlaybackCheck NormalPlaybackChecks::checkDecoderLifecycle(const PlaybackObservations& observations)
{
    UT_LOG("DEBUG checkDecoderLifecycle invoked: sinkStates=%lu", static_cast<unsigned long>(observations.sinkStateHistory.size()));
    if (observations.sinkDecodeErrorDetected) {
        return {false, "Sink reported a decode error (decode-error-callback code=" +
                           std::to_string(observations.sinkDecodeErrorCode) + ")"};
    }
    const auto& states = observations.sinkStateHistory;
    const auto paused = std::find(states.begin(), states.end(), GST_STATE_PAUSED);
    if (paused == states.end()) {
        return {false, "Video sink never reached PAUSED"};
    }
    if (std::find(paused, states.end(), GST_STATE_PLAYING) == states.end()) {
        return {false, "Video sink did not reach PLAYING after PAUSED"};
    }
    if (!observations.pipelineReachedPlaying) {
        return {false, "Pipeline never reached PLAYING"};
    }
    if (!observations.firstFrameSeen) {
        return {false, "No first video frame observed at the sink"};
    }
    return {true, "Video sink went PAUSED to PLAYING; pipeline reached PLAYING and rendered a first frame"};
}

/** Checks caps are present, H.264 has access-unit alignment, and resolution and frame rate match the file. */
NormalPlaybackCheck NormalPlaybackChecks::checkCaps(const PlaybackObservations& observations, const MediaInfo& expected)
{
    UT_LOG("DEBUG checkCaps invoked: capsSeen=%d", observations.capsSeen ? 1 : 0);
    if (!observations.capsSeen || observations.negotiatedCaps.empty()) {
        return {false, "No caps event observed at the video sink pad"};
    }
    if (observations.negotiatedCaps.find("video/x-h264") != std::string::npos &&
        observations.negotiatedCaps.find("alignment=(string)au") == std::string::npos) {
        return {false, "H.264 caps missing alignment=au: " + observations.negotiatedCaps};
    }
    if (expected.width > 0 &&
        (observations.capsWidth != expected.width || observations.capsHeight != expected.height)) {
        return {false, "Caps resolution " + std::to_string(observations.capsWidth) + "x" +
                           std::to_string(observations.capsHeight) + " differs from file " +
                           std::to_string(expected.width) + "x" + std::to_string(expected.height)};
    }
    if (expected.frameRateNumerator > 0) {
        const std::string expectedRate = framerateText(expected.frameRateNumerator, expected.frameRateDenominator);
        if (observations.capsFrameRateNumerator == 0) {
            return {false, "Caps have no framerate; file is " + expectedRate};
        }
        if (gst_util_fraction_compare(observations.capsFrameRateNumerator, observations.capsFrameRateDenominator,
                                      expected.frameRateNumerator, expected.frameRateDenominator) != 0) {
            return {false, "Caps framerate " +
                               framerateText(observations.capsFrameRateNumerator,
                                             observations.capsFrameRateDenominator) +
                               " differs from file " + expectedRate};
        }
    }
    return {true, "Negotiated caps: " + observations.negotiatedCaps};
}

/** Checks that buffers actually flowed into the video sink pad. */
NormalPlaybackCheck NormalPlaybackChecks::checkBufferFlow(const PlaybackObservations& observations)
{
    UT_LOG("DEBUG checkBufferFlow invoked: buffers=%llu", static_cast<unsigned long long>(observations.playbackBuffers));
    if (observations.playbackBuffers == 0) {
        return {false, "No buffers observed at the video sink pad"};
    }
    return {true, std::to_string(observations.playbackBuffers) + " buffers observed"};
}

/** Checks dropped frames and the sink's underflow callback against the configured limits. */
NormalPlaybackCheck NormalPlaybackChecks::checkBufferStatus(const PlaybackObservations& observations,
                                                            const L4Configuration& configuration)
{
    UT_LOG("DEBUG checkBufferStatus invoked: allowedFrameDrops=%u", configuration.allowedFrameDrops);
    const std::uint64_t droppedFrames = observations.sinkStatsAvailable ? observations.sinkStatsDroppedFrames
                                                                        : observations.qosDroppedFrames;
    const std::string droppedSource = observations.sinkStatsAvailable
                                          ? "sink stats property"
                                          : "QOS from '" + observations.qosDroppedSource + "'";
    if (droppedFrames > configuration.allowedFrameDrops) {
        return {false, "Dropped " + std::to_string(droppedFrames) + " frames (source: " + droppedSource +
                           "), exceeding allowed " + std::to_string(configuration.allowedFrameDrops)};
    }
    if (observations.sinkBufferUnderflowCount > 0) {
        return {false, "Sink reported " + std::to_string(observations.sinkBufferUnderflowCount) +
                           " buffer-underflow-callback event(s)"};
    }
    std::string detail = std::to_string(droppedFrames) + " frames dropped (source: " + droppedSource +
                         "; allowed " + std::to_string(configuration.allowedFrameDrops) + ")";
    if (observations.sinkStatsAvailable) {
        detail += "; sink rendered_frames=" + std::to_string(observations.sinkStatsRenderedFrames);
    }
    detail += "; sink buffer_underflows=" + std::to_string(observations.sinkBufferUnderflowCount);
    return {true, detail};
}

/** Checks the sink's pts-error-callback signal for A/V sync errors. */
NormalPlaybackCheck NormalPlaybackChecks::checkAvSync(const PlaybackObservations& observations)
{
    UT_LOG("DEBUG checkAvSync invoked: ptsSignalConnected=%d", observations.sinkPtsErrorSignalConnected ? 1 : 0);
    if (!observations.sinkPtsErrorSignalConnected) {
        return {false, "Sink does not expose a pts-error-callback signal; cannot verify A/V sync"};
    }
    if (observations.sinkPtsErrorDetected) {
        return {false, "Sink reported a PTS error (pts-error-callback code=" +
                           std::to_string(observations.sinkPtsErrorCode) + ")"};
    }
    return {true, "No PTS errors reported by the sink's pts-error-callback"};
}

/** Checks the run reached EOS without an error or a GstValidate report. */
NormalPlaybackCheck NormalPlaybackChecks::checkCompletion(const PlaybackObservations& observations)
{
    UT_LOG("DEBUG checkCompletion invoked: reachedEos=%d", observations.reachedEos ? 1 : 0);
    if (observations.failed) {
        return {false, "Pipeline reported an error"};
    }
    if (observations.validateReportCount > 0) {
        return {false, "GstValidate reported " + std::to_string(observations.validateReportCount) + " issue(s)"};
    }
    if (!observations.reachedEos) {
        return {false, "Run stopped without reaching EOS"};
    }
    return {true, "Reached EOS"};
}



