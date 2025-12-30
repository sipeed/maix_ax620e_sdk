/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <string.h>
#include <pthread.h>
#include "ax_opal_hal_aeroi.h"
#include "ax_isp_3a_api.h"
#include "ax_opal_utils.h"
#include "ax_opal_log.h"

#define LOG_TAG  "AEROI"

#define  MAX_OPAL_AE_ROI_ITEM_COUNT  AX_AE_DETECT_OBJECT_MAX_NUM

typedef struct axOPAL_ALGO_AEROI_INFO_T {
    AX_U8  bValid;
    AX_U8  nSnsId;
    AX_U8  nPipeId;
    AX_U8  bAeRoiManual;
    pthread_mutex_t mtx;
    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr;
} OPAL_ALGO_AEROI_INFO_T;

static OPAL_ALGO_AEROI_INFO_T g_stAlgoAeRoiInfo[AX_OPAL_SNS_ID_BUTT] = {
    [0] = {
        .bValid = 0,
        .nSnsId = 0,
        .nPipeId = 0,
        .bAeRoiManual = 0,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
    },
    [1] = {
        .bValid = 0,
        .nSnsId = 1,
        .nPipeId = 1,
        .bAeRoiManual = 0,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
    }
};


static AX_U32 AeFloat2Int(AX_F32 value, AX_U32 int_bit, AX_U32 frac_bit, AX_BOOL signed_value) {
    AX_U32 result = 0;
    if ((int_bit + frac_bit + signed_value) > 32) {
        return -1;
    }

    result = AX_ABS(value) * (AX_U64)(1 << frac_bit);

    if (signed_value) {
        if (value < 0) {
            result = -result;
        }
    }

    return result;
}

static AxAeDetectObjectCategory GetAeDetectObjectCategory(const AX_OPAL_ALGO_HVCFP_TYPE_E eType) {
    switch (eType) {
    case AX_OPAL_ALGO_HVCFP_BODY:
        return AXAE_BODY_ROI;
    case AX_OPAL_ALGO_HVCFP_VEHICLE:
        return AXAE_VEHICLE_ROI;
    case AX_OPAL_ALGO_HVCFP_CYCLE:
        return AXAE_CYCLE_ROI;
    case AX_OPAL_ALGO_HVCFP_FACE:
        return AXAE_FACE_ROI;
    case AX_OPAL_ALGO_HVCFP_PLATE:
        return AXAE_PLATE_ROI;
    default:
        break;
    }
    return AXAE_FACE_ROI;
}

