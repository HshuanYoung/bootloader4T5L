/**
 * @file    sys.c
 * @brief   T5L启动加载器系统服务实现。
 * @details 保留原BOOT工程的DGUS APP RAM访问方式，同时使用Template4T5L源码组织风格。
 * @author  yangming
 * @version 1.0.0
 */

#include "sys.h"

#define dgusVP_COPY_LIMIT 0x20000UL
#define dgusCMD_WAIT_STEP_MS 10U
#define dgusFLASH_CMD_WAIT_TIMEOUT_MS 3000U

static uint8_t DgusWaitCmdIdle(uint32_t cmd_addr, uint16_t timeout_ms)
{
    uint8_t cmd_state[2];
    uint16_t elapsed_ms;

    elapsed_ms = 0U;
    cmd_state[0] = 0xFFU;
    while(elapsed_ms < timeout_ms)
    {
        delay_ms(dgusCMD_WAIT_STEP_MS);
        read_dgus_vp(cmd_addr, cmd_state, 1U);
        if(cmd_state[0] == 0U)
        {
            return 1U;
        }
        elapsed_ms += dgusCMD_WAIT_STEP_MS;
    }

    return 0U;
}

void read_dgus_vp(uint32_t addr,uint8_t *buf,uint16_t len)
{
	uint32_t OS_addr = 0;
	uint16_t OS_addr_offset = 0;
	uint16_t OS_len = 0, OS_len_offset = 0;

	OS_addr = addr >> 1;
	OS_addr_offset = addr & 0x01;
    ADR_H=(uint8_t)(OS_addr>>16);
    ADR_M=(uint8_t)(OS_addr>>8);
    ADR_L=(uint8_t)OS_addr;
    ADR_INC=1;                 
    RAMMODE=0xAF;               
	while(!APP_ACK);     
    if(OS_addr_offset)       
    {
        APP_EN=1;
        while(APP_EN); 
        *buf++=DATA1;
        *buf++=DATA0;              
        len--;
    }
    OS_len=len>>1;
    OS_len_offset=len&0x01;
    while(OS_len--)
    {
        APP_EN=1;
        while(APP_EN);      
        *buf++=DATA3;
        *buf++=DATA2;
        *buf++=DATA1;
        *buf++=DATA0;                          
    }   
	if(OS_len_offset)
	{          
		APP_EN=1;
		while(APP_EN);      
		*buf++=DATA3;
		*buf++=DATA2;              
	} 
    RAMMODE=0x00;           
}


void write_dgus_vp ( uint32_t  addr, uint8_t *buf, uint16_t len )
{
    uint8_t i;
    uint8_t *p = buf;
    i= ( unsigned char ) ( addr&0x01 );

    ADR_H= ( unsigned char ) ( addr>>17 );
    ADR_M= ( unsigned char ) ( addr>>9 );
    ADR_L= ( unsigned char ) ( addr>>1 );
    ADR_INC=0x01;
    RAMMODE=0x8F;
    while ( APP_ACK==0 );
    if ( i && len>0 )
    {
        RAMMODE=0x83;
        DATA1=*p++;
        DATA0=*p++;
        APP_EN=1;
        while ( APP_EN==1 );
        len--;
    }
    RAMMODE=0x8F;
    while ( len>=2 )
    {
        DATA3=*p++;
        DATA2=*p++;
        DATA1=*p++;
        DATA0=*p++;
        APP_EN=1;
        while ( APP_EN==1 );
        len=len-2;
    }
    if ( len )
    {
        RAMMODE=0x8C;
        DATA3=*p++;
        DATA2=*p++;
        APP_EN=1;
        while ( APP_EN==1 );
    }
    RAMMODE=0x00;

}

/**
 * @brief 忙等待微秒延时。
 * @param[in] us 近似延时时间，单位微秒。
 */
void delay_us(uint16_t us)
{
    uint8_t i;

    while(us-- != 0U)
    {
        for(i = 0U; i < 50U; i++)
        {
            i = i;
        }
    }
}

/**
 * @brief 忙等待毫秒延时。
 * @param[in] ms 近似延时时间，单位毫秒。
 */
void delay_ms(uint16_t ms)
{
    uint16_t i;
    uint16_t j;

    for(i = 0U; i < ms; i++)
    {
        for(j = 0U; j < 300U; j++)
        {
            delay_us(1U);
        }
    }
}

/**
 * @brief 将片内NOR Flash数据读回DGUS VP。
 * @param[in] flash_addr 片内NOR Flash字地址。
 * @param[in] dgus_vp_addr DGUS VP目标地址。
 * @param[in] len_words 读取字长度。
 */
void FlashToDgus(uint32_t flash_addr, uint16_t dgus_vp_addr, uint16_t len_words)
{
    uint8_t cmd[8];

    if(len_words == 0U)
    {
        return;
    }

    cmd[0] = 0x5AU;
    cmd[1] = (uint8_t)(flash_addr >> 16);
    cmd[2] = (uint8_t)(flash_addr >> 8);
    cmd[3] = (uint8_t)flash_addr;
    cmd[4] = (uint8_t)(dgus_vp_addr >> 8);
    cmd[5] = (uint8_t)dgus_vp_addr;
    cmd[6] = (uint8_t)(len_words >> 8);
    cmd[7] = (uint8_t)len_words;

    write_dgus_vp(sysDGUS_FLASH_RW_CMD_ADDR + 1U, &cmd[2], 3U);
    write_dgus_vp(sysDGUS_FLASH_RW_CMD_ADDR, cmd, 1U);
    (void)DgusWaitCmdIdle(sysDGUS_FLASH_RW_CMD_ADDR, dgusFLASH_CMD_WAIT_TIMEOUT_MS);
    delay_ms(FLASH_ACCESS_CYCLE);
}

