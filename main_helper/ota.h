/**
 * @file    ota.h
 * @brief   APP端触发BOOT OTA升级/热切换的接入伪代码。
 * @details 本文件只作为APP端参考，不依赖bootloader工程内部头文件。
 *          APP工程需要将APP_WriteDgusVp、APP_DgusToFlash、APP_SoftReset
 *          替换为自身平台的DGUS VP写入、Flash写入和软复位接口。
 */

#ifndef MAIN_HELPER_OTA_H
#define MAIN_HELPER_OTA_H

#define OTA_CTRL_ADDR 0x0020U
#define OTA_CTRL_WORDS 2U

#define OTA_UPGRADE_MAGIC 0x5AA55AA5UL
#define OTA_HOT_SWITCH_PREFIX 0xAA550000UL
#define OTA_PREFIX_MASK 0xFFFF0000UL

#define OTA_IS_UPGRADE_REQUEST(value) \
    (((unsigned long)(value)) == OTA_UPGRADE_MAGIC)

#define OTA_IS_HOT_SWITCH_REQUEST(value) \
    ((((unsigned long)(value)) & OTA_PREFIX_MASK) == OTA_HOT_SWITCH_PREFIX)

#define OTA_IS_RESET_REQUEST(value) \
    (OTA_IS_UPGRADE_REQUEST(value) || OTA_IS_HOT_SWITCH_REQUEST(value))

#define OTA_U32_TO_BE_BYTES(value, buf) \
    do { \
        (buf)[0] = (unsigned char)((unsigned long)(value) >> 24); \
        (buf)[1] = (unsigned char)((unsigned long)(value) >> 16); \
        (buf)[2] = (unsigned char)((unsigned long)(value) >> 8); \
        (buf)[3] = (unsigned char)(unsigned long)(value); \
    }while(0)

#define OTA_APP_WRITE_CTRL(value) \
    do { \
        unsigned char ota_ctrl_buf[4]; \
        OTA_U32_TO_BE_BYTES((value), ota_ctrl_buf); \
        APP_WriteDgusVp(OTA_CTRL_ADDR, ota_ctrl_buf, OTA_CTRL_WORDS); \
    }while(0)

#define OTA_APP_PERSIST_CTRL() \
    APP_DgusToFlash((unsigned long)OTA_CTRL_ADDR, OTA_CTRL_ADDR, OTA_CTRL_WORDS)

/**
 * @brief 手动触发升级。
 * @note  APP调用后会写入0x0020 = 0x5AA55AA5，保存到Flash，然后软复位进入BOOT。
 */
#define OTA_APP_TRIGGER_UPGRADE() \
    do { \
        OTA_APP_WRITE_CTRL(OTA_UPGRADE_MAGIC); \
        OTA_APP_PERSIST_CTRL(); \
        APP_SoftReset(); \
    }while(0)

/**
 * @brief 轮询处理0x0020控制字。
 * @param[in] value APP从0x0020读出的32位值。
 * @note  当value为0x5AA55AA5或AA55xxxx时，保存当前控制字到Flash并软复位。
 */
#define OTA_APP_POLL_AND_RESET(value) \
    do { \
        unsigned long ota_ctrl_value = (unsigned long)(value); \
        if(OTA_IS_RESET_REQUEST(ota_ctrl_value)) \
        { \
            OTA_APP_WRITE_CTRL(ota_ctrl_value); \
            OTA_APP_PERSIST_CTRL(); \
            APP_SoftReset(); \
        } \
    }while(0)

/*
 * APP端轮询伪代码：
 *
 * while(1)
 * {
 *     unsigned long ctrl_value;
 *
 *     ctrl_value = APP_ReadDgusVpU32(OTA_CTRL_ADDR);
 *     OTA_APP_POLL_AND_RESET(ctrl_value);
 * }
 */

#endif /* MAIN_HELPER_OTA_H */
