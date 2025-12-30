/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_IVPS_H_
#define _AX_OPAL_HAL_IVPS_H_

#include "ax_opal_hal_ivps_def.h"

AX_S32 AX_OPAL_HAL_IVPS_Init(AX_VOID);
AX_S32 AX_OPAL_HAL_IVPS_Deinit(AX_VOID);

AX_S32 AX_OPAL_HAL_IVPS_Open(AX_S32 nGrpId, AX_OPAL_IVPS_ATTR_T *ptIvpsAttr);
AX_S32 AX_OPAL_HAL_IVPS_Close(AX_S32 nGrpId);
AX_S32 AX_OPAL_HAL_IVPS_Start(AX_S32 nGrpId, AX_OPAL_IVPS_ATTR_T *ptIvpsAttr);
AX_S32 AX_OPAL_HAL_IVPS_Stop(AX_S32 nGrpId);

AX_S32 AX_OPAL_HAL_IVPS_EnableChn(AX_S32 nGrpId, AX_S32 nChnId);
AX_S32 AX_OPAL_HAL_IVPS_DisableChn(AX_S32 nGrpId, AX_S32 nChnId);

AX_S32 AX_OPAL_HAL_IVPS_SetRotation(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotationType);

AX_S32 AX_OPAL_HAL_IVPS_SetResolution(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation, AX_S32 nWidth, AX_S32 nHeight);
AX_S32 AX_OPAL_HAL_IVPS_GetResolution(AX_S32 nGrpId, AX_S32 nChnId, AX_S32* nWidth, AX_S32* nHeight);
AX_S32 AX_OPAL_HAL_IVPS_SetFps(AX_S32 nGrpId, AX_S32 nChnId, AX_F32 fSrcFps, AX_F32 fDstFps);
AX_S32 AX_OPAL_HAL_IVPS_SnapShot(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_VIDEO_SNAPSHOT_T* ptSnapShot);
AX_S32 AX_OPAL_HAL_IVPS_CaptureFrame(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_VIDEO_CAPTUREFRAME_T* ptCaptureFrame);

IVPS_RGN_HANDLE AXOP_HAL_IVPS_CreateOsd(AX_S32 nGrpId, AX_S32 nChnId, const AX_OPAL_OSD_ATTR_T* pOsdAttr);
AX_S32 AXOP_HAL_IVPS_DestroyOsd(AX_S32 nGrpId, AX_S32 nChnId, IVPS_RGN_HANDLE hdlOsd, const AX_OPAL_OSD_ATTR_T* pOsdAttr);

#endif // _AX_OPAL_HAL_IVPS_H_


