/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_opal_hal_algo.h"
#include "ax_skel_api.h"
#include "ax_skel_type.h"
#include "ax_opal_log.h"
#include "ax_ives_api.h"
#include "ax_opal_hal_skel.h"
#include "ax_opal_hal_aeroi.h"
#include "ax_opal_frmctrl.h"
#include "ax_isp_3a_api.h"
#include <pthread.h>

#define LOG_TAG  ("HAL_ALGO")

#define IVES_DEFAULT_MBSIZE_W       (32)
#define IVES_DEFAULT_MBSIZE_H       (32)
#define IVES_DEFAULT_FRAMERATE      (1)
#define SKEL_MAX_USER_DATA_NUM      (64)
#define SKEL_MAX_RESULT_COUNT       (32)

#define SKEL_DISABLE_DELAY_FRM      (2)

typedef struct axOPAL_SKEL_FRAME_USER_DATA{
    AX_U64 u64Pts;
    AX_U64 nSeqNum;
    AX_U32 nChnId;
    AX_S8 nRefCnt;
} AX_OPAL_HAL_SKEL_FRAME_USER_DATA_T;

typedef struct axOPAL_ALGO_INFO {
    AX_BOOL bMdCreated;
    AX_BOOL bOdCreated;
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_U64 nFrameID;
    AX_OPAL_SNS_ROTATION_E eRotation;
    AX_OPAL_ALGO_PARAM_T * pParam;
    AX_OPAL_VIDEO_ALGO_CALLBACK callback;
    AX_VOID *pUserData;
    AX_U8 nMdResult[AX_OPAL_MAX_ALGO_MD_REGION_COUNT];
    AX_U8 nOdResult;
    AX_FRMCTRL_HANDLE pFrmCtrl;
}AX_OPAL_HAL_ALGO_INFO_T;

static AX_OPAL_ALGO_PARAM_T g_stAlgoParam[AX_OPAL_SNS_ID_BUTT] = {
    [0] = {
        .stHvcfpParam = {
            .bEnable = AX_TRUE,
            .bPushActive = AX_FALSE,
            .nCropEncoderQpLevel = 80,
            .nAlgoType = AX_OPAL_ALGO_HVCP,
            .nSrcFramerate = -1,
            .nDstFramerate = 10,
            .stRoiConfig = {
                .bEnable = AX_FALSE,
                .stPolygon = {
                    .nPointNum = 0,
                    .stPoints = {
                        [0] = {0, 0},
                    }
                }
            },
            .stTrackSize = {
                .nTrackHumanSize = 0,
                .nTrackVehicleSize = 0,
                .nTrackCycleSize = 0,
            },
            .stPushStrategy = {
                .ePushMode = AX_OPAL_ALGO_PUSH_MODE_BEST,
                .nInterval =  2000,
                .nPushCount = 3,
            },
            .stPanoramaConfig = {
                .bEnable = AX_FALSE,
            },
            .stObjectFliterConfig = {
                [0] = {0, 0, 0},
                [1] = {0, 0, 0},
                [2] = {0, 0, 0},
                [3] = {0, 0, 0},
                [4] = {0, 0, 0},
            },
            .stPushFliterConfig = {
                [0] = {
                    .stCommonPushFilterConfig = {
                        .fQuality = 0,
                    },
                },
                [1] = {
                    .stCommonPushFilterConfig = {
                        .fQuality = 0,
                    },
                },
                [2] = {
                    .stCommonPushFilterConfig = {
                        .fQuality = 0,
                    },
                },
                [3] = {
                    .stFacePushFilterConfig = {
                        .nWidth = 0,
                        .nHeight = 0,
                        .stFacePoseBlur = {
                            .fPitch = 180,
                            .fYaw = 180,
                            .fRoll = 180,
                            .fBlur = 1.0,
                        },
                    },
                },
                [4] = {
                    .stCommonPushFilterConfig = {
                        .fQuality = 0,
                    },
                },
            },
            .stAeRoiConfig = {
                [0] = { AX_FALSE, 0, 0},
                [1] = { AX_FALSE, 0, 0},
                [2] = { AX_FALSE, 0, 0},
                [3] = { AX_FALSE, 0, 0},
                [4] = { AX_FALSE, 0, 0},
            },
        },
        .stIvesParam = {
            .nSrcFramerate = 10,
            .nDstFramerate = IVES_DEFAULT_FRAMERATE,
            .stMdParam = {
                .bEnable = AX_TRUE,
                .bCapture = AX_TRUE,
                .nRegionSize = 1,
                .stRegions = {
                    [0] = {
                        .fConfidence = 0.8,
                        .fThreshold = 0.5,
                        .stRect = {0, 0, 0, 0},
                    },
                }
            },
            .stOdParam = {
                .bEnable = AX_FALSE,
                .fConfidence = 0.8,
                .fThreshold = 0.5,
            },
        },
    },
    [1] = {
        .stHvcfpParam = {
            .bEnable = AX_TRUE,
            .bPushActive = AX_FALSE,
            .nCropEncoderQpLevel = 80,
            .nAlgoType = AX_OPAL_ALGO_HVCP,
            .nSrcFramerate = -1,
            .nDstFramerate = 10,
            .stRoiConfig = {
                .bEnable = AX_FALSE,
                .stPolygon = {
                    .nPointNum = 0,
                    .stPoints = {
                        [0] = {0, 0},
                    }
                }
            },
            .stTrackSize = {
                .nTrackHumanSize = 0,
                .nTrackVehicleSize = 0,
                .nTrackCycleSize = 0,
            },
            .stPushStrategy = {
                .ePushMode = AX_OPAL_ALGO_PUSH_MODE_BEST,
                .nInterval =  2000,
                .nPushCount = 3,
            },
            .stPanoramaConfig = {
                .bEnable = AX_FALSE,
            },
            .stObjectFliterConfig = {
                [0] = {0, 0, 0},
                [1] = {0, 0, 0},
                [2] = {0, 0, 0},
                [3] = {0, 0, 0},
                [4] = {0, 0, 0},
            },
            .stPushFliterConfig = {
                [0] = {
                    .stCommonPushFilterConfig = {
                        .fQuality = 0,
                    },
                },
                [1] = {
                    .stCommonPushFilterConfig = {
                        .fQuality = 0,
                    },
                },
                [2] = {
                    .stCommonPushFilterConfig = {
                        .fQuality = 0,
                    },
                },
                [3] = {
                    .stFacePushFilterConfig = {
                        .nWidth = 0,
                        .nHeight = 0,
                        .stFacePoseBlur = {
                            .fPitch = 180,
                            .fYaw = 180,
                            .fRoll = 180,
                            .fBlur = 1.0,
                        },
                    },
                },
                [4] = {
                    .stCommonPushFilterConfig = {
                        .fQuality = 0,
                    },
                },
            },
            .stAeRoiConfig = {
                [0] = { AX_FALSE, 0, 0},
                [1] = { AX_FALSE, 0, 0},
                [2] = { AX_FALSE, 0, 0},
                [3] = { AX_FALSE, 0, 0},
                [4] = { AX_FALSE, 0, 0},
            },
        },
        .stIvesParam = {
            .nSrcFramerate = 10,
            .nDstFramerate = IVES_DEFAULT_FRAMERATE,
            .stMdParam = {
                .bEnable = AX_TRUE,
                .bCapture = AX_TRUE,
                .nRegionSize = 1,
                .stRegions = {
                    [0] = {
                        .fConfidence = 0.8,
                        .fThreshold = 0.5,
                        .stRect = {0, 0, 0, 0},
                    },
                }
            },
            .stOdParam = {
                .bEnable = AX_FALSE,
                .fConfidence = 0.8,
                .fThreshold = 0.5,
            },
        },
    },
};

