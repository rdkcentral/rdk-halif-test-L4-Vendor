/** @file vts_verification.cpp
 * @brief Uses libavformat to detect the audio and video streams in a media file.
 */
#include "vts_verification.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

#include <memory>
#include <stdexcept>

namespace {
/** Converts an FFmpeg error code into its diagnostic text. */
std::string ffmpegError(int code)
{
    char message[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(code, message, sizeof(message));
    return message;
}
}

/** Opens the input with libavformat and records whether it has audio and video streams. */
DetectedVideoInfo MediaStreamDetector::detect(const std::string& mediaPath)
{
    AVFormatContext* formatContext = nullptr;
    const int openResult = avformat_open_input(&formatContext, mediaPath.c_str(), nullptr, nullptr);
    if (openResult < 0) {
        throw std::runtime_error("FFmpeg could not open media: " + mediaPath +
                                 " (" + ffmpegError(openResult) + ", code " +
                                 std::to_string(openResult) + ")");
    }
    const auto closeInput = [](AVFormatContext* context) { avformat_close_input(&context); };
    std::unique_ptr<AVFormatContext, decltype(closeInput)> input(formatContext, closeInput);

    const int streamResult = avformat_find_stream_info(input.get(), nullptr);
    if (streamResult < 0) {
        throw std::runtime_error("FFmpeg could not read media streams: " + mediaPath +
                                 " (" + ffmpegError(streamResult) + ", code " +
                                 std::to_string(streamResult) + ")");
    }

    DetectedVideoInfo info;
    for (unsigned int index = 0; index < input->nb_streams; ++index) {
        const AVStream* stream = input->streams[index];
        // Cover art is reported as a video stream but is not playable video.
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            continue;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            info.hasAudio = true;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            info.hasVideo = true;
        }
    }
    return info;
}



