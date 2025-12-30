/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_VENC_H_
#define _AX_OPAL_HAL_VENC_H_

#include "ax_opal_hal_venc_def.h"

AX_S32 AX_OPAL_HAL_VENC_Init(AX_VOID);
AX_S32 AX_OPAL_HAL_VENC_Deinit(AX_VOID);

AX_S32 AX_OPAL_HAL_VENC_CreateChn(AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation, AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr);
AX_S32 AX_OPAL_HAL_VENC_DestroyChn(AX_S32 nChnId);
AX_S32 AX_OPAL_HAL_VENC_StartRecv(AX_S32 nChnId);
AX_S32 AX_OPAL_HAL_VENC_StopRecv(AX_S32 nChnId);

AX_S32 AX_OPAL_HAL_VENC_SetChnAttr(AX_S32 nChnId, AX_OPAL_VIDEO_CHN_ATTR_T *pChnAttr);
// AX_S32 AX_OPAL_HAL_VENC_GetChnAttr(AX_S32 nChnId, AX_OPAL_VIDEO_CHAN_ATTR_T *pChnAttr);
AX_S32 AX_OPAL_HAL_VENC_SetJpegQf(AX_S32 nChnId, AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr);
AX_S32 AX_OPAL_HAL_VENC_RequestIDR(AX_S32 nChnId);
AX_S32 AX_OPAL_HAL_VENC_SetRcAttr(AX_S32 nChnId, AX_OPAL_VIDEO_ENCODER_ATTR_T *pstEncAttr);
// AX_S32 AX_OPAL_HAL_VENC_GetRcAttr(AX_S32 nChnId, AX_OPAL_VIDEO_ENCODER_ATTR_T *pstEncAttr);
AX_S32 AX_OPAL_HAL_VENC_SetRotation(AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation);
AX_S32 AX_OPAL_HAL_VENC_SetResolution(AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation, AX_S32 nWidth, AX_S32 nHeight);
AX_S32 AX_OPAL_HAL_VENC_SetFps(AX_S32 nChnId, AX_F32 fSrcFps, AX_F32 fDstFps);

AX_S32 AX_OPAL_HAL_VENC_SetSvcParam(AX_S32 nChnId, const AX_OPAL_VIDEO_SVC_PARAM_T* pstParam);
AX_S32 AX_OPAL_HAL_VENC_GetSvcParam(AX_S32 nChnId, AX_OPAL_VIDEO_SVC_PARAM_T* pstParam);
AX_S32 AX_OPAL_HAL_VENC_SetSvcRegion(AX_S32 nChnId, const AX_OPAL_VIDEO_SVC_REGION_T* pstRegion);

AX_OPAL_VIDEO_NALU_TYPE_E AX_OPAL_HAL_VENC_CvtNaluType(AX_PAYLOAD_TYPE_E eType, AX_VENC_DATA_TYPE_U nVencNaluType, AX_BOOL bIFrame);


#endif // _AX_OPAL_HAL_VENC_H_

