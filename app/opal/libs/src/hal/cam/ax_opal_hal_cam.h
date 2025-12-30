/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_CAM_H_
#define _AX_OPAL_HAL_CAM_H_

#include "ax_opal_hal_cam_def.h"

AX_S32 AX_OPAL_HAL_CAM_Init(AX_VOID);
AX_S32 AX_OPAL_HAL_CAM_Deinit(AX_VOID);

AX_S32 AX_OPAL_HAL_CAM_Open(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_MAL_CAM_ATTR_T *ptCamAttr);
AX_S32 AX_OPAL_HAL_CAM_Close(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_MAL_CAM_ATTR_T *ptCamAttr);
AX_S32 AX_OPAL_HAL_CAM_Start(const AX_OPAL_MAL_CAM_ID_T* pCamId);
AX_S32 AX_OPAL_HAL_CAM_Stop(const AX_OPAL_MAL_CAM_ID_T* pCamId);

AX_S32 AX_OPAL_HAL_CAM_GetFps(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetFps(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetFlip(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetFlip(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetMirror(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetMirror(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetMode(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetMode(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetRotation(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetRotation(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetDayNight(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetDayNight(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetEZoom(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetEZoom(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetColor(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetColor(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetLdc(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetLdc(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_GetDis(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);
AX_S32 AX_OPAL_HAL_CAM_SetDis(const AX_OPAL_MAL_CAM_ID_T* pCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData);

#endif //_AX_OPAL_HAL_CAM_H_
