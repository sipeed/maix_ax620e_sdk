/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_COMMON_H__
#define __AX_COMMON_H__

#include <stdint.h>
#include "drv_timer64.h"
#include "ax_log.h"

typedef struct {
    uint32_t reg_val;
    uint32_t reg_addr;
} reg_info_t;

#define BIT_0   0
#define BIT_1   1
#define BIT_2   2
#define BIT_3   3
#define BIT_4   4
#define BIT_5   5
#define BIT_6   6
#define BIT_7   7
#define BIT_8   8
#define BIT_9   9
#define BIT_10  10
#define BIT_11  11
#define BIT_12  12
#define BIT_13  13
#define BIT_14  14
#define BIT_15  15
#define BIT_16  16
#define BIT_17  17
#define BIT_18  18
#define BIT_19  19
#define BIT_20  20
#define BIT_21  21
#define BIT_22  22
#define BIT_23  23
#define BIT_24  24
#define BIT_25  25
#define BIT_26  26
#define BIT_27  27
#define BIT_28  28
#define BIT_29  29
#define BIT_30  30
#define BIT_31  31

#define ax_udelay t64_udelay
#define ax_mdelay t64_mdelay

#define ax_readl(addr)           (*(volatile uint32_t *)(addr))
#define ax_writel(value, addr)   (*(volatile uint32_t *)(addr) = (value))

#endif //__AX_COMMON_H__
