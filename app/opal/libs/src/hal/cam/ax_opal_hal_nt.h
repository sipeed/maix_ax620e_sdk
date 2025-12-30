/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_HAL_NT_H_
#define _AX_OPAL_HAL_NT_H_

#include "ax_opal_hal_cam_def.h"

AX_S32 AX_OPAL_HAL_NT_Init(AX_U32 nStreamPort, AX_U32 nCtrlPort);
AX_S32 AX_OPAL_HAL_NT_UpdateSource(AX_U32 nPipeId);
AX_VOID AX_OPAL_HAL_NT_Deinit(AX_VOID);

#endif // _AX_OPAL_HAL_NT_H_
