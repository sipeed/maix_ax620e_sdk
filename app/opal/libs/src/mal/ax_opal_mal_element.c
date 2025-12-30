/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_element.h"

#include "ax_opal_log.h"
#include "ax_opal_utils.h"

#define LOG_TAG ("MAL_ELE")

// interface vtable
AX_OPAL_MAL_ELE_Interface ele_interface = {
    .event_proc = AX_OPAL_MAL_ELE_Process,
};

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELE_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr) {

    AX_OPAL_MAL_ELE_HANDLE handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_ELE_T));
    if (handle == AX_NULL) {
        LOG_M_E(LOG_TAG, "malloc failed.");
        return AX_NULL;
    }

    AX_OPAL_MAL_ELE_T* ele = (AX_OPAL_MAL_ELE_T*)handle;
    memset(ele, 0x0, sizeof(AX_OPAL_MAL_ELE_T));
    ele->nId = pEleAttr->nId;
    ele->pParent = parent;
    ele->vTable = &ele_interface;

    return handle;
}

AX_S32 AX_OPAL_MAL_ELE_Destroy(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }
    AX_OPAL_FREE(self);

    return nRet;
}

AX_S32 AX_OPAL_MAL_ELE_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    return nRet;
}

AX_VOID AX_OPAL_MAL_ELE_MappingGrpChn(AX_OPAL_MAL_ELE_HANDLE self) {

    AX_OPAL_MAL_ELE_T* pEle = (AX_OPAL_MAL_ELE_T*)self;
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pEle->pParent;

    for (AX_S32 iChn = 0; iChn < pEle->stAttr.arrGrpAttr[0].nChnCnt; ++iChn) {

        AX_S32 iLink = 0;
        AX_S32 nEleIdTmp = pEle->stAttr.nId;
        AX_OPAL_UNIT_TYPE_E eUnitType =pEle->stAttr.eType;
        AX_S32 nGrpIdTmp = pEle->stAttr.arrGrpAttr[0].nGrpId;
        AX_S32 nChnIdTmp = pEle->stAttr.arrGrpAttr[0].nChnId[iChn];
        LOG_M_I(LOG_TAG, "type=%d ele=%d, grp=%d, chn=%d, linkcnt=%d", (AX_U32)eUnitType, nEleIdTmp, nGrpIdTmp, nChnIdTmp, pSubPipeline->stAttr.nLinkCnt);

        LOG_M_I(LOG_TAG, "find input grp: eSrcType = AX_OPAL_SUBPPL; eLinkType = AX_OPAL_SUBPPL_IN.");
        if (eUnitType == AX_OPAL_ELE_IVPS) {
            nChnIdTmp = 0;
        }
        iLink = 0;
        while (iLink < pSubPipeline->stAttr.nLinkCnt) {
            if (nEleIdTmp == pSubPipeline->stAttr.arrLinkAttr[iLink].nDstEleId
                && nGrpIdTmp == pSubPipeline->stAttr.arrLinkAttr[iLink].nDstGrpId
                && nChnIdTmp == pSubPipeline->stAttr.arrLinkAttr[iLink].nDstChnId) {

                if (pSubPipeline->stAttr.arrLinkAttr[iLink].eLinkType == AX_OPAL_SUBPPL_IN) {
                    pEle->stAttr.arrGrpAttr[0].nUniGrpId = pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcGrpId;
                    LOG_M_I(LOG_TAG, "    get unit grp = %d", pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcGrpId);
                    break;
                }
                else {

                    nEleIdTmp = pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcEleId;
                    nGrpIdTmp = pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcGrpId;
                    nChnIdTmp = pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcChnId;
                    eUnitType = pSubPipeline->stAttr.arrLinkAttr[iLink].eSrcType;
                    LOG_M_I(LOG_TAG, "    continue ele=%d, grp=%d, chn=%d, type=%d", nEleIdTmp, nGrpIdTmp, nChnIdTmp, eUnitType);
                    if (eUnitType == AX_OPAL_ELE_IVPS) {
                        nChnIdTmp = 0;
                    }
                    iLink = 0;
                    continue;
                }
            }
            iLink = iLink + 1;
        }

        LOG_M_I(LOG_TAG, "find output chn: eDstType = AX_OPAL_SUBPPL; eDstType = AX_OPAL_SUBPPL_OUT");
        iLink = 0;
        nEleIdTmp = pEle->stAttr.nId;
        eUnitType = pEle->stAttr.eType;
        nGrpIdTmp = pEle->stAttr.arrGrpAttr[0].nGrpId;
        nChnIdTmp = pEle->stAttr.arrGrpAttr[0].nChnId[iChn];
        while (iLink < pSubPipeline->stAttr.nLinkCnt) {

            if (nEleIdTmp == pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcEleId
                    && nGrpIdTmp == pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcGrpId
                    && nChnIdTmp == pSubPipeline->stAttr.arrLinkAttr[iLink].nSrcChnId
                    && pSubPipeline->stAttr.arrLinkAttr[iLink].eSrcType != AX_OPAL_SUBPPL) {

                if (pSubPipeline->stAttr.arrLinkAttr[iLink].eLinkType == AX_OPAL_SUBPPL_OUT) {
                    pEle->stAttr.arrGrpAttr[0].nUniChnId[iChn] = pSubPipeline->stAttr.arrLinkAttr[iLink].nDstChnId;
                    LOG_M_I(LOG_TAG, "    get unit chn = %d", pSubPipeline->stAttr.arrLinkAttr[iLink].nDstChnId);
                    break;
                }
                else {
                    eUnitType = pSubPipeline->stAttr.arrLinkAttr[iLink].eDstType;
                    nEleIdTmp = pSubPipeline->stAttr.arrLinkAttr[iLink].nDstEleId;
                    nGrpIdTmp = pSubPipeline->stAttr.arrLinkAttr[iLink].nDstGrpId;
                    nChnIdTmp = pSubPipeline->stAttr.arrLinkAttr[iLink].nDstChnId;
                    LOG_M_I(LOG_TAG, "    continue ele=%d, grp=%d, chn=%d, type=%d", nEleIdTmp, nGrpIdTmp, nChnIdTmp, eUnitType);
                    iLink = 0;
                    continue;
                }

            }
            iLink = iLink + 1;
        }
    }
}

