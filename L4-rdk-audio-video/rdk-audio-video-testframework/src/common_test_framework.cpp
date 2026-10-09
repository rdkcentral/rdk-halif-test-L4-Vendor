/** @file common_test_framework.cpp
 * @brief Executes GstValidate scenarios and records pipeline/timestamp reports.
 */
#include "L4-rdk-audio-video.h"
#include <ut.h>

#include "l4_configuration.h"
#include "pipeline_management_layer.h"
#include "verification_layer.h"
#include <gst/gst.h>
#include <gst/validate/validate.h>

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
/** Selects the environment override or the configured installation path. */
std::string configurationPath()
{
    const char* path = std::getenv("L4_CONFIG");
    return path ? path : L4_DEFAULT_CONFIG_PATH;
}
}

/** Initializes the common scenario runner. */
RDKAudioDecoderL4::RDKAudioDecoderL4()

{
}

/** Emits the UT smoke-test message. */
void RDKAudioDecoderL4::helloWorld()
{
    std::cout << "Hello world" << std::endl;
}


/** Loads runtime settings and runs the pause/resume scenario. */
bool RDKAudioDecoderL4::runPauseValidationScenario()
{
    const L4Configuration configuration = ConfigurationLayer::load(configurationPath());
    return runVideoScenario(configuration, "pause", configuration.pauseVideo, configuration.videoSink,
                            configuration.decoderElement, false,
                            configuration.eosTimeoutSeconds);
}

/** Loads runtime settings and runs the forward-seek scenario. */
bool RDKAudioDecoderL4::runForwardSeekValidationScenario()
{
    const L4Configuration configuration = ConfigurationLayer::load(configurationPath());
    return runVideoScenario(configuration, "forward-seek", configuration.forwardSeekVideo, configuration.videoSink,
                            configuration.decoderElement, false,
                            configuration.eosTimeoutSeconds);
}

/** Runs both configured timestamp-defective inputs and requires both expectations to pass. */
bool RDKAudioDecoderL4::runTimestampContinuityValidationScenario()
{
    const L4Configuration configuration = ConfigurationLayer::load(configurationPath());
    const bool gapResult = runVideoScenario(configuration, "timestamp-continuity", configuration.timestampGapVideo,
                                            configuration.videoSink, configuration.decoderElement, true,
                                            configuration.eosTimeoutSeconds);
    const bool negativeJumpResult = runVideoScenario(
        configuration, "timestamp-continuity", configuration.timestampNegativeJumpVideo,
        configuration.videoSink, configuration.decoderElement, true,
        configuration.eosTimeoutSeconds);
    return gapResult && negativeJumpResult;
}


namespace {

/** Returns the GstValidate action script associated with a supported scenario name. */
const char* scenario_content(const std::string& scenario_name)
{
    if (scenario_name == "timestamp-continuity") {
        return R"SCENARIO(description, handles-states=true, need-clock-sync=true
play, playback-time=0.0
stop, playback-time=20.0
)SCENARIO";
    }
    if (scenario_name == "pause") {
        return R"SCENARIO(description, handles-states=true, need-clock-sync=true
play, playback-time=0.0
pause, playback-time=5.0, duration=5.0
play, playback-time=10.0
stop, playback-time=15.0
)SCENARIO";
    }
    if (scenario_name == "forward-seek") {
        return R"SCENARIO(description, seek=true, handles-states=true, need-clock-sync=true
play, playback-time=0.0
wait, duration=5.0
seek, name=ForwardSeek, playback-time=5.0, start=30.0, flags=accurate+flush
wait, duration=5.0
stop, playback-time=10.0
)SCENARIO";
    }
    return nullptr;
}

