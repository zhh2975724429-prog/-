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

#define RGB565(r,g,b)       (uint16_t)((((r) & 0xF8U) << 8) | (((g) & 0xFCU) << 3) | ((b) >> 3))

#define CAL_SCREEN_W        240U
#define CAL_SCREEN_H        320U
#define CAL_TITLE_H         52U
#define CAL_CARD_X          14U
#define CAL_CARD_Y          60U
#define CAL_CARD_W          212U
#define CAL_CARD_H          110U
#define CAL_BTN_W           56U
#define CAL_BTN_H           28U
#define CAL_BTN_ROW1_Y      184U
#define CAL_BTN_ROW2_Y      224U
#define CAL_FOOTER_Y        276U

#define CAL_COLOR_BG        RGB565(8, 12, 24)
#define CAL_COLOR_HEADER_1  RGB565(15, 54, 130)
#define CAL_COLOR_HEADER_2  RGB565(20, 80, 180)
#define CAL_COLOR_CARD      RGB565(20, 28, 45)
#define CAL_COLOR_CARD_DK   RGB565(12, 18, 32)
#define CAL_COLOR_BUTTON    RGB565(23, 34, 54)
#define CAL_COLOR_FOOTER    RGB565(9, 14, 26)
#define CAL_COLOR_LINE      RGB565(68, 88, 120)
#define CAL_COLOR_MUTED     RGB565(150, 174, 206)
#define CAL_COLOR_HILITE    RGB565(0, 185, 255)
#define CAL_COLOR_DONE      RGB565(42, 220, 130)
#define CAL_COLOR_TIP       RGB565(255, 216, 96)

static GearLevel cal_display_gear = GEAR_INVALID;
static const char *cal_status_text = "--";
static uint16_t cal_status_color = CAL_COLOR_MUTED;
static const char *cal_tip_left = "PA9 \xE7\xA1\xAE\xE8\xAE\xA4";
static const char *cal_tip_right = "PA10 \xE9\x95\xBF\xE6\x8C\x89\xE5\xA4\x8D\xE4\xBD\x8D";
static char cal_status_buffer[24];

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

static uint16_t calTextWidth(const char *str, uint8_t size)
{
	uint16_t width = 0U;
	const uint8_t *p = (const uint8_t *)str;

	while (*p)
	{
		if (*p < 0x80U)
		{
			width = (uint16_t)(width + 8U * size);
			p++;
		}
		else
		{
			width = (uint16_t)(width + 16U * size);
			p += 3U;
		}
	}

	return width;
}

static void calPutCentered(uint16_t x, uint16_t y, uint16_t w, const char *str, uint16_t color, uint16_t bg, uint8_t size)
{
	uint16_t text_w = calTextWidth(str, size);
	uint16_t px = x;

	if (text_w < w)
	{
		px = (uint16_t)(x + ((w - text_w) / 2U));
	}

	ILI9341_PutString(px, y, str, color, bg, size);
}

static void setCalibrationUi(GearLevel gear, const char *status, uint16_t status_color, const char *left_tip, const char *right_tip)
{
	cal_display_gear = gear;
	cal_status_text = (status != 0) ? status : "--";
	cal_status_color = status_color;
	cal_tip_left = (left_tip != 0) ? left_tip : "";
	cal_tip_right = (right_tip != 0) ? right_tip : "";
}

static void setCalibrationSpeedUi(GearLevel gear, const char *prefix, double speed, const char *left_tip, const char *right_tip)
{
	snprintf(cal_status_buffer, sizeof(cal_status_buffer), "%s %4.0f r/min", prefix, speed);
	setCalibrationUi(gear, cal_status_buffer, CAL_COLOR_MUTED, left_tip, right_tip);
}

static void setCalibrationSelectUi(GearLevel gear)
{
	if (gear < GEAR_1 || gear > GEAR_5)
	{
		setCalibrationUi(gear, "--", CAL_COLOR_MUTED, "G1-G5", "");
	}
	else if (cal_complete[gear - 1])
	{
		setCalibrationUi(gear, "\xE5\xAE\x8C\xE6\x88\x90", CAL_COLOR_DONE,
			"\xE5\xAE\x8C\xE6\x88\x90", "PA10 \xE9\x95\xBF\xE6\x8C\x89\xE5\xA4\x8D\xE4\xBD\x8D");
	}
	else
	{
		setCalibrationUi(gear, "\xE9\x87\x87\xE9\x9B\x86", CAL_COLOR_MUTED,
			"PA9 \xE5\xBC\x80\xE5\xA7\x8B", "");
	}
}

static void drawButtonFrame(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t fill, uint16_t border, uint8_t strong)
{
	ILI9341_FillRect(x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U), fill);
	ILI9341_DrawRect(x, y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U), border);
	if (strong)
	{
		ILI9341_DrawRect((uint16_t)(x + 1U), (uint16_t)(y + 1U), (uint16_t)(x + w - 2U), (uint16_t)(y + h - 2U), ILI9341_WHITE);
	}
}

