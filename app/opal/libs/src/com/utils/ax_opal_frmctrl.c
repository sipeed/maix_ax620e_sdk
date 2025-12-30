/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "ax_opal_frmctrl.h"

typedef struct axOPAL_FRMCTR_T {
    AX_U32 nSrcFrmRate;
    AX_U32 nDstFrmRate;
    AX_U64 nSeq;
}AX_OPAL_FRMCTRL_T;

AX_S32  AX_OPAL_FrmCtrlCreate(AX_FRMCTRL_HANDLE *pHandle, AX_U32 nSrcFrmRate, AX_U32 nDstFrmRate) {
    AX_OPAL_FRMCTRL_T * pInst = (AX_OPAL_FRMCTRL_T *)malloc(sizeof(AX_OPAL_FRMCTRL_T));
    if (pInst) {
        pInst->nSrcFrmRate = nSrcFrmRate;
        pInst->nDstFrmRate = nDstFrmRate;
        pInst->nSeq = 0;
        *pHandle = (AX_FRMCTRL_HANDLE)pInst;
        return AX_SUCCESS;
    } else {
        return -1;
    }
}

AX_VOID AX_OPAL_FrmCtrlDestroy(AX_FRMCTRL_HANDLE pHandle) {
    if (pHandle) {
        free(pHandle);
    }
}

AX_BOOL AX_OPAL_FrmCtrlReset(AX_FRMCTRL_HANDLE pHandle, AX_U32 nSrcFrmRate, AX_U32 nDstFrmRate) {
    AX_OPAL_FRMCTRL_T * pInst = (AX_OPAL_FRMCTRL_T *)pHandle;
    if (!pInst) {
        return AX_FALSE;
    }
    pInst->nSrcFrmRate = nSrcFrmRate;
    pInst->nDstFrmRate = nDstFrmRate;
    pInst->nSeq = 0;
    return AX_TRUE;
}

AX_BOOL AX_OPAL_FrmCtrlFilter(AX_FRMCTRL_HANDLE pHandle, AX_BOOL bTry) {
    AX_OPAL_FRMCTRL_T * pInst = (AX_OPAL_FRMCTRL_T *)pHandle;
    if (!pInst) {
        return AX_FALSE;
    }

    if (pInst->nSrcFrmRate <= pInst->nDstFrmRate) {
        return AX_FALSE;
    }

    AX_BOOL bSkip = AX_TRUE;
    do {
        if (0 == pInst->nSeq) {
            bSkip = AX_FALSE;
            break;
        }

        if ((pInst->nSeq * pInst->nDstFrmRate / pInst->nSrcFrmRate) > ((pInst->nSeq - 1) * pInst->nDstFrmRate / pInst->nSrcFrmRate)) {
            bSkip = AX_FALSE;
            break;
        }
    } while (0);

    if (!bTry) {
        pInst->nSeq++;
    }

    return bSkip;
}