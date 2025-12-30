
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_utils.h"
#include "ax_opal_mal_element.h"

AX_OPAL_VIDEO_SNS_ATTR_T *AX_OPAL_MAL_GetSnsAttr(AX_OPAL_MAL_ELE_HANDLE hdlEle) {
    AX_OPAL_MAL_ELE_T *pEle = (AX_OPAL_MAL_ELE_T*)hdlEle;
    AX_OPAL_MAL_SUBPPL_T* pSubPpl = (AX_OPAL_MAL_SUBPPL_T*)pEle->pParent;
    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)pSubPpl->pParent;
    AX_S32 nUniGrpId = AX_OPAL_MAL_ELE_GetUniGrpId(hdlEle);
    return &pPipeline->stOpalAttr.stVideoAttr[nUniGrpId].stSnsAttr;
}

AX_OPAL_VIDEO_CHN_ATTR_T *AX_OPAL_MAL_GetChnAttr(AX_OPAL_MAL_ELE_HANDLE hdlEle, AX_S32 nEleGrpId, AX_S32 nEleChnId) {
    AX_OPAL_MAL_ELE_T *pEle = (AX_OPAL_MAL_ELE_T*)hdlEle;
    AX_OPAL_MAL_SUBPPL_T* pSubPpl = (AX_OPAL_MAL_SUBPPL_T*)pEle->pParent;
    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)pSubPpl->pParent;
    AX_S32 nUniGrpId = AX_OPAL_MAL_ELE_GetUniGrpId(hdlEle);
    AX_S32 nUniChnId = AX_OPAL_MAL_ELE_GetUniChnId(hdlEle, nEleGrpId, nEleChnId);
    return &pPipeline->stOpalAttr.stVideoAttr[nUniGrpId].stPipeAttr.stVideoChnAttr[nUniChnId];
}