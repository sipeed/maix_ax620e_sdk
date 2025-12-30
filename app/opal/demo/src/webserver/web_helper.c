/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "web_helper.h"
#include "web_server.h"
#include "ax_opal_api.h"
#include "ax_timer.h"
#include "ax_utils.h"
#include "ax_log.h"
#include "ax_option.h"
#include "config.h"
#include "cJSON.h"

#include "appweb.h"
#include "http.h"
#include "mpr.h"

#include "GlobalDef.h"
#include "ax_osd_utils.h"

#define WEB_HELPER "WEB_HELPER"


extern AX_OPAL_ATTR_T g_stOpalAttr;
extern DEMO_SNS_OSD_CONFIG_T g_stOsdCfg[AX_OPAL_SNS_ID_BUTT];
extern AX_U8 g_opal_nSnsNum;
extern DEMO_SNS_TYPE_E g_opal_nSnsType;
extern AX_U8 GetSrcChn(AX_U8 nSnsId, AX_U8 nPrevChn);
extern AX_U8 GetVideoCount(AX_U8 nSnsId);
extern const AX_OPAL_VIDEO_ENCODER_ATTR_T g_stOpalEncoderList[AX_OPAL_VIDEO_RC_MODE_BUTT];
extern AX_VOID rtsp_update_payloadtype(AX_U8 nSnsId, AX_U8 nChn, AX_PAYLOAD_TYPE_E eType);

static AX_S32 g_arrDayNightMode[AX_OPAL_SNS_ID_BUTT] = {0};
static pthread_mutex_t  g_mtxWebReqProcess = PTHREAD_MUTEX_INITIALIZER;

typedef struct _app_opal_resolution_t {
    AX_U32 nWidth;
    AX_U32 nHeight;
} app_opal_resolution_t;

const static app_opal_resolution_t g_stAvailResolutionList[] = {
    {3840, 2160},
    {3072, 2048},
    {3072, 1728},
    {3200, 1600},
    {2624, 1944},
    {2688, 1520},
    {2048, 1536},
    {2304, 1296},
    {1920, 1080},
    {1280, 960},
    {1280, 720},
    {720, 576},
    {704, 576},
    {640, 480},
    //{384, 288}
};

static inline MprJson* ReadJsonValeInArry(MprJson* object, AX_S32 index) {
    MprJson* result = NULL;
    if (object->type & MPR_JSON_ARRAY) {
        MprJson *child = NULL;
        AX_S32 i = 0;
        for (ITERATE_JSON(object, child, i)) {
            if (i == index) {
                result = child;
                break;
            }
        }
    }

    return result;
}

static inline MprJson* ReadJsonValeIn2DArry(MprJson* object, AX_S32 i, AX_S32 j) {
    MprJson* target = ReadJsonValeInArry(object, i);
    return ReadJsonValeInArry(target, j);
}

AX_VOID GetSnsPnFps(AX_U8 nSnsId, AX_S32 nFps[2], AX_S32 *pCount) {

    AX_BOOL b620Q = (AX_OPAL_GetChipType() != AX_OPAL_CHIP_TYPE_AX630C) ? AX_TRUE : AX_FALSE;

    if ((g_opal_nSnsNum == 2) && b620Q) {
        switch (g_opal_nSnsType) {
        case DEMO_OS04A10:
        case DEMO_SC450AI:
            nFps[0] = 25;
            *pCount = 1;
            return;
        case DEMO_SC200AI:
            nFps[0] = 25;
            nFps[1] = 30;
            *pCount = 2;
            return;
        default:
            break;
        }
    } else {
        switch (g_opal_nSnsType)
        {
        case DEMO_OS04A10:
        case DEMO_SC450AI:
        case DEMO_SC200AI:
        case DEMO_SC500AI:
            nFps[0] = 25;
            nFps[1] = 30;
            *pCount = 2;
            return;
        default:
            break;
        }
    }

    if (b620Q) {
        switch (g_opal_nSnsType) {
        case DEMO_SC850SL:
            nFps[0] = 20;
            *pCount = 1;
        default:
            break;
        }
    } else {
        switch (g_opal_nSnsType) {
        case DEMO_SC850SL:
            nFps[0] = 25;
            nFps[1] = 30;
            *pCount = 2;
        default:
            break;
        }
    }
    nFps[0] = 25;
    nFps[1] = 30;
    *pCount = 2;
    return;
}


cJSON* GetCameraJson(AX_U8 nSnsId) {
    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
    AX_S32 nRet = AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAttr);
    if (0 != nRet) {
        LOG_M_E(WEB_HELPER, "AX_OPAL_Video_GetSnsAttr(snsId=%d) failed, ret=0x%x", nSnsId, nRet);
    }
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "sns_work_mode", stSnsAttr.eMode);
    cJSON_AddNumberToObject(root, "rotation", stSnsAttr.eRotation);
    cJSON_AddNumberToObject(root, "framerate", stSnsAttr.fFrameRate);
    cJSON_AddNumberToObject(root, "daynight", g_arrDayNightMode[nSnsId] == 2 ? 2 : stSnsAttr.eDayNight);
    cJSON_AddBoolToObject(root, "mirror", stSnsAttr.bMirror);
    cJSON_AddBoolToObject(root, "flip", stSnsAttr.bFlip);
    cJSON_AddBoolToObject(root, "switch_work_mode_enable", 1);
    cJSON_AddBoolToObject(root, "switch_PN_mode_enable", 1);
    cJSON_AddBoolToObject(root, "switch_mirror_flip_enable", 1);
    cJSON_AddBoolToObject(root, "switch_rotation_enable", 1);
    cJSON_AddBoolToObject(root, "hdr_ratio_enable", 0);
    cJSON_AddNumberToObject(root, "hdr_ratio", 0);
    return root;
}

cJSON* GetSnsFramerateJson(AX_U8 nSnsId) {
    AX_S32 arrFps[2] = {25,30};
    AX_S32 nCount = 2;
    GetSnsPnFps(nSnsId, arrFps, &nCount);
    cJSON* root = cJSON_CreateIntArray(arrFps, nCount);
    return root;
}

cJSON* GetSnsResolutionJson(AX_U8 nSnsId) {
    // not support, return null array
    cJSON* root = cJSON_CreateArray();
    return root;
}

cJSON* GetImageJson(AX_U8 nSnsId) {
    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
    AX_S32 nRet = AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAttr);
    if (0 != nRet) {
        LOG_M_E(WEB_HELPER, "AX_OPAL_Video_GetSnsAttr(snsId=%d) failed, ret=0x%x", nSnsId, nRet);
    }
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "isp_auto_mode", !stSnsAttr.stColorAttr.bManual);
    cJSON_AddNumberToObject(root, "sharpness", stSnsAttr.stColorAttr.fSharpness);
    cJSON_AddNumberToObject(root, "brightness", stSnsAttr.stColorAttr.fBrightness);
    cJSON_AddNumberToObject(root, "contrast", stSnsAttr.stColorAttr.fContrast);
    cJSON_AddNumberToObject(root, "saturation", stSnsAttr.stColorAttr.fSaturation);
    return root;
}

cJSON* GetLdcJson(AX_U8 nSnsId) {
    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
    AX_S32 nRet = AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAttr);
    if (0 != nRet) {
        LOG_M_E(WEB_HELPER, "AX_OPAL_Video_GetSnsAttr(snsId=%d) failed, ret=0x%x", nSnsId, nRet);
    }
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ldc_support", AX_TRUE);
    cJSON_AddBoolToObject(root, "ldc_enable", stSnsAttr.stLdcAttr.bEnable);
    cJSON_AddBoolToObject(root, "aspect", stSnsAttr.stLdcAttr.bAspect);
    cJSON_AddNumberToObject(root, "x_ratio", stSnsAttr.stLdcAttr.nXRatio);
    cJSON_AddNumberToObject(root, "y_ratio", stSnsAttr.stLdcAttr.nYRatio);
    cJSON_AddNumberToObject(root, "xy_ratio", stSnsAttr.stLdcAttr.nXYRatio);
    cJSON_AddNumberToObject(root, "distor_ratio", stSnsAttr.stLdcAttr.nDistortionRatio);
    return root;
}

cJSON* GetDisJson(AX_U8 nSnsId) {
    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
    AX_S32 nRet = AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAttr);
    if (0 != nRet) {
        LOG_M_E(WEB_HELPER, "AX_OPAL_Video_GetSnsAttr(snsId=%d) failed, ret=0x%x", nSnsId, nRet);
    }
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "dis_support", IsEnableDIS());
    cJSON_AddBoolToObject(root, "dis_enable", stSnsAttr.stDisAttr.bEnable);
    return root;
}

cJSON*  GetAudioJson() {
    AX_F32 fCapVol = 0.0;
    AX_F32 fPalyVol = 0.0;

    AX_OPAL_AUDIO_ATTR_T stAttr = {0};
    AX_OPAL_Audio_GetAttr(&stAttr);
    AX_OPAL_Audio_GetCapVolume(&fCapVol);
    AX_OPAL_Audio_GetPlayVolume(&fPalyVol);

    cJSON* root = cJSON_CreateObject();
    cJSON* capture_attr = cJSON_CreateObject();
    cJSON* play_attr = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "capture_attr", capture_attr);
    cJSON_AddItemToObject(root, "play_attr", play_attr);

    cJSON_AddNumberToObject(capture_attr, "volume_val", fCapVol);
    cJSON_AddNumberToObject(play_attr, "volume_val", fPalyVol);

    return root;
}

