/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_SPS_H_
#define _AX_OPAL_HAL_SPS_H_

#include "ax_opal_hal_cam_def.h"

AX_S32 AX_OPAL_HAL_SPS_Init();
AX_S32 AX_OPAL_HAL_SPS_Deinit();
AX_S32 AX_OPAL_HAL_SPS_SetSnsInfo(AX_S32 nSnsId, AX_S32 nPipeId, AX_S32 nChnId);
AX_S32 AX_OPAL_HAL_SPS_Start(AX_S32 nSnsId);
AX_S32 AX_OPAL_HAL_SPS_Stop(AX_S32 nSnsId);
AX_S32 AX_OPAL_HAL_SPS_SetAttr(AX_S32 nSnsId, const AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T* pstAttr);
AX_S32 AX_OPAL_HAL_SPS_GetAttr(AX_S32 nSnsId, AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T* pstAttr);
AX_S32 AX_OPAL_HAL_SPS_RegisterCallback(AX_S32 nSnsId,
                                        const AX_OPAL_VIDEO_SNS_SOFTPHOTOSENSITIVITY_CALLBACK callback,
                                        AX_VOID *pUserData);
AX_S32 AX_OPAL_HAL_SPS_UnRegisterCallback(AX_S32 nSnsId);


#endif //_AX_OPAL_HAL_SPS_H_

