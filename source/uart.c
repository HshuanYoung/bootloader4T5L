/**
 * @file    uart.c
 * @brief   精简UART5驱动与AB CD帧提取实现。
 * @author  yangming
 * @version 1.0.0
 */

#include "uart.h"
#include "ota.h"

UART_TYPE Uart5;

static uint8_t xdata Uart5TxBuffer[uartUART5_TXBUF_SIZE + 1U];
static uint8_t xdata Uart5RxBuffer[uartUART5_RXBUF_SIZE + 1U];
static uint8_t Uart5RecoveryFlag;

/**
 * @brief 清空字节缓存。
 * @param[out] buf 待清空缓存。
 * @param[in] len 字节长度。
 */
static void UartClearBuffer(uint8_t *buf, uint16_t len)
{
    uint16_t i;

    for(i = 0U; i < len; i++)
    {
        buf[i] = 0U;
    }
}

/**
 * @brief 初始化UART5用于OTA传输。
 * @param[in] baudrate UART波特率。
 */
void Uart5Init(uint32_t baudrate)
{
    uint32_t divider;

    UartClearBuffer((uint8_t *)&Uart5, sizeof(Uart5));
    UartClearBuffer(Uart5TxBuffer, sizeof(Uart5TxBuffer));
    UartClearBuffer(Uart5RxBuffer, sizeof(Uart5RxBuffer));
    Uart5RecoveryFlag = 0U;

    SCON3T = 0x80U;
    SCON3R = 0x80U;

    divider = sysFCLK / 16UL / baudrate;
    BODE3_DIV_H = (uint8_t)(divider >> 8);
    BODE3_DIV_L = (uint8_t)divider;

    ES3T = 1U;
    ES3R = 1U;
    EA = 1U;
}

/**
 * @brief 停止UART5收发中断。
 */
void Uart5Stop(void)
{
    ES3T = 0U;
    ES3R = 0U;
    Uart5.RxHead = 0U;
    Uart5.RxTail = 0U;
    Uart5.RxTimeout = 0U;
    Uart5.RxFlag = UART_NON_REC;
    Uart5.TxBusy = 0U;
    Uart5RecoveryFlag = 0U;
}

/**
 * @brief UART5接收中断。
 */
void Uart5RxIsr(void) interrupt 13
{
    uint16_t recovery_index;

    if((SCON3R & 0x01U) == 0x01U)
    {
        if(Uart5.RxHead < uartUART5_RXBUF_SIZE)
        {
            Uart5RxBuffer[Uart5.RxHead++] = SBUF3_RX;
            if(Uart5.RxHead >= 4U)
            {
                recovery_index = Uart5.RxHead - 4U;
                if((Uart5RxBuffer[recovery_index] == BOOT_RECOVERY_CMD_0) &&
                   (Uart5RxBuffer[recovery_index + 1U] == BOOT_RECOVERY_CMD_1) &&
                   (Uart5RxBuffer[recovery_index + 2U] == BOOT_RECOVERY_CMD_2) &&
                   (Uart5RxBuffer[recovery_index + 3U] == BOOT_RECOVERY_CMD_3))
                {
                    Uart5RecoveryFlag = 1U;
                }
            }
            Uart5.RxFlag = UART_RECING;
            Uart5.RxTimeout = uartUART5_TIMEOUTSET;
        }

        SCON3R &= 0xFEU;
    }
}

/**
 * @brief UART5发送中断。
 */
void Uart5TxIsr(void) interrupt 12
{
    if((SCON3T & 0x01U) == 0x01U)
    {
        SCON3T &= 0xFEU;
        if(Uart5.TxHead != Uart5.TxTail)
        {
            SBUF3_TX = Uart5TxBuffer[Uart5.TxTail++];
            if(Uart5.TxTail >= uartUART5_TXBUF_SIZE)
            {
                Uart5.TxTail = 0U;
            }
        }
        else
        {
            Uart5.TxBusy = 0U;
        }
    }
}

/**
 * @brief 通过UART5发送字节数据。
 * @param[in] uart UART控制指针，仅支持Uart5。
 * @param[in] buf 源字节数据。
 * @param[in] len 字节长度。
 */
void UartSendData(UART_TYPE *uart, uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint16_t next_head;

    if((uart != &Uart5) || (buf == NULL))
    {
        return;
    }

    for(i = 0U; i < len; i++)
    {
        do
        {
            next_head = uart->TxHead + 1U;
            if(next_head >= uartUART5_TXBUF_SIZE)
            {
                next_head = 0U;
            }
        }while(next_head == uart->TxTail);

        Uart5TxBuffer[uart->TxHead] = *buf++;
        uart->TxHead = next_head;
    }

    if(uart->TxBusy == 0U)
    {
        uart->TxBusy = 1U;
        SCON3T |= 0x01U;
    }
}

/**
 * @brief 解析UART5缓存数据并分发AB CD OTA帧。
 * @param[in] uart UART控制指针。
 */
void UartReadFrame(UART_TYPE *uart)
{
    uint16_t remaining_len;
    uint16_t total_len;
    uint16_t frame_offset;
    uint16_t one_frame_len;

    if((uart != &Uart5) || (uart->RxFlag == UART_NON_REC) || (uart->RxTimeout != 0U))
    {
        return;
    }

    total_len = uart->RxHead;
    uart->RxFlag = UART_NON_REC;
    uart->RxHead = 0U;
    uart->RxTail = 0U;
    remaining_len = total_len;

    while(remaining_len >= 2U)
    {
        frame_offset = total_len - remaining_len;
        if((Uart5RxBuffer[frame_offset] == 0xABU) &&
           (Uart5RxBuffer[frame_offset + 1U] == 0xCDU))
        {
            if(remaining_len < 4U)
            {
                break;
            }

            one_frame_len = ((uint16_t)Uart5RxBuffer[frame_offset + 2U] << 8) |
                            (uint16_t)Uart5RxBuffer[frame_offset + 3U];
            one_frame_len += 4U;

            if(one_frame_len > uartUART5_RXBUF_SIZE)
            {
                remaining_len--;
            }
            else if(remaining_len < one_frame_len)
            {
                break;
            }
            else
            {
                OtaReceive(&Uart5RxBuffer[frame_offset], one_frame_len);
                remaining_len -= one_frame_len;
            }
        }
        else
        {
            remaining_len--;
        }
    }
}

/**
 * @brief 检查UART5缓存中是否收到恢复指令。
 * @return 收到5A A5 5A A5时返回1，否则返回0。
 */
uint8_t UartRecoveryRequested(void)
{
    return Uart5RecoveryFlag;
}
