/**
 * @file    timer.c
 * @brief   UART5和OTA超时使用的精简Timer0节拍实现。
 * @author  yangming
 * @version 1.0.0
 */

#include "timer.h"
#include "uart.h"
#include "ota.h"

/**
 * @brief 初始化Timer0为1ms周期中断。
 */
void TimerInit(void)
{
    TMOD &= 0xF0U;
    TMOD |= 0x01U;
    TH0 = (uint8_t)(timeT0_TICK >> 8);
    TL0 = (uint8_t)timeT0_TICK;
    ET0 = 1U;
    TR0 = 1U;
    EA = 1U;
}

/**
 * @brief 停止Timer0。
 */
void TimerStop(void)
{
    TR0 = 0U;
    ET0 = 0U;
}

/**
 * @brief Timer0中断服务函数。
 */
void Timer0Isr(void) interrupt 1
{
    ET0 = 0U;
    TH0 = (uint8_t)(timeT0_TICK >> 8);
    TL0 = (uint8_t)timeT0_TICK;

    if(Uart5.RxTimeout != 0U)
    {
        Uart5.RxTimeout--;
    }

    OtaTimerTick1ms();

    ET0 = 1U;
}
