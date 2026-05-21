#include "bsp_frequency.h"
#include "bsp_input_capture.h"

// 电机参数
#define STATOR_FREQUENCY 50.0  // 定子频率（Hz）
#define POLE_PAIRS 4           // 极对数（默认值）

// 获取电机转速（RPM）
// 使用公式：n = 60(f1 - f2) / p
// 其中：f1为定子频率（50Hz），f2为转子频率，p为极对数
double GetMotorSpeed(void)
{
	double rotor_frequency = Get_Frequency();  // 转子频率
	// 计算转速
	double speed = 60.0 * (STATOR_FREQUENCY - rotor_frequency) / POLE_PAIRS;
	// 确保转速不为负数
	if (speed < 0.0)
	{
		speed = 0.0;
	}
	return speed;
}