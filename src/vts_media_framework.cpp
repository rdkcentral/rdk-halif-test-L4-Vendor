/** @file vts_media_framework.cpp
 * @brief Runs a GstValidate scenario file on the platform pipeline and evaluates the run.
 *
 * The scenario file is read by GstValidate from the device (scenario_dir in the AV test configuration).
 * The platform pipeline is created by PipelineManagementLayer; GstValidate monitors are attached to it.
 */
#include "vts_audio_video.h"

#include <ut.h>
#include <ut_log.h>

#include "vts_configuration.h"
#include "vts_pipeline_management.h"
#include "vts_verification.h"

#ifdef L4_AMLOGIC_PLATFORM
#include "vts_amlogic_verification.h"
#endif

#include <gst/gst.h>
#include <gst/validate/validate.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace {

namespace fs = std::filesystem;

/** Initializes GStreamer and GstValidate once per process and returns the shared runner. */
GstValidateRunner* ensureGstValidateInitialized()
{
    static GstValidateRunner* runner = nullptr;
    static std::once_flag once;
    std::call_once(once, [] {
        gst_init(nullptr, nullptr);
        gst_validate_init();
        runner = gst_validate_runner_new();
    });
    return runner;
}

struct GstObjectDeleter {
    void operator()(gpointer object) const
    {
        if (object) gst_object_unref(object);
    }
};

template <typename T>
using GstObjectPtr = std::unique_ptr<T, GstObjectDeleter>;

struct MainLoopDeleter {
    void operator()(GMainLoop* loop) const
    {
        if (loop) g_main_loop_unref(loop);
    }
};

using MainLoopPtr = std::unique_ptr<GMainLoop, MainLoopDeleter>;

struct RunContext {
    GMainLoop* loop;
    GstElement* pipeline;
    GstElement* sink;
    PlaybackObservations observations;
    MediaInfo expectedMedia;
    unsigned int allowedDecoderDrops;
};

void poll_decoder_dimensions(RunContext* context)
{
#ifdef L4_AMLOGIC_PLATFORM
    if (!context->expectedMedia.hasVideo || !context->observations.pipelineReachedPlaying) {
        return;
    }
    const auto check = validateAmlogicFrameDimensions(context->expectedMedia.width,
                                                     context->expectedMedia.height);
    auto& observations = context->observations;
    if (!observations.decoderDimensionsObserved || observations.decoderDimensions.passed) {
        if (check.available || !observations.decoderDimensionsObserved) {
            observations.decoderDimensions = {check.passed, check.detail};
        }
    }
    observations.decoderDimensionsObserved = observations.decoderDimensionsObserved || check.available;
#else
    (void)context;
#endif
}

void poll_decoder_drop_count(RunContext* context)
{
#ifdef L4_AMLOGIC_PLATFORM
    if (!context->expectedMedia.hasVideo || !context->observations.pipelineReachedPlaying) {
        return;
    }
    const auto check = validateAmlogicDropCount(context->allowedDecoderDrops);
    auto& observations = context->observations;
    if (!observations.decoderDropCountObserved || observations.decoderDropCount.passed) {
        if (check.available || !observations.decoderDropCountObserved) {
            observations.decoderDropCount = {check.passed, check.detail};
        }
    }
    observations.decoderDropCountObserved = observations.decoderDropCountObserved || check.available;
#else
    (void)context;
#endif
}

void poll_decoder_error_counts(RunContext* context)
{
#ifdef L4_AMLOGIC_PLATFORM
    if (!context->expectedMedia.hasVideo || !context->observations.pipelineReachedPlaying) {
        return;
    }
    auto retainCheck = [](const AmlogicErrorCountCheck& check, bool& observed,
                          NormalPlaybackCheck& result) {
        if ((!observed || result.passed) && (check.available || !observed)) {
            result = {check.passed, check.detail};
        }
        observed = observed || check.available;
    };
    auto& observations = context->observations;
    retainCheck(validateAmlogicFrameErrorCount(), observations.decoderFrameErrorCountObserved,
                observations.decoderFrameErrorCount);
    retainCheck(validateAmlogicHardwareErrorCount(), observations.decoderHardwareErrorCountObserved,
                observations.decoderHardwareErrorCount);
#else
    (void)context;
#endif
}

} // namespace

