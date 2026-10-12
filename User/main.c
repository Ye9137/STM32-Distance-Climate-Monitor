#include "stm32f10x.h"
#include "OLED.h"
#include "BoardSensors.h"
#include "HCSR04.h"
#include "AppConfig.h"
#include <stdio.h>
#include <string.h>

/*
 * frame 保存下一帧逻辑内容，shown 保存屏幕上已显示的内容。
 * 两者逐格比较后只刷新变化位置，以缩短软件 I2C 的占用时间。
 */
static uint8_t frame[4][16];
static uint8_t shown[4][16];
static uint8_t screen_cursor;

/* 0x80 起表示中文字模索引，0x7F 表示中文字符占用的第二列。 */
#define UI_CH_BASE   0x80U
#define UI_CH_CONT   0x7FU

/* 用空格清空待显示帧，保证旧字符能够被覆盖。 */
static void Frame_Clear(void)
{
    memset(frame, ' ', sizeof(frame));
}

/* 从指定位置写入 ASCII 字符串，并返回写入后的下一列。 */
static uint8_t Frame_PutAscii(uint8_t row, uint8_t column, const char *text)
{
    while (column < 16U && *text != '\0')
    {
        frame[row][column++] = (uint8_t)*text++;
    }
    return column;
}

/* 写入一个 16x16 中文字形；每个中文字占两个 8 像素字符列。 */
static uint8_t Frame_PutChinese(uint8_t row, uint8_t column, uint8_t glyph)
{
    if (column > 14U || glyph >= OLED_CH_COUNT)
    {
        return column;
    }
    frame[row][column] = (uint8_t)(UI_CH_BASE + glyph);
    frame[row][column + 1U] = UI_CH_CONT;
    return (uint8_t)(column + 2U);
}

/* 每次只刷新一个发生变化的字形，避免软件 I2C 长时间阻塞传感器时序。 */
static void Display_Service(void)
{
    uint8_t i;
    uint8_t row;
    uint8_t column;
    uint8_t token;

    for (i = 0; i < 64U; i++)
    {
        row = screen_cursor / 16U;
        column = screen_cursor % 16U;
        screen_cursor = (screen_cursor + 1U) % 64U;

        if (shown[row][column] != frame[row][column])
        {
            token = frame[row][column];
            if (token >= UI_CH_BASE &&
                token < (uint8_t)(UI_CH_BASE + OLED_CH_COUNT))
            {
                OLED_ShowChinese(row + 1U, column + 1U,
                                 (uint8_t)(token - UI_CH_BASE));
                shown[row][column] = token;
                if (column < 15U)
                {
                    shown[row][column + 1U] = UI_CH_CONT;
                }
            }
            else if (token == UI_CH_CONT)
            {
                continue;
            }
            else
            {
                OLED_ShowChar(row + 1U, column + 1U, (char)token);
                shown[row][column] = token;
            }
            break;
        }
    }
}

