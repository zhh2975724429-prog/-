#ifndef __KEY_SERVICE_H
#define __KEY_SERVICE_H

#include "stm32f10x.h"

extern volatile uint8_t button_pressed;
extern volatile uint8_t button_long_pressed;
extern volatile uint8_t cancel_pressed;
extern volatile uint8_t cancel_long_pressed;

void CheckModeSwitch(void);
void CheckCancelSwitch(void);

#endif
