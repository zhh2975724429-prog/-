                                                                                                               #include "main.h"
#include "timebase.h"

// 其他模块需要的外部变量
// Debug watch value for the monitor screen.

volatile double display_frequency = 0.0;

// 系统模式变量
volatile SystemMode current_mode = MODE_CALIBRATION;
volatile uint8_t mode_switch_flag = 0;

// 标定模式变量
typedef enum {
    CAL_STATE_SELECT_GEAR = 0,
    CAL_STATE_CAPTURE_NO_LOAD = 1,
    CAL_STATE_CONFIRM_NO_LOAD = 2,
    CAL_STATE_CAPTURE_LOAD_2T = 3,
    CAL_STATE_CONFIRM_LOAD_2T = 4,
    CAL_STATE_ALL_DONE = 5
} CalibrationState;

static CalibrationState cal_state = CAL_STATE_SELECT_GEAR;
static GearLevel current_cal_gear = GEAR_INVALID;
static double no_load_speed = 0.0;
static double load_2t_speed = 0.0;
static uint8_t cal_complete[5] = {0};  // 记录各挡位的标定状态

static uint8_t cal_ui_dirty = 1;
static uint8_t cal_refresh_div = 0;
static uint8_t cal_need_recapture = 0;

static void CalibSetState(CalibrationState s)
{
	cal_state = s;
	cal_ui_dirty = 1;
	cal_refresh_div = 0;
}

static void SaveCalibrationResult(void)
{
	uint8_t all_calibrated = 1;

	SaveWeightCurves();
	if (current_cal_gear >= GEAR_1 && current_cal_gear <= GEAR_5)
	{
		cal_complete[current_cal_gear - 1] = 1;
	}

	for (int i = 0; i < 5; i++)
	{
		if (!cal_complete[i])
		{
			all_calibrated = 0;
			break;
		}
	}

	current_cal_gear = GEAR_INVALID;
	no_load_speed = 0.0;
	load_2t_speed = 0.0;

	if (all_calibrated)
	{
		CalibSetState(CAL_STATE_ALL_DONE);
	}
	else
	{
		CalibSetState(CAL_STATE_SELECT_GEAR);
	}
}

static double SampleMotorSpeed(uint8_t samples)
{
	double sum = 0.0;
	if (samples == 0) samples = 1;
	for (uint8_t i = 0; i < samples; i++)
	{
		sum += GetMotorSpeed();
		Delay_ms(30);
	}
	return sum / samples;
}