struct TimestampAnomaly {
    /** Captures one PTS discontinuity for the runtime JSON report. */
    struct TimestampAnomaly {
    guint buffer_index = 0;
    GstClockTime previous_pts = GST_CLOCK_TIME_NONE;
    GstClockTime current_pts = GST_CLOCK_TIME_NONE;
    GstClockTime delta = GST_CLOCK_TIME_NONE;
    GstClockTime expected = GST_CLOCK_TIME_NONE;
    bool explicitly_signaled = false;
    std::string reason;
    std::string description;
    std::string occurred_at_utc;
    double previous_pts_seconds = 0.0;
    double current_pts_seconds = 0.0;
    double delta_seconds = 0.0;
    double expected_seconds = 0.0;
};

struct GStreamerLogEntry {
    /** Stores a warning-or-higher GStreamer log with source and UTC metadata. */
    struct GStreamerLogEntry {
    std::string level;
    int level_number = 0;
    std::string category;
    std::string file;
    std::string function;
    int line = 0;
    std::string object;
    std::string message;
    std::string full_line;
    std::string occurred_at_utc;
};

std::string utc_timestamp(std::chrono::system_clock::time_point time);

struct RunContext {
    /** Shared state passed to GLib/GStreamer callbacks for one scenario run. */
    struct RunContext {
    GMainLoop* loop;
    GstElement* pipeline;
    bool failed;
    gint64 media_duration;
    gint64 final_position;
    bool timestamp_failed;
    bool timestamp_signal_pending;
    bool timestamp_have_previous;
    GstClockTime timestamp_previous;
    GstClockTime timestamp_interval;
    guint timestamp_buffers;
    guint timestamp_negative_jumps;
    guint timestamp_gaps;
    guint timestamp_duplicates;
    guint timestamp_unsignaled_discontinuities;
    std::vector<TimestampAnomaly> timestamp_anomalies;
    mutable std::mutex gstreamer_log_mutex;
    std::vector<GStreamerLogEntry> gstreamer_logs;
    guint timeout_source_id;
    bool timed_out;
};

/** Fails the run and quits its main loop when the configured deadline expires. */
gboolean stop_scenario_on_timeout(gpointer data)
{
    auto* context = static_cast<RunContext*>(data);
    context->timeout_source_id = 0;
    context->timed_out = true;
    context->failed = true;
    std::cerr << "GStreamer scenario timed out before EOS or stop" << '\n';
    g_main_loop_quit(context->loop);
    return G_SOURCE_REMOVE;
}

/** Retains warnings and errors; GStreamer may invoke this from streaming threads. */
void capture_gstreamer_log(GstDebugCategory* category, GstDebugLevel level,
                           const gchar* file, const gchar* function, gint line,
                           GObject* object, GstDebugMessage* message,
                           gpointer user_data)
{
    if (level < GST_LEVEL_WARNING || !user_data || !message) {
        return;
    }
    auto* context = static_cast<RunContext*>(user_data);
    GStreamerLogEntry entry;
    // Logging callbacks can run concurrently, so protect the report collection.
    entry.level = gst_debug_level_get_name(level);
    entry.level_number = level;
    entry.category = category && category->name ? category->name : "";
    entry.file = file ? file : "";
    entry.function = function ? function : "";
    entry.line = line;
    entry.object = object ? GST_OBJECT_NAME(object) : "";
    entry.message = gst_debug_message_get(message);
    entry.occurred_at_utc = utc_timestamp(std::chrono::system_clock::now());
    gchar* formatted_line = gst_debug_log_get_line(category, level, file, function,
                                                   line, object, message);
    entry.full_line = formatted_line ? formatted_line : entry.message;
    g_free(formatted_line);
    std::lock_guard<std::mutex> lock(context->gstreamer_log_mutex);
    context->gstreamer_logs.push_back(std::move(entry));
}

/** Counts PTS anomalies at the decoder output and returns their probe buffers unchanged. */
GstPadProbeReturn inspect_timestamps(GstPad*, GstPadProbeInfo* info, gpointer data)
{
    auto* context = static_cast<RunContext*>(data);
    if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) != 0) {
        GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);
        if (GST_EVENT_TYPE(event) == GST_EVENT_SEGMENT) {
            // A new segment marks a stream boundary where a timestamp jump may be intentional.
            context->timestamp_signal_pending = true;
        }
        return GST_PAD_PROBE_OK;
    }
    if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) == 0) {
        return GST_PAD_PROBE_OK;
    }
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GstClockTime pts = GST_BUFFER_PTS(buffer);
    if (!GST_CLOCK_TIME_IS_VALID(pts)) {
        return GST_PAD_PROBE_OK;
    }
    ++context->timestamp_buffers;
    const bool explicitly_signaled = context->timestamp_signal_pending ||
                                      GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT);
    context->timestamp_signal_pending = false;
    if (!context->timestamp_have_previous) {
        context->timestamp_previous = pts;
        context->timestamp_have_previous = true;
        return GST_PAD_PROBE_OK;
    }
    const GstClockTime delta = pts >= context->timestamp_previous
                                   ? pts - context->timestamp_previous
                                   : 0;
    if (delta > 0 && context->timestamp_interval == GST_CLOCK_TIME_NONE) {
        context->timestamp_interval = delta;
    }
    const GstClockTime expected = context->timestamp_interval;
    if (pts < context->timestamp_previous) {
            // The first positive PTS delta establishes a baseline; gaps beyond 1.5x fail if unsignaled.
            if (pts < context->timestamp_previous) {
        ++context->timestamp_negative_jumps;
        context->timestamp_failed = true;
        TimestampAnomaly anomaly;
        anomaly.buffer_index = context->timestamp_buffers;
        anomaly.previous_pts = context->timestamp_previous;
        anomaly.current_pts = pts;
        anomaly.delta = delta;
        anomaly.expected = expected;
        anomaly.explicitly_signaled = explicitly_signaled;
        anomaly.reason = "negative_jump";
        anomaly.occurred_at_utc = utc_timestamp(std::chrono::system_clock::now());
        anomaly.previous_pts_seconds = static_cast<double>(anomaly.previous_pts) / GST_SECOND;
        anomaly.current_pts_seconds = static_cast<double>(anomaly.current_pts) / GST_SECOND;
        anomaly.delta_seconds = static_cast<double>(anomaly.delta) / GST_SECOND;
        anomaly.expected_seconds = expected == GST_CLOCK_TIME_NONE ? 0.0 : static_cast<double>(anomaly.expected) / GST_SECOND;
        anomaly.description = "PTS jumped backwards by " + std::to_string(anomaly.delta_seconds) +
                              "s from " + std::to_string(anomaly.previous_pts_seconds) +
                              "s to " + std::to_string(anomaly.current_pts_seconds) + "s";
        context->timestamp_anomalies.push_back(anomaly);
    } else if (delta == 0) {
        ++context->timestamp_duplicates;
        context->timestamp_failed = true;
        TimestampAnomaly anomaly;
        anomaly.buffer_index = context->timestamp_buffers;
        anomaly.previous_pts = context->timestamp_previous;
        anomaly.current_pts = pts;
        anomaly.delta = delta;
        anomaly.expected = expected;
        anomaly.explicitly_signaled = explicitly_signaled;
        anomaly.reason = "duplicate";
        anomaly.occurred_at_utc = utc_timestamp(std::chrono::system_clock::now());
        anomaly.previous_pts_seconds = static_cast<double>(anomaly.previous_pts) / GST_SECOND;
        anomaly.current_pts_seconds = static_cast<double>(anomaly.current_pts) / GST_SECOND;
        anomaly.delta_seconds = static_cast<double>(anomaly.delta) / GST_SECOND;
        anomaly.expected_seconds = expected == GST_CLOCK_TIME_NONE ? 0.0 : static_cast<double>(anomaly.expected) / GST_SECOND;
        anomaly.description = "Duplicate PTS detected: " + std::to_string(anomaly.current_pts_seconds) +
                              "s repeated without change";
        context->timestamp_anomalies.push_back(anomaly);
    } else if (expected != GST_CLOCK_TIME_NONE && delta > expected * 3 / 2) {
        ++context->timestamp_gaps;
        TimestampAnomaly anomaly;
        anomaly.buffer_index = context->timestamp_buffers;
        anomaly.previous_pts = context->timestamp_previous;
        anomaly.current_pts = pts;
        anomaly.delta = delta;
        anomaly.expected = expected;
        anomaly.explicitly_signaled = explicitly_signaled;
        anomaly.reason = explicitly_signaled ? "gap_signaled" : "gap_unsignaled";
        anomaly.occurred_at_utc = utc_timestamp(std::chrono::system_clock::now());
        anomaly.previous_pts_seconds = static_cast<double>(anomaly.previous_pts) / GST_SECOND;
        anomaly.current_pts_seconds = static_cast<double>(anomaly.current_pts) / GST_SECOND;
        anomaly.delta_seconds = static_cast<double>(anomaly.delta) / GST_SECOND;
        anomaly.expected_seconds = static_cast<double>(anomaly.expected) / GST_SECOND;
        anomaly.description = explicitly_signaled
            ? "Large gap detected but explicitly signaled: " + std::to_string(anomaly.delta_seconds) + "s jump"
            : "Gap without discontinuity flag: PTS jumped forward by " + std::to_string(anomaly.delta_seconds) + "s";
        context->timestamp_anomalies.push_back(anomaly);
        if (!explicitly_signaled) {
            ++context->timestamp_unsignaled_discontinuities;
            context->timestamp_failed = true;
            TimestampAnomaly discontinuity = anomaly;
            discontinuity.reason = "unsignaled_discontinuity";
            discontinuity.description = "PTS jumped forward by " + std::to_string(discontinuity.delta_seconds) +
                                        "s without signaling discontinuity";
            context->timestamp_anomalies.push_back(discontinuity);
        }
    }
    context->timestamp_previous = pts;
    return GST_PAD_PROBE_OK;
}

