#include "Status.h"

#include <cstdio>

namespace Status {
	int8_t target = 0;
	int8_t brightness = 0;
	int8_t lastBrightness = 0;
	bool isFanOn = false;
	bool lastFanStatus = false;

	// 风扇关灯后保持
	uint16_t fanOffTick = 0;
	uint16_t fanOffTickMax = 150; // 150 * 0.1s = 15s
	bool isFanStopped = false;

	// 蓝牙状态
	bool ble3v3Detected = false;
	// std::array<uint8_t, 6> bleMAC{};

	bool getFanStatus() {
		return isFanOn;
	}

	void fanAutoControl() {
		if (brightness > 0) {
			isFanOn = true;
			fanOffTick = 0;
			isFanStopped = false;
		} else {
			if (!isFanStopped) {
				if (++fanOffTick >= fanOffTickMax) {
					isFanOn = false;
					fanOffTick = 0;
					isFanStopped = true;
				}
			}
		}
	}

	// void bleMACFromCString(const char *macStr) {
	// 	sscanf(macStr, "%hhx%hhx%hhx%hhx%hhx%hhx",
	// 		&bleMAC[0], &bleMAC[1], &bleMAC[2],
	// 		&bleMAC[3], &bleMAC[4], &bleMAC[5]);
	// }

	// uint8_t bleMACToCString(char *macStr) {
	// 	sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
	// 		bleMAC[0], bleMAC[1], bleMAC[2],
	// 		bleMAC[3], bleMAC[4], bleMAC[5]);
	// 	return 17; // 返回字符串长度
	// }
}