AX_S32 AX_OPAL_MAL_ELE_GetUniGrpId(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_OPAL_MAL_ELE_T* pEle = (AX_OPAL_MAL_ELE_T*)self;
    return pEle->stAttr.arrGrpAttr[0].nUniGrpId;
}

AX_S32 AX_OPAL_MAL_ELE_GetUniChnId(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nGrpId, AX_S32 nChnId) {
    AX_OPAL_MAL_ELE_T* pEle = (AX_OPAL_MAL_ELE_T*)self;
    for (AX_S32 iChn = 0; iChn < pEle->stAttr.arrGrpAttr[0].nChnCnt; ++iChn) {
        if (nGrpId == pEle->stAttr.arrGrpAttr[0].nGrpId
            && nChnId == pEle->stAttr.arrGrpAttr[0].nChnId[iChn]) {
            return pEle->stAttr.arrGrpAttr[0].nUniChnId[iChn];
        }
    }
    return -1;
}

AX_S32 AX_OPAL_MAL_ELE_GetGrpId(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_OPAL_MAL_ELE_T* pEle = (AX_OPAL_MAL_ELE_T*)self;
    return pEle->stAttr.arrGrpAttr[0].nGrpId;
}

AX_S32 AX_OPAL_MAL_ELE_GetChnId(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nUniGrpId, AX_S32 nUniChnId) {
    AX_OPAL_MAL_ELE_T* pEle = (AX_OPAL_MAL_ELE_T*)self;
    for (AX_S32 iChn = 0; iChn < pEle->stAttr.arrGrpAttr[0].nChnCnt; ++iChn) {
        if (nUniGrpId == pEle->stAttr.arrGrpAttr[0].nUniGrpId
            && nUniChnId == pEle->stAttr.arrGrpAttr[0].nUniChnId[iChn]) {
            return pEle->stAttr.arrGrpAttr[0].nChnId[iChn];
        }
    }
    return -1;
}

AX_BOOL AX_OPAL_MAL_ELE_CheckUniGrpChnId(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nUniGrpId, AX_S32 nUniChnId) {
    /* nUniGrpId and nUniChnId from EVENT procdata */
    AX_OPAL_MAL_ELE_T* pEle = (AX_OPAL_MAL_ELE_T*)self;
    if (nUniGrpId != pEle->stAttr.arrGrpAttr[0].nUniGrpId) {
        return AX_FALSE;
    }
    if (-1 != nUniChnId) {
        AX_BOOL bExist = AX_FALSE;
        for (AX_S32 iChn = 0; iChn < pEle->stAttr.arrGrpAttr[0].nChnCnt; ++iChn) {
            if (nUniChnId == pEle->stAttr.arrGrpAttr[0].nUniChnId[iChn]) {
                bExist = AX_TRUE;
                break;
            }
        }
        if (!bExist) {
            return AX_FALSE;
        }
    }
    return AX_TRUE;
}