/** Converts pipeline bus messages into run status and main-loop termination. */
gboolean handle_bus_message(GstBus*, GstMessage* message, gpointer data)
{
    auto* context = static_cast<RunContext*>(data);
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
        GError* error = nullptr;
        gchar* debug = nullptr;
        gst_message_parse_error(message, &error, &debug);
        std::cerr << "Pipeline error: " << (error ? error->message : "unknown") << '\n';
        g_clear_error(&error);
        g_free(debug);
        context->failed = true;
        g_main_loop_quit(context->loop);
    } else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
        g_main_loop_quit(context->loop);
    } else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_REQUEST_STATE) {
        GstState requested_state;
        gst_message_parse_request_state(message, &requested_state);
        gst_element_set_state(context->pipeline, requested_state);
        if (requested_state == GST_STATE_NULL) {
            g_main_loop_quit(context->loop);
        }
    } else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED &&
               GST_MESSAGE_SRC(message) == GST_OBJECT(context->pipeline)) {
        GstState old_state;
        GstState new_state;
        GstState pending;
        gst_message_parse_state_changed(message, &old_state, &new_state, &pending);
        if (new_state == GST_STATE_PLAYING) {
            gst_element_query_duration(context->pipeline, GST_FORMAT_TIME, &context->media_duration);
            gst_element_query_position(context->pipeline, GST_FORMAT_TIME, &context->final_position);
        }
        if (new_state == GST_STATE_NULL) {
            g_main_loop_quit(context->loop);
        }
    }
    return G_SOURCE_CONTINUE;
}

