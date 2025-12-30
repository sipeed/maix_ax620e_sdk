
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_SNS_H_
#define _AX_OPAL_HAL_SNS_H_

#include "ax_opal_hal_cam_def.h"

AX_S32 AX_OPAL_HAL_SNS_Init(const AX_CHAR* pDriverFileName, const AX_CHAR* pSensorObjName, AX_OPAL_MAL_SNSLIB_ATTR_T* ptSnsLibAttr);
AX_S32 AX_OPAL_HAL_SNS_Deinit(AX_OPAL_MAL_SNSLIB_ATTR_T* pySnsLibAttr);
AX_S32 AX_OPAL_HAL_SNS_Create(AX_S32 nPipeId, AX_VOID* pSensorHandle, AX_OPAL_CAM_SENSOR_CFG_T* ptSnsCfg);
AX_S32 AX_OPAL_HAL_SNS_Destroy(AX_S32 nPipeId, AX_OPAL_CAM_SENSOR_CFG_T* ptSnsCfg);

#endif // _AX_OPAL_HAL_SNS_H_