#ifndef __DELAY_H
#define __DELAY_H

/* 基于 TIM2 微秒计数器实现的阻塞式延时接口。 */
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);

#endif
