#include "stm32f10x.h"
#include "Timer.h"

/*
 * TIM2 为 16 位计数器，单段延时限制在 60000 us 内；
 * 较长延时拆分为多段，避免一次等待跨越过大的计数范围。
 */
void Delay_us(uint32_t us)
{
    uint16_t start, part;
    while (us) {
        part = us > 60000U ? 60000U : (uint16_t)us;
        start = Timer_GetUs();
        while ((uint16_t)(Timer_GetUs() - start) < part) {}
        us -= part;
    }
}
/* 毫秒延时由 1000 us 延时循环组成。 */
void Delay_ms(uint32_t ms) { while(ms--) Delay_us(1000); }
/* 秒延时由 1000 ms 延时循环组成。 */
void Delay_s(uint32_t seconds) { while(seconds--) Delay_ms(1000); }
