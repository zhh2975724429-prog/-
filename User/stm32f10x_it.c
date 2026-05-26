/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
  * @brief   Interrupt handlers.
  ******************************************************************************
  */

#include "stm32f10x_it.h"
#include "bsp_direction.h"
#include "bsp_input_capture.h"
#include "timebase.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1)
    {
    }
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
}

void TIM2_IRQHandler(void)
{
    static uint32_t start_ticks = 0;
    uint32_t capture_value;
    uint32_t total_ticks;

    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        capture_overflow_count++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }

    if (TIM_GetITStatus(TIM2, TIM_IT_CC4) != RESET)
    {
        capture_value = TIM_GetCapture4(TIM2);
        total_ticks = (capture_overflow_count << 16) | capture_value;
        capture_last_edge_ms = Timebase_Millis();

        if (capture_flag == 0U)
        {
            capture_start = capture_value;
            start_ticks = total_ticks;
            capture_flag = 1U;
        }
        else if (capture_flag == 1U)
        {
            capture_end = capture_value;
            capture_period_ticks = total_ticks - start_ticks;
            capture_flag = 2U;
        }

        TIM_ClearITPendingBit(TIM2, TIM_IT_CC4);
    }
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        Timebase_TickIrq();
        Direction_Update1ms();
    }
}
