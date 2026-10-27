# rdk-halif-test-L4-Vendor: Build Steps

## 1. Runtime Configuration

The default configuration is:

```text
/etc/rdk/ut-core/platform.ini
```

A custom configuration can be selected for the public scenario methods with:

```sh
export L4_CONFIG=/tmp/platform.ini
```

The parser accepts `key=value` lines and ignores blank lines, comments beginning with `#` or `;`, and section headers such as `[platform]`.

### Configuration Model

`L4Configuration` contains:

| Group | Keys |
| --- | --- |
| Platform | `platform` |
| General media | `test_url`, `pause_video`, `forward_seek_video`, `timestamp_gap_video`, `timestamp_negative_jump_video` |
| Playbin elements | `source_element`, `audio_sink`, `video_sink`, `audio_filter`, `video_filter` |
| Scenario elements | `scenario_source_element`, `mp4_demux_element`, `ts_demux_element`, `mp4_demux_link`, `ts_demux_link`, `h264_parser_element`, `video_convert_element`, `decoder_element` |
| Playback | `volume`, `mute` |
| Validation | `eos_timeout_seconds`, `allowed_frame_drops`, `av_sync_tolerance_ms` |

## 2. Build and Deployment

### Build

```sh
cd /path/to/yocto/workspace
export PATH=/path/to/repo-tool/bin:$PATH
MACHINE=apache-4k
source scripts/setup-environment
bitbake -c compile -f lib32-l4-rdk-audio-video
bitbake -c install -f lib32-l4-rdk-audio-video
```

Build the selected image after changing the recipe, FFmpeg configuration, platform implementation, or runtime assets.

### Device Verification

```sh
ls -l /usr/bin/rdkgstreamerplugin/l4test/l4-rdk-audio-video
ls -lh /usr/share/rdk/l4-rdk-audio-video/video-inputs/
ls -l /etc/rdk/ut-core/platform.ini
```

If copying a standalone binary:

```sh
scp -P 10022 l4-rdk-audio-video root@DEVICE_IP:/tmp/
ssh -p 10022 root@DEVICE_IP
chmod +x /tmp/l4-rdk-audio-video
/tmp/l4-rdk-audio-video
```

If `/tmp` is mounted `noexec`, run the binary from an executable filesystem.

## 3. Adding a New Test Group

Create a sibling directory:

```text
pipeline_validation/
|-- CMakeLists.txt
|-- include/
`-- src/
```

Register it from the parent:

```cmake
add_subdirectory(pipeline_validation)
```

The child CMake should own its sources and executable, and link shared targets:

```cmake
target_link_libraries(new_test_target PRIVATE
    ut_core
    l4-rdk-audio-video_dependencies
)
```

Keep these concerns in the parent:

- Common dependency discovery.
- UT Core and UT Control integration.
- Shared install data.
- Shared package-level configuration.

Keep these concerns in each child:

- Test sources.
- Test-specific headers.
- Test-specific compile definitions.
- Test executable installation.
