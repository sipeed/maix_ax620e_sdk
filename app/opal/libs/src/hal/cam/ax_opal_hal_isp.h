
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_HAL_ISP_H_
#define _AX_OPAL_HAL_ISP_H_

#include "ax_opal_hal_cam_def.h"

AX_S32 AX_OPAL_HAL_ISP_Create(AX_S32 nPipeId, AX_VOID* pSensorHandle, AX_OPAL_CAM_ISP_CFG_T* ptIspCfg, AX_OPAL_SNS_MODE_E eSnsMode);
AX_S32 AX_OPAL_HAL_ISP_Destroy(AX_S32 nPipeId);

AX_S32 AX_OPAL_HAL_ISP_Start(AX_S32 nPipeId);
AX_S32 AX_OPAL_HAL_ISP_Stop(AX_S32 nPipeId);

#endif //_AX_OPAL_HAL_ISP_H_