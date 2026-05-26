#include "bsp_weight.h"
#include "stm32f10x_flash.h"

#define CURVE_COUNT 5U
#define FLASH_STORAGE_ADDR 0x0800FC00UL
#define STORAGE_MAGIC 0x5A5AU
#define STORAGE_VERSION 2U
#define STORAGE_VERSION_V1 1U
#define CURVE_VALID_MASK_ALL ((1UL << CURVE_COUNT) - 1UL)
#define MIN_CAL_SPEED_DELTA 5.0f
#define MIN_VALID_SPEED 0.1f
#define MAX_VALID_SLOPE 10000.0f
#define MAX_VALID_INTERCEPT 1000000.0f

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint32_t checksum;
    uint32_t valid_mask;
    WeightCurveParams curves[CURVE_COUNT];
} WeightCurveStorage;

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint32_t checksum;
    WeightCurveParams curves[CURVE_COUNT];
} WeightCurveStorageV1;

static WeightCurveParams weightCurves[CURVE_COUNT];
static uint32_t weightCurveValidMask = 0U;

static uint8_t WeightCurveParamValid(const WeightCurveParams *curve)
{
    if (curve->a == 0.0f)
    {
        return 0;
    }

    if (curve->a > MAX_VALID_SLOPE || curve->a < -MAX_VALID_SLOPE)
    {
        return 0;
    }

    if (curve->b > MAX_VALID_INTERCEPT || curve->b < -MAX_VALID_INTERCEPT)
    {
        return 0;
    }

    return 1;
}

static void ClearWeightCurves(WeightCurveParams *curves)
{
    for (uint32_t i = 0; i < CURVE_COUNT; i++)
    {
        curves[i].a = 0.0f;
        curves[i].b = 0.0f;
    }
}

