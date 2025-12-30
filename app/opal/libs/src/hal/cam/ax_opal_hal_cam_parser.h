/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_CAM_PARSER_H_
#define _AX_OPAL_HAL_CAM_PARSER_H_

#include "ax_opal_hal_cam_def.h"

AX_S32 AX_OPAL_HAL_CAM_Parse(const AX_CHAR* pFileName, AX_OPAL_CAM_CFG_T* ptCamCfg, AX_OPAL_MAL_CAM_ID_T* ptCamId);

#endif // _AX_OPAL_HAL_CAM_PARSER_H_
