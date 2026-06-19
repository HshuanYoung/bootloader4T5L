/**
 * @file    main.c
 * @brief   T5L启动加载器入口。
 * @author  yangming
 * @version 1.0.0
 */

#include "boot.h"

/**
 * @brief 启动加载器主入口。
 */
void main(void)
{
    uint16_t start_block;
    uint8_t code_valid;

    if((BootIsUpgradeRequested() != 0U) || (BootWaitRecoveryCommand() != 0U))
    {
        BootEnterUpgradeMode();
    }

    start_block = BootResolveStartBlock();

    code_valid = BootCodeCheck(start_block);
    if(code_valid == 0U)
    {
        if(start_block != BOOT_DEFAULT_START_BLOCK)
        {
            start_block = BOOT_DEFAULT_START_BLOCK;
            code_valid = BootCodeCheck(start_block);
        }
    }

    if(code_valid != 0U)
    {
        BootLoadApp();
    }

    while(1)
    {
    }
}