/**
 * @brief 将DGUS VP数据写入片内NOR Flash。
 * @param[in] flash_addr 片内NOR Flash字地址。
 * @param[in] dgus_vp_addr DGUS VP源地址。
 * @param[in] len_words 写入字长度。
 */
void DgusToFlash(uint32_t flash_addr, uint16_t dgus_vp_addr, uint16_t len_words)
{
    uint8_t cmd[8];

    if(len_words == 0U)
    {
        return;
    }

    cmd[0] = 0xA5U;
    cmd[1] = (uint8_t)(flash_addr >> 16);
    cmd[2] = (uint8_t)(flash_addr >> 8);
    cmd[3] = (uint8_t)flash_addr;
    cmd[4] = (uint8_t)(dgus_vp_addr >> 8);
    cmd[5] = (uint8_t)dgus_vp_addr;
    cmd[6] = (uint8_t)(len_words >> 8);
    cmd[7] = (uint8_t)len_words;

    write_dgus_vp(sysDGUS_FLASH_RW_CMD_ADDR + 1U, &cmd[2], 3U);
    write_dgus_vp(sysDGUS_FLASH_RW_CMD_ADDR, cmd, 1U);
    (void)DgusWaitCmdIdle(sysDGUS_FLASH_RW_CMD_ADDR, dgusFLASH_CMD_WAIT_TIMEOUT_MS);
    delay_ms(FLASH_ACCESS_CYCLE);
}

/**
 * @brief 按页面ID切换DGUS页面，等待OS完成切页命令。
 * @param[in] page_id 页面ID。
 */
void SwitchPageById(uint16_t page_id)
{
    uint8_t cmd[4];
    uint8_t state[2];

    cmd[0] = 0x5AU;
    cmd[1] = 0x01U;
    cmd[2] = (uint8_t)(page_id >> 8);
    cmd[3] = (uint8_t)page_id;
    write_dgus_vp(sysDGUS_PIC_SET_ADDR, cmd, 2U);

    do
    {
        delay_us(100U);
        read_dgus_vp(sysDGUS_PIC_SET_ADDR, state, 1U);
    }while(state[0] != 0U);
}

/**
 * @brief 触发T5L软复位。
 */
void SoftReset(void)
{
    uint8_t cmd[4];

    cmd[0] = 0x55U;
    cmd[1] = 0xAAU;
    cmd[2] = 0x5AU;
    cmd[3] = 0xA5U;
    write_dgus_vp(sysDGUS_BOOT_RESET_ADDR, cmd, 2U);
}

/**
 * @brief 在VP区域之间拷贝数据。
 * @param[in] exchange_msg VP拷贝请求。
 */
void vpExchange(VP_EXCHANGE *exchange_msg)
{
    uint32_t vp_gap;
    uint16_t source_dword_vp;
    uint16_t target_dword_vp;
    uint16_t dword_len;
    uint16_t i;

    if(exchange_msg == NULL)
    {
        return;
    }

    if(exchange_msg->sourceVP >= dgusVP_COPY_LIMIT)
    {
        return;
    }
    if(exchange_msg->targetVP >= dgusVP_COPY_LIMIT)
    {
        return;
    }
    if((exchange_msg->sourceVP + exchange_msg->len) >= dgusVP_COPY_LIMIT)
    {
        return;
    }
    if((exchange_msg->targetVP + exchange_msg->len) >= dgusVP_COPY_LIMIT)
    {
        return;
    }

    if(exchange_msg->sourceVP > exchange_msg->targetVP)
    {
        vp_gap = exchange_msg->sourceVP - exchange_msg->targetVP;
    }
    else
    {
        vp_gap = exchange_msg->targetVP - exchange_msg->sourceVP;
    }

    if(exchange_msg->len > vp_gap)
    {
        return;
    }

    source_dword_vp = (uint16_t)(exchange_msg->sourceVP >> 1);
    target_dword_vp = (uint16_t)(exchange_msg->targetVP >> 1);
    dword_len = exchange_msg->len >> 1;

#ifdef INTVPACTION
    EA = 0;
#endif /* INTVPACTION */

    ADR_H = 0U;
    ADR_INC = 0U;
    RAMMODE = 0xAFU;
    while(!APP_ACK)
    {
    }

    for(i = 0U; i < dword_len; i++)
    {
        APP_RW = 1U;
        ADR_M = (uint8_t)(source_dword_vp >> 8);
        ADR_L = (uint8_t)source_dword_vp;
        APP_EN = 1U;
        while(APP_EN)
        {
        }

        APP_RW = 0U;
        ADR_M = (uint8_t)(target_dword_vp >> 8);
        ADR_L = (uint8_t)target_dword_vp;
        APP_EN = 1U;
        while(APP_EN)
        {
        }

        source_dword_vp++;
        target_dword_vp++;
    }

    if(exchange_msg->mode != 0U)
    {
        APP_RW = 0U;
        ADR_INC = 1U;
        source_dword_vp = (uint16_t)(exchange_msg->sourceVP >> 1);
        ADR_M = (uint8_t)(source_dword_vp >> 8);
        ADR_L = (uint8_t)source_dword_vp;
        DATA3 = 0U;
        DATA2 = 0U;
        DATA1 = 0U;
        DATA0 = 0U;

        for(i = 0U; i < dword_len; i++)
        {
            APP_EN = 1U;
            while(APP_EN)
            {
            }
        }
    }

    RAMMODE = 0x00U;

#ifdef INTVPACTION
    EA = 1;
#endif /* INTVPACTION */
}
