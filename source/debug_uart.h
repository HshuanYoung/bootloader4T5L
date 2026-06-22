/**
 * @file    debug_uart.h
 * @brief   UART2 blocking debug log output.
 * @author  yangming
 * @version 1.0.0
 */

#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include "sys.h"

#if debugUART2_ENABLED
void DebugUart2Init(void);
void DebugLog(const char *msg);
void DebugLogU8(uint8_t value);
void DebugLogU16(uint16_t value);
void DebugLogU32(uint32_t value);
void DebugLogHex8(uint8_t value);
void DebugLogHex16(uint16_t value);
void DebugLogHex32(uint32_t value);

#define DBG_INIT() DebugUart2Init()

#if debugLOG_KEY_FLOW_ENABLED
#define DBG_LOG(msg) DebugLog(msg)
#define DBG_LOG_LINE(msg) \
    do { \
        DebugLog(msg); \
        DebugLog("\r\n"); \
    }while(0)
#define DBG_LOG_U8(prefix, value) \
    do { \
        DebugLog(prefix); \
        DebugLogU8((uint8_t)(value)); \
        DebugLog("\r\n"); \
    }while(0)
#define DBG_LOG_U16(prefix, value) \
    do { \
        DebugLog(prefix); \
        DebugLogU16((uint16_t)(value)); \
        DebugLog("\r\n"); \
    }while(0)
#define DBG_LOG_U32(prefix, value) \
    do { \
        DebugLog(prefix); \
        DebugLogU32((uint32_t)(value)); \
        DebugLog("\r\n"); \
    }while(0)
#define DBG_LOG_HEX8(prefix, value) \
    do { \
        DebugLog(prefix); \
        DebugLogHex8((uint8_t)(value)); \
        DebugLog("\r\n"); \
    }while(0)
#define DBG_LOG_HEX16(prefix, value) \
    do { \
        DebugLog(prefix); \
        DebugLogHex16((uint16_t)(value)); \
        DebugLog("\r\n"); \
    }while(0)
#define DBG_LOG_HEX32(prefix, value) \
    do { \
        DebugLog(prefix); \
        DebugLogHex32((uint32_t)(value)); \
        DebugLog("\r\n"); \
    }while(0)
#else
#define DBG_LOG(msg) ((void)0)
#define DBG_LOG_LINE(msg) ((void)0)
#define DBG_LOG_U8(prefix, value) ((void)0)
#define DBG_LOG_U16(prefix, value) ((void)0)
#define DBG_LOG_U32(prefix, value) ((void)0)
#define DBG_LOG_HEX8(prefix, value) ((void)0)
#define DBG_LOG_HEX16(prefix, value) ((void)0)
#define DBG_LOG_HEX32(prefix, value) ((void)0)
#endif /* debugLOG_KEY_FLOW_ENABLED */

#else
#define DBG_INIT() ((void)0)
#define DBG_LOG(msg) ((void)0)
#define DBG_LOG_LINE(msg) ((void)0)
#define DBG_LOG_U8(prefix, value) ((void)0)
#define DBG_LOG_U16(prefix, value) ((void)0)
#define DBG_LOG_U32(prefix, value) ((void)0)
#define DBG_LOG_HEX8(prefix, value) ((void)0)
#define DBG_LOG_HEX16(prefix, value) ((void)0)
#define DBG_LOG_HEX32(prefix, value) ((void)0)
#endif /* debugUART2_ENABLED */

#endif /* DEBUG_UART_H */
