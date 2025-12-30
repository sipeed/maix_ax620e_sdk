/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>

#include "ax_opal_api.h"
#include "ax_opal_type.h"
#include "ax_timer.h"
#include "ax_log.h"
#include "ax_utils.h"
#include "config.h"
#include "ax_rtsp_api.h"
#include "web_server.h"
#include "ax_option.h"
#include "ax_mp4.h"

#define LOG_TAG "DEMO"

#define MAIN_TRIM_THRESHOLD (128*1024)

extern int malloc_trim(size_t pad);

AXOP_LOG_LEVEL_E g_opalapp_log_level = AXOP_LOG_ERROR;


AX_U8 g_opal_nSnsNum = 1;
DEMO_SNS_TYPE_E g_opal_nSnsType = DEMO_OS04A10;
// RTSP
AX_RTSP_HANDLE g_pRtsp = NULL;
AX_S32 g_opal_RtspChnList[AX_OPAL_SNS_ID_BUTT][AX_OPAL_VIDEO_CHN_BUTT];
// MP4
AX_MP4_HANDLE g_pMp4Handle[AX_OPAL_SNS_ID_BUTT] = {0};
pthread_mutex_t g_mtxMp4[AX_OPAL_SNS_ID_BUTT] = {0};

extern AX_OPAL_ATTR_T g_stOpalAttr;
extern DEMO_SNS_OSD_CONFIG_T g_stOsdCfg[AX_OPAL_SNS_ID_BUTT];
extern const AX_OPAL_VIDEO_SVC_PARAM_T g_stSvcParam[AX_OPAL_SNS_ID_BUTT];
//
static AX_BOOL g_Running = AX_FALSE;
static AX_S32 g_ExitCount = 0;
static AX_S32 g_nDelayExitTime = 0;

AX_CHAR g_strConfigPath[128] = "./config";

static const AX_CHAR* AX_OPAL_ALGO_GET_RESPIRATOR(AX_OPAL_ALGO_FACE_RESPIRATOR_TYPE_E eType) {
    if (eType == AX_OPAL_ALGO_FACE_RESPIRATOR_NONE) {
        return "no_respirator";
    } else if (eType == AX_OPAL_ALGO_FACE_RESPIRATOR_COMMON) {
        return "common";
    }

    return "unknown";
}

void exit_handler(int s) {
    LOG_M_C(LOG_TAG, "\n====================== Caught signal: SIGINT ======================");
    if (g_nDelayExitTime > 0 && g_ExitCount == 0) {
        LOG_M_C(LOG_TAG, "====================== Wait %d ms ======================", g_nDelayExitTime);
        OPAL_mSleep((AX_U32)g_nDelayExitTime);
    }

    g_Running = AX_FALSE;
    g_ExitCount++;
    if (g_ExitCount >= 3) {
        LOG_M_C(LOG_TAG, "====================== Force to exit ======================");
        _exit(1);
    }
}

void ignore_sig_pipe() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1) {
        perror("failed to ignore SIGPIPE, sigaction");
        exit(EXIT_FAILURE);
    }
}

static AX_VOID OPALDemo_VideoPacketCallback(AX_S32 nSnsId, AX_S32 nChnId, const AX_OPAL_VIDEO_PKT_T* pstPkt) {
    if (pstPkt) {
        FpsStatUpdate(nSnsId, nChnId);
        WS_SendPreviewData(nSnsId, nChnId, pstPkt->pData, pstPkt->nDataSize, pstPkt->u64Pts, pstPkt->bIFrame);

        if (-1 != g_opal_RtspChnList[nSnsId][nChnId]) {
            AX_RTSP_SendVideo(g_pRtsp, (AX_U32)g_opal_RtspChnList[nSnsId][nChnId], pstPkt->pData, pstPkt->nDataSize,
                                                   pstPkt->u64Pts, pstPkt->bIFrame);
        }

        pthread_mutex_lock(&g_mtxMp4[nSnsId]);
        if (g_pMp4Handle[nSnsId] && nChnId == VIDEO_CHN_VENC_MAIN_ID) {
            AX_Mp4_SaveVideo(g_pMp4Handle[nSnsId], pstPkt->pData, pstPkt->nDataSize, pstPkt->u64Pts, pstPkt->bIFrame);
        }
        pthread_mutex_unlock(&g_mtxMp4[nSnsId]);
    }
}

static AX_VOID OPALDemo_AudioPacketCallback(AX_S32 nChn, const AX_OPAL_AUDIO_PKT_T* pstPkt) {
    if (pstPkt) {
        // audio channel 0
        if (0 == nChn) {
            // web audio
            if (g_stOpalAttr.stAudioAttr.stCapAttr.bEnable) {
                WS_SendAudioData(pstPkt->pData, pstPkt->nDataSize, pstPkt->u64Pts);
            }

            // rtsp and mp4 audio
            if (g_stOpalAttr.stAudioAttr.stCapAttr.bEnable) {
                for (AX_U32 i = 0; i < g_opal_nSnsNum; i++) {
                    for (AX_U32 j = 0; j < AX_OPAL_VIDEO_CHN_BUTT; j++) {
                        if (-1 != g_opal_RtspChnList[i][j]) {
                            AX_RTSP_SendAudio(g_pRtsp, (AX_U32)g_opal_RtspChnList[i][j], pstPkt->pData, pstPkt->nDataSize, pstPkt->u64Pts);
                        }
                    }

                    pthread_mutex_lock(&g_mtxMp4[i]);
                    if (g_pMp4Handle[i]) {
                        AX_Mp4_SaveAudio(g_pMp4Handle[i], pstPkt->pData, pstPkt->nDataSize, pstPkt->u64Pts);
                    }
                    pthread_mutex_unlock(&g_mtxMp4[i]);
                }
            }
        }
    }
}

