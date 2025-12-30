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
#include <stdlib.h>
#include "ax_option.h"
#include "ax_opal_type.h"
#include "minIni.h"

#define AX_WEB_VENC_FRM_SIZE_RATIO (0.125f)
#define AX_WEB_JENC_FRM_SIZE_RATIO (0.05f)
#define AX_WEB_AENC_FRM_SIZE (8192)
#define AX_WEB_VENC_RING_BUFF_COUNT (5)
#define AX_WEB_JENC_RING_BUFF_COUNT (10)
#define AX_WEB_MJENC_RING_BUFF_COUNT (10)
#define AX_WEB_EVENTS_RING_BUFF_COUNT (5)
#define AX_WEB_AENC_RING_BUFF_COUNT (5)
#define AX_WEB_SNAPSHOT_QP_LEVEL (63)

#define AX_RTSP_FRM_SIZE (700000)
#define AX_RTSP_RING_BUFF_COUNT (2)

#define OPATION_DEFAULT_PATH  "./config/options.ini"

static AX_CHAR g_szOptionPath[260] = {0};

static const AX_CHAR* GetOptionPath() {
    if (strlen(g_szOptionPath) == 0) {
        return OPATION_DEFAULT_PATH;
    } else {
        return &g_szOptionPath[0];
    }
}

AX_BOOL SetCfgPath(AX_CHAR* pPath) {
    if(!pPath) {
        return AX_FALSE;
    }
    else {
        sprintf(g_szOptionPath, "%s/options.ini", pPath);
    }
    return AX_TRUE;
}

AX_F32 GetJencOutBuffRatio() {
    return (AX_F32)ini_getf("options", "WebJencFrmSizeRatio", AX_WEB_JENC_FRM_SIZE_RATIO, GetOptionPath());
}

AX_U32 GetAencOutFrmSize() {
    AX_U32 value = (AX_U32)ini_getl("options", "WebAencFrmSize", AX_WEB_AENC_FRM_SIZE, GetOptionPath());
    return value;
}

AX_U32 GetWebJencRingBufCount() {
    AX_U32 value = (AX_U32)ini_getl("options", "WebJencRingBufCount", AX_WEB_JENC_RING_BUFF_COUNT, GetOptionPath());
    return value;
}

AX_U32 GetWebMjencRingBufCount() {
    AX_U32 value = (AX_U32)ini_getl("options", "WebMjencRingBufCount", AX_WEB_MJENC_RING_BUFF_COUNT, GetOptionPath());
    return value;
}

AX_U32 GetWebEventsRingBufCount() {
    AX_U32 value = (AX_U32)ini_getl("options", "WebEventsRingBufCount", AX_WEB_EVENTS_RING_BUFF_COUNT, GetOptionPath());
    return value;
}

AX_U32 GetWebAencRingBufCount() {
    AX_U32 value = (AX_U32)ini_getl("options", "WebAencRingBufCount", AX_WEB_AENC_RING_BUFF_COUNT, GetOptionPath());
    return value;
}

AX_U32 GetRTSPMaxFrmSize() {
    AX_U32 value = (AX_U32)ini_getl("options", "RTSPMaxFrmSize", AX_RTSP_FRM_SIZE, GetOptionPath());
    return value;
}

AX_U32 GetRTSPRingBufCount() {
    AX_U32 value = (AX_U32)ini_getl("options", "RTSPRingBufCount", AX_RTSP_RING_BUFF_COUNT, GetOptionPath());
    return value;
}


AX_U32 GetSnapShotQpLevel() {
    AX_U32 value = (AX_U32)ini_getl("options", "WebSnapShotQpLevel", AX_WEB_SNAPSHOT_QP_LEVEL, GetOptionPath());
    return value;
}

AX_BOOL IsEnableMp4Record() {
    AX_U32 value = (AX_U32)ini_getl("mp4", "EnableMp4Record", 0, GetOptionPath());
    return value ? AX_TRUE : AX_FALSE;
}

AX_U32 GetMp4SavedPath(AX_CHAR* szPath, AX_U32 nLen) {
    return ini_gets("mp4", "MP4RecordSavedPath", "./", szPath, nLen, GetOptionPath());
}

AX_U32 GetMp4FileSize() {
    AX_U32 value = (AX_U32)ini_getl("mp4", "MP4RecordFileSize", 64, GetOptionPath());
    return value;
}

AX_U32 GetMp4FileCount() {
    AX_U32 value = (AX_U32)ini_getl("mp4", "MP4RecordFileCount", 10, GetOptionPath());
    return value;
}

AX_BOOL GetMp4LoopSet() {
    AX_U32 value = (AX_U32)ini_getl("mp4", "MP4RecordLoopSet", 1, GetOptionPath());
    return value ? AX_TRUE : AX_FALSE;
}

AX_BOOL IsEnableAudio() {
    AX_U32 value = (AX_U32)ini_getl("audio", "EnableAudioFeature", 0, GetOptionPath());
    return value ? AX_TRUE : AX_FALSE;
}

