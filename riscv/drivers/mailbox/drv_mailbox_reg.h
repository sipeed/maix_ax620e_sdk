/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_MAILBOX_REG_H__
#define __DRV_MAILBOX_REG_H__

#include <rtdevice.h>

typedef struct {
    uint8_t id;
    void *data;
    mbox_callback_t callback;
    rt_list_t list;
} mbox_callback_info_t;

#define NUM_CHANS                    32

#define AX_MAILBOX_MASTER_ID_ARM0    0
#define AX_MAILBOX_MASTER_ID_DSP     1
#define AX_MAILBOX_MASTER_ID_RISCV   2

#define INT_STATUS_REG_OFFSET    ((0x3UL << 8) | (AX_MAILBOX_MASTER_ID_RISCV << 4) | (0x00UL << 2))
#define INT_CLEAR_REG_OFFSET     ((0x3UL << 8) | (AX_MAILBOX_MASTER_ID_RISCV << 4) | (0x01UL << 2))
#define REG_QUERY_REG_OFFSET     ((0x3UL << 8) | (AX_MAILBOX_MASTER_ID_RISCV << 4) | (0x02UL << 2))
#define REG_LOCK_REG_OFFSET      ((0x3UL << 8) | (AX_MAILBOX_MASTER_ID_RISCV << 4) | (0x03UL << 2))

#define INFO_REG_X(x)    ((0x1UL << 8) | ((x) << 2)) // G = 1
#define DATA_REG_X(x)    ((0x0UL << 8) | ((x) << 2)) // G = 0

#endif //__DRV_MAILBOX_REG_H__