cJSON* GetVideoAttrJson(AX_U8 nSnsId, AX_U32 nSrcChn, AX_U32 nWFirst, AX_U32 nHFirst, AX_U32 nWLast, AX_U32 nHLast) {
    AX_OPAL_VIDEO_CHN_ATTR_T stChnAttr = {0};
    AX_OPAL_Video_GetChnAttr(nSnsId, nSrcChn, &stChnAttr);
    const AX_OPAL_VIDEO_CHN_TYPE_E arrChnType[3] = {AX_OPAL_VIDEO_CHN_TYPE_H264, AX_OPAL_VIDEO_CHN_TYPE_H265, AX_OPAL_VIDEO_CHN_TYPE_MJPEG};

    cJSON* root = cJSON_CreateObject();

    cJSON* arrEncType = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "enc_rc_info", arrEncType);

    for (int i = 0; i < 3; i++) {

        cJSON* rcInfo = cJSON_CreateObject();
        cJSON_AddItemToArray(arrEncType, rcInfo);

        cJSON_AddNumberToObject(rcInfo, "encoder_type", EncoderType2Int(arrChnType[i]));

        cJSON* arrRc = cJSON_CreateArray();
        cJSON_AddItemToObject(rcInfo, "rc", arrRc);
        for (AX_U32 j = 0; j < AX_OPAL_VIDEO_RC_MODE_BUTT; j++) {
            cJSON* rcItem = cJSON_CreateObject();
            cJSON_AddItemToArray(arrRc, rcItem);

            AX_OPAL_VIDEO_ENCODER_ATTR_T stRcInfo = g_stOpalEncoderList[j];
            cJSON_AddNumberToObject(rcItem, "rc_type", RcMode2Int(stRcInfo.eRcMode));
            cJSON_AddNumberToObject(rcItem, "min_qp", stRcInfo.nMinQp);
            cJSON_AddNumberToObject(rcItem, "max_qp", stRcInfo.nMaxQp);
            cJSON_AddNumberToObject(rcItem, "min_iqp", stRcInfo.nMinIQp);
            cJSON_AddNumberToObject(rcItem, "max_iqp", stRcInfo.nMaxIQp);
            cJSON_AddNumberToObject(rcItem, "min_iprop", stRcInfo.nMinIprop);
            cJSON_AddNumberToObject(rcItem, "max_iprop", stRcInfo.nMaxIprop);
        }
    }

    cJSON* arrResolutionList = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "resolution_opt", arrResolutionList);

    AX_S32 arrAvlResSize = sizeof(g_stAvailResolutionList) / sizeof(g_stAvailResolutionList[0]);

    AX_CHAR szReslution[64] = {0};
    sprintf(szReslution,"%dx%d", nWFirst, nHFirst);
    cJSON_AddItemToArray(arrResolutionList, cJSON_CreateString(szReslution));

    if (nWFirst * nHFirst < nWLast * nHLast) {
        nWLast = 0;
        nHLast = 0;
    }
    for (AX_S32 i = 0; i < arrAvlResSize; i++) {
        if (g_stAvailResolutionList[i].nWidth * g_stAvailResolutionList[i].nHeight < nWFirst * nHFirst && g_stAvailResolutionList[i].nWidth * g_stAvailResolutionList[i].nHeight > nWLast * nHLast) {
            sprintf(szReslution,"%dx%d", g_stAvailResolutionList[i].nWidth, g_stAvailResolutionList[i].nHeight);
            cJSON_AddItemToArray(arrResolutionList, cJSON_CreateString(szReslution));
        }
    }

    return root;
}

cJSON* GetVideoJson(AX_U8 nSnsId, AX_U32 nSrcChn) {
    AX_OPAL_VIDEO_CHN_ATTR_T stChnAttr = {0};
    AX_OPAL_Video_GetChnAttr(nSnsId, nSrcChn, &stChnAttr);
    AX_U32 nEncType = EncoderType2Int(stChnAttr.eType);
    AX_U32 nRcType = RcMode2Int(stChnAttr.stEncoderAttr.eRcMode);
    AX_CHAR szStr[64] = {0};
    cJSON* root = cJSON_CreateObject();
    sprintf(szStr,"%dx%d", stChnAttr.nWidth, stChnAttr.nHeight);
    cJSON_AddBoolToObject(root, "enable_stream", stChnAttr.bEnable);
    cJSON_AddTrueToObject(root, "enable_res_chg");
    cJSON_AddNumberToObject(root, "encoder_type", nEncType);
    cJSON_AddNumberToObject(root, "bit_rate", stChnAttr.stEncoderAttr.nBitrate);
    cJSON_AddNumberToObject(root, "gop", stChnAttr.stEncoderAttr.nGop);
    cJSON_AddStringToObject(root, "resolution", szStr);
    cJSON_AddNumberToObject(root, "rc_type", nRcType);
    cJSON_AddNumberToObject(root, "min_qp", stChnAttr.stEncoderAttr.nMinQp);
    cJSON_AddNumberToObject(root, "max_qp", stChnAttr.stEncoderAttr.nMaxQp);
    cJSON_AddNumberToObject(root, "min_iqp", stChnAttr.stEncoderAttr.nMinIQp);
    cJSON_AddNumberToObject(root, "max_iqp", stChnAttr.stEncoderAttr.nMaxIQp);
    cJSON_AddNumberToObject(root, "min_iprop", stChnAttr.stEncoderAttr.nMinIprop);
    cJSON_AddNumberToObject(root, "max_iprop", stChnAttr.stEncoderAttr.nMaxIprop);
    return root;
}

const AX_CHAR* GetDetectModelStr(AX_U8 nAlgo) {
    if (AX_OPAL_ALGO_FACE == nAlgo) {
        return "facehuman";
    } else if (AX_OPAL_ALGO_HVCP == nAlgo) {
        return "hvcfp";
    } else {
        return "hvcfp";
    }
}

AX_OPAL_ALGO_PUSH_MODE_E PushModeStr2Int(const AX_CHAR *strAiPushMode) {
    if (strcmp(strAiPushMode, "FAST") == 0) {
        return AX_OPAL_ALGO_PUSH_MODE_FAST;
    } else if (strcmp(strAiPushMode, "INTERVAL") == 0) {
        return AX_OPAL_ALGO_PUSH_MODE_INTERVAL;
    } else {
        return AX_OPAL_ALGO_PUSH_MODE_BEST;
    }
}

const AX_CHAR* Int2PushModeStr(AX_S32 mode) {
    if (mode == AX_OPAL_ALGO_PUSH_MODE_FAST){
        return "FAST";
    } else if (mode == AX_OPAL_ALGO_PUSH_MODE_INTERVAL) {
        return "INTERVAL";
    } else {
        return "BEST_FRAME";
    }
}

