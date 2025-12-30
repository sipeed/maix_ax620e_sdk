/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_HAL_AEROI_H_
#define _AX_OPAL_HAL_AEROI_H_
#include "ax_global_type.h"
#include "ax_sys_api.h"
#include "ax_skel_api.h"
#include "ax_opal_type.h"

// algorithm ae roi item
typedef struct axOPAL_ALGO_AE_ROI_ITEM_T {
    AX_U64 u64TrackId;
    AX_F32 fConfidence;
    AX_OPAL_ALGO_HVCFP_TYPE_E eType;
    AX_OPAL_ALGO_BOX_T stBox;
} AX_OPAL_ALGO_AE_ROI_ITEM_T;


#ifdef __cplusplus
extern "C" {
#endif

AX_S32 AX_OPAL_HAL_SetAeRoiParam(AX_U8 nSnsId, AX_U8 nPipeId, AX_OPAL_VIDEO_SNS_ATTR_T* pstAttr);
AX_S32 AX_OPAL_HAL_SetAeRoiAttr(AX_U32 nSnsId, const AX_OPAL_ALGO_AE_ROI_CONFIG_T* pstAeRoiConfig, AX_BOOL bDetectEnable);
AX_S32 AX_OPAL_HAL_UpdateAeRoi(AX_U32 nSnsId, const AX_OPAL_ALGO_HVCFP_RESULT_T *hvcfp, const AX_OPAL_ALGO_AE_ROI_CONFIG_T* pstAeRoiConfig);

#ifdef __cplusplus
}
#endif

#endif // _AX_OPAL_HAL_AEROI_H_