static AX_VOID AeRoiBoxConvert(AX_U8 nSnsId, AX_OPAL_ALGO_AE_ROI_ITEM_T* pstVecItem, AX_U32 nCount) {
    if (!pstVecItem) {
        return;
    }

    AX_F32 fEZoomRatio = g_stAlgoAeRoiInfo[nSnsId].stSnsAttr.stEZoomAttr.fRatio;
    AX_BOOL bMirror = g_stAlgoAeRoiInfo[nSnsId].stSnsAttr.bMirror;
    AX_BOOL bFlip = g_stAlgoAeRoiInfo[nSnsId].stSnsAttr.bFlip;
    AX_OPAL_SNS_ROTATION_E eRotation = g_stAlgoAeRoiInfo[nSnsId].stSnsAttr.eRotation;
    AX_U32 nImageWidth = g_stAlgoAeRoiInfo[nSnsId].stSnsAttr.nWidth;
    AX_U32 nImageHeight = g_stAlgoAeRoiInfo[nSnsId].stSnsAttr.nHeight;

    AX_F32 fCropWidth = 0;
    AX_F32 fCropHeight = 0;
    AX_F32 fCropStartX = 0;
    AX_F32 fCropStartY = 0;

    if (fEZoomRatio >= 1.0) {
        fCropWidth = 1.0 / fEZoomRatio;
        fCropHeight = 1.0 / fEZoomRatio;
        fCropStartX = (1 - fCropWidth) / 2;
        fCropStartY = (1 - fCropHeight) / 2;
    }

    if (bMirror || bFlip || eRotation != AX_OPAL_SNS_ROTATION_0 || fEZoomRatio > 0) {
        for (AX_U32 i = 0; i < nCount; i++) {
            AX_OPAL_ALGO_AE_ROI_ITEM_T *pstItem = &pstVecItem[i];
            if (fEZoomRatio >= 1.0) {
                pstItem->stBox.fX = pstItem->stBox.fX / fEZoomRatio;
                pstItem->stBox.fY = pstItem->stBox.fY / fEZoomRatio;
                pstItem->stBox.fW = pstItem->stBox.fW / fEZoomRatio;
                pstItem->stBox.fH = pstItem->stBox.fH / fEZoomRatio;

                pstItem->stBox.fX += fCropStartX;
                pstItem->stBox.fY += fCropStartY;
            }

            AX_OPAL_ALGO_BOX_T stBox = pstItem->stBox;

            stBox.nImgWidth = nImageWidth;
            stBox.nImgHeight = nImageHeight;

            if (bMirror) {
                if (stBox.fX + stBox.fW > 1) {
                    stBox.fX = 0;
                } else {
                    stBox.fX = 1 - (stBox.fX + stBox.fW);
                }

                pstItem->stBox.fX = stBox.fX;

                if (eRotation == AX_OPAL_SNS_ROTATION_90) {
                    eRotation = AX_OPAL_SNS_ROTATION_270;
                } else if (eRotation == AX_OPAL_SNS_ROTATION_270) {
                    eRotation = AX_OPAL_SNS_ROTATION_90;
                }
            }

            if (bFlip) {
                if (stBox.fY + stBox.fH > 1) {
                    stBox.fY = 0;
                } else {
                    stBox.fY = 1 - (stBox.fY + stBox.fH);
                }

                pstItem->stBox.fY = stBox.fY;

                if (eRotation == AX_OPAL_SNS_ROTATION_90) {
                    eRotation = AX_OPAL_SNS_ROTATION_270;
                } else if (eRotation == AX_OPAL_SNS_ROTATION_270) {
                    eRotation = AX_OPAL_SNS_ROTATION_90;
                }
            }

            switch (eRotation) {
                case AX_OPAL_SNS_ROTATION_0: {
                    break;
                }
                case AX_OPAL_SNS_ROTATION_90: {
                    AX_SWAP(stBox.nImgWidth, stBox.nImgHeight);
                    stBox.fX = 1 - pstItem->stBox.fY - pstItem->stBox.fH;
                    stBox.fY = pstItem->stBox.fX;
                    stBox.fW = pstItem->stBox.fW;
                    stBox.fH = pstItem->stBox.fH;
                    break;
                }
                case AX_OPAL_SNS_ROTATION_180: {
                    stBox.fX = 1 - pstItem->stBox.fX - pstItem->stBox.fW;
                    stBox.fY = 1 - pstItem->stBox.fY - pstItem->stBox.fH;
                    break;
                }
                case AX_OPAL_SNS_ROTATION_270: {
                    AX_SWAP(stBox.nImgWidth, stBox.nImgHeight);
                    stBox.fX = pstItem->stBox.fY;
                    stBox.fY = 1 - pstItem->stBox.fX - pstItem->stBox.fW;
                    stBox.fW = pstItem->stBox.fH;
                    stBox.fH = pstItem->stBox.fW;
                    AX_SWAP(stBox.fW, stBox.fH);
                    break;
                }
                default:
                    break;
            }

            pstItem->stBox = stBox;
        }
    }
}



