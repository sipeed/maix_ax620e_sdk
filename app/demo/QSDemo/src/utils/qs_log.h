/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _QSCOMMON_H__
#define _QSCOMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "ax_global_type.h"
#include "qs_timer.h"

#define IS_AX620Q  (AX_SYS_GetChipType() != AX630C_CHIP && AX_SYS_GetChipType() != AX631_CHIP)

#define ALOGW(fmt, ...) printf("\033[1;30;33m[%12llu][qsdemo] " fmt "\033[0m\n", GetTickCountPts(),##__VA_ARGS__) // yellow
#define ALOGI(fmt, ...) printf("\033[1;30;33m[%12llu][qsdemo] " fmt "\033[0m\n", GetTickCountPts(),##__VA_ARGS__) // yellow
#define ALOGE(fmt, ...) printf("\033[1;30;31m[%12llu][qsdemo] " fmt "\033[0m\n", GetTickCountPts(),##__VA_ARGS__) // red

//用于关键时间点打印
#if 1
#define ALOGD(fmt, ...) printf("\033[1;30;32m[%12llu][qsdemo] " fmt "\033[0m\n", GetTickCountPts(),##__VA_ARGS__) // green
#else
#define ALOGD(fmt, ...)
#endif

AX_BOOL LogOpen();
AX_VOID LogToFile(const AX_CHAR *pFmt, ...);
AX_VOID LogClose();

#endif // _QSCOMMON_H__