/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-20     zx.chen      add E906 ram size
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <csi_core.h>
#include "soc.h"

#define RUN_IN_DDR

#ifdef RUN_IN_IRAM
#define HEAP_RAM_SIZE          	(16+4)
#define HEAP_RAM_END            (0x00000000 + HEAP_RAM_SIZE * 1024)
#endif

#ifdef RUN_IN_OCM
#define HEAP_RAM_SIZE           2560
#define HEAP_RAM_END            (0x3000000 + HEAP_RAM_SIZE * 1024)
#endif

#ifdef RUN_IN_DDR
#define HEAP_RAM_SIZE           (RISCV_MEM_LEN_MB)
#define HEAP_RAM_END            (RISCV_DDR_BASE + HEAP_RAM_SIZE * 1024 * 1024)
#endif

extern int __bss_end__;
#define HEAP_BEGIN              (&__bss_end__)

#endif /* __BOARD_H__ */
