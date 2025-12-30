/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __LOAD_ROOTFS_H__
#define __LOAD_ROOTFS_H__

#include <stdbool.h>
#include "cmn.h"

#define DEBUG_ROOTFS_TRACE_BASE 0x4300 //iram0 addr
#define DEBUG_MODEL_TRACE_BASE 0x4310 //iram0 addr

#define ROOTFS_START                    BIT(0)
#define MODEL_START                     BIT(0)
#define BOOT_PROCESS                    BIT(1)
#define FLASH_INIT_FAILED               BIT(2)
#define FLASH_READ_HEAD_FAILED          BIT(3)
#define READ_HEAD_SUCCESS               BIT(4)
#define READ_IMAGE_START                BIT(5)
#define GZIP_FLASH_READ_HEAD_FAILED     BIT(6)
#define GZIP_GET_HEAD_INFO_ERROR        BIT(7)
#define GZIP_RUN_LAST_TILE_ERROR        BIT(8)
#define GZIP_CHECK_GZIP_ST_ERROR        BIT(9)
#define GZIP_FLASH_READ_IMAGE_FAILED    BIT(10)
#define GZIP_DEV_RUN_ERROR              BIT(11)
#define GZIP_WATI_FINISH_ERROR          BIT(12)
#define GZIP_DECOMPRESS_SUCCESS         BIT(13)
#define READ_IMAGE_SUCCESS              BIT(14)

int read_rootfs(void);
void debug_trace(int flag);

#endif //__LOAD_ROOTFS_H__
