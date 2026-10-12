#ifndef __HCSR04_H
#define __HCSR04_H

/* HC-SR04 与 STM32 的引脚连接。 */
#define Trig_Port 		GPIOA
#define Trig_Pin 		GPIO_Pin_2
#define Trig_RCC		RCC_APB2Periph_GPIOA

#define Echo_Port 		GPIOA
#define Echo_Pin 		GPIO_Pin_3
#define Echo_RCC		RCC_APB2Periph_GPIOA

typedef enum
{
	HCSR04_OK = 0,          /* 测量成功 */
	HCSR04_NO_ECHO,         /* 未检测到完整回波 */
	HCSR04_OUT_OF_RANGE     /* 回波存在，但距离超出 20~4000 mm */
} HCSR04_Status;

/* 初始化 TRIG 输出和 ECHO 输入引脚。 */
void HCSR04_Init(void);
/* 读取距离（毫米），并返回本次测量状态。 */
HCSR04_Status HCSR04_ReadDistanceMm(uint16_t *DistanceMm);

#endif 