/** Records pipeline errors, EOS, PLAYING, and QOS drops for the checks, and ends the run at NULL. */
gboolean handle_bus_message(GstBus*, GstMessage* message, gpointer data)
{
    auto* context = static_cast<RunContext*>(data);
    PlaybackObservations& observations = context->observations;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError* error = nullptr;
        gchar* debug = nullptr;
        gst_message_parse_error(message, &error, &debug);
        UT_LOG_ERROR("Pipeline error: %s", error ? error->message : "unknown");
        g_clear_error(&error);
        g_free(debug);
        observations.failed = true;
        g_main_loop_quit(context->loop);
        break;
    }
    case GST_MESSAGE_EOS:
        UT_LOG("DEBUG bus: EOS received");
        poll_decoder_dimensions(context);
        poll_decoder_drop_count(context);
        poll_decoder_error_counts(context);
        observations.reachedEos = true;
        g_main_loop_quit(context->loop);
        break;
    case GST_MESSAGE_QOS: {
        guint64 dropped = 0;
        gst_message_parse_qos_stats(message, nullptr, nullptr, &dropped);
        UT_LOG("DEBUG bus: QOS message, dropped=%llu", static_cast<unsigned long long>(dropped));
        if (dropped > observations.qosDroppedFrames) {
            observations.qosDroppedFrames = dropped;
            observations.qosDroppedSource = GST_MESSAGE_SRC(message) ? GST_OBJECT_NAME(GST_MESSAGE_SRC(message)) : "unknown";
        }
        break;
    }
    case GST_MESSAGE_REQUEST_STATE: {
        GstState requested_state;
        gst_message_parse_request_state(message, &requested_state);
        poll_decoder_dimensions(context);
        poll_decoder_drop_count(context);
        poll_decoder_error_counts(context);
        gst_element_set_state(context->pipeline, requested_state);
        if (requested_state == GST_STATE_NULL) {
            g_main_loop_quit(context->loop);
        }
        break;
    }
    case GST_MESSAGE_STATE_CHANGED:
        if (GST_MESSAGE_SRC(message) == GST_OBJECT(context->pipeline)) {
            GstState old_state;
            GstState new_state;
            GstState pending;
            gst_message_parse_state_changed(message, &old_state, &new_state, &pending);
            if (new_state == GST_STATE_PLAYING) {
                UT_LOG("DEBUG bus: pipeline state -> PLAYING");
                observations.pipelineReachedPlaying = true;
            }
            if (new_state == GST_STATE_NULL) {
                g_main_loop_quit(context->loop);
            }
        } else if (context->sink && GST_MESSAGE_SRC(message) == GST_OBJECT(context->sink)) {
            GstState old_state;
            GstState new_state;
            GstState pending;
            gst_message_parse_state_changed(message, &old_state, &new_state, &pending);
            UT_LOG("DEBUG bus: video sink state -> %s", gst_element_state_get_name(new_state));
            observations.sinkStateHistory.push_back(new_state);
        }
        break;
    default:
        break;
    }
    return G_SOURCE_CONTINUE;
}

/** Ends the run when the scenario has not finished within the time limit. */
gboolean handle_watchdog(gpointer data)
{
    auto* context = static_cast<RunContext*>(data);
    UT_LOG("DEBUG hook: handle_watchdog invoked");
    UT_LOG_ERROR("Scenario did not finish within its time limit");
    context->observations.failed = true;
    g_main_loop_quit(context->loop);
    return G_SOURCE_REMOVE;
}

