/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "AXMp4Encoder.h"
#include "AXMp4EncoderWrapper.h"

#define VIDEO_DEFAULT_MAX_FRM_SIZE (700000)
#define AUDIO_DEFAULT_MAX_FRM_SIZE (10240)

AX_S32 AX_Mp4Encoder_Init(AX_MP4ENCODER_HANDLE *ppHandle, AX_MP4ENCODER_INFO_T *pstInfo) {
    if (!pstInfo) {
        return -1;
    }

    CMPEG4Encoder *inst = new CMPEG4Encoder;

    if (inst) {
        inst->Init();

        {
            MPEG4EC_INFO_T stMpeg4Info;

            stMpeg4Info.bLoopSet = pstInfo->bLoopSet;
            stMpeg4Info.nSnsId = pstInfo->nSnsId;
            stMpeg4Info.nChn = pstInfo->nChn;
            stMpeg4Info.nMaxFileInMBytes = pstInfo->nMaxFileInMBytes;
            stMpeg4Info.nMaxFileCount = pstInfo->nMaxFileCount;
            if (pstInfo->pstrSavePath) {
                stMpeg4Info.strSavePath = pstInfo->pstrSavePath;
            } else {
                stMpeg4Info.strSavePath = "./";
            }

            // video
            stMpeg4Info.stVideoAttr.bEnable = pstInfo->stVideoAttr.bEnable;
            stMpeg4Info.stVideoAttr.ePt = pstInfo->stVideoAttr.ePt;
            stMpeg4Info.stVideoAttr.nFrameRate = pstInfo->stVideoAttr.nFrameRate;
            stMpeg4Info.stVideoAttr.nfrWidth = pstInfo->stVideoAttr.nfrWidth;
            stMpeg4Info.stVideoAttr.nfrHeight = pstInfo->stVideoAttr.nfrHeight;
            stMpeg4Info.stVideoAttr.nBitrate = pstInfo->stVideoAttr.nBitrate;
            stMpeg4Info.stVideoAttr.nMaxFrmSize = VIDEO_DEFAULT_MAX_FRM_SIZE;

            // audio
            stMpeg4Info.stAudioAttr.bEnable = pstInfo->stAudioAttr.bEnable;
            stMpeg4Info.stAudioAttr.ePt = pstInfo->stAudioAttr.ePt;
            stMpeg4Info.stAudioAttr.nBitrate = pstInfo->stAudioAttr.nBitrate;
            stMpeg4Info.stAudioAttr.nSampleRate = pstInfo->stAudioAttr.nSampleRate;
            stMpeg4Info.stAudioAttr.nChnCnt = pstInfo->stAudioAttr.nChnCnt;
            stMpeg4Info.stAudioAttr.nAOT = pstInfo->stAudioAttr.nAOT;
            stMpeg4Info.stAudioAttr.nMaxFrmSize = AUDIO_DEFAULT_MAX_FRM_SIZE;

            if (!inst->InitParam(stMpeg4Info)) {
                return -1;
            }
        }

        inst->Start();

        if (ppHandle) {
            *ppHandle = (AX_MP4ENCODER_HANDLE)inst;
        }
    }

    return 0;
}

AX_S32 AX_Mp4Encoder_SaveVideo(AX_MP4ENCODER_HANDLE pHandle, AX_VOID *data, AX_U32 size, AX_U64 nPts, AX_BOOL bIFrame) {
    if (!pHandle) {
        return -1;
    }

    CMPEG4Encoder *inst = (CMPEG4Encoder *)pHandle;

    if (!inst->SendVideoFrame(data, size, nPts, bIFrame)) {
        return -1;
    }

    return 0;
}

AX_S32 AX_Mp4Encoder_SaveAudio(AX_MP4ENCODER_HANDLE pHandle, AX_VOID *data, AX_U32 size, AX_U64 nPts) {
    if (!pHandle) {
        return -1;
    }

    CMPEG4Encoder *inst = (CMPEG4Encoder *)pHandle;

    if (!inst->SendAudioFrame(data, size, nPts)) {
        return -1;
    }

    return 0;
}

AX_S32 AX_Mp4Encoder_DeInit(AX_MP4ENCODER_HANDLE pHandle) {
    if (!pHandle) {
        return -1;
    }

    CMPEG4Encoder *inst = (CMPEG4Encoder *)pHandle;

    inst->Stop();

    inst->DeInit();

    delete inst;

    return 0;
}