static AX_VOID OPALDemo_AudioPlayFileResultCallback(AX_S32 nChn, const AX_OPAL_AUDIO_PLAY_FILE_RESULT_T* pstResult) {
    if (pstResult) {
        LOG_M_C(LOG_TAG, "[%d] %s play complete, status: %d", nChn, pstResult->pstrFileName, pstResult->eStatus);
    }
}

// algo result callback
static AX_VOID OPALDemo_AlgoResultCallback(AX_S32 nSnsId, const AX_OPAL_ALGO_RESULT_T* pstResult) {
    if (pstResult) {
        // hvcfp
        if (pstResult->stHvcfpResult.bValid) {
            AX_OPAL_Video_OsdClearRect(nSnsId, AX_OPAL_VIDEO_CHN_0);
            AX_OPAL_Video_OsdClearRect(nSnsId, AX_OPAL_VIDEO_CHN_2);

            AX_OPAL_VIDEO_CHN_ATTR_T stAttrMain;
            AX_OPAL_Video_GetChnAttr(nSnsId, AX_OPAL_VIDEO_CHN_0, &stAttrMain);

            AX_OPAL_VIDEO_CHN_ATTR_T stAttrSub;
            AX_OPAL_Video_GetChnAttr(nSnsId, AX_OPAL_VIDEO_CHN_2, &stAttrSub);

            AX_U32 nCountList[AX_OPAL_ALGO_HVCFP_TYPE_BUTT] = {pstResult->stHvcfpResult.nBodySize, pstResult->stHvcfpResult.nVehicleSize,
                                                           pstResult->stHvcfpResult.nCycleSize, pstResult->stHvcfpResult.nFaceSize,
                                                           pstResult->stHvcfpResult.nPlateSize};
            AX_OPAL_ALGO_HVCFP_ITEM_T* pItems[AX_OPAL_ALGO_HVCFP_TYPE_BUTT] = {
                pstResult->stHvcfpResult.pstBodys, pstResult->stHvcfpResult.pstVehicles, pstResult->stHvcfpResult.pstCycles,
                pstResult->stHvcfpResult.pstFaces, pstResult->stHvcfpResult.pstPlates};
            AX_U32 nColorList[AX_OPAL_ALGO_HVCFP_TYPE_BUTT] = {WHITE, PURPLE, GREEN, YELLOW, RED};

            AX_OPAL_RECT_T rectMain[AX_OPAL_DEMO_HVCFP_RECT_NUM] = {0};
            AX_OPAL_RECT_T rectSub[AX_OPAL_DEMO_HVCFP_RECT_NUM] = {0};

            for (AX_U32 i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; ++i) {
                AX_U8 nRectIndex = 0;

                for (AX_U32 j = 0; j < nCountList[i]; ++j) {
                    if (nRectIndex < AX_OPAL_DEMO_HVCFP_RECT_NUM && (pItems[i][j].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_NEW ||
                                                                     pItems[i][j].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_UPDATE)) {
                        rectMain[nRectIndex].fX = pItems[i][j].stBox.fX;
                        rectMain[nRectIndex].fY = pItems[i][j].stBox.fY;
                        rectMain[nRectIndex].fW = pItems[i][j].stBox.fW;
                        rectMain[nRectIndex].fH = pItems[i][j].stBox.fH;

                        rectSub[nRectIndex].fX = pItems[i][j].stBox.fX;
                        rectSub[nRectIndex].fY = pItems[i][j].stBox.fY;
                        rectSub[nRectIndex].fW = pItems[i][j].stBox.fW;
                        rectSub[nRectIndex].fH = pItems[i][j].stBox.fH;
                        nRectIndex++;
                    }
                    if (pItems[i][j].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_SELECT) {
                        if (pItems[i][j].stImg.bExist && pItems[i][j].stImg.pData) {
                            JPEG_DATA_INFO_T tJpegInfo;
                            tJpegInfo.eType = (JPEG_TYPE_E)i;
                            if (tJpegInfo.eType == JPEG_TYPE_PLATE) {
                                if (strlen(pItems[i][j].stPlateAttr.szPlateCode) != 0) {
                                    strncpy(tJpegInfo.tPlateInfo.szNum, pItems[i][j].stPlateAttr.szPlateCode,
                                    sizeof(tJpegInfo.tPlateInfo.szNum) - 1);
                                } else {
                                    strncpy(tJpegInfo.tPlateInfo.szNum, "unknown",
                                    sizeof(tJpegInfo.tPlateInfo.szNum) - 1);
                                }
                                strncpy(tJpegInfo.tPlateInfo.szColor, "unknown", sizeof(tJpegInfo.tPlateInfo.szColor) - 1);
                            } else if (tJpegInfo.eType == JPEG_TYPE_FACE) {
                                tJpegInfo.tFaceInfo.nGender = pItems[i][j].stFaceAttr.nGender;
                                tJpegInfo.tFaceInfo.nAge = pItems[i][j].stFaceAttr.nAge;
                                strncpy(tJpegInfo.tFaceInfo.szMask, AX_OPAL_ALGO_GET_RESPIRATOR(pItems[i][j].stFaceAttr.eRespirator), sizeof(tJpegInfo.tFaceInfo.szMask) - 1);
                                strncpy(tJpegInfo.tFaceInfo.szInfo, "unknown", sizeof(tJpegInfo.tFaceInfo.szInfo) - 1);
                            } else {
                                tJpegInfo.tCaptureInfo.tHeaderInfo.nSnsSrc = nSnsId;
                                tJpegInfo.tCaptureInfo.tHeaderInfo.nChannel = 0;
                                tJpegInfo.tCaptureInfo.tHeaderInfo.nWidth = pItems[i][j].stImg.nWidth;
                                tJpegInfo.tCaptureInfo.tHeaderInfo.nHeight = pItems[i][j].stImg.nHeight;
                            }

                            WS_SendPushImgData(nSnsId, (AX_VOID *)pItems[i][j].stImg.pData, pItems[i][j].stImg.nDataSize, 0, AX_TRUE, &tJpegInfo);
                        }
                    }
                }

                {
                    AX_OPAL_ALGO_PARAM_T stParam;
                    AX_OPAL_Video_AlgoGetParam(nSnsId, &stParam);

                    if (nRectIndex > 0 && stParam.stHvcfpParam.bEnable) {
                        AX_OPAL_Video_OsdDrawRect(nSnsId, AX_OPAL_VIDEO_CHN_0, nRectIndex, rectMain, 4, nColorList[i] | 0xFF000000);
                        AX_OPAL_Video_OsdDrawRect(nSnsId, AX_OPAL_VIDEO_CHN_2, nRectIndex, rectSub, 4, nColorList[i] | 0xFF000000);
                    }
                }
            }

            // update svc region
            {
                AX_OPAL_VIDEO_SVC_PARAM_T stMainParam;
                AX_OPAL_VIDEO_SVC_PARAM_T stSubParam;
                AX_OPAL_Video_GetSvcParam(nSnsId, AX_OPAL_VIDEO_CHN_0, &stMainParam);
                AX_OPAL_Video_GetSvcParam(nSnsId, AX_OPAL_VIDEO_CHN_2, &stSubParam);

                if (stMainParam.bEnable || stSubParam.bEnable) {
                    AX_OPAL_VIDEO_SVC_REGION_T stSvcRegion;
                    AX_OPAL_VIDEO_SVC_REGION_ITEM_T stSvcRegionItem[AX_OPAL_SVC_MAX_REGION_NUM];
                    AX_U32 nItemIndex = 0;

                    memset(&stSvcRegion, 0x00, sizeof(stSvcRegion));
                    memset(stSvcRegionItem, 0x00, sizeof(stSvcRegionItem));

                    stSvcRegion.u64Pts = pstResult->stHvcfpResult.u64Pts;
                    stSvcRegion.u64SeqNum = pstResult->stHvcfpResult.u64FrameId;

                    for (AX_U32 i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; ++i) {
                        if (stMainParam.stQpCfg[i].bEnable || stSubParam.stQpCfg[i].bEnable) {
                            for (AX_U32 j = 0; j < nCountList[i]; ++j) {
                                if (nItemIndex < AX_OPAL_SVC_MAX_REGION_NUM && (pItems[i][j].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_NEW ||
                                                                                 pItems[i][j].eTrackStatus == AX_OPAL_ALGO_TRACK_STATUS_UPDATE)) {
                                    stSvcRegionItem[nItemIndex].stRect.fX = pItems[i][j].stBox.fX;
                                    stSvcRegionItem[nItemIndex].stRect.fY = pItems[i][j].stBox.fY;
                                    stSvcRegionItem[nItemIndex].stRect.fW = pItems[i][j].stBox.fW;
                                    stSvcRegionItem[nItemIndex].stRect.fH = pItems[i][j].stBox.fH;

                                    stSvcRegionItem[nItemIndex].eRegionType = (AX_OPAL_VIDEO_SVC_REGION_TYPE_E)i;

                                    nItemIndex++;
                                }
                            }
                        }
                    }

                    stSvcRegion.nItemSize = nItemIndex;
                    stSvcRegion.pstItems = (AX_OPAL_VIDEO_SVC_REGION_ITEM_T*)stSvcRegionItem;

                    if (stAttrMain.eType == AX_OPAL_VIDEO_CHN_TYPE_H264
                        || stAttrMain.eType == AX_OPAL_VIDEO_CHN_TYPE_H265) {
                        AX_OPAL_Video_SetSvcRegion(nSnsId, AX_OPAL_VIDEO_CHN_0, &stSvcRegion);
                    }

                    if (stAttrSub.eType == AX_OPAL_VIDEO_CHN_TYPE_H264
                        || stAttrSub.eType == AX_OPAL_VIDEO_CHN_TYPE_H265) {
                        AX_OPAL_Video_SetSvcRegion(nSnsId, AX_OPAL_VIDEO_CHN_2, &stSvcRegion);
                    }
                }
            }
        }

        if (pstResult->stIvesResult.bValid) {
            if (pstResult->stIvesResult.nMdSize > 0) {
                WEB_EVENTS_DATA_T stEvent;

                stEvent.eType = E_WEB_EVENTS_TYPE_MD;
                stEvent.nReserved = nSnsId;
                stEvent.tMD.nAreaID = 0;
                WS_SendEventsData(&stEvent);
            }

            if (pstResult->stIvesResult.nOdSize > 0) {
                WEB_EVENTS_DATA_T stEvent;

                stEvent.eType = E_WEB_EVENTS_TYPE_OD;
                stEvent.nReserved = nSnsId;
                stEvent.tMD.nAreaID = 0;
                WS_SendEventsData(&stEvent);
            }
        }
    }
}

