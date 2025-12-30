/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_RISCV_DEV_AX__H__
#define __OSAL_RISCV_DEV_AX__H__

#include "ax_base_type.h"
#include "ax_global_type.h"

typedef struct AX_POLL {
    AX_VOID *poll_table;
    AX_VOID *data;
    AX_VOID *wait; /*AX_WAIT, only support one poll, read or write*/
} AX_POLL_T;

typedef struct AX_FILEOPS {
    AX_S32(*open)(AX_VOID *private_data);
    AX_S32(*read)(AX_S8 *buf, AX_S32 size, AX_LONG *offset, AX_VOID *private_data);
    AX_S32(*write)(const AX_S8 *buf, AX_S32 size, AX_LONG *offset, AX_VOID *private_data);
    AX_LONG(*llseek)(AX_LONG offset, AX_S32 whence, AX_VOID *private_data);
    AX_S32(*release)(AX_VOID *private_data);
    AX_LONG(*unlocked_ioctl)(AX_U32 cmd, AX_ULONG arg, AX_VOID *private_data);
    AX_U32(*poll)(AX_POLL_T *osal_poll, AX_VOID *private_data);
} AX_FILEOPS_T;

typedef struct AX_WAIT {
    AX_VOID *wait;
} AX_WAIT_T;

typedef struct AX_DEV {
    AX_S8 name[48];
    AX_VOID *dev;
    AX_S32 minor;
    struct AX_FILEOPS *fops;
    struct AX_PMOPS *osal_pmops;
    AX_VOID *private_data;
    struct AX_POLL dev_poll;
    struct AX_WAIT dev_wait;
} AX_DEV_T;

#endif
