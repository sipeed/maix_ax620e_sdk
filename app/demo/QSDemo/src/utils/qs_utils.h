/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _QS_UTILS_H__
#define _QS_UTILS_H__

#include "ax_global_type.h"
#include <math.h>

#define IS_FPS_EQUAL(float_a, float_b)    ((fabs(float_a - float_b) < (0.01)) ? (1) : (0))

typedef enum _E_FLAG_FILE {
    E_FLAG_FILE_TIMES = 0,
    E_FLAG_FILE_REBOOT,
    E_FLAG_FILE_AOV
}E_FLAG_FILE;

const AX_CHAR* GetFlagFilePath(E_FLAG_FILE eFlag);
const AX_CHAR* GetFlagFile(E_FLAG_FILE eFlag);

AX_VOID MSSleep(AX_U32 nSleep);
AX_S32  QS_RunCmd(AX_CHAR* cmd, AX_CHAR *result, AX_S32 len);
AX_S32  QS_GetSc850sl2MFlag(AX_S32 nDefault);
AX_S32  QS_GetSingleSnsPipeId(AX_S32 nDefault);
AX_S32  QS_SetSnsSwitch(AX_S32 nSnsId);
AX_S32  QS_GetSnsSwitch(AX_S32 nDefault);
AX_BOOL QS_CheckMounted(const char* mntpt, AX_S32 nTimes);

#endif //_QS_UTILS_H__