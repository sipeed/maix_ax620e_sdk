/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_MP4_H__
#define __AX_MP4_H__

#include "ax_base_type.h"
#include "ax_global_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef AX_VOID* AX_MP4_HANDLE;

typedef struct _AX_MP4_INFO_T {
    AX_BOOL bLoopSet;
    AX_U8 nSnsId;
    AX_U32 nChn;
    AX_U32 nMaxFileInMBytes;
    AX_U32 nMaxFileCount;
    AX_CHAR strSavePath[260];

    struct mpeg4_video_sess_attr {
        AX_BOOL bEnable;
        AX_PAYLOAD_TYPE_E ePt;
        AX_U32 nFrameRate;
        AX_S32 nfrWidth;
        AX_S32 nfrHeight;
        AX_S32 nBitrate;
    } stVideoAttr;

    struct mpeg4_audio_sess_attr {
        AX_BOOL bEnable;
        AX_PAYLOAD_TYPE_E ePt;
        AX_S32 nBitrate;
        AX_U32 nSampleRate;
        AX_U8 nChnCnt;
        AX_S32 nAOT;  // audio object type
    } stAudioAttr;
} AX_MP4_INFO_T;

AX_S32 AX_MP4_Init(AX_MP4_HANDLE *ppHandle, AX_MP4_INFO_T *pstInfo);
AX_S32 AX_Mp4_DeInit(AX_MP4_HANDLE pHandle);
AX_S32 AX_Mp4_SaveVideo(AX_MP4_HANDLE pHandle, AX_VOID *data, AX_U32 size, AX_U64 nPts, AX_BOOL bIFrame);
AX_S32 AX_Mp4_SaveAudio(AX_MP4_HANDLE pHandle, AX_VOID *data, AX_U32 size, AX_U64 nPts);

#ifdef __cplusplus
}
#endif

#endif