#include "stm32f10x.h"
#include "BoardSensors.h"
#include "Delay.h"
#include "Timer.h"

#define DHT_PORT GPIOA
#define DHT_PIN  GPIO_Pin_11
#define LED_PORT GPIOA
#define LED_PIN  GPIO_Pin_10

/* SysTick 每毫秒递增，用作无符号软件时钟。 */
static volatile uint32_t milliseconds;

/* SysTick 中断入口调用此函数维护系统时基。 */
void Board_Tick(void)
{
    milliseconds++;
}

/* 读取 32 位毫秒计数，回绕后利用无符号减法仍可正确计算时间差。 */
uint32_t Board_Millis(void)
{
    return milliseconds;
}

/* 本工程 LED 为高电平点亮。 */
void Board_LED(uint8_t on)
{
    GPIO_WriteBit(LED_PORT, LED_PIN, on ? Bit_SET : Bit_RESET);
}

void Board_Init(void)
{
    GPIO_InitTypeDef io;

    /* 更新系统主频，并启动 TIM2 微秒计时器。 */
    SystemCoreClockUpdate();
    Timer_Init();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* 先输出低电平，再配置推挽输出，避免上电瞬间误亮。 */
    GPIO_ResetBits(LED_PORT, LED_PIN);
    io.GPIO_Pin = LED_PIN;
    io.GPIO_Mode = GPIO_Mode_Out_PP;
    io.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(LED_PORT, &io);

    /* DHT11 总线空闲时释放为高电平，外部需 4.7k~10k 上拉。 */
    GPIO_SetBits(DHT_PORT, DHT_PIN);
    io.GPIO_Pin = DHT_PIN;
    io.GPIO_Mode = GPIO_Mode_Out_OD;
    io.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(DHT_PORT, &io);

    /* 配置 1 kHz SysTick，形成 1 ms 系统节拍。 */
    SysTick_Config(SystemCoreClock / 1000U);
}

/* DHT11 起始信号阶段使用开漏输出，由外部上拉产生高电平。 */
static void DHT_SetOutput(void)
{
    GPIO_InitTypeDef io;

    io.GPIO_Pin = DHT_PIN;
    io.GPIO_Mode = GPIO_Mode_Out_OD;
    io.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(DHT_PORT, &io);
}

/* 接收传感器响应和数据时将总线切换为浮空输入。 */
static void DHT_SetInput(void)
{
    GPIO_InitTypeDef io;

    io.GPIO_Pin = DHT_PIN;
    io.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    io.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(DHT_PORT, &io);
}

/* 等待数据线到达指定电平，超时返回 0，防止程序永久阻塞。 */
static uint8_t DHT_WaitLevel(uint8_t level, uint16_t timeout_us)
{
    uint16_t start_us = Timer_GetUs();

    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) != level)
    {
        if ((uint16_t)(Timer_GetUs() - start_us) >= timeout_us)
        {
            return 0U;
        }
    }
    return 1U;
}

uint8_t DHT_Read(int16_t *temperature_10, uint16_t *humidity_10)
{
    uint8_t data[5] = {0U, 0U, 0U, 0U, 0U};
    uint8_t i;
    uint8_t valid = 0U;
    uint16_t pulse_us;
    uint16_t pulse_start_us;
    uint32_t primask;

    /* 输出指针无效时直接退出。 */
    if (temperature_10 == 0 || humidity_10 == 0)
    {
        return 0U;
    }

    /* 主机拉低至少 18 ms，通知 DHT11 开始一次传输。 */
    DHT_SetOutput();
    GPIO_ResetBits(DHT_PORT, DHT_PIN);
    Delay_ms(20U);

    /* DHT11 的 40 位数据时序需连续完成，期间暂时关闭中断。 */
    primask = __get_PRIMASK();
    __disable_irq();
    GPIO_SetBits(DHT_PORT, DHT_PIN);
    Delay_us(30U);
    DHT_SetInput();

    /* 等待传感器的低—高—低响应序列。 */
    if (!DHT_WaitLevel(0U, 150U) ||
        !DHT_WaitLevel(1U, 150U) ||
        !DHT_WaitLevel(0U, 150U))
    {
        goto finish;
    }

    /* 依次读取 40 位，高电平持续时间大于 45 us 判定为 1。 */
    for (i = 0U; i < 40U; i++)
    {
        if (!DHT_WaitLevel(1U, 120U))
        {
            goto finish;
        }

        pulse_start_us = Timer_GetUs();
        if (!DHT_WaitLevel(0U, 120U))
        {
            goto finish;
        }
        pulse_us = (uint16_t)(Timer_GetUs() - pulse_start_us);

        if (pulse_us < 10U || pulse_us > 100U)
        {
            goto finish;
        }
        data[i / 8U] = (uint8_t)((data[i / 8U] << 1U) |
                                 (pulse_us > 45U ? 1U : 0U));
    }

    /* 第 5 字节为前 4 字节之和的低 8 位。 */
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
    {
        goto finish;
    }

    /* DHT11：整数字节 + 小数字节；这里统一换算为 0.1 单位。 */
    if (data[0] > 100U || data[1] > 9U ||
        data[2] > 80U || (data[3] & 0x7FU) > 9U)
    {
        goto finish;
    }

    *humidity_10 = (uint16_t)(data[0] * 10U + data[1]);
    *temperature_10 = (int16_t)(data[2] * 10U + (data[3] & 0x7FU));
    if ((data[3] & 0x80U) != 0U)
    {
        *temperature_10 = -*temperature_10;
    }
    valid = 1U;

finish:
    /* 无论成功或失败，都恢复总线空闲高电平及原中断状态。 */
    DHT_SetOutput();
    GPIO_SetBits(DHT_PORT, DHT_PIN);
    __set_PRIMASK(primask);
    return valid;
}
