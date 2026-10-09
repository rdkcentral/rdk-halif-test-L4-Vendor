#[[ File: CMakeLists.txt
	Purpose: Configure shared dependencies and install data for the RDK E AV suite. ]]
cmake_minimum_required(VERSION 3.10)
project(L4-rdk-audio-video VERSION 1.0 LANGUAGES C CXX)

include(GNUInstallDirs)

set(L4_CONFIG_INSTALL_PATH "/etc/rdk/ut-core/platform.ini" CACHE FILEPATH
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
target_compile_definitions(ut_core PRIVATE UT_VERSION="yocto-${PROJECT_VERSION}")
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
add_library(L4-rdk-audio-video_dependencies INTERFACE)
target_include_directories(L4-rdk-audio-video_dependencies INTERFACE
	${GST_INCLUDE_DIRS}
	${GST_VALIDATE_INCLUDE_DIRS}
	${FFMPEG_INCLUDE_DIRS}
	${CURL_INCLUDE_DIRS}
	${LIBFYAML_INCLUDE_DIRS}
)

target_link_directories(L4-rdk-audio-video_dependencies INTERFACE
	${GST_LIBRARY_DIRS}
	${GST_VALIDATE_LIBRARY_DIRS}
	${FFMPEG_LIBRARY_DIRS}
	${CURL_LIBRARY_DIRS}
	${LIBFYAML_LIBRARY_DIRS}
)

target_link_libraries(L4-rdk-audio-video_dependencies INTERFACE
	${GST_LIBRARIES}
	${GST_VALIDATE_LIBRARIES}
	${FFMPEG_LIBRARIES}
	${CURL_LIBRARIES}
	${LIBFYAML_LIBRARIES}
)

add_subdirectory(rdk-audio-video-testframework)

install(FILES config/platform.ini
	DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rdk/ut-core
)

install(DIRECTORY video-inputs/
	DESTINATION ${CMAKE_INSTALL_DATADIR}/rdk/L4-rdk-audio-video/video-inputs
)
