# rdk-halif-test-L4-Vendor: Dependencies

Local copies of dependency metadata available in this checkout are under `../meta-layers/`. This directory does not include dependencies fetched from other layers or external sources. The copied FFmpeg append references six CVE patch files that are not present in this checkout.

## 1. Build Design

### 1.1 Parent CMake

The root `CMakeLists.txt` owns shared build concerns:

- CMake project and install directories.
- Threads and pkg-config discovery.
- GStreamer and GstValidate discovery.
- FFmpeg/libavformat, libavcodec, and libavutil discovery.
- CURL and libfyaml discovery.
- GoogleTest and GoogleMock discovery.
- The `ut_core` static library.
- The shared `l4-rdk-audio-video_dependencies` interface target.
- Installation of `platform.ini` and the `video-inputs/` directory.
- Registration of child test folders.

The parent receives these paths from BitBake:

```text
UT_CORE_DIR
UT_CONTROL_DIR
L4_CONFIG_INSTALL_PATH
```

### 1.2 Test Framework Child CMake

`rdk-audio-video-testframework/CMakeLists.txt` owns:

- Common audio/video test sources.
- The selected platform source.
- The `l4-rdk-audio-video` executable.
- Test-specific include paths and compile definitions.
- Linking to `ut_core` and `l4-rdk-audio-video_dependencies`.
- Installation of the executable.

The child does not rediscover shared libraries. This keeps future child projects consistent and avoids duplicating dependency logic.

### 1.3 UT Core Source Integration

The parent builds UT sources fetched by the Yocto recipe:

```text
UT_CORE_DIR/src/cpp_source/ut_gtest.cpp
UT_CORE_DIR/src/ut_main.c
UT_CORE_DIR/src/ut_kvp_profile.c
UT_CONTROL_DIR/src/ut_log.c
UT_CONTROL_DIR/src/ut_kvp.c
```

The C/C++ language split is intentional:

- `ut_gtest.cpp`, `ut_main.c`, `ut_kvp_profile.c`, and `ut_log.c` are compiled as C++ for the GTest-based UT integration.
- `ut_kvp.c` remains C because its implementation uses C allocation semantics.

## 2. Yocto Integration

The recipe is `l4-rdk-audio-video.bb`.

### Source Inputs

The recipe fetches:

```text
https://github.com/rdkcentral/ut-core.git
https://github.com/rdkcentral/ut-control.git
```

Both use the `develop` branch and currently use `${AUTOREV}`. Pin `SRCREV_utcore` and `SRCREV_utcontrol` for reproducible release builds.

The local source directory is:

```bitbake
S = "${WORKDIR}/l4-rdk-audio-video"
```

### Build Dependencies

The recipe builds against:

- GoogleTest.
- pkg-config native support.
- CURL.
- libfyaml.
- FFmpeg.
- GStreamer 1.18.5.
- GstValidate through `gst-devtools` 1.18.5.

To enable local file input, add `--enable-protocol=file` to the `EXTRA_OECONF:append` options in [`meta-layers/meta-rdk-oss-reference/recipes-multimedia/ffmpeg/ffmpeg_4.2.%.bbappend`](../meta-layers/meta-rdk-oss-reference/recipes-multimedia/ffmpeg/ffmpeg_4.2.%.bbappend).
Just an example use this as referance.

The FFmpeg runtime package is also included because `VideoCodecDetector` calls libavformat at runtime:

```bitbake
RDEPENDS:${PN} = " \\
    ffmpeg \\
    ...
"
```

### Installed Files

The package installs:

```text
/usr/bin/rdkgstreamerplugin/l4test/l4-rdk-audio-video
/etc/rdk/ut-core/platform.ini
/usr/share/rdk/l4-rdk-audio-video/video-inputs/*
```