cJSON* GetAiInfoJson(AX_U8 nSnsId) {
    AX_OPAL_ALGO_PARAM_T stParam = {0};
    AX_OPAL_Video_AlgoGetParam((AX_OPAL_SNS_ID_E)nSnsId, &stParam);

    cJSON * root = cJSON_CreateObject();

    cJSON_AddBoolToObject(root, "ai_enable", stParam.stHvcfpParam.bEnable);
    cJSON_AddStringToObject(root, "detect_model", GetDetectModelStr(stParam.stHvcfpParam.nAlgoType));
    cJSON_AddNumberToObject(root, "detect_fps", stParam.stHvcfpParam.nDstFramerate);

    cJSON * push_strategy = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "push_strategy", push_strategy);

    cJSON_AddStringToObject(push_strategy, "push_mode", Int2PushModeStr(stParam.stHvcfpParam.stPushStrategy.ePushMode));
    cJSON_AddNumberToObject(push_strategy, "push_interval", stParam.stHvcfpParam.stPushStrategy.nInterval);
    cJSON_AddNumberToObject(push_strategy, "push_count", stParam.stHvcfpParam.stPushStrategy.nPushCount);

    cJSON * events = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "events", events);

    cJSON * md = cJSON_CreateObject();
    cJSON * od = cJSON_CreateObject();
    cJSON_AddItemToObject(events, "motion_detect", md);
    cJSON_AddItemToObject(events, "occlusion_detect", od);

    cJSON_AddBoolToObject(md, "enable", stParam.stIvesParam.stMdParam.bEnable);
    cJSON_AddNumberToObject(md, "threshold_y", (AX_U32)(stParam.stIvesParam.stMdParam.stRegions[0].fThreshold * 255));
    cJSON_AddNumberToObject(md, "confidence", (AX_U32)(stParam.stIvesParam.stMdParam.stRegions[0].fConfidence * 100));

    cJSON_AddBoolToObject(od, "enable", stParam.stIvesParam.stOdParam.bEnable);
    cJSON_AddNumberToObject(od, "threshold_y", (AX_U32)(stParam.stIvesParam.stOdParam.fThreshold * 255));
    cJSON_AddNumberToObject(od, "confidence", (AX_U32)(stParam.stIvesParam.stOdParam.fConfidence * 100));

    cJSON * body_roi = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "body_roi", body_roi);

    cJSON_AddBoolToObject(body_roi, "enable", stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].bEnable);
    cJSON_AddNumberToObject(body_roi, "min_width", stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].nWidth);
    cJSON_AddNumberToObject(body_roi, "min_height", stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].nHeight);


    cJSON * vehicle_roi = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "vehicle_roi", vehicle_roi);
    cJSON_AddBoolToObject(vehicle_roi, "enable", stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].bEnable);
    cJSON_AddNumberToObject(vehicle_roi, "min_width", stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].nWidth);
    cJSON_AddNumberToObject(vehicle_roi, "min_height", stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].nHeight);

    cJSON * svc = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "svc", svc);

    AX_OPAL_VIDEO_SVC_PARAM_T stSvcParam;
    AX_OPAL_Video_GetSvcParam(nSnsId, (AX_OPAL_VIDEO_CHN_E)0, &stSvcParam);

    cJSON_AddBoolToObject(svc, "valid", AX_TRUE);
    cJSON_AddBoolToObject(svc, "sync_valid", AX_FALSE); // not support sync
    cJSON_AddBoolToObject(svc, "enable", stSvcParam.bEnable);
    cJSON_AddBoolToObject(svc, "sync_mode", stSvcParam.bSync);

    cJSON * bg_qp = cJSON_CreateObject();
    cJSON_AddItemToObject(svc, "bg_qp", bg_qp);
    cJSON_AddNumberToObject(bg_qp, "iQp", stSvcParam.stBgQpCfg.nIQp);
    cJSON_AddNumberToObject(bg_qp, "pQp", stSvcParam.stBgQpCfg.nPQp);

    cJSON * body = cJSON_CreateObject();
    cJSON_AddItemToObject(svc, "body", body);
    cJSON_AddBoolToObject(body, "enable", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_BODY].bEnable);
    cJSON * body_qp = cJSON_CreateObject();
    cJSON_AddItemToObject(body, "qp", body_qp);
    cJSON_AddNumberToObject(body_qp, "iQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_BODY].stQpMap.nIQp);
    cJSON_AddNumberToObject(body_qp, "pQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_BODY].stQpMap.nPQp);

    cJSON * vehicle = cJSON_CreateObject();
    cJSON_AddItemToObject(svc, "vehicle", vehicle);
    cJSON_AddBoolToObject(vehicle, "enable", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_VEHICLE].bEnable);
    cJSON * vehicle_qp = cJSON_CreateObject();
    cJSON_AddItemToObject(vehicle, "qp", vehicle_qp);
    cJSON_AddNumberToObject(vehicle_qp, "iQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_VEHICLE].stQpMap.nIQp);
    cJSON_AddNumberToObject(vehicle_qp, "pQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_VEHICLE].stQpMap.nPQp);

    cJSON * cycle = cJSON_CreateObject();
    cJSON_AddItemToObject(svc, "cycle", cycle);
    cJSON_AddBoolToObject(cycle, "enable", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_CYCLE].bEnable);
    cJSON * cycle_qp = cJSON_CreateObject();
    cJSON_AddItemToObject(cycle, "qp", cycle_qp);
    cJSON_AddNumberToObject(cycle_qp, "iQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_CYCLE].stQpMap.nIQp);
    cJSON_AddNumberToObject(cycle_qp, "pQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_CYCLE].stQpMap.nPQp);

    cJSON * face = cJSON_CreateObject();
    cJSON_AddItemToObject(svc, "face", face);
    cJSON_AddBoolToObject(face, "enable", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_FACE].bEnable);
    cJSON * face_qp = cJSON_CreateObject();
    cJSON_AddItemToObject(face, "qp", face_qp);
    cJSON_AddNumberToObject(face_qp, "iQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_FACE].stQpMap.nIQp);
    cJSON_AddNumberToObject(face_qp, "pQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_FACE].stQpMap.nPQp);

    cJSON * plate = cJSON_CreateObject();
    cJSON_AddItemToObject(svc, "plate", plate);
    cJSON_AddBoolToObject(plate, "enable", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_PLATE].bEnable);
    cJSON * plate_qp = cJSON_CreateObject();
    cJSON_AddItemToObject(plate, "qp", plate_qp);
    cJSON_AddNumberToObject(plate_qp, "iQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_PLATE].stQpMap.nIQp);
    cJSON_AddNumberToObject(plate_qp, "pQp", stSvcParam.stQpCfg[AX_OPAL_ALGO_HVCFP_PLATE].stQpMap.nPQp);

    return root;
}

cJSON* GetOsdJson(AX_U8 nSnsId) {
    cJSON * root = cJSON_CreateArray();

    AX_U32 nVideoCount = GetVideoCount(nSnsId);
    for (AX_U32 i = 0; i < nVideoCount && i < AX_OPAL_DEMO_VIDEO_CHN_NUM; i++) {
        cJSON* jsonOverlayItem = cJSON_CreateObject();

        AX_OPAL_VIDEO_CHN_ATTR_T stChnAttr = {0};
        AX_OPAL_Video_GetChnAttr(nSnsId, g_stOsdCfg[nSnsId].stOsd[i][0].nSrcChn, &stChnAttr);

        AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
        AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAttr);

        AX_U32 nWidth = stChnAttr.nWidth;
        AX_U32 nHeight = stChnAttr.nHeight;

        AX_U8 nRotation = stSnsAttr.eRotation;
        if (AX_OPAL_SNS_ROTATION_90 == nRotation || AX_OPAL_SNS_ROTATION_270 == nRotation) {
            AX_SWAP(nWidth, nHeight);
        }

        cJSON* jsonVideo = cJSON_CreateObject();
        cJSON_AddNumberToObject(jsonVideo, "id", i);
        cJSON_AddNumberToObject(jsonVideo, "width", nWidth);
        cJSON_AddNumberToObject(jsonVideo, "height", nHeight);
        cJSON_AddItemToObject(jsonOverlayItem, "video", jsonVideo);

        AX_CHAR szStr[64] = {0};
        DEMO_OSD_ITEM_T *pOsd = NULL;

        // time
        pOsd = &g_stOsdCfg[nSnsId].stOsd[i][OSD_IND_TIME];
        AX_U32 nFontSize = pOsd->stOsdAttr.stDatetimeAttr.nFontSize; //GetTimeFontSize(nWidth, nHeight);
        AX_S32 nBoundaryW = ALIGN_UP(nFontSize / 2, BASE_FONT_SIZE) * 20;
        AX_S32 nBoundaryH = ALIGN_UP(nFontSize, BASE_FONT_SIZE);
        AX_S32 nOffsetX = CalcOsdOffsetX(nWidth, nBoundaryW, pOsd->stOsdAttr.nXBoundary, pOsd->stOsdAttr.eAlign);
        AX_S32 nOffsetY = CalcOsdOffsetY(nHeight, nBoundaryH, pOsd->stOsdAttr.nYBoundary, pOsd->stOsdAttr.eAlign);

        cJSON*  jsonTime = cJSON_CreateObject();
        cJSON_AddBoolToObject(jsonTime, "enable", pOsd->stOsdAttr.bEnable);
        cJSON_AddNumberToObject(jsonTime, "format", pOsd->stOsdAttr.stDatetimeAttr.eFormat);
        sprintf(szStr, "0x%06X", pOsd->stOsdAttr.nARGB & 0xFFFFFF);
        cJSON_AddStringToObject(jsonTime, "color", szStr);
        cJSON_AddNumberToObject(jsonTime, "fontsize", pOsd->stOsdAttr.stDatetimeAttr.nFontSize);
        cJSON_AddNumberToObject(jsonTime, "align", pOsd->stOsdAttr.eAlign);
        cJSON_AddBoolToObject(jsonTime, "inverse_enable", pOsd->stOsdAttr.stDatetimeAttr.bInvEnable);
        sprintf(szStr, "0x%06X", pOsd->stOsdAttr.stDatetimeAttr.nColorInv & 0xFFFFFF);
        cJSON_AddStringToObject(jsonTime, "color_inv", szStr);
        AX_S32 boundTime[4] = {nOffsetX, nOffsetY, nBoundaryW, nBoundaryH};
        cJSON_AddItemToObject(jsonTime, "rect", cJSON_CreateIntArray(boundTime, 4));
        cJSON_AddItemToObject(jsonOverlayItem, "time", jsonTime);


        // logo
        cJSON* jsonLogo = cJSON_CreateObject();
        pOsd = &g_stOsdCfg[nSnsId].stOsd[i][OSD_IND_PICTURE];
        cJSON_AddBoolToObject(jsonLogo, "enable", pOsd->stOsdAttr.bEnable);
        cJSON_AddNumberToObject(jsonLogo, "align", pOsd->stOsdAttr.eAlign);
        nBoundaryW = pOsd->stOsdAttr.stPicAttr.nWidth;
        nBoundaryH = pOsd->stOsdAttr.stPicAttr.nHeight;
        nOffsetX = CalcOsdOffsetX(nWidth, nBoundaryW, pOsd->stOsdAttr.nXBoundary, pOsd->stOsdAttr.eAlign);
        nOffsetY = CalcOsdOffsetY(nHeight, nBoundaryH, pOsd->stOsdAttr.nYBoundary, pOsd->stOsdAttr.eAlign);
        AX_S32 boundLogo[4] = {nOffsetX, nOffsetY, nBoundaryW, nBoundaryH};
        cJSON_AddItemToObject(jsonLogo, "rect", cJSON_CreateIntArray(boundLogo, 4));
        cJSON_AddItemToObject(jsonOverlayItem, "logo", jsonLogo);

        // channel
        cJSON*  jsonChannel = cJSON_CreateObject();
        pOsd = &g_stOsdCfg[nSnsId].stOsd[i][OSD_IND_STRING_CHANNEL];
        cJSON_AddBoolToObject(jsonChannel, "enable", pOsd->stOsdAttr.bEnable);
        cJSON_AddStringToObject(jsonChannel, "text", pOsd->szStr);
        sprintf(szStr, "0x%06X", pOsd->stOsdAttr.nARGB & 0xFFFFFF);
        cJSON_AddStringToObject(jsonChannel, "color", szStr);
        cJSON_AddNumberToObject(jsonChannel, "fontsize", pOsd->stOsdAttr.stStrAttr.nFontSize);
        cJSON_AddNumberToObject(jsonChannel, "align", pOsd->stOsdAttr.eAlign);
        sprintf(szStr, "0x%06X",  pOsd->stOsdAttr.stStrAttr.nColorInv & 0xFFFFFF);
        cJSON_AddBoolToObject(jsonChannel, "inverse_enable",  pOsd->stOsdAttr.stStrAttr.bInvEnable);
        cJSON_AddStringToObject(jsonChannel, "color_inv", szStr);
        nFontSize = pOsd->stOsdAttr.stStrAttr.nFontSize;
        nBoundaryW = ALIGN_UP(nFontSize / 2, BASE_FONT_SIZE) * 20;
        nBoundaryH = ALIGN_UP(nFontSize, BASE_FONT_SIZE);
        nOffsetX = CalcOsdOffsetX(nWidth, nBoundaryW, pOsd->stOsdAttr.nXBoundary, pOsd->stOsdAttr.eAlign);
        nOffsetY = CalcOsdOffsetY(nHeight, nBoundaryH, pOsd->stOsdAttr.nYBoundary, pOsd->stOsdAttr.eAlign);
        AX_S32 boundChannel[4] = {nOffsetX, nOffsetY, nBoundaryW, nBoundaryH};
        cJSON_AddItemToObject(jsonChannel, "rect", cJSON_CreateIntArray(boundChannel, 4));
        cJSON_AddItemToObject(jsonOverlayItem, "channel", jsonChannel);

        // location
        cJSON*  jsonLocation = cJSON_CreateObject();
        pOsd = &g_stOsdCfg[nSnsId].stOsd[i][OSD_IND_STRING_LOCATION];
        cJSON_AddBoolToObject(jsonLocation, "enable", pOsd->stOsdAttr.bEnable);
        cJSON_AddStringToObject(jsonLocation, "text", pOsd->szStr);
        sprintf(szStr, "0x%06X", pOsd->stOsdAttr.nARGB & 0xFFFFFF);
        cJSON_AddStringToObject(jsonLocation, "color", szStr);
        cJSON_AddNumberToObject(jsonLocation, "fontsize", pOsd->stOsdAttr.stStrAttr.nFontSize);
        cJSON_AddNumberToObject(jsonLocation, "align", pOsd->stOsdAttr.eAlign);
        sprintf(szStr, "0x%06X",  pOsd->stOsdAttr.stStrAttr.nColorInv & 0xFFFFFF);
        cJSON_AddBoolToObject(jsonLocation, "inverse_enable",  pOsd->stOsdAttr.stStrAttr.bInvEnable);
        cJSON_AddStringToObject(jsonLocation, "color_inv", szStr);
        nFontSize = pOsd->stOsdAttr.stStrAttr.nFontSize;
        nBoundaryW = ALIGN_UP(nFontSize / 2, BASE_FONT_SIZE) * 20;
        nBoundaryH = ALIGN_UP(nFontSize, BASE_FONT_SIZE);
        nOffsetX = CalcOsdOffsetX(nWidth, nBoundaryW, pOsd->stOsdAttr.nXBoundary, pOsd->stOsdAttr.eAlign);
        nOffsetY = CalcOsdOffsetY(nHeight, nBoundaryH, pOsd->stOsdAttr.nYBoundary, pOsd->stOsdAttr.eAlign);
        AX_S32 boundLocation[4] = {nOffsetX, nOffsetY, nBoundaryW, nBoundaryH};
        cJSON_AddItemToObject(jsonLocation, "rect", cJSON_CreateIntArray(boundLocation, 4));
        cJSON_AddItemToObject(jsonOverlayItem, "location", jsonLocation);
        cJSON_AddItemToArray(root, jsonOverlayItem);
    }

    return root;
}

cJSON* GetPrivJson(AX_U8 nSnsId) {
    cJSON * root = cJSON_CreateObject();

    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
    AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAttr);

    AX_U32 nWidth = stSnsAttr.nWidth;
    AX_U32 nHeight = stSnsAttr.nHeight;
    AX_U8 nRotation = stSnsAttr.eRotation;
    if (AX_OPAL_SNS_ROTATION_90 == nRotation || AX_OPAL_SNS_ROTATION_270 == nRotation) {
        AX_SWAP(nWidth, nHeight);
    }
    DEMO_OSD_ITEM_T *pOsd = &g_stOsdCfg[nSnsId].stPriv;
    AX_CHAR szStr[64] = {0};

    cJSON_AddBoolToObject(root, "enable", pOsd->stOsdAttr.bEnable);
    cJSON_AddNumberToObject(root, "type", pOsd->stOsdAttr.stPrivacyAttr.eType);
    cJSON_AddNumberToObject(root, "linewidth", pOsd->stOsdAttr.stPrivacyAttr.nLineWidth);
    sprintf(szStr, "0x%06X", pOsd->stOsdAttr.nARGB & 0xFFFFFF);
    cJSON_AddStringToObject(root, "color", szStr);
    cJSON_AddBoolToObject(root, "solid", pOsd->stOsdAttr.stPrivacyAttr.bSolid);
    cJSON* jsonPointArray = cJSON_CreateArray();
    for (AX_S32 i = 0; i < 4; i++) {
        AX_F32 pt[2] = {pOsd->stOsdAttr.stPrivacyAttr.stPoints[i].fX, pOsd->stOsdAttr.stPrivacyAttr.stPoints[i].fY};
        cJSON_AddItemToArray(jsonPointArray, cJSON_CreateFloatArray(pt, 2));
    }
    cJSON_AddItemToObject(root, "points", jsonPointArray);
    AX_S32 reslution[2] = {nWidth, nHeight};
    cJSON_AddItemToObject(root, "sourceResolution", cJSON_CreateIntArray(reslution, 2));
    return root;
}

typedef struct _WS_CAP_T {
    AX_U8 bSupportSys;
    AX_U8 bSupportCam;
    AX_U8 bSupportImg;
    AX_U8 bSupportAI;
    AX_U8 bSupportAudio;
    AX_U8 bSupportVideo;
    AX_U8 bSupportOverlay;
    AX_U8 bSupportStorage;
    AX_U8 bSupportPlayback;
} WS_CAT_T;

static WS_CAT_T g_tCapInfo = {
    .bSupportSys = 1,
    .bSupportCam = 1,
    .bSupportImg = 1,
    .bSupportAI = 1,
    .bSupportAudio = 0,
    .bSupportVideo = 1,
    .bSupportOverlay = 1,
    .bSupportStorage = 0,
    .bSupportPlayback = 1,
};

cJSON* GetCapSettingJson() {
    cJSON *root = cJSON_CreateObject();
    AX_BOOL bDualSnsMode = g_opal_nSnsNum == 1 ? AX_FALSE : AX_TRUE;
    g_tCapInfo.bSupportAudio = (AX_U8)(g_stOpalAttr.stAudioAttr.stCapAttr.bEnable || g_stOpalAttr.stAudioAttr.stPlayAttr.bEnable);
    cJSON_AddNumberToObject(root, "support_dual_sns", bDualSnsMode);
    cJSON_AddNumberToObject(root, "support_page_sys", g_tCapInfo.bSupportSys);
    cJSON_AddNumberToObject(root, "support_page_cam", g_tCapInfo.bSupportCam);
    cJSON_AddNumberToObject(root, "support_page_img", g_tCapInfo.bSupportImg);
    cJSON_AddNumberToObject(root, "support_page_ai", g_tCapInfo.bSupportAI);
    cJSON_AddNumberToObject(root, "support_page_audio", g_tCapInfo.bSupportAudio);
    cJSON_AddNumberToObject(root, "support_page_video", g_tCapInfo.bSupportVideo);
    cJSON_AddNumberToObject(root, "support_page_overlay", g_tCapInfo.bSupportOverlay);
    cJSON_AddNumberToObject(root, "support_page_storage", g_tCapInfo.bSupportStorage);
    cJSON_AddNumberToObject(root, "support_page_playback", g_tCapInfo.bSupportPlayback);
    return root;
}

 cJSON * GetPreviewInfoJson(const AX_U8 *pSnsPrevChnCount) {
    cJSON *root = cJSON_CreateObject();

    AX_U8 nSnsCnt = g_opal_nSnsNum;
    AX_OPAL_AUDIO_ATTR_T stAttr = {0};
    AX_OPAL_Audio_GetAttr(&stAttr);
    AX_OPAL_ALGO_PARAM_T stParam = {0};
    AX_OPAL_Video_AlgoGetParam((AX_OPAL_SNS_ID_E)0, &stParam);

    AX_CHAR *aencType = "aac";
    AX_OPAL_AUDIO_ENCODER_ATTR_T stEncodeAttr;
    memset(&stEncodeAttr, 0, sizeof(AX_OPAL_AUDIO_ENCODER_ATTR_T));
    if (stAttr.stCapAttr.bEnable) {
        AX_OPAL_Audio_GetEncoderAttr(AX_OPAL_AUDIO_CHN_0, &stEncodeAttr);
        switch (stEncodeAttr.eType) {
            case PT_G711A:
                aencType = "g711a";
                break;
            case PT_G711U:
                aencType = "g711u";
                break;
            case PT_LPCM:
                aencType = "lpcm";
                break;
            default:
                aencType = "aac";
                break;
        }
    }

    // not used
    int arrFps[4] = {0, 0, 0, 0};
    cJSON* fpsArr = cJSON_CreateArray();
    cJSON_AddItemToArray(fpsArr, cJSON_CreateIntArray(arrFps, 4));
    cJSON_AddItemToArray(fpsArr, cJSON_CreateIntArray(arrFps, 4));
    cJSON_AddItemToArray(fpsArr, cJSON_CreateIntArray(arrFps, 4));
    cJSON_AddItemToArray(fpsArr, cJSON_CreateIntArray(arrFps, 4));


    cJSON_AddNumberToObject(root, "sns_num", nSnsCnt);
    cJSON_AddBoolToObject(root, "ai_enable", stParam.stHvcfpParam.bEnable);
    cJSON_AddBoolToObject(root, "searchimg", AX_FALSE);
    cJSON_AddStringToObject(root, "detect_mode", GetDetectModelStr(stParam.stHvcfpParam.nAlgoType));
    cJSON_AddBoolToObject(root, "osd_enable", AX_TRUE);
    cJSON_AddBoolToObject(root, "aenc_enable", stAttr.stCapAttr.bEnable);
    cJSON_AddStringToObject(root, "aenc_type", aencType);
    cJSON_AddNumberToObject(root, "aenc_sample_rate", stEncodeAttr.eSampleRate);
    cJSON_AddNumberToObject(root, "aenc_bit_width", stEncodeAttr.eBitWidth);
    cJSON_AddBoolToObject(root, "ezoom_enable", AX_TRUE);
    cJSON_AddItemToObject(root, "sns_video_fps", fpsArr);

    // ezomm
    cJSON *ezoomArr = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "ezoom", ezoomArr);
    for (AX_U8 i = 0; i < MAX_PREV_SNS_NUM; i++) {
        AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
        cJSON * ezoomItem = cJSON_CreateObject();
        if (i < nSnsCnt) {
            AX_OPAL_Video_GetSnsAttr(i, &stSnsAttr);
            cJSON_AddNumberToObject(ezoomItem, "ezoom_ratio", stSnsAttr.stEZoomAttr.fRatio);
            cJSON_AddNumberToObject(ezoomItem, "center_offset_x", stSnsAttr.stEZoomAttr.nCenterOffsetX);
            cJSON_AddNumberToObject(ezoomItem, "center_offset_y", stSnsAttr.stEZoomAttr.nCenterOffsetY);
            cJSON_AddNumberToObject(ezoomItem, "max_ratio", 8);
            cJSON_AddNumberToObject(ezoomItem, "step", 0.1);
        } else {
            cJSON_AddNumberToObject(ezoomItem, "ezoom_ratio", 1);
            cJSON_AddNumberToObject(ezoomItem, "center_offset_x", 0);
            cJSON_AddNumberToObject(ezoomItem, "center_offset_y", 0);
            cJSON_AddNumberToObject(ezoomItem, "max_ratio", 8);
            cJSON_AddNumberToObject(ezoomItem, "step", 0.1);
        }
        cJSON_AddItemToArray(ezoomArr, ezoomItem);
    }

    // stream list
    cJSON * streamArr = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "stream_list", streamArr);
    int arrStream[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    for (AX_U8 i = 0; i < MAX_PREV_SNS_NUM; i++) {
        if (i < nSnsCnt) {
            cJSON_AddItemToArray(streamArr, cJSON_CreateIntArray(arrStream, pSnsPrevChnCount[i]));
        } else {
            cJSON_AddItemToArray(streamArr, cJSON_CreateIntArray(arrStream, 2));
        }
    }

    // codec type list and enable list
    cJSON * codecArr = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "sns_codec", codecArr);

    cJSON * streamEnableArr = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "stream_list_enable", streamEnableArr);
    int arrCodec[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 0: h264, 1: mjpg, 2: h265
    int arrStreamEnable[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // disable
    for (AX_U8 i = 0; i < MAX_PREV_SNS_NUM; i++) {
        if (i < nSnsCnt) {
            cJSON* codecItem = cJSON_CreateArray();
            cJSON * streamEnableItem = cJSON_CreateArray();
            AX_U8 nPrevChnCount = pSnsPrevChnCount[i];
            for (AX_U8 j = 0; j < nPrevChnCount; j++) {
                AX_OPAL_VIDEO_CHN_ATTR_T stChnAttr = {0};
                AX_OPAL_Video_GetChnAttr(i, GetSrcChn(i, j), &stChnAttr);
                cJSON_AddItemToArray(codecItem, cJSON_CreateNumber(EncoderType2Int(stChnAttr.eType)));
                cJSON_AddItemToArray(streamEnableItem, cJSON_CreateNumber(stChnAttr.bEnable));
            }
            cJSON_AddItemToArray(codecArr, codecItem);
            cJSON_AddItemToArray(streamEnableArr, streamEnableItem);
        } else {
            cJSON_AddItemToArray(codecArr, cJSON_CreateIntArray(arrCodec, 2));
            cJSON_AddItemToArray(streamEnableArr, cJSON_CreateIntArray(arrStreamEnable, 2));
        }
    }

    return root;
}

AX_BOOL ProcCameraReq(MprJson* pJson, AX_VOID** pResult) {
    AX_S32 nRet = 0;
    AX_U8  nSnsId = 0;
    WEB_SET_INT(nSnsId, pJson, "src_id");
    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAtt = {0};
    nRet = AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAtt);
    if (nRet == 0) {
        MprJson* pJsonCamera = mprReadJsonObj(pJson, "camera_attr");
        AX_S32 nSnsMode = 0;
        AX_S32 nDayNightMode = 0;
        AX_S32 nRotation = 0;
        AX_S32 nFrameRate = 0;

        WEB_SET_INT(nSnsMode, pJsonCamera, "sns_work_mode");
        //WEB_SET_INT(nHdrRatio, pJsonCamera, "hdr_ratio");
        WEB_SET_INT(nDayNightMode, pJsonCamera, "daynight");
        WEB_SET_INT(nRotation, pJsonCamera, "rotation");
        WEB_SET_INT(nFrameRate, pJsonCamera, "framerate");

        if (nRotation != stSnsAtt.eRotation) {
            if ((nRotation == AX_OPAL_SNS_ROTATION_0 && stSnsAtt.eRotation == AX_OPAL_SNS_ROTATION_180) ||
                (nRotation == AX_OPAL_SNS_ROTATION_180 && stSnsAtt.eRotation == AX_OPAL_SNS_ROTATION_0) ||
                (nRotation == AX_OPAL_SNS_ROTATION_90 && stSnsAtt.eRotation == AX_OPAL_SNS_ROTATION_270) ||
                (nRotation == AX_OPAL_SNS_ROTATION_270 && stSnsAtt.eRotation == AX_OPAL_SNS_ROTATION_90) ) {
                // resolution is not changed
            } else {
                WS_SendWebResetEvent(nSnsId);
            }
        }

        stSnsAtt.eMode = (AX_OPAL_SNS_MODE_E)nSnsMode;
        stSnsAtt.eRotation = (AX_OPAL_SNS_ROTATION_E)nRotation;
        stSnsAtt.fFrameRate = (AX_F32)nFrameRate;

        g_arrDayNightMode[nSnsId] = nDayNightMode;

        AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T stAttr = {0};
        if (0 == nDayNightMode || 1 == nDayNightMode) {
            stSnsAtt.eDayNight = (AX_OPAL_SNS_DAYNIGHT_E)nDayNightMode;
            AX_OPAL_Video_GetSnsSoftPhotoSensitivityAttr((AX_OPAL_SNS_ID_E)nSnsId, &stAttr);
            // disable sensor soft photo sensitivity
            stAttr.eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_NONE;
            AX_OPAL_Video_SetSnsSoftPhotoSensitivityAttr((AX_OPAL_SNS_ID_E)nSnsId, &stAttr);
        } else {
            AX_OPAL_Video_GetSnsSoftPhotoSensitivityAttr((AX_OPAL_SNS_ID_E)nSnsId, &stAttr);
#if 1
            // warmlight
            stAttr.bAutoCtrl = AX_TRUE;
            stAttr.eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_WARMLIGHT;
            stAttr.stWarmAttr.nOnLightSensitivity  = 50;
            stAttr.stWarmAttr.nOnLightExpValMax    = 10000446016;//21475403776;
            stAttr.stWarmAttr.nOnLightExpValMid    = 5417253376;//10000446016;
            stAttr.stWarmAttr.nOnLightExpValMin    = 1740800000;
            stAttr.stWarmAttr.nOffLightSensitivity = 50;
            stAttr.stWarmAttr.nOffLightExpValMax   = 939488256;
            stAttr.stWarmAttr.nOffLightExpValMid   = 456960000;
            stAttr.stWarmAttr.nOffLightExpValMin   = 101807424;
#else
            // ir
            stAttr.bAutoCtrl = AX_TRUE;
            stAttr.eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_IR;
            stAttr.stIrAttr.fIrCalibR                = 1.056;
            stAttr.stIrAttr.fIrCalibG                = 1;
            stAttr.stIrAttr.fIrCalibB                = 1.036;
            stAttr.stIrAttr.fNight2DayIrStrengthTh   = 0.4;
            stAttr.stIrAttr.fNight2DayIrDetectTh     = 0.08;
            stAttr.stIrAttr.nInitDayNightMode        = 0; // 0: day; 1: night
            stAttr.stIrAttr.fDay2NightLuxTh          = 0.7;
            stAttr.stIrAttr.fNight2DayLuxTh          = 20.0;
            stAttr.stIrAttr.fNight2DayBrightTh       = 230.0;
            stAttr.stIrAttr.fNight2DayDarkTh         = 5.0;
            stAttr.stIrAttr.fNight2DayUsefullWpRatio = 0.5;
            stAttr.stIrAttr.nCacheTime               = 2;
#endif
            AX_OPAL_Video_SetSnsSoftPhotoSensitivityAttr((AX_OPAL_SNS_ID_E)nSnsId, &stAttr);
        }

        cchar* szMirrorEnable = mprReadJson(pJsonCamera, "mirror");
        cchar* szFlipEnable = mprReadJson(pJsonCamera, "flip");
        if (NULL != szMirrorEnable && NULL != szFlipEnable) {
            AX_BOOL bMirrorEnable = (strcmp(szMirrorEnable, "true") == 0 ? AX_TRUE : AX_FALSE);
            AX_BOOL bFlipEnable = (strcmp(szFlipEnable, "true") == 0 ? AX_TRUE : AX_FALSE);
            stSnsAtt.bFlip = bFlipEnable;
            stSnsAtt.bMirror = bMirrorEnable;
        }
        nRet = AX_OPAL_Video_SetSnsAttr(nSnsId, &stSnsAtt);

        return nRet == AX_SUCCESS ? AX_TRUE : AX_FALSE;
    }

    return AX_FALSE;
}

