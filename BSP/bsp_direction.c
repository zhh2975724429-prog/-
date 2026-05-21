#include "bsp_direction.h"
#include "gpio.h"

// 方向判断函数
// 原理：通过检测U相上升沿时V相的状态来判断转向
// - 当U相上升沿时V相为低电平，电机正转（顺时针）
// - 当U相上升沿时V相为高电平，电机反转（逆时针）
// - 如果信号稳定不变，判断为停止状态
// 添加状态保持机制，避免显示闪烁
MotorDirection GetMotorDirection(void)
{
	static uint8_t last_u_state = 1;
	static uint32_t same_state_count = 0;
	static MotorDirection last_direction = DIRECTION_STOP;  // 状态保持
	
	// 读取当前U相和V相的状态
	uint8_t current_u_state = GPIO_ReadInputDataBit(uCollect_GPIO_Port, uCollect_Pin);
	uint8_t current_v_state = GPIO_ReadInputDataBit(vCollect_GPIO_Port, vCollect_Pin);
	
	// 检查信号是否稳定（停止状态）
	if (current_u_state == last_u_state)
	{
		same_state_count++;
		if (same_state_count > 100)  // 连续检测到100次相同状态
		{
			last_direction = DIRECTION_STOP;
		}
	}
	else
	{
		same_state_count = 0;
		// 检测到U相的边沿变化
		if (last_u_state == 0 && current_u_state == 1)  // U相上升沿
		{
			// 根据V相的状态判断方向
			if (current_v_state == 0)
			{
				last_direction = DIRECTION_CW;  // 正转
			}
			else
			{
				last_direction = DIRECTION_CCW;  // 反转
			}
		}
	}
	
	last_u_state = current_u_state;
	return last_direction;  // 返回保持的状态
}