int main(void)
{
	// 系统初始化
	SetSysClockTo72();
	MX_GPIO_Init();
	Timebase_Init();
	
	// 等待系统稳定（外部晶振起振、PLL锁定）
	Delay_ms(2000);
	
	// 背光已经在ILI9341_Init中初始化，不需要重复初始化
	
	// 初始化输入捕获（用于频率测量）
	InputCapture_Init();
	
	// 初始化TFT屏幕
	ILI9341_Init();
	
	// 加载重量曲线参数
	LoadWeightCurves();

	if (WeightCurvesValid())
	{
		for (int i = 0; i < 5; i++)
		{
			cal_complete[i] = 1;
		}
		CalibSetState(CAL_STATE_ALL_DONE);
	}
	else
	{
		for (int i = 0; i < 5; i++)
		{
			cal_complete[i] = 0;
		}
		CalibSetState(CAL_STATE_SELECT_GEAR);
	}
	
	// 再等待一段时间，让所有模块稳定
	Delay_ms(500);
	
	while(1)
	{
		uint32_t now = Timebase_Millis();
		uint32_t ui_interval = (current_mode == MODE_MONITOR) ? 30U : 100U;
		static uint32_t last_ui_ms = 0;
		static uint32_t last_led_ms = 0;
		GearLevel gear_now_cached = GetCurrentGear();

		// 检查模式切换
		CheckModeSwitch();
		CheckCancelSwitch();

		if (mode_switch_flag)
		{
			mode_switch_flag = 0;
			button_pressed = 0;
			button_long_pressed = 0;
			cancel_pressed = 0;
			cancel_long_pressed = 0;
			ILI9341_Clear(ILI9341_BLACK);
			cal_ui_dirty = 1;
			cal_refresh_div = 0;
		}
		
		if ((uint32_t)(now - last_ui_ms) >= ui_interval)
		{
			last_ui_ms = now;

			// 根据当前模式执行不同的操作
			if (current_mode == MODE_CALIBRATION)
			{
				switch(cal_state)
				{
				case CAL_STATE_SELECT_GEAR:
				{
					GearLevel gear_now = gear_now_cached;

					if (cal_ui_dirty)
					{
						char buffer[32];
						ILI9341_Clear(ILI9341_BLACK);
						ILI9341_PutString(10, 10, "\xE6\xA8\xA1\xE5\xBC\x8F:\xE6\xA0\x87\xE5\xAE\x9A", ILI9341_WHITE, ILI9341_BLACK, 2);
						ILI9341_PutString(10, 60, "\xE6\x8C\xA1\xE4\xBD\x8D 1-5", ILI9341_YELLOW, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 80, "\xE6\x8C\x89=\xE5\xBC\x80\xE5\xA7\x8B", ILI9341_YELLOW, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 290, "PB1=\xE8\xBF\x94\xE5\x9B\x9E \xE9\x95\xBF\xE6\x8C\x89=\xE5\xA4\x8D\xE4\xBD\x8D", ILI9341_WHITE, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 105, "\xE5\xAE\x8C\xE6\x88\x90:", ILI9341_WHITE, ILI9341_BLACK, 1);
						for (int i = 0; i < 5; i++)
						{
							snprintf(buffer, sizeof(buffer), "G%d:%s", i+1, cal_complete[i] ? "OK " : "-- ");
							ILI9341_PutString(10 + (i * 40), 125, buffer, cal_complete[i] ? ILI9341_GREEN : ILI9341_RED, ILI9341_BLACK, 1);
						}
						cal_ui_dirty = 0;
					}

					{
						char buffer[32];
						if (gear_now == GEAR_INVALID)
						{
							ILI9341_PutString(10, 150, "\xE6\x8C\xA1\xE4\xBD\x8D: -- ", ILI9341_CYAN, ILI9341_BLACK, 2);
							ILI9341_PutString(10, 220, "                ", ILI9341_WHITE, ILI9341_BLACK, 1);
						}
						else
						{
							snprintf(buffer, sizeof(buffer), "\xE6\x8C\xA1\xE4\xBD\x8D: G%d ", (int)gear_now);
							ILI9341_PutString(10, 150, buffer, ILI9341_CYAN, ILI9341_BLACK, 2);

							if (cal_complete[gear_now - 1])
							{
								ILI9341_PutString(10, 220, "\xE5\xAE\x8C\xE6\x88\x90 \xE9\x95\xBF\xE6\x8C\x89PB1", ILI9341_WHITE, ILI9341_BLACK, 1);
							}
							else
							{
								ILI9341_PutString(10, 220, "                ", ILI9341_WHITE, ILI9341_BLACK, 1);
							}
						}
					}

					if (cancel_long_pressed)
					{
						cancel_long_pressed = 0;
						if (gear_now != GEAR_INVALID && cal_complete[gear_now - 1])
						{
							cal_complete[gear_now - 1] = 0;
							current_cal_gear = gear_now;
							no_load_speed = 0.0;
							load_2t_speed = 0.0;
							CalibSetState(CAL_STATE_CAPTURE_NO_LOAD);
							break;
						}
					}

					if (button_pressed)
					{
						button_pressed = 0;
						if (gear_now != GEAR_INVALID)
						{
							if (!cal_complete[gear_now - 1])
							{
								current_cal_gear = gear_now;
								CalibSetState(CAL_STATE_CAPTURE_NO_LOAD);
							}
						}
					}
					break;
				}
				case CAL_STATE_CAPTURE_NO_LOAD:
				{
					if (cancel_pressed)
					{
						cancel_pressed = 0;
						current_cal_gear = GEAR_INVALID;
						no_load_speed = 0.0;
						load_2t_speed = 0.0;
						CalibSetState(CAL_STATE_SELECT_GEAR);
						break;
					}
					if (cal_ui_dirty)
					{
						char buffer[32];
						ILI9341_Clear(ILI9341_BLACK);
						ILI9341_PutString(10, 10, "\xE6\xA8\xA1\xE5\xBC\x8F:\xE6\xA0\x87\xE5\xAE\x9A", ILI9341_WHITE, ILI9341_BLACK, 2);
						snprintf(buffer, sizeof(buffer), "\xE6\x8C\xA1\xE4\xBD\x8D: G%d ", (int)current_cal_gear);
						ILI9341_PutString(10, 60, buffer, ILI9341_CYAN, ILI9341_BLACK, 2);
						ILI9341_PutString(10, 110, "\xE6\x8C\x89=\xE9\x87\x87\xE9\x9B\x86" "0T", ILI9341_YELLOW, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 290, "PB1=\xE5\x8F\x96\xE6\xB6\x88", ILI9341_WHITE, ILI9341_BLACK, 1);
						cal_ui_dirty = 0;
					}

					if (cal_refresh_div == 0)
					{
						char buffer[32];
						double live_speed = GetMotorSpeed();
						snprintf(buffer, sizeof(buffer), "\xE8\xBD\xAC\xE9\x80\x9F:%4.0f  ", live_speed);
						ILI9341_PutString(10, 160, buffer, ILI9341_GREEN, ILI9341_BLACK, 2);
						ILI9341_PutRMin(160, 160, ILI9341_GREEN, ILI9341_BLACK, 2);
					}
					cal_refresh_div = (cal_refresh_div + 1) % 3;

					if (button_pressed)
					{
						button_pressed = 0;
						no_load_speed = SampleMotorSpeed(3);
						CalibSetState(CAL_STATE_CONFIRM_NO_LOAD);
					}
					break;
				}
				case CAL_STATE_CONFIRM_NO_LOAD:
				{
					if (cancel_pressed)
					{
						cancel_pressed = 0;
						CalibSetState(CAL_STATE_CAPTURE_NO_LOAD);
						break;
					}
					if (cal_ui_dirty)
					{
						char buffer[32];
						ILI9341_Clear(ILI9341_BLACK);
						ILI9341_PutString(10, 10, "\xE6\xA8\xA1\xE5\xBC\x8F:\xE6\xA0\x87\xE5\xAE\x9A", ILI9341_WHITE, ILI9341_BLACK, 2);
						snprintf(buffer, sizeof(buffer), "\xE6\x8C\xA1\xE4\xBD\x8D: G%d ", (int)current_cal_gear);
						ILI9341_PutString(10, 60, buffer, ILI9341_CYAN, ILI9341_BLACK, 2);
						snprintf(buffer, sizeof(buffer), "\xE8\xBD\xAC\xE9\x80\x9F:%4.0f  ", no_load_speed);
						ILI9341_PutString(10, 120, buffer, ILI9341_GREEN, ILI9341_BLACK, 2);
						ILI9341_PutRMin(160, 120, ILI9341_GREEN, ILI9341_BLACK, 2);
						ILI9341_PutString(10, 200, "\xE6\x8C\x89=\xE7\xA1\xAE\xE8\xAE\xA4", ILI9341_YELLOW, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 290, "PB1=\xE5\x8F\x96\xE6\xB6\x88", ILI9341_WHITE, ILI9341_BLACK, 1);
						cal_ui_dirty = 0;
					}

					if (button_pressed)
					{
						button_pressed = 0;
						CalibSetState(CAL_STATE_CAPTURE_LOAD_2T);
					}
					break;
				}
				case CAL_STATE_CAPTURE_LOAD_2T:
				{
					if (cancel_pressed)
					{
						cancel_pressed = 0;
						CalibSetState(CAL_STATE_CAPTURE_NO_LOAD);
						break;
					}
					if (cal_ui_dirty)
					{
						char buffer[32];
						ILI9341_Clear(ILI9341_BLACK);
						ILI9341_PutString(10, 10, "\xE6\xA8\xA1\xE5\xBC\x8F:\xE6\xA0\x87\xE5\xAE\x9A", ILI9341_WHITE, ILI9341_BLACK, 2);
						snprintf(buffer, sizeof(buffer), "\xE6\x8C\xA1\xE4\xBD\x8D: G%d ", (int)current_cal_gear);
						ILI9341_PutString(10, 60, buffer, ILI9341_CYAN, ILI9341_BLACK, 2);
						snprintf(buffer, sizeof(buffer), "\xE7\xA9\xBA\xE8\xBD\xBD:%5.0f   ", no_load_speed);
						ILI9341_PutString(10, 110, buffer, ILI9341_YELLOW, ILI9341_BLACK, 2);
						ILI9341_PutString(10, 160, "\xE6\x8C\x89=\xE9\x87\x87\xE9\x9B\x86" "2T", ILI9341_YELLOW, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 290, "PB1=\xE5\x8F\x96\xE6\xB6\x88", ILI9341_WHITE, ILI9341_BLACK, 1);
						cal_ui_dirty = 0;
					}

					if (cal_refresh_div == 0)
					{
						char buffer[32];
						double live_speed = GetMotorSpeed();
						snprintf(buffer, sizeof(buffer), "\xE8\xBD\xAC\xE9\x80\x9F:%4.0f  ", live_speed);
						ILI9341_PutString(10, 200, buffer, ILI9341_GREEN, ILI9341_BLACK, 2);
						ILI9341_PutRMin(160, 200, ILI9341_GREEN, ILI9341_BLACK, 2);
					}
					cal_refresh_div = (cal_refresh_div + 1) % 3;

					if (button_pressed)
					{
						button_pressed = 0;
						load_2t_speed = SampleMotorSpeed(3);
						CalibSetState(CAL_STATE_CONFIRM_LOAD_2T);
					}
					break;
				}
				case CAL_STATE_CONFIRM_LOAD_2T:
				{
					if (cancel_pressed)
					{
						cancel_pressed = 0;
						CalibSetState(CAL_STATE_CAPTURE_LOAD_2T);
						break;
					}
					if (cal_ui_dirty)
					{
						char buffer[32];
						ILI9341_Clear(ILI9341_BLACK);
						ILI9341_PutString(10, 10, "\xE6\xA8\xA1\xE5\xBC\x8F:\xE6\xA0\x87\xE5\xAE\x9A", ILI9341_WHITE, ILI9341_BLACK, 2);
						snprintf(buffer, sizeof(buffer), "\xE6\x8C\xA1\xE4\xBD\x8D: G%d ", (int)current_cal_gear);
						ILI9341_PutString(10, 60, buffer, ILI9341_CYAN, ILI9341_BLACK, 2);
						snprintf(buffer, sizeof(buffer), "\xE8\xBD\xAC\xE9\x80\x9F:%4.0f  ", load_2t_speed);
						ILI9341_PutString(10, 120, buffer, ILI9341_GREEN, ILI9341_BLACK, 2);
						ILI9341_PutRMin(160, 120, ILI9341_GREEN, ILI9341_BLACK, 2);
						ILI9341_PutString(10, 200, cal_need_recapture ? "\xE6\x8C\x89=\xE9\x87\x8D\xE9\x87\x87\xE9\x9B\x86" : "\xE6\x8C\x89=\xE7\xA1\xAE\xE8\xAE\xA4", ILI9341_YELLOW, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 290, "PB1=\xE5\x8F\x96\xE6\xB6\x88", ILI9341_WHITE, ILI9341_BLACK, 1);
						cal_ui_dirty = 0;
					}

					if (button_pressed)
					{
						button_pressed = 0;
						if (cal_need_recapture)
						{
							cal_need_recapture = 0;
							CalibSetState(CAL_STATE_CAPTURE_LOAD_2T);
							break;
						}

						if (CalibrateWeightCurve(current_cal_gear, no_load_speed, load_2t_speed))
						{
							cal_need_recapture = 0;
							SaveCalibrationResult();
						}
						else
						{
							cal_need_recapture = 1;
							cal_ui_dirty = 1;
						}
					}
					break;
				}
				case CAL_STATE_ALL_DONE:
				{
					if (cancel_pressed)
					{
						cancel_pressed = 0;
						for (int i = 0; i < 5; i++)
						{
							cal_complete[i] = 0;
						}
						current_cal_gear = GEAR_INVALID;
						no_load_speed = 0.0;
						load_2t_speed = 0.0;
						CalibSetState(CAL_STATE_SELECT_GEAR);
						break;
					}
					if (cal_ui_dirty)
					{
						ILI9341_Clear(ILI9341_BLACK);
						ILI9341_PutString(10, 10, "\xE6\xA8\xA1\xE5\xBC\x8F:\xE6\xA0\x87\xE5\xAE\x9A", ILI9341_WHITE, ILI9341_BLACK, 2);
						ILI9341_PutString(10, 70, "\xE5\x85\xA8\xE9\x83\xA8\xE5\xAE\x8C\xE6\x88\x90", ILI9341_GREEN, ILI9341_BLACK, 2);
						ILI9341_PutString(10, 140, "\xE8\xBF\x9B\xE5\x85\xA5\xE7\x9B\x91\xE6\xB5\x8B?", ILI9341_YELLOW, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 160, "\xE6\x8C\x89=\xE8\xBF\x9B\xE5\x85\xA5", ILI9341_YELLOW, ILI9341_BLACK, 1);
						ILI9341_PutString(10, 290, "PB1=\xE5\x8F\x96\xE6\xB6\x88", ILI9341_WHITE, ILI9341_BLACK, 1);
						cal_ui_dirty = 0;
					}

					if (button_pressed)
					{
						button_pressed = 0;
						current_mode = MODE_MONITOR;
						mode_switch_flag = 1;
					}
					break;
				}
				}
			}
			else if (current_mode == MODE_MONITOR)
			{
				// 监测模式逻辑
				// 测量频率（Get_Frequency内部已保持上一次的值）
				display_frequency = Get_Frequency();
				
				// 测量转速
				double speed = GetMotorSpeed();
				
				// 识别挡位
				GearLevel gear = gear_now_cached;
				
				// 计算起重量
				float weight = CalculateWeight(gear, speed);
				
				// 判断转向
				MotorDirection direction = GetMotorDirection();
				
				// 在TFT屏幕上显示数据
				ILI9341_DisplayData(speed, weight, gear, direction);
			}
		}
		
		if ((uint32_t)(now - last_led_ms) >= 500)
		{
			last_led_ms = now;
			GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		}

		Delay_ms(5);
	}
}