/** Periodically samples duration and position for the run report. */
gboolean sample_pipeline_position(gpointer data)
{
    auto* context = static_cast<RunContext*>(data);
    gst_element_query_duration(context->pipeline, GST_FORMAT_TIME, &context->media_duration);
    gst_element_query_position(context->pipeline, GST_FORMAT_TIME, &context->final_position);
    return G_SOURCE_CONTINUE;
}

/** Formats a system-clock point as an ISO-8601 UTC timestamp. */
std::string utc_timestamp(std::chrono::system_clock::time_point time)
{
    const std::time_t raw_time = std::chrono::system_clock::to_time_t(time);
    std::tm utc_time{};
    gmtime_r(&raw_time, &utc_time);
    std::ostringstream value;
    value << std::put_time(&utc_time, "%Y-%m-%dT%H:%M:%SZ");
    return value.str();
}

/** Escapes JSON string control characters and delimiters. */
std::string json_escape(const std::string& value)
{
    std::ostringstream escaped;
    for (const char character : value) {
        switch (character) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default: escaped << character; break;
        }
    }
    return escaped.str();
}

/** Mirrors the selected scenario's actions in the machine-readable report. */
std::string scenario_actions(const std::string& scenario_name)
{
    if (scenario_name == "timestamp-continuity") {
        return "[\n"
               "      {\"type\": \"play\", \"playback_time_seconds\": 0.0},\n"
               "      {\"type\": \"stop\", \"playback_time_seconds\": 20.0}\n"
               "    ]";
    }
    if (scenario_name == "pause") {
        return "[\n"
               "      {\"type\": \"play\", \"playback_time_seconds\": 0.0},\n"
               "      {\"type\": \"pause\", \"playback_time_seconds\": 5.0, \"duration_seconds\": 5.0},\n"
               "      {\"type\": \"play\", \"playback_time_seconds\": 10.0},\n"
               "      {\"type\": \"stop\", \"playback_time_seconds\": 15.0}\n"
               "    ]";
    }
    return "[\n"
           "      {\"type\": \"play\", \"playback_time_seconds\": 0.0},\n"
           "      {\"type\": \"wait\", \"duration_seconds\": 5.0},\n"
           "      {\"type\": \"seek\", \"playback_time_seconds\": 5.0, \"target_seconds\": 30.0},\n"
           "      {\"type\": \"wait\", \"duration_seconds\": 5.0},\n"
           "      {\"type\": \"stop\", \"playback_time_seconds\": 10.0}\n"
           "    ]";
}

