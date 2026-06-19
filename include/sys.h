/**
 * @file    sys.h
 * @brief   T5L启动加载器系统服务接口。
 * @details 提供BOOT底座和OTA模块使用的DGUS VP访问、延时和VP拷贝辅助接口。
 * @author  yangming
 * @version 1.0.0
 */

#ifndef SYS_H
#define SYS_H

#include "T5LOSConfig.h"

#define sysDGUS_FLASH_RW_CMD_ADDR 0x0008U
#define sysDGUS_BOOT_RESET_ADDR 0x0004U
#define sysDGUS_PIC_SET_ADDR 0x0084U
#define sysDGUS_NAND_CMD_ADDR 0x00AAU
#define sysDGUS_NAND_CRC_ADDR 0x00AEU

/**
 * @brief VP区域拷贝请求。
 * @details sourceVP和targetVP为VP字地址，len为拷贝字数；mode为1时清空源区域。
 */
typedef struct _VP_EXCHANGE
{
    uint32_t sourceVP;
    uint32_t targetVP;
    uint16_t len;
    uint8_t mode;
} VP_EXCHANGE;

void read_dgus_vp(uint16_t addr, uint8_t *buf, uint16_t len);
void write_dgus_vp(uint16_t addr, uint8_t *buf, uint16_t len);

void delay_us(uint16_t us);
void delay_ms(uint16_t ms);

void vpExchange(VP_EXCHANGE *exchange_msg);
void FlashToDgus(uint32_t flash_addr, uint16_t dgus_vp_addr, uint16_t len_words);
void DgusToFlash(uint32_t flash_addr, uint16_t dgus_vp_addr, uint16_t len_words);
void SwitchPageById(uint16_t page_id);
void SoftReset(void);

#endif /* SYS_H */