static AX_SKEL_HANDLE g_pSkelHandle = NULL;
static AX_S32  g_nSkelHandleRef = 0;
static AX_BOOL g_bSkelInited = AX_FALSE;
static AX_BOOL g_bIvesInited = AX_FALSE;
static AX_BOOL g_bMdInited = AX_FALSE;
static AX_BOOL g_bOdInited = AX_FALSE;
static AX_OPAL_HAL_SKEL_FRAME_USER_DATA_T g_stSkelFrmPrvData[SKEL_MAX_USER_DATA_NUM] = {0};
static pthread_mutex_t g_mtxSkelFrmUsrData = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_mtxAlgo = PTHREAD_MUTEX_INITIALIZER;
static AX_OPAL_ALGO_ATTR_T g_stAlgoAttr = {0};
static AX_S32 g_nSkelDisableDelayFrm = 0;

static AX_OPAL_HAL_ALGO_INFO_T g_stAlgoInfo[AX_OPAL_SNS_ID_BUTT] = {
    [0] = {
        .bMdCreated = AX_FALSE,
        .bOdCreated = AX_FALSE,
        .nWidth = 0,
        .nHeight = 0,
        .nFrameID = 0,
        .eRotation = AX_OPAL_SNS_ROTATION_0,
        .pParam = &g_stAlgoParam[0],
        .callback = NULL,
        .pUserData = NULL,
    },
    [1] = {
        .bMdCreated = AX_FALSE,
        .bOdCreated = AX_FALSE,
        .nWidth = 0,
        .nHeight = 0,
        .nFrameID = 0,
        .eRotation = AX_OPAL_SNS_ROTATION_0,
        .pParam = &g_stAlgoParam[1],
        .callback = NULL,
        .pUserData = NULL,
    },
};

static AX_U32 align_down(AX_U32 x, AX_U32 a) {
    if (a > 0) {
        return ((x / a) * a);
    } else {
        return x;
    }
}

static AX_S32 MD_Clean(AX_U32 nChnId) {
    AX_S32 nRet = 0;
    if(g_stAlgoInfo[nChnId].bMdCreated) {
        for (AX_S32 i = 0; i < g_stAlgoParam[nChnId].stIvesParam.stMdParam.nRegionSize; i++) {
            nRet = AX_IVES_MD_DestoryChn(nChnId * AX_OPAL_MAX_ALGO_MD_REGION_COUNT + i);
            if(nRet != 0){
                LOG_M_E(LOG_TAG, "AX_IVES_MD_DestoryChn [%d] failed, ret=0x%x", i, nRet);
            }
        }

        g_stAlgoInfo[nChnId].bMdCreated = AX_FALSE;
    }
    return 0;
}

