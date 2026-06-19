/**
 * @file    boot.c
 * @brief   启动加载器应用加载与校验实现。
 * @details APP RAM拷贝、CRC校验和跳转行为保持原T5L51_SHJB BOOT工程一致。
 * @author  yangming
 * @version 1.0.0
 */

#include "boot.h"
#include "uart.h"

#define bootAPP_DATA_BYTES 65280UL
#define bootAPP_DWORD_COUNT (uint16_t)(bootAPP_DATA_BYTES / 4UL)
#define bootAPP_CRC_EXTRA_BYTES 2U
#define bootF000_CACHE_WORDS 2048U
#define bootF000_CACHE_BYTES (bootF000_CACHE_WORDS * 2U)
#define bootCODE_TARGET_VP_START 0x10000UL
#define bootCODE_COPY_BLOCK_WORDS 0x0800U
#define bootCODE_COPY_BLOCK_COUNT 16U

static uint8_t xdata BootVpF000Cache[bootF000_CACHE_BYTES];

/**
 * @brief 从VP 0x0020读取4字节启动控制值。
 * @param[out] control_buf 4字节控制缓存。
 */
static void BootReadControl(uint8_t *control_buf)
{
    read_dgus_vp(BOOT_CTRL_ADDR, control_buf, 2U);
}

/**
 * @brief 读取1个VP字。
 * @param[in] addr VP字地址。
 * @return VP字值。
 */
static uint16_t BootReadVpWord(uint16_t addr)
{
    uint8_t buf[2];

    read_dgus_vp(addr, buf, 1U);
    return ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
}

/**
 * @brief 写入1个VP字。
 * @param[in] addr VP字地址。
 * @param[in] value VP字值。
 */
static void BootWriteVpWord(uint16_t addr, uint16_t value)
{
    uint8_t buf[2];

    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)value;
    write_dgus_vp(addr, buf, 1U);
}

/**
 * @brief 写入启动控制值，并可同步保存到片内NOR Flash。
 * @param[in] control_buf 4字节控制值。
 * @param[in] persist 非0时保存到NOR Flash。
 */
void BootSetControl(uint8_t *control_buf, uint8_t persist)
{
    if(control_buf == NULL)
    {
        return;
    }

    write_dgus_vp(BOOT_CTRL_ADDR, control_buf, 2U);
    if(persist != 0U)
    {
        DgusToFlash((uint32_t)BOOT_CTRL_ADDR, BOOT_CTRL_ADDR, 2U);
    }
}

/**
 * @brief 写入默认用户程序起始块控制值。
 */
void BootSetDefaultLoadControl(void)
{
    uint8_t control_buf[BOOT_CTRL_BYTES];

    control_buf[0] = BOOT_CTRL_LOAD_0;
    control_buf[1] = BOOT_CTRL_LOAD_1;
    control_buf[2] = (uint8_t)(BOOT_DEFAULT_START_BLOCK >> 8);
    control_buf[3] = (uint8_t)BOOT_DEFAULT_START_BLOCK;
    BootSetControl(control_buf, 1U);
}

/**
 * @brief 从片内NOR Flash恢复BOOT配置到DGUS VP。
 */
void BootReloadConfigFromFlash(void)
{
    FlashToDgus((uint32_t)BOOT_CTRL_ADDR, BOOT_CTRL_ADDR, BOOT_CONFIG_WORDS);
}

/**
 * @brief 写入升级进度。
 * @param[in] progress 0到100。
 */
void BootWriteProgress(uint8_t progress)
{
    if(progress > 100U)
    {
        progress = 100U;
    }

    BootWriteVpWord(BOOT_PROGRESS_ADDR, (uint16_t)progress);
}

/**
 * @brief 按配置决定是否切换到指定配置地址中的页面。
 * @param[in] page_addr 存放页面ID的VP地址。
 */
void BootSwitchConfiguredPage(uint16_t page_addr)
{
    uint16_t page_id;

    if(BootReadVpWord(BOOT_PAGE_SWITCH_ADDR) != BOOT_PAGE_SWITCH_VALUE)
    {
        return;
    }

    page_id = BootReadVpWord(page_addr);
    SwitchPageById(page_id);
}

/**
 * @brief 判断VP 0x0020是否请求进入升级模式。
 * @return 控制值为5A A5 5A A5时返回1，否则返回0。
 */
