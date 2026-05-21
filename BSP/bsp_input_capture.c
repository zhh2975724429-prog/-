#include "bsp_input_capture.h"
#include "timebase.h"

#define LOW_PRECISION_TIMER_HZ 10000.0
#define HIGH_PRECISION_TIMER_HZ 100000.0
#define MEASURE_TIMEOUT_MS 800U
#define MAX_MEASURE_TIMEOUT_MS 3000U

volatile uint32_t capture_start = 0;
volatile uint32_t capture_end = 0;
volatile uint32_t capture_period_ticks = 0;
volatile uint32_t capture_overflow_count = 0;
volatile uint32_t capture_last_edge_ms = 0;
volatile uint8_t capture_flag = 0;
volatile uint8_t use_high_precision = 0;

static void ResetCaptureState(void)
{
    __disable_irq();
    capture_start = 0;
    capture_end = 0;
    capture_period_ticks = 0;
    capture_overflow_count = 0;
    capture_flag = 0;
    capture_last_edge_ms = Timebase_Millis();
    __enable_irq();
}

static uint32_t GetCaptureTimeoutMs(double last_frequency)
{
    uint32_t timeout_ms = MEASURE_TIMEOUT_MS;

    if (last_frequency > 0.1)
    {
        double period_ms = 1000.0 / last_frequency;
        timeout_ms = (uint32_t)(period_ms * 2.2 + 200.0);
        if (timeout_ms < MEASURE_TIMEOUT_MS)
        {
            timeout_ms = MEASURE_TIMEOUT_MS;
        }
        if (timeout_ms > MAX_MEASURE_TIMEOUT_MS)
        {
            timeout_ms = MAX_MEASURE_TIMEOUT_MS;
        }
    }

    return timeout_ms;
}

void InputCapture_Init(void)
{
    GPIO_InitTypeDef gpioInitStruct;
    TIM_TimeBaseInitTypeDef timeBaseInitStruct;
    TIM_ICInitTypeDef icInitStruct;
    NVIC_InitTypeDef nvicInitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    gpioInitStruct.GPIO_Pin = GPIO_Pin_3;
    gpioInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpioInitStruct);

    timeBaseInitStruct.TIM_Period = 0xFFFF;
    timeBaseInitStruct.TIM_Prescaler = 7199;
    timeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    timeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &timeBaseInitStruct);

    icInitStruct.TIM_Channel = TIM_Channel_4;
    icInitStruct.TIM_ICPolarity = TIM_ICPolarity_Rising;
    icInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
    icInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    icInitStruct.TIM_ICFilter = 0x8;
    TIM_ICInit(TIM2, &icInitStruct);

    nvicInitStruct.NVIC_IRQChannel = TIM2_IRQn;
    nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
    nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicInitStruct);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update | TIM_FLAG_CC4);
    TIM_ITConfig(TIM2, TIM_IT_Update | TIM_IT_CC4, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    use_high_precision = 0;
    ResetCaptureState();
}

void Switch_Prescaler(uint8_t high_precision)
{
    TIM_TimeBaseInitTypeDef timeBaseInitStruct;

    TIM_Cmd(TIM2, DISABLE);

    timeBaseInitStruct.TIM_Period = 0xFFFF;
    timeBaseInitStruct.TIM_Prescaler = high_precision ? 719U : 7199U;
    timeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    timeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &timeBaseInitStruct);

    TIM_SetCounter(TIM2, 0);
    TIM_ClearFlag(TIM2, TIM_FLAG_Update | TIM_FLAG_CC4);
    use_high_precision = high_precision ? 1U : 0U;
    ResetCaptureState();

    TIM_Cmd(TIM2, ENABLE);
}

double Get_Frequency(void)
{
    static double last_frequency = 0.0;
    static uint8_t first_measurement = 1U;
    uint32_t now = Timebase_Millis();
    uint32_t last_edge_ms;
    uint32_t period_ticks;
    uint8_t ready;
    uint8_t high_precision;
    double timer_hz;
    double frequency;

    __disable_irq();
    last_edge_ms = capture_last_edge_ms;
    period_ticks = capture_period_ticks;
    ready = (capture_flag == 2U && period_ticks != 0U) ? 1U : 0U;
    high_precision = use_high_precision;
    if (ready)
    {
        capture_flag = 0U;
    }
    __enable_irq();

    if ((uint32_t)(now - last_edge_ms) > GetCaptureTimeoutMs(last_frequency))
    {
        ResetCaptureState();
        last_frequency = 0.0;
        first_measurement = 1U;
        return 0.0;
    }

    if (!ready)
    {
        return last_frequency;
    }

    timer_hz = high_precision ? HIGH_PRECISION_TIMER_HZ : LOW_PRECISION_TIMER_HZ;
    frequency = timer_hz / (double)period_ticks;

    if (frequency < 0.1 || frequency > 1000.0)
    {
        return last_frequency;
    }

    if (first_measurement || (!high_precision && frequency > 5.0))
    {
        first_measurement = 0U;
        if (frequency > 5.0)
        {
            Switch_Prescaler(1U);
            return last_frequency;
        }
    }
    else if (high_precision && frequency <= 3.0)
    {
        Switch_Prescaler(0U);
        return last_frequency;
    }

    last_frequency = frequency;
    return last_frequency;
}
