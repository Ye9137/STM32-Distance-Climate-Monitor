#ifndef __OLED_H
#define __OLED_H

/* 中文字模索引，排列顺序必须与 OLED_Font.h 中的字模表完全一致。 */
typedef enum {
	OLED_CH_DISTANCE = 0, /* 距 */
	OLED_CH_LEAVE,        /* 离 */
	OLED_CH_TEMP,         /* 温 */
	OLED_CH_DEGREE,       /* 度 */
	OLED_CH_HUMID,        /* 湿 */
	OLED_CH_WRONG,        /* 错 */
	OLED_CH_ERROR,        /* 误 */
	OLED_CH_EXCEED,       /* 超 */
	OLED_CH_QUANTITY,     /* 量 */
	OLED_CH_RANGE,        /* 程 */
	OLED_CH_ALARM,        /* 报 */
	OLED_CH_WARNING,      /* 警 */
	OLED_CH_LAMP,         /* 灯 */
	OLED_CH_FLASH,        /* 闪 */
	OLED_CH_FLICKER,      /* 烁 */
	OLED_CH_STATE,        /* 状 */
	OLED_CH_ATTITUDE,     /* 态 */
	OLED_CH_POSITIVE,     /* 正 */
	OLED_CH_USUAL,        /* 常 */
	OLED_CH_AT,           /* 在 */
	OLED_CH_START,        /* 启 */
	OLED_CH_MOTION,       /* 动 */
	OLED_CH_CALIBRATE,    /* 校 */
	OLED_CH_ACCURATE,     /* 准 */
	OLED_CH_COUNT         /* 中文字模总数 */
} OLED_ChineseChar;

/* OLED 基础显示接口，行列参数均从 1 开始。 */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t Index);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