static AX_S32 MD_Setup(AX_U32 nChnId) {
    AX_S32 nRet = 0;
    for (AX_S32 i = 0; i < g_stAlgoParam[nChnId].stIvesParam.stMdParam.nRegionSize; i++) {
        AX_MD_CHN_ATTR_T stMdChnAttr = {0};
        stMdChnAttr.enAlgMode = AX_MD_MODE_REF;
        stMdChnAttr.mdChn = nChnId * AX_OPAL_MAX_ALGO_MD_REGION_COUNT + i;
        if (g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect.fW == 0 ||
            g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect.fH == 0) {
            g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect.fX = 0;
            g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect.fY = 0;
            g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect.fW = g_stAlgoInfo[nChnId].nWidth;
            g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect.fH = g_stAlgoInfo[nChnId].nHeight;
        } else {
            AX_OPAL_RECT_T stRectTmp = g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect;
            if (((AX_S32)stRectTmp.fX) < 0 || ((AX_U32)stRectTmp.fX) >= g_stAlgoInfo[nChnId].nWidth) {
                stRectTmp.fX = 0;
            }
            if (((AX_S32)stRectTmp.fY) < 0 || ((AX_U32)stRectTmp.fY) >= g_stAlgoInfo[nChnId].nHeight) {
                stRectTmp.fY = 0;
            }
            if (((AX_U32)(stRectTmp.fX + stRectTmp.fW)) > g_stAlgoInfo[nChnId].nWidth) {
                stRectTmp.fW = g_stAlgoInfo[nChnId].nWidth - stRectTmp.fX;
            }
            if (((AX_U32)(stRectTmp.fY + stRectTmp.fH)) > g_stAlgoInfo[nChnId].nHeight) {
                stRectTmp.fH = g_stAlgoInfo[nChnId].nHeight - stRectTmp.fY;
            }
            g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect = stRectTmp;
        }
        AX_OPAL_RECT_T stRect = g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].stRect;

        if (g_stAlgoInfo[nChnId].eRotation == AX_OPAL_SNS_ROTATION_0) {
            stMdChnAttr.stArea.u32X = (AX_U32)stRect.fX;
            stMdChnAttr.stArea.u32Y = (AX_U32)stRect.fY;
            stMdChnAttr.stArea.u32W = (AX_U32)stRect.fW;
            stMdChnAttr.stArea.u32H = (AX_U32)stRect.fH;
            stMdChnAttr.stMbSize.u32W = IVES_DEFAULT_MBSIZE_W;
            stMdChnAttr.stMbSize.u32H = IVES_DEFAULT_MBSIZE_H;
        } else if (g_stAlgoInfo[nChnId].eRotation == AX_OPAL_SNS_ROTATION_90) {
            stMdChnAttr.stArea.u32X = g_stAlgoInfo[nChnId].nHeight - (AX_U32)stRect.fH - (AX_U32)stRect.fY;
            stMdChnAttr.stArea.u32Y = (AX_U32)stRect.fX;
            stMdChnAttr.stArea.u32W = (AX_U32)stRect.fH;
            stMdChnAttr.stArea.u32H = (AX_U32)stRect.fW;
            stMdChnAttr.stMbSize.u32W = IVES_DEFAULT_MBSIZE_H;
            stMdChnAttr.stMbSize.u32H = IVES_DEFAULT_MBSIZE_W;
        } else if (g_stAlgoInfo[nChnId].eRotation == AX_OPAL_SNS_ROTATION_180) {
            stMdChnAttr.stArea.u32X = g_stAlgoInfo[nChnId].nWidth - (AX_U32)stRect.fW - (AX_U32)stRect.fX;
            stMdChnAttr.stArea.u32Y = g_stAlgoInfo[nChnId].nHeight - (AX_U32)stRect.fH - (AX_U32)stRect.fY;
            stMdChnAttr.stArea.u32W = (AX_U32)stRect.fW;
            stMdChnAttr.stArea.u32H = (AX_U32)stRect.fH;
            stMdChnAttr.stMbSize.u32W = IVES_DEFAULT_MBSIZE_W;
            stMdChnAttr.stMbSize.u32H = IVES_DEFAULT_MBSIZE_H;
        } else if (g_stAlgoInfo[nChnId].eRotation == AX_OPAL_SNS_ROTATION_270) {
            stMdChnAttr.stArea.u32X = (AX_U32)stRect.fY;
            stMdChnAttr.stArea.u32Y = g_stAlgoInfo[nChnId].nWidth - (AX_U32)stRect.fW - (AX_U32)stRect.fX;
            stMdChnAttr.stArea.u32W = (AX_U32)stRect.fH;
            stMdChnAttr.stArea.u32H = (AX_U32)stRect.fW;
            stMdChnAttr.stMbSize.u32W = IVES_DEFAULT_MBSIZE_H;
            stMdChnAttr.stMbSize.u32H = IVES_DEFAULT_MBSIZE_W;
        }

        stMdChnAttr.u8ThrY = (AX_U8)(g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i].fThreshold * 100);

        stMdChnAttr.stArea.u32X = align_down(stMdChnAttr.stArea.u32X, 2);
        stMdChnAttr.stArea.u32Y = align_down(stMdChnAttr.stArea.u32Y, 2);
        stMdChnAttr.stArea.u32W = align_down(stMdChnAttr.stArea.u32W, stMdChnAttr.stMbSize.u32W);
        stMdChnAttr.stArea.u32H = align_down(stMdChnAttr.stArea.u32H, stMdChnAttr.stMbSize.u32H);

        LOG_M_I(LOG_TAG, "MD[%d]: rect=(%d,%d,%d,%d), thrY=%d", stMdChnAttr.mdChn,
                stMdChnAttr.stArea.u32X, stMdChnAttr.stArea.u32Y, stMdChnAttr.stArea.u32W, stMdChnAttr.stArea.u32H, stMdChnAttr.u8ThrY);

        nRet = AX_IVES_MD_CreateChn(stMdChnAttr.mdChn, &stMdChnAttr);
        if(nRet != 0){
            LOG_M_E(LOG_TAG, "AX_IVES_MD_CreateChn failed, ret=0x%x", nRet);
            return -1;
        }
    }

    g_stAlgoInfo[nChnId].bMdCreated = AX_TRUE;
    return 0;
}

static AX_S32 OD_Clean(AX_U32 nChnId) {
    AX_S32 nRet = 0;
    if (g_bOdInited && g_stAlgoInfo[nChnId].bOdCreated) {
        nRet = AX_IVES_OD_DestoryChn(nChnId);
         if(nRet != 0){
            LOG_M_E(LOG_TAG, "AX_IVES_OD_DestoryChn [%d] failed, ret=0x%x", nChnId, nRet);
            return -1;
        }
    }
    return 0;
}

static AX_S32 OD_Setup(AX_U32 nChnId) {
    AX_S32 nRet = 0;
    AX_OD_CHN_ATTR_T stOdChnAttr = {0};
    stOdChnAttr.odChn = nChnId;
    stOdChnAttr.u32FrameRate = g_stAlgoParam[nChnId].stIvesParam.nSrcFramerate;
    stOdChnAttr.u8ThrdY = (AX_U8)(g_stAlgoParam[nChnId].stIvesParam.stOdParam.fThreshold * 100);
    stOdChnAttr.u8ConfidenceY = (AX_U8)(g_stAlgoParam[nChnId].stIvesParam.stOdParam.fConfidence * 100);
    stOdChnAttr.stArea.u32X = 0;
    stOdChnAttr.stArea.u32Y = 0;
    if (g_stAlgoInfo[nChnId].eRotation == AX_OPAL_SNS_ROTATION_0 || g_stAlgoInfo[nChnId].eRotation == AX_OPAL_SNS_ROTATION_180) {
        stOdChnAttr.stArea.u32W = g_stAlgoInfo[nChnId].nWidth;
        stOdChnAttr.stArea.u32H = g_stAlgoInfo[nChnId].nHeight;
    } else {
        stOdChnAttr.stArea.u32W = g_stAlgoInfo[nChnId].nHeight;
        stOdChnAttr.stArea.u32H = g_stAlgoInfo[nChnId].nWidth;
    }

    stOdChnAttr.stArea.u32X = align_down(stOdChnAttr.stArea.u32X, 2);
    stOdChnAttr.stArea.u32Y = align_down(stOdChnAttr.stArea.u32Y, 2);
    stOdChnAttr.stArea.u32W = align_down(stOdChnAttr.stArea.u32W, 2);
    stOdChnAttr.stArea.u32H = align_down(stOdChnAttr.stArea.u32H, 2);

    stOdChnAttr.u32LuxDiff = 60;
    stOdChnAttr.u32LuxThrd = 0;

    nRet = AX_IVES_OD_CreateChn(stOdChnAttr.odChn, &stOdChnAttr);
    if(nRet != 0){
        LOG_M_E(LOG_TAG, "AX_IVES_MD_CreateChn failed, ret=0x%x", nRet);
        return -1;
    }

    g_stAlgoInfo[nChnId].bOdCreated = AX_TRUE;
    return 0;
}