AX_U32 GetAudioEncoderType() {
    AX_U32 value = (AX_U32)ini_getl("audio", "AudioEncoderType", 19, GetOptionPath());
    return value;
}

AX_BOOL GetInterpolationResolution(AX_U32* nWidth, AX_U32* nHeight) {
    if (!nWidth || !nHeight) {
        return AX_FALSE;
    }

    AX_CHAR szVals[64] = {0};
    ini_gets("options", "InterpolationResolution", "", szVals, 63, GetOptionPath());

    if (strlen(szVals) != 0) {
        AX_U32 nW = 0;
        AX_U32 nH = 0;
        sscanf(&szVals[0], "%dx%d", &nW, &nH);

        if (0 < nW && 0 < nH) {
            *nWidth = nW;
            *nHeight = nH;
            return AX_TRUE;
        }
        return AX_FALSE;
    }
    return AX_FALSE;
}

AX_BOOL IsEnableWebServerStatusCheck() {
    AX_U32 value = (AX_U32)ini_getl("options", "WebServerStatusCheck", 0, GetOptionPath());
    return value ? AX_TRUE : AX_FALSE;
}

AX_U32 GetDetectAlgoType() {
    AX_U32 value = (AX_U32)ini_getl("algo", "DetectAlgoType", 1, GetOptionPath());
    return value;
}

AX_BOOL IsEnableBodyAeRoi() {
    AX_U32 value = (AX_U32)ini_getl("algo", "EnableBodyAeRoi", 0, GetOptionPath());
    return value ? AX_TRUE : AX_FALSE;
}

AX_BOOL IsEnableVehicleAeRoi() {
    AX_U32 value = (AX_U32)ini_getl("algo", "EnableVehicleAeRoi", 0, GetOptionPath());
    return value ? AX_TRUE : AX_FALSE;
}

AX_U32 GetSensorMode() {
    AX_U32 value = (AX_U32)ini_getl("sns", "SensorMode", 1, GetOptionPath());
    return value;
}

AX_BOOL IsEnableDIS() {
    AX_BOOL value = (AX_BOOL)ini_getl("sns", "EnableDIS", 0, GetOptionPath());
    return value;
}

AX_U8 GetDISDelayFrameNum() {
    AX_U8 value = (AX_U8)ini_getl("sns", "DISDelayNum", 4, GetOptionPath());
    return value;
}

AX_BOOL GetDISMotionShare() {
    AX_BOOL value = (AX_BOOL)ini_getl("sns", "DISMotionShare", 0, GetOptionPath());
    return value;
}

AX_BOOL GetDISMotionEst() {
    AX_BOOL value = (AX_BOOL)ini_getl("sns", "DISMotionEst", 0, GetOptionPath());
    return value;
}

AX_BOOL GetTuningOption(AX_U32* pPort) {
    AX_U32 value = (AX_U32)ini_getl("tuning", "TuningCtrl", 0, GetOptionPath());
    *pPort = (AX_U32)ini_getl("tuning", "TuningPort", 8082, GetOptionPath());
    return value ? AX_TRUE : AX_FALSE;
}


AX_U32 Str2Array(AX_CHAR* strValues, AX_F64 *pValues, AX_U32 nValCount) {
    AX_U32 nValNum = 0;
    AX_CHAR* pszData = strValues;
    AX_CHAR* pszToken = NULL;

    AX_CHAR* p = pszData;
    while (p && *p != '\0') {
        if (*p == '[' || *p == ']') {
            *p = ' ';
        }
        p++;
    }
    const AX_CHAR* pszDelimiters = ",";
    pszToken = strtok(pszData, pszDelimiters);

    while (pszToken) {
        AX_F64 fValue = (AX_F64)strtod(pszToken, NULL);
        if (nValNum+1 <= nValCount) {
            pValues[nValNum] = fValue;
            nValNum++;
        }
        pszToken = strtok(NULL, pszDelimiters);
    }

    return nValNum;
}

AX_U32 GetVencRingBufSize(AX_U32 width, AX_U32 height) {
    AX_U32 nSize = 0;
    AX_U32 nCount = AX_WEB_VENC_RING_BUFF_COUNT;
    AX_F64 nRatio = (AX_F64)ini_getf("vencRingBuffer", "defaultRatio", AX_WEB_VENC_FRM_SIZE_RATIO, GetOptionPath());
    if(!nRatio) {
        nRatio = AX_WEB_VENC_FRM_SIZE_RATIO;
    }
    AX_CHAR resolution[16];
    sprintf(resolution, "%dx%d", width, height);
    AX_CHAR szVals[64] = {0};
    ini_gets("vencRingBuffer", resolution, "", szVals, 63, GetOptionPath());

    AX_F64 fVals[2] = {0};
    AX_U32 nNum = Str2Array(szVals, fVals, 2);

    if (nNum != 2) {
        nSize = width * height * 3 / 2 * nRatio;
    } else {
        nCount = (AX_U32)(fVals[0]);
        nSize = (AX_U32)(fVals[1]);
    }

    return nSize * nCount;
}

