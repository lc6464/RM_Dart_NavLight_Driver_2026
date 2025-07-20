#pragma once

#include <array>
#include <cstdint>

namespace Status {
	extern int8_t target;
	extern int8_t brightness;
	extern int8_t lastBrightness;

	extern bool lastFanStatus;

	extern bool ble3v3Detected;
	// extern std::array<uint8_t, 6> bleMAC;

	bool getFanStatus();
	void fanAutoControl();

	// void bleMACFromCString(const char *macStr);
	// uint8_t bleMACToCString(char *macStr);
}