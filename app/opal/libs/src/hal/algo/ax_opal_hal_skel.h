/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_HAL_SKEL_H_
#define _AX_OPAL_HAL_SKEL_H_
#include "ax_global_type.h"
#include "ax_sys_api.h"
#include "ax_skel_api.h"
#include "ax_opal_type.h"

#ifdef __cplusplus
extern "C" {
#endif

AX_VOID AX_OPAL_HAL_GetSkelResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, AX_OPAL_ALGO_HVCFP_ITEM_T* pstItem);
AX_S32  AX_OPAL_HAL_SetSkelParam(AX_U8 nSnsId, AX_SKEL_HANDLE pSkel, AX_OPAL_ALGO_HVCFP_PARAM_T *pCurParam, const AX_OPAL_ALGO_HVCFP_PARAM_T *pNewParam);
AX_S32  AX_OPAL_HAL_InitSkelParam(AX_U8 nSnsId, AX_SKEL_HANDLE pSkel, AX_OPAL_ALGO_HVCFP_PARAM_T *pParam);

#ifdef __cplusplus
}
#endif

#endif // _AX_OPAL_HAL_SKEL_H_
