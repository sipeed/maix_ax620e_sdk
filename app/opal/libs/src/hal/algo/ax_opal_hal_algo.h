/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_HAL_ALGO_H_
#define _AX_OPAL_HAL_ALOG_H_

#include "ax_opal_type.h"

AX_S32 AX_OPAL_HAL_ALGO_Init(AX_OPAL_ALGO_ATTR_T *pAttr);
AX_S32 AX_OPAL_HAL_ALGO_Deinit(AX_VOID);
AX_S32 AX_OPAL_HAL_ALGO_CreateChn(AX_U32 nChnId, AX_U32 nWdith, AX_U32 nHeight);
AX_S32 AX_OPAL_HAL_ALGO_DestroyChn(AX_U32 nChnId);
AX_S32 AX_OPAL_HAL_ALGO_SetParam(AX_U32 nChnId, const AX_OPAL_ALGO_PARAM_T *pParam);
AX_S32 AX_OPAL_HAL_ALGO_GetParam(AX_U32 nChnId, AX_OPAL_ALGO_PARAM_T *pParam);
AX_S32 AX_OPAL_HAL_ALGO_RegCallback(AX_U32 nChnId, const AX_OPAL_VIDEO_ALGO_CALLBACK callback, AX_VOID *pUserData);
AX_S32 AX_OPAL_HAL_ALGO_UnregCallback(AX_U32 nChnId);
AX_S32 AX_OPAL_HAL_ALGO_UpdateRotation(AX_U32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation);
AX_S32 AX_OPAL_HAL_ALGO_ProcessFrame(AX_U32 nChnId, AX_VIDEO_FRAME_T *pFrame);

#endif // _AX_OPAL_HAL_ALOG_H_