static AX_OPAL_HAL_SKEL_FRAME_USER_DATA_T* GetSkelUserData(AX_VOID) {
    pthread_mutex_lock(&g_mtxSkelFrmUsrData);
    for (AX_U32 i = 0; i < SKEL_MAX_USER_DATA_NUM; i++) {
        if (g_stSkelFrmPrvData[i].nRefCnt == 0) {
            g_stSkelFrmPrvData[i].nRefCnt = 1;
            pthread_mutex_unlock(&g_mtxSkelFrmUsrData);
            return &g_stSkelFrmPrvData[i];
        }
    }
    pthread_mutex_unlock(&g_mtxSkelFrmUsrData);
    return NULL;
}

static AX_VOID ReleaseSkelUserData(AX_OPAL_HAL_SKEL_FRAME_USER_DATA_T* pData) {
    if (pData) {
        pthread_mutex_lock(&g_mtxSkelFrmUsrData);
        pData->nRefCnt = 0;
        pthread_mutex_unlock(&g_mtxSkelFrmUsrData);
    }
}

static AX_VOID SkelResult2DetectResult(AX_OPAL_ALGO_HVCFP_RESULT_T* hvcfp, const AX_SKEL_RESULT_T *pstResult) {
    hvcfp->nWidth = pstResult->nOriginalWidth;
    hvcfp->nHeight = pstResult->nOriginalHeight;

    for (AX_U32 i = 0; i < pstResult->nObjectSize; ++i) {
        AX_SKEL_OBJECT_ITEM_T* pstObjectItem = &pstResult->pstObjectItems[i];

        AX_U32 nCountLimit = SKEL_MAX_RESULT_COUNT;
        AX_U32 *pCount = NULL;
        AX_OPAL_ALGO_HVCFP_ITEM_T* pstItem = NULL;
        AX_OPAL_ALGO_HVCFP_TYPE_E eType;
        if (strcmp(pstObjectItem->pstrObjectCategory, "body") == 0) {
            pCount = &hvcfp->nBodySize;
            pstItem = hvcfp->pstBodys;
            eType = AX_OPAL_ALGO_HVCFP_BODY;
        } else if (strcmp(pstObjectItem->pstrObjectCategory, "vehicle") == 0) {
            pCount = &hvcfp->nVehicleSize;
            pstItem = hvcfp->pstVehicles;
            eType = AX_OPAL_ALGO_HVCFP_VEHICLE;
        } else if (strcmp(pstObjectItem->pstrObjectCategory, "cycle") == 0) {
            pCount = &hvcfp->nCycleSize;
            pstItem = hvcfp->pstCycles;
            eType = AX_OPAL_ALGO_HVCFP_CYCLE;
        } else if (strcmp(pstObjectItem->pstrObjectCategory, "face") == 0) {
            pCount = &hvcfp->nFaceSize;
            pstItem = hvcfp->pstFaces;
            eType = AX_OPAL_ALGO_HVCFP_FACE;
        } else if (strcmp(pstObjectItem->pstrObjectCategory, "plate") == 0) {
            pCount = &hvcfp->nPlateSize;
            pstItem = hvcfp->pstPlates;
            eType = AX_OPAL_ALGO_HVCFP_PLATE;
        } else {
            continue;
        }

        AX_U32 nCount = *(AX_U32 *)pCount;
        if (nCount >= nCountLimit) {
            continue;
        }

        pstItem = pstItem + nCount;

        pstItem->eType = eType;
        pstItem->u64FrameId = hvcfp->u64FrameId;
        pstItem->u64TrackId = pstObjectItem->nTrackId;
        pstItem->fConfidence = pstObjectItem->fConfidence;

        // track status
        if (pstObjectItem->eTrackState == AX_SKEL_TRACK_STATUS_NEW) {
            pstItem->eTrackStatus = AX_OPAL_ALGO_TRACK_STATUS_NEW;
        } else if (pstObjectItem->eTrackState == AX_SKEL_TRACK_STATUS_UPDATE) {
            pstItem->eTrackStatus = AX_OPAL_ALGO_TRACK_STATUS_UPDATE;
        } else if (pstObjectItem->eTrackState == AX_SKEL_TRACK_STATUS_DIE) {
            pstItem->eTrackStatus = AX_OPAL_ALGO_TRACK_STATUS_LOST;
        } else if (pstObjectItem->eTrackState == AX_SKEL_TRACK_STATUS_SELECT) {
            pstItem->eTrackStatus = AX_OPAL_ALGO_TRACK_STATUS_SELECT;
        } else {
            pstItem->eTrackStatus = AX_OPAL_ALGO_TRACK_STATUS_BUTT;
        }

        if ((pstObjectItem->eTrackState == AX_SKEL_TRACK_STATUS_NEW) || (pstObjectItem->eTrackState == AX_SKEL_TRACK_STATUS_UPDATE)) {
            if (0 >= pstObjectItem->stRect.fW || 0 >= pstObjectItem->stRect.fH) {
                continue;
            }
        }

        // box
        pstItem->stBox.fX = pstObjectItem->stRect.fX / pstResult->nOriginalWidth;
        pstItem->stBox.fY = pstObjectItem->stRect.fY / pstResult->nOriginalHeight;
        pstItem->stBox.fW = pstObjectItem->stRect.fW / pstResult->nOriginalWidth;
        pstItem->stBox.fH = pstObjectItem->stRect.fH / pstResult->nOriginalHeight;
        pstItem->stBox.nImgWidth = pstResult->nOriginalWidth;
        pstItem->stBox.nImgHeight = pstResult->nOriginalHeight;

        LOG_M_I(LOG_TAG, "rect(%.4f,%.4f,%.4f,%.4f), img(%d,%d)",pstItem->stBox.fX, pstItem->stBox.fY,pstItem->stBox.fW,pstItem->stBox.fH,
         pstItem->stBox.nImgWidth, pstItem->stBox.nImgHeight);

        // img
        pstItem->stImg.bExist = pstObjectItem->bCropFrame;
        pstItem->stImg.pData = pstObjectItem->stCropFrame.pFrameData;
        pstItem->stImg.nDataSize = pstObjectItem->stCropFrame.nFrameDataSize;
        pstItem->stImg.nWidth = pstObjectItem->stCropFrame.nFrameWidth;
        pstItem->stImg.nHeight = pstObjectItem->stCropFrame.nFrameHeight;

        // panorama img
        pstItem->stPanoramaImg.bExist = pstObjectItem->bCropFrame;
        pstItem->stPanoramaImg.pData = pstObjectItem->stPanoraFrame.pFrameData;
        pstItem->stPanoramaImg.nDataSize = pstObjectItem->stPanoraFrame.nFrameDataSize;
        pstItem->stPanoramaImg.nWidth = pstObjectItem->stPanoraFrame.nFrameWidth;
        pstItem->stPanoramaImg.nHeight = pstObjectItem->stPanoraFrame.nFrameHeight;

        AX_OPAL_HAL_GetSkelResult(pstObjectItem, pstItem);

        *(AX_U32 *)pCount = nCount + 1;
    }
}

