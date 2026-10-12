#ifndef __TIMER_H
#define __TIMER_H

/* TIM2 配置为 1 MHz 自由运行计数器，每个计数对应 1 us。 */
void Timer_Init(void);
/* 将 TIM2 当前计数值清零。 */
void Timer_Reset(void);
/* 获取 16 位微秒计数值，约每 65.536 ms 回绕一次。 */
uint16_t Timer_GetUs(void);

#endif
