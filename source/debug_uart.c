/**
 * @file    debug_uart.c
 * @brief   UART2 blocking debug log output.
 * @author  yangming
 * @version 1.0.0
 */

#include "debug_uart.h"

#if debugUART2_ENABLED

static uint8_t DebugUartReady;

/**
 * @brief Send one byte on UART2 using blocking polling.
 */
static void DebugUart2PutChar(char ch)
{
    if(DebugUartReady == 0U)
    {
        return;
    }

    TI0 = 0U;
    SBUF0 = (uint8_t)ch;
    while(TI0 == 0U)
    {
    }
    TI0 = 0U;
}

/**
 * @brief Output one raw hex byte without 0x prefix.
 */
static void DebugLogHexByteRaw(uint8_t value)
{
    uint8_t nibble;

    nibble = (value >> 4) & 0x0FU;
    DebugUart2PutChar((char)((nibble < 10U) ? ('0' + nibble) : ('A' + nibble - 10U)));

    nibble = value & 0x0FU;
    DebugUart2PutChar((char)((nibble < 10U) ? ('0' + nibble) : ('A' + nibble - 10U)));
}

/**
 * @brief Initialize UART2 on P0.4/P0.5 for debug output.
 */
void DebugUart2Init(void)
{
    uint32_t divider;
    uint16_t reload;

    DebugUartReady = 0U;

    MUX_SEL |= 0x40U;
    P0MDOUT &= 0xCFU;
    P0MDOUT |= 0x10U;
    P0 |= 0x30U;
    ADCON = 0x80U;
    SCON0 = 0x50U;

    PCON &= 0x7FU;
    PCON |= 0x80U;
#if _2K_RATIO == 1
    PCON &= 0x7FU;
#endif /* _2K_RATIO */

    if((PCON & 0x80U) != 0U)
    {
        divider = sysFOSC / 32UL / debugUART2_BAUDRATE;
    }
    else
    {
        divider = sysFOSC / 64UL / debugUART2_BAUDRATE;
    }

    if(divider >= 1024UL)
    {
        reload = 0U;
    }
    else
    {
        reload = (uint16_t)(1024UL - divider);
    }

    SREL0H = (uint8_t)(reload >> 8);
    SREL0L = (uint8_t)reload;
    RI0 = 0U;
    TI0 = 0U;
    ES0 = 0U;

    DebugUartReady = 1U;
#if debugLOG_KEY_FLOW_ENABLED
    DebugLog("[DBG] UART2 ready\r\n");
#endif /* debugLOG_KEY_FLOW_ENABLED */
}

/**
 * @brief Output a zero-terminated ASCII string.
 */
void DebugLog(const char *msg)
{
    if(msg == NULL)
    {
        return;
    }

    while(*msg != '\0')
    {
        DebugUart2PutChar(*msg++);
    }
}

/**
 * @brief Output an unsigned 32-bit decimal value.
 */
void DebugLogU32(uint32_t value)
{
    uint32_t divisor;
    uint8_t digit;
    uint8_t started;

    divisor = 1000000000UL;
    started = 0U;
    while(divisor != 0UL)
    {
        digit = (uint8_t)(value / divisor);
        if((digit != 0U) || (started != 0U) || (divisor == 1UL))
        {
            DebugUart2PutChar((char)('0' + digit));
            started = 1U;
        }
        value -= ((uint32_t)digit * divisor);
        divisor /= 10UL;
    }
}

void DebugLogU8(uint8_t value)
{
    DebugLogU32((uint32_t)value);
}

void DebugLogU16(uint16_t value)
{
    DebugLogU32((uint32_t)value);
}

void DebugLogHex8(uint8_t value)
{
    DebugLog("0x");
    DebugLogHexByteRaw(value);
}

void DebugLogHex16(uint16_t value)
{
    DebugLog("0x");
    DebugLogHexByteRaw((uint8_t)(value >> 8));
    DebugLogHexByteRaw((uint8_t)value);
}

void DebugLogHex32(uint32_t value)
{
    DebugLog("0x");
    DebugLogHexByteRaw((uint8_t)(value >> 24));
    DebugLogHexByteRaw((uint8_t)(value >> 16));
    DebugLogHexByteRaw((uint8_t)(value >> 8));
    DebugLogHexByteRaw((uint8_t)value);
}

#endif /* debugUART2_ENABLED */