void SetSysClockTo72(void) 
{
	// 使用外部晶振HSE = 8MHz，通过PLL倍频到72MHz
	// 以前用这个配置能测准，恢复这个配置
	
	// 先切换到HSI，确保系统有时钟
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_HSI;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);
	
	// 关闭PLL
	RCC->CR &= ~RCC_CR_PLLON;
	while (RCC->CR & RCC_CR_PLLRDY);
	
	// 使能外部晶振HSE
	RCC->CR |= RCC_CR_HSEON;
	// 等待HSE稳定
	while (!(RCC->CR & RCC_CR_HSERDY));
	
	// 配置Flash延迟（72MHz时需要2个等待周期）
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |= FLASH_ACR_LATENCY_2;
	
	// 配置PLL：HSE作为输入，9倍频 = 72MHz
	RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);
	RCC->CFGR |= RCC_CFGR_PLLSRC_HSE;
	RCC->CFGR |= RCC_CFGR_PLLMULL9;
	
	// 配置总线分频
	// HCLK = SYSCLK = 72MHz
	// PCLK1 = HCLK / 2 = 36MHz
	// PCLK2 = HCLK = 72MHz
	RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;
	
	// 使能PLL
	RCC->CR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));
	
	// 切换系统时钟到PLL
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

void Error(void)
{
	GPIO_WriteBit(LED_GPIO_Port, LED_Pin, Bit_SET);
}
