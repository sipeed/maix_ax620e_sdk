/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_SYS_H_
#define _AX_OPAL_HAL_SYS_H_

#include "ax_opal_hal_sys_def.h"

#include "ax_base_type.h"
#include "ax_global_type.h"

#include "ax_sys_api.h"
#include "ax_engine_api.h"
#include "ax_pool_type.h"

#include "ax_opal_mal_def.h"

#define AX_YUV_FBC_STRIDE_ALIGN_VAL (128)
#define AX_YUV_NONE_FBC_STRIDE_ALIGN_VAL (16)

#ifndef AX_MAX
#define AX_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

AX_S32 AX_OPAL_HAL_SYS_Init(AX_VOID);
AX_S32 AX_OPAL_HAL_SYS_Deinit(AX_VOID);
AX_S32 AX_OPAL_HAL_POOL_Init(AX_OPAL_HAL_POOL_ATTR_T *ptPoolAttr);
AX_S32 AX_OPAL_HAL_POOL_Deinit(AX_VOID);

AX_S32 AX_OPAL_HAL_MOD_Link(AX_OPAL_LINK_ATTR_T *pLinkAttr);
AX_S32 AX_OPAL_HAL_MOD_UnLink(AX_OPAL_LINK_ATTR_T *pLinkAttr);

AX_S32 AX_OPAL_HAL_SetVinIvpsMode(AX_S32 nVinPipe, AX_S32 IvpsGrp, AX_S32 eMode);

AX_POOL AX_OPAL_HAL_SYS_CreatePool(AX_U32 nFrameSize, AX_U32 nDepth, const AX_CHAR* PoolName);
AX_VOID AX_OPAL_HAL_SYS_DestroyPool(AX_POOL nPoolId);

#endif // _AX_OPAL_HAL_SYS_H_

