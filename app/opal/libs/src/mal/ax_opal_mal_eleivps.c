/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_eleivps.h"
#include "ax_opal_mal_utils.h"

#include "ax_opal_hal_ivps_parser.h"
#include "ax_opal_hal_ivps_def.h"
#include "ax_opal_hal_ivps.h"
#include "ax_opal_hal_osd.h"

#include "ax_opal_log.h"
#include "ax_opal_utils.h"
#include "ax_opal_mal_element.h"

#define LOG_TAG ("ELEIVPS")

#include <unistd.h>


static AX_OPAL_IVPS_ATTR_T g_OpalIvpsAttr = {
    .nGrpChnCnt = 3,
    .bMaskEnable = AX_TRUE,
    .tGrpAttr = {
        .eEngineType0 = 5,
        .eEngineType1 = 3,
        .tFramerate = {
            .fSrc = -1,
            .fDst = -1,
        },
        .tResolution = {
            .nWidth = -1,
            .nHeight = -1,
        },
        .nFifoDepth = 1,
        .tCompress = {
            .enCompressMode = 0,
            .u32CompressLevel = 0,
        },
        .bInplace0 = AX_FALSE,
        .bInplace1 = AX_FALSE,
    },
    .arrChnAttr = {
        [0] = {
            .bEnable = AX_TRUE,
            .eEngineType0 = 0,
            .eEngineType1 = 1,
            .tFramerate = {
                .fSrc = -1,
                .fDst = -1,
            },
            .tResolution = {
                .nWidth = -1,
                .nHeight = -1,
            },
            .nFifoDepth = 0,
            .tCompress = {
                .enCompressMode = 2,
                .u32CompressLevel = 4,
            },
            .bInplace0 = AX_FALSE,
            .bInplace1 = AX_TRUE,
            .eSclType = 2,
            .bVoOsd = AX_TRUE,
            .bVoRect = AX_FALSE,
        },
        [1] = {
            .bEnable = AX_TRUE,
            .eEngineType0 = 0,
            .eEngineType1 = 5,
            .tFramerate = {
                .fSrc = -1,
                .fDst = 10,
            },
            .tResolution = {
                .nWidth = 1280,
                .nHeight = 720,
            },
            .nFifoDepth = 1,
            .tCompress = {
                .enCompressMode = 0,
                .u32CompressLevel = 0,
            },
            .bInplace0 = AX_FALSE,
            .bInplace1 = AX_FALSE,
            .eSclType = 1,
            .bVoOsd = AX_FALSE,
            .bVoRect = AX_FALSE,
        },
        [2] = {
            .bEnable = AX_TRUE,
            .eEngineType0 = 0,
            .eEngineType1 = 1,
            .tFramerate = {
                .fSrc = -1,
                .fDst = -1,
            },
            .tResolution = {
                .nWidth = 720,
                .nHeight = 576,
            },
            .nFifoDepth = 0,
            .tCompress = {
                .enCompressMode = 2,
                .u32CompressLevel = 4,
            },
            .bInplace0 = AX_FALSE,
            .bInplace1 = AX_TRUE,
            .eSclType = 2,
            .bVoOsd = AX_TRUE,
            .bVoRect = AX_FALSE,
        },
    }
};

// interface vtable
AX_OPAL_MAL_ELE_Interface ivps_interface = {
    .start = AX_OPAL_MAL_ELEIVPS_Start,
    .stop = AX_OPAL_MAL_ELEIVPS_Stop,
    .event_proc = AX_OPAL_MAL_ELEIVPS_Process,
};

static void msSleep(AX_U32 milliseconds)
{
    struct timespec ts = {
        (time_t)(milliseconds / 1000),
        (long)((milliseconds % 1000) * 1000000)
        };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
}

static AX_VOID RectThreadProc(AX_VOID *arg) {
    LOG_M_D(LOG_TAG, "+++");

    AX_OPAL_THREAD_T *pThread = (AX_OPAL_THREAD_T*)arg;
    if (!pThread) {
        return;
    }

    AX_OPAL_MAL_RECT_THREADPARAM_T* pRectParam = (AX_OPAL_MAL_RECT_THREADPARAM_T*)pThread->arg;
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T* pChnOsd = (AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T*)pRectParam->pChnOsd;

    AX_CHAR thread_name[16];
    sprintf(thread_name, "rect_%d_%d", pRectParam->nGrpId, pRectParam->nChnId);
    prctl(PR_SET_NAME, thread_name);

    while (pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING) {
        if (!pChnOsd->nOsdHandle) {
            goto EXIT;
        }

        AX_OPAL_VIDEO_CHN_ATTR_T *pstVideoChnAttr = AX_OPAL_MAL_GetChnAttr(pRectParam->elehdl, pRectParam->nGrpId, pRectParam->nChnId);
        if (pstVideoChnAttr && !pstVideoChnAttr->bEnable) {
            for(int i = 0; i < 25 && pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING; i++) {
                msSleep(20);
            }
            continue;
        }

        AX_OPAL_HAL_OSD_UpdatePolygon(pChnOsd->nOsdHandle,
                                        pRectParam->nRectNum,
                                        pRectParam->stRects,
                                        pRectParam->nPolygonNum,
                                        pRectParam->stPolygons,
                                        AX_FALSE);

        msSleep(33);
    }

EXIT:
    LOG_M_D(LOG_TAG, "---");
}

