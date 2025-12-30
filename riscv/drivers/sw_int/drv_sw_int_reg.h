/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_SW_INT_REG_H__
#define __DRV_SW_INT_REG_H__

#include <stdbool.h>
#include "drv_sw_int.h"

typedef struct {
    sw_int_group_e group;
    sw_int_channel_e channel;
    void *param;
    sw_int_cb_t cb;
    rt_list_t list;
} sw_int_cb_info_t;

typedef struct {
    volatile uint32_t int_en;
    volatile uint32_t int_en_set;
    volatile uint32_t int_en_clr;
    volatile uint32_t int_raw;
    volatile uint32_t int_raw_set;
    volatile uint32_t int_raw_clr;
    volatile uint32_t int_sta;
} sw_int_regs_t;

#endif //__DRV_SW_INT_REG_H__