static void drawCalibrationTitle(void)
{
	ILI9341_FillRect(0, 0, 239, 25, CAL_COLOR_HEADER_1);
	ILI9341_FillRect(0, 26, 239, (uint16_t)(CAL_TITLE_H - 1U), CAL_COLOR_HEADER_2);
	calPutCentered(0, 3, CAL_SCREEN_W, "\xE6\xA0\x87\xE5\xAE\x9A\xE6\xA8\xA1\xE5\xBC\x8F", ILI9341_WHITE, CAL_COLOR_HEADER_1, 2);
	calPutCentered(0, 34, CAL_SCREEN_W, "Calibration", CAL_COLOR_MUTED, CAL_COLOR_HEADER_2, 1);
}

static void drawCalibrationCard(void)
{
	uint16_t card_x2 = (uint16_t)(CAL_CARD_X + CAL_CARD_W - 1U);
	uint16_t card_y2 = (uint16_t)(CAL_CARD_Y + CAL_CARD_H - 1U);

	ILI9341_FillRect((uint16_t)(CAL_CARD_X + 4U), (uint16_t)(CAL_CARD_Y + 4U), (uint16_t)(card_x2 + 4U), (uint16_t)(card_y2 + 4U), CAL_COLOR_CARD_DK);
	ILI9341_FillRect(CAL_CARD_X, CAL_CARD_Y, card_x2, card_y2, CAL_COLOR_CARD);
	ILI9341_DrawRect(CAL_CARD_X, CAL_CARD_Y, card_x2, card_y2, CAL_COLOR_LINE);
	ILI9341_DrawLine((uint16_t)(CAL_CARD_X + 18U), (uint16_t)(CAL_CARD_Y + 31U), (uint16_t)(card_x2 - 18U), (uint16_t)(CAL_CARD_Y + 31U), RGB565(36, 68, 105));

	calPutCentered(CAL_CARD_X, (uint16_t)(CAL_CARD_Y + 9U), CAL_CARD_W, "\xE6\x8C\xA1\xE4\xBD\x8D", CAL_COLOR_MUTED, CAL_COLOR_CARD, 1);
	calPutCentered(CAL_CARD_X, (uint16_t)(CAL_CARD_Y + 34U), CAL_CARD_W, CalGearText(cal_display_gear), CAL_COLOR_HILITE, CAL_COLOR_CARD, 4);
	calPutCentered(CAL_CARD_X, (uint16_t)(CAL_CARD_Y + 88U), CAL_CARD_W, cal_status_text, cal_status_color, CAL_COLOR_CARD, 1);
}

static void drawGearButtons(void)
{
	static const uint16_t btn_x[5] = {18U, 92U, 166U, 55U, 129U};
	static const uint16_t btn_y[5] = {CAL_BTN_ROW1_Y, CAL_BTN_ROW1_Y, CAL_BTN_ROW1_Y, CAL_BTN_ROW2_Y, CAL_BTN_ROW2_Y};
	char label[4];

	ILI9341_FillRect(0, 176, 239, 264, CAL_COLOR_BG);
	for (uint8_t i = 0; i < 5; i++)
	{
		GearLevel gear = (GearLevel)(i + 1U);
		uint8_t selected = (gear == cal_display_gear) ? 1U : 0U;
		uint8_t done = cal_complete[i] ? 1U : 0U;
		uint16_t fill = selected ? CAL_COLOR_HILITE : CAL_COLOR_BUTTON;
		uint16_t border = selected ? ILI9341_WHITE : (done ? CAL_COLOR_DONE : CAL_COLOR_LINE);
		uint16_t text = selected ? ILI9341_WHITE : (done ? CAL_COLOR_DONE : ILI9341_WHITE);

		drawButtonFrame(btn_x[i], btn_y[i], CAL_BTN_W, CAL_BTN_H, fill, border, selected);
		snprintf(label, sizeof(label), "G%d", i + 1);
		calPutCentered(btn_x[i], (uint16_t)(btn_y[i] + 6U), CAL_BTN_W, label, text, fill, 1);
	}
}

static void drawBottomTips(void)
{
	ILI9341_FillRect(0, CAL_FOOTER_Y, 239, 319, CAL_COLOR_FOOTER);
	ILI9341_DrawLine(0, CAL_FOOTER_Y, 239, CAL_FOOTER_Y, CAL_COLOR_LINE);
	ILI9341_PutString(12, 292, cal_tip_left, CAL_COLOR_MUTED, CAL_COLOR_FOOTER, 1);
	ILI9341_PutString(126, 292, cal_tip_right, CAL_COLOR_TIP, CAL_COLOR_FOOTER, 1);
}