uint8_t BootIsUpgradeRequested(void)
{
    uint8_t control_buf[BOOT_CTRL_BYTES];

    BootReadControl(control_buf);
    if((control_buf[0] == BOOT_CTRL_UPGRADE_0) &&
       (control_buf[1] == BOOT_CTRL_UPGRADE_1) &&
       (control_buf[2] == BOOT_CTRL_UPGRADE_2) &&
       (control_buf[3] == BOOT_CTRL_UPGRADE_3))
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief 判断VP 0x0020是否为AA55动态加载命令。
 * @return 控制值以AA 55开头时返回1，否则返回0。
 */
uint8_t BootControlIsLoadCommand(void)
{
    uint8_t control_buf[BOOT_CTRL_BYTES];

    BootReadControl(control_buf);
    if((control_buf[0] == BOOT_CTRL_LOAD_0) &&
       (control_buf[1] == BOOT_CTRL_LOAD_1))
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief 从VP 0x0020解析NOR起始块。
 * @return 存在AA55xxxx时返回动态块号，否则返回默认块号。
 */
uint16_t BootResolveStartBlock(void)
{
    uint8_t control_buf[BOOT_CTRL_BYTES];

    BootReadControl(control_buf);
    if((control_buf[0] == BOOT_CTRL_LOAD_0) &&
       (control_buf[1] == BOOT_CTRL_LOAD_1))
    {
        return ((uint16_t)control_buf[2] << 8) | (uint16_t)control_buf[3];
    }

    return BOOT_DEFAULT_START_BLOCK;
}

/**
 * @brief 等待升级端写入AA55动态加载命令。
 * @param[in] timeout_ms 等待时间，单位毫秒。
 * @return 检测到AA55xxxx时返回1，否则返回0。
 */
uint8_t BootWaitLoadCommand(uint32_t timeout_ms)
{
    uint32_t elapsed_ms;
    uint8_t control_buf[BOOT_CTRL_BYTES];
    uint8_t recovery_type;

    elapsed_ms = 0UL;
    while(elapsed_ms < timeout_ms)
    {
        recovery_type = UartRecoveryGetControl(control_buf);
        if(recovery_type != 0U)
        {
            if((control_buf[0] == BOOT_CTRL_LOAD_0) &&
               (control_buf[1] == BOOT_CTRL_LOAD_1))
            {
                BootSetControl(control_buf, 1U);
                return 1U;
            }
        }

        if(BootControlIsLoadCommand() != 0U)
        {
            BootReadControl(control_buf);
            BootSetControl(control_buf, 1U);
            return 1U;
        }

        UartReadFrame(&Uart5);
        delay_ms(10U);
        elapsed_ms += 10UL;
    }

    BootSetDefaultLoadControl();
    return 0U;
}

/**
 * @brief 清除VP 0x0020启动控制值并同步到NOR Flash。
 */
void BootClearControl(void)
{
    uint8_t zero_buf[BOOT_CTRL_BYTES];

    zero_buf[0] = 0U;
    zero_buf[1] = 0U;
    zero_buf[2] = 0U;
    zero_buf[3] = 0U;
    BootSetControl(zero_buf, 1U);
}

/**
 * @brief 将已暂存的APP代码从VP空间拷贝到执行RAM并跳转。
 * @warning 该函数通过链接器固定在boot交接段。
 */
void BootLoadApp(void)
{
#pragma ASM
        ANL     PSW,#0E7H
        MOV     D_PAGESEL,#01H
        MOV     DPTR,#0000H
        MOV     DPC,#01H
        MOV     ADR_H,#00H
        MOV     ADR_M,#80H
        MOV     ADR_L,#00H
        MOV     ADR_INC,#01H
        MOV     RAMMODE,#0AFH
        JNB     APP_ACK,$
        MOV     R0,#64
READC1: MOV     R1,#255
READC2: SETB    APP_EN
        JB      APP_EN,$
        MOV     A,DATA3
        MOVX    @DPTR,A
        MOV     A,DATA2
        MOVX    @DPTR,A
        MOV     A,DATA1
        MOVX    @DPTR,A
        MOV     A,DATA0
        MOVX    @DPTR,A
        DJNZ    R1,READC2
        DJNZ    R0,READC1

        MOV     R0,#08H
        MOV     R1,#248
DATACLR:MOV     @R0,#00
        INC     R0
        DJNZ    R1,DATACLR

        MOV     D_PAGESEL,#02H
        MOV     DPTR,#8000H
RAMCLR: CLR     A
        MOVX    @DPTR,A
        MOVX    @DPTR,A
        MOVX    @DPTR,A
        MOVX    @DPTR,A
        MOV     A,DPH
        JNZ     RAMCLR
        MOV     R0,#00H
        MOV     R1,#00H
        MOV     R2,#00H
        MOV     R3,#00H
        MOV     R4,#00H
        MOV     R5,#00H
        MOV     R6,#00H
        MOV     R7,#00H
        MOV     DPC,#00H
#pragma ENDASM
    ((void (*)(void))0)();
}

/**
 * @brief 将1字节数据送入原BOOT使用的Modbus风格CRC16。
 * @param[in] crc 当前CRC值。
 * @param[in] data_byte 输入字节。
 * @return 更新后的CRC值。
 */
static uint16_t BootCrc16Update(uint16_t crc, uint8_t data_byte)
{
    uint8_t bit_index;

    crc ^= data_byte;
    for(bit_index = 0U; bit_index < 8U; bit_index++)
    {
        if((crc & 0x0001U) != 0U)
        {
            crc = (crc >> 1) ^ 0xA001U;
        }
        else
        {
            crc >>= 1;
        }
    }

    return crc;
}

/**
 * @brief 从APP RAM计算用户代码CRC。
 * @return CRC值；有效APP连同存储CRC一起计算后结果为0。
 */
static uint16_t BootCodeCrc16(void)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;

    ADR_H = 0U;
    ADR_M = 0x80U;
    ADR_L = 0x00U;
    ADR_INC = 1U;
    RAMMODE = 0xAFU;
    while(!APP_ACK)
    {
    }

    for(i = 0U; i < bootAPP_DWORD_COUNT; i++)
    {
        APP_EN = 1U;
        while(APP_EN)
        {
        }
        crc = BootCrc16Update(crc, DATA3);
        crc = BootCrc16Update(crc, DATA2);
        crc = BootCrc16Update(crc, DATA1);
        crc = BootCrc16Update(crc, DATA0);
    }

    APP_EN = 1U;
    while(APP_EN)
    {
    }
    crc = BootCrc16Update(crc, DATA3);
    crc = BootCrc16Update(crc, DATA2);

    RAMMODE = 0U;
    return crc;
}

/**
 * @brief 等待DGUS命令寄存器空闲。
 * @param[in] cmd_addr 命令VP地址。
 */
static void BootWaitDgusCmdIdle(uint16_t cmd_addr)
{
    uint8_t cmd_state[4];

    do
    {
        delay_ms(2U);
        read_dgus_vp(cmd_addr, cmd_state, 2U);
    }while(cmd_state[0] != 0U);
}

/**
 * @brief 将1个4KB NOR代码块读入暂存VP区域。
 * @param[in] flash_addr 源NOR字地址。
 */
static void BootReadNorCodeBlock(uint32_t flash_addr)
{
    uint8_t cmd[12];

    cmd[0] = 0x5AU;
    cmd[1] = (uint8_t)(flash_addr >> 16);
    cmd[2] = (uint8_t)(flash_addr >> 8);
    cmd[3] = (uint8_t)flash_addr;
    cmd[4] = 0xF0U;
    cmd[5] = 0x00U;
    cmd[6] = (uint8_t)(bootCODE_COPY_BLOCK_WORDS >> 8);
    cmd[7] = (uint8_t)bootCODE_COPY_BLOCK_WORDS;
    cmd[8] = 0U;
    cmd[9] = 0U;
    cmd[10] = 0U;
    cmd[11] = 0U;

    write_dgus_vp(sysDGUS_FLASH_RW_CMD_ADDR, cmd, 4U);
    BootWaitDgusCmdIdle(sysDGUS_FLASH_RW_CMD_ADDR);
    delay_ms(50U);
}

/**
 * @brief 从指定NOR起始块拷贝代码到APP RAM并校验CRC。
 * @param[in] start_block NOR 4KB块号。
 * @return 用户代码有效时返回1，否则返回0。
 */
uint8_t BootCodeCheck(uint16_t start_block)
{
    uint32_t flash_addr;
    uint32_t target_vp;
    uint8_t i;
    VP_EXCHANGE exchange_msg;

    read_dgus_vp(0xF000U, BootVpF000Cache, bootF000_CACHE_WORDS);

    flash_addr = (uint32_t)start_block << 11;
    target_vp = bootCODE_TARGET_VP_START;

    for(i = 0U; i < bootCODE_COPY_BLOCK_COUNT; i++)
    {
        BootReadNorCodeBlock(flash_addr);

        exchange_msg.sourceVP = 0xF000UL;
        exchange_msg.targetVP = target_vp;
        exchange_msg.len = bootF000_CACHE_WORDS;
        exchange_msg.mode = 0U;
        vpExchange(&exchange_msg);

        flash_addr += bootCODE_COPY_BLOCK_WORDS;
        target_vp += bootCODE_COPY_BLOCK_WORDS;
    }

    write_dgus_vp(0xF000U, BootVpF000Cache, bootF000_CACHE_WORDS);

    if(BootCodeCrc16() == 0U)
    {
        return 1U;
    }

    return 0U;
}
