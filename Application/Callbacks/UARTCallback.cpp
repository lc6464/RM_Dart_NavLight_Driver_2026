#include <algorithm>
#include <charconv>
#include <string_view>
#include <system_error>

#include "usart.h"

#include "data.h"
#include "Status.h"

constexpr std::string_view CMD_TURN_OFF = "关灯";
constexpr std::string_view CMD_PREFIX_BRIGHTNESS = "亮度";

// HAL 库串口回调函数
extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	if (huart->Instance != USART1) {
		return;
	}

	std::string_view received_data(reinterpret_cast<const char *>(uart_data_buffer.data()), Size);

	if (received_data == CMD_TURN_OFF) { // 精确匹配 "关灯" 命令
		Status::target = 0;
	} else if (received_data.starts_with(CMD_PREFIX_BRIGHTNESS)) { // 检查是否为 "亮度" 命令前缀
		// 提取数值部分
		std::string_view value_part = received_data.substr(CMD_PREFIX_BRIGHTNESS.length());

		// 检查数值部分是否为空，防止 "亮度" 命令导致解析错误
		if (!value_part.empty()) {
			int temp_brightness = 0;
			auto result = std::from_chars(value_part.data(),
				value_part.data() + value_part.size(),
				temp_brightness);

			// 检查转换结果
			if (result.ec == std::errc{} && result.ptr == value_part.data() + value_part.size()) {
				// 检查数值范围
				if (temp_brightness >= 0 && temp_brightness <= 100) {
					Status::target = static_cast<int8_t>(temp_brightness);
				}
			}
		}
	}

	// 重新启动 DMA 接收
	HAL_UARTEx_ReceiveToIdle_DMA(huart, uart_data_buffer.data(), uart_data_buffer.size());
}