#include "tim.h"
#include "usart.h"

#include <algorithm>
#include <array>

#include "data.h"
#include "Status.h"
#include "strings.h"
#include "SSD1306_Shared.h"

static uint8_t tim4_scaler_5 = 0;

static char string_buffer[24] = { 0 }; // 字符串缓冲区

inline static void RefreshDisplay() {
	ssd1306.Clear();
	ssd1306.SetCursor(0, 0);
	ssd1306.WriteString("Nav Light", SSD1306Fonts::Font_11x18);

	// Status::bleMACToCString(mac_string);
	// ssd1306.SetCursor(0, 18);
	// ssd1306.WriteString(mac_string, SSD1306Fonts::Font_7x10);

	ssd1306.SetCursor(0, 24);
	ssd1306.WriteString("'(.", SSD1306Fonts::Font_CN); // “亮度：”
	string_buffer[int8ToString(Status::target, string_buffer)] = '\0'; // 确保字符串以 '\0' 结尾
	ssd1306.WriteString(string_buffer, SSD1306Fonts::Font_11x18);
	ssd1306.WriteString("%  ", SSD1306Fonts::Font_11x18);

	ssd1306.SetCursor(0, 44);
	ssd1306.WriteString(")*.", SSD1306Fonts::Font_CN); // “风扇：”
	if (Status::brightness > 0) {
		ssd1306.WriteString("+,-", SSD1306Fonts::Font_CN); // “运行中”
	} else if (Status::getFanStatus()) {
		ssd1306.WriteString("1/0", SSD1306Fonts::Font_CN); // “将停止”
	} else {
		ssd1306.WriteString("2/0", SSD1306Fonts::Font_CN); // “已停止”
	}

	ssd1306.UpdateScreen();
}

inline static void TIM4_Update_Callback() {
	// 100ms

	Status::fanAutoControl();

	auto diff = std::abs(Status::target - Status::brightness);
	auto step = 0;
	if (Status::target == 0) {
		step = 50;
	} else if (diff > 50) {
		step = 20;
	} else if (diff > 20) {
		step = 10;
	} else {
		step = 5;
	}

	if (Status::target > Status::brightness) {
		Status::brightness = std::min(static_cast<int8_t>(Status::brightness + step), Status::target);
	} else if (Status::target < Status::brightness) {
		Status::brightness = std::max(static_cast<int8_t>(Status::brightness - step * 2), Status::target);
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