/** @file vts_media_framework.cpp
 * @brief Executes GstValidate scenarios and evaluates pass/fail results.
 */
#include "vts_audio_video.h"
#include <ut.h>
#include <ut_log.h>

#include "vts_configuration.h"
#include "vts_pipeline_management.h"
#include "vts_verification.h"
#include <gst/gst.h>
#include <gst/validate/validate.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace {
/** Selects the environment override or the configured installation path. */
std::string configurationPath()
{
    const char* path = std::getenv("L4_CONFIG");
    return path ? path : L4_DEFAULT_CONFIG_PATH;
}
}

/** Initializes the common scenario runner. */
RDKAudioVideo::RDKAudioVideo()

{
}

/** Emits the UT smoke-test message. */
void RDKAudioVideo::helloWorld()
{
    UT_LOG("Hello world");
}


/** Loads runtime settings and runs the pause/resume scenario. */
bool RDKAudioVideo::runPauseValidationScenario()
{
    try {
        const L4Configuration configuration =
            ConfigurationLayer::load(configurationPath());
        return runScenario(configuration, "pause", configuration.pauseVideo);
    } catch (const std::exception& error) {
        UT_LOG_ERROR("Pause scenario configuration failed: %s", error.what());
        return false;
    }
}

/** Loads runtime settings and runs the forward-seek scenario. */
bool RDKAudioVideo::runForwardSeekValidationScenario()
{
    try {
        const L4Configuration configuration =
            ConfigurationLayer::load(configurationPath());
        return runScenario(configuration, "forward-seek", configuration.forwardSeekVideo);
    } catch (const std::exception& error) {
        UT_LOG_ERROR("Forward-seek scenario configuration failed: %s", error.what());
        return false;
    }
}

namespace {

/** Returns the GstValidate action script associated with a supported scenario name. */
const char* scenario_content(const std::string& scenario_name)
{
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

/** GstValidate rejects runner creation after any GstElement exists, so the runner is created here once. */
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

/** Removes the temporary scenario file when the run leaves scope. */
struct ScopedFileRemover {
    std::filesystem::path path;

    ~ScopedFileRemover()
    {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
};

struct RunContext {
    GMainLoop* loop;
    GstElement* pipeline;
    bool failed;
};

/** Converts pipeline bus messages into run status and main-loop termination. */
gboolean handle_bus_message(GstBus*, GstMessage* message, gpointer data)
{
    auto* context = static_cast<RunContext*>(data);
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
        GError* error = nullptr;
        gchar* debug = nullptr;
        gst_message_parse_error(message, &error, &debug);
        UT_LOG_ERROR("Pipeline error: %s", error ? error->message : "unknown");
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
        if (new_state == GST_STATE_NULL) {
            g_main_loop_quit(context->loop);
        }
    }
    return G_SOURCE_CONTINUE;
}


} // namespace

/** Builds, runs, evaluates, and releases one configured GstValidate scenario. */
bool RDKAudioVideo::runScenario(const L4Configuration& configuration,
                                         const std::string& scenarioName,
                                         const std::string& fileUri)
{
    if (scenarioName.empty()) {
        UT_LOG_ERROR("Scenario name is required: [pause|forward-seek]");
        return false;
    }

    if (fileUri.empty()) {
        UT_LOG_ERROR("Video path is required for scenario: %s", scenarioName.c_str());
        return false;
    }

    const std::string scenario_name = scenarioName;
    const std::filesystem::path video_path_obj(fileUri);
    const char* scenario = scenario_content(scenario_name);

    if (!scenario) {
        UT_LOG_ERROR("Usage: [pause|forward-seek]");
        return false;
    }
    if (!std::filesystem::exists(video_path_obj)) {
        UT_LOG_ERROR("Video file does not exist: %s", video_path_obj.c_str());
        return false;
    }


    GstValidateRunner* runner = ensureGstValidateInitialized();
    if (!runner) {
        UT_LOG_ERROR("Could not create GStreamer Validate runner.");
        return false;
    }
    const std::string absolute_path = std::filesystem::absolute(video_path_obj).lexically_normal().string();
    GError* parse_error = nullptr;
    GstObjectPtr<GstElement> pipeline;
    try {
        PipelineManagementLayer pipelineManagementLayer;
        pipeline.reset(pipelineManagementLayer.createScenarioGstPipeline(
            configuration, absolute_path, &parse_error));
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
    const std::filesystem::path scenario_path = std::filesystem::temp_directory_path() /
        ("gst_validate_video_" + scenario_name + ".scenario");
    ScopedFileRemover scenario_file_remover{scenario_path};
    {
        std::ofstream scenario_file(scenario_path);
        if (!scenario_file) {
            UT_LOG_ERROR("Could not create scenario file: %s", scenario_path.c_str());
            return false;
        }
        scenario_file << scenario;
    }
    GstObjectPtr<GstValidateMonitor> monitor(
        gst_validate_monitor_factory_create(GST_OBJECT(pipeline.get()), runner, nullptr));
    if (!monitor) {
        UT_LOG_ERROR("Could not create GStreamer Validate monitor.");
        return false;
    }
    GstObjectPtr<GstValidateScenario> validate_scenario(
        gst_validate_scenario_factory_create(runner, pipeline.get(), scenario_path.c_str()));
    if (!validate_scenario) {
        UT_LOG_ERROR("Could not create scenario from %s", scenario_path.c_str());
        return false;
    }

    
    UT_LOG("Running supported GStreamer Validate API scenario '%s' on %s", scenario_name.c_str(), absolute_path.c_str());
    MainLoopPtr loop(g_main_loop_new(nullptr, FALSE));
    RunContext context{loop.get(), pipeline.get(), false};
    GstObjectPtr<GstBus> bus(gst_element_get_bus(pipeline.get()));
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(handle_bus_message), &context);
    if (gst_element_set_state(pipeline.get(), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        context.failed = true;
    } else {
        g_main_loop_run(loop.get());
    }
    gst_element_set_state(pipeline.get(), GST_STATE_NULL);
    const int gst_validate_exit_code = gst_validate_runner_exit(runner, TRUE);
    const bool run_failed = context.failed || gst_validate_exit_code != 0;
    gst_bus_remove_signal_watch(bus.get());
    return !run_failed;
}
