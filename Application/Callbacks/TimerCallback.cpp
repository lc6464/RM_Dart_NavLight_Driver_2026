#include "tim.h"
#include "usart.h"

#include <algorithm>
#include <array>

#include "data.h"
#include "Status.h"
#include "strings.h"
#include "SSD1306_Shared.h"

static uint8_t tim4_scaler_5 = 0;

std::array<char, 24> screen_string_buffer{}; // 字符串缓冲区

inline static void RefreshDisplay() {
	if (!Status::ssd1306Enabled) {
		return; // 如果屏幕未启用，则不进行刷新
	}

	ssd1306.SetCursor(0, Status::bleMACParsed ? 28 : 24);
	ssd1306.WriteString("'(.", SSD1306Fonts::Font_CN); // “亮度：”
	int8ToString(Status::target, screen_string_buffer.data());
	ssd1306.WriteString(screen_string_buffer.data(), SSD1306Fonts::Font_11x18);
	ssd1306.WriteString("%  ", SSD1306Fonts::Font_11x18);

	ssd1306.SetCursor(0, Status::bleMACParsed ? 48 : 46);
	ssd1306.WriteString(")*.", SSD1306Fonts::Font_CN); // “风扇：”
	if (Status::target > 0) { // 通过 target 判断，以免 UI 不同步使人误解
		ssd1306.WriteString("+,-", SSD1306Fonts::Font_CN); // “运行中”
		ssd1306.WriteString("  ", SSD1306Fonts::Font_Space);
	} else if (Status::getFanStatus()) {
		ssd1306.WriteString("1/0", SSD1306Fonts::Font_CN); // “将停止”
		ssd1306.WriteString("  ", SSD1306Fonts::Font_Space);

		auto remaining_time = Status::getFanOffRemainingTime();
		remaining_time = (remaining_time < 0 ? Status::getFanOffTickMax() : remaining_time) / 10 + 1; // 转换为秒

		ssd1306.SetCursor(110 - remaining_time / 10 * 7, 54);
		auto length = int16ToString(remaining_time, screen_string_buffer.data());
		screen_string_buffer[length++] = 's'; // 添加单位
		screen_string_buffer[length] = '\0'; // 确保字符串以 '\0' 结尾
		ssd1306.WriteString(screen_string_buffer.data(), SSD1306Fonts::Font_7x10);
	} else {
		ssd1306.WriteString("2/0", SSD1306Fonts::Font_CN); // “已停止”
		ssd1306.WriteString("  ", SSD1306Fonts::Font_Space);
	}

	ssd1306.UpdateScreen(3); // 刷新第3页到第7页
}

inline static void TIM4_Update_Callback() {
	// 100ms

	Status::fanAutoControl();

	auto diff = std::abs(Status::target - Status::brightness);
	auto step = 0;
	if (diff > 50) {
		step = 20;
	} else if (diff > 25) {
		step = 10;
	} else if (diff > 12) {
		step = 5;
	} else {
		step = 2;
	}

	if (Status::target > Status::brightness) {
		Status::brightness = std::min(static_cast<int8_t>(Status::brightness + step), Status::target);
	} else if (Status::target < Status::brightness) {
		Status::brightness = std::max(static_cast<int8_t>(Status::brightness - step), Status::target);
	}

	RefreshDisplay();

	if (++tim4_scaler_5 == 5) {
		// 500ms
		tim4_scaler_5 = 0;

		// 发送电压
		// HAL_UART_Transmit_DMA(&huart1, reinterpret_cast<uint8_t *>(voltageString), length);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM4) {
		TIM4_Update_Callback();
	}
}