AX_VOID rtsp_init(AX_VOID) {
    AX_RTSP_Init(&g_pRtsp);

    for (AX_U32 i = 0; i < g_opal_nSnsNum; i++) {
        for (AX_U32 j = 0; j < AX_OPAL_VIDEO_CHN_BUTT; j++) {
            g_opal_RtspChnList[i][j] = -1;
        }
    }

    AX_U8 rtspChnIndex = 0;
    for (AX_U32 i = 0; i < g_opal_nSnsNum; i++) {
        if (g_stOpalAttr.stVideoAttr[i].bEnable) {
            for (AX_U32 j = 0; j < AX_OPAL_VIDEO_CHN_BUTT; j++) {
                if (g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].bEnable
                    && (AX_OPAL_VIDEO_CHN_TYPE_ALGO != g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].eType
                    && AX_OPAL_VIDEO_CHN_TYPE_JPEG != g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].eType)) {

                    AX_RTSP_SESS_ATTR_T rtspAttr;

                    AX_OPAL_VIDEO_CHN_ATTR_T stAttr;
                    memcpy(&stAttr, &g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr, sizeof(AX_OPAL_VIDEO_CHN_ATTR_T));
                    // AX_OPAL_Video_GetChnAttr((AX_OPAL_SNS_ID_E)i, (AX_OPAL_VIDEO_CHN_E)j, &stAttr);

                    rtspAttr.nChannel = rtspChnIndex;
                    rtspAttr.stVideoAttr.bEnable = AX_TRUE;
                    rtspAttr.stVideoAttr.ePt = EncoderType2PayloadType(stAttr.eType);

                    if (g_stOpalAttr.stAudioAttr.stCapAttr.bEnable &&
                        g_stOpalAttr.stAudioAttr.stCapAttr.stPipeAttr.stAudioChnAttr[AX_OPAL_AUDIO_CHN_0].bEnable) {
                        AX_OPAL_AUDIO_ENCODER_ATTR_T stAudioAttr = {0};
                        AX_OPAL_Audio_GetEncoderAttr(AX_OPAL_AUDIO_CHN_0, &stAudioAttr);

                        rtspAttr.stAudioAttr.bEnable = AX_TRUE;
                        rtspAttr.stAudioAttr.ePt = stAudioAttr.eType;
                        //rtspAttr.stAudioAttr.nMaxFrmSize = GetAencOutFrmSize();
                        //rtspAttr.stAudioAttr.nBitRate = stAudioAttr.nBitRate / 1024;
                        rtspAttr.stAudioAttr.nSampleRate = (AX_U32)stAudioAttr.eSampleRate;
                        rtspAttr.stAudioAttr.nAOT = stAudioAttr.nAOT;
                        if (stAudioAttr.eSoundMode == AX_OPAL_AUDIO_SOUND_MODE_MONO) {
                            rtspAttr.stAudioAttr.nChnCnt = 1;
                        } else {
                            rtspAttr.stAudioAttr.nChnCnt = 2;
                        }
                    } else {
                        rtspAttr.stAudioAttr.bEnable = AX_FALSE;
                    }

                    AX_RTSP_AddSessionAttr(g_pRtsp, rtspChnIndex, &rtspAttr);

                    g_opal_RtspChnList[i][j] = (AX_S32)rtspChnIndex;

                    rtspChnIndex++;
                }
            }
        }
    }
}

