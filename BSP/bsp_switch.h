#ifndef __BSP_SWITCH_H
#define __BSP_SWITCH_H

#include "stm32f10x.h"
#include "bsp_types.h"

GearLevel GetCurrentGear(void);
GearLevel GetCurrentGearRaw(void);

#endif
