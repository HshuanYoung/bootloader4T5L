/**
 * @file    main.c
 * @brief   T5L启动加载器入口。
 * @author  yangming
 * @version 1.0.0
 */

#include "boot.h"
#include "debug_uart.h"

/**
 * @brief 启动加载器主入口。
 */
void main(void)
{
    uint16_t start_block;
    uint8_t code_valid;
    uint8_t upgrade_requested;

    DBG_INIT();
    DBG_LOG_LINE("[BOOT] start");
    BootWaitRecoveryCommand();

    upgrade_requested = BootIsUpgradeRequested();
    if(upgrade_requested != 0U)
    {
        DBG_LOG_LINE("[BOOT] upgrade requested");
        BootEnterUpgradeMode();
        DBG_LOG_LINE("[BOOT] upgrade mode exited");
    }
    else
    {
        DBG_LOG_LINE("[BOOT] upgrade not requested");
    }

    start_block = BootResolveStartBlock();
    DBG_LOG_U16("[BOOT] start block=", start_block);

    code_valid = BootCodeCheck(start_block);
#if debugUART2_ENABLED && debugLOG_KEY_FLOW_ENABLED
    DebugLog("[BOOT] code check block=");
    DebugLogU16(start_block);
    if(code_valid != 0U)
    {
        DebugLog(" ok\r\n");
    }
    else
    {
        DebugLog(" fail\r\n");
    }
#endif /* debugUART2_ENABLED && debugLOG_KEY_FLOW_ENABLED */
    if(code_valid == 0U)
    {
        if(start_block != BOOT_DEFAULT_START_BLOCK)
        {
            DBG_LOG_U16("[BOOT] fallback block=", BOOT_DEFAULT_START_BLOCK);
            start_block = BOOT_DEFAULT_START_BLOCK;
            code_valid = BootCodeCheck(start_block);
#if debugUART2_ENABLED && debugLOG_KEY_FLOW_ENABLED
            DebugLog("[BOOT] fallback check block=");
            DebugLogU16(start_block);
            if(code_valid != 0U)
            {
                DebugLog(" ok\r\n");
            }
            else
            {
                DebugLog(" fail\r\n");
            }
#endif /* debugUART2_ENABLED && debugLOG_KEY_FLOW_ENABLED */
        }
    }

    if(code_valid != 0U)
    {
        DBG_LOG_LINE("[BOOT] load app");
        BootLoadApp();
    }

    DBG_LOG_LINE("[BOOT] invalid app halt");
    while(1)
    {
    }
}
