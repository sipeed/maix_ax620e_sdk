/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AUDIO_CONFIG__
#define _AUDIO_CONFIG__

#include "common_audio.h"

#ifdef __cplusplus
extern "C"
{
#endif

const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T *qs_audio_capture_config(AX_VOID);
const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T *qs_audio_play_config(AX_VOID);

#ifdef __cplusplus
}
#endif

#endif // _AUDIO_CONFIG__