AX_BOOL ProcImageReq(MprJson* pJson, AX_VOID** pResult) {
    AX_S32 nRet = 0;
    AX_U8  nSnsId = 0;
    WEB_SET_INT(nSnsId, pJson, "src_id");
    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAtt = {0};
    nRet = AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAtt);
    if (nRet == 0) {
        AX_S32 nSharpness = 0;
        AX_S32 nBrightness = 0;
        AX_S32 nContrast = 0;
        AX_S32 nSaturation = 0;
        AX_S32 nAutoMode = 0;
        MprJson* pJsonImage = mprReadJsonObj(pJson, "image_attr");
        WEB_SET_INT(nAutoMode, pJsonImage, "isp_auto_mode");
        WEB_SET_INT(nSharpness, pJsonImage, "sharpness_val");
        WEB_SET_INT(nBrightness, pJsonImage, "brightness_val");
        WEB_SET_INT(nContrast, pJsonImage, "contrast_val");
        WEB_SET_INT(nSaturation, pJsonImage, "saturation_val");

        stSnsAtt.stColorAttr.fSharpness = nSharpness;
        stSnsAtt.stColorAttr.fBrightness = nBrightness;
        stSnsAtt.stColorAttr.fContrast = nContrast;
        stSnsAtt.stColorAttr.fSaturation = nSaturation;
        stSnsAtt.stColorAttr.bManual = nAutoMode == 0 ? AX_TRUE : AX_FALSE;

        MprJson* pJsonLDC = mprReadJsonObj(pJson, "ldc_attr");
        cchar* szLdcEnable = mprReadJson(pJsonLDC, "ldc_enable");
        cchar* szAspect = mprReadJson(pJsonLDC, "aspect");
        cchar* szXRatio = mprReadJson(pJsonLDC, "x_ratio");
        cchar* szYRatio = mprReadJson(pJsonLDC, "y_ratio");
        cchar* szXYRatio = mprReadJson(pJsonLDC, "xy_ratio");
        cchar* szDistorRatio = mprReadJson(pJsonLDC, "distor_ratio");
        if (NULL != szLdcEnable && NULL != szAspect && NULL != szXRatio && NULL != szYRatio && NULL != szXYRatio &&
            NULL != szDistorRatio) {
            stSnsAtt.stLdcAttr.bEnable = (strcmp(szLdcEnable, "true") == 0 ? AX_TRUE : AX_FALSE);
            stSnsAtt.stLdcAttr.bAspect = (strcmp(szAspect, "true") == 0 ? AX_TRUE : AX_FALSE);
            stSnsAtt.stLdcAttr.nXRatio = atoi(szXRatio);
            stSnsAtt.stLdcAttr.nYRatio = atoi(szYRatio);
            stSnsAtt.stLdcAttr.nXYRatio = atoi(szXYRatio);
            stSnsAtt.stLdcAttr.nDistortionRatio = atoi(szDistorRatio);
        }

        cchar* szDisEnable = mprReadJson(mprReadJsonObj(pJson, "dis_attr"), "dis_enable");
        if (NULL != szDisEnable) {
            stSnsAtt.stDisAttr.bEnable = (strcmp(szDisEnable, "true") == 0 ? AX_TRUE : AX_FALSE);
        }

        nRet = AX_OPAL_Video_SetSnsAttr(nSnsId, &stSnsAtt);
        return nRet == AX_SUCCESS ? AX_TRUE : AX_FALSE;
    }

    return AX_FALSE;
}