static uint8_t WeightCurveSetValid(const WeightCurveParams *curves, uint32_t validMask)
{
    for (uint32_t i = 0; i < CURVE_COUNT; i++)
    {
        if ((validMask & (1UL << i)) && !WeightCurveParamValid(&curves[i]))
        {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t WeightCurveSetComplete(const WeightCurveParams *curves)
{
    return WeightCurveSetValid(curves, CURVE_VALID_MASK_ALL);
}

static uint32_t WeightCurveChecksumV1(const WeightCurveParams *curves)
{
    const uint8_t *bytes = (const uint8_t *)curves;
    uint32_t checksum = 2166136261UL;

    for (uint32_t i = 0; i < sizeof(WeightCurveParams) * CURVE_COUNT; i++)
    {
        checksum ^= bytes[i];
        checksum *= 16777619UL;
    }

    return checksum;
}

static uint32_t WeightCurveChecksum(uint32_t validMask, const WeightCurveParams *curves)
{
    const uint8_t *maskBytes = (const uint8_t *)&validMask;
    const uint8_t *curveBytes = (const uint8_t *)curves;
    uint32_t checksum = 2166136261UL;

    for (uint32_t i = 0; i < sizeof(validMask); i++)
    {
        checksum ^= maskBytes[i];
        checksum *= 16777619UL;
    }

    for (uint32_t i = 0; i < sizeof(WeightCurveParams) * CURVE_COUNT; i++)
    {
        checksum ^= curveBytes[i];
        checksum *= 16777619UL;
    }

    return checksum;
}

static uint32_t LoadLegacyCurves(WeightCurveParams *curves)
{
    uint32_t addr = FLASH_STORAGE_ADDR + 2U;
    float *data = (float *)curves;
    uint32_t dataCount = (sizeof(WeightCurveParams) / sizeof(float)) * CURVE_COUNT;

    for (uint32_t i = 0; i < dataCount; i++)
    {
        uint32_t floatData = 0U;
        floatData |= *((uint16_t *)addr);
        addr += 2U;
        floatData |= ((uint32_t)(*((uint16_t *)addr))) << 16;
        addr += 2U;
        data[i] = *((float *)(&floatData));
    }

    return WeightCurveSetComplete(curves) ? CURVE_VALID_MASK_ALL : 0U;
}

static uint32_t LoadStoredCurves(WeightCurveParams *curves)
{
    const WeightCurveStorage *stored = (const WeightCurveStorage *)FLASH_STORAGE_ADDR;
    const WeightCurveStorageV1 *storedV1 = (const WeightCurveStorageV1 *)FLASH_STORAGE_ADDR;
    uint32_t validMask;

    ClearWeightCurves(curves);

    if (stored->magic != STORAGE_MAGIC)
    {
        return 0U;
    }

    if (stored->version == STORAGE_VERSION)
    {
        validMask = stored->valid_mask & CURVE_VALID_MASK_ALL;
        if (stored->valid_mask != validMask)
        {
            return 0U;
        }

        if (stored->checksum != WeightCurveChecksum(validMask, stored->curves))
        {
            return 0U;
        }

        if (!WeightCurveSetValid(stored->curves, validMask))
        {
            return 0U;
        }

        for (uint32_t i = 0; i < CURVE_COUNT; i++)
        {
            curves[i] = stored->curves[i];
        }

        return validMask;
    }

    if (storedV1->version == STORAGE_VERSION_V1)
    {
        if (storedV1->checksum != WeightCurveChecksumV1(storedV1->curves))
        {
            return 0U;
        }

        if (!WeightCurveSetComplete(storedV1->curves))
        {
            return 0U;
        }

        for (uint32_t i = 0; i < CURVE_COUNT; i++)
        {
            curves[i] = storedV1->curves[i];
        }

        return CURVE_VALID_MASK_ALL;
    }

    return LoadLegacyCurves(curves);
}

uint8_t CalibrateWeightCurve(GearLevel gear, double noLoadSpeed, double load2tSpeed)
{
    float delta;
    uint32_t index;

    if (gear < GEAR_1 || gear > GEAR_5)
    {
        return 0;
    }

    if (noLoadSpeed <= 0.0 || load2tSpeed <= 0.0)
    {
        return 0;
    }

    delta = (float)(load2tSpeed - noLoadSpeed);
    if (delta > -MIN_CAL_SPEED_DELTA && delta < MIN_CAL_SPEED_DELTA)
    {
        return 0;
    }

    index = (uint32_t)(gear - 1);
    weightCurves[index].a = 2.0f / delta;
    weightCurves[index].b = -weightCurves[index].a * (float)noLoadSpeed;

    if (!WeightCurveParamValid(&weightCurves[index]))
    {
        weightCurves[index].a = 0.0f;
        weightCurves[index].b = 0.0f;
        weightCurveValidMask &= ~(1UL << index);
        return 0;
    }

    weightCurveValidMask |= (1UL << index);
    return 1;
}

float CalculateWeight(GearLevel gear, double speed)
{
    uint32_t index;
    float weight;

    if (gear < GEAR_1 || gear > GEAR_5)
    {
        return 0.0f;
    }

    if (speed < MIN_VALID_SPEED)
    {
        return 0.0f;
    }

    index = (uint32_t)(gear - 1);
    if ((weightCurveValidMask & (1UL << index)) == 0U)
    {
        return 0.0f;
    }

    if (!WeightCurveParamValid(&weightCurves[index]))
    {
        return 0.0f;
    }

    weight = weightCurves[index].a * (float)speed + weightCurves[index].b;
    if (weight < 0.0f)
    {
        weight = 0.0f;
    }

    return weight;
}

void SaveWeightCurves(void)
{
    FLASH_Status flashStatus;
    uint32_t addr = FLASH_STORAGE_ADDR;
    WeightCurveStorage storage;
    const uint16_t *data = (const uint16_t *)&storage;
    uint32_t dataCount = sizeof(storage) / sizeof(uint16_t);

    storage.magic = STORAGE_MAGIC;
    storage.version = STORAGE_VERSION;
    storage.valid_mask = weightCurveValidMask & CURVE_VALID_MASK_ALL;
    storage.checksum = WeightCurveChecksum(storage.valid_mask, weightCurves);
    for (uint32_t i = 0; i < CURVE_COUNT; i++)
    {
        storage.curves[i] = weightCurves[i];
    }

    FLASH_Unlock();

    flashStatus = FLASH_ErasePage(FLASH_STORAGE_ADDR);
    if (flashStatus != FLASH_COMPLETE)
    {
        FLASH_Lock();
        return;
    }

    for (uint32_t i = 0; i < dataCount; i++)
    {
        flashStatus = FLASH_ProgramHalfWord(addr, data[i]);
        if (flashStatus != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return;
        }
        addr += 2U;
    }

    FLASH_Lock();
}

void LoadWeightCurves(void)
{
    weightCurveValidMask = LoadStoredCurves(weightCurves) & CURVE_VALID_MASK_ALL;
}

uint8_t WeightCurvesValid(void)
{
    return ((weightCurveValidMask & CURVE_VALID_MASK_ALL) == CURVE_VALID_MASK_ALL) ? 1U : 0U;
}

uint8_t GetWeightCurveValidMask(void)
{
    return (uint8_t)(weightCurveValidMask & CURVE_VALID_MASK_ALL);
}

void ClearWeightCurve(GearLevel gear)
{
    uint32_t index;

    if (gear < GEAR_1 || gear > GEAR_5)
    {
        return;
    }

    index = (uint32_t)(gear - 1);
    weightCurves[index].a = 0.0f;
    weightCurves[index].b = 0.0f;
    weightCurveValidMask &= ~(1UL << index);
    SaveWeightCurves();
}

void ClearAllWeightCurves(void)
{
    ClearWeightCurves(weightCurves);
    weightCurveValidMask = 0U;
    SaveWeightCurves();
}