static void MakeFrame(uint16_t distance_mm,
                      uint8_t distance_valid,
                      int16_t temperature_10,
                      uint16_t humidity_10,
                      uint8_t dht_valid,
                      HCSR04_Status distance_status,
                      uint8_t alarm_active,
                      uint8_t calibrating)
{
    char text[24];
    int temperature_abs;
    uint8_t column;

    /* 每次从空白帧重新组织四行显示内容。 */
    Frame_Clear();

    /* 第 1 行：距离。数据无效时显示占位符。 */
    column = Frame_PutChinese(0, 0, OLED_CH_DISTANCE);
    column = Frame_PutChinese(0, column, OLED_CH_LEAVE);
    if (distance_valid)
    {
        sprintf(text, ":%u.%ucm",
                (unsigned)(distance_mm / 10U),
                (unsigned)(distance_mm % 10U));
        (void)Frame_PutAscii(0, column, text);
    }
    else
    {
        (void)Frame_PutAscii(0, column, ":----.-cm");
    }

    /* 第 2、3 行：温度和湿度。 */
    column = Frame_PutChinese(1, 0, OLED_CH_TEMP);
    column = Frame_PutChinese(1, column, OLED_CH_DEGREE);
    if (dht_valid)
    {
        temperature_abs = temperature_10 < 0 ? -temperature_10 : temperature_10;
        sprintf(text, ":%s%d.%dC",
                temperature_10 < 0 ? "-" : "",
                temperature_abs / 10,
                temperature_abs % 10);
        (void)Frame_PutAscii(1, column, text);

        sprintf(text, ":%u.%u%%",
                (unsigned)(humidity_10 / 10U),
                (unsigned)(humidity_10 % 10U));
        column = Frame_PutChinese(2, 0, OLED_CH_HUMID);
        column = Frame_PutChinese(2, column, OLED_CH_DEGREE);
        (void)Frame_PutAscii(2, column, text);
    }
    else
    {
        (void)Frame_PutAscii(1, column, ":--.-C");
        column = Frame_PutChinese(2, 0, OLED_CH_HUMID);
        column = Frame_PutChinese(2, column, OLED_CH_DEGREE);
        (void)Frame_PutAscii(2, column, ":--.-%");
    }

    /* 第 4 行按优先级显示：校准、故障、超量程、报警、正常。 */
    column = 0;
    if (calibrating)
    {
        column = Frame_PutChinese(3, column, OLED_CH_TEMP);
        column = Frame_PutChinese(3, column, OLED_CH_HUMID);
        column = Frame_PutChinese(3, column, OLED_CH_DEGREE);
        column = Frame_PutChinese(3, column, OLED_CH_CALIBRATE);
        (void)Frame_PutChinese(3, column, OLED_CH_ACCURATE);
    }
    else if (!dht_valid)
    {
        column = Frame_PutChinese(3, column, OLED_CH_TEMP);
        column = Frame_PutChinese(3, column, OLED_CH_HUMID);
        column = Frame_PutChinese(3, column, OLED_CH_DEGREE);
        column = Frame_PutChinese(3, column, OLED_CH_WRONG);
        (void)Frame_PutChinese(3, column, OLED_CH_ERROR);
    }
    else if (!distance_valid && distance_status == HCSR04_OUT_OF_RANGE)
    {
        column = Frame_PutChinese(3, column, OLED_CH_DISTANCE);
        column = Frame_PutChinese(3, column, OLED_CH_LEAVE);
        column = Frame_PutChinese(3, column, OLED_CH_EXCEED);
        column = Frame_PutChinese(3, column, OLED_CH_QUANTITY);
        (void)Frame_PutChinese(3, column, OLED_CH_RANGE);
    }
    else if (!distance_valid)
    {
        column = Frame_PutChinese(3, column, OLED_CH_DISTANCE);
        column = Frame_PutChinese(3, column, OLED_CH_LEAVE);
        column = Frame_PutChinese(3, column, OLED_CH_WRONG);
        (void)Frame_PutChinese(3, column, OLED_CH_ERROR);
    }
    else if (alarm_active)
    {
        column = Frame_PutChinese(3, column, OLED_CH_ALARM);
        column = Frame_PutChinese(3, column, OLED_CH_WARNING);
        column = Frame_PutAscii(3, column, ":");
        column = Frame_PutChinese(3, column, OLED_CH_LAMP);
        column = Frame_PutChinese(3, column, OLED_CH_FLASH);
        (void)Frame_PutChinese(3, column, OLED_CH_FLICKER);
    }
    else
    {
        column = Frame_PutChinese(3, column, OLED_CH_STATE);
        column = Frame_PutChinese(3, column, OLED_CH_ATTITUDE);
        column = Frame_PutAscii(3, column, ":");
        column = Frame_PutChinese(3, column, OLED_CH_POSITIVE);
        (void)Frame_PutChinese(3, column, OLED_CH_USUAL);
    }
}