AX_BOOL ProcAudioReq(MprJson* pJson, AX_VOID** pResult) {
    //AX_S32 nRet = 0;
    AX_S32 nPlayVolume = 0;
    AX_S32 nCapVolume = 0;
    //LOG_M_C(WEB_HELPER, "[Audio] Web request: ...");
    WEB_SET_INT(nCapVolume, mprReadJsonObj(pJson, "capture_attr"), "volume_val");
    WEB_SET_INT(nPlayVolume, mprReadJsonObj(pJson, "play_attr"), "volume_val");

    AX_OPAL_Audio_SetCapVolume((AX_F32)nCapVolume);
    AX_OPAL_Audio_SetPlayVolume((AX_F32)nPlayVolume);

    return AX_TRUE;
}

AX_BOOL ProcVideoReq(MprJson* pJson, AX_VOID** pResult) {
    //AX_S32 nRet = 0;
    AX_U8  nSnsId = 0;
    WEB_SET_INT(nSnsId, pJson, "src_id");
    AX_U8 nSize = GetVideoCount(nSnsId);
    for (AX_U8 i = 0; i < nSize; i++) {
        char szKey[64] = {0};
        sprintf(szKey, "video%d", i);
        MprJson* jsonObj = mprReadJsonObj(pJson, szKey);
        if (!jsonObj) {
            //LOG_M_I(WEB_HELPER, "[video] [%d] mprReadJsonObj %s is empty", nSnsId, szKey);
            continue;
        }
        AX_U8 nSrcChn = GetSrcChn(nSnsId, i);

        AX_OPAL_VIDEO_CHN_ATTR_T stAttr;
        AX_OPAL_Video_GetChnAttr((AX_OPAL_SNS_ID_E)nSnsId, (AX_OPAL_VIDEO_CHN_E)nSrcChn, &stAttr);

        AX_U8 nEncoderType = 0;
        WEB_SET_INT(nEncoderType, jsonObj, "encoder_type");
        stAttr.eType = Int2EncoderType(nEncoderType);

        WEB_SET_BOOL(stAttr.bEnable, jsonObj, "enable_stream");

        cchar* cMin_qp = mprReadJson(jsonObj, "min_qp");
        cchar* cMax_qp = mprReadJson(jsonObj, "max_qp");
        cchar* cMin_iqp = mprReadJson(jsonObj, "min_iqp");
        cchar* cMax_iqp = mprReadJson(jsonObj, "max_iqp");
        cchar* cMin_iprop = mprReadJson(jsonObj, "min_iprop");
        cchar* cMax_iprop = mprReadJson(jsonObj, "max_iprop");
        cchar* cBitrate = mprReadJson(jsonObj, "bit_rate");
        cchar* cGop = mprReadJson(jsonObj, "gop");
        if (NULL != cMin_qp && NULL != cMax_qp && NULL != cMin_iqp && NULL != cMax_iqp && NULL != cMin_iprop &&
            NULL != cMax_iprop && NULL != cBitrate && NULL != cGop) {
            AX_U32 nRcType = atoi(mprReadJson(jsonObj, "rc_type"));
            stAttr.stEncoderAttr.eRcMode = Int2RcMode(nEncoderType, nRcType);
            stAttr.stEncoderAttr.nMinQp =(AX_U32)atoi(cMin_qp);
            stAttr.stEncoderAttr.nMaxQp = (AX_U32)atoi(cMax_qp);
            stAttr.stEncoderAttr.nMinIQp = (AX_U32)atoi(cMin_iqp);
            stAttr.stEncoderAttr.nMaxIQp = (AX_U32)atoi(cMax_iqp);
            stAttr.stEncoderAttr.nMinIprop = (AX_U32)atoi(cMin_iprop);
            stAttr.stEncoderAttr.nMaxIprop = (AX_U32)atoi(cMax_iprop);
            stAttr.stEncoderAttr.nBitrate = (AX_U32)atoi(cBitrate);
            stAttr.stEncoderAttr.nGop = (AX_U32)atoi(cGop);
        }

        cchar* szResStr = mprReadJson(jsonObj, "resolution");
        if (szResStr) {
            sscanf(szResStr, "%dx%d", &stAttr.nWidth, &stAttr.nHeight);
        }

        WEB_SET_BOOL(stAttr.bEnable, jsonObj, "enable_stream");

        AX_U32 nNewFontSize = GetTimeFontSize(stAttr.nWidth, stAttr.nHeight);

        WS_SendWebResetEvent(nSnsId);
        AX_OPAL_Video_SetChnAttr((AX_OPAL_SNS_ID_E)nSnsId, (AX_OPAL_VIDEO_CHN_E)nSrcChn, &stAttr);
        AX_OPAL_Video_RequestIDR((AX_OPAL_SNS_ID_E)nSnsId, (AX_OPAL_VIDEO_CHN_E)nSrcChn);
        OPAL_mSleep(40);
        AX_OPAL_Video_RequestIDR((AX_OPAL_SNS_ID_E)nSnsId, (AX_OPAL_VIDEO_CHN_E)nSrcChn);
        rtsp_update_payloadtype(nSnsId, nSrcChn, Int2PayloadType(nEncoderType));
        DEMO_OSD_ITEM_T *pOsd = &g_stOsdCfg[nSnsId].stOsd[nSrcChn][OSD_IND_TIME];
        if (nNewFontSize != pOsd->stOsdAttr.stDatetimeAttr.nFontSize && stAttr.bEnable) {
             if(nNewFontSize != pOsd->stOsdAttr.stDatetimeAttr.nFontSize) {
                pOsd->stOsdAttr.stDatetimeAttr.nFontSize = nNewFontSize;
                AX_U32 nWdith = stAttr.nWidth;
                AX_U32 nHeight = stAttr.nHeight;
                AX_OPAL_VIDEO_SNS_ATTR_T stSnsAtt = {0};
                AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAtt);
                if (stSnsAtt.eRotation == AX_OPAL_SNS_ROTATION_90 || stSnsAtt.eRotation == AX_OPAL_SNS_ROTATION_270) {
                    AX_SWAP(nWdith, nHeight);
                }
                AX_S32 nW = ALIGN_UP(nNewFontSize / 2, BASE_FONT_SIZE) * 20;
                AX_S32 nH = ALIGN_UP(nNewFontSize, BASE_FONT_SIZE);
                AX_S32 nX = CalcOsdOffsetX(nWdith, nW, pOsd->stOsdAttr.nXBoundary, pOsd->stOsdAttr.eAlign);
                AX_S32 nY = CalcOsdOffsetY(nHeight, nH, pOsd->stOsdAttr.nYBoundary, pOsd->stOsdAttr.eAlign);
                pOsd->stOsdAttr.nXBoundary = CalcOsdBoudingX(nWdith, nW, nX, pOsd->stOsdAttr.eAlign);
                pOsd->stOsdAttr.nYBoundary = CalcOsdBoudingY(nHeight, nH, nY, pOsd->stOsdAttr.eAlign);
                AX_OPAL_Video_OsdUpdate(nSnsId, pOsd->nSrcChn, pOsd->pHandle,  &pOsd->stOsdAttr);
             }
        }
    }
    return AX_TRUE;
}

