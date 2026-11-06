# L4 AV VTS Test Framework

The AV VTS Test Framework is composed of the following:
- UT Core-based Test Framework
- Platform agnostic architecture
- Reusable framework for all test cases
- GstValidate-based pipeline validation
- YAML driven configuration
- Timestamp anomaly detection

### Features

#### Platform Abstraction
- Platform is selected through YAML configuration
- No platform specific code change required in Framework
- Common testcase definitions

#### GstValidate Monitors & Scenarios

- **GstValidateRunner:** Orchestrates test execution, manages test lifecycle, and aggregates validation results and reports.
- **GstValidateScenario:** Executes validation scenarios such as:
  - Playback
  - Seek
  - Pause
  - Flush
  - EOS handling
- **GstValidateMonitor:** Monitors pipelines, elements, pads, buffers and events.

#### GST-Validate Integration

GST-Validate is integrated into the AV Test Framework to perform automated validation of GStreamer pipeline behavior during test execution. The framework leverages GST-Validate monitors and validation infrastructure to detect media pipeline anomalies, protocol violations, synchronization issues, and playback errors without requiring explicit validation logic within individual test cases.

## End-to-End Architecture

```mermaid
flowchart LR
    UTF["UT Core Framework"]
    CTF["Common Test<br/>Framework<br/>Module"]
    Report["Test Report"]

    
    UTF <-->|Executes and<br/>returns result| CTF
    UTF --> Report

    subgraph launcher["Gst-Validate Launcher"]
        Checks["• Playback check<br/>• seek check<br/>• End-of-stream - EOS handling<br/>• Pause/Play transitions<br/>• Codec switching<br/>• Underflow check<br/>• Playback rate<br/>• Resolution and framerate check<br/>• Track switching"] 

        subgraph av["AV Pipeline"]
            direction LR
            AVS["AV source"] --> DM["Demuxer"]
            DM --> AVD["rdkviddec"] --> AVK["rdkvidsink"]
            DM --> AAD["rdkauddec"] --> AAK["rdkaudsink"]
        end

        subgraph audio["Audio Pipeline"]
            direction LR
            AS["Audio source"] --> AD["rdkauddec"] --> AK["rdkaudsink"]
        end

        subgraph video["Video Pipeline"]
            direction LR
            VS["Video source"] --> VD["rdkviddec"] --> VK["rdkvidsink"]
        end

        Checks --> VS
        Checks --> AS
        Checks --> AVS
    end

    CTF --- Checks

    classDef framework fill:#f4f4f4,stroke:#666,color:#222;
    classDef report fill:#d9ead3,stroke:#7cad6d,color:#222;
    classDef checks fill:#d7e6f8,stroke:#6f8eae,color:#222;
    classDef pipeline fill:#f5f5f5,stroke:#777,color:#222;
    class UTF,CTF framework;
    class Report report;
    class Checks checks;
    class VS,VD,VK,AS,AD,AK,AVS,DM,AVD,AVK,AAD,AAK pipeline;
    style launcher fill:#fff3cc,stroke:#c9aa52,stroke-width:1px;
    style video fill:#e5d7e8,stroke:#9474a0,stroke-width:1px;
    style audio fill:#e5d7e8,stroke:#9474a0,stroke-width:1px;
    style av fill:#e5d7e8,stroke:#9474a0,stroke-width:1px;
```

## HLD - Test Framework

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

## End-to-End Execution Flow

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


