# rdk-halif-test-L4-Vendor: Overview

## 1. Overview

`l4-rdk-audio-video` is a Yocto-built RDK test package for validating GStreamer video pipelines on platform-specific devices.

The package combines:

- UT Core test registration and execution.
- UT Control logging/KVP support.
- INI-based runtime configuration.
- FFmpeg/libavformat codec detection.
- Platform-specific pipeline construction.
- GStreamer and GstValidate scenario execution.
- Timestamp anomaly detection.
- JSON runtime reports.

The current target executable is:

```text
/usr/bin/rdkgstreamerplugin/l4test/l4-rdk-audio-video
```

The design is intended to support additional test groups and platform implementations without moving common dependency or package logic into each test folder.

## 2. Design Goals

The framework is organized around these goals:

1. Keep test registration separate from pipeline construction.
2. Keep platform-specific pipeline decisions outside the common test code.
3. Load runtime paths, element names, sinks, and thresholds from `platform.ini`.
4. Detect the media codec when required by the selected platform pipeline.
5. Reuse the common GstValidate execution and reporting flow.
6. Allow future test folders to be added through child CMake projects.
7. Keep Yocto packaging responsible for supplying the executable, configuration, libraries, and video assets.

## 3. High-Level Architecture

```mermaid
flowchart LR
    subgraph configuration_layer["Configuration Layer"]
        config_file["Platform Configuration File<br/>(JSON/YAML/INI)"]
        config_values["Test URL<br/>Audio Sink<br/>Video Sink<br/><br/>AV Sync Threshold<br/>Allowed Frame Drops<br/>Consecutive Frame Drops<br/><br/>Platform Type<br/>Decoder Type<br/><br/>Logging Options"]
        config_file --- config_values
    end

    subgraph l4_suite["L4 GStreamer Test Suite"]
        ut_core["UT-Core Framework"] -->|Execute Tests| wrapper["Wrapper Test Module<br/>(Test Cases)"]
        wrapper -->|Run Test Case| common["Common Test Framework"]
    end

    platform_header["Platform Specific Implementations"]

    subgraph verification_layer["Verification Layer"]
        validation_manager["Validation Manager"] --> validation_interface(("IValidationInterface"))
        verification_examples["Verification Examples:<br/><br/>&bull; Playback Start<br/>&bull; EOS Handling<br/>&bull; Pause/Resume<br/><br/>&bull; AV Sync Validation<br/>&bull; Audio Quality<br/>&bull; Video Frame Drop Check<br/><br/>&bull; Decoder State<br/>&bull; Buffer Health<br/>&bull; Resource Monitoring"]
        validation_manager -.-> verification_examples
    end

    subgraph pipeline_layer["Pipeline Management Layer"]
        pipeline_interface(("IPipelineManager"))
        pipeline_builder["Playbin Pipeline Builder"] --> playbin(("Playbin"))
        playbin --> audio_sink["Audio Sink"]
        playbin --> video_sink["Video Sink"]
        broadcom["Broadcom Implementation"]
        amlogic["Amlogic Implementation"]
        realtek["Realtek Implementation"]
    end

    config_file -->|Read Runtime Parameters| common
    common -->|Verification Requests| validation_manager
    common -->|Create Pipeline| pipeline_builder
    broadcom -.-> pipeline_interface
    amlogic -.-> pipeline_interface
    realtek -.-> pipeline_interface
    broadcom -.-> validation_interface
    amlogic -.-> validation_interface
    realtek -.-> validation_interface
```

## 4. End-to-End Execution Flow

```mermaid
flowchart TD
    start(( )) --> ut_execute["UT-Core Executes Test"]
    ut_execute --> select_case["Wrapper Test Module Selects Test Case"]
    select_case --> common_framework["Common Test Framework"]
    common_framework --> load_configuration["Load Platform Configuration"]
    configuration_values["Read:<br/>- URL<br/>- AudioSink<br/>- VideoSink<br/>- AV Sync Threshold<br/>- Frame Drop Limits<br/>- Platform Type"]
    load_configuration --- configuration_values
    load_configuration --> create_pipeline["Create Playbin Pipeline"]
    create_pipeline --> configure_audio["Configure Audio Sink"]
    configure_audio --> configure_video["Configure Video Sink"]
    configure_video --> set_playing["Set Pipeline To PLAYING"]
    set_playing --> test_running{"Test Running?"}

    test_running --> collect_metrics["Collect Runtime Metrics"]
    collect_metrics --> invoke_verification["Invoke Common Verification Interface"]
    invoke_verification --> platform_check{"Platform Check Required?"}
    platform_check -->|Yes| platform_implementation["Platform Specific Implementation"]
    platform_implementation --> perform_validation["Perform Validation"]
    platform_check -->|No| validation_merge{" "}
    perform_validation --> validation_merge
    validation_merge --> test_running

    test_running --> compare_thresholds["Compare Against Thresholds"]
    compare_thresholds --> pass_check{"Pass?"}
    pass_check -->|PASS| pass_report["Generate PASS Report"]
    pass_check -->|FAIL| fail_report["Generate FAIL Report"]
    pass_report --> report_merge{" "}
    fail_report --> report_merge
    report_merge --> finish((( )))
```

## 5. Folder Structure

```text
l4-rdk-audio-video/
|-- CMakeLists.txt
|-- l4-rdk-audio-video.bb
|-- README.md
|-- audio-inputs/
|-- meta-layers/
|   |-- meta-rdk-oss-reference/
|   |   `-- recipes-multimedia/ffmpeg/ffmpeg_4.2.%.bbappend
|   `-- meta-rdk-vendor-test-utils/
|       |-- recipes-multimedia/gstreamer/gst-devtools_1.18.5.bb
|       `-- recipes-support/libfyaml/
|           |-- libfyaml_git.bb
|           `-- libfyaml/0001-allocator-durable-skip-pointer-alignment-on-32bit.patch
|-- config/
|   `-- platform.ini
|-- docs/
|   |-- README_Overview.md
|   |-- README_Dependencies.md
|   `-- README_Build_Steps.md
|-- rdk-audio-video-testframework/
|   |-- CMakeLists.txt
|   |-- rdk-AV-UT.cpp
|   |-- include/
|   |   |-- l4-rdk-audio-video.h
|   |   |-- l4_configuration.h
|   |   |-- pipeline_management_interface.h
|   |   |-- pipeline_management_layer.h
|   |   `-- verification_layer.h
|   |-- platforms/
|   |   |-- Amlogic/
|   |   |   `-- src/platform_pipeline_management.cpp
|   |   |-- Broadcom/
|   |   |   `-- src/platform_pipeline_management.cpp
|   |   `-- Realtek/
|   |       `-- src/platform_pipeline_management.cpp
|   `-- src/
|       |-- common_test_framework.cpp
|       |-- configuration_layer.cpp
|       |-- l4-rdk-audio-video.cpp
|       |-- pipeline_management_layer.cpp
|       `-- verification_layer.cpp
`-- video-inputs/
    |-- BigBuckBunny.mp4
    |-- ffmpeg-defective-test.mp4
    |-- negative-jump-test.ts
    |-- duplicate-timestamp-test.ts
    |-- timestamp-continuity-defective.ts
    `-- bigbuck-clean-20s.mp4
```

The parent CMake file is the common build entry point. The `rdk-audio-video-testframework` child CMake file owns the current test executable. Future test groups can be added as sibling folders and registered from the parent with `add_subdirectory(...)`.
