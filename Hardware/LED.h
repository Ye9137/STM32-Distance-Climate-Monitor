#ifndef __LED_H
#define __LED_H

/* PA1、PA2 两路 LED 均为低电平点亮。 */
void LED_Init(void);
void LED1_ON(void);
void LED1_OFF(void);
void LED1_Turn(void);
void LED2_ON(void);
void LED2_OFF(void);
void LED2_Turn(void);

#endif
