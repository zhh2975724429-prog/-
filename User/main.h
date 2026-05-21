#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f10x.h"
#include "Delay.h"
#include "gpio.h"
#include "key_service.h"
#include "stdio.h"

#include "bsp_switch.h"
#include "bsp_direction.h"
#include "bsp_frequency.h"
#include "bsp_input_capture.h"
#include "bsp_weight.h"
#include "bsp_ili9341.h"
#include "bsp_types.h"

// 系统模式定义
typedef enum {
    MODE_CALIBRATION = 0,  // 标定模式
    MODE_MONITOR = 1       // 监测模式
} SystemMode;

// 全局变量
extern volatile SystemMode current_mode;
extern volatile uint8_t mode_switch_flag;

// 函数声明
void SetSysClockTo72(void);
void Error(void);

#endif

