
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_ELEAUDIO_H_
#define _AX_OPAL_MAL_ELEAUDIO_H_

#include "ax_opal_mal_def.h"
#include "ax_opal_queue.h"
#include "ax_opal_thread.h"

#define FILE_NAME_LEN (256)

#define AUDIO_AAC_HEADER_SIZE (7)
#define AUDIO_AAC_HEADER_WITH_CRC_SIZE (AUDIO_AAC_HEADER_SIZE + 2)

#define AUDIO_ID_RIFF 0x46464952
#define AUDIO_ID_WAVE 0x45564157
#define AUDIO_ID_FMT 0x20746d66
#define AUDIO_ID_DATA 0x61746164

// typedef struct _AX_OPAL_MAL_AUDIO_WAV_RIFF_CHUNK_T {
//     AX_U32 nChunkID;    //'R','I','F','F'
//     AX_U32 nChunkSize;
//     AX_U32 nFormat;     //'W','A','V','E'
// } AX_OPAL_MAL_AUDIO_WAV_RIFF_CHUNK_T;

// typedef struct _AX_OPAL_MAL_AUDIO_WAV_FMT_CHUNK_T {
//     AX_U32 nFmtID;
//     AX_U32 nFmtSize;
//     AX_U16 nFmtTag;
//     AX_U16 nFmtChannels;
//     AX_U32 nSampleRate;
//     AX_U32 nByteRate;
//     AX_U16 nBlockAilgn;
//     AX_U16 nBitsPerSample;
// } AX_OPAL_MAL_AUDIO_WAV_FMT_CHUNK_T;

// typedef struct _AX_OPAL_AUDIO_WAV_DATA_CHUNK_T {
//     AX_U32 nDataID;  //'d','a','t','a'
//     AX_U32 nDataSize;
// } AX_OPAL_AUDIO_WAV_DATA_CHUNK_T;

// typedef struct _AX_OPAL_AUDIO_WAV_STRUCT_T {
//     AX_OPAL_MAL_AUDIO_WAV_RIFF_CHUNK_T stRiffRegion;
//     AX_OPAL_MAL_AUDIO_WAV_FMT_CHUNK_T stFmtRegion;
//     AX_OPAL_AUDIO_WAV_DATA_CHUNK_T stDataRegion;
// } AX_OPAL_AUDIO_WAV_STRUCT_T;

// typedef struct _AX_OPAL_AUDIO_FILE_T {
//     AX_PAYLOAD_TYPE_E eType;
//     AX_CHAR strFileName[FILE_NAME_LEN];
//     AX_U32 nLoop;
//     AX_OPAL_AUDIO_PLAYFILERESULT_CALLBACK callback;
//     AX_VOID *pUserData;
//     AX_OPAL_AUDIO_PLAY_FILE_STATUS_E eStatus;
// } AX_OPAL_AUDIO_FILE_T;

// typedef struct _AX_OPAL_AUDIO_STREAM_T {
//     AX_PAYLOAD_TYPE_E eType;
//     const AX_U8 *pData;
//     AX_U32 nDataSize;
// } AX_OPAL_AUDIO_STREAM_T;

// typedef struct _AX_OPAL_AUDIO_PLAY_T {
//     AX_BOOL bInited;
//     AX_OPAL_AUDIO_PLAY_ATTR_T tAudioPlayAttr;
//     /* play file */
//     AX_OPAL_QUEUE_T *pFileQueue;
//     AX_OPAL_THREAD_T *pFileThread;
//     AX_U32 nPoolId;
// } AX_OPAL_MAL_AUDIO_PLAY_T;

// typedef struct _AXOP_HAL_AUDIO_CAP_T {
//     AX_BOOL bCapEnable;
//     AX_OPAL_AUDIO_CAP_ATTR_T tAudioCapAttr;
//     AX_OPAL_THREAD_T *pCapThread;
//     AX_OPAL_AUDIO_PKT_CALLBACK callback;
//     AX_VOID *pUserData;
// } AX_OPAL_MAL_AUDIO_CAP_T;

// typedef struct _AXOP_AUDIO_PKTCALLBACK_T {
//     AX_OPAL_AUDIO_PKT_CALLBACK callback;
//     AX_VOID *pUserData;
// } AX_OPAL_AUDIO_PKTCALLBACK_T;

typedef struct _AX_OPAL_MAL_ELEAUDIO_T {
    AX_OPAL_MAL_ELE_T stBase;

    AX_OPAL_AUDIO_ATTR_T stAttr;

    /* out encoder attr */
    AX_OPAL_AUDIO_ENCODER_ATTR_T stEncoderAttr;

    AX_OPAL_THREAD_T *pThreadGetAencStream;
    AX_OPAL_AUDIO_PKT_CALLBACK_T stAudioPktCb;

    AX_OPAL_MAL_AUDIO_PLAY_T stAudioFilePlay;
} AX_OPAL_MAL_ELEAUDIO_T;

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELEAUDIO_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pAttr);
AX_S32 AX_OPAL_MAL_ELEAUDIO_Destroy(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEAUDIO_Start(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEAUDIO_Stop(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEAUDIO_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);

#endif // __AXOP_MAL_ELEAUDIO_H__