#ifndef __BSP_TYPES_H
#define __BSP_TYPES_H

// 挡位定义
typedef enum {
    GEAR_INVALID = 0,  // 无效挡位
    GEAR_1 = 1,        // 1挡
    GEAR_2 = 2,        // 2挡
    GEAR_3 = 3,        // 3挡
    GEAR_4 = 4,        // 4挡
    GEAR_5 = 5         // 5挡
} GearLevel;

// 方向定义
typedef enum {
    DIRECTION_CW = 0,      // 顺时针（正转）
    DIRECTION_CCW = 1,     // 逆时针（反转）
    DIRECTION_STOP = 2     // 停止
} MotorDirection;

#endif