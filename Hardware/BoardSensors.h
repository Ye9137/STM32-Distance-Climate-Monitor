#ifndef BOARD_SENSORS_H
#define BOARD_SENSORS_H
#include <stdint.h>

/* 初始化系统毫秒时基、板载 LED 与 DHT11 数据引脚。 */
void Board_Init(void);
/* 返回自系统启动以来累计的毫秒数。 */
uint32_t Board_Millis(void);
/* 由 SysTick 中断每 1 ms 调用一次。 */
void Board_Tick(void);
/* 控制报警 LED：on 非零时点亮。 */
void Board_LED(uint8_t on);
/* 读取 DHT11，输出值均放大 10 倍；成功返回 1，失败返回 0。 */
uint8_t DHT_Read(int16_t *t10, uint16_t *h10);
#endif
