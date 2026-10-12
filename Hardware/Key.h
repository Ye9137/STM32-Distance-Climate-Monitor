#ifndef __KEY_H
#define __KEY_H

/* 初始化 PB1、PB11 两个低电平有效按键。 */
void Key_Init(void);
/* 阻塞式扫描按键：无按键返回 0，两个按键分别返回 1、2。 */
uint8_t Key_GetNum(void);

#endif
