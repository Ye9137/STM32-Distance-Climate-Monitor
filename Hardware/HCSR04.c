#include "stm32f10x.h"                  // STM32 外设库头文件
#include "HCSR04.h"
#include "Timer.h"
#include "Delay.h"

void HCSR04_Init(void)
{
	/* 同时开启 TRIG 与 ECHO 所在 GPIO 端口时钟。 */
	RCC_APB2PeriphClockCmd(Trig_RCC | Echo_RCC, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = Trig_Pin;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Trig_Port, &GPIO_InitStruct);
	
	/* HC-SR04 主动把 ECHO 拉低，不需要内部下拉。 */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Pin = Echo_Pin;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Echo_Port, &GPIO_InitStruct);
	
	GPIO_ResetBits(Trig_Port, Trig_Pin);
}

static uint8_t HCSR04_WaitForLevel(BitAction Level, uint16_t TimeoutUs)
{
	/* 使用 16 位自由运行计数器，无符号相减可自然处理计数回绕。 */
	uint16_t StartUs = Timer_GetUs();
	while (GPIO_ReadInputDataBit(Echo_Port, Echo_Pin) != Level)
	{
		if ((uint16_t)(Timer_GetUs() - StartUs) >= TimeoutUs)
		{
			return 0;
		}
	}
	return 1;
}

HCSR04_Status HCSR04_ReadDistanceMm(uint16_t *DistanceMm)
{
	uint16_t PulseUs;
	uint16_t PulseStartUs;
	uint32_t Distance;

	/* 防止调用者传入空指针。 */
	if (DistanceMm == 0)
	{
		return HCSR04_NO_ECHO;
	}

	/* 等待上一次回波结束，防止把残留高电平当成本次测量。 */
	if (!HCSR04_WaitForLevel(Bit_RESET, 3000))
	{
		return HCSR04_NO_ECHO;
	}

	/* HC-SR04 要求至少 10us 的高电平触发脉冲。 */
	GPIO_SetBits(Trig_Port, Trig_Pin);
	Delay_us(12);
	GPIO_ResetBits(Trig_Port, Trig_Pin);

	/* 最长等待约 30ms；无回波时也能正常退出，不会卡死。 */
	if (!HCSR04_WaitForLevel(Bit_SET, 30000))
	{
		return HCSR04_NO_ECHO;
	}

	PulseStartUs = Timer_GetUs();
	while (GPIO_ReadInputDataBit(Echo_Port, Echo_Pin) == Bit_SET)
	{
		if ((uint16_t)(Timer_GetUs() - PulseStartUs) >= 30000U)
		{
			return HCSR04_NO_ECHO;
		}
	}
	PulseUs = (uint16_t)(Timer_GetUs() - PulseStartUs);

	/* 距离(mm) = 回波时间(us) * 343m/s / 2，整数四舍五入。 */
	Distance = ((uint32_t)PulseUs * 343U + 1000U) / 2000U;
	if (Distance < 20U || Distance > 4000U)
	{
		return HCSR04_OUT_OF_RANGE;
	}

	*DistanceMm = (uint16_t)Distance;
	return HCSR04_OK;
}
