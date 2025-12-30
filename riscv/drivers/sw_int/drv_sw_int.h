/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_SW_INT_H__
#define __DRV_SW_INT_H__

#include <stdint.h>
#include <stdbool.h>

/* ========================================================================================= */
/* ============  Notes: You should list the used sw_int here to avoid conflict  ============ */
/* ============  sw_int_group_3 sw_int_channel_31: for riscv verify             ============ */
/* ============  sw_int_group_3 sw_int_channel_30: for riscv verify             ============ */
/* ============  sw_int_group_3 sw_int_channel_29: for load rootfs              ============ */
/* ============  sw_int_group_3 sw_int_channel_28: for suspend                  ============ */
/* ============  sw_int_group_3 sw_int_channel_27: for resume                   ============ */
/* ============  Others: unused                                                 ============ */
/* ========================================================================================= */

typedef void(*sw_int_cb_t)(void *param);

#define SW_INT_TRIGGER_OFFSET   0x10

typedef enum {
    sw_int_group_0 = 0,
    sw_int_group_1,
    sw_int_group_2,
    sw_int_group_3,
    sw_int_group_max
} sw_int_group_e;

typedef enum {
    sw_int_channel_0 = 0,
    sw_int_channel_1,
    sw_int_channel_2,
    sw_int_channel_3,
    sw_int_channel_4,
    sw_int_channel_5,
    sw_int_channel_6,
    sw_int_channel_7,
    sw_int_channel_8,
    sw_int_channel_9,
    sw_int_channel_10,
    sw_int_channel_11,
    sw_int_channel_12,
    sw_int_channel_13,
    sw_int_channel_14,
    sw_int_channel_15,
    sw_int_channel_16,
    sw_int_channel_17,
    sw_int_channel_18,
    sw_int_channel_19,
    sw_int_channel_20,
    sw_int_channel_21,
    sw_int_channel_22,
    sw_int_channel_23,
    sw_int_channel_24,
    sw_int_channel_25,
    sw_int_channel_26,
    sw_int_channel_27,
    sw_int_channel_28,
    sw_int_channel_29,
    sw_int_channel_30,
    sw_int_channel_31,
    sw_int_channel_max
} sw_int_channel_e;

int sw_int_cb_register(sw_int_group_e group, sw_int_channel_e channel, sw_int_cb_t cb, void *param);
int sw_int_cb_unregister(sw_int_group_e group, sw_int_channel_e channel);
void riscv_sw_int_trigger(sw_int_group_e group, sw_int_channel_e channel);

#endif //__DRV_SW_INT_H__
