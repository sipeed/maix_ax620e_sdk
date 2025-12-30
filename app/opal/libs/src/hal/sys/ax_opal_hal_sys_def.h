/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_SYS_DEF_H_
#define _AX_OPAL_HAL_SYS_DEF_H_

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_opal_type.h"

#define AX_OPAL_HAL_MAX_POOL_CNT (5)

typedef struct {
    AX_U32 nWidth;
    AX_U32 nHeight;
    // AX_U32 nWidthStride;
    AX_IMG_FORMAT_E nFmt;
    AX_U32 nBlkCnt;
    AX_COMPRESS_MODE_E enCompressMode;
    AX_U32 u32CompressLevel;
} AX_OPAL_HAL_POOL_CFG_T;

typedef struct {
    AX_U32 nCommPoolCfgCnt;
    AX_OPAL_HAL_POOL_CFG_T arrCommPoolCfg[AX_OPAL_HAL_MAX_POOL_CNT];
    AX_U32 nPrivPoolCfgCnt;
    AX_OPAL_HAL_POOL_CFG_T arrPrivPoolCfg[AX_OPAL_HAL_MAX_POOL_CNT];
} AX_OPAL_POOL_ATTR_T;

typedef struct {
    AX_OPAL_POOL_ATTR_T stPoolAttr[AX_OPAL_SNS_ID_BUTT];
} AX_OPAL_HAL_POOL_ATTR_T;

#endif // _AX_OPAL_HAL_SYS_DEF_H_