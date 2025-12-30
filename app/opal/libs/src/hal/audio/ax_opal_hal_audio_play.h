
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_AUDIO_PLAY_H_
#define _AX_OPAL_HAL_AUDIO_PLAY_H_

#include "ax_opal_hal_audio_def.h"

typedef struct audioWAV_RIFF_CHUNK_T {
    AX_U32 nChunkID;  //'R','I','F','F'
    AX_U32 nChunkSize;
    AX_U32 nFormat;  //'W','A','V','E'
} AUDIO_WAV_RIFF_CHUNK_T;

typedef struct audioWAV_FMT_CHUNK_T {
    AX_U32 nFmtID;
    AX_U32 nFmtSize;
    AX_U16 nFmtTag;
    AX_U16 nFmtChannels;
    AX_U32 nSampleRate;
    AX_U32 nByteRate;
    AX_U16 nBlockAilgn;
    AX_U16 nBitsPerSample;
} AUDIO_WAV_FMT_CHUNK_T;

typedef struct audioWAV_DATA_CHUNK_T {
    AX_U32 nDataID;  //'d','a','t','a'
    AX_U32 nDataSize;
} AUDIO_WAV_DATA_CHUNK_T;

typedef struct audioWAV_STRUCT_T {
    AUDIO_WAV_RIFF_CHUNK_T stRiffRegion;
    AUDIO_WAV_FMT_CHUNK_T stFmtRegion;
    AUDIO_WAV_DATA_CHUNK_T stDataRegion;
} AUDIO_WAV_STRUCT_T;

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_Init(AX_VOID);
AX_S32 AX_OPAL_HAL_AUDIO_PLAY_Deinit(AX_VOID);

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_Start(AX_OPAL_AUDIO_ATTR_T *pstAttr);
AX_S32 AX_OPAL_HAL_AUDIO_PLAY_Stop(AX_OPAL_AUDIO_ATTR_T *pstAttr);

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_GetVolume(AX_OPAL_AUDIO_ATTR_T* pstAttr);
AX_S32 AX_OPAL_HAL_AUDIO_PLAY_SetVolume(const AX_OPAL_AUDIO_ATTR_T* pstAttr);

AX_S32 AX_OPAL_HAL_AUDIO_Play(AX_OPAL_AUDIO_ATTR_T* pstAttr, AX_PAYLOAD_TYPE_E eType, const AX_U8 *pData, AX_U32 nDataSize);
AX_S32 AX_OPAL_HAL_AUDIO_StopPlay(AX_OPAL_AUDIO_ATTR_T* pstAttr);

AX_S32 AX_OPAL_HAL_AUDIO_PlayFile(AX_OPAL_AUDIO_ATTR_T* pstAttr, AX_OPAL_AUDIO_PLAYFILE_T* pPlayFile, AX_OPAL_MAL_AUDIO_PLAY_T *pMalPlayFile);
AX_S32 AX_OPAL_HAL_AUDIO_StopPlayFile(AX_OPAL_AUDIO_ATTR_T* pstAttr, AX_OPAL_MAL_AUDIO_PLAY_T *pMalPlayFile);

#endif // _AX_OPAL_HAL_AUDIO_PLAY_H_