#[[ File: CMakeLists.txt
	Purpose: Configure shared dependencies and install data for the l4-rdk-audio-video project. ]]
cmake_minimum_required(VERSION 3.18)
project(l4-rdk-audio-video VERSION 1.0 LANGUAGES C CXX)

include(GNUInstallDirs)

set(L4_CONFIG_INSTALL_PATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}/rdk/ut-core/platform.yaml" CACHE FILEPATH
	"Installed path of the default test configuration")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
	gstreamer-1.0
	gstreamer-audio-1.0
	gstreamer-app-1.0
)
pkg_check_modules(GST_VALIDATE REQUIRED gst-validate-1.0)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(LIBFYAML REQUIRED libfyaml)

find_path(GTEST_INCLUDE_DIR gtest/gtest.h REQUIRED)
find_path(GMOCK_INCLUDE_DIR gmock/gmock.h REQUIRED)
find_library(GTEST_LIBRARY gtest REQUIRED)
find_library(GMOCK_LIBRARY gmock REQUIRED)

# UT Core sources are built here so each child target shares one integration library.
add_library(ut_core STATIC
	${UT_CORE_DIR}/src/cpp_source/ut_gtest.cpp
	${UT_CORE_DIR}/src/ut_main.c
	${UT_CORE_DIR}/src/ut_kvp_profile.c
	${UT_CONTROL_DIR}/src/ut_log.c
	${UT_CONTROL_DIR}/src/ut_kvp.c
)

# These C sources use C++ compilation for the UT GoogleTest integration.
set_source_files_properties(
	${UT_CORE_DIR}/src/ut_main.c
	${UT_CORE_DIR}/src/ut_kvp_profile.c
	${UT_CONTROL_DIR}/src/ut_log.c
	PROPERTIES LANGUAGE CXX
)

target_compile_features(ut_core PUBLIC cxx_std_17)
target_compile_definitions(ut_core PRIVATE UT_VERSION="${PROJECT_VERSION}")
target_include_directories(ut_core PUBLIC
	${UT_CORE_DIR}/include
	${UT_CORE_DIR}/src
	${UT_CORE_DIR}/src/cpp_source
	${UT_CONTROL_DIR}/include
	${CURL_INCLUDE_DIRS}
	${LIBFYAML_INCLUDE_DIRS}
	${GTEST_INCLUDE_DIR}
	${GMOCK_INCLUDE_DIR}
)
target_link_libraries(ut_core PUBLIC
	${GMOCK_LIBRARY}
	${GTEST_LIBRARY}
	Threads::Threads
)

# Keep multimedia and utility dependencies centralized for child test targets.
add_library(l4-rdk-audio-video_dependencies INTERFACE)
target_include_directories(l4-rdk-audio-video_dependencies INTERFACE
	${GST_INCLUDE_DIRS}
	${GST_VALIDATE_INCLUDE_DIRS}
	${FFMPEG_INCLUDE_DIRS}
	${CURL_INCLUDE_DIRS}
	${LIBFYAML_INCLUDE_DIRS}
)

target_link_directories(l4-rdk-audio-video_dependencies INTERFACE
	${GST_LIBRARY_DIRS}
	${GST_VALIDATE_LIBRARY_DIRS}
	${FFMPEG_LIBRARY_DIRS}
	${CURL_LIBRARY_DIRS}
	${LIBFYAML_LIBRARY_DIRS}
)

target_link_libraries(l4-rdk-audio-video_dependencies INTERFACE
	${GST_LIBRARIES}
	${GST_VALIDATE_LIBRARIES}
	${FFMPEG_LIBRARIES}
	${CURL_LIBRARIES}
	${LIBFYAML_LIBRARIES}
)

# Select exactly one platform implementation, matching the runtime YAML selection.
set(PLATFORM_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config/platform.yaml")
file(STRINGS "${PLATFORM_CONFIG_FILE}" PLATFORM_CONFIG_LINE
	REGEX "^[ \t]*platform[ \t]*:")
if(NOT PLATFORM_CONFIG_LINE)
	message(FATAL_ERROR "No platform: entry found in ${PLATFORM_CONFIG_FILE}")
endif()
string(REGEX REPLACE "^[ \t]*platform[ \t]*:[ \t]*" "" L4_PLATFORM_NAME
	"${PLATFORM_CONFIG_LINE}")
string(STRIP "${L4_PLATFORM_NAME}" L4_PLATFORM_NAME)
string(TOLOWER "${L4_PLATFORM_NAME}" L4_PLATFORM_NAME_LOWER)

if(L4_PLATFORM_NAME_LOWER MATCHES "amlogic")
	set(PLATFORM_PIPELINE_SOURCE src/vts_amlogic_pipeline_management.cpp)
	set(L4_COMPILED_PLATFORM "amlogic")
elseif(L4_PLATFORM_NAME_LOWER MATCHES "broadcom|broadcome")
	set(PLATFORM_PIPELINE_SOURCE src/vts_broadcom_pipeline_management.cpp)
	set(L4_COMPILED_PLATFORM "broadcom")
elseif(L4_PLATFORM_NAME_LOWER MATCHES "realtek")
	set(PLATFORM_PIPELINE_SOURCE src/vts_realtek_pipeline_management.cpp)
	set(L4_COMPILED_PLATFORM "realtek")
else()
	message(FATAL_ERROR "Unsupported platform '${L4_PLATFORM_NAME}' in ${PLATFORM_CONFIG_FILE}")
endif()
message(STATUS "Building L4 pipeline implementation for ${L4_COMPILED_PLATFORM}")

# The common harness and selected platform source make one executable.
set(AUDIO_DECODER_SOURCES
	src/vts_main.cpp
	src/vts_media_framework.cpp
	src/vts_configuration.cpp
	src/vts_audio_video.cpp
	src/vts_pipeline_management.cpp
	src/vts_verification.cpp
	${PLATFORM_PIPELINE_SOURCE}
)

add_executable(l4-rdk-audio-video ${AUDIO_DECODER_SOURCES})

target_compile_features(l4-rdk-audio-video PRIVATE cxx_std_17)
target_compile_definitions(l4-rdk-audio-video PRIVATE
	L4_DEFAULT_CONFIG_PATH="${L4_CONFIG_INSTALL_PATH}"
)
target_include_directories(l4-rdk-audio-video PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(l4-rdk-audio-video PRIVATE
	ut_core
	l4-rdk-audio-video_dependencies
)

install(TARGETS l4-rdk-audio-video
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/rdkgstreamerplugin/l4test
)

install(FILES config/platform.yaml
	DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rdk/ut-core
)

install(DIRECTORY video-inputs/
	DESTINATION ${CMAKE_INSTALL_DATADIR}/rdk/l4-rdk-audio-video/video-inputs
)
