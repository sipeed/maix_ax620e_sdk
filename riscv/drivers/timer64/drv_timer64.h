/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_TIMER64_H__
#define __DRV_TIMER64_H__

#include <stdint.h>

#define U32_MAX_VAL 0xffffffff
#define T64_MS_2_US 1000

uint64_t t64_get_val(void);
uint32_t t64_calc_dur_us(uint64_t start, uint64_t end);
uint32_t t64_calc_dur_ms(uint64_t start, uint64_t end);
void t64_udelay(uint32_t us);
void t64_mdelay(uint32_t ms);

#endif //__DRV_TIMER64_H__

