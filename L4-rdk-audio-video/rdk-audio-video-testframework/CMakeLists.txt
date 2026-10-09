#[[ File: rdk-audio-video-testframework/CMakeLists.txt
    Purpose: Select the platform implementation and build the L4 executable. ]]
set(PLATFORM_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../config/platform.ini")
file(STRINGS "${PLATFORM_CONFIG_FILE}" PLATFORM_CONFIG_LINE
    REGEX "^[ \t]*platform[ \t]*=")
if(NOT PLATFORM_CONFIG_LINE)
    message(FATAL_ERROR "No platform= entry found in ${PLATFORM_CONFIG_FILE}")
endif()
string(REGEX REPLACE "^[ \t]*platform[ \t]*=[ \t]*" "" L4_PLATFORM_NAME
    "${PLATFORM_CONFIG_LINE}")
string(STRIP "${L4_PLATFORM_NAME}" L4_PLATFORM_NAME)
string(TOLOWER "${L4_PLATFORM_NAME}" L4_PLATFORM_NAME_LOWER)

# Compile exactly one platform implementation, matching the runtime INI selection.
if(L4_PLATFORM_NAME_LOWER MATCHES "amlogic")
    set(PLATFORM_PIPELINE_SOURCE platforms/Amlogic/src/platform_pipeline_management.cpp)
    set(L4_COMPILED_PLATFORM "amlogic")
elseif(L4_PLATFORM_NAME_LOWER MATCHES "broadcom|broadcome")
    set(PLATFORM_PIPELINE_SOURCE platforms/Broadcom/src/platform_pipeline_management.cpp)
    set(L4_COMPILED_PLATFORM "broadcom")
elseif(L4_PLATFORM_NAME_LOWER MATCHES "realtek")
    set(PLATFORM_PIPELINE_SOURCE platforms/Realtek/src/platform_pipeline_management.cpp)
    set(L4_COMPILED_PLATFORM "realtek")
else()
    message(FATAL_ERROR "Unsupported platform '${L4_PLATFORM_NAME}' in ${PLATFORM_CONFIG_FILE}")
endif()
message(STATUS "Building L4 pipeline implementation for ${L4_COMPILED_PLATFORM}")

# The common harness and selected platform source make one executable.
set(AUDIO_DECODER_SOURCES
    rdk-AV-UT.cpp
    src/common_test_framework.cpp
    src/configuration_layer.cpp
    src/L4-rdk-audio-video.cpp
    src/pipeline_management_layer.cpp
    src/verification_layer.cpp
    ${PLATFORM_PIPELINE_SOURCE}
)

add_executable(L4-rdk-audio-video ${AUDIO_DECODER_SOURCES})

target_compile_features(L4-rdk-audio-video PRIVATE cxx_std_17)
target_compile_definitions(L4-rdk-audio-video PRIVATE
    L4_DEFAULT_CONFIG_PATH="${L4_CONFIG_INSTALL_PATH}"
)
target_include_directories(L4-rdk-audio-video PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(L4-rdk-audio-video PRIVATE
    ut_core
    L4-rdk-audio-video_dependencies
)

install(TARGETS L4-rdk-audio-video
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/rdkgstreamerplugin/l4test
)