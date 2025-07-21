#include "Status.h"

#include <cstdio>
#include <charconv>
#include <system_error>

namespace Status {
	int8_t target = 0;
	int8_t brightness = 0;
	int8_t lastBrightness = 0;

	bool isFanOn = false;
	bool lastFanStatus = false;

	// 风扇在关灯后保持运行一段时间
	uint16_t fanOffTick = 0;
	uint16_t fanOffTickMax = 150; // 150 * 0.1s = 15s
	bool isFanStopped = false;
	int16_t fanOffRemainingTime = 0; // 风扇停止剩余时间，-1 为持续运行，0 为已关闭，单位为 100ms

	// 蓝牙状态
	bool ble3v3Detected = false;
	bool bleMACParsed = false;
	std::array<uint8_t, 6> bleMAC{};

	// 屏幕启用
	bool ssd1306Enabled = true;

	bool getFanStatus() {
		return isFanOn;
	}

	uint16_t getFanOffTickMax() {
		return fanOffTickMax;
	}

	int16_t getFanOffRemainingTime() {
		return fanOffRemainingTime;
	}

	void fanAutoControl() {
		if (brightness > 0) {
			isFanOn = true;
			fanOffTick = 0;
			isFanStopped = false;
			fanOffRemainingTime = -1; // 持续运行
		} else {
			if (!isFanStopped) {
				if (++fanOffTick >= fanOffTickMax) {
					isFanOn = false;
					fanOffTick = 0;
					isFanStopped = true;
				}
				fanOffRemainingTime = fanOffTickMax - fanOffTick; // 计算剩余时间，不会出现负数
			}
		}
	}

	bool bleMACFrom(std::string_view macStr) {
		// 验证输入长度是否为12个字符
		if (macStr.length() != 12) {
			return false;
		}

		// 逐字节解析
		for (size_t i = 0; i < bleMAC.size(); ++i) {
			// 从输入字符串中每次取2个字符
			const char *const first = macStr.data() + (i * 2);
			const char *const last = first + 2;

			// 使用 std::from_chars 进行转换
			auto result = std::from_chars(first, last, bleMAC[i], 16);

			// 检查转换是否成功
			if (result.ec != std::errc{} || result.ptr != last) {
				// 如果发生错误，将已解析的部分清零并返回失败
				bleMAC.fill(0);
				return false;
			}
		}

		return true;
	}

	size_t bleMACTo(std::span<char> buffer) {
		// 格式化后的字符串 "AA:BB:CC:DD:EE:FF" 需要17个字符，加上 '\0' 需要18个字节
		constexpr size_t required_size = 18;

		if (buffer.size() < required_size) {
			return 0; // 缓冲区空间不足
		}

		int chars_written = snprintf(buffer.data(), buffer.size(), "%02X:%02X:%02X:%02X:%02X:%02X",
			bleMAC[0], bleMAC[1], bleMAC[2],
			bleMAC[3], bleMAC[4], bleMAC[5]);

		if (chars_written > 0) {
			return static_cast<size_t>(chars_written);
		}

		return 0; // 发生未知错误
	}
}