static AX_VOID TimeThreadProc(AX_VOID *arg) {
    LOG_M_D(LOG_TAG, "+++");

    AX_OPAL_THREAD_T *pThread = (AX_OPAL_THREAD_T*)arg;
    if (!pThread) {
        return;
    }

    AX_OPAL_MAL_TIMEOSD_THREADPARAM_T* pTimeParam = (AX_OPAL_MAL_TIMEOSD_THREADPARAM_T*)pThread->arg;
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T* pChnOsd = (AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T*)pTimeParam->pChnOsd;

    AX_CHAR thread_name[16];
    sprintf(thread_name, "time_%d_%d", pTimeParam->nGrpId, pTimeParam->nChnId);
    prctl(PR_SET_NAME, thread_name);

	while (pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING) {
        if (!pChnOsd->bInit) {
            goto EXIT;
        }

        AX_OPAL_VIDEO_CHN_ATTR_T *pstVideoChnAttr = AX_OPAL_MAL_GetChnAttr(pTimeParam->elehdl, pTimeParam->nGrpId, pTimeParam->nChnId);
        if (pstVideoChnAttr && !pstVideoChnAttr->bEnable) {
            for(int i = 0; i < 25 && pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING; i++) {
                msSleep(20);
            }
            continue;
        }

        AX_IVPS_CHN_ATTR_T tChnAttr;
        memset(&tChnAttr, 0x0, sizeof(AX_IVPS_CHN_ATTR_T));
        AX_S32 s32Ret = AX_IVPS_GetChnAttr(pTimeParam->nGrpId, pTimeParam->nChnId, 0, &tChnAttr);
        if (AX_SUCCESS != s32Ret) {
            goto EXIT;
        }

        AX_OPAL_HAL_OSD_UpdateTime(pChnOsd->nOsdHandle, &pChnOsd->stOsdAttr, tChnAttr.nDstPicWidth, tChnAttr.nDstPicHeight);

        for (int i = 0; i < 25; i++) {
            if (!(pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING)) {
                goto EXIT;
            }
            msSleep(10);
        }
	}

EXIT:
    LOG_M_D(LOG_TAG, "---");
}

static AX_VOID GetFrameThread(AX_VOID* arg) {
    LOG_M_D(LOG_TAG, "+++");

    AX_S32 nRet = 0;

    AX_S32 nTimeOutMS = 30;

    AX_OPAL_THREAD_T* pThread = (AX_OPAL_THREAD_T*)arg;
    if (!pThread) {
        return;
    }

    AX_OPAL_MAL_ELEIVPS_T* pEle = (AX_OPAL_MAL_ELEIVPS_T*)pThread->arg;
    if (!pEle) {
        return;
    }

    AX_S32 nGrpId = pEle->stBase.stAttr.arrGrpAttr[0].nGrpId;

    AX_CHAR thread_name[16];
    sprintf(thread_name, "ivps_%d", nGrpId);
    prctl(PR_SET_NAME, thread_name);

    AX_VIDEO_FRAME_T tVideoFrame;
    memset(&tVideoFrame, 0x0, sizeof(AX_VIDEO_FRAME_T));
    AX_IVPS_PIPELINE_ATTR_T tPipelineAttr;
    memset(&tPipelineAttr, 0x0, sizeof(AX_IVPS_PIPELINE_ATTR_T));

    /* only one grp */
    AX_OPAL_GRP_ATTR_T *pGrpAttr = &pEle->stBase.stAttr.arrGrpAttr[0];
    AX_BOOL bPipeAttrChanged[AX_OPAL_MAX_CHN_CNT] = {AX_FALSE};
    while (pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING) {
        AX_BOOL bChnGet[AX_OPAL_MAX_CHN_CNT] = {AX_FALSE};

        // s32Ret = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
        // if (AX_SUCCESS != s32Ret) {
        //     usleep(10 * 1000);
        //     continue;
        // }

        for (AX_S32 iChn = 0; iChn < pGrpAttr->nChnCnt; ++iChn) {
            // TODO: map index to chnid
            // AX_S32 nChnId = pGrpAttr->nChnId[iChn];

            // TODO: set outfifo depth
            // if (tPipelineAttr.nOutFifoDepth[iChn] == 0) {
            //     continue;
            // }

            AX_OPAL_MAL_IVPS_FRAME_THREADPARAM_T *pThreadParam = &pEle->stEleIvpsAttr.arrThreadFrameParam[iChn];
            AX_OPAL_MAL_OBS_T *parrObs = pGrpAttr->arrObs[iChn];
            for (AX_S32 iObs = 0; iObs < AX_OPAL_MAL_OBS_CNT; ++iObs) {
                AX_OPAL_MAL_OBS_T *pObs = &parrObs[iObs];
                AX_OPAL_MAL_ELE_T* pEleObs = (AX_OPAL_MAL_ELE_T*)pObs->pEleHdl;
                if (pEleObs) {
                    if (pEleObs->vTable && pEleObs->vTable->data_proc) {
                        bChnGet[iChn] = AX_TRUE;
                        break;
                    }
                }
            }

            if (bChnGet[iChn]) {
                AX_IVPS_PIPELINE_ATTR_T tPipelineAttr = {0};
                nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
                if (AX_SUCCESS == nRet) {
                    if (tPipelineAttr.nOutFifoDepth[iChn] <= 0) {
                        bPipeAttrChanged[iChn] = AX_TRUE;

                        tPipelineAttr.nOutFifoDepth[iChn] = 1;
                        nRet = AX_IVPS_SetPipelineAttr(nGrpId, &tPipelineAttr);
                        if (AX_SUCCESS != nRet) {
                            LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, iChn, nRet);
                            break;
                        }
                    }
                }
            } else {
                if (bPipeAttrChanged[iChn]) {
                    bPipeAttrChanged[iChn] = AX_FALSE;

                    AX_IVPS_PIPELINE_ATTR_T tPipelineAttr = {0};
                    nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
                    if (AX_SUCCESS == nRet) {
                        if (tPipelineAttr.nOutFifoDepth[iChn] > 0) {
                            tPipelineAttr.nOutFifoDepth[iChn] = 0;
                            nRet = AX_IVPS_SetPipelineAttr(nGrpId, &tPipelineAttr);
                            if (AX_SUCCESS != nRet) {
                                LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, iChn, nRet);
                                break;
                            }
                        }
                    }
                }

                msSleep(10);
                continue;
            }

            nRet = AX_IVPS_GetChnFrame(nGrpId, iChn, &tVideoFrame, nTimeOutMS);
            if (AX_SUCCESS != nRet) {
                if (AX_ERR_IVPS_BUF_EMPTY == nRet || AX_ERR_IVPS_NOT_PERM == nRet) {
                    msSleep(1);
                    continue;
                }
                LOG_M_E(LOG_TAG, "[%d][%d] Get ivps frame failed. ret=0x%x", nGrpId, iChn, nRet);
                msSleep(1);
                continue;
            }

            /* observer for NONLINK_* type element*/
            for (AX_S32 iObs = 0; iObs < AX_OPAL_MAL_OBS_CNT; ++iObs) {
                AX_OPAL_MAL_OBS_T *pObs = &parrObs[iObs];
                AX_OPAL_MAL_ELE_T* pEleObs = (AX_OPAL_MAL_ELE_T*)pObs->pEleHdl;
                if (pEleObs) {
                    if (pEleObs->vTable && pEleObs->vTable->data_proc) {
                        pEleObs->vTable->data_proc(pEleObs,
                                    pThreadParam->nUniGrpId, pThreadParam->nUniChnId,
                                    pObs->eLinkType, (AX_VOID*)&tVideoFrame, sizeof(tVideoFrame));
                    }
                }
            }

            AX_IVPS_ReleaseChnFrame(nGrpId, iChn, &tVideoFrame);
        }
    }

    LOG_M_D(LOG_TAG, "---");
}