static void MakeStartupFrame(void)
{
    uint8_t column;

    /* 上电先显示固定占位值和“温湿度校准”，等待首批有效数据。 */
    Frame_Clear();
    column = Frame_PutChinese(0, 0, OLED_CH_DISTANCE);
    column = Frame_PutChinese(0, column, OLED_CH_LEAVE);
    (void)Frame_PutAscii(0, column, ":----.-cm");
    column = Frame_PutChinese(1, 0, OLED_CH_TEMP);
    column = Frame_PutChinese(1, column, OLED_CH_DEGREE);
    (void)Frame_PutAscii(1, column, ":--.-C");
    column = Frame_PutChinese(2, 0, OLED_CH_HUMID);
    column = Frame_PutChinese(2, column, OLED_CH_DEGREE);
    (void)Frame_PutAscii(2, column, ":--.-%");
    column = Frame_PutChinese(3, 0, OLED_CH_TEMP);
    column = Frame_PutChinese(3, column, OLED_CH_HUMID);
    column = Frame_PutChinese(3, column, OLED_CH_DEGREE);
    column = Frame_PutChinese(3, column, OLED_CH_CALIBRATE);
    (void)Frame_PutChinese(3, column, OLED_CH_ACCURATE);
}

int main(void)
{
    uint32_t now;
    uint32_t last_distance_read;
    uint32_t last_distance_good = 0U;
    uint32_t last_dht_read;
    uint32_t last_dht_good = 0U;
    uint32_t last_frame = 0U;
    uint32_t startup_started;
    uint16_t distance_mm = 0U;
    uint16_t humidity_10 = 0U;
    int16_t temperature_10 = 0;
    uint8_t have_distance = 0U;
    uint8_t have_dht = 0U;
    uint8_t distance_valid;
    uint8_t dht_valid;
    uint8_t alarm_active;
    uint8_t calibrating;
    HCSR04_Status distance_status = HCSR04_NO_ECHO;

    /* 依次初始化系统时基、OLED 和超声波测距模块。 */
    Board_Init();
    OLED_Init();
    HCSR04_Init();

    /* shown 清零可确保启动帧中的所有有效字符都会被首次刷新。 */
    memset(shown, 0, sizeof(shown));
    MakeStartupFrame();

    startup_started = Board_Millis();
    now = startup_started;
    last_distance_read = now - DISTANCE_PERIOD_MS;
    /* DHT11 上电后需要稳定时间，首次读取安排在一个完整采样周期后。 */
    last_dht_read = now;

    while (1)
    {
        now = Board_Millis();

        /* 按固定周期发起一次超声波测距，成功时更新时间戳。 */
        if ((uint32_t)(now - last_distance_read) >= DISTANCE_PERIOD_MS)
        {
            last_distance_read = now;
            distance_status = HCSR04_ReadDistanceMm(&distance_mm);
            if (distance_status == HCSR04_OK)
            {
                have_distance = 1U;
                last_distance_good = Board_Millis();
            }
        }

        /* DHT11 读取耗时较长，因此使用独立且更慢的采样周期。 */
        now = Board_Millis();
        if ((uint32_t)(now - last_dht_read) >= DHT_PERIOD_MS)
        {
            last_dht_read = now;
            if (DHT_Read(&temperature_10, &humidity_10))
            {
                have_dht = 1U;
                last_dht_good = Board_Millis();
            }
        }

        /* 超过有效期仍未获得新样本时，不再使用上一次旧数据报警。 */
        now = Board_Millis();
        distance_valid = have_distance &&
                         (uint32_t)(now - last_distance_good) < DISTANCE_STALE_MS;
        dht_valid = have_dht &&
                    (uint32_t)(now - last_dht_good) < DHT_STALE_MS;
        calibrating = (uint32_t)(now - startup_started) <
                      STARTUP_CALIBRATION_MS;

        /* 三个条件必须同时满足才闪烁；数据无效时强制关闭 LED。 */
        alarm_active = distance_valid && dht_valid &&
                       temperature_10 > ALARM_TEMP_10 &&
                       humidity_10 > ALARM_HUMIDITY_10 &&
                       distance_mm < ALARM_DISTANCE_MM;
        Board_LED(alarm_active &&
                  (((now / LED_HALF_PERIOD_MS) & 1U) != 0U));

        /* 定期生成目标帧，实际屏幕写入由 Display_Service 分散完成。 */
        if ((uint32_t)(now - last_frame) >= DISPLAY_PERIOD_MS)
        {
            last_frame = now;
            MakeFrame(distance_mm, distance_valid,
                      temperature_10, humidity_10, dht_valid,
                      distance_status, alarm_active, calibrating);
        }

        Display_Service();
    }
}
