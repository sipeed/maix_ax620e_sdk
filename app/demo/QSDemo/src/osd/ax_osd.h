/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AXOSD_H_
#define _AXOSD_H_

#include <stddef.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include "ax_base_type.h"
#include "ax_osd_type.h"
#include "ax_skel_type.h"
#include "osd.h"
#include "qsdemo.h"

#ifdef __cplusplus
extern "C" {
#endif

AX_S32 AX_OSD_Init(SAMPLE_ENTRY_PARAM_T *pEntryParam);
AX_S32 AX_OSD_DeInit(SAMPLE_ENTRY_PARAM_T *pEntryParam);
AX_S32 AX_OSD_UpdateImmediately(AX_VOID);
AX_S32 AX_OSD_DrawRect(AX_SKEL_RESULT_T *pstResult, AX_S32 nIvpsChn, AX_U8 nRotation);
AX_S32 AX_OSD_SetAovStatus(AX_BOOL bAov);

#ifdef __cplusplus
}
#endif

#endif