AX_VOID rtsp_deinit(AX_VOID) {
    AX_RTSP_Stop(g_pRtsp);
    AX_RTSP_Deinit(g_pRtsp);
    g_pRtsp = NULL;
}

AX_VOID rtsp_start() {
    AX_RTSP_Start(g_pRtsp);
}

AX_VOID rtsp_update_payloadtype(AX_U8 nSnsId, AX_U8 nChn, AX_PAYLOAD_TYPE_E eType) {
    AX_S32 nRtspChn = g_opal_RtspChnList[nSnsId][nChn];
    if (nRtspChn != -1 ) {
        AX_RTSP_SESS_ATTR_T stSessAttr;
        if (AX_RTSP_GetSessionAttr(g_pRtsp, nRtspChn, &stSessAttr)) {
            stSessAttr.stVideoAttr.ePt = eType;
            AX_RTSP_UpdateSessionAttr(g_pRtsp, nRtspChn, &stSessAttr);
            AX_RTSP_RestartSessions(g_pRtsp);
        }
    }
}

static AX_BOOL SetSvcParam() {
    AX_S32 nRet = AX_SUCCESS;
    for(AX_S32 i = 0; i < g_opal_nSnsNum; i++) {
        for (AX_U32 j = 0; j < AX_OPAL_VIDEO_CHN_BUTT; j++) {
            if (g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].bEnable
                && (AX_OPAL_VIDEO_CHN_TYPE_ALGO != g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].eType
                && AX_OPAL_VIDEO_CHN_TYPE_JPEG != g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].eType)) {
                nRet = AX_OPAL_Video_SetSvcParam(i, j, &g_stSvcParam[i]);
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_OPAL_Video_SetSvcParam[%d][%d] faild, nRet=0x%x", i, j, nRet);
                }
            }
        }
    }
    return (nRet == AX_SUCCESS) ? AX_TRUE : AX_FALSE;
}

