#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include "data.h"
#include "SSD1306_Shared.h"
#include "Status.h"


/**
	* @brief  The application entry point.
	* @retval int
	*/
int main(void) {
	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();
	MX_DMA_Init();
	MX_TIM1_Init();
	MX_TIM4_Init();
	MX_USART1_UART_Init();
	MX_I2C1_Init();

	HAL_Delay(80); // 等待 80ms 以确保屏幕启动完成
	if (Status::ssd1306Enabled) {
		Status::ssd1306Enabled = ssd1306.Start() == HAL_OK; // 启动屏幕
	} else {
		HAL_Delay(40); // 如果屏幕未启用，等待 40ms
	}

	HAL_Delay(80); // 继续等待 80ms 以确保蓝牙模块启动完成
	auto ble_3v3_detected = HAL_GPIO_ReadPin(BLE_3V3_Detect_GPIO_Port, BLE_3V3_Detect_Pin); // 检查蓝牙模块是否插上

	if (ble_3v3_detected == GPIO_PIN_SET) {
		// 蓝牙模块已插上，进一步初始化变量和片上外设

		constexpr std::string_view AT_CMD_GET_ADDR = "AT+LADDR\r\n";
		constexpr std::string_view EXPECTED_RESPONSE_PREFIX = "+LADDR=";
		constexpr size_t MAC_STRING_LENGTH = 12; // "AABBCCDDEEFF"
		// 预期响应的总长度："+LADDR=" (7) + MAC (12) = 19
		constexpr size_t EXPECTED_RESPONSE_LENGTH = EXPECTED_RESPONSE_PREFIX.length() + MAC_STRING_LENGTH;

		Status::ble3v3Detected = true;

		HAL_GPIO_WritePin(BLE_Key_GPIO_Port, BLE_Key_Pin, GPIO_PIN_RESET); // 使蓝牙模块断开连接
		HAL_Delay(220); // 等待 220ms
		HAL_GPIO_WritePin(BLE_Key_GPIO_Port, BLE_Key_Pin, GPIO_PIN_SET); // 恢复蓝牙模块

		HAL_Delay(80); // 等待 80ms 以确保蓝牙模块恢复正常

		// 查询 MAC 地址
		HAL_UART_Transmit(&huart1,
			reinterpret_cast<const uint8_t *>(AT_CMD_GET_ADDR.data()),
			AT_CMD_GET_ADDR.size(),
			100);

		// +LADDR=xxxxxxxxxxxx
		HAL_StatusTypeDef rx_status = HAL_UART_Receive(&huart1,
			uart_data_buffer.data(),
			EXPECTED_RESPONSE_LENGTH,
			100);

		if (rx_status == HAL_OK) {
			// 使用 string_view 无内存拷贝
			std::string_view response(reinterpret_cast<const char *>(uart_data_buffer.data()),
				EXPECTED_RESPONSE_LENGTH);

			// 检查响应头是否正确
			if (response.starts_with(EXPECTED_RESPONSE_PREFIX)) {
				// 提取 MAC 地址部分
				std::string_view mac_part = response.substr(EXPECTED_RESPONSE_PREFIX.length());
				// 调用解析函数
				if (Status::bleMACFrom(mac_part)) {
					// 成功解析 MAC 地址
					Status::bleMACParsed = true;
				}
			}
		}

		if (Status::ssd1306Enabled) {
			// 在屏幕显示标题和 MAC 地址
			extern std::array<char, 18> screen_string_buffer;
			ssd1306.SetCursor(0, 0);

			if (Status::bleMACParsed && Status::bleMACTo(screen_string_buffer) > 0) {
				ssd1306.WriteString("Dart Nav Light", SSD1306Fonts::Font_7x10);
				ssd1306.SetCursor(0, 14);
				ssd1306.WriteString(screen_string_buffer.data(), SSD1306Fonts::Font_7x10);
			} else {
				Status::bleMACParsed = false;
				ssd1306.WriteString("Nav Light", SSD1306Fonts::Font_11x18);
			}

			ssd1306.UpdateScreen(0, 2);
		}

		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_data_buffer.data(), uart_data_buffer.size());
		__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); // 启用空闲中断

		HAL_TIM_Base_Start(&htim1);
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

		HAL_TIM_Base_Start_IT(&htim4); // 10Hz
	} else {
		// 蓝牙模块未插上，不执行正常逻辑

		if (Status::ssd1306Enabled) {
			ssd1306.SetCursor((128 - 3 * 16) / 2, 12);
			ssd1306.WriteString(" !\"", SSD1306Fonts::Font_CN); // “未插入”

			ssd1306.SetCursor((128 - 4 * 16) / 2, 36);
			ssd1306.WriteString("#$%&", SSD1306Fonts::Font_CN); // “蓝牙模块”

			ssd1306.UpdateScreen();
		}

		while (1) { }
	}

	while (1) {
		if (Status::getFanStatus() != Status::lastFanStatus) {
			Status::lastFanStatus = Status::getFanStatus();
			// 风扇状态改变

			HAL_GPIO_WritePin(Fan_Enable_GPIO_Port, Fan_Enable_Pin, Status::getFanStatus() ? GPIO_PIN_SET : GPIO_PIN_RESET);
		}

		if (Status::brightness != Status::lastBrightness) {
			Status::lastBrightness = Status::brightness;
			// 亮度改变

			__HAL_TIM_SetCompare(&htim1, Main_LED_Channel, Status::brightness);
			HAL_GPIO_WritePin(LED_Enable_GPIO_Port, LED_Enable_Pin, Status::brightness == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
		}

		HAL_Delay(5);
	}
}

/**
	* @brief System Clock Configuration
	* @retval None
	*/
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct{};
	RCC_ClkInitTypeDef RCC_ClkInitStruct{};

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
		| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
	* @brief  This function is executed in case of error occurrence.
	* @retval None
	*/
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
	* @brief  Reports the name of the source file and the source line number
	*         where the assert_param error has occurred.
	* @param  file: pointer to the source file name
	* @param  line: assert_param error line source number
	* @retval None
	*/
void assert_failed(uint8_t *file, uint32_t line) {
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
		 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
		 /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
