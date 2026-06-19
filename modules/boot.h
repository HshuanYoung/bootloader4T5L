/**
 * @file    boot.h
 * @brief   启动加载器应用加载与校验接口。
 * @author  yangming
 * @version 1.0.0
 */

#ifndef BOOT_H
#define BOOT_H

#include "sys.h"

#define BOOT_CTRL_UPGRADE_0 0x5AU
#define BOOT_CTRL_UPGRADE_1 0xA5U
#define BOOT_CTRL_UPGRADE_2 0x5AU
#define BOOT_CTRL_UPGRADE_3 0xA5U
#define BOOT_CTRL_LOAD_0 0xAAU
#define BOOT_CTRL_LOAD_1 0x55U

void BootLoadApp(void);
uint8_t BootIsUpgradeRequested(void);
uint8_t BootControlIsLoadCommand(void);
uint16_t BootResolveStartBlock(void);
uint8_t BootWaitLoadCommand(uint32_t timeout_ms);
uint8_t BootWaitRecoveryCommand(void);
void BootClearControl(void);
uint8_t BootCodeCheck(uint16_t start_block);
void BootEnterUpgradeMode(void);

#endif /* BOOT_H */
