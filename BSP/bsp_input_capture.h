#ifndef __BSP_INPUT_CAPTURE_H
#define __BSP_INPUT_CAPTURE_H

#include "stm32f10x.h"

extern volatile uint32_t capture_start;
extern volatile uint32_t capture_end;
extern volatile uint32_t capture_period_ticks;
extern volatile uint32_t capture_overflow_count;
extern volatile uint32_t capture_last_edge_ms;
extern volatile uint8_t capture_flag;
extern volatile uint8_t use_high_precision;

void InputCapture_Init(void);
void Switch_Prescaler(uint8_t high_precision);
double Get_Frequency(void);

#endif