static AX_VOID OpalSkelResultCallback(AX_SKEL_HANDLE pHandle, AX_SKEL_RESULT_T *pstResult, AX_VOID *private) {
    if (!pstResult) {
        return;
    }

    AX_OPAL_HAL_SKEL_FRAME_USER_DATA_T *pUserData = (AX_OPAL_HAL_SKEL_FRAME_USER_DATA_T *)(pstResult->pUserData);
    if (!pUserData) {
        LOG_M_E(LOG_TAG, "skel handle %p frame user data is null", pHandle);
        return;
    }
    AX_OPAL_ALGO_HVCFP_ITEM_T stBodys[SKEL_MAX_RESULT_COUNT] = {0};
    AX_OPAL_ALGO_HVCFP_ITEM_T stVehicles[SKEL_MAX_RESULT_COUNT] = {0};
    AX_OPAL_ALGO_HVCFP_ITEM_T stCycles[SKEL_MAX_RESULT_COUNT] = {0};
    AX_OPAL_ALGO_HVCFP_ITEM_T stFaces[SKEL_MAX_RESULT_COUNT] = {0};
    AX_OPAL_ALGO_HVCFP_ITEM_T stPlats[SKEL_MAX_RESULT_COUNT] = {0};

    AX_U32 nChnId = pUserData->nChnId;
    AX_OPAL_ALGO_RESULT_T stResult;
    memset(&stResult, 0, sizeof(stResult));
    stResult.pUserData = g_stAlgoInfo[nChnId].pUserData;
    stResult.stHvcfpResult.bValid = AX_TRUE;
    stResult.stHvcfpResult.nSnsId = nChnId;
    stResult.stHvcfpResult.u64FrameId = pUserData->nSeqNum;
    stResult.stHvcfpResult.u64Pts = pUserData->u64Pts;
    stResult.stHvcfpResult.pstBodys = &stBodys[0];
    stResult.stHvcfpResult.pstVehicles = &stVehicles[0];
    stResult.stHvcfpResult.pstCycles = &stCycles[0];
    stResult.stHvcfpResult.pstFaces = &stFaces[0];
    stResult.stHvcfpResult.pstPlates = &stPlats[0];

    if (g_stAlgoParam[nChnId].stHvcfpParam.bEnable) {
        SkelResult2DetectResult(&stResult.stHvcfpResult, pstResult);
        AX_OPAL_HAL_UpdateAeRoi(nChnId, &stResult.stHvcfpResult, &g_stAlgoParam[nChnId].stHvcfpParam.stAeRoiConfig[0]);
    }

    if (stResult.stHvcfpResult.bValid && g_stAlgoInfo[nChnId].callback) {
        g_stAlgoInfo[nChnId].callback(nChnId, &stResult);
    }

    ReleaseSkelUserData(pUserData);
}

AX_S32 AX_OPAL_HAL_ALGO_Init(AX_OPAL_ALGO_ATTR_T *pAttr) {
    if (!pAttr) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_ALGO_Init failed, pAttr is nulll");
        return AX_ERR_OPAL_NULL_PTR;
    }
    AX_S32 nRet = 0;
    g_stAlgoAttr = *pAttr;

    if ((pAttr->nAlgoType & AX_OPAL_ALGO_HVCP) || (pAttr->nAlgoType & AX_OPAL_ALGO_FACE)) {
        AX_SKEL_PPL_E ePPL = (pAttr->nAlgoType & AX_OPAL_ALGO_HVCP) ? AX_SKEL_PPL_HVCP : AX_SKEL_PPL_FACE;
        AX_SKEL_INIT_PARAM_T stInit;
        if (pAttr->strDetectModelsPath) {
            stInit.pStrModelDeploymentPath = pAttr->strDetectModelsPath;
            LOG_M_I(LOG_TAG, "detect models path: %s", stInit.pStrModelDeploymentPath);
        } else {
            stInit.pStrModelDeploymentPath = "/opt/etc/skelModels";
            LOG_M_I(LOG_TAG, "detect models path use default: /opt/etc/skelModels");
        }

        for (AX_S32 i = 0; i < AX_OPAL_SNS_ID_BUTT; i++) {
            g_stAlgoParam[i].stHvcfpParam.nAlgoType = (pAttr->nAlgoType & AX_OPAL_ALGO_HVCP) ? AX_OPAL_ALGO_HVCP : AX_OPAL_ALGO_FACE;
        }

        nRet = AX_SKEL_Init(&stInit);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_SKEL_Init failed, ret=0x%x", nRet);
            return nRet;
        }

        g_bSkelInited = AX_TRUE;

        // check detect models
        const AX_SKEL_CAPABILITY_T *pstCapability = NULL;
        nRet = AX_SKEL_GetCapability(&pstCapability);
        if (AX_SKEL_SUCC != nRet) {
            LOG_M_E(LOG_TAG, "AX_SKEL_GetCapability() fail, ret= 0x%x\n", nRet);
            return nRet;
        } else {
            AX_BOOL bHVCFP = AX_FALSE;
            for (AX_U32 i = 0; i < pstCapability->nPPLConfigSize; i++) {
                if (ePPL == pstCapability->pstPPLConfig[i].ePPL) {
                    bHVCFP = AX_TRUE;
                }
            }
            AX_SKEL_Release((AX_VOID *)pstCapability);
            if (!bHVCFP) {
                LOG_M_E(LOG_TAG, "SKEL not found related models for ppl=%s", (ePPL == AX_SKEL_PPL_HVCP) ? "HVCP" : "FACE");
                return nRet;
            }
        }
    }

    if ((pAttr->nAlgoType & AX_OPAL_ALGO_MD) || (pAttr->nAlgoType & AX_OPAL_ALGO_OD)) {
        nRet = AX_IVES_Init();
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVES_Init failed, ret=0x%x", nRet);
            return nRet;
        }

        g_bIvesInited = AX_TRUE;

        if (pAttr->nAlgoType & AX_OPAL_ALGO_MD) {
            nRet = AX_IVES_MD_Init();
            if (0 != nRet) {
                LOG_M_E(LOG_TAG, "AX_IVES_MD_Init failed, ret=0x%x", nRet);
                return nRet;
            }

            g_bMdInited = AX_TRUE;
        }

        if (pAttr->nAlgoType & AX_OPAL_ALGO_OD) {
            nRet = AX_IVES_OD_Init();
            if (0 != nRet) {
                LOG_M_E(LOG_TAG, "AX_IVES_OD_Init failed, ret=0x%x", nRet);
                return nRet;
            }

            g_bOdInited = AX_TRUE;
        }
    }

	return 0;
}

