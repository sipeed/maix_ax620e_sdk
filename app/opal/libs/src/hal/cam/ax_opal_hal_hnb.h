/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_HAL_HNB_H_
#define _AX_OPAL_HAL_HNB_H_

// hnb = hot noise balance

#include "ax_opal_hal_cam_def.h"

AX_S32 AX_OPAL_HAL_HNB_Init();
AX_S32 AX_OPAL_HAL_HNB_Deinit();
AX_S32 AX_OPAL_HAL_HNB_SetSnsInfo(AX_S32 nSnsId, AX_S32 nPipeId, AX_S32 nChnId, AX_SENSOR_REGISTER_FUNC_T* ptSnsHdl);
AX_S32 AX_OPAL_HAL_HNB_Start(AX_S32 nSnsId);
AX_S32 AX_OPAL_HAL_HNB_Stop(AX_S32 nSnsId);
AX_S32 AX_OPAL_HAL_HNB_SetAttr(AX_S32 nSnsId, const AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T* pstAttr);
AX_S32 AX_OPAL_HAL_HNB_GetAttr(AX_S32 nSnsId, AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T* pstAttr);


#endif // _AX_OPAL_HAL_HNB_H_
