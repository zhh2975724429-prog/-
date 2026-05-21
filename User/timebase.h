#ifndef __TIMEBASE_H
#define __TIMEBASE_H

#include "stm32f10x.h"

void Timebase_Init(void);
uint32_t Timebase_Millis(void);
void Timebase_TickIrq(void);

#endif