AX_S32 AX_OPAL_HAL_ALGO_Deinit(AX_VOID) {
    AX_S32 nRet = 0;
    if (g_bSkelInited) {
        nRet = AX_SKEL_DeInit();
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_SKEL_DeInit failed, ret=0x%x", nRet);
            return nRet;
        }
    }
    if (g_bMdInited) {
        nRet = AX_IVES_MD_DeInit();
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVES_MD_DeInit failed, ret=0x%x", nRet);
            return nRet;
        }
    }
    if (g_bOdInited) {
        nRet = AX_IVES_OD_DeInit();
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVES_OD_DeInit failed, ret=0x%x", nRet);
            return nRet;
        }
    }
    if (g_bIvesInited) {
        nRet = AX_IVES_DeInit();
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVES_DeInit failed, ret=0x%x", nRet);
            return nRet;
        }
    }
	return 0;
}

AX_S32 AX_OPAL_HAL_ALGO_CreateChn(AX_U32 nChnId, AX_U32 nWidth, AX_U32 nHeight) {
    if (nWidth == 0 || nHeight == 0 || nChnId >= AX_OPAL_SNS_ID_BUTT) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }

    AX_S32 nRet = 0;

    pthread_mutex_lock(&g_mtxAlgo);

    g_stAlgoInfo[nChnId].nWidth = nWidth;
    g_stAlgoInfo[nChnId].nHeight = nHeight;

    if (g_bSkelInited) {
        if (!g_pSkelHandle) {
            AX_SKEL_HANDLE_PARAM_T stHandleParam = {0};
            stHandleParam.ePPL = (g_stAlgoAttr.nAlgoType & AX_OPAL_ALGO_HVCP) ? AX_SKEL_PPL_HVCP : AX_SKEL_PPL_FACE;
            stHandleParam.nFrameDepth = 1;
            stHandleParam.nFrameCacheDepth = 1;
            stHandleParam.nIoDepth = 0;
            stHandleParam.nWidth = nWidth;  // nWidth X nHeight maybe not MAX between two sensors.
            stHandleParam.nHeight = nHeight;

            nRet = AX_SKEL_Create(&stHandleParam, &g_pSkelHandle);
            if(nRet != 0){
                LOG_M_E(LOG_TAG, "AX_SKEL_Create failed, ret=0x%x", nRet);
                goto fail;
            }

            AX_SKEL_RegisterResultCallback(g_pSkelHandle, OpalSkelResultCallback, NULL);

            AX_OPAL_HAL_InitSkelParam(0, g_pSkelHandle, &g_stAlgoParam[0].stHvcfpParam);
            g_nSkelHandleRef++;
        } else {
            g_nSkelHandleRef++;
        }
    }

    if (g_bMdInited || g_bOdInited) {
        LOG_M_C(LOG_TAG, "[%d]ives frmctrl %d->%d", nChnId, g_stAlgoParam[nChnId].stIvesParam.nSrcFramerate, g_stAlgoParam[nChnId].stIvesParam.nDstFramerate);
        if (!g_stAlgoInfo[nChnId].pFrmCtrl) {
            AX_OPAL_FrmCtrlCreate(&g_stAlgoInfo[nChnId].pFrmCtrl, g_stAlgoParam[nChnId].stIvesParam.nSrcFramerate, g_stAlgoParam[nChnId].stIvesParam.nDstFramerate);
        } else {
            AX_OPAL_FrmCtrlReset(g_stAlgoInfo[nChnId].pFrmCtrl, g_stAlgoParam[nChnId].stIvesParam.nSrcFramerate, g_stAlgoParam[nChnId].stIvesParam.nDstFramerate);
        }
    }

    if (g_bMdInited) {
        if (g_stAlgoInfo[nChnId].bMdCreated) {
            LOG_M_E(LOG_TAG, "MD[%d] channels has created", nChnId);
            goto fail;
        }

        nRet = MD_Setup(nChnId);
        if(nRet != 0){
            goto fail;
        }
    }

    if (g_bOdInited) {
        if (g_stAlgoInfo[nChnId].bOdCreated) {
            LOG_M_E(LOG_TAG, "OD[%d] channels has created", nChnId);
            goto fail;
        }

        nRet = OD_Setup(nChnId);
        if(nRet != 0){
            goto fail;
        }
    }
    pthread_mutex_unlock(&g_mtxAlgo);
    return 0;
fail:
    pthread_mutex_unlock(&g_mtxAlgo);
    return -1;
}

AX_S32 AX_OPAL_HAL_ALGO_DestroyChn(AX_U32 nChnId) {
    if (nChnId >= AX_OPAL_SNS_ID_BUTT) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }
    AX_S32 nRet = 0;
    pthread_mutex_lock(&g_mtxAlgo);
    if (g_bSkelInited && g_pSkelHandle) {
        g_nSkelHandleRef--;
        if (g_nSkelHandleRef == 0) {
            nRet = AX_SKEL_Destroy(g_pSkelHandle);
            if(nRet != 0){
                LOG_M_E(LOG_TAG, "AX_SKEL_Destroy [%d] failed, ret=0x%x", nChnId, nRet);
                goto fail;
            }
            g_pSkelHandle = NULL;
        }
    }

    nRet = MD_Clean(nChnId);
    if(nRet != 0){
        goto fail;
    }

    nRet = OD_Clean(nChnId);
    if(nRet != 0){
        goto fail;
    }
    pthread_mutex_unlock(&g_mtxAlgo);
    return 0;
fail:
    pthread_mutex_unlock(&g_mtxAlgo);
    return -1;
}