/** Serializes one scenario run, its timestamp anomalies, and captured GStreamer logs. */
std::string make_run_report(GstValidateRunner* runner, const RunContext& context,
                            const std::string& scenario_name,
                            const std::string& video_path, bool pipeline_failed,
                            const std::string& started_at, const std::string& finished_at,
                            double elapsed_seconds, double media_duration_seconds,
                            double final_position_seconds)
{
    std::ostringstream report;
    // The JSON report keeps timestamps in both nanoseconds and seconds for consumers.
    report << "  {\n"
           << "    \"scenario\": \"" << json_escape(scenario_name) << "\",\n"
           << "    \"video\": \"" << json_escape(video_path) << "\",\n"
           << "    \"pipeline_failed\": " << (pipeline_failed ? "true" : "false") << ",\n"
           << "    \"started_at\": \"" << started_at << "\",\n"
           << "    \"finished_at\": \"" << finished_at << "\",\n"
           << "    \"elapsed_seconds\": " << elapsed_seconds << ",\n"
           << "    \"media_duration_seconds\": " << media_duration_seconds << ",\n"
           << "    \"final_position_seconds\": " << final_position_seconds << ",\n"
           << "    \"validation_failed\": " << (context.timestamp_failed ? "true" : "false") << ",\n"
           << "    \"actions\": " << scenario_actions(scenario_name) << ",\n"
            ;
        if (scenario_name == "timestamp-continuity") {
         report << "    \"timestamp_continuity\": {\n"
             << "      \"buffers_checked\": " << context.timestamp_buffers << ",\n"
             << "      \"negative_jumps\": " << context.timestamp_negative_jumps << ",\n"
             << "      \"unexpected_gaps\": " << context.timestamp_gaps << ",\n"
             << "      \"duplicates\": " << context.timestamp_duplicates << ",\n"
             << "      \"unsignaled_discontinuities\": "
             << context.timestamp_unsignaled_discontinuities << ",\n"
             << "      \"failed\": " << (context.timestamp_failed ? "true" : "false") << ",\n"
             << "      \"anomalies\": [\n";
         for (std::size_t index = 0; index < context.timestamp_anomalies.size(); ++index) {
             const auto& anomaly = context.timestamp_anomalies[index];
             report << "        {\n"
                 << "          \"buffer_index\": " << anomaly.buffer_index << ",\n"
                 << "          \"reason\": \"" << json_escape(anomaly.reason) << "\",\n"
                 << "          \"description\": \"" << json_escape(anomaly.description) << "\",\n"
                 << "          \"previous_pts_ns\": " << anomaly.previous_pts << ",\n"
                 << "          \"current_pts_ns\": " << anomaly.current_pts << ",\n"
                 << "          \"delta_ns\": " << anomaly.delta << ",\n"
                 << "          \"expected_interval_ns\": " << anomaly.expected << ",\n"
                 << "          \"previous_pts_seconds\": " << anomaly.previous_pts_seconds << ",\n"
                 << "          \"current_pts_seconds\": " << anomaly.current_pts_seconds << ",\n"
                 << "          \"delta_seconds\": " << anomaly.delta_seconds << ",\n"
                 << "          \"expected_interval_seconds\": " << anomaly.expected_seconds << ",\n"
                 << "          \"explicitly_signaled\": " << (anomaly.explicitly_signaled ? "true" : "false") << ",\n"
                 << "          \"occurred_at_utc\": \"" << json_escape(anomaly.occurred_at_utc) << "\"\n"
                 << "        }" << (index + 1 < context.timestamp_anomalies.size() ? "," : "") << "\n";
         }
         report << "      ]\n"
             << "    },\n";
        }

        std::lock_guard<std::mutex> lock(context.gstreamer_log_mutex);
        // Hold the same mutex used by the asynchronous GStreamer log callback.
        report << "    \"gstreamer_report_count\": " << context.gstreamer_logs.size() << ",\n"
            << "    \"gstreamer_reports\": [\n";
        for (std::size_t index = 0; index < context.gstreamer_logs.size(); ++index) {
         const auto& entry = context.gstreamer_logs[index];
         report << "      {\n"
             << "        \"level\": \"" << json_escape(entry.level) << "\",\n"
             << "        \"level_number\": " << entry.level_number << ",\n"
             << "        \"category\": \"" << json_escape(entry.category) << "\",\n"
             << "        \"object\": \"" << json_escape(entry.object) << "\",\n"
             << "        \"file\": \"" << json_escape(entry.file) << "\",\n"
             << "        \"function\": \"" << json_escape(entry.function) << "\",\n"
             << "        \"line\": " << entry.line << ",\n"
             << "        \"message\": \"" << json_escape(entry.message) << "\",\n"
             << "        \"native_log\": \"" << json_escape(entry.full_line) << "\",\n"
             << "        \"occurred_at_utc\": \"" << json_escape(entry.occurred_at_utc) << "\"\n"
             << "      }" << (index + 1 < context.gstreamer_logs.size() ? "," : "") << "\n";
        }
        report << "    ]\n  }\n";
    return report.str();
}

