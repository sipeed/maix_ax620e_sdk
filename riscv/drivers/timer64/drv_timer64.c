/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "drv_timer64.h"
#include "soc.h"
#include "ax_common.h"

uint64_t t64_get_val(void)
{
    uint64_t r;

    uint32_t high0 = ax_readl(TMR64_0_BASE_ADDR + 0x04);
    uint32_t low = ax_readl(TMR64_0_BASE_ADDR);
    uint32_t high1 = ax_readl(TMR64_0_BASE_ADDR + 0x04);

    if (high0 != high1 && low > 0x7fffffff) {
        r = high0;
    } else {
        r = high1;
    }

    r <<= 32;
    r += low;

    return r;
}

uint32_t t64_calc_dur_us(uint64_t start, uint64_t end)
{
    uint32_t r;
    uint64_t us = (end - start) / TMR64_0_FREQ_M;
    if (us <= U32_MAX_VAL) {
        r = us;
    } else {
        r = U32_MAX_VAL;
    }

    return r;
}

uint32_t t64_calc_dur_ms(uint64_t start, uint64_t end)
{
    uint32_t r;

    uint64_t ms = (end - start) / (TMR64_0_FREQ_M * T64_MS_2_US);
    if (ms <= U32_MAX_VAL) {
        r = ms;
    } else {
        r = U32_MAX_VAL;
    }

    return r;
}

void t64_udelay(uint32_t us)
{
    uint64_t now = t64_get_val();
    uint64_t end = now + us * TMR64_0_FREQ_M;

    while (1) {
        now = t64_get_val();
        if (now >= end) {
            break;
        }
    }
}

void t64_mdelay(uint32_t ms)
{
    uint64_t now = t64_get_val();
    uint64_t end = now + ms * TMR64_0_FREQ_M * T64_MS_2_US;

    while (1) {
        now = t64_get_val();
        if (now >= end) {
            break;
        }
    }
}
