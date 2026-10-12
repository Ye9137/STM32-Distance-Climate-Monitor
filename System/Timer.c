#include "stm32f10x.h"                  // STM32 外设库头文件
#include "Timer.h"

void Timer_Init(void)
{
	RCC_ClocksTypeDef RCC_Clocks;
	uint32_t TimerClock;

	/* 开启 APB1 总线上的 TIM2 外设时钟。 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	/* APB1 分频不为 1 时，定时器输入时钟自动倍频。 */
	RCC_GetClocksFreq(&RCC_Clocks);
	TimerClock = RCC_Clocks.PCLK1_Frequency;
	if (RCC_Clocks.PCLK1_Frequency != RCC_Clocks.HCLK_Frequency)
	{
		TimerClock *= 2U;
	}

	TIM_InternalClockConfig(TIM2);		// TIM2 使用内部时钟
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	/* 预分频至 1 MHz，16 位计数器每 1 us 自增一次。 */
	TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;
	TIM_TimeBaseInitStructure.TIM_Prescaler = (TimerClock / 1000000U) - 1U;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, ENABLE);
}

void Timer_Reset(void)
{
	/* 软件需要重新计时时可主动清零。 */
	TIM_SetCounter(TIM2, 0);
}

uint16_t Timer_GetUs(void)
{
	return (uint16_t)TIM_GetCounter(TIM2);
}
