/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _WAKEUP_TIMER_H__
#define _WAKEUP_TIMER_H__

#include "ax_global_type.h"

AX_S32 wakeup_timer_set(AX_U32 wait_time); // unit ms

AX_U64 GetTickCountPts(AX_VOID);
AX_S32 ReadISPTimes(AX_U64 *u64Started, AX_U64 *u64Opended, AX_U64 *u64FrameReadyed, AX_U64 *u64OutToNext, AX_U64 *u64Fsof, AX_S32 nCamCnt);
AX_S32 GpioTag(AX_U32 value, const AX_CHAR* szFlagFile);

#endif //_WAKEUP_TIMER_H__