static AX_S32 UpdateRgn(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pChnOsd) {

    AX_S32 s32Ret = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_S32 hdlRgn = pChnOsd->nOsdHandle;
    AX_OPAL_OSD_ATTR_T *pOsdAttr = &(pChnOsd->stOsdAttr);

    AX_IVPS_CHN_ATTR_T tChnAttr;
    memset(&tChnAttr, 0x0, sizeof(AX_IVPS_CHN_ATTR_T));
    s32Ret = AX_IVPS_GetChnAttr(nGrpId, nChnId, 0, &tChnAttr);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(LOG_TAG, "AX_IVPS_GetChnAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, s32Ret);
        return -1;
    }

    if (AX_OPAL_OSD_TYPE_PICTURE == pOsdAttr->eType) {
        s32Ret = AX_OPAL_HAL_OSD_UpdatePic(hdlRgn, pOsdAttr, tChnAttr.nDstPicWidth, tChnAttr.nDstPicHeight);
        if (AX_SUCCESS != s32Ret) {
            LOG_M_E(LOG_TAG, "AX_OPAL_HAL_OSD_UpdatePic(Grp: %d, Chn: %d, Rgn: %d) failed, ret=0x%x", nGrpId, nChnId, hdlRgn, s32Ret);
            return -1;
        }
    } else if (AX_OPAL_OSD_TYPE_STRING == pOsdAttr->eType || AX_OPAL_OSD_TYPE_STRING_TOP == pOsdAttr->eType) {
        s32Ret = AX_OPAL_HAL_OSD_UpdateStr(hdlRgn, pOsdAttr, tChnAttr.nDstPicWidth, tChnAttr.nDstPicHeight);
        if (AX_SUCCESS != s32Ret) {
            LOG_M_E(LOG_TAG, "AX_OPAL_HAL_OSD_UpdateStr(Grp: %d, Chn: %d, Rgn: %d) failed, ret=0x%x", nGrpId, nChnId, hdlRgn, s32Ret);
            return -1;
        }
    }
    else if (AX_OPAL_OSD_TYPE_PRIVACY == pOsdAttr->eType) {
        s32Ret = AX_OPAL_HAL_OSD_UpdatePrivacy(hdlRgn, pOsdAttr, tChnAttr.nDstPicWidth, tChnAttr.nDstPicHeight);
        if (AX_SUCCESS != s32Ret) {
            LOG_M_E(LOG_TAG, "AX_OPAL_HAL_OSD_UpdatePrivacy(Grp: %d, Chn: %d, Rgn: %d) failed, ret=0x%x", nGrpId, nChnId, hdlRgn, s32Ret);
            return -1;
        }
    }
    else if (AX_OPAL_OSD_TYPE_TIME == pOsdAttr->eType) {
        AX_OPAL_MAL_ELEIVPS_T* pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
        if (pEle->stEleIvpsAttr.arrThreadTimeParam[nChnId].pTimeThread == AX_NULL) {
            pEle->stEleIvpsAttr.arrThreadTimeParam[nChnId].nGrpId = nGrpId;
            pEle->stEleIvpsAttr.arrThreadTimeParam[nChnId].nChnId = nChnId;
            pEle->stEleIvpsAttr.arrThreadTimeParam[nChnId].pChnOsd = pChnOsd;
            pEle->stEleIvpsAttr.arrThreadTimeParam[nChnId].elehdl = self;
            pEle->stEleIvpsAttr.arrThreadTimeParam[nChnId].pTimeThread = AX_OPAL_CreateThread(TimeThreadProc, &pEle->stEleIvpsAttr.arrThreadTimeParam[nChnId]);
            AX_OPAL_StartThread(pEle->stEleIvpsAttr.arrThreadTimeParam[nChnId].pTimeThread);
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return s32Ret;
}

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELEIVPS_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr) {

    AX_OPAL_MAL_ELE_HANDLE handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_ELEIVPS_T));
    if (handle == AX_NULL) {
        LOG_M_E(LOG_TAG, "malloc failed.");
        return AX_NULL;
    }

    AX_OPAL_MAL_ELEIVPS_T* pEle = (AX_OPAL_MAL_ELEIVPS_T*)handle;
    memset(pEle, 0x0, sizeof(AX_OPAL_MAL_ELEIVPS_T));
    pEle->stBase.nId = pEleAttr->nId;
    pEle->stBase.pParent = parent;
    pEle->stBase.vTable = &ivps_interface;
    memcpy(&pEle->stBase.stAttr, pEleAttr, sizeof(AX_OPAL_ELEMENT_ATTR_T));
    AX_OPAL_MAL_ELE_MappingGrpChn(handle);

    /* parse attr */
    if (pEle->stBase.stAttr.pConfigIniPath != AX_NULL) {
        AX_S32 nRet = AX_OPAL_HAL_IVPS_Parse(pEle->stBase.stAttr.pConfigIniPath, &pEle->stOpalIvpsAttr);
        if (0 != nRet) {
            AX_OPAL_FREE(pEle);
            return AX_NULL;
        }
    }
    else {
        memcpy(&pEle->stOpalIvpsAttr, &g_OpalIvpsAttr, sizeof(AX_OPAL_IVPS_ATTR_T));
    }

    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pEle->stBase.pParent;
    if (pEle->stOpalIvpsAttr.tGrpAttr.tResolution.nWidth == -1) {
        pEle->stOpalIvpsAttr.tGrpAttr.tResolution.nWidth = pSubPipeline->stAttr.nInWidth;
        pEle->stOpalIvpsAttr.tGrpAttr.tResolution.nHeight = pSubPipeline->stAttr.nInHeight;
    }

    for (AX_S32 iChn = 0; iChn < pEle->stOpalIvpsAttr.nGrpChnCnt; ++iChn) {
        if (pEle->stOpalIvpsAttr.arrChnAttr[iChn].tResolution.nWidth == -1) {
            pEle->stOpalIvpsAttr.arrChnAttr[iChn].tResolution.nWidth = pSubPipeline->stAttr.nOutWidth == -1 ? pSubPipeline->stAttr.nInWidth : pSubPipeline->stAttr.nOutWidth;
            pEle->stOpalIvpsAttr.arrChnAttr[iChn].tResolution.nHeight = pSubPipeline->stAttr.nOutHeight == -1 ? pSubPipeline->stAttr.nInHeight : pSubPipeline->stAttr.nOutHeight;
        }
    }

    return handle;
}

AX_S32 AX_OPAL_MAL_ELEIVPS_Destroy(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_FREE(self);

    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEIVPS_Start(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEIVPS_T* pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    AX_OPAL_GRP_ATTR_T *pGrpAttr = &pEle->stBase.stAttr.arrGrpAttr[0];
    AX_S32 nGrpId = pGrpAttr->nGrpId;

    nRet = AX_OPAL_HAL_IVPS_Open(nGrpId, &pEle->stOpalIvpsAttr);
    if (nRet != 0) {
        return -1;
    }

    for (AX_S32 iChn = 0; iChn < AX_OPAL_MAX_CHN_CNT; ++iChn) {
        for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
            pEle->stEleIvpsAttr.arrChnOsdAttr[iChn][iOsd].nOsdHandle = -1;
        }
        pEle->stEleIvpsAttr.arrChnOsdRect[iChn][0].nOsdHandle = -1;
        pEle->stEleIvpsAttr.arrChnOsdRect[iChn][1].nOsdHandle = -1;
    }

    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = AX_NULL;
    for (AX_S32 iChn = 0; iChn < pGrpAttr->nChnCnt; ++iChn) {
        for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
            pOsd = &pEle->stEleIvpsAttr.arrChnOsdAttr[iChn][iOsd];
            if (pOsd->bInit == AX_TRUE) {
                pOsd->nOsdHandle = AXOP_HAL_IVPS_CreateOsd(nGrpId, iChn, &(pOsd->stOsdAttr));
                if (pOsd->nOsdHandle == AX_IVPS_INVALID_REGION_HANDLE) {
                    return -1;
                }
                UpdateRgn(self, nGrpId, iChn, pOsd);
                if (nRet != 0) {
                    return -1;
                }
            }
        }
    }

    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = AX_OPAL_MAL_GetSnsAttr(self);
    AX_F32 fFrameRate = pstSnsAttr->fFrameRate;
    for (AX_S32 iChn = 0; iChn < pGrpAttr->nChnCnt; ++iChn) {
        AX_F32 fSrcFps = pEle->stOpalIvpsAttr.arrChnAttr[iChn].tFramerate.fSrc == -1 ? fFrameRate : pEle->stOpalIvpsAttr.arrChnAttr[iChn].tFramerate.fSrc;
        AX_F32 fDstFps = pEle->stOpalIvpsAttr.arrChnAttr[iChn].tFramerate.fDst == -1 ? fFrameRate : pEle->stOpalIvpsAttr.arrChnAttr[iChn].tFramerate.fDst;
        if (0 != AX_OPAL_HAL_IVPS_SetFps(nGrpId, iChn, fSrcFps, fDstFps)) {
            return AX_ERR_OPAL_GENERIC;
        }
    }

    nRet = AX_OPAL_HAL_IVPS_Start(nGrpId, &pEle->stOpalIvpsAttr);
    if (nRet != 0) {
        return -1;
    }

    /* create ivps get frame thread */
    if (pEle->stEleIvpsAttr.pThreadGetStream == AX_NULL) {
        pEle->stEleIvpsAttr.pThreadGetStream = AX_OPAL_CreateThread(GetFrameThread, pEle);
        AX_OPAL_StartThread(pEle->stEleIvpsAttr.pThreadGetStream);
    }

    pEle->stBase.bStart = AX_TRUE;
    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEIVPS_Stop(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEIVPS_T* pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;

    if (pEle->stEleIvpsAttr.pThreadGetStream != AX_NULL) {
        AX_OPAL_StopThread(pEle->stEleIvpsAttr.pThreadGetStream);
        AX_OPAL_DestroyThread(pEle->stEleIvpsAttr.pThreadGetStream);
        pEle->stEleIvpsAttr.pThreadGetStream = AX_NULL;
    }

    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = AX_NULL;
    AX_OPAL_GRP_ATTR_T *pGrpAttr = &pEle->stBase.stAttr.arrGrpAttr[0];
    AX_S32 nGrpId = pGrpAttr->nGrpId;
    for (AX_S32 iChn = 0; iChn < pGrpAttr->nChnCnt; ++iChn) {
        for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
            pOsd = &pEle->stEleIvpsAttr.arrChnOsdAttr[iChn][iOsd];
            if (pOsd->nOsdHandle != AX_IVPS_INVALID_REGION_HANDLE) {
                if (pEle->stEleIvpsAttr.arrThreadTimeParam[iChn].pTimeThread != AX_NULL) {
                    AX_OPAL_StopThread(pEle->stEleIvpsAttr.arrThreadTimeParam[iChn].pTimeThread);
                    AX_OPAL_DestroyThread(pEle->stEleIvpsAttr.arrThreadTimeParam[iChn].pTimeThread);
                    pEle->stEleIvpsAttr.arrThreadTimeParam[iChn].pTimeThread = AX_NULL;
                }
                nRet = AXOP_HAL_IVPS_DestroyOsd(nGrpId, iChn, pOsd->nOsdHandle, &(pOsd->stOsdAttr));
            }
        }
        if (pEle->stEleIvpsAttr.arrThreadRectParam[iChn][0].pRectThread != AX_NULL) {
            AX_OPAL_StopThread(pEle->stEleIvpsAttr.arrThreadRectParam[iChn][0].pRectThread);
            AX_OPAL_DestroyThread(pEle->stEleIvpsAttr.arrThreadRectParam[iChn][0].pRectThread);
            pEle->stEleIvpsAttr.arrThreadRectParam[iChn][0].pRectThread = AX_NULL;
        }
        pOsd = &pEle->stEleIvpsAttr.arrChnOsdRect[iChn][0];
        if (pOsd->nOsdHandle != AX_IVPS_INVALID_REGION_HANDLE) {
            nRet = AXOP_HAL_IVPS_DestroyOsd(nGrpId, iChn, pOsd->nOsdHandle, &(pOsd->stOsdAttr));
        }

        if (pEle->stEleIvpsAttr.arrThreadRectParam[iChn][1].pRectThread != AX_NULL) {
            AX_OPAL_StopThread(pEle->stEleIvpsAttr.arrThreadRectParam[iChn][1].pRectThread);
            AX_OPAL_DestroyThread(pEle->stEleIvpsAttr.arrThreadRectParam[iChn][1].pRectThread);
            pEle->stEleIvpsAttr.arrThreadRectParam[iChn][1].pRectThread = AX_NULL;
        }
        pOsd = &pEle->stEleIvpsAttr.arrChnOsdRect[iChn][1];
        if (pOsd->nOsdHandle != AX_IVPS_INVALID_REGION_HANDLE) {
            nRet = AXOP_HAL_IVPS_DestroyOsd(nGrpId, iChn, pOsd->nOsdHandle, &(pOsd->stOsdAttr));
        }
    }

    for (AX_S32 iChn = 0; iChn < AX_OPAL_MAX_CHN_CNT; ++iChn) {
        for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
            pEle->stEleIvpsAttr.arrChnOsdAttr[iChn][iOsd].nOsdHandle = -1;
        }
        pEle->stEleIvpsAttr.arrChnOsdRect[iChn][0].nOsdHandle = -1;
        pEle->stEleIvpsAttr.arrChnOsdRect[iChn][1].nOsdHandle = -1;
    }

    nRet = AX_OPAL_HAL_IVPS_Stop(nGrpId);
    if (nRet != 0) {
        return -1;
    }

    nRet = AX_OPAL_HAL_IVPS_Close(nGrpId);
    if (nRet != 0) {
        return -1;
    }

    pEle->stBase.bStart = AX_FALSE;
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 create_osdhdl(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_OSD_CREATE_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    AX_OPAL_VIDEO_OSD_CREATE_T *pData  = pPorcessData->pData;

    /* verify osd number */
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = AX_NULL;
    for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
        pOsd = &(pEle->stEleIvpsAttr.arrChnOsdAttr[nChnId][iOsd]);
        if (pOsd->bInit == AX_FALSE) {
            break;
        }
    }
    if (pOsd == AX_NULL) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* if started, create osd handle */
    if (pEle->stBase.bStart) {
        pOsd->nOsdHandle = AXOP_HAL_IVPS_CreateOsd(nGrpId, nChnId, &(pOsd->stOsdAttr));
        if (pOsd->nOsdHandle == -1) {
            return AX_ERR_OPAL_GENERIC;
        }
    }

    /* save osd attribute */
    pData->OsdHandle = &(pOsd->nOsdHandle);
    pOsd->bInit = AX_TRUE;
    memcpy(&(pOsd->stOsdAttr), pData->pstAttr, sizeof(AX_OPAL_OSD_ATTR_T));

    return nRet;
}

static AX_S32 destroy_osdhdl(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_OSD_DESTROY_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }


    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    AX_OPAL_VIDEO_OSD_DESTROY_T *pData  = pPorcessData->pData;

    /* get osd handle */
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = AX_NULL;
    for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
        pOsd = &(pEle->stEleIvpsAttr.arrChnOsdAttr[nChnId][iOsd]);
        if (pOsd->bInit == AX_TRUE && &(pOsd->nOsdHandle) == pData->OsdHandle) {
            break;
        }
    }
    if (pOsd == AX_NULL) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* if started, destroy osd handle */
    if (pEle->stBase.bStart) {
        if (pOsd->nOsdHandle != -1) {
            nRet = AXOP_HAL_IVPS_DestroyOsd(nGrpId, nChnId, pOsd->nOsdHandle, &(pOsd->stOsdAttr));
            if (0 != nRet) {
                return AX_ERR_OPAL_GENERIC;
            }
        }
    }

    /* reset attribute */
    pOsd->nOsdHandle = -1;
    pOsd->bInit = AX_FALSE;
    memset(&(pOsd->stOsdAttr), 0x0, sizeof(AX_OPAL_OSD_ATTR_T));

    return nRet;
}

static AX_S32 update_osd(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    /* Verify */
    /* Step 1. verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* Step 2. verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_OSD_UPDATE_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* Query IDs */
    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* Process */
    AX_OPAL_VIDEO_OSD_UPDATE_T *pData  = (AX_OPAL_VIDEO_OSD_UPDATE_T*)pPorcessData->pData;
    /* Step 1. get osd object */
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = AX_NULL;
    for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
        pOsd = &(pEle->stEleIvpsAttr.arrChnOsdAttr[nChnId][iOsd]);
        if (pOsd->bInit == AX_TRUE && &(pOsd->nOsdHandle) == pData->OsdHandle) {
            break;
        }
    }
    if (pOsd == AX_NULL) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* Step 2. verify osd type match */
    if (pOsd->stOsdAttr.eType != pData->pstAttr->eType) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* Step 3. update osd */
    memcpy(&(pOsd->stOsdAttr), pData->pstAttr, sizeof(AX_OPAL_OSD_ATTR_T));
    nRet = UpdateRgn((AX_OPAL_MAL_ELE_HANDLE)pEle, nGrpId, nChnId, pOsd);

    return nRet;
}

static AX_S32 draw_rect(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_OSD_DRAWRECT_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_OSD_DRAWRECT_T *pData  = (AX_OPAL_VIDEO_OSD_DRAWRECT_T*)pPorcessData->pData;

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* index = 0: AX_IVPS_RGN_TYPE_RECT */
    const AX_U32 nOsdIndex = 0;
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = &(pEle->stEleIvpsAttr.arrChnOsdRect[nChnId][nOsdIndex]);
    if (pOsd->nOsdHandle == -1) {
        pOsd->nOsdHandle = AXOP_HAL_IVPS_CreateOsd(nGrpId, nChnId, &(pOsd->stOsdAttr));
        if (pOsd->nOsdHandle == -1) {
            return AX_ERR_OPAL_GENERIC;
        }

        if (pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].pRectThread == AX_NULL) {
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nGrpId = nGrpId;
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nChnId = nChnId;
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].pChnOsd = pOsd;
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].elehdl = self;
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].pRectThread = AX_OPAL_CreateThread(RectThreadProc, &pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex]);
            AX_OPAL_StartThread(pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].pRectThread);
        }
    }

    AX_IVPS_CHN_ATTR_T tChnAttr;
    memset(&tChnAttr, 0x0, sizeof(AX_IVPS_CHN_ATTR_T));
    nRet = AX_IVPS_GetChnAttr(nGrpId, nChnId, 0, &tChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_GetChnAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        return AX_ERR_OPAL_GENERIC;
    }

    for (AX_U32 i = 0; i < pData->nRectSize && pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nRectNum < AX_IVPS_REGION_MAX_DISP_NUM; i ++) {
        AX_U32 nRectNum = pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nRectNum;
        AX_IVPS_RGN_POLYGON_T *pstRect = &pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].stRects[nRectNum];

        memset(pstRect, 0x00, sizeof(AX_IVPS_RGN_POLYGON_T));
        pstRect->nLineWidth = pData->nLineWidth;
        pstRect->nColor = (AX_U32)(pData->nARGB & 0xFFFFFF);
        pstRect->nAlpha = (AX_U8)((pData->nARGB & 0xFF000000) >> 24) == 0 ? 0xFF : (AX_U8)((pData->nARGB & 0xFF000000) >> 24);
        pstRect->bSolid = AX_TRUE;
        pstRect->tRect.nX = pData->pstRects[i].fX * tChnAttr.nDstPicWidth;
        pstRect->tRect.nY = pData->pstRects[i].fY * tChnAttr.nDstPicHeight;
        pstRect->tRect.nW = pData->pstRects[i].fW * tChnAttr.nDstPicWidth;
        pstRect->tRect.nH = pData->pstRects[i].fH * tChnAttr.nDstPicHeight;

        pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nRectNum ++;
    }

    return nRet;
}

static AX_S32 draw_polygon(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_OSD_DRAWPOLYGON_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_OSD_DRAWPOLYGON_T *pData  = (AX_OPAL_VIDEO_OSD_DRAWPOLYGON_T*)pPorcessData->pData;

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* index = 0: AX_IVPS_RGN_TYPE_RECT */
    const AX_U32 nOsdIndex = 0;
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = &(pEle->stEleIvpsAttr.arrChnOsdRect[nChnId][nOsdIndex]);
    if (pOsd->nOsdHandle == -1) {
        pOsd->nOsdHandle = AXOP_HAL_IVPS_CreateOsd(nGrpId, nChnId, &(pOsd->stOsdAttr));
        if (pOsd->nOsdHandle == -1) {
            return AX_ERR_OPAL_GENERIC;
        }

        if (pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].pRectThread == AX_NULL) {
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nGrpId = nGrpId;
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nChnId = nChnId;
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].pChnOsd = pOsd;
            pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].pRectThread = AX_OPAL_CreateThread(RectThreadProc, &pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex]);
            AX_OPAL_StartThread(pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].pRectThread);
        }
    }

    AX_IVPS_CHN_ATTR_T tChnAttr;
    memset(&tChnAttr, 0x0, sizeof(AX_IVPS_CHN_ATTR_T));
    nRet = AX_IVPS_GetChnAttr(nGrpId, nChnId, 0, &tChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_GetChnAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        return AX_ERR_OPAL_GENERIC;
    }

    for (AX_U32 i = 0; i < pData->nPolygonSize && pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nPolygonNum < AX_IVPS_REGION_MAX_DISP_NUM; i ++) {
        AX_U32 nPolygonNum = pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nPolygonNum;
        AX_IVPS_RGN_POLYGON_T *pstPolygon = &pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].stPolygons[nPolygonNum];

        memset(pstPolygon, 0x00, sizeof(AX_IVPS_RGN_POLYGON_T));
        pstPolygon->nLineWidth = pData->nLineWidth;
        pstPolygon->nColor = (AX_U32)(pData->nARGB & 0xFFFFFF);
        pstPolygon->nAlpha = (AX_U8)((pData->nARGB & 0xFF000000) >> 24) == 0 ? 0xFF : (AX_U8)((pData->nARGB & 0xFF000000) >> 24);
        pstPolygon->bSolid = AX_TRUE;
        pstPolygon->nPointNum = pData->pstPolygons[i].nPointNum;

        for (AX_U32 j = 0; j < AX_IVPS_MAX_POLYGON_POINT_NUM && j < pData->pstPolygons[i].nPointNum; j ++) {
            pstPolygon->tPTs[j].nX = pData->pstPolygons[i].stPoints[j].fX * tChnAttr.nDstPicWidth;
            pstPolygon->tPTs[j].nY = pData->pstPolygons[i].stPoints[j].fY * tChnAttr.nDstPicHeight;
        }

        pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nPolygonNum ++;
    }

    return nRet;
}

static AX_S32 clear_rect(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* index = 0: AX_IVPS_RGN_TYPE_POLYGON */
    const AX_U32 nOsdIndex = 0;
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = &(pEle->stEleIvpsAttr.arrChnOsdRect[nChnId][nOsdIndex]);
    if (pOsd->nOsdHandle == -1) {
        return AX_ERR_OPAL_GENERIC;
    }

    pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nRectNum = 0;
    memset(pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].stRects, 0x00, sizeof(pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].stRects));

    return nRet;
}

static AX_S32 clear_polygon(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* index = 0: AX_IVPS_RGN_TYPE_POLYGON */
    const AX_U32 nOsdIndex = 0;
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = &(pEle->stEleIvpsAttr.arrChnOsdRect[nChnId][nOsdIndex]);
    if (pOsd->nOsdHandle == -1) {
        return AX_ERR_OPAL_GENERIC;
    }

    pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].nPolygonNum = 0;
    memset(pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].stPolygons, 0x00, sizeof(pEle->stEleIvpsAttr.arrThreadRectParam[nChnId][nOsdIndex].stPolygons));

    return nRet;
}

static AX_S32 disable(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_S32 nGrpId = pEle->stBase.stAttr.arrGrpAttr[0].nGrpId;
    for (AX_S32 iChn = 0; iChn < pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt; ++iChn) {
        if (pPorcessData->nUniChnId == -1) {
            AX_OPAL_HAL_IVPS_DisableChn(nGrpId, pEle->stBase.stAttr.arrGrpAttr[0].nUniChnId[iChn]);
        }
        else {
            if (pEle->stBase.stAttr.arrGrpAttr[0].nUniChnId[iChn] == pPorcessData->nUniChnId) {
                AX_OPAL_HAL_IVPS_DisableChn(nGrpId, pEle->stBase.stAttr.arrGrpAttr[0].nUniChnId[iChn]);
                break;
            }
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 enable(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_S32 nGrpId = pEle->stBase.stAttr.arrGrpAttr[0].nGrpId;
    for (AX_S32 iChn = 0; iChn < pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt; ++iChn) {
        if (pPorcessData->nUniChnId == -1) {
            AX_OPAL_HAL_IVPS_EnableChn(nGrpId, pEle->stBase.stAttr.arrGrpAttr[0].nUniChnId[iChn]);
        }
        else {
            if (pEle->stBase.stAttr.arrGrpAttr[0].nUniChnId[iChn] == pPorcessData->nUniChnId) {
                AX_OPAL_HAL_IVPS_EnableChn(nGrpId, pEle->stBase.stAttr.arrGrpAttr[0].nUniChnId[iChn]);
                break;
            }
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 set_fps(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_SNS_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_F32 fFrameRate = ((AX_OPAL_VIDEO_SNS_ATTR_T*)pPorcessData->pData)->fFrameRate;

    AX_OPAL_GRP_ATTR_T *pGrpAttr = &pEle->stBase.stAttr.arrGrpAttr[0];
    AX_S32 nGrpId = pGrpAttr->nGrpId;
    for (AX_S32 iChn = 0; iChn < pGrpAttr->nChnCnt; ++iChn) {
        AX_F32 fSrcFps = pEle->stOpalIvpsAttr.arrChnAttr[iChn].tFramerate.fSrc == -1 ? fFrameRate : pEle->stOpalIvpsAttr.arrChnAttr[iChn].tFramerate.fSrc;
        AX_F32 fDstFps = pEle->stOpalIvpsAttr.arrChnAttr[iChn].tFramerate.fDst == -1 ? fFrameRate : pEle->stOpalIvpsAttr.arrChnAttr[iChn].tFramerate.fDst;
        if (0 != AX_OPAL_HAL_IVPS_SetFps(nGrpId, iChn, fSrcFps, fDstFps)) {
            return AX_ERR_OPAL_GENERIC;
        }
    }

    return AX_SUCCESS;
}

static AX_S32 set_rotation(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    LOG_M_D(LOG_TAG, "+++");

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* for only one channel of IVPS */
    /* verify grp and chn */
    if (pPorcessData->nUniGrpId == -1) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_SNS_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_OPAL_SNS_ROTATION_E eRotation = ((AX_OPAL_VIDEO_SNS_ATTR_T*)pPorcessData->pData)->eRotation;

    AX_OPAL_GRP_ATTR_T *pGrpAttr = &pEle->stBase.stAttr.arrGrpAttr[0];
    AX_S32 nGrpId = pGrpAttr->nGrpId;
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = AX_NULL;
    for (AX_S32 iChn = 0; iChn < pGrpAttr->nChnCnt; ++iChn) {
        if (!pEle->stOpalIvpsAttr.bMaskEnable) {
            if (0 != AX_OPAL_HAL_IVPS_SetRotation(nGrpId, iChn, eRotation)) {
                return AX_ERR_OPAL_GENERIC;
            }
        }

        for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
            pOsd = &pEle->stEleIvpsAttr.arrChnOsdAttr[iChn][iOsd];
            if (pOsd->bInit == AX_TRUE && pOsd->nOsdHandle != AX_IVPS_INVALID_REGION_HANDLE) {
                if (0 != UpdateRgn(pEle, nGrpId, iChn, pOsd)) {
                    return AX_ERR_OPAL_GENERIC;
                }
            }
        }
    }
    LOG_M_D(LOG_TAG, "---");
    return AX_SUCCESS;
}

static AX_S32 set_resolution(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_CHN_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_CHN_ATTR_T *pChnAttr = (AX_OPAL_VIDEO_CHN_ATTR_T *)pPorcessData->pData;

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_OPAL_VIDEO_SNS_ATTR_T *pSnsAttr = AX_OPAL_MAL_GetSnsAttr(self);
    nRet = AX_OPAL_HAL_IVPS_SetResolution(nGrpId, nChnId, pSnsAttr->eRotation, pChnAttr->nWidth, pChnAttr->nHeight);

    /* update osd */
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pOsd = AX_NULL;
    for (AX_S32 iOsd = 0; iOsd < AX_OPAL_MAX_CHN_OSD_CNT; ++iOsd) {
        pOsd = &pEle->stEleIvpsAttr.arrChnOsdAttr[nChnId][iOsd];
        if (pOsd->bInit == AX_TRUE && pOsd->nOsdHandle != AX_IVPS_INVALID_REGION_HANDLE) {
            if (0 != UpdateRgn(pEle, nGrpId, nChnId, pOsd)) {
                return AX_ERR_OPAL_GENERIC;
            }
        }
    }

    return nRet;
}

static AX_S32 snapshot(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_SNAPSHOT_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    nRet = AX_OPAL_HAL_IVPS_SnapShot(nGrpId, nChnId, (AX_OPAL_VIDEO_SNAPSHOT_T*)pPorcessData->pData);
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 captureframe(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    /* verify ivps started */
    AX_OPAL_MAL_ELEIVPS_T *pEle = (AX_OPAL_MAL_ELEIVPS_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_CAPTUREFRAME_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* get group id and chnanel id */
    AX_S32 nGrpId = AX_OPAL_MAL_ELE_GetGrpId(self);
    AX_S32 nChnId = AX_OPAL_MAL_ELE_GetChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);
    if (-1 == nGrpId || -1 == nChnId) {
        return AX_ERR_OPAL_GENERIC;
    }

    nRet = AX_OPAL_HAL_IVPS_CaptureFrame(nGrpId, nChnId, (AX_OPAL_VIDEO_CAPTUREFRAME_T*)pPorcessData->pData);
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}


AX_S32 AX_OPAL_MAL_ELEIVPS_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    if (!AX_OPAL_MAL_ELE_CheckUniGrpChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId)) {
        return AX_ERR_OPAL_UNEXIST;
    }

    switch (pPorcessData->eSubCmdType) {
        case AX_OPAL_SUBCMD_PREPROC:
            nRet = disable(self, pPorcessData);
            break;
        case AX_OPAL_SUBCMD_POSTPROC:
            nRet = enable(self, pPorcessData);
            break;
        default:
            break;
    }
    switch (pPorcessData->eMainCmdType) {
        case AX_OPAL_MAINCMD_VIDEO_SETSNSATTR:
            switch (pPorcessData->eSubCmdType) {
                case AX_OPAL_SUBCMD_SNSATTR_FPS:
                    set_fps(self, pPorcessData);
                    break;
                case AX_OPAL_SUBCMD_SNSATTR_ROTATION:
                    nRet = set_rotation(self, pPorcessData);
                    break;
                default:
                    LOG_M_W(LOG_TAG, "unsupported main command type.");
                    break;
            }
            break;
        case AX_OPAL_MAINCMD_VIDEO_SETCHNATTR:
            switch (pPorcessData->eSubCmdType) {
                case AX_OPAL_SUBCMD_CHNATTR_RESOLUTION:
                    nRet = set_resolution(self, pPorcessData);
                    break;
                default:
                    LOG_M_W(LOG_TAG, "unsupported main command type.");
                    break;
            }
            break;
        case AX_OPAL_MAINCMD_VIDEO_OSDCREATE:
            nRet = create_osdhdl(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_OSDDESTROY:
            nRet = destroy_osdhdl(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_OSDUPDATE:
            nRet = update_osd(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_OSDDRAWRECT:
            nRet = draw_rect(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_OSDCLEARRECT:
            nRet = clear_rect(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_OSDDRAWPOLYGON:
            nRet = draw_polygon(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_OSDCLEARPOLYGON:
            nRet = clear_polygon(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_SNAPSHOT:
            nRet = snapshot(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_CAPTUREFRAME:
            nRet = captureframe(self, pPorcessData);
            break;
        default:
            LOG_M_W(LOG_TAG, "unsupported main command type.");
            break;
    }
    return nRet;
}