static AX_VOID GetAeRoi(AX_U32 nSnsId, AX_U32 nItemSize,  const AX_OPAL_ALGO_HVCFP_ITEM_T* stItems,
                                const AX_OPAL_ALGO_AE_ROI_CONFIG_T* pstAeRoiConfig,
                                AX_OPAL_ALGO_AE_ROI_ITEM_T *pstVecItem, AX_U32 *pItemSize, AX_U32 nMaxItem) {
    AX_U32 nImageWidth = g_stAlgoAeRoiInfo[nSnsId].stSnsAttr.nWidth;
    AX_U32 nImageHeight = g_stAlgoAeRoiInfo[nSnsId].stSnsAttr.nHeight;
    AX_U32 nAiRoiItem = 0;

    if (nItemSize > 0) {
 #if 1
        /* all */
        for (AX_U32 i = 0 ; i < nItemSize; i++) {
            if (stItems[i].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_NEW
                || stItems[i].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_UPDATE) {
                AX_U32 nWidth = (AX_U32)(stItems[i].stBox.fW * nImageWidth);
                AX_U32 nHeight = (AX_U32)(stItems[i].stBox.fH * nImageHeight);

                if (nWidth >= pstAeRoiConfig->nWidth
                    && nHeight >= pstAeRoiConfig->nHeight
                    && nAiRoiItem < nMaxItem) {
                    pstVecItem[nAiRoiItem].u64TrackId = stItems[i].u64TrackId;
                    pstVecItem[nAiRoiItem].fConfidence = stItems[i].fConfidence;
                    pstVecItem[nAiRoiItem].eType = stItems[i].eType;
                    pstVecItem[nAiRoiItem].stBox = stItems[i].stBox;
                    nAiRoiItem++;
                }
            }
        }
#else
        AX_BOOL bFound = AX_FALSE;
        AX_OPAL_ALGO_AE_ROI_ITEM_T stRoiItem = {0};
        for (AX_U32 i = 0 ; i < nItemSize; i++) {
            if (stItems[i].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_NEW
                || stItems[i].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_UPDATE) {
                AX_U32 nWidth = (AX_U32)(stItems[i].stBox.fW * nImageWidth);
                AX_U32 nHeight = (AX_U32)(stItems[i].stBox.fH * nImageHeight);

                if (nWidth >= pstAeRoiConfig->nWidth
                    && nHeight >= pstAeRoiConfig->nHeight
                    && nWidth > (AX_U32)(stRoiItem.tBox.fW * nImageWidth)
                    && nHeight > (AX_U32)(stRoiItem.tBox.fH * nImageHeight)) {
                    stRoiItem.u64TrackId = stItems[i].u64TrackId;
                    stRoiItem.fConfidence = stItems[i].fConfidence;
                    stRoiItem.eType = stItems[i].eType;
                    stRoiItem.tBox = stItems[i].stBox;
                    bFound = AX_TRUE;
                }
            }
        }

        if (bFound && nMaxItem > 0) {
            stVecItem[0] = stRoiItem;
            nAiRoiItem = 1;
        }
#endif
    }
    *pItemSize = nAiRoiItem;
}

AX_S32 UpdateAeRoiAttr(AX_U8 nPipeId, AX_BOOL bFaceAeRoiEnable, AX_BOOL bVehicleAeRoiEnable) {
    AX_ISP_IQ_AE_PARAM_T stAeParam;
    memset(&stAeParam, 0, sizeof(stAeParam));
    AX_S32 nRet = AX_ISP_IQ_GetAeParam(nPipeId, &stAeParam);
    if (0 != nRet) {
        return nRet;
    }
    stAeParam.tAeAlgAuto.tFaceUIParam.nEnable = bFaceAeRoiEnable;
    stAeParam.tAeAlgAuto.tVehicleUIParam.nEnable = bVehicleAeRoiEnable;
    nRet = AX_ISP_IQ_SetAeParam(nPipeId, &stAeParam);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_SetAeParam failed, ret=0x%x.", nRet);
        return nRet;
    }

    return AX_SUCCESS;
}

