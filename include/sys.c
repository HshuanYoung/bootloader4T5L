/**
 * @file    sys.c
 * @brief   T5L启动加载器系统服务实现。
 * @details 保留原BOOT工程的DGUS APP RAM访问方式，同时使用Template4T5L源码组织风格。
 * @author  yangming
 * @version 1.0.0
 */

#include "sys.h"

#define dgusVP_ADDR_LIMIT 0xFFFFU
#define dgusVP_COPY_LIMIT 0x20000UL

/**
 * @brief 限制VP字长度，避免跨越16位DGUS VP窗口。
 * @param[in] addr 起始VP字地址。
 * @param[in,out] len 请求访问的VP字长度。
 */
static void DgusClampWordLength(uint16_t addr, uint16_t *len)
{
    uint32_t len_limit;

    len_limit = (uint32_t)dgusVP_ADDR_LIMIT - (uint32_t)addr + 1UL;
    if(len_limit < (uint32_t)(*len))
    {
        *len = (uint16_t)len_limit;
    }
}

/**
 * @brief 读取DGUS VP数据。
 * @param[in] addr VP字地址。
 * @param[out] buf 目标字节缓存。
 * @param[in] len VP字长度。
 */
void read_dgus_vp(uint16_t addr, uint8_t *buf, uint16_t len)
{
    uint16_t os_addr;
    uint16_t os_addr_offset;
    uint16_t os_len;
    uint16_t os_len_offset;

    if((buf == NULL) || (len == 0U))
    {
        return;
    }

    DgusClampWordLength(addr, &len);

    os_addr = addr >> 1;
    os_addr_offset = addr & 0x01U;

#ifdef INTVPACTION
    EA = 0;
#endif /* INTVPACTION */

    ADR_H = 0U;
    ADR_M = (uint8_t)(os_addr >> 8);
    ADR_L = (uint8_t)os_addr;
    ADR_INC = 1U;
    RAMMODE = 0xAFU;
    while(!APP_ACK)
    {
    }

    if(os_addr_offset != 0U)
    {
        APP_EN = 1U;
        while(APP_EN)
        {
        }
        *buf++ = DATA1;
        *buf++ = DATA0;
        len--;
    }

    os_len = len >> 1;
    os_len_offset = len & 0x01U;
    while(os_len-- != 0U)
    {
        APP_EN = 1U;
        while(APP_EN)
        {
        }
        *buf++ = DATA3;
        *buf++ = DATA2;
        *buf++ = DATA1;
        *buf++ = DATA0;
    }

    if(os_len_offset != 0U)
    {
        APP_EN = 1U;
        while(APP_EN)
        {
        }
        *buf++ = DATA3;
        *buf = DATA2;
    }

    RAMMODE = 0x00U;

#ifdef INTVPACTION
    EA = 1;
#endif /* INTVPACTION */
}

/**
 * @brief 写入DGUS VP数据。
 * @param[in] addr VP字地址。
 * @param[in] buf 源字节缓存。
 * @param[in] len VP字长度。
 */
void write_dgus_vp(uint16_t addr, uint8_t *buf, uint16_t len)
{
    uint16_t os_addr;
    uint16_t os_addr_offset;
    uint16_t os_len;
    uint16_t os_len_offset;

    if((buf == NULL) || (len == 0U))
    {
        return;
    }

    DgusClampWordLength(addr, &len);

    os_addr = addr >> 1;
    os_addr_offset = addr & 0x01U;

#ifdef INTVPACTION
    EA = 0;
#endif /* INTVPACTION */

    ADR_H = 0U;
    ADR_M = (uint8_t)(os_addr >> 8);
    ADR_L = (uint8_t)os_addr;
    ADR_INC = 1U;
    RAMMODE = 0x83U;
    while(!APP_ACK)
    {
    }

    if(os_addr_offset != 0U)
    {
        DATA1 = *buf++;
        DATA0 = *buf++;
        APP_EN = 1U;
        while(APP_EN)
        {
        }
        len--;
    }

    os_len = len >> 1;
    os_len_offset = len & 0x01U;
    RAMMODE = 0x8FU;
    while(os_len-- != 0U)
    {
        DATA3 = *buf++;
        DATA2 = *buf++;
        DATA1 = *buf++;
        DATA0 = *buf++;
        APP_EN = 1U;
        while(APP_EN)
        {
        }
    }

    if(os_len_offset != 0U)
    {
        RAMMODE = 0x8CU;
        DATA3 = *buf++;
        DATA2 = *buf;
        APP_EN = 1U;
        while(APP_EN)
        {
        }
    }

    RAMMODE = 0x00U;

#ifdef INTVPACTION
    EA = 1;
#endif /* INTVPACTION */
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
