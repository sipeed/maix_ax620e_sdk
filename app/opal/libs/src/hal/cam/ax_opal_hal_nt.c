/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_nt.h"

#include "ax_opal_log.h"

#define LOG_TAG ("HAL_NT")

AX_S32 AX_OPAL_HAL_NT_Init(AX_U32 nStreamPort, AX_U32 nCtrlPort)
{
    AX_S32 s32Ret = 0;

    /* Net Preview */
    LOG_M_E(LOG_TAG, "Start the service on the tuning device side.");

    s32Ret =  AX_NT_StreamInit(nStreamPort);
    if (0 != s32Ret) {
        LOG_M_E(LOG_TAG, "AX_NT_StreamInit failed! Error Code:0x%X", s32Ret);
        return -1;
    }
    s32Ret =  AX_NT_CtrlInit(nCtrlPort);
    if (0 != s32Ret) {
        LOG_M_E(LOG_TAG, "AX_NT_CtrlInit failed, ret=0x%x.", s32Ret);
        goto EXIT_FAIL;
    }

    LOG_M_E(LOG_TAG, "NT Stream Listen Port %d.", nStreamPort);
    LOG_M_E(LOG_TAG, "NT Ctrl Listen Port %d.", nCtrlPort);

    return 0;

EXIT_FAIL:
    AX_NT_StreamDeInit();
    return s32Ret;
}

AX_S32 AX_OPAL_HAL_NT_UpdateSource(AX_U32 nPipeId)
{
    return AX_NT_SetStreamSource(nPipeId);
}

AX_VOID AX_OPAL_HAL_NT_Deinit(AX_VOID)
{
    AX_NT_CtrlDeInit();
    AX_NT_StreamDeInit();
}