AX_BOOL ProcAiReq(MprJson* pJson, AX_VOID** pResult) {
    //AX_S32 nRet = 0;
    AX_U8  nSnsId = 0;
    WEB_SET_INT(nSnsId, pJson, "src_id");
    AX_OPAL_ALGO_PARAM_T stParam = {0};
    AX_OPAL_Video_AlgoGetParam((AX_OPAL_SNS_ID_E)nSnsId, &stParam);

    AX_BOOL bAiEnable = AX_FALSE;
    MprJson* pJsonAI = mprReadJsonObj(pJson, "ai_attr");
    WEB_SET_BOOL(bAiEnable, pJsonAI, "ai_enable");

    stParam.stHvcfpParam.bEnable = bAiEnable;
    stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].bEnable = bAiEnable;

    MprJson* pJsonPushStategy = mprReadJsonObj(pJsonAI, "push_strategy");
    if (pJsonPushStategy) {
        cchar* szPushMode = mprReadJson(pJsonPushStategy, "push_mode");
        if (szPushMode) {
            stParam.stHvcfpParam.stPushStrategy.ePushMode = PushModeStr2Int(szPushMode);
            WEB_SET_INT(stParam.stHvcfpParam.stPushStrategy.nPushCount, pJsonPushStategy, "push_count");
            WEB_SET_INT(stParam.stHvcfpParam.stPushStrategy.nInterval, pJsonPushStategy, "push_interval");
        }
    }

    MprJson* pJsonEvents = mprReadJsonObj(pJsonAI, "events");
    if (pJsonEvents) {
        MprJson* pJsonMotion = mprReadJsonObj(pJsonEvents, "motion_detect");
        MprJson* pJsonOcclusion = mprReadJsonObj(pJsonEvents, "occlusion_detect");

        WEB_SET_BOOL(stParam.stIvesParam.stMdParam.bEnable, pJsonMotion, "enable");
        WEB_SET_BOOL(stParam.stIvesParam.stOdParam.bEnable, pJsonOcclusion, "enable");

        AX_U32 nThrsHoldY = 0;
        AX_U32 nConfidence = 0;
        WEB_SET_INT(nThrsHoldY, pJsonMotion, "threshold_y");
        WEB_SET_INT(nConfidence, pJsonMotion, "confidence");
        for (AX_U8 i = 0; i < stParam.stIvesParam.stMdParam.nRegionSize; i++) {
            stParam.stIvesParam.stMdParam.stRegions[i].fThreshold = nThrsHoldY / 255.0;
            stParam.stIvesParam.stMdParam.stRegions[i].fConfidence = nConfidence / 100.0;
        }

        WEB_SET_INT(nThrsHoldY, pJsonOcclusion, "threshold_y");
        WEB_SET_INT(nConfidence, pJsonOcclusion, "confidence");
        stParam.stIvesParam.stOdParam.fThreshold = nThrsHoldY / 255.0;
        stParam.stIvesParam.stOdParam.fConfidence = nConfidence / 100.0;
    }

    MprJson* pJsonBodyRoi = mprReadJsonObj(pJsonAI, "body_roi");
    if (pJsonBodyRoi) {
        AX_BOOL bAiAeRoiEnable = AX_FALSE;
        AX_U32 nAiAeRoiWidth = 0;
        AX_U32 nAiAeRoiHeight = 0;

        WEB_SET_BOOL(bAiAeRoiEnable, pJsonBodyRoi, "enable");
        WEB_SET_INT(nAiAeRoiWidth, pJsonBodyRoi, "min_width");
        WEB_SET_INT(nAiAeRoiHeight, pJsonBodyRoi, "min_height");

        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].bEnable = bAiAeRoiEnable;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].nWidth = nAiAeRoiWidth;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_BODY].nHeight = nAiAeRoiHeight;

        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_FACE].bEnable = bAiAeRoiEnable;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_FACE].nWidth = nAiAeRoiWidth;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_FACE].nHeight = nAiAeRoiHeight;

    }

    MprJson* pJsonVehicleRoi = mprReadJsonObj(pJsonAI, "vehicle_roi");
    if (pJsonVehicleRoi) {
        AX_BOOL bAiAeRoiEnable = AX_FALSE;
        AX_U32 nAiAeRoiWidth = 0;
        AX_U32 nAiAeRoiHeight = 0;
        WEB_SET_BOOL(bAiAeRoiEnable, pJsonVehicleRoi, "enable");
        WEB_SET_INT(nAiAeRoiWidth, pJsonVehicleRoi, "min_width");
        WEB_SET_INT(nAiAeRoiHeight, pJsonVehicleRoi, "min_height");

        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].bEnable = bAiAeRoiEnable;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].nWidth = nAiAeRoiWidth;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_VEHICLE].nHeight = nAiAeRoiHeight;

        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_CYCLE].bEnable = bAiAeRoiEnable;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_CYCLE].nWidth = nAiAeRoiWidth;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_CYCLE].nHeight = nAiAeRoiHeight;

        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_PLATE].bEnable = bAiAeRoiEnable;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_PLATE].nWidth = nAiAeRoiWidth;
        stParam.stHvcfpParam.stAeRoiConfig[AX_OPAL_ALGO_HVCFP_PLATE].nHeight = nAiAeRoiHeight;
    }

    AX_OPAL_Video_AlgoSetParam((AX_OPAL_SNS_ID_E)nSnsId, &stParam);

    MprJson* pJsonSvc = mprReadJsonObj(pJsonAI, "svc");
    if (pJsonSvc) {
        AX_OPAL_VIDEO_SVC_PARAM_T stSvcParamNew;
        WEB_SET_BOOL(stSvcParamNew.bEnable, pJsonSvc, "enable");
        WEB_SET_BOOL(stSvcParamNew.bSync, pJsonSvc, "sync_mode");

        if (!bAiEnable) {
            stSvcParamNew.bEnable = AX_FALSE;
        }

        MprJson* pJsonSvcBgQp = mprReadJsonObj(pJsonSvc, "bg_qp");
        WEB_SET_INT(stSvcParamNew.stBgQpCfg.nIQp, pJsonSvcBgQp, "iQp");
        WEB_SET_INT(stSvcParamNew.stBgQpCfg.nPQp, pJsonSvcBgQp, "pQp");

        MprJson* pJsonSvcBody = mprReadJsonObj(pJsonSvc, "body");
        MprJson* pJsonSvcBodyQp = mprReadJsonObj(pJsonSvcBody, "qp");
        WEB_SET_BOOL(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_BODY].bEnable, pJsonSvcBody, "enable");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_BODY].stQpMap.nIQp, pJsonSvcBodyQp, "iQp");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_BODY].stQpMap.nPQp, pJsonSvcBodyQp, "pQp");

        MprJson* pJsonSvcVehicle = mprReadJsonObj(pJsonSvc, "vehicle");
        MprJson* pJsonSvcVehicleQp = mprReadJsonObj(pJsonSvcVehicle, "qp");
        WEB_SET_BOOL(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_VEHICLE].bEnable, pJsonSvcVehicle, "enable");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_VEHICLE].stQpMap.nIQp, pJsonSvcVehicleQp, "iQp");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_VEHICLE].stQpMap.nPQp, pJsonSvcVehicleQp, "pQp");

        MprJson* pJsonSvcCycle = mprReadJsonObj(pJsonSvc, "cycle");
        MprJson* pJsonSvcCycleQp = mprReadJsonObj(pJsonSvcCycle, "qp");
        WEB_SET_BOOL(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_CYCLE].bEnable, pJsonSvcCycle, "enable");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_CYCLE].stQpMap.nIQp, pJsonSvcCycleQp, "iQp");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_CYCLE].stQpMap.nPQp, pJsonSvcCycleQp, "pQp");

        MprJson* pJsonSvcFace = mprReadJsonObj(pJsonSvc, "face");
        MprJson* pJsonSvcFaceQp = mprReadJsonObj(pJsonSvcFace, "qp");
        WEB_SET_BOOL(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_FACE].bEnable, pJsonSvcFace, "enable");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_FACE].stQpMap.nIQp, pJsonSvcFaceQp, "iQp");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_FACE].stQpMap.nPQp, pJsonSvcFaceQp, "pQp");

        MprJson* pJsonSvcPlate = mprReadJsonObj(pJsonSvc, "plate");
        MprJson* pJsonSvcPlateQp = mprReadJsonObj(pJsonSvcPlate, "qp");
        WEB_SET_BOOL(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_PLATE].bEnable, pJsonSvcPlate, "enable");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_PLATE].stQpMap.nIQp, pJsonSvcPlateQp, "iQp");
        WEB_SET_INT(stSvcParamNew.stQpCfg[AX_OPAL_ALGO_HVCFP_PLATE].stQpMap.nPQp, pJsonSvcPlateQp, "pQp");

        for (AX_U32 eVideoChn = AX_OPAL_VIDEO_CHN_0; eVideoChn < AX_OPAL_VIDEO_CHN_BUTT; eVideoChn++) {
            if (g_stOpalAttr.stVideoAttr[nSnsId].stPipeAttr.stVideoChnAttr[eVideoChn].bEnable
                && (g_stOpalAttr.stVideoAttr[nSnsId].stPipeAttr.stVideoChnAttr[eVideoChn].eType != AX_OPAL_VIDEO_CHN_TYPE_ALGO
                    && g_stOpalAttr.stVideoAttr[nSnsId].stPipeAttr.stVideoChnAttr[eVideoChn].eType != AX_OPAL_VIDEO_CHN_TYPE_JPEG)) {
                AX_OPAL_VIDEO_SVC_PARAM_T stSvcParam;
                AX_OPAL_Video_GetSvcParam(nSnsId, (AX_OPAL_VIDEO_CHN_E)eVideoChn, &stSvcParam);

                stSvcParam.bEnable = stSvcParamNew.bEnable;
                stSvcParam.bSync = stSvcParamNew.bSync;
                stSvcParam.stBgQpCfg.nIQp = stSvcParamNew.stBgQpCfg.nIQp;
                stSvcParam.stBgQpCfg.nPQp = stSvcParamNew.stBgQpCfg.nPQp;

                for (size_t i = 0; i < AX_OPAL_VIDEO_SVC_REGION_TYPE_BUTT && i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; i++) {
                    stSvcParam.stQpCfg[i].bEnable = stSvcParamNew.stQpCfg[i].bEnable;
                    stSvcParam.stQpCfg[i].stQpMap.nIQp = stSvcParamNew.stQpCfg[i].stQpMap.nIQp;
                    stSvcParam.stQpCfg[i].stQpMap.nPQp = stSvcParamNew.stQpCfg[i].stQpMap.nPQp;
                }

                AX_OPAL_Video_SetSvcParam(nSnsId, (AX_OPAL_VIDEO_CHN_E)eVideoChn, &stSvcParam);
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL ProcOsdReq(MprJson* pJson, AX_VOID** pResult) {
    //AX_S32 nRet = 0;
    AX_U8 nSnsId = 0;
    WEB_SET_INT(nSnsId, pJson, "src_id");

    MprJson* jsonOverly = mprReadJsonObj(pJson, "overlay_attr");
    MprJson* jsonPrivacy = mprReadJsonObj(pJson, "privacy_attr");

    DEMO_OSD_ITEM_T *pOsd = NULL;
    AX_U32 nFontSize = 8;
    AX_S32  nX = 0;
    AX_S32  nY = 0;
    AX_S32  nW = 0;
    AX_S32  nH = 0;

    for (AX_U8 i = 0; i < AX_OPAL_DEMO_VIDEO_CHN_NUM; i++) {

        MprJson* jsonOverlyChn = ReadJsonValeInArry(jsonOverly, i);

        if (jsonOverlyChn && jsonPrivacy) {
            AX_U32 nIndex = 0;
            AX_S32 nVideoW = 0;
            AX_S32 nVideoH = 0;

            MprJson* pJsonVideo = mprReadJsonObj(jsonOverlyChn, "video");
            WEB_SET_INT(nIndex, pJsonVideo, "id");
            WEB_SET_INT(nVideoW, pJsonVideo, "width");
            WEB_SET_INT(nVideoH, pJsonVideo, "height");

            // time
            {
                pOsd = &g_stOsdCfg[nSnsId].stOsd[nIndex][OSD_IND_TIME];
                nFontSize = pOsd->stOsdAttr.stDatetimeAttr.nFontSize; //GetTimeFontSize(nWidth, nHeight);
                nW = ALIGN_UP(nFontSize / 2, BASE_FONT_SIZE) * 20;
                nH = ALIGN_UP(nFontSize, BASE_FONT_SIZE);

                MprJson* pJsonTime = mprReadJsonObj(jsonOverlyChn, "time");
                WEB_SET_BOOL(pOsd->stOsdAttr.bEnable, pJsonTime, "enable");
                WEB_SET_HEX(pOsd->stOsdAttr.nARGB, pJsonTime, "color");
                //pOsd->stOsdAttr.nARGB = pOsd->stOsdAttr.nARGB | 0xFF000000;
                WEB_SET_BOOL(pOsd->stOsdAttr.stDatetimeAttr.bInvEnable, pJsonTime, "inverse_enable");
                WEB_SET_HEX(pOsd->stOsdAttr.stDatetimeAttr.nColorInv, pJsonTime, "color_inv");
                MprJson* jsonRect = mprReadJsonObj(pJsonTime, "rect");
                WEB_SET_INT_OBJ(nX, ReadJsonValeInArry(jsonRect, 0));
                WEB_SET_INT_OBJ(nY, ReadJsonValeInArry(jsonRect, 1));
                pOsd->stOsdAttr.nXBoundary = CalcOsdBoudingX(nVideoW, nW, nX, pOsd->stOsdAttr.eAlign);
                pOsd->stOsdAttr.nYBoundary = CalcOsdBoudingY(nVideoH, nH, nY, pOsd->stOsdAttr.eAlign);
                AX_OPAL_Video_OsdUpdate(nSnsId, pOsd->nSrcChn, pOsd->pHandle,  &pOsd->stOsdAttr);
            }
            // logo
            {
                pOsd = &g_stOsdCfg[nSnsId].stOsd[nIndex][OSD_IND_PICTURE];
                nW = pOsd->stOsdAttr.stPicAttr.nWidth;
                nH = pOsd->stOsdAttr.stPicAttr.nHeight;
                MprJson* pJsonLogo = mprReadJsonObj(jsonOverlyChn, "logo");
                WEB_SET_BOOL(pOsd->stOsdAttr.bEnable, pJsonLogo, "enable");
                MprJson* jsonRect = mprReadJsonObj(pJsonLogo, "rect");
                WEB_SET_INT_OBJ(nX, ReadJsonValeInArry(jsonRect, 0));
                WEB_SET_INT_OBJ(nY, ReadJsonValeInArry(jsonRect, 1));
                pOsd->stOsdAttr.nXBoundary = CalcOsdBoudingX(nVideoW, nW, nX, pOsd->stOsdAttr.eAlign);
                pOsd->stOsdAttr.nYBoundary = CalcOsdBoudingY(nVideoH, nH, nY, pOsd->stOsdAttr.eAlign);

                AX_OPAL_Video_OsdUpdate(nSnsId, pOsd->nSrcChn, pOsd->pHandle,  &pOsd->stOsdAttr);
            }
            // channel
            {
                pOsd = &g_stOsdCfg[nSnsId].stOsd[nIndex][OSD_IND_STRING_CHANNEL];
                nFontSize = pOsd->stOsdAttr.stStrAttr.nFontSize; //GetTimeFontSize(nWidth, nHeight);
                nW = ALIGN_UP(nFontSize / 2, BASE_FONT_SIZE) * 20;
                nH = ALIGN_UP(nFontSize, BASE_FONT_SIZE);

                MprJson* pJsonChannel = mprReadJsonObj(jsonOverlyChn, "channel");
                WEB_SET_BOOL(pOsd->stOsdAttr.bEnable, pJsonChannel, "enable");
                WEB_SET_HEX(pOsd->stOsdAttr.nARGB, pJsonChannel, "color");
                //pOsd->stOsdAttr.nARGB = pOsd->stOsdAttr.nARGB | 0xFF000000;
                WEB_SET_BOOL(pOsd->stOsdAttr.stStrAttr.bInvEnable, pJsonChannel, "inverse_enable");
                WEB_SET_HEX(pOsd->stOsdAttr.stStrAttr.nColorInv, pJsonChannel, "color_inv");
                MprJson* jsonRect = mprReadJsonObj(pJsonChannel, "rect");
                WEB_SET_INT_OBJ(nX, ReadJsonValeInArry(jsonRect, 0));
                WEB_SET_INT_OBJ(nY, ReadJsonValeInArry(jsonRect, 1));
                pOsd->stOsdAttr.nXBoundary = CalcOsdBoudingX(nVideoW, nW, nX, pOsd->stOsdAttr.eAlign);
                pOsd->stOsdAttr.nYBoundary = CalcOsdBoudingY(nVideoH, nH, nY, pOsd->stOsdAttr.eAlign);
                cchar *pText = mprReadJson(pJsonChannel, "text");
                if (pText) {
                    strcpy(pOsd->szStr, pText);
                    pOsd->stOsdAttr.stStrAttr.pStr = pOsd->szStr;
                }
                AX_OPAL_Video_OsdUpdate(nSnsId, pOsd->nSrcChn, pOsd->pHandle,  &pOsd->stOsdAttr);
            }
            // location
            {
                pOsd = &g_stOsdCfg[nSnsId].stOsd[nIndex][OSD_IND_STRING_LOCATION];
                nFontSize = pOsd->stOsdAttr.stStrAttr.nFontSize; //GetTimeFontSize(nWidth, nHeight);
                nW = ALIGN_UP(nFontSize / 2, BASE_FONT_SIZE) * 20;
                nH = ALIGN_UP(nFontSize, BASE_FONT_SIZE);

                MprJson* pJsonLocation = mprReadJsonObj(jsonOverlyChn, "location");
                WEB_SET_BOOL(pOsd->stOsdAttr.bEnable, pJsonLocation, "enable");
                WEB_SET_HEX(pOsd->stOsdAttr.nARGB, pJsonLocation, "color");
                //pOsd->stOsdAttr.nARGB = pOsd->stOsdAttr.nARGB | 0xFF000000;
                WEB_SET_BOOL(pOsd->stOsdAttr.stStrAttr.bInvEnable, pJsonLocation, "inverse_enable");
                WEB_SET_HEX(pOsd->stOsdAttr.stStrAttr.nColorInv, pJsonLocation, "color_inv");
                MprJson* jsonRect = mprReadJsonObj(pJsonLocation, "rect");
                WEB_SET_INT_OBJ(nX, ReadJsonValeInArry(jsonRect, 0));
                WEB_SET_INT_OBJ(nY, ReadJsonValeInArry(jsonRect, 1));
                pOsd->stOsdAttr.nXBoundary = CalcOsdBoudingX(nVideoW, nW, nX, pOsd->stOsdAttr.eAlign);
                pOsd->stOsdAttr.nYBoundary = CalcOsdBoudingY(nVideoH, nH, nY, pOsd->stOsdAttr.eAlign);
                cchar *pText = mprReadJson(pJsonLocation, "text");
                if (pText) {
                    strcpy(pOsd->szStr, pText);
                    pOsd->stOsdAttr.stStrAttr.pStr = pOsd->szStr;
                }
                AX_OPAL_Video_OsdUpdate(nSnsId, pOsd->nSrcChn, pOsd->pHandle,  &pOsd->stOsdAttr);
            }
        }
    }

    // privacy
    {
        pOsd = &g_stOsdCfg[nSnsId].stPriv;
        AX_U32 nType = 0;
        WEB_SET_BOOL(pOsd->stOsdAttr.bEnable, jsonPrivacy, "enable");
        WEB_SET_HEX(pOsd->stOsdAttr.nARGB, jsonPrivacy, "color");
        pOsd->stOsdAttr.nARGB = pOsd->stOsdAttr.nARGB | 0xFF000000;
        WEB_SET_INT(nType, jsonPrivacy, "type");
        pOsd->stOsdAttr.stPrivacyAttr.eType = (AX_OPAL_OSD_PRIVACY_TYPE_E)nType;

        WEB_SET_INT(pOsd->stOsdAttr.stPrivacyAttr.nLineWidth, jsonPrivacy, "linewidth");
        WEB_SET_BOOL(pOsd->stOsdAttr.stPrivacyAttr.bSolid, jsonPrivacy, "solid");
        MprJson* jsonPoints = mprReadJsonObj(jsonPrivacy, "points");
        WEB_SET_INT_OBJ(pOsd->stOsdAttr.stPrivacyAttr.stPoints[0].fX, ReadJsonValeIn2DArry(jsonPoints, 0, 0));
        WEB_SET_INT_OBJ(pOsd->stOsdAttr.stPrivacyAttr.stPoints[0].fY, ReadJsonValeIn2DArry(jsonPoints, 0, 1));
        WEB_SET_INT_OBJ(pOsd->stOsdAttr.stPrivacyAttr.stPoints[1].fX, ReadJsonValeIn2DArry(jsonPoints, 1, 0));
        WEB_SET_INT_OBJ(pOsd->stOsdAttr.stPrivacyAttr.stPoints[1].fY, ReadJsonValeIn2DArry(jsonPoints, 1, 1));
        WEB_SET_INT_OBJ(pOsd->stOsdAttr.stPrivacyAttr.stPoints[2].fX, ReadJsonValeIn2DArry(jsonPoints, 2, 0));
        WEB_SET_INT_OBJ(pOsd->stOsdAttr.stPrivacyAttr.stPoints[2].fY, ReadJsonValeIn2DArry(jsonPoints, 2, 1));
        WEB_SET_INT_OBJ(pOsd->stOsdAttr.stPrivacyAttr.stPoints[3].fX, ReadJsonValeIn2DArry(jsonPoints, 3, 0));
        WEB_SET_INT_OBJ(pOsd->stOsdAttr.stPrivacyAttr.stPoints[3].fY, ReadJsonValeIn2DArry(jsonPoints, 3, 1));
        AX_OPAL_Video_OsdUpdate(nSnsId, pOsd->nSrcChn, pOsd->pHandle, &pOsd->stOsdAttr);
    }

    return AX_TRUE;
}

AX_BOOL ProcSystemReq(MprJson* pJson, AX_VOID** pResult) {
    // AX_U8 nSnsId = 0;
    // AX_S32 nRet = 0;
    // WEB_SET_INT(nSnsId, pJson, "src_id");
    if (pResult) {
        sprintf((AX_CHAR*)pResult, "%s", "OPALDemo-IPC");
    }
    return AX_TRUE;
}

AX_BOOL ProcAssistReq(MprJson* pJson, AX_VOID** pResult) {
    AX_U8 nSnsId = 0;
    AX_U8 nChnId = 0;
    WEB_SET_INT(nSnsId, pJson, "src_id");
    WEB_SET_INT(nChnId, pJson, "stream");

    AX_U8 nSrcChn = GetSrcChn(nSnsId, nChnId);
    if (nSrcChn != 0xFF && pResult) {
        AX_OPAL_VIDEO_CHN_ATTR_T stChnAttr;
        AX_OPAL_VIDEO_SNS_ATTR_T stSnsAtt = {0};
        AX_OPAL_Video_GetChnAttr((AX_OPAL_SNS_ID_E)nSnsId, (AX_OPAL_VIDEO_CHN_E)nSrcChn, &stChnAttr);
        AX_OPAL_Video_GetSnsAttr(nSnsId, &stSnsAtt);
        AX_OPAL_SNS_ROTATION_E eRotation = stSnsAtt.eRotation;
        if (AX_OPAL_SNS_ROTATION_90 == eRotation
            || AX_OPAL_SNS_ROTATION_270 == eRotation) {
            sprintf((AX_CHAR*)pResult, "%dx%d", stChnAttr.nHeight, stChnAttr.nWidth);
        } else {
            sprintf((AX_CHAR*)pResult, "%dx%d", stChnAttr.nWidth, stChnAttr.nHeight);
        }
        return AX_TRUE;
    }
    return AX_FALSE;
}

AX_BOOL ProcCaptureReq(MprJson* pJson, AX_VOID** pResult) {
    AX_U8  nSnsId = 0;
    AX_U8  nChnId = 0;
    WEB_SET_INT(nSnsId, pJson, "src_id");
    WEB_SET_INT(nChnId, pJson, "stream");
    const AX_U32 nImageBufSize = 1024 * 1024;
    AX_U32 nQpLevel = GetSnapShotQpLevel();
    AX_U32 nActSize = 0;
    AX_U8 nSrcChn = GetSrcChn(nSnsId, nChnId);
    if (nSrcChn != 0xFF) {
        AX_U8* pImageBuf = (AX_U8* )AX_MALLOC(nImageBufSize);
        AX_OPAL_Video_Snapshot((AX_OPAL_SNS_ID_E)nSnsId, (AX_OPAL_VIDEO_CHN_E)nSrcChn, pImageBuf, nImageBufSize, &nActSize, nQpLevel);
        if (nActSize && pResult) {
            WS_SendSnapshotData(pImageBuf, nActSize, *pResult);
        }
        AX_FREE(pImageBuf);
    }
    return AX_TRUE;
}

AX_BOOL ProcEZoomReq(MprJson* pJson, AX_VOID** pResult) {
    AX_S32 nRet = 0;
    AX_U8  nSnsId = 0;
    WEB_SET_INT(nSnsId, pJson, "src_id");
    AX_OPAL_VIDEO_SNS_ATTR_T stSnsAttr = {0};
    nRet = AX_OPAL_Video_GetSnsAttr((AX_OPAL_SNS_ID_E)nSnsId, &stSnsAttr);
    if (0 == nRet) {
        AX_F32 fEZoomRatio = 0;
        AX_S32 nCenterOffsetX = 0;
        AX_S32 nCenterOffsetY = 0;
        MprJson* pJsonEzoom = mprReadJsonObj(pJson, "ezoom");
        WEB_SET_DOUBLE(fEZoomRatio, pJsonEzoom, "ezoom_ratio");
        WEB_SET_INT(nCenterOffsetX, pJsonEzoom, "center_offset_x");
        WEB_SET_INT(nCenterOffsetY, pJsonEzoom, "center_offset_y");
        stSnsAttr.stEZoomAttr.fRatio = fEZoomRatio;
        stSnsAttr.stEZoomAttr.nCenterOffsetX = nCenterOffsetX;
        stSnsAttr.stEZoomAttr.nCenterOffsetY = nCenterOffsetY;
        nRet = AX_OPAL_Video_SetSnsAttr((AX_OPAL_SNS_ID_E)nSnsId, &stSnsAttr);
        return AX_SUCCESS == nRet ? AX_TRUE : AX_FALSE;
    }
    return AX_TRUE;
}

AX_BOOL ProcWebRequest(WEB_REQUEST_TYPE_E eReqType, const AX_VOID* pJsonReq, AX_VOID** pResult) {
    if (!pJsonReq) {
        return AX_FALSE;
    }

    AX_BOOL bRet = AX_FALSE;
    MprJson* pJson = (MprJson*)pJsonReq;
    pthread_mutex_lock(&g_mtxWebReqProcess);
    switch (eReqType) {
        case E_REQ_TYPE_CAMERA:
            bRet = ProcCameraReq(pJson, pResult);
            break;
        case E_REQ_TYPE_IMAGE:
            bRet = ProcImageReq(pJson, pResult);
            break;
        case E_REQ_TYPE_AUDIO:
            bRet = ProcAudioReq(pJson, pResult);
            break;
        case E_REQ_TYPE_VIDEO:
            bRet = ProcVideoReq(pJson, pResult);
            break;
        case E_REQ_TYPE_AI:
            bRet = ProcAiReq(pJson, pResult);
            break;
        case E_REQ_TYPE_OSD:
            bRet = ProcOsdReq(pJson, pResult);
            break;
        case E_REQ_TYPE_SYSTEM:
            bRet = ProcSystemReq(pJson, pResult);
            break;
        case E_REQ_TYPE_ASSIST:
            bRet = ProcAssistReq(pJson, pResult);
            break;
        case E_REQ_TYPE_CAPTURE:
            bRet = ProcCaptureReq(pJson, pResult);
            break;
        case E_REQ_TYPE_EZOOM:
            bRet = ProcEZoomReq(pJson, pResult);
            break;
        default: {
            LOG_M_E(WEB_HELPER, "Invalid web request: unknown type(%d)", eReqType);
            pthread_mutex_unlock(&g_mtxWebReqProcess);
            return AX_FALSE;
        }
    }
    pthread_mutex_unlock(&g_mtxWebReqProcess);
    return bRet;
}