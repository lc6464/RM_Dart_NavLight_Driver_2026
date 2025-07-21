#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <span>

namespace Status {
	extern int8_t target;
	extern int8_t brightness;
	extern int8_t lastBrightness;

	extern bool lastFanStatus;

	extern bool ble3v3Detected;
	extern bool bleMACParsed;
	extern std::array<uint8_t, 6> bleMAC;

	extern bool ssd1306Enabled;

	bool getFanStatus();
	uint16_t getFanOffTickMax();
	int16_t getFanOffRemainingTime();
	void fanAutoControl();

	bool bleMACFrom(std::string_view macStr);
	size_t bleMACTo(std::span<char> buffer);
}