AX_S32 AX_OPAL_HAL_ALGO_SetParam(AX_U32 nChnId, const AX_OPAL_ALGO_PARAM_T *pParam) {
    if (nChnId >= AX_OPAL_SNS_ID_BUTT || !pParam) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }
    AX_S32 nRet = 0;

    if (g_bSkelInited) {
        if (g_stAlgoParam[nChnId].stHvcfpParam.bEnable
            && !pParam->stHvcfpParam.bEnable) {
            g_nSkelDisableDelayFrm = SKEL_DISABLE_DELAY_FRM;
        }

        AX_OPAL_HAL_SetSkelParam(nChnId, g_pSkelHandle, &g_stAlgoParam[nChnId].stHvcfpParam, &pParam->stHvcfpParam);
        AX_OPAL_HAL_SetAeRoiAttr(nChnId, &g_stAlgoParam[nChnId].stHvcfpParam.stAeRoiConfig[0], g_stAlgoParam[nChnId].stHvcfpParam.bEnable);
    }

    g_stAlgoParam[nChnId].stIvesParam.nSrcFramerate = pParam->stIvesParam.nSrcFramerate;
    g_stAlgoParam[nChnId].stIvesParam.nDstFramerate = pParam->stIvesParam.nDstFramerate;
    if (g_stAlgoInfo[nChnId].pFrmCtrl) {
        AX_OPAL_FrmCtrlReset(g_stAlgoInfo[nChnId].pFrmCtrl, g_stAlgoParam[nChnId].stIvesParam.nSrcFramerate, g_stAlgoParam[nChnId].stIvesParam.nDstFramerate);
    }

    if (g_bMdInited) {
        if (g_stAlgoInfo[nChnId].bMdCreated) {
            MD_Clean(nChnId);
            g_stAlgoParam[nChnId].stIvesParam.stMdParam = pParam->stIvesParam.stMdParam;
            nRet = MD_Setup(nChnId);
            if(nRet != 0){
                return -1;
            }
        } else {
            g_stAlgoParam[nChnId].stIvesParam.stMdParam = pParam->stIvesParam.stMdParam;
        }
    } else {
        g_stAlgoParam[nChnId].stIvesParam.stMdParam = pParam->stIvesParam.stMdParam;
    }

    if (g_bOdInited) {
        if (g_stAlgoInfo[nChnId].bOdCreated) {
            OD_Clean(nChnId);
            g_stAlgoParam[nChnId].stIvesParam.stOdParam = pParam->stIvesParam.stOdParam;
            nRet = OD_Setup(nChnId);
            if(nRet != 0){
                return -1;
            }
        } else {
            g_stAlgoParam[nChnId].stIvesParam.stOdParam = pParam->stIvesParam.stOdParam;
        }
    } else {
        g_stAlgoParam[nChnId].stIvesParam.stOdParam = pParam->stIvesParam.stOdParam;
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_ALGO_GetParam(AX_U32 nChnId, AX_OPAL_ALGO_PARAM_T *pParam) {
    if (nChnId >= AX_OPAL_SNS_ID_BUTT || !pParam) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }

    *pParam = g_stAlgoParam[nChnId];
    return 0;
}

AX_S32 AX_OPAL_HAL_ALGO_RegCallback(AX_U32 nChnId, const AX_OPAL_VIDEO_ALGO_CALLBACK callback, AX_VOID *pUserData) {
    if (nChnId >= AX_OPAL_SNS_ID_BUTT || !callback) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }
    g_stAlgoInfo[nChnId].callback = callback;
    g_stAlgoInfo[nChnId].pUserData = pUserData;
    return 0;
}


AX_S32 AX_OPAL_HAL_ALGO_UnregCallback(AX_U32 nChnId) {
    if (nChnId >= AX_OPAL_SNS_ID_BUTT) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }
    g_stAlgoInfo[nChnId].callback = NULL;
    g_stAlgoInfo[nChnId].pUserData = NULL;
    return 0;

}