static void drawCalibrationScreen(void)
{
	ILI9341_Clear(CAL_COLOR_BG);
	drawCalibrationTitle();
	drawCalibrationCard();
	drawGearButtons();
	drawBottomTips();
	cal_last_drawn_gear = (uint8_t)cal_display_gear;
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

	{
		uint8_t curve_mask = GetWeightCurveValidMask();

		for (int i = 0; i < 5; i++)
		{
			cal_complete[i] = ((curve_mask & (1U << i)) != 0U) ? 1U : 0U;
		}

		if (WeightCurvesValid())
		{
			CalibSetState(CAL_STATE_ALL_DONE);
		}
		else
		{
			CalibSetState(CAL_STATE_SELECT_GEAR);
		}
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
						setCalibrationSelectUi(gear_now);
						drawCalibrationScreen();
						cal_ui_dirty = 0;
					}

					if (cal_last_drawn_gear != (uint8_t)gear_now)
					{
						setCalibrationSelectUi(gear_now);
						drawCalibrationCard();
						drawGearButtons();
						drawBottomTips();
						cal_last_drawn_gear = (uint8_t)gear_now;
					}

					if (cancel_pressed)
					{
						cancel_pressed = 0;
					}

					if (cancel_long_pressed)
					{
						cancel_long_pressed = 0;
						if (gear_now != GEAR_INVALID && cal_complete[gear_now - 1])
						{
							cal_complete[gear_now - 1] = 0;
							ClearWeightCurve(gear_now);
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
						else
						{
							setCalibrationUi(GEAR_INVALID, "NO GEAR", ILI9341_RED, "G1-G5", "");
							drawCalibrationCard();
							drawBottomTips();
							cal_last_drawn_gear = (uint8_t)gear_now;
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
						setCalibrationSpeedUi(current_cal_gear, "0T", GetMotorSpeed(),
							"PA9 \xE9\x87\x87\xE9\x9B\x86", "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						drawCalibrationScreen();
						cal_ui_dirty = 0;
					}

					if (cal_refresh_div == 0)
					{
						double live_speed = GetMotorSpeed();
						setCalibrationSpeedUi(current_cal_gear, "0T", live_speed,
							"PA9 \xE9\x87\x87\xE9\x9B\x86", "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						drawCalibrationCard();
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
						setCalibrationSpeedUi(current_cal_gear, "0T", no_load_speed,
							"PA9 \xE7\xA1\xAE\xE8\xAE\xA4", "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						drawCalibrationScreen();
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
						setCalibrationSpeedUi(current_cal_gear, "2T", GetMotorSpeed(),
							"PA9 \xE9\x87\x87\xE9\x9B\x86", "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						drawCalibrationScreen();
						cal_ui_dirty = 0;
					}

					if (cal_refresh_div == 0)
					{
						double live_speed = GetMotorSpeed();
						setCalibrationSpeedUi(current_cal_gear, "2T", live_speed,
							"PA9 \xE9\x87\x87\xE9\x9B\x86", "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						drawCalibrationCard();
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
						const char *action = cal_need_recapture ? "PA9 \xE9\x87\x8D\xE9\x87\x87\xE9\x9B\x86" : "PA9 \xE7\xA1\xAE\xE8\xAE\xA4";
						if (cal_need_recapture)
						{
							setCalibrationUi(current_cal_gear, "\xE9\x87\x8D\xE9\x87\x87\xE9\x9B\x86", ILI9341_RED,
								action, "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						}
						else
						{
							setCalibrationSpeedUi(current_cal_gear, "2T", load_2t_speed,
								action, "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						}
						drawCalibrationScreen();
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
							setCalibrationUi(current_cal_gear, "\xE9\x87\x8D\xE9\x87\x87\xE9\x9B\x86", ILI9341_RED,
								"PA9 \xE9\x87\x8D\xE9\x87\x87\xE9\x9B\x86", "PA10 \xE5\x8F\x96\xE6\xB6\x88");
							drawCalibrationCard();
							drawBottomTips();
							cal_ui_dirty = 0;
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
						ClearAllWeightCurves();
						current_cal_gear = GEAR_INVALID;
						no_load_speed = 0.0;
						load_2t_speed = 0.0;
						CalibSetState(CAL_STATE_SELECT_GEAR);
						break;
					}
					if (cal_ui_dirty)
					{
						setCalibrationUi(gear_now_cached, "\xE5\x85\xA8\xE9\x83\xA8\xE5\xAE\x8C\xE6\x88\x90", CAL_COLOR_DONE,
							"PA9 \xE8\xBF\x9B\xE5\x85\xA5", "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						drawCalibrationScreen();
						cal_ui_dirty = 0;
					}

					if (cal_last_drawn_gear != (uint8_t)gear_now_cached)
					{
						setCalibrationUi(gear_now_cached, "\xE5\x85\xA8\xE9\x83\xA8\xE5\xAE\x8C\xE6\x88\x90", CAL_COLOR_DONE,
							"PA9 \xE8\xBF\x9B\xE5\x85\xA5", "PA10 \xE5\x8F\x96\xE6\xB6\x88");
						drawCalibrationCard();
						drawGearButtons();
						cal_last_drawn_gear = (uint8_t)gear_now_cached;
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