/** Writes a new report array or appends this run to an existing JSON array. */
    void write_report(const std::filesystem::path& report_path, GstValidateRunner* runner,
                const RunContext& context,
                  const std::string& scenario_name, const std::string& video_path,
                  bool pipeline_failed, const std::string& started_at,
                  const std::string& finished_at, double elapsed_seconds,
                  double media_duration_seconds, double final_position_seconds)
{
    const std::string run_report = make_run_report(runner, context, scenario_name, video_path,
                                                   pipeline_failed, started_at, finished_at,
                                                   elapsed_seconds, media_duration_seconds,
                                                   final_position_seconds);
    std::ifstream existing(report_path);
    std::string contents((std::istreambuf_iterator<char>(existing)),
                         std::istreambuf_iterator<char>());
    std::ofstream report(report_path, std::ios::trunc);
    if (contents.empty()) {
        report << "[\n" << run_report << "]\n";
        return;
    }
    const std::size_t closing_bracket = contents.rfind(']');
    if (contents.front() != '[' || closing_bracket == std::string::npos) {
        report << "[\n" << run_report << "]\n";
        return;
    }
    contents.erase(closing_bracket);
    while (!contents.empty() && std::isspace(static_cast<unsigned char>(contents.back()))) {
        contents.pop_back();
    }
    report << contents << (contents.back() == '[' ? "" : ",\n")
           << run_report << "]\n";
}

} // namespace

