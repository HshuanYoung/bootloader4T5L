/**
 * @file    uart.h
 * @brief   BOOT底座OTA使用的精简UART5驱动。
 * @author  yangming
 * @version 1.0.0
 */

#ifndef UART_H
#define UART_H

#include "sys.h"

typedef struct UartxDefine
{
    uint16_t TxHead;
    uint16_t TxTail;
    uint16_t RxHead;
    uint16_t RxTail;
    uint8_t RxTimeout;
    uint8_t RxFlag:2;
    uint8_t TxBusy:1;
} UART_TYPE;

#define UART_NON_REC 0U
#define UART_RECING 1U
#define UART_RECOVERY_NONE 0U
#define UART_RECOVERY_UPGRADE 1U
#define UART_RECOVERY_LOAD 2U

extern UART_TYPE Uart5;

void Uart5Init(uint32_t baudrate);
void Uart5Stop(void);
void UartSendData(UART_TYPE *uart, uint8_t *buf, uint16_t len);
void UartReadFrame(UART_TYPE *uart);
uint8_t UartRecoveryGetControl(uint8_t *control_buf);

#endif /* UART_H */
