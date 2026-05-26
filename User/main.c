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
static uint8_t cal_last_drawn_gear = 0xFF;

static void CalibSetState(CalibrationState s)
{
	cal_state = s;
	cal_ui_dirty = 1;
	cal_refresh_div = 0;
	cal_last_drawn_gear = 0xFF;
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

#define CAL_COLOR_BG        ILI9341_BLACK
#define CAL_COLOR_HEADER    0x013F
#define CAL_COLOR_PANEL     0x1082
#define CAL_COLOR_PANEL_2   0x2104
#define CAL_COLOR_LINE      0x7BEF

static const char *CalGearText(GearLevel gear)
{
	switch (gear)
	{
	case GEAR_1: return "G1";
	case GEAR_2: return "G2";
	case GEAR_3: return "G3";
	case GEAR_4: return "G4";
	case GEAR_5: return "G5";
	default:     return "--";
	}
}

static void CalDrawGearBadge(GearLevel gear)
{
	ILI9341_FillRect(176, 7, 232, 33, CAL_COLOR_HEADER);
	ILI9341_DrawRect(176, 7, 232, 33, ILI9341_CYAN);
	ILI9341_PutString(188, 8, CalGearText(gear), ILI9341_CYAN, CAL_COLOR_HEADER, 2);
}

static void CalDrawBase(const char *hint)
{
	cal_last_drawn_gear = 0xFF;
	ILI9341_Clear(CAL_COLOR_BG);
	ILI9341_FillRect(0, 0, 239, 39, CAL_COLOR_HEADER);
	ILI9341_PutString(12, 8, "\xE6\xA8\xA1\xE5\xBC\x8F:\xE6\xA0\x87\xE5\xAE\x9A", ILI9341_WHITE, CAL_COLOR_HEADER, 2);
	ILI9341_DrawLine(0, 40, 239, 40, ILI9341_CYAN);
	if (hint != 0)
	{
		ILI9341_PutString(12, 50, hint, ILI9341_YELLOW, CAL_COLOR_BG, 1);
	}
}

static void CalDrawProgress(void)
{
	char buffer[8];

	ILI9341_PutString(12, 74, "\xE5\xAE\x8C\xE6\x88\x90", ILI9341_WHITE, CAL_COLOR_BG, 1);
	for (uint8_t i = 0; i < 5; i++)
	{
		uint16_t x = (uint16_t)(12 + i * 45);
		uint16_t fill = cal_complete[i] ? ILI9341_GREEN : CAL_COLOR_PANEL;
		uint16_t text = cal_complete[i] ? ILI9341_BLACK : ILI9341_WHITE;
		ILI9341_FillRect(x, 96, (uint16_t)(x + 35), 121, fill);
		ILI9341_DrawRect(x, 96, (uint16_t)(x + 35), 121, cal_complete[i] ? ILI9341_GREEN : CAL_COLOR_LINE);
		snprintf(buffer, sizeof(buffer), "G%d", i + 1);
		ILI9341_PutString((uint16_t)(x + 9), 101, buffer, text, fill, 1);
	}
}

static void CalDrawFooter(const char *left, const char *right)
{
	ILI9341_FillRect(0, 286, 239, 319, CAL_COLOR_PANEL_2);
	ILI9341_DrawLine(0, 286, 239, 286, CAL_COLOR_LINE);
	if (left != 0)
	{
		ILI9341_PutString(10, 296, left, ILI9341_WHITE, CAL_COLOR_PANEL_2, 1);
	}
	if (right != 0)
	{
		ILI9341_PutString(132, 296, right, ILI9341_YELLOW, CAL_COLOR_PANEL_2, 1);
	}
}

static void CalDrawGearFocus(GearLevel gear)
{
	char buffer[24];

	ILI9341_FillRect(0, 135, 239, 205, CAL_COLOR_PANEL);
	ILI9341_DrawLine(0, 135, 239, 135, ILI9341_CYAN);
	ILI9341_DrawLine(0, 205, 239, 205, ILI9341_CYAN);
	ILI9341_PutString(16, 148, "\xE6\x8C\xA1\xE4\xBD\x8D:", ILI9341_WHITE, CAL_COLOR_PANEL, 2);
	snprintf(buffer, sizeof(buffer), "%s", CalGearText(gear));
	ILI9341_PutString(126, 146, buffer, ILI9341_CYAN, CAL_COLOR_PANEL, 3);

	ILI9341_FillRect(10, 218, 229, 243, CAL_COLOR_BG);
	if (gear >= GEAR_1 && gear <= GEAR_5 && cal_complete[gear - 1])
	{
		ILI9341_PutString(12, 222, "\xE5\xAE\x8C\xE6\x88\x90  PA10\xE9\x95\xBF\xE6\x8C\x89=\xE5\xA4\x8D\xE4\xBD\x8D", ILI9341_YELLOW, CAL_COLOR_BG, 1);
	}
}

static void CalDrawSpeedBox(uint16_t y, const char *label, double speed, uint16_t color)
{
	char buffer[24];

	ILI9341_FillRect(12, y, 227, (uint16_t)(y + 55), CAL_COLOR_PANEL);
	ILI9341_DrawRect(12, y, 227, (uint16_t)(y + 55), CAL_COLOR_LINE);
	ILI9341_PutString(22, (uint16_t)(y + 6), label, ILI9341_WHITE, CAL_COLOR_PANEL, 1);
	snprintf(buffer, sizeof(buffer), "%4.0f  ", speed);
	ILI9341_PutString(22, (uint16_t)(y + 25), buffer, color, CAL_COLOR_PANEL, 2);
	ILI9341_PutRMin(130, (uint16_t)(y + 29), color, CAL_COLOR_PANEL, 1);
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
						CalDrawBase("PA9=\xE5\xBC\x80\xE5\xA7\x8B");
						CalDrawProgress();
						CalDrawFooter("PA9=\xE5\xBC\x80\xE5\xA7\x8B", "PA10\xE9\x95\xBF\xE6\x8C\x89=\xE5\xA4\x8D\xE4\xBD\x8D");
						cal_ui_dirty = 0;
					}

					if (cal_last_drawn_gear != (uint8_t)gear_now)
					{
						CalDrawGearFocus(gear_now);
						cal_last_drawn_gear = (uint8_t)gear_now;
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
						CalDrawBase("PA9=\xE9\x87\x87\xE9\x9B\x86" "0T");
						CalDrawGearBadge(current_cal_gear);
						CalDrawProgress();
						CalDrawFooter("PA9=\xE9\x87\x87\xE9\x9B\x86" "0T", "PA10=\xE5\x8F\x96\xE6\xB6\x88");
						cal_ui_dirty = 0;
					}

					if (cal_refresh_div == 0)
					{
						double live_speed = GetMotorSpeed();
						CalDrawSpeedBox(150, "0T LIVE", live_speed, ILI9341_GREEN);
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
						CalDrawBase("PA9=\xE7\xA1\xAE\xE8\xAE\xA4");
						CalDrawGearBadge(current_cal_gear);
						CalDrawProgress();
						CalDrawSpeedBox(150, "0T", no_load_speed, ILI9341_GREEN);
						CalDrawFooter("PA9=\xE7\xA1\xAE\xE8\xAE\xA4", "PA10=\xE5\x8F\x96\xE6\xB6\x88");
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
						CalDrawBase("PA9=\xE9\x87\x87\xE9\x9B\x86" "2T");
						CalDrawGearBadge(current_cal_gear);
						CalDrawProgress();
						CalDrawSpeedBox(132, "0T", no_load_speed, ILI9341_YELLOW);
						CalDrawFooter("PA9=\xE9\x87\x87\xE9\x9B\x86" "2T", "PA10=\xE5\x8F\x96\xE6\xB6\x88");
						cal_ui_dirty = 0;
					}

					if (cal_refresh_div == 0)
					{
						double live_speed = GetMotorSpeed();
						CalDrawSpeedBox(204, "2T LIVE", live_speed, ILI9341_GREEN);
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
						const char *action = cal_need_recapture ? "PA9=\xE9\x87\x8D\xE9\x87\x87\xE9\x9B\x86" : "PA9=\xE7\xA1\xAE\xE8\xAE\xA4";
						CalDrawBase(action);
						CalDrawGearBadge(current_cal_gear);
						CalDrawProgress();
						CalDrawSpeedBox(132, "0T", no_load_speed, ILI9341_YELLOW);
						CalDrawSpeedBox(204, "2T", load_2t_speed, cal_need_recapture ? ILI9341_RED : ILI9341_GREEN);
						CalDrawFooter(action, "PA10=\xE5\x8F\x96\xE6\xB6\x88");
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
						CalDrawBase("PA9=\xE8\xBF\x9B\xE5\x85\xA5\xE7\x9B\x91\xE6\xB5\x8B");
						CalDrawProgress();
						ILI9341_FillRect(0, 138, 239, 210, CAL_COLOR_PANEL);
						ILI9341_DrawLine(0, 138, 239, 138, ILI9341_GREEN);
						ILI9341_DrawLine(0, 210, 239, 210, ILI9341_GREEN);
						ILI9341_PutString(24, 158, "\xE5\x85\xA8\xE9\x83\xA8\xE5\xAE\x8C\xE6\x88\x90", ILI9341_GREEN, CAL_COLOR_PANEL, 2);
						CalDrawFooter("PA9=\xE8\xBF\x9B\xE5\x85\xA5", "PA10=\xE5\x8F\x96\xE6\xB6\x88");
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