/** Builds, runs, evaluates, reports, and releases one configured GstValidate scenario. */
bool RDKAudioDecoderL4::runVideoScenario(const L4Configuration& configuration,
                                         const std::string& scenarioName,
                                         const std::string& videoPath,
                                         const std::string& videoSink,
                                         const std::string& decoderElement,
                                         bool expectTimestampAnomaly,
                                         unsigned int eosTimeoutSeconds)
{
    if (scenarioName.empty()) {
        std::cerr << "Scenario name is required: [pause|forward-seek|timestamp-continuity]\n";
        return false;
    }

    if (videoPath.empty()) {
        std::cerr << "Video path is required for scenario: " << scenarioName << '\n';
        return false;
    }

    const std::string scenario_name = scenarioName;
    const std::filesystem::path video_path_obj(videoPath);
    const char* scenario = scenario_content(scenario_name);

    if (!scenario) {
        std::cerr << "Usage: [pause|forward-seek|timestamp-continuity] [video.mp4] [report.json]\n";
        return false;
    }
    if (!std::filesystem::exists(video_path_obj)) {
        std::cerr << "Video file does not exist: " << video_path_obj << '\n';
        return false;
    }


    gst_init(nullptr, nullptr);
    gst_validate_init();
    const std::string absolute_path = std::filesystem::absolute(video_path_obj).lexically_normal().string();
    GstElement* pipeline_element = nullptr;
    try {
        PipelineManagementLayer pipelineManagementLayer;
        GError* parse_error = nullptr;
        pipeline_element = pipelineManagementLayer.createScenarioGstPipeline(
            configuration, absolute_path, &parse_error);
        if (!pipeline_element) {
            std::cerr << "Could not create pipeline: "
                      << (parse_error ? parse_error->message : "unknown") << '\n';
            g_clear_error(&parse_error);
            gst_validate_deinit();
            return false;
        }
        g_clear_error(&parse_error);
    } catch (const std::exception& error) {
        std::cerr << "Could not create scenario pipeline: " << error.what() << '\n';
        gst_validate_deinit();
        return false;
    }
    const std::filesystem::path scenario_path = std::filesystem::temp_directory_path() /
        ("gst_validate_video_" + scenario_name + ".scenario");
    std::ofstream scenario_file(scenario_path);
    if (!scenario_file) {
        std::cerr << "Could not create scenario file: " << scenario_path << '\n';
        gst_validate_deinit();
        return false;
    }
    scenario_file << scenario;
    scenario_file.close();
    GstValidateRunner* runner = gst_validate_runner_new();
    GstValidateMonitor* monitor = gst_validate_monitor_factory_create(GST_OBJECT(pipeline_element), runner, nullptr);
    if (!runner || !monitor) {
        std::cerr << "Could not create GStreamer Validate monitor.\n";
        if (monitor) gst_object_unref(monitor);
        if (runner) gst_object_unref(runner);
        gst_object_unref(pipeline_element);
        gst_validate_deinit();
        return false;
    }
    GstValidateScenario* validate_scenario = gst_validate_scenario_factory_create(runner, pipeline_element, scenario_path.c_str());
    if (!validate_scenario) {
        std::cerr << "Could not create scenario from " << scenario_path << '\n';
        gst_object_unref(monitor);
        gst_object_unref(runner);
        gst_object_unref(pipeline_element);
        gst_validate_deinit();
        return false;
    }

    
    std::cout << "Running supported GStreamer Validate API scenario '" << scenario_name << "' on " << absolute_path << '\n';
    const auto run_started = std::chrono::system_clock::now();
    const auto steady_started = std::chrono::steady_clock::now();
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
    RunContext context{loop, pipeline_element, false, -1, -1, false, false, false,
                       GST_CLOCK_TIME_NONE, GST_CLOCK_TIME_NONE, 0, 0, 0, 0, 0,
                       {}, {}, {}, 0, false};
    if (scenario_name == "timestamp-continuity") {
        GstElement* output = gst_bin_get_by_name(GST_BIN(pipeline_element), "timestamp_output");
        GstPad* probe_pad = output ? gst_element_get_static_pad(output, "src") : nullptr;
        if (!probe_pad) {
            std::cerr << "Could not attach timestamp continuity probe to timestamp_output.\n";
            if (output) gst_object_unref(output);
            g_main_loop_unref(loop);
            gst_object_unref(validate_scenario);
            gst_object_unref(monitor);
            gst_object_unref(runner);
            gst_object_unref(pipeline_element);
            gst_validate_deinit();
            return false;
        }
        gst_pad_add_probe(probe_pad, static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM), inspect_timestamps, &context, nullptr);
        gst_object_unref(probe_pad);
        gst_object_unref(output);
    }
    gst_debug_set_default_threshold(GST_LEVEL_WARNING);
    gst_debug_add_log_function(capture_gstreamer_log, &context, nullptr);
    GstBus* bus = gst_element_get_bus(pipeline_element);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(handle_bus_message), &context);
    const guint position_source_id = g_timeout_add(250, sample_pipeline_position, &context);
    context.timeout_source_id = g_timeout_add_seconds(eosTimeoutSeconds == 0 ? 1 : eosTimeoutSeconds, stop_scenario_on_timeout, &context);
    if (gst_element_set_state(pipeline_element, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        context.failed = true;
        g_main_loop_quit(loop);
    }
    g_main_loop_run(loop);
    gst_element_set_state(pipeline_element, GST_STATE_NULL);
    const auto run_finished = std::chrono::system_clock::now();
    const double elapsed_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - steady_started).count();
    const auto to_seconds = [](gint64 time) { return time < 0 ? 0.0 : static_cast<double>(time) / GST_SECOND; };
    const std::filesystem::path report_path = std::filesystem::temp_directory_path() / ("validate-report-" + scenario_name + ".json");
    const bool run_failed = context.failed || (expectTimestampAnomaly != context.timestamp_failed);
        // A mismatch in anomaly expectation is a test failure even when GStreamer itself ran cleanly.
        const bool run_failed = context.failed || (expectTimestampAnomaly != context.timestamp_failed);
    write_report(report_path, runner, context, scenario_name, absolute_path, context.failed,
                 utc_timestamp(run_started), utc_timestamp(run_finished), elapsed_seconds,
                 to_seconds(context.media_duration), to_seconds(context.final_position));
    gst_debug_remove_log_function_by_data(&context);
    if (position_source_id != 0) g_source_remove(position_source_id);
    if (context.timeout_source_id != 0) g_source_remove(context.timeout_source_id);
    gst_bus_remove_signal_watch(bus);
    gst_object_unref(bus);
    std::cout << "Validation report written to " << report_path << '\n';
    g_main_loop_unref(loop);
    gst_object_unref(validate_scenario);
    gst_object_unref(monitor);
    gst_object_unref(runner);
    gst_object_unref(pipeline_element);
    gst_validate_deinit();
    std::error_code scenario_remove_error;
    std::filesystem::remove(scenario_path, scenario_remove_error);
    return !run_failed;
}
