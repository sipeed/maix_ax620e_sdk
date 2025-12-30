
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_AUDIO_CAP_H_
#define _AX_OPAL_HAL_AUDIO_CAP_H_

#include "ax_opal_hal_audio_def.h"

AX_S32 AX_OPAL_HAL_AUDIO_CAP_Init(AX_VOID);
AX_S32 AX_OPAL_HAL_AUDIO_CAP_Deinit(AX_VOID);

AX_S32 AX_OPAL_HAL_AUDIO_CAP_Start(AX_OPAL_AUDIO_ATTR_T *pstAttr, AX_OPAL_AUDIO_ENCODER_ATTR_T *pstEncoderAttr);
AX_S32 AX_OPAL_HAL_AUDIO_CAP_Stop(AX_OPAL_AUDIO_ATTR_T *pstAttr, const AX_OPAL_AUDIO_ENCODER_ATTR_T *pstEncoderAttr);

AX_S32 AX_OPAL_HAL_AUDIO_CAP_SetVolume(const AX_OPAL_AUDIO_ATTR_T *pstAttr);
AX_S32 AX_OPAL_HAL_AUDIO_CAP_GetVolume(AX_OPAL_AUDIO_ATTR_T *pstAttr);

#endif // _AX_OPAL_HAL_AUDIO_CAP_H_