AX_VOID UpdateAeRoi(AX_U32 nSnsId, AX_OPAL_ALGO_AE_ROI_ITEM_T* pstVecItem, AX_U32 nCount) {
    pthread_mutex_lock(&g_stAlgoAeRoiInfo[nSnsId].mtx);
    AeRoiBoxConvert(nSnsId, pstVecItem, nCount);
    pthread_mutex_unlock(&g_stAlgoAeRoiInfo[nSnsId].mtx);

    AX_BOOL bUpdate = AX_FALSE;
    AX_ISP_AE_DETECT_OBJECT_PARAM_T stAeParam;

    if (nCount > 0) {
        bUpdate = AX_TRUE;
        memset(&stAeParam, 0, sizeof(stAeParam));

        for (AX_U32 i = 0; i < nCount && stAeParam.nObjectNum < AX_AE_DETECT_OBJECT_MAX_NUM; i ++) {
            stAeParam.nObjectID[stAeParam.nObjectNum] = pstVecItem[i].u64TrackId;
            stAeParam.nObjectCategory[stAeParam.nObjectNum] = (AX_U32)GetAeDetectObjectCategory(pstVecItem[i].eType);
            stAeParam.nObjectConfidence[stAeParam.nObjectNum] = AeFloat2Int(pstVecItem[i].fConfidence, 1, 10, AX_FALSE);
            stAeParam.tObjectInfos[stAeParam.nObjectNum].nObjectStartX = AeFloat2Int(pstVecItem[i].stBox.fX, 1, 10, AX_FALSE);
            stAeParam.tObjectInfos[stAeParam.nObjectNum].nObjectStartY = AeFloat2Int(pstVecItem[i].stBox.fY, 1, 10, AX_FALSE);
            stAeParam.tObjectInfos[stAeParam.nObjectNum].nObjectWidth = AeFloat2Int(pstVecItem[i].stBox.fW, 1, 10, AX_FALSE);
            stAeParam.tObjectInfos[stAeParam.nObjectNum].nObjectHeight = AeFloat2Int(pstVecItem[i].stBox.fH, 1, 10, AX_FALSE);
            stAeParam.nObjectNum ++;
        }
        g_stAlgoAeRoiInfo[nSnsId].bAeRoiManual = AX_TRUE;
    } else {
        if (g_stAlgoAeRoiInfo[nSnsId].bAeRoiManual) {
            bUpdate = AX_TRUE;
            g_stAlgoAeRoiInfo[nSnsId].bAeRoiManual = AX_FALSE;
            memset(&stAeParam, 0, sizeof(stAeParam));
        }
    }

    if (bUpdate) {
        AX_S32 nRet = AX_ISP_IQ_SetAeDetectObjectROI(g_stAlgoAeRoiInfo[nSnsId].nPipeId, &stAeParam);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_ISP_IQ_SetAeDetectObjectROI failed, ret=0x%08X.", nRet);
        }
    }
}

AX_S32  AX_OPAL_HAL_SetAeRoiParam(AX_U8 nSnsId, AX_U8 nPipeId, AX_OPAL_VIDEO_SNS_ATTR_T* pstAttr) {
    if (nSnsId < AX_OPAL_SNS_ID_BUTT && pstAttr) {
        pthread_mutex_lock(&g_stAlgoAeRoiInfo[nSnsId].mtx);
        g_stAlgoAeRoiInfo[nSnsId].nPipeId = nPipeId;
        g_stAlgoAeRoiInfo[nSnsId].stSnsAttr = *pstAttr;
        g_stAlgoAeRoiInfo[nSnsId].bValid = AX_TRUE;
        pthread_mutex_unlock(&g_stAlgoAeRoiInfo[nSnsId].mtx);
        return 0;
    } else {
        return -1;
    }
}

