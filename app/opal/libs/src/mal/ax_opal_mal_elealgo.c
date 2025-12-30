/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_elealgo.h"
#include "ax_opal_hal_algo.h"
#include "ax_opal_hal_aeroi.h"
#include "ax_opal_log.h"
#include "ax_opal_utils.h"
#include "ax_opal_mal_utils.h"
#include "ax_opal_mal_element.h"
#define LOG_TAG ("ELEALGO")

// interface vtable
AX_OPAL_MAL_ELE_Interface algo_interface = {
    .start = AX_OPAL_MAL_ELEALGO_Start,
    .stop = AX_OPAL_MAL_ELEALGO_Stop,
    .event_proc = AX_OPAL_MAL_ELEALGO_Process,
    .data_proc = AX_OPAL_MAL_ELEALGO_ProcData,
};

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELEALGO_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr) {

    AX_OPAL_MAL_ELE_HANDLE handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_ELEALGO_T));
    if (handle == AX_NULL) {
        LOG_M_E(LOG_TAG, "malloc failed.");
        return AX_NULL;
    }

    AX_OPAL_MAL_ELEALGO_T* pEle = (AX_OPAL_MAL_ELEALGO_T*)handle;
    memset(pEle, 0x0, sizeof(AX_OPAL_MAL_ELEALGO_T));
    pEle->stBase.nId = pEleAttr->nId;
    pEle->stBase.pParent = parent;
    pEle->stBase.vTable = &algo_interface;
    memcpy(&pEle->stBase.stAttr, pEleAttr, sizeof(AX_OPAL_ELEMENT_ATTR_T));
    AX_OPAL_MAL_ELE_MappingGrpChn(handle);

    AX_OPAL_MAL_SUBPPL_T *pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)parent;

    AX_S32 nSnsId = 0;
    AX_S32 nChnId = 0;
    for (AX_U32 iLink = 0; iLink < pSubPipeline->stAttr.nLinkCnt; ++iLink) {
        if (pSubPipeline->stAttr.arrLinkAttr[iLink].eLinkType == AX_OPAL_SUBPPL_IN) {
            nSnsId = pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcGrpId;
            break;
        }
    }
    for (AX_U32 iLink = 0; iLink < pSubPipeline->stAttr.nLinkCnt; ++iLink) {
        if (pSubPipeline->stAttr.arrLinkAttr[iLink].eLinkType == AX_OPAL_SUBPPL_OUT
        && pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcEleId == pEle->stBase.nId) {
            nChnId = pSubPipeline->stAttr.arrLinkAttr[iLink].nDstChnId;
            break;
        }
    }

    pEle->nSnsID = nSnsId;
    pEle->nChnID = nChnId;

    AX_OPAL_ATTR_T *pOpalAttr = &((AX_OPAL_MAL_PPL_T*)pSubPipeline->pParent)->stOpalAttr;
    memcpy(&pEle->stAttr, &(pOpalAttr->stVideoAttr[nSnsId].stPipeAttr.stVideoChnAttr[nChnId]), sizeof(AX_OPAL_VIDEO_CHN_ATTR_T));
    AX_OPAL_HAL_SetAeRoiParam(nSnsId, nSnsId, &pOpalAttr->stVideoAttr[nSnsId].stSnsAttr);
    return handle;
}

AX_S32 AX_OPAL_MAL_ELEALGO_Destroy(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }
    AX_OPAL_FREE(self);

    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEALGO_Start(AX_OPAL_MAL_ELE_HANDLE self) {
    LOG_M_D(LOG_TAG, "+++");
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        LOG_M_E(LOG_TAG, "invalid element handle");
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEALGO_T* pEle = (AX_OPAL_MAL_ELEALGO_T*)self;

    if (pEle->stBase.stAttr.nGrpCnt != 1 && pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt != 1) {
        LOG_M_E(LOG_TAG, "invalid element attr in and out");
        return -1;
    }

    nRet = AX_OPAL_HAL_ALGO_CreateChn(pEle->nSnsID, pEle->stAttr.nWidth, pEle->stAttr.nHeight);
    if (nRet != 0) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_ALGO_CreateChn failed.");
        return -1;
    }

    pEle->stBase.bStart = AX_TRUE;
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}
AX_S32 AX_OPAL_MAL_ELEALGO_Stop(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEALGO_T* pEle = (AX_OPAL_MAL_ELEALGO_T*)self;
    if (pEle->stBase.stAttr.nGrpCnt != 1 && pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt != 1) {
        return -1;
    }

    nRet = AX_OPAL_HAL_ALGO_DestroyChn(pEle->nSnsID);
    if (nRet != 0) {
        return -1;
    }
    pEle->stBase.bStart = AX_FALSE;
    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEALGO_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL || pPorcessData == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEALGO_T* pEle = (AX_OPAL_MAL_ELEALGO_T*)self;

    if (AX_OPAL_MAINCMD_VIDEO_ALGOGETPARAM == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_ALGO_GetParam(pEle->nSnsID, (AX_OPAL_ALGO_PARAM_T *)(pPorcessData->pData));
    } else if (AX_OPAL_MAINCMD_VIDEO_ALGOSETPARAM == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_ALGO_SetParam(pEle->nSnsID, (AX_OPAL_ALGO_PARAM_T *)(pPorcessData->pData));
    } else if (AX_OPAL_MAINCMD_VIDEO_REGISTERALGOCALLBACK == pPorcessData->eMainCmdType) {
        AX_OPAL_VIDEO_ALGO_CALLBACK_T* pstAlgoCallback = pPorcessData->pData;
        nRet = AX_OPAL_HAL_ALGO_RegCallback(pEle->nSnsID, pstAlgoCallback->callback, pstAlgoCallback->pUserData);
    } else if (AX_OPAL_MAINCMD_VIDEO_UNREGISTERALGOCALLBACK == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_ALGO_UnregCallback(pEle->nSnsID);
    } else if (AX_OPAL_MAINCMD_VIDEO_SETSNSATTR == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_SetAeRoiParam(pEle->nSnsID,pEle->nSnsID, (AX_OPAL_VIDEO_SNS_ATTR_T *)(pPorcessData->pData));
    }

    if (AX_OPAL_SUBCMD_SNSATTR_ROTATION == pPorcessData->eSubCmdType) {
        nRet = AX_OPAL_HAL_ALGO_UpdateRotation(pEle->nSnsID, ((AX_OPAL_VIDEO_SNS_ATTR_T*)(pPorcessData->pData))->eRotation);
    }

    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEALGO_ProcData(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nUniGrpId, AX_S32 nUniChnId, AX_OPAL_UNITLINK_TYPE_E eLinkType, AX_VOID* pData, AX_U32 nSize) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    AX_OPAL_MAL_ELE_T* pEle = (AX_OPAL_MAL_ELE_T*)self;
    if (pEle->bStart) {

        if (nSize != sizeof(AX_VIDEO_FRAME_T) || eLinkType != AX_OPAL_ELE_NONLINK_FRM) {
            LOG_M_E(LOG_TAG, "error data");
            return AX_ERR_OPAL_GENERIC;
        }
        nRet = AX_OPAL_HAL_ALGO_ProcessFrame(((AX_OPAL_MAL_ELEALGO_T*)pEle)->nSnsID, (AX_VIDEO_FRAME_T*)pData);
    }
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}