AX_S32 AX_OPAL_HAL_ALGO_UpdateRotation(AX_U32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation) {
    if (nChnId >= AX_OPAL_SNS_ID_BUTT) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }
    AX_S32 nRet = 0;
    if (g_stAlgoInfo[nChnId].eRotation != eRotation)
    {
        MD_Clean(nChnId);
        OD_Clean(nChnId);
        g_stAlgoInfo[nChnId].eRotation = eRotation;
        nRet = MD_Setup(nChnId);
        if(nRet != 0){
            return -1;
        }
        nRet = OD_Setup(nChnId);
        if(nRet != 0){
            return -1;
        }
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_ALGO_ProcessFrame(AX_U32 nChnId, AX_VIDEO_FRAME_T *pFrame) {
    if (nChnId >= AX_OPAL_SNS_ID_BUTT || !pFrame) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }
    AX_S32 nRet = 0;
    if (g_pSkelHandle
        && (g_stAlgoParam[nChnId].stHvcfpParam.bEnable
            || g_nSkelDisableDelayFrm > 0)) {

        AX_OPAL_HAL_SKEL_FRAME_USER_DATA_T *pUserData = GetSkelUserData();
        if (pUserData) {
            pUserData->u64Pts = pFrame->u64PTS;
            pUserData->nSeqNum = pFrame->u64SeqNum;
            pUserData->nChnId = nChnId;
            AX_SKEL_FRAME_T skel_frame = {0};
            skel_frame.nStreamId = nChnId;
            skel_frame.nFrameId = g_stAlgoInfo[nChnId].nFrameID++;
            skel_frame.stFrame = *pFrame;
            skel_frame.pUserData = pUserData;
            nRet = AX_SKEL_SendFrame(g_pSkelHandle, &skel_frame, 500);

            if (AX_SKEL_SUCC != nRet) {
                ReleaseSkelUserData(pUserData);
                if (AX_ERR_SKEL_QUEUE_FULL != nRet
                    && AX_ERR_SKEL_TIMEOUT != nRet) {
                    LOG_M_E(LOG_TAG, "AX_SKEL_SendFrame [%d] failed, ret=0x%x", nChnId, nRet);
                }
            } else {
                if (g_nSkelDisableDelayFrm > 0) {
                    g_nSkelDisableDelayFrm--;
                }
            }
        } else {
            LOG_M_E(LOG_TAG, "GetSkelUserData [%d] failed", nChnId);
        }
    }

    AX_OPAL_ALGO_IVES_ITEM_T stMds[AX_OPAL_MAX_ALGO_MD_REGION_COUNT];
    AX_OPAL_ALGO_IVES_ITEM_T stOds;
    AX_OPAL_ALGO_RESULT_T stResult;
    memset(&stResult, 0, sizeof(stResult));
    memset(&stMds[0], 0, sizeof(AX_OPAL_ALGO_IVES_ITEM_T) * AX_OPAL_MAX_ALGO_MD_REGION_COUNT);
    memset(&stOds, 0, sizeof(stOds));
    stResult.pUserData = g_stAlgoInfo[nChnId].pUserData;
    stResult.stIvesResult.pstMds = &stMds[0];
    stResult.stIvesResult.pstOds = &stOds;

    AX_F32 nW = g_stAlgoInfo[nChnId].nWidth;
    AX_F32 nH = g_stAlgoInfo[nChnId].nHeight;
    if (g_stAlgoInfo[nChnId].eRotation == AX_OPAL_SNS_ROTATION_90 || g_stAlgoInfo[nChnId].eRotation == AX_OPAL_SNS_ROTATION_270) {
        nW = g_stAlgoInfo[nChnId].nHeight;
        nH = g_stAlgoInfo[nChnId].nWidth;
    }

    AX_BOOL bSkip = AX_OPAL_FrmCtrlFilter(g_stAlgoInfo[nChnId].pFrmCtrl, AX_FALSE);

    if (g_stAlgoInfo[nChnId].bMdCreated && g_stAlgoParam[nChnId].stIvesParam.stMdParam.bEnable && !bSkip) {

        for (AX_S32 i = 0; i < g_stAlgoParam[nChnId].stIvesParam.stMdParam.nRegionSize; i++) {
            AX_MD_MB_THR_T stThrs;
            AX_MD_MB_SAD_T stSad;
            AX_IVES_CCBLOB_T stBlob;
            memset(&stThrs, 0, sizeof(stThrs));
            memset(&stSad, 0, sizeof(stSad));
            memset(&stBlob, 0, sizeof(stBlob));
            MD_CHN mdChn = nChnId * AX_OPAL_MAX_ALGO_MD_REGION_COUNT + i;
            nRet = AX_IVES_MD_Process(mdChn, (AX_IVES_IMAGE_T*)pFrame, &stThrs, &stSad, &stBlob);
            if(nRet != 0){
                // LOG_M_E(LOG_TAG, "AX_IVES_MD_Process [%d] failed, ret=0x%x", mdChn, nRet);
            }

            AX_U32 nSumThrs = 0;
            for (AX_U32 k = 0; k < stThrs.u32Count; ++k) {
                nSumThrs += stThrs.pMbThrs[k];
            }

            AX_U8 nLastMdRslt = g_stAlgoInfo[nChnId].nMdResult[i];
            AX_OPAL_ALGO_MD_REGION_T rgn = g_stAlgoParam[nChnId].stIvesParam.stMdParam.stRegions[i];
            AX_U8 nConfidence = (AX_U8)(rgn.fConfidence * (rgn.stRect.fW / rgn.stRect.fH) * 100);

            g_stAlgoInfo[nChnId].nMdResult[i] = (nSumThrs >= nConfidence) ? 1 : 0;

            if (1 == g_stAlgoInfo[nChnId].nMdResult[i] && nLastMdRslt != g_stAlgoInfo[nChnId].nMdResult[i]) {
                AX_MD_CHN_ATTR_T stMdChnAtrr = {0};
                AX_IVES_MD_GetChnAttr(mdChn, &stMdChnAtrr);
                stMds[stResult.stIvesResult.nMdSize].eType = AX_OPAL_ALGO_IVES_MD;
                stMds[stResult.stIvesResult.nMdSize].fConfidence = 0;
                stMds[stResult.stIvesResult.nMdSize].stBox.fX = (AX_F32)stMdChnAtrr.stArea.u32X / nW;
                stMds[stResult.stIvesResult.nMdSize].stBox.fY = (AX_F32)stMdChnAtrr.stArea.u32Y / nH;
                stMds[stResult.stIvesResult.nMdSize].stBox.fH = (AX_F32)stMdChnAtrr.stArea.u32W / nW;
                stMds[stResult.stIvesResult.nMdSize].stBox.fW = (AX_F32)stMdChnAtrr.stArea.u32H / nH;
                stMds[stResult.stIvesResult.nMdSize].stBox.nImgWidth = (AX_U32)nW;
                stMds[stResult.stIvesResult.nMdSize].stBox.nImgHeight = (AX_U32)nH;
                stMds[stResult.stIvesResult.nMdSize].u64FrameId = pFrame->u64SeqNum;
                stResult.stIvesResult.nMdSize++;
            }
        }
        if (stResult.stIvesResult.nMdSize > 0) {
            stResult.stIvesResult.bValid = AX_TRUE;
            stResult.stIvesResult.nSnsId = nChnId;
            stResult.stIvesResult.u64FrameId = pFrame->u64SeqNum;
            stResult.stIvesResult.u64Pts = pFrame->u64PTS;
        }
    }

    if (g_stAlgoInfo[nChnId].bOdCreated && g_stAlgoParam[nChnId].stIvesParam.stOdParam.bEnable && !bSkip) {
        AX_IVES_OD_IMAGE_T stOdImg;
        memset(&stOdImg, 0, sizeof(stOdImg));
        stOdImg.pstImg = (AX_IVES_IMAGE_T*)pFrame;

        AX_ISP_IQ_AE_STATUS_T tAE;
        AX_U8 nPipeID = (AX_U8) nChnId;
        if (0 == AX_ISP_IQ_GetAeStatus(nPipeID, &tAE)) {
            stOdImg.u32Lux = tAE.tAlgStatus.nLux;
        } else {
            stOdImg.u32Lux = 0;
            LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetAeStatus(pipe: %d) fail", nChnId);
        }

        AX_U8 nLastOdResult = g_stAlgoInfo[nChnId].nOdResult;
        nRet = AX_IVES_OD_Process(nChnId, &stOdImg, &g_stAlgoInfo[nChnId].nOdResult);
        if(nRet != 0){
            LOG_M_E(LOG_TAG, "AX_IVES_OD_Process [%d] failed, ret=0x%x", nChnId, nRet);
        }

        if (1 == g_stAlgoInfo[nChnId].nOdResult && nLastOdResult != g_stAlgoInfo[nChnId].nOdResult) {
            stOds.eType = AX_OPAL_ALGO_IVES_OD;
            stOds.fConfidence = g_stAlgoParam[nChnId].stIvesParam.stOdParam.fConfidence;
            stOds.stBox.fX = 0;
            stOds.stBox.fY = 0;
            stOds.stBox.fH = 1;
            stOds.stBox.fW = 1;
            stOds.stBox.nImgWidth = (AX_U32)nW;
            stOds.stBox.nImgHeight = (AX_U32)nH;
            stOds.u64FrameId = pFrame->u64SeqNum;
            stResult.stIvesResult.bValid = AX_TRUE;
            stResult.stIvesResult.nOdSize = 1;
            stResult.stIvesResult.nSnsId = nChnId;
            stResult.stIvesResult.u64FrameId = pFrame->u64SeqNum;
            stResult.stIvesResult.u64Pts = pFrame->u64PTS;
        }
    }

    if (stResult.stIvesResult.bValid && g_stAlgoInfo[nChnId].callback) {
        g_stAlgoInfo[nChnId].callback(nChnId, &stResult);
    }
    return 0;
}
