#include "bsp_switch.h"
#include "gpio.h"

static uint8_t ReadGearBit(GPIO_TypeDef *port, uint16_t pin)
{
    return (GPIO_ReadInputDataBit(port, pin) == 0U) ? 1U : 0U;
}

GearLevel GetCurrentGearRaw(void)
{
    uint8_t bit0 = ReadGearBit(switch0_GPIO_Port, switch0_Pin);
    uint8_t bit1 = ReadGearBit(switch1_GPIO_Port, switch1_Pin);
    uint8_t bit2 = ReadGearBit(switch2_GPIO_Port, switch2_Pin);
    uint8_t bit3 = ReadGearBit(switch3_GPIO_Port, switch3_Pin);
    uint8_t bit4 = ReadGearBit(switch4_GPIO_Port, switch4_Pin);
    uint8_t code = (uint8_t)((bit4 << 4) | (bit3 << 3) | (bit2 << 2) | (bit1 << 1) | bit0);

    switch (code)
    {
        case 0x00:
            return GEAR_1;
        case 0x01:
            return GEAR_2;
        case 0x03:
            return GEAR_3;
        case 0x07:
            return GEAR_4;
        case 0x1F:
            return GEAR_5;
        default:
            return GEAR_INVALID;
    }
}

GearLevel GetCurrentGear(void)
{
    static GearLevel stable_gear = GEAR_INVALID;
    static GearLevel candidate_gear = GEAR_INVALID;
    static uint8_t stable_count = 0;
    GearLevel raw_gear = GetCurrentGearRaw();

    if (raw_gear == candidate_gear)
    {
        if (stable_count < 2U)
        {
            stable_count++;
        }
    }
    else
    {
        candidate_gear = raw_gear;
        stable_count = 1U;
    }

    if (raw_gear != GEAR_INVALID && stable_count >= 2U)
    {
        stable_gear = raw_gear;
    }

    if (raw_gear == GEAR_INVALID)
    {
        return stable_gear;
    }

    return (stable_count >= 2U) ? raw_gear : stable_gear;
}