/** Records the caps negotiated at the video sink pad and counts the buffers passing it. */
GstPadProbeReturn inspect_sink_pad(GstPad*, GstPadProbeInfo* info, gpointer data)
{
    PlaybackObservations& observations = static_cast<RunContext*>(data)->observations;
    if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) != 0) {
        GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);
        if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS && !observations.capsSeen) {
            GstCaps* caps = nullptr;
            gst_event_parse_caps(event, &caps);
            if (caps && !gst_caps_is_empty(caps)) {
                gchar* capsString = gst_caps_to_string(caps);
                observations.negotiatedCaps = capsString ? capsString : "";
                g_free(capsString);
                const GstStructure* structure = gst_caps_get_structure(caps, 0);
                gst_structure_get_int(structure, "width", &observations.capsWidth);
                gst_structure_get_int(structure, "height", &observations.capsHeight);
                gint numerator = 0;
                gint denominator = 1;
                if (gst_structure_get_fraction(structure, "framerate", &numerator, &denominator)) {
                    observations.capsFrameRateNumerator = numerator;
                    observations.capsFrameRateDenominator = denominator;
                }
                observations.capsSeen = true;
                UT_LOG("DEBUG probe: caps negotiated: %s", observations.negotiatedCaps.c_str());
            }
        }
        return GST_PAD_PROBE_OK;
    }
    if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) != 0) {
        if (++observations.playbackBuffers == 1) {
            UT_LOG("DEBUG probe: first buffer at video sink pad");
        }
    }
    return GST_PAD_PROBE_OK;
}

/** Flags the first rendered frame from the sink's first-video-frame-callback signal. */
void on_first_video_frame(GstElement*, guint, gpointer, gpointer data)
{
    UT_LOG("DEBUG signal: first-video-frame-callback invoked");
    static_cast<RunContext*>(data)->observations.firstFrameSeen = true;
}

/** Records a PTS error from the sink's pts-error-callback signal. */
void on_pts_error(GstElement*, guint errorCode, gpointer, gpointer data)
{
    PlaybackObservations& observations = static_cast<RunContext*>(data)->observations;
    UT_LOG("DEBUG signal: pts-error-callback invoked, code=%u", errorCode);
    observations.sinkPtsErrorDetected = true;
    observations.sinkPtsErrorCode = errorCode;
}

/** Records a decode error from the sink's decode-error-callback signal. */
void on_decode_error(GstElement*, guint errorCode, gpointer, gpointer data)
{
    PlaybackObservations& observations = static_cast<RunContext*>(data)->observations;
    UT_LOG("DEBUG signal: decode-error-callback invoked, code=%u", errorCode);
    observations.sinkDecodeErrorDetected = true;
    observations.sinkDecodeErrorCode = errorCode;
}

/** Counts underflows from the sink's buffer-underflow-callback signal. */
void on_buffer_underflow(GstElement*, guint, gpointer, gpointer data)
{
    UT_LOG("DEBUG signal: buffer-underflow-callback invoked");
    ++static_cast<RunContext*>(data)->observations.sinkBufferUnderflowCount;
}

namespace {

/** Reads one unsigned field of a stats structure, stored as uint64 or uint. */
std::uint64_t structure_u64(const GstStructure* structure, const char* field)
{
    guint64 value64 = 0;
    if (gst_structure_get_uint64(structure, field, &value64)) {
        return value64;
    }
    guint value32 = 0;
    if (gst_structure_get_uint(structure, field, &value32)) {
        return value32;
    }
    return 0;
}

/** Reads the sink's stats property when the sink exposes one. */
void poll_sink_stats(RunContext* context)
{
    GstElement* sink = context->sink;
    if (!sink || !g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "stats")) {
        return;
    }
    GstStructure* stats = nullptr;
    g_object_get(sink, "stats", &stats, nullptr);
    if (!stats) {
        return;
    }
    context->observations.sinkStatsAvailable = true;
    context->observations.sinkStatsDroppedFrames = structure_u64(stats, "dropped");
    context->observations.sinkStatsRenderedFrames = structure_u64(stats, "rendered");
    gst_structure_free(stats);
}

} // namespace