AX_S32 AX_OPAL_HAL_SetAeRoiAttr(AX_U32 nSnsId, const AX_OPAL_ALGO_AE_ROI_CONFIG_T* pstAeRoiConfig, AX_BOOL bDetectEnable) {

    AX_BOOL bFaceAeRoiEnable = (AX_BOOL)(pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].bEnable
                                        || pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_FACE].bEnable);

    AX_BOOL bVehicleAeRoiEnable = (AX_BOOL)(pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].bEnable
                                        || pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_CYCLE].bEnable
                                        || pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_PLATE].bEnable);

    AX_BOOL bAeRoiEnable = (bFaceAeRoiEnable || bVehicleAeRoiEnable) && bDetectEnable;

    if (!bAeRoiEnable) {
        UpdateAeRoi(nSnsId, NULL, 0);
    }
    UpdateAeRoiAttr(g_stAlgoAeRoiInfo[nSnsId].nPipeId, bFaceAeRoiEnable, bVehicleAeRoiEnable);
    return AX_SUCCESS;
}

AX_S32 AX_OPAL_HAL_UpdateAeRoi(AX_U32 nSnsId, const AX_OPAL_ALGO_HVCFP_RESULT_T *hvcfp, const AX_OPAL_ALGO_AE_ROI_CONFIG_T* pstAeRoiConfig) {
    AX_BOOL bAeRoiEnable = AX_FALSE;
    AX_OPAL_ALGO_AE_ROI_ITEM_T stVecItem[MAX_OPAL_AE_ROI_ITEM_COUNT];
    AX_U32 nLeft = MAX_OPAL_AE_ROI_ITEM_COUNT;
    AX_U32 nCount = 0;
    AX_U32 nCurSize = 0;

    if (pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].bEnable) {
        bAeRoiEnable = AX_TRUE;
        nCurSize = 0;
        GetAeRoi(nSnsId, hvcfp->nBodySize, (const AX_OPAL_ALGO_HVCFP_ITEM_T*)hvcfp->pstBodys, &pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY], &stVecItem[nCount], &nCurSize, nLeft);
        nCount += nCurSize;
        nLeft -= nCurSize;
    }

    if (pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].bEnable) {
        bAeRoiEnable = AX_TRUE;
        GetAeRoi(nSnsId, hvcfp->nVehicleSize, (const AX_OPAL_ALGO_HVCFP_ITEM_T*)hvcfp->pstVehicles, &pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE], &stVecItem[nCount], &nCurSize, nLeft);
        nCount += nCurSize;
        nLeft -= nCurSize;
    }

    if (pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_CYCLE].bEnable) {
        bAeRoiEnable = AX_TRUE;
        GetAeRoi(nSnsId, hvcfp->nCycleSize, (const AX_OPAL_ALGO_HVCFP_ITEM_T*)hvcfp->pstCycles, &pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_CYCLE], &stVecItem[nCount], &nCurSize, nLeft);
        nCount += nCurSize;
        nLeft -= nCurSize;
    }

    if (pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_FACE].bEnable) {
        bAeRoiEnable = AX_TRUE;
        GetAeRoi(nSnsId, hvcfp->nFaceSize, (const AX_OPAL_ALGO_HVCFP_ITEM_T*)hvcfp->pstFaces, &pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_FACE], &stVecItem[nCount], &nCurSize, nLeft);
        nCount += nCurSize;
        nLeft -= nCurSize;
    }

    if (pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_PLATE].bEnable) {
        bAeRoiEnable = AX_TRUE;
        GetAeRoi(nSnsId, hvcfp->nPlateSize, (const AX_OPAL_ALGO_HVCFP_ITEM_T*)hvcfp->pstPlates, &pstAeRoiConfig[AX_OPAL_ALGO_HVCFP_PLATE], &stVecItem[nCount], &nCurSize, nLeft);
        nCount += nCurSize;
        nLeft -= nCurSize;
    }

    if (bAeRoiEnable && g_stAlgoAeRoiInfo[nSnsId].bValid) {
        UpdateAeRoi(nSnsId, &stVecItem[0], nCount);
    }
    return 0;
}
