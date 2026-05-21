#include "key_service.h"
#include "gpio.h"
#include "main.h"
#include "timebase.h"

volatile uint8_t button_pressed = 0;
volatile uint8_t button_long_pressed = 0;
volatile uint8_t cancel_pressed = 0;
volatile uint8_t cancel_long_pressed = 0;

typedef struct {
	uint8_t inited;
	uint8_t raw_last;
	uint8_t stable_state;
	uint32_t stable_ms;
	uint32_t press_ms;
	uint8_t handled;
	uint32_t last_ms;
} KeyState;

static uint8_t KeyUpdate(KeyState *key, uint8_t raw, uint32_t debounce_ms, uint32_t longpress_ms)
{
	uint32_t now = Timebase_Millis();
	uint32_t dt = now - key->last_ms;
	if (dt > 1000U) dt = 1000U;
	key->last_ms = now;

	if (!key->inited)
	{
		key->inited = 1U;
		key->raw_last = raw;
		key->stable_state = raw;
		key->last_ms = now;
		return 0U;
	}

	if (raw != key->raw_last)
	{
		key->raw_last = raw;
		key->stable_ms = 0U;
	}
	else if (key->stable_ms < debounce_ms)
	{
		key->stable_ms += dt;
	}

	if (key->stable_ms >= debounce_ms && raw != key->stable_state)
	{
		key->stable_state = raw;
		if (key->stable_state == 0U)
		{
			uint8_t is_short = (!key->handled && key->press_ms > 0U && key->press_ms < longpress_ms);
			key->press_ms = 0U;
			key->handled = 0U;
			return is_short ? 1U : 0U;
		}

		key->press_ms = 0U;
		key->handled = 0U;
	}

	if (key->stable_state == 1U)
	{
		if (key->press_ms < longpress_ms) key->press_ms += dt;
		if (!key->handled && key->press_ms >= longpress_ms)
		{
			key->handled = 1U;
			return 2U;
		}
	}

	return 0U;
}

void CheckModeSwitch(void)
{
	static KeyState mode_key = {0};
	uint8_t event = KeyUpdate(&mode_key, GPIO_ReadInputDataBit(switch_GPIO_Port, switch_Pin), 50U, 1500U);

	if (event == 1U)
	{
		button_pressed = 1U;
	}
	else if (event == 2U)
	{
		button_long_pressed = 1U;
		current_mode = (current_mode == MODE_CALIBRATION) ? MODE_MONITOR : MODE_CALIBRATION;
		mode_switch_flag = 1U;
	}
}

void CheckCancelSwitch(void)
{
	static KeyState cancel_key = {0};
	uint8_t event = KeyUpdate(&cancel_key, GPIO_ReadInputDataBit(cancel_GPIO_Port, cancel_Pin), 50U, 800U);

	if (event == 1U)
	{
		cancel_pressed = 1U;
	}
	else if (event == 2U)
	{
		cancel_long_pressed = 1U;
	}
}
