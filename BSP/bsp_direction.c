#include "bsp_direction.h"
#include "gpio.h"

#define DIRECTION_STABLE_STEPS 3U
#define DIRECTION_STOP_TIMEOUT_MS 300U
#define DIRECTION_INVALID_STATE 0xFFU

static volatile MotorDirection stable_direction = DIRECTION_STOP;
static uint8_t last_state = DIRECTION_INVALID_STATE;
static uint8_t cw_score = 0U;
static uint8_t ccw_score = 0U;
static uint16_t no_transition_ms = 0U;

static uint8_t ReadPhaseState(void)
{
	uint8_t u = GPIO_ReadInputDataBit(uCollect_GPIO_Port, uCollect_Pin);
	uint8_t v = GPIO_ReadInputDataBit(vCollect_GPIO_Port, vCollect_Pin);
	uint8_t w = GPIO_ReadInputDataBit(wCollect_GPIO_Port, wCollect_Pin);

	return (uint8_t)((u << 2) | (v << 1) | w);
}

static int8_t PhaseStateIndex(uint8_t state)
{
	static const uint8_t sequence[6] = {0x01U, 0x05U, 0x04U, 0x06U, 0x02U, 0x03U};

	for (uint8_t i = 0U; i < 6U; i++)
	{
		if (sequence[i] == state)
		{
			return (int8_t)i;
		}
	}

	return -1;
}

static int8_t DirectionStep(uint8_t from_state, uint8_t to_state)
{
	int8_t from = PhaseStateIndex(from_state);
	int8_t to = PhaseStateIndex(to_state);

	if (from < 0 || to < 0)
	{
		return 0;
	}

	if (((from + 1) % 6) == to)
	{
		return 1;
	}

	if (((from + 5) % 6) == to)
	{
		return -1;
	}

	return 0;
}

static void DirectionMarkStopIfTimedOut(void)
{
	if (no_transition_ms < DIRECTION_STOP_TIMEOUT_MS)
	{
		no_transition_ms++;
	}
	else
	{
		cw_score = 0U;
		ccw_score = 0U;
		stable_direction = DIRECTION_STOP;
	}
}

static void DirectionVote(int8_t step)
{
	if (step > 0)
	{
		ccw_score = 0U;
		if (cw_score < DIRECTION_STABLE_STEPS)
		{
			cw_score++;
		}
		if (cw_score >= DIRECTION_STABLE_STEPS)
		{
			stable_direction = DIRECTION_CW;
		}
	}
	else if (step < 0)
	{
		cw_score = 0U;
		if (ccw_score < DIRECTION_STABLE_STEPS)
		{
			ccw_score++;
		}
		if (ccw_score >= DIRECTION_STABLE_STEPS)
		{
			stable_direction = DIRECTION_CCW;
		}
	}
}

void Direction_Update1ms(void)
{
	uint8_t state = ReadPhaseState();
	int8_t step;

	if (PhaseStateIndex(state) < 0)
	{
		DirectionMarkStopIfTimedOut();
		return;
	}

	if (last_state == DIRECTION_INVALID_STATE)
	{
		last_state = state;
		DirectionMarkStopIfTimedOut();
		return;
	}

	if (state == last_state)
	{
		DirectionMarkStopIfTimedOut();
		return;
	}

	step = DirectionStep(last_state, state);
	last_state = state;

	if (step == 0)
	{
		DirectionMarkStopIfTimedOut();
		return;
	}

	no_transition_ms = 0U;
	DirectionVote(step);
}

MotorDirection GetMotorDirection(void)
{
	return stable_direction;
}
