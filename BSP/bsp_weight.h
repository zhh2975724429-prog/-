#ifndef __BSP_WEIGHT_H
#define __BSP_WEIGHT_H

#include "stm32f10x.h"
#include "bsp_types.h"

typedef struct {
    float a;
    float b;
} WeightCurveParams;

uint8_t CalibrateWeightCurve(GearLevel gear, double noLoadSpeed, double load2tSpeed);
float CalculateWeight(GearLevel gear, double speed);
void SaveWeightCurves(void);
void LoadWeightCurves(void);
uint8_t WeightCurvesValid(void);

#endif
