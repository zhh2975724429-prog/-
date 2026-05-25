#ifndef __GPIO_H
#define __GPIO_H

#include "stm32f10x.h"

void MX_GPIO_Init(void);
void GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

#define GPIO_NUMBER     (16U)
#define LED_Pin GPIO_Pin_13
#define LED_GPIO_Port GPIOC
//PA0��U��PA1��V��PA2��W��PA3��2.5��
#define uCollect_Pin GPIO_Pin_0
#define uCollect_GPIO_Port GPIOA
#define vCollect_Pin GPIO_Pin_1
#define vCollect_GPIO_Port GPIOA
#define wCollect_Pin  GPIO_Pin_2
#define wCollect_GPIO_Port GPIOA
#define zCollect_Pin  GPIO_Pin_3
#define zCollect_GPIO_Port GPIOA

#define switch_Pin GPIO_Pin_9
#define switch_GPIO_Port GPIOA

#define cancel_Pin GPIO_Pin_10
#define cancel_GPIO_Port GPIOA
#define switch0_Pin GPIO_Pin_9
#define switch0_GPIO_Port GPIOB
#define switch1_Pin GPIO_Pin_8
#define switch1_GPIO_Port GPIOB
#define switch2_Pin GPIO_Pin_7
#define switch2_GPIO_Port GPIOB
#define switch3_Pin GPIO_Pin_6
#define switch3_GPIO_Port GPIOB
#define switch4_Pin GPIO_Pin_5
#define switch4_GPIO_Port GPIOB







//#define TX1_Pin GPIO_Pin_9
//#define TX1_GPIO_Port GPIOA


#endif


