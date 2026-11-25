/** @file vts_amlogic_verification.h
 * @brief Amlogic-specific decoder status access.
 */
#ifndef VTS_AMLOGIC_VERIFICATION_H
#define VTS_AMLOGIC_VERIFICATION_H

#include <string>
#include <cstdint>

struct AmlogicFrameDimensionsCheck {
	bool available = false;
	bool passed = false;
	std::string detail;
};

AmlogicFrameDimensionsCheck validateAmlogicFrameDimensions(int inputWidth, int inputHeight);

struct AmlogicDropCountCheck {
	bool available = false;
	bool passed = false;
	std::string detail;
};

AmlogicDropCountCheck validateAmlogicDropCount(std::uint64_t allowedDrops);

struct AmlogicErrorCountCheck {
	bool available = false;
	bool passed = false;
	std::string detail;
};

AmlogicErrorCountCheck validateAmlogicFrameErrorCount();
AmlogicErrorCountCheck validateAmlogicHardwareErrorCount();

#endif // VTS_AMLOGIC_VERIFICATION_H