/** @file vts_amlogic_verification.cpp
 * @brief Validates Amlogic decoder properties during playback.
 */
#include "vts_amlogic_verification.h"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>

namespace {

constexpr const char* kDefaultVdecStatusPath = "/sys/class/vdec/vdec_status";

AmlogicErrorCountCheck validateErrorCount(const std::regex& propertyPattern, const std::string& label)
{
    const char* configuredPath = std::getenv("VDEC_STATUS_PATH");
    const std::string path = configuredPath && *configuredPath ? configuredPath : kDefaultVdecStatusPath;
    std::ifstream statusFile(path);
    if (!statusFile) {
        return {false, false, "Cannot read decoder status: " + path};
    }
    const std::string status((std::istreambuf_iterator<char>(statusFile)),
                             std::istreambuf_iterator<char>());
    if (statusFile.bad()) {
        return {false, false, "Could not complete decoder status read: " + path};
    }
    const std::regex channelPattern(R"(vdec\s+channel\s+([0-9]+)\s+statistics\s*:)", std::regex::icase);
    const auto channels = std::distance(std::sregex_iterator(status.begin(), status.end(), channelPattern),
                                        std::sregex_iterator());
    if (channels == 0) {
        return {false, false, "No active decoder channel in " + path};
    }
    if (channels != 1) {
        return {true, false, "Multiple decoder channels; cannot identify the current stream"};
    }
    std::smatch propertyMatch;
    if (!std::regex_search(status, propertyMatch, propertyPattern)) {
        return {false, false, label + " is missing/invalid in " + path};
    }
    std::uint64_t count;
    try {
        count = std::stoull(propertyMatch[1].str());
    } catch (const std::exception&) {
        return {false, false, label + " is outside the supported integer range"};
    }
    return {true, count == 0, label + "=" + std::to_string(count) + ", expected=0"};
}

} // namespace

AmlogicErrorCountCheck validateAmlogicFrameErrorCount()
{
    static const std::regex pattern(R"(\bfra\s+err\s+count\s*:\s*([0-9]+)(?=\s|$|[A-Za-z_]))",
                                     std::regex::icase);
    return validateErrorCount(pattern, "Decoder frame error count");
}

AmlogicErrorCountCheck validateAmlogicHardwareErrorCount()
{
    static const std::regex pattern(R"(\bhw\s+err\s+count\s*:\s*([0-9]+)(?=\s|$|[A-Za-z_]))",
                                     std::regex::icase);
    return validateErrorCount(pattern, "Decoder hardware error count");
}

AmlogicFrameDimensionsCheck validateAmlogicFrameDimensions(int inputWidth, int inputHeight)
{
    if (inputWidth <= 0 || inputHeight <= 0) {
        return {true, false, "Input video dimensions are unavailable"};
    }
    const char* configuredPath = std::getenv("VDEC_STATUS_PATH");
    const std::string path = configuredPath && *configuredPath ? configuredPath : kDefaultVdecStatusPath;
    std::ifstream statusFile(path);
    if (!statusFile) {
        return {false, false, "Cannot read decoder status: " + path};
    }
    const std::string status((std::istreambuf_iterator<char>(statusFile)),
                             std::istreambuf_iterator<char>());
    if (statusFile.bad()) {
        return {false, false, "Could not complete decoder status read: " + path};
    }

    const std::regex channelPattern(R"(vdec\s+channel\s+([0-9]+)\s+statistics\s*:)", std::regex::icase);
    const auto channels = std::distance(std::sregex_iterator(status.begin(), status.end(), channelPattern),
                                        std::sregex_iterator());
    if (channels == 0) {
        return {false, false, "No active decoder channel in " + path};
    }
    if (channels != 1) {
        return {true, false, "Multiple decoder channels; cannot identify the current stream"};
    }
    const std::regex widthPattern(R"(frame\s+width\s*:\s*([0-9]+)\b)", std::regex::icase);
    const std::regex heightPattern(R"(frame\s+height\s*:\s*([0-9]+)\b)", std::regex::icase);
    std::smatch widthMatch;
    std::smatch heightMatch;
    if (!std::regex_search(status, widthMatch, widthPattern) ||
        !std::regex_search(status, heightMatch, heightPattern)) {
        return {false, false, "Decoder frame width or height is missing/invalid in " + path};
    }
    int outputWidth;
    int outputHeight;
    try {
        outputWidth = std::stoi(widthMatch[1].str());
        outputHeight = std::stoi(heightMatch[1].str());
    } catch (const std::exception&) {
        return {false, false, "Decoder dimensions are outside the supported integer range"};
    }
    if (outputWidth <= 0 || outputHeight <= 0) {
        return {false, false, "Decoder frame dimensions are not ready"};
    }
    const bool matches = outputWidth == inputWidth && outputHeight == inputHeight;
    return {true, matches, "Input=" + std::to_string(inputWidth) + "x" + std::to_string(inputHeight) +
                               ", decoder=" + std::to_string(outputWidth) + "x" +
                               std::to_string(outputHeight) + (matches ? " (match)" : " (mismatch)")};
}

AmlogicDropCountCheck validateAmlogicDropCount(std::uint64_t allowedDrops)
{
    const char* configuredPath = std::getenv("VDEC_STATUS_PATH");
    const std::string path = configuredPath && *configuredPath ? configuredPath : kDefaultVdecStatusPath;
    std::ifstream statusFile(path);
    if (!statusFile) {
        return {false, false, "Cannot read decoder status: " + path};
    }
    const std::string status((std::istreambuf_iterator<char>(statusFile)),
                             std::istreambuf_iterator<char>());
    if (statusFile.bad()) {
        return {false, false, "Could not complete decoder status read: " + path};
    }
    const std::regex channelPattern(R"(vdec\s+channel\s+([0-9]+)\s+statistics\s*:)", std::regex::icase);
    const auto channels = std::distance(std::sregex_iterator(status.begin(), status.end(), channelPattern),
                                        std::sregex_iterator());
    if (channels == 0) {
        return {false, false, "No active decoder channel in " + path};
    }
    if (channels != 1) {
        return {true, false, "Multiple decoder channels; cannot identify the current stream"};
    }
    const std::regex dropPattern(R"(\bdrop\s+count\s*:\s*([0-9]+)(?=\s|$|[A-Za-z_]))", std::regex::icase);
    std::smatch dropMatch;
    if (!std::regex_search(status, dropMatch, dropPattern)) {
        return {false, false, "Decoder drop count is missing/invalid in " + path};
    }
    std::uint64_t drops;
    try {
        drops = std::stoull(dropMatch[1].str());
    } catch (const std::exception&) {
        return {false, false, "Decoder drop count is outside the supported integer range"};
    }
    return {true, drops <= allowedDrops, "Decoder drop count=" + std::to_string(drops) +
                                           ", allowed=" + std::to_string(allowedDrops)};
}