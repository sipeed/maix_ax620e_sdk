/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_TIMESTAMP_H__
#define __AX_TIMESTAMP_H__

#include <stdint.h>
#include "soc.h"
#include "ax_common.h"

#define STAMP_SPL_START_LOAD_RISCV      0x954
#define STAMP_SPL_END_LOAD_RISCV        0x958
#define STAMP_RISCV_ENTRY_INIT          0x95c
#define STAMP_SPL_WAIT_RISCV_READY      0x960
#define STAMP_SPL_RISCV_READY           0x964
#define STAMP_RISCV_START_LOAD_ROOTFS   0x968
#define STAMP_RISCV_END_LOAD_ROOTFS     0x96c
#define STAMP_MOUNT_START_WAIT          0x970
#define STAMP_MOUNT_END_WAIT            0x974

static inline void ax_timestamp(uint32_t addr)
{
    uint32_t val = ax_readl(TMR64_0_BASE_ADDR);
    ax_writel(val, addr);
}

#endif //__AX_TIMESTAMP_H__

