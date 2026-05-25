#include "gpio.h"


//GPIO��ʼ������
void MX_GPIO_Init(void)
{
	//�ṹ�嶨��
	GPIO_InitTypeDef GPIO_InitStruct={0};
	
	//ʹ�ܸ�����ʱ��
	//使能各端口时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD,ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE,ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF,ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG,ENABLE);
	
	//�����ų�ʼ��
	GPIO_WriteBit(LED_GPIO_Port,LED_Pin,Bit_SET);
	
	//��������
	GPIO_InitStruct.GPIO_Pin=LED_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(LED_GPIO_Port,&GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin=uCollect_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_Init(uCollect_GPIO_Port,&GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin=vCollect_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_Init(vCollect_GPIO_Port,&GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin=wCollect_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_Init(wCollect_GPIO_Port,&GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin=zCollect_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_Init(zCollect_GPIO_Port,&GPIO_InitStruct);
	
	/* Keys are active-high: idle low, pressed high. */
	GPIO_InitStruct.GPIO_Pin=switch_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_Init(switch_GPIO_Port,&GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin=cancel_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPD;
	GPIO_Init(cancel_GPIO_Port,&GPIO_InitStruct);

	/* Gear inputs use the raw binary level: low = 0, high = 1. */
	GPIO_InitStruct.GPIO_Pin=switch0_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(switch0_GPIO_Port,&GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin=switch1_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(switch1_GPIO_Port,&GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin=switch2_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(switch2_GPIO_Port,&GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin=switch3_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(switch3_GPIO_Port,&GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin=switch4_Pin;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(switch4_GPIO_Port,&GPIO_InitStruct);

	// ILI9341显示屏引脚初始化
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	
	// SPI2引脚初始化（PB13=SCK, PB14=MISO, PB15=MOSI）
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
}

//��ƽ��ת����
void GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  uint32_t odr;

  /* ʹ�ö���ȷ����������Ų�������Ч��GPIO���� */
  assert_param(IS_GPIO_PIN(GPIO_Pin));

  /* ��ȡ��ǰ���״̬ */
  odr = GPIOx->ODR;

  /* ���㷭תֵ��д��BSRR�Ĵ��� */
  GPIOx->BSRR = ((odr & GPIO_Pin) << GPIO_NUMBER) | (~odr & GPIO_Pin);
}
