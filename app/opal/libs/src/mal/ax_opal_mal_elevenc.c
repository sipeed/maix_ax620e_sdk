/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_elevenc.h"
#include "ax_opal_mal_utils.h"

#include "ax_opal_hal_venc.h"
#include "ax_opal_log.h"
#include "ax_opal_utils.h"
#include "ax_opal_mal_element.h"

#define LOG_TAG ("ELEVENC")

#include <string.h>

// interface vtable
AX_OPAL_MAL_ELE_Interface venc_interface = {
    .start = AX_OPAL_MAL_ELEVENC_Start,
    .stop = AX_OPAL_MAL_ELEVENC_Stop,
    .event_proc = AX_OPAL_MAL_ELEVENC_Process,
};

static AX_S32 get_venc_chnid(AX_OPAL_MAL_ELEVENC_T* pEle) {

    if (pEle->stBase.stAttr.nGrpCnt != 1 && pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt != 1) {
        LOG_M_E(LOG_TAG, "invalid element attr in and out");
        return -1;
    }

    return pEle->stBase.stAttr.arrGrpAttr[0].nChnId[0];
}

static AX_VOID VencGetStreamProc(AX_VOID *arg) {
    LOG_M_D(LOG_TAG, "+++");

    AX_OPAL_THREAD_T *pThread = (AX_OPAL_THREAD_T*)arg;
    if (!pThread) {
        return;
    }

    AX_OPAL_MAL_ELEVENC_T* pEle = (AX_OPAL_MAL_ELEVENC_T*)pThread->arg;
    if (pEle->stBase.stAttr.nGrpCnt != 1 && pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt != 1) {
        return;
    }
    AX_S32 nVencChn = pEle->stBase.stAttr.arrGrpAttr[0].nChnId[0];
    AX_S32 nUniGrpId = pEle->stPktThreadAttr.nUniGrpId;
    AX_S32 nUniChnId = pEle->stPktThreadAttr.nUniChnId;
    AX_OPAL_VIDEO_PKT_CALLBACK_T *pstOpalVideoPktCb = &pEle->stPktThreadAttr.stVideoPktCb;

    AX_S32 nRet = 0;

    AX_CHAR thread_name[16];
    sprintf(thread_name, "venc_%d", nVencChn);
    prctl(PR_SET_NAME, thread_name);

    AX_VENC_STREAM_T stStream = {0};
	while (pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING) {
        nRet = AX_VENC_GetStream(nVencChn, &stStream, -1);
        if (AX_SUCCESS == nRet) {
            if (pstOpalVideoPktCb->callback) {
                AX_OPAL_VIDEO_PKT_T stPkt = {0};
                stPkt.bIFrame = (AX_VENC_INTRA_FRAME == stStream.stPack.enCodingType || PT_MJPEG == stStream.stPack.enType) ? AX_TRUE : AX_FALSE;
                stPkt.nSnsId = nUniGrpId;
                stPkt.nChnId = nUniChnId;
                stPkt.eType = stStream.stPack.enType;
                stPkt.eNaluType = AX_OPAL_HAL_VENC_CvtNaluType(stStream.stPack.enType, stStream.stPack.stNaluInfo[0].unNaluType, stPkt.bIFrame);
                stPkt.pData = stStream.stPack.pu8Addr;
                stPkt.nDataSize = stStream.stPack.u32Len;
                stPkt.u64Pts = stStream.stPack.u64PTS;
                stPkt.pPrivateData = NULL;
                pstOpalVideoPktCb->callback(nUniGrpId, nUniChnId, &stPkt);
            }

            nRet = AX_VENC_ReleaseStream(nVencChn, &stStream);
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AX_VENC_ReleaseStream failed, ret=0x%x", nRet);
            }
		} else if (AX_ERR_VENC_UNEXIST == nRet) {
            LOG_M_W(LOG_TAG, "AX_VENC_GetStream return AX_ERR_VENC_UNEXIST, ret=0x%x", nRet);
		 	break;
		} else if (AX_ERR_VENC_FLOW_END == nRet) {
            LOG_M_W(LOG_TAG, "AX_VENC_GetStream return AX_ERR_VENC_FLOW_END, ret=0x%x", nRet);
		 	break;
        }
	}

    LOG_M_D(LOG_TAG, "---");
}

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELEVENC_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr) {
    LOG_M_D(LOG_TAG, "+++");

    AX_OPAL_MAL_ELE_HANDLE handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_ELEVENC_T));
    if (handle == AX_NULL) {
        LOG_M_E(LOG_TAG, "malloc failed.");
        return AX_NULL;
    }

    AX_OPAL_MAL_ELEVENC_T* pEle = (AX_OPAL_MAL_ELEVENC_T*)handle;
    memset(pEle, 0x0, sizeof(AX_OPAL_MAL_ELEVENC_T));
    pEle->stBase.nId = pEleAttr->nId;
    pEle->stBase.pParent = parent;
    pEle->stBase.vTable = &venc_interface;
    memcpy(&pEle->stBase.stAttr, pEleAttr, sizeof(AX_OPAL_ELEMENT_ATTR_T));
    AX_OPAL_MAL_ELE_MappingGrpChn(handle);

    AX_S32 nVencChn = get_venc_chnid(pEle);
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pEle->stBase.pParent;
    pEle->pstVideoChnAttr = AX_OPAL_MAL_GetChnAttr(handle, 0, nVencChn);
    if (pEle->pstVideoChnAttr->nWidth == -1) {
        pEle->pstVideoChnAttr->nWidth = pSubPipeline->stAttr.nInWidth;
        pEle->pstVideoChnAttr->nHeight = pSubPipeline->stAttr.nInHeight;
        pEle->pstVideoChnAttr->nMaxWidth = pSubPipeline->stAttr.nInWidth;
        pEle->pstVideoChnAttr->nMaxHeight = pSubPipeline->stAttr.nInHeight;
    }

    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = AX_OPAL_MAL_GetSnsAttr(pEle);
    pEle->pstVideoChnAttr->nFramerate = pstSnsAttr->fFrameRate;

    /* parse attr */