static AX_VOID opal_config_attr(DEMO_SNS_TYPE_E eSnsType, DEMO_SNS_COMB_TYPE_E eSncCombType, AX_CHAR* pConfigPath, AX_OPAL_ATTR_T *pOpalAttr) {

    AX_OPAL_CHIP_TYPE_E eChipType = AX_OPAL_GetChipType();
    AX_CHAR *pSnsCfgPath = "630c";
    if (eChipType != AX_OPAL_CHIP_TYPE_AX630C) {
        pSnsCfgPath = "620q";
    }
    if (eSncCombType == DEMO_DUAL_SNS) {
        pOpalAttr->stVideoAttr[0].bEnable = AX_TRUE;
        pOpalAttr->stVideoAttr[1].bEnable = AX_TRUE;
        /* config sensor cfg  */
        switch (eSnsType) {
            case DEMO_OS04A10:
                memset(pOpalAttr->szPoolConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->szPoolConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "os04a10x2_pool.ini");

                memset(pOpalAttr->stVideoAttr[0].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[0].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "os04a10_2lane_0.ini");
                memset(pOpalAttr->stVideoAttr[1].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[1].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "os04a10_2lane_1.ini");
                break;
            case DEMO_SC200AI:
                memset(pOpalAttr->szPoolConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->szPoolConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc200aix2_pool.ini");

                memset(pOpalAttr->stVideoAttr[0].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[0].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc200ai_2lane_0.ini");
                memset(pOpalAttr->stVideoAttr[1].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[1].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc200ai_2lane_1.ini");

                break;
            case DEMO_SC450AI:
                memset(pOpalAttr->szPoolConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->szPoolConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc450aix2_pool.ini");

                memset(pOpalAttr->stVideoAttr[0].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[0].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc450ai_2lane_0.ini");
                memset(pOpalAttr->stVideoAttr[1].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[1].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc450ai_2lane_1.ini");
                break;
            case DEMO_OS04A10_SC200AI:
            case DEMO_SC450AI_SC200AI:
            case DEMO_SC500AI:
            case DEMO_SC850SL:
            default:
                printf("Not support dual sensor, sns type=%d\n", eSnsType);
                break;
        }

    }
    else if (eSncCombType == DEMO_SINGLE_SNS) {
        g_stOpalAttr.stVideoAttr[0].bEnable = AX_TRUE;
        g_stOpalAttr.stVideoAttr[1].bEnable = AX_FALSE;
        /* config sensor cfg  */
        switch (eSnsType) {
            case DEMO_OS04A10:
                memset(pOpalAttr->szPoolConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->szPoolConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "os04a10_pool.ini");

                memset(pOpalAttr->stVideoAttr[0].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[0].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "os04a10_4lane.ini");
                break;
            case DEMO_SC200AI:
                memset(pOpalAttr->szPoolConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->szPoolConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc200ai_pool.ini");

                memset(pOpalAttr->stVideoAttr[0].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[0].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc200ai_2lane_0.ini");

                break;
            case DEMO_SC450AI:
                memset(pOpalAttr->szPoolConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->szPoolConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc450ai_pool.ini");

                memset(pOpalAttr->stVideoAttr[0].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[0].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc450ai_4lane.ini");
                break;
            case DEMO_SC500AI:
                memset(pOpalAttr->szPoolConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->szPoolConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc500ai_pool.ini");

                memset(pOpalAttr->stVideoAttr[0].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[0].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc500ai_4lane.ini");
                break;
            case DEMO_SC850SL:
                memset(pOpalAttr->szPoolConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->szPoolConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc850sl_pool.ini");

                memset(pOpalAttr->stVideoAttr[0].szSnsConfigPath, 0x0, AX_OPAL_PATH_LEN);
                sprintf(pOpalAttr->stVideoAttr[0].szSnsConfigPath, "%s/sensor/%s/%s", pConfigPath, pSnsCfgPath, "sc850sl_4lane.ini");
                break;
            case DEMO_OS04A10_SC200AI:
            case DEMO_SC450AI_SC200AI:
            default:
                printf("Not support single sensor, sns type=%d\n", eSnsType);
                break;
        }
    }
}

static AX_S32 opal_init_video(AX_S32 nSnsId) {
    AX_S32 nRet = 0;
    if (nSnsId == VIDEO_SNS_ID_INVALID) {
        return nRet;
    }

    AX_OPAL_Video_RegisterPacketCallback(nSnsId, VIDEO_CHN_VENC_MAIN_ID, OPALDemo_VideoPacketCallback, AX_NULL);
    AX_OPAL_Video_RegisterPacketCallback(nSnsId, VIDEO_CHN_VENC_SUB1_ID, OPALDemo_VideoPacketCallback, AX_NULL);
    AX_OPAL_Video_RegisterAlgoCallback(nSnsId, OPALDemo_AlgoResultCallback, AX_NULL);

    for (AX_U32 iChn = 0; iChn < AX_OPAL_DEMO_VIDEO_CHN_NUM; iChn++) {
        for (AX_U32 iOsd = 0; iOsd < AX_OPAL_DEMO_OSD_CNT; iOsd++) {
            DEMO_OSD_ITEM_T *pItem =  &g_stOsdCfg[nSnsId].stOsd[iChn][iOsd];
            nRet = AX_OPAL_Video_OsdCreate(nSnsId, pItem->nSrcChn, &pItem->stOsdAttr, &pItem->pHandle);
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AX_OPAL_Video_OsdCreate(%d, %d) osd failed, nRet=0x%x", nSnsId, pItem->nSrcChn, nRet);
            }
        }
    }
    DEMO_OSD_ITEM_T *pItem =  &g_stOsdCfg[nSnsId].stPriv;
    nRet = AX_OPAL_Video_OsdCreate(nSnsId, pItem->nSrcChn, &pItem->stOsdAttr, &pItem->pHandle);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_Video_OsdCreate(%d, %d) priv failed, nRet=0x%x", nSnsId, pItem->nSrcChn, nRet);
    }

    // algorithm parameter
    {
#if 1
        AX_OPAL_ALGO_PARAM_T stAlgoParam;

        // get algorithm parameter
        nRet = AX_OPAL_Video_AlgoGetParam(nSnsId, &stAlgoParam);
        stAlgoParam.stIvesParam.nSrcFramerate = g_stOpalAttr.stVideoAttr[nSnsId].stPipeAttr.stVideoChnAttr[AX_OPAL_VIDEO_CHN_1].nFramerate;
        stAlgoParam.stIvesParam.nDstFramerate = 1;

        // motion parameter
        if (stAlgoParam.stIvesParam.stMdParam.bEnable) {
            stAlgoParam.stIvesParam.stMdParam.nRegionSize = 1;
            AX_OPAL_ALGO_MD_REGION_T *pstMdRegion = &stAlgoParam.stIvesParam.stMdParam.stRegions[0];
            pstMdRegion->fThreshold = 0.5;
            pstMdRegion->fConfidence = 0.5;
            pstMdRegion->stRect.fX = 0;
            pstMdRegion->stRect.fY = 0;
            pstMdRegion->stRect.fW = (AX_F32)g_stOpalAttr.stVideoAttr[nSnsId].stPipeAttr.stVideoChnAttr[AX_OPAL_VIDEO_CHN_1].nWidth;
            pstMdRegion->stRect.fH = (AX_F32)g_stOpalAttr.stVideoAttr[nSnsId].stPipeAttr.stVideoChnAttr[AX_OPAL_VIDEO_CHN_1].nHeight;
        }

        // body ae roi
        stAlgoParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].bEnable = IsEnableBodyAeRoi();
        stAlgoParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].nWidth = 50;
        stAlgoParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].nHeight = 50;

        // vehicle ae roi
        stAlgoParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].bEnable = IsEnableVehicleAeRoi();
        stAlgoParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].nWidth = 50;
        stAlgoParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].nHeight = 50;
        // set algorithm parameter
        nRet = AX_OPAL_Video_AlgoSetParam(nSnsId, &stAlgoParam);
#endif
    }

    return nRet;
}

static AX_VOID opal_deinit_video(AX_S32 nSnsId) {
    if (nSnsId == VIDEO_SNS_ID_INVALID) {
        return;
    }

    AX_OPAL_Video_UnRegisterPacketCallback(nSnsId, VIDEO_CHN_VENC_MAIN_ID);
    AX_OPAL_Video_UnRegisterPacketCallback(nSnsId, VIDEO_CHN_VENC_SUB1_ID);
    AX_OPAL_Video_UnRegisterAlgoCallback(nSnsId);

    for (AX_U32 iChn = 0; iChn < AX_OPAL_DEMO_VIDEO_CHN_NUM; iChn++) {
        for (AX_U32 iOsd = 0; iOsd < AX_OPAL_DEMO_OSD_CNT; iOsd++) {
            DEMO_OSD_ITEM_T *pItem =  &g_stOsdCfg[nSnsId].stOsd[iChn][iOsd];
            AX_OPAL_Video_OsdDestroy(nSnsId, pItem->nSrcChn, pItem->pHandle);
        }
    }
    DEMO_OSD_ITEM_T *pItem =  &g_stOsdCfg[nSnsId].stPriv;
    AX_OPAL_Video_OsdDestroy(nSnsId, pItem->nSrcChn, pItem->pHandle);
}

static AX_BOOL SetSnsParam() {
    AX_S32 nRet = AX_SUCCESS;
    for (AX_S32 i = 0; i < g_opal_nSnsNum; i++) {
        AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
        AX_S32 nRet = AX_OPAL_Video_GetSnsAttr(i, &stSnsAttr);
        if (IsEnableDIS()) {
            stSnsAttr.stDisAttr.bMotionEst = GetDISMotionEst();
            stSnsAttr.stDisAttr.bMotionShare = GetDISMotionShare();
            stSnsAttr.stDisAttr.nDelayFrameNum = GetDISDelayFrameNum();
        }
        stSnsAttr.eMode = GetSensorMode();
        nRet = AX_OPAL_Video_SetSnsAttr(i, &stSnsAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_OPAL_Video_SetSnsAttr[%d] faild, nRet=0x%x", i, nRet);
        }
    }
    return  (nRet == AX_SUCCESS) ? AX_TRUE : AX_FALSE;
}

static AX_VOID SetInterpolationResolution() {
    AX_U32 nWidth = 0;
    AX_U32 nHeight = 0;
    GetInterpolationResolution(&nWidth, &nHeight);
    if (nWidth > 0 && nHeight > 0) {
        for (AX_S32 i = 0; i < g_opal_nSnsNum; i++) {
            g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[0].nMaxWidth = nWidth;
            g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[0].nMaxHeight = nHeight;
            g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[0].nWidth = nWidth;
            g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[0].nHeight = nHeight;
        }
    }
}

void ResetMp4(AX_U8 nSnsId) {
    AX_OPAL_VIDEO_CHN_ATTR_T stVideoAttr = {0};
    AX_OPAL_Video_GetChnAttr(nSnsId, VIDEO_CHN_VENC_MAIN_ID, &stVideoAttr);
    if (stVideoAttr.bEnable && (stVideoAttr.eType == AX_OPAL_VIDEO_CHN_TYPE_H264 || stVideoAttr.eType == AX_OPAL_VIDEO_CHN_TYPE_H265)) {
        AX_MP4_INFO_T stMp4Info = {0};
        stMp4Info.nSnsId = nSnsId;
        stMp4Info.nChn = VIDEO_CHN_VENC_MAIN_ID;
        stMp4Info.bLoopSet = GetMp4LoopSet();
        stMp4Info.nMaxFileInMBytes = GetMp4FileSize();
        stMp4Info.nMaxFileCount = GetMp4FileCount();

        stMp4Info.stVideoAttr.bEnable = AX_TRUE;
        stMp4Info.stVideoAttr.ePt = EncoderType2PayloadType(stVideoAttr.eType);
        stMp4Info.stVideoAttr.nfrWidth = stVideoAttr.nWidth;
        stMp4Info.stVideoAttr.nfrHeight = stVideoAttr.nHeight;
        stMp4Info.stVideoAttr.nFrameRate = stVideoAttr.nFramerate;
        stMp4Info.stVideoAttr.nBitrate = stVideoAttr.stEncoderAttr.nBitrate;

        AX_OPAL_AUDIO_ENCODER_ATTR_T stAudioEncAttr;
        AX_OPAL_Audio_GetEncoderAttr(0, &stAudioEncAttr);
        AX_OPAL_AUDIO_ATTR_T stAudioAttr;
        AX_OPAL_Audio_GetAttr(&stAudioAttr);
        stMp4Info.stAudioAttr.bEnable = stAudioAttr.stCapAttr.bEnable;
        stMp4Info.stAudioAttr.ePt = (AX_PAYLOAD_TYPE_E)stAudioEncAttr.eType;
        stMp4Info.stAudioAttr.nBitrate = stAudioEncAttr.nBitRate;
        stMp4Info.stAudioAttr.nSampleRate = (AX_U32)stAudioEncAttr.eSampleRate;
        stMp4Info.stAudioAttr.nChnCnt = (AX_OPAL_AUDIO_SOUND_MODE_MONO == stAudioEncAttr.eSoundMode) ? 1 : 2;
        stMp4Info.stAudioAttr.nAOT = (AX_S32)stAudioEncAttr.nAOT;
        GetMp4SavedPath(stMp4Info.strSavePath, sizeof(stMp4Info.strSavePath) -1);
        pthread_mutex_lock(&g_mtxMp4[nSnsId]);
        if (g_pMp4Handle[0]) {
            AX_Mp4_DeInit(g_pMp4Handle[nSnsId]);
            g_pMp4Handle[0] = NULL;
        }
        AX_MP4_Init(&g_pMp4Handle[nSnsId], &stMp4Info);
        pthread_mutex_unlock(&g_mtxMp4[nSnsId]);
    }
}

static AX_VOID InitMp4() {
    if (IsEnableMp4Record()) {
        for (AX_S32 i = 0; i < g_opal_nSnsNum; i++) {
            pthread_mutex_init(&g_mtxMp4[i], NULL);
            ResetMp4(i);
        }
    }
}

static AX_VOID DeinitMp4() {
    for (AX_S32 i = 0; i < g_opal_nSnsNum; i++) {
        pthread_mutex_lock(&g_mtxMp4[i]);
        if (g_pMp4Handle[0]) {
            AX_Mp4_DeInit(g_pMp4Handle[0]);
            g_pMp4Handle[0] = NULL;
        }
        pthread_mutex_unlock(&g_mtxMp4[i]);
    }
}

static AX_VOID app_mallopt_policy(AX_VOID) {
    mallopt(M_TRIM_THRESHOLD, MAIN_TRIM_THRESHOLD);
}

int main(int argc, char *argv[]) {
    LOG_M_C(LOG_TAG, "=============== APP(APP Ver: %s, SDK Ver: %s) Started %s %s ===============", APP_BUILD_VERSION, AX_OPAL_GetVersion(), __DATE__, __TIME__);

    AX_S32 nRet = AX_SUCCESS;
    AX_U32 nRunTimes = 0;
    AX_CHAR szIP[64] = {0};

    const AX_CHAR* pstrRes = "./res/welcome.g711a";
    AX_PAYLOAD_TYPE_E eType = PT_G711A;

    signal(SIGINT, exit_handler);
    ignore_sig_pipe();

    prctl(PR_SET_NAME, "APP_MAIN");
    app_mallopt_policy();

    DEMO_SNS_TYPE_E sensor_type = DEMO_OS04A10;
    DEMO_SNS_COMB_TYPE_E sensor_comb_type = DEMO_SINGLE_SNS;

    // cmdline parser
    {
        char cmd_args[512] = "cmdline args:";

        int c;
        int isExit = 0;
        while ((c = getopt(argc, argv, "s:n:l:t:c:z:adh::")) != -1) {
            switch (c) {
            case 's':
                sensor_type = (DEMO_SNS_TYPE_E)atoi(optarg);
                strncat(cmd_args, " -s ", 255);
                strncat(cmd_args, optarg, 255);
                break;
            case 'n':
                sensor_comb_type = (DEMO_SNS_COMB_TYPE_E)atoi(optarg);
                strncat(cmd_args, " -n ", 255);
                strncat(cmd_args, optarg, 255);
                break;
            case 'l':
                g_opalapp_log_level = atoi(optarg);
                setenv("OPAL_LOG_level", optarg, 1);
                strncat(cmd_args, " -l ", 255);
                strncat(cmd_args, optarg, 255);
                break;
            case 't':
                setenv("OPAL_LOG_target", optarg, 1);
                strncat(cmd_args, " -t ", 255);
                strncat(cmd_args, optarg, 255);
                break;
            case 'c':
                strncpy(g_strConfigPath, optarg, sizeof(g_strConfigPath)-1);
                SetCfgPath(g_strConfigPath);
                strncat(cmd_args, " -c ", 255);
                strncat(cmd_args, optarg, 255);
                break;
            case 'a':
                strncat(cmd_args, " -a", 255);
                break;
            case 'd':
                strncat(cmd_args, " -d", 255);
                break;
            case 'z':
                g_nDelayExitTime = atoi(optarg);
                break;
            case 'h':
            default:
                isExit = 1;
                break;
            }
        }

        LOG_M_C(LOG_TAG, "%s", cmd_args);

        if (isExit) {
            return 0;
        }
    }

    /* set sensor count */
    g_opal_nSnsNum = 1;
    if (sensor_comb_type == DEMO_DUAL_SNS) {
        g_opal_nSnsNum = 2;
    }
    else if (sensor_comb_type == DEMO_SINGLE_SNS) {
        g_opal_nSnsNum = 1;
    }

    if (sensor_type >= DEMO_OS04A10 && sensor_type <= DEMO_SC850SL) {
        g_opal_nSnsType = sensor_type;
    } else {
        LOG_M_E(LOG_TAG, "unknown sensor type");
        return 0;
    }

    opal_config_attr(sensor_type, sensor_comb_type, g_strConfigPath, &g_stOpalAttr);

    g_stOpalAttr.stAudioAttr.stCapAttr.bEnable = IsEnableAudio();
    g_stOpalAttr.stAudioAttr.stPlayAttr.bEnable = IsEnableAudio();
    g_stOpalAttr.stAudioAttr.stCapAttr.stPipeAttr.stAudioChnAttr[0].eType = (AX_PAYLOAD_TYPE_E)GetAudioEncoderType();

    /* set interpolation resolution */
    SetInterpolationResolution();

    nRet = AX_OPAL_Init(&g_stOpalAttr);
    if (nRet != AX_SUCCESS) {
        LOG_M_C(LOG_TAG, "AX_OPAL_Init failed, nRet=0x%x", nRet);
        goto EXIT_ERR;
    }

    AX_OPAL_Audio_RegisterPacketCallback(AUDIO_CHN_ID, OPALDemo_AudioPacketCallback, AX_NULL);

    for (AX_S32 i = 0; i < g_opal_nSnsNum; i++) {
        nRet = opal_init_video(i);
        if (nRet != AX_SUCCESS) {
            goto EXIT_ERR;
        }
    }

    FpsStatInit(g_opal_nSnsNum);
    rtsp_init();
    WS_Init();
    WS_Start();
    InitMp4();

    nRet = AX_OPAL_Start();
    if (nRet != AX_SUCCESS) {
        LOG_M_E(LOG_TAG, "AX_OPAL_Start failed, nRet=0x%x", nRet);
        goto EXIT_ERR;
    }

    /* set svc param */
    SetSvcParam();

    /* update sns param*/
    SetSnsParam();

    AX_OPAL_Audio_PlayFile(AUDIO_CHN_ID, eType, pstrRes, 1, OPALDemo_AudioPlayFileResultCallback, NULL);

    rtsp_start();

    if (AX_RTSP_GetIP(&szIP[0], 64) == 0) {
        LOG_M_C(LOG_TAG, "Preview the video using URL: <<<<< http://%s:8088 >>>>>", szIP);
    } else {
        LOG_M_E(LOG_TAG, "Can not get host ip address.");
    }

    g_Running = AX_TRUE;
    while (g_Running) {
        OPAL_mSleep(100);
        if (++nRunTimes > 10 * 30) {
            // Release memory back to the system every 30 seconds.
            // https://linux.die.net/man/3/malloc_trim
            malloc_trim(0);
            nRunTimes = 0;
        }
    }

EXIT_ERR:
    AX_OPAL_Audio_UnRegisterPacketCallback(AUDIO_CHN_ID);
    for (AX_S32 i = 0; i < g_opal_nSnsNum; i++) {
        opal_deinit_video(i);
    }

    nRet = AX_OPAL_Audio_StopPlay(AUDIO_CHN_ID);
    nRet = AX_OPAL_Stop();
    if (nRet != AX_SUCCESS) {
        LOG_M_E(LOG_TAG, "AX_OPAL_Stop failed, nRet=0x%x", nRet);
    } else {
        LOG_M_C(LOG_TAG, "AX_OPAL_Stop done");
    }
    nRet = AX_OPAL_Deinit();
    if (nRet != AX_SUCCESS) {
        LOG_M_E(LOG_TAG, "AX_OPAL_Deinit failed, nRet=0x%x", nRet);
    } else {
        LOG_M_C(LOG_TAG, "AX_OPAL_Deinit done");
    }

    rtsp_deinit();
    WS_Stop();
    WS_DeInit();
    FpsStatDeinit();
    DeinitMp4();
    LOG_M_C(LOG_TAG, "=============== APP(APP Ver: %s, SDK Ver: %s) Exited %s %s ===============", APP_BUILD_VERSION, AX_OPAL_GetVersion(), __DATE__, __TIME__);
    return 0;
}
