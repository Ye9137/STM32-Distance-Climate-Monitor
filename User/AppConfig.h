#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* 各任务的调度周期与数据有效期，单位均为毫秒。 */
#define DISTANCE_PERIOD_MS 200U
#define DISTANCE_STALE_MS  1500U
#define DHT_PERIOD_MS      2500U
#define DHT_STALE_MS       8000U
#define DISPLAY_PERIOD_MS  100U
#define STARTUP_CALIBRATION_MS 5000U

/* 报警阈值：温湿度单位为 0.1，距离单位为 mm。 */
#define ALARM_TEMP_10       300
#define ALARM_HUMIDITY_10   600U
#define ALARM_DISTANCE_MM   300U
#define LED_HALF_PERIOD_MS  250U
/* OLED 软件 I2C 引脚选择：
 * 1 = 使用原理图中的 PC14/PC15，此时不能安装 LSE 低速外部晶振；
 * 0 = 改用 PB8/PB9，同时需要按该定义重新接线。 */
#define OLED_USE_PC14_PC15  1
#endif