/** Samples the sink's stats property while the scenario runs. */
gboolean sample_sink_stats(gpointer data)
{
    poll_sink_stats(static_cast<RunContext*>(data));
    poll_decoder_dimensions(static_cast<RunContext*>(data));
    poll_decoder_drop_count(static_cast<RunContext*>(data));
    poll_decoder_error_counts(static_cast<RunContext*>(data));
    return G_SOURCE_CONTINUE;
}

namespace {

/** Returns a new reference to the video sink set on playbin, or null when the pipeline has none. */
GstObjectPtr<GstElement> videoSinkOf(GstElement* pipeline)
{
    GstElement* videoSink = nullptr;
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(pipeline), "video-sink")) {
        g_object_get(pipeline, "video-sink", &videoSink, nullptr);
    }
    return GstObjectPtr<GstElement>(videoSink);
}

/** Writes one check line to the log. */
void log_check(const std::string& label, const char* name, const NormalPlaybackCheck& check)
{
    if (check.passed) {
        UT_LOG("%s %s ok: %s", label.c_str(), name, check.detail.c_str());
    } else {
        UT_LOG_ERROR("%s %s failed: %s", label.c_str(), name, check.detail.c_str());
    }
}

} // namespace

bool runScenarioFile(const L4Configuration& configuration,
                     const std::string& scenarioPath,
                     const std::string& mediaPath,
                     uint32_t timeoutS)
{
    GstValidateRunner* runner = ensureGstValidateInitialized();
    if (!runner) {
        UT_LOG_ERROR("Could not create GStreamer Validate runner.");
        return false;
    }

    const std::string absoluteMedia = fs::absolute(mediaPath).lexically_normal().string();
    const std::string label = fs::path(absoluteMedia).filename().string();

    MediaInfo mediaInfo;
    try {
        mediaInfo = MediaStreamDetector::detect(absoluteMedia);
    } catch (const std::exception& error) {
        UT_LOG_ERROR("%s: %s", label.c_str(), error.what());
        return false;
    }

    GError* parse_error = nullptr;
    GstObjectPtr<GstElement> pipeline;
    try {
        PipelineManagementLayer pipelineManagementLayer;
        pipeline.reset(pipelineManagementLayer.createScenarioGstPipeline(configuration, absoluteMedia, mediaInfo, &parse_error));
    } catch (const std::exception& error) {
        UT_LOG_ERROR("Could not create scenario pipeline: %s", error.what());
        g_clear_error(&parse_error);
        return false;
    }
    if (!pipeline) {
        UT_LOG_ERROR("Could not create pipeline: %s", parse_error ? parse_error->message : "unknown");
        g_clear_error(&parse_error);
        return false;
    }
    g_clear_error(&parse_error);

    GstObjectPtr<GstElement> sink(videoSinkOf(pipeline.get()));
    if (mediaInfo.hasVideo && sink && !configuration.videoSink.empty()) {
        try {
            verifySinkSupportsCodec(configuration.videoSink, mediaInfo);
        } catch (const std::exception& error) {
            UT_LOG_ERROR("%s: %s", label.c_str(), error.what());
            return false;
        }
    }

    GstObjectPtr<GstValidateMonitor> monitor(
        gst_validate_monitor_factory_create(GST_OBJECT(pipeline.get()), runner, nullptr));
    if (!monitor) {
        UT_LOG_ERROR("Could not create GStreamer Validate monitor.");
        return false;
    }

    GstObjectPtr<GstValidateScenario> validate_scenario(
        gst_validate_scenario_factory_create(runner, pipeline.get(), scenarioPath.c_str()));
    if (!validate_scenario) {
        UT_LOG_ERROR("Could not create scenario from %s", scenarioPath.c_str());
        return false;
    }

    UT_LOG("Running GStreamer Validate scenario %s on %s", scenarioPath.c_str(), absoluteMedia.c_str());

    const guint reportsBefore = gst_validate_runner_get_reports_count(runner);
    MainLoopPtr loop(g_main_loop_new(nullptr, FALSE));
    RunContext context{loop.get(), pipeline.get(), sink.get(), {}, mediaInfo, configuration.allowedFrameDrops};

    GstObjectPtr<GstPad> sinkPad(sink ? gst_element_get_static_pad(sink.get(), "sink") : nullptr);
    UT_LOG("DEBUG hook: video sink pad probe %s", sinkPad ? "attached" : "not attached (no video sink)");
    if (sinkPad) {
        gst_pad_add_probe(sinkPad.get(),
                          static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
                          inspect_sink_pad, &context, nullptr);
    }

    auto connectIfSupported = [](GstElement* element, const char* signalName, GCallback callback,
                                 gpointer data) -> gulong {
        if (!element || g_signal_lookup(signalName, G_OBJECT_TYPE(element)) == 0) {
            UT_LOG("DEBUG hook: signal %s not connected (not supported by sink)", signalName);
            return 0;
        }
        UT_LOG("DEBUG hook: signal %s connected", signalName);
        return g_signal_connect(element, signalName, callback, data);
    };
    connectIfSupported(sink.get(), "first-video-frame-callback", G_CALLBACK(on_first_video_frame), &context);
    context.observations.sinkPtsErrorSignalConnected =
        connectIfSupported(sink.get(), "pts-error-callback", G_CALLBACK(on_pts_error), &context) != 0;
    context.observations.sinkDecodeErrorSignalConnected =
        connectIfSupported(sink.get(), "decode-error-callback", G_CALLBACK(on_decode_error), &context) != 0;
    connectIfSupported(sink.get(), "buffer-underflow-callback", G_CALLBACK(on_buffer_underflow), &context);

    GstObjectPtr<GstBus> bus(gst_element_get_bus(pipeline.get()));
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(handle_bus_message), &context);
    UT_LOG("DEBUG hook: bus message handler connected");

    const guint watchdog = g_timeout_add_seconds(timeoutS, handle_watchdog, &context);
    const guint sampleId = g_timeout_add(250, sample_sink_stats, &context);
    if (gst_element_set_state(pipeline.get(), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        context.observations.failed = true;
    } else {
        g_main_loop_run(loop.get());
    }

    g_source_remove(watchdog);
    g_source_remove(sampleId);
    poll_sink_stats(&context);
    poll_decoder_dimensions(&context);
    poll_decoder_drop_count(&context);
    poll_decoder_error_counts(&context);
    gst_element_set_state(pipeline.get(), GST_STATE_NULL);

    const guint validateReports = gst_validate_runner_get_reports_count(runner) - reportsBefore;
    const int validateIssues = gst_validate_runner_exit(runner, TRUE);
    if (validateIssues != 0) {
        UT_LOG_ERROR("GStreamer Validate reported %d issue(s)", validateIssues);
    }

    gst_bus_remove_signal_watch(bus.get());

    PlaybackObservations& observations = context.observations;
    observations.validateReportCount = validateReports;
    const NormalPlaybackResult result = NormalPlaybackChecks::evaluate(observations, configuration, mediaInfo);
    log_check(label, "decoder_lifecycle", result.decoderLifecycle);
    log_check(label, "decoder_dimensions", result.decoderDimensions);
    log_check(label, "decoder_drop_count", result.decoderDropCount);
    log_check(label, "decoder_frame_error_count", result.decoderFrameErrorCount);
    log_check(label, "decoder_hardware_error_count", result.decoderHardwareErrorCount);
    log_check(label, "caps", result.caps);
    log_check(label, "buffer_flow", result.bufferFlow);
    log_check(label, "buffer_status", result.bufferStatus);
    log_check(label, "av_sync", result.avSync);
    log_check(label, "completion", result.completion);
    UT_LOG("%s overall: %s", label.c_str(), result.overallPass() ? "PASS" : "FAIL");
    return result.overallPass();
}
