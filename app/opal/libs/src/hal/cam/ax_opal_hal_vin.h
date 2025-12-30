/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_HAL_VIN_H_
#define _AX_OPAL_HAL_VIN_H_

#include "ax_opal_hal_cam_def.h"

AX_S32 AX_OPAL_HAL_VIN_CreateMipi(AX_S32 nRxDevId, AX_S32 nDevId, AX_S32 nPipeId, AX_VOID* pSensorHandle, AX_OPAL_CAM_MIPI_CFG_T* ptMipiCfg);
AX_S32 AX_OPAL_HAL_VIN_StartMipi(AX_S32 nRxDevId);
AX_S32 AX_OPAL_HAL_VIN_StopMipi(AX_S32 nRxDevId);

AX_S32 AX_OPAL_HAL_VIN_CreateDev(AX_S32 nRxDevId, AX_S32 nDevId, AX_S32 nPipeId, AX_S32 nLaneNum, AX_OPAL_CAM_DEV_CFG_T* ptDevCfg);
AX_S32 AX_OPAL_HAL_VIN_DestroyDev(AX_S32 nDevId);
AX_S32 AX_OPAL_HAL_VIN_StartDev(AX_S32 nDevId);
AX_S32 AX_OPAL_HAL_VIN_StopDev(AX_S32 nDevId);

AX_S32 AX_OPAL_HAL_VIN_CreatePipe(AX_S32 nPipeId, AX_OPAL_CAM_PIPE_CFG_T* ptPipeCfg);
AX_S32 AX_OPAL_HAL_VIN_DestroyPipe(AX_S32 nPipeId);
AX_S32 AX_OPAL_HAL_VIN_StartPipe(AX_S32 nPipeId);
AX_S32 AX_OPAL_HAL_VIN_StopPipe(AX_S32 nPipeId);

AX_S32 AX_OPAL_HAL_VIN_CreateChn(AX_S32 nPipeId, AX_S32 nChnId, AX_OPAL_CAM_CHN_CFG_T* ptChnCfg);
AX_S32 AX_OPAL_HAL_VIN_StartChn(AX_S32 nPipeId, AX_S32 nChnId);
AX_S32 AX_OPAL_HAL_VIN_StopChn(AX_S32 nPipeId, AX_S32 nChnId);

#endif //_AX_OPAL_HAL_VIN_H_