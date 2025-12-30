/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _WEB_OPTION_HELPER_
#define _WEB_OPTION_HELPER_

#include "ax_opal_type.h"
#include "mpr.h"
#include "cJSON.h"

#define MAX_PREV_SNS_CHN_NUM        (AX_OPAL_VIDEO_CHN_BUTT)
#define MAX_PREV_SNS_NUM            (AX_OPAL_SNS_ID_BUTT)

#define MAX_VIDEO_ATTR_NUM          4
#define MAX_REGION_NUM              (32)
#define MAX_ENCODER_TYPE_NUM        4
#define RC_TYPE_MAX                 5

typedef enum {
    JPEG_TYPE_BODY = 0,
    JPEG_TYPE_VEHICLE,
    JPEG_TYPE_CYCLE,
    JPEG_TYPE_FACE,
    JPEG_TYPE_PLATE,
    JPEG_TYPE_BUTT
} JPEG_TYPE_E;

typedef enum {
    E_REQ_TYPE_CAMERA = 0,
    E_REQ_TYPE_AUDIO,
    E_REQ_TYPE_VIDEO,
    E_REQ_TYPE_AI,
    E_REQ_TYPE_SYSTEM,
    E_REQ_TYPE_ASSIST,
    E_REQ_TYPE_OSD,
    E_REQ_TYPE_CAPTURE,
    E_REQ_TYPE_IMAGE,
    E_REQ_TYPE_EZOOM,
    E_REQ_TYPE_MAX,
} WEB_REQUEST_TYPE_E;

typedef struct {
    AX_U8 nSnsSrc;
    AX_U8 nChannel;
    AX_U32 nWidth;
    AX_U32 nHeight;
} JPEG_HEAD_INFO_T;

typedef struct _JPEG_CAPTURE_INFO_T {
    JPEG_HEAD_INFO_T tHeaderInfo;
    AX_VOID* pData;
} JPEG_CAPTURE_INFO_T;

typedef struct {
    AX_U8 nGender; /* 0-female, 1-male */
    AX_U8 nAge;
    AX_CHAR szMask[32];
    AX_CHAR szInfo[32];
} JPEG_FACE_INFO_T;

typedef struct {
    AX_CHAR szNum[16];
    AX_CHAR szColor[32];
} JPEG_PLATE_INFO_T;

typedef struct _JPEG_DATA_INFO_T {
    JPEG_TYPE_E eType; /* JPEG_TYPE_E */
    union {
        JPEG_CAPTURE_INFO_T tCaptureInfo;
        JPEG_FACE_INFO_T tFaceInfo;
        JPEG_PLATE_INFO_T tPlateInfo;
    };
} JPEG_DATA_INFO_T;

typedef struct _JpegHead {
    AX_U32 nMagic;
    AX_U32 nTotalLen;
    AX_U32 nTag;
    AX_U32 nJsonLen;
    AX_CHAR szJsonData[256];
} JPEG_HEAD_T;

// {"AXIT", total_len, "JSON", json_len, json_data}
#define INIT_JPEG_HEAD {.nMagic=0x54495841,.nTotalLen=0,.nTag=0x4E4F534A,.nJsonLen=0,.szJsonData={0}}

typedef struct _STATISTICS_INFO_T {
    AX_U64 nStartTick;
    AX_U32 nVencOutBytes;
    AX_F32 fBitrate;
} STATISTICS_INFO_T, *STATISTICS_INFO_PTR;


AX_BOOL ProcWebRequest(WEB_REQUEST_TYPE_E eReqType, const AX_VOID* pJsonReq, AX_VOID** pResult);

cJSON*  GetCapSettingJson();
cJSON*  GetPreviewInfoJson(const AX_U8 *pSnsPrevChnCount);
cJSON*  GetSnsFramerateJson(AX_U8 nSnsId);
cJSON*  GetSnsResolutionJson(AX_U8 nSnsId);
cJSON*  GetCameraJson(AX_U8 nSnsId);
cJSON*  GetImageJson(AX_U8 nSnsId);
cJSON*  GetLdcJson(AX_U8 nSnsId);
cJSON*  GetDisJson(AX_U8 nSnsId);
cJSON*  GetAudioJson();
cJSON*  GetVideoAttrJson(AX_U8 nSnsId, AX_U32 nSrcChn, AX_U32 nWFirst, AX_U32 nHFirst, AX_U32 nWLast, AX_U32 nHLast);
cJSON*  GetVideoJson(AX_U8 nSnsId, AX_U32 nSrcChn);
cJSON*  GetAiInfoJson(AX_U8 nSnsId);
cJSON*  GetOsdJson(AX_U8 nSnsId);
cJSON*  GetPrivJson(AX_U8 nSnsId);

#endif // _WEB_OPTION_HELPER_