#if 0
    AX_CHAR *cCfgIniPath = "./venc.ini";
    AX_S32 nRet = AX_OPAL_HAL_VENC_Parse(cCfgIniPath, &pEle->stAttr);
    if (0 != nRet) {
        AX_OPAL_FREE(pEle);
        return AX_NULL;
    }
#endif

    LOG_M_D(LOG_TAG, "---");
    return handle;
}

AX_S32 AX_OPAL_MAL_ELEVENC_Destroy(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }
    AX_OPAL_FREE(self);

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEVENC_Start(AX_OPAL_MAL_ELE_HANDLE self) {
    LOG_M_D(LOG_TAG, "+++");
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        LOG_M_E(LOG_TAG, "invalid element handle");
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEVENC_T* pEle = (AX_OPAL_MAL_ELEVENC_T*)self;

    if (pEle->stBase.stAttr.nGrpCnt != 1 && pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt != 1) {
        LOG_M_E(LOG_TAG, "invalid element attr in and out");
        return -1;
    }

    AX_S32 nVencChn = get_venc_chnid(pEle);
    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = AX_OPAL_MAL_GetSnsAttr(pEle);
    nRet = AX_OPAL_HAL_VENC_CreateChn(nVencChn, pstSnsAttr->eRotation, pEle->pstVideoChnAttr);
    if (nRet != 0) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VENC_CreateChn failed.");
        return -1;
    }

    nRet = AX_OPAL_HAL_VENC_StartRecv(nVencChn);
    if (nRet != 0) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VENC_StartRecv failed.");
        return -1;
    }

    if (pEle->stPktThreadAttr.pVideoPktThread == AX_NULL) {
        pEle->stPktThreadAttr.pVideoPktThread = AX_OPAL_CreateThread(VencGetStreamProc, pEle);
        AX_OPAL_StartThread(pEle->stPktThreadAttr.pVideoPktThread);
    }

    pEle->stBase.bStart = AX_TRUE;
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEVENC_Stop(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEVENC_T* pEle = (AX_OPAL_MAL_ELEVENC_T*)self;
    if (pEle->stBase.stAttr.nGrpCnt != 1 && pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt != 1) {
        return -1;
    }

    AX_S32 nVencChn = get_venc_chnid(pEle);
    nRet = AX_OPAL_HAL_VENC_StopRecv(nVencChn);
    if (nRet != 0) {
        return nRet;
    }

    nRet = AX_OPAL_HAL_VENC_DestroyChn(nVencChn);
    if (nRet != 0) {
        return nRet;
    }

    if (pEle->stPktThreadAttr.pVideoPktThread != AX_NULL) {
        AX_OPAL_StopThread(pEle->stPktThreadAttr.pVideoPktThread);
        AX_OPAL_DestroyThread(pEle->stPktThreadAttr.pVideoPktThread);
        pEle->stPktThreadAttr.pVideoPktThread = AX_NULL;
    }

    pEle->stBase.bStart = AX_FALSE;

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 disable(AX_OPAL_MAL_ELEVENC_T* pEle, AX_S32 nVencChn, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    nRet = AX_OPAL_HAL_VENC_StopRecv(nVencChn);

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 enable(AX_OPAL_MAL_ELEVENC_T* pEle, AX_S32 nVencChn, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    nRet = AX_OPAL_HAL_VENC_StartRecv(nVencChn);
    if (nRet != 0) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VENC_StartRecv failed.");
        return -1;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 set_rotation(AX_OPAL_MAL_ELEVENC_T* pEle, AX_S32 nVencChn, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_SNS_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = (AX_OPAL_VIDEO_SNS_ATTR_T*)pPorcessData->pData;
    nRet = AX_OPAL_HAL_VENC_SetResolution(nVencChn, pstSnsAttr->eRotation, pEle->pstVideoChnAttr->nWidth, pEle->pstVideoChnAttr->nHeight);

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 set_sns_fps(AX_OPAL_MAL_ELEVENC_T* pEle, AX_S32 nVencChn, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_SNS_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = (AX_OPAL_VIDEO_SNS_ATTR_T*)pPorcessData->pData;
    nRet = AX_OPAL_HAL_VENC_SetFps(nVencChn, pstSnsAttr->fFrameRate, pstSnsAttr->fFrameRate);

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 set_enctype(AX_OPAL_MAL_ELEVENC_T* pEle, AX_S32 nVencChn, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_CHN_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr = (AX_OPAL_VIDEO_CHN_ATTR_T*)pPorcessData->pData;

    nRet = AX_OPAL_HAL_VENC_StopRecv(nVencChn);
    if (0 != nRet) {
        return nRet;
    }

    AX_OPAL_VIDEO_SVC_PARAM_T stSvcParam = {0};
    nRet = AX_OPAL_HAL_VENC_GetSvcParam(nVencChn, &stSvcParam);

    if (0 != nRet) {
        return nRet;
    }

    nRet = AX_OPAL_HAL_VENC_DestroyChn(nVencChn);
    if (0 != nRet) {
        return nRet;
    }

    if (pEle->stPktThreadAttr.pVideoPktThread != AX_NULL) {
        AX_OPAL_StopThread(pEle->stPktThreadAttr.pVideoPktThread);
        AX_OPAL_DestroyThread(pEle->stPktThreadAttr.pVideoPktThread);
        pEle->stPktThreadAttr.pVideoPktThread = AX_NULL;
    }

    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = AX_OPAL_MAL_GetSnsAttr(pEle);
    pstChnAttr->nFramerate = pstSnsAttr->fFrameRate;
    nRet = AX_OPAL_HAL_VENC_CreateChn(nVencChn, pstSnsAttr->eRotation, pstChnAttr);
    if (0 != nRet) {
        return nRet;
    }

    nRet = AX_OPAL_HAL_VENC_SetSvcParam(nVencChn, &stSvcParam);
    if (0 != nRet) {
        return nRet;
    }

    nRet = AX_OPAL_HAL_VENC_StartRecv(nVencChn);
    if (0 != nRet) {
        return nRet;
    }

    if (pEle->stPktThreadAttr.pVideoPktThread == AX_NULL) {
        pEle->stPktThreadAttr.pVideoPktThread = AX_OPAL_CreateThread(VencGetStreamProc, pEle);
        AX_OPAL_StartThread(pEle->stPktThreadAttr.pVideoPktThread);
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 set_resolution(AX_OPAL_MAL_ELEVENC_T* pEle, AX_S32 nVencChn, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_CHN_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr = (AX_OPAL_VIDEO_CHN_ATTR_T*)pPorcessData->pData;
    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = AX_OPAL_MAL_GetSnsAttr((AX_OPAL_MAL_ELE_HANDLE)pEle);

    nRet = AX_OPAL_HAL_VENC_SetResolution(nVencChn, pstSnsAttr->eRotation, pstChnAttr->nWidth, pstChnAttr->nHeight);

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 set_enccfg(AX_OPAL_MAL_ELEVENC_T* pEle, AX_S32 nVencChn, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_CHN_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr = (AX_OPAL_VIDEO_CHN_ATTR_T*)pPorcessData->pData;

    nRet = AX_OPAL_HAL_VENC_SetRcAttr(nVencChn, &pstChnAttr->stEncoderAttr);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 set_chn_fps(AX_OPAL_MAL_ELEVENC_T* pEle, AX_S32 nVencChn, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (pPorcessData->nDataSize != sizeof(AX_OPAL_VIDEO_CHN_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr = (AX_OPAL_VIDEO_CHN_ATTR_T*)pPorcessData->pData;

    nRet = AX_OPAL_HAL_VENC_SetFps(nVencChn, -1, (AX_F32)pstChnAttr->nFramerate);

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEVENC_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    if (self == AX_NULL || pPorcessData == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEVENC_T* pEle = (AX_OPAL_MAL_ELEVENC_T*)self;
    if (pEle->stBase.stAttr.nGrpCnt != 1 && pEle->stBase.stAttr.arrGrpAttr[0].nChnCnt != 1) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_S32 nVencChn = get_venc_chnid(pEle);
    AX_S32 nUniChnId = pEle->stBase.stAttr.arrGrpAttr[0].nUniChnId[0];
    if (pPorcessData->nUniChnId != -1 && nUniChnId != pPorcessData->nUniChnId) {
        return AX_SUCCESS;
    }

    switch (pPorcessData->eMainCmdType) {
        case AX_OPAL_MAINCMD_VIDEO_SETSNSATTR:
            {
                AX_OPAL_VIDEO_CHN_ATTR_T *pstVideoChnAttr = AX_OPAL_MAL_GetChnAttr(self, 0, nVencChn);
                if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_PREPROC) {
                    if (pstVideoChnAttr->bEnable) {
                        nRet = disable(pEle, nVencChn, pPorcessData);
                    }
                }
                else if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_POSTPROC) {
                    if (pstVideoChnAttr->bEnable) {
                        nRet = enable(pEle, nVencChn, pPorcessData);
                    }
                }
                else if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_SNSATTR_ROTATION) {
                    nRet = set_rotation(pEle, nVencChn, pPorcessData);
                } else if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_SNSATTR_FPS) {
                    nRet = set_sns_fps(pEle, nVencChn, pPorcessData);
                }
            }
            break;
        case AX_OPAL_MAINCMD_VIDEO_SETCHNATTR:
            if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_PREPROC) {
                nRet = disable(pEle, nVencChn, pPorcessData);
            }
            else if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_POSTPROC) {
                nRet = enable(pEle, nVencChn, pPorcessData);
            }
            else if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_CHNATTR_ENCTYPE) {
                nRet = set_enctype(pEle, nVencChn, pPorcessData);
            }
            else if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_CHNATTR_RESOLUTION) {
                nRet = set_resolution(pEle, nVencChn, pPorcessData);
            }
            else if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_CHNATTR_ENCCFG) {
                nRet = set_enccfg(pEle, nVencChn, pPorcessData);
            }
            else if (pPorcessData->eSubCmdType == AX_OPAL_SUBCMD_CHNATTR_FPS) {
                nRet = set_chn_fps(pEle, nVencChn, pPorcessData);
            }
            break;
        case AX_OPAL_MAINCMD_VIDEO_REGISTERPACKETCALLBACK:
            {
                AX_OPAL_MAL_ELEVENC_T* pEle = (AX_OPAL_MAL_ELEVENC_T*)self;
                memset(&pEle->stPktThreadAttr, 0x0, sizeof(AX_OPAL_VIDEO_PKT_THREAD_T));
                pEle->stPktThreadAttr.nUniGrpId = pPorcessData->nUniGrpId;
                pEle->stPktThreadAttr.nUniChnId = pPorcessData->nUniChnId;
                memcpy(&pEle->stPktThreadAttr.stVideoPktCb, pPorcessData->pData, pPorcessData->nDataSize);
            }
            break;
        case AX_OPAL_MAINCMD_VIDEO_UNREGISTERPACKETCALLBACK:
            {
                AX_OPAL_MAL_ELEVENC_T* pEle = (AX_OPAL_MAL_ELEVENC_T*)self;
                memset(&pEle->stPktThreadAttr, 0x0, sizeof(AX_OPAL_VIDEO_PKT_THREAD_T));
            }
            break;
        case AX_OPAL_MAINCMD_VIDEO_REQUESTIDR:
            nRet = AX_OPAL_HAL_VENC_RequestIDR(nVencChn);
            break;
        case AX_OPAL_MAINCMD_VIDEO_SETSVCPARAM:
            nRet = AX_OPAL_HAL_VENC_SetSvcParam(nVencChn, (const AX_OPAL_VIDEO_SVC_PARAM_T*)pPorcessData->pData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_GETSVCPARAM:
            nRet = AX_OPAL_HAL_VENC_GetSvcParam(nVencChn, (AX_OPAL_VIDEO_SVC_PARAM_T*)pPorcessData->pData);
            break;
        case AX_OPAL_MAINCMD_VIDEO_SETSVCREGION:
            nRet = AX_OPAL_HAL_VENC_SetSvcRegion(nVencChn, (AX_OPAL_VIDEO_SVC_REGION_T*)pPorcessData->pData);
            break;
        default:
            break;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}
