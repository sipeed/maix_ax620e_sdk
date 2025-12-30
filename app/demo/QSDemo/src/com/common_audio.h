/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __COMMON_AUDIO_H__
#define __COMMON_AUDIO_H__

#include "ax_global_type.h"
#include "ax_ai_api.h"
#include "ax_ao_api.h"
#include "ax_acodec_api.h"
#include "ax_aenc_api.h"
#include "ax_adec_api.h"
#include "ax_audio_process.h"
#include "ax_aac.h"

#ifndef COMM_AUDIO_PRT
#define COMM_AUDIO_PRT(fmt...)   \
do {\
    printf("[COMM_AUDIO][%s][%5d] ", __FUNCTION__, __LINE__);\
    printf(fmt);\
    printf("\n");\
}while(0)
#endif

// audio capture
typedef struct {
    AX_U32 nCardNum;
    AX_U32 nDeviceNum;
    AX_AI_LAYOUT_MODE_E eLayoutMode;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_U32 nSampleRate;
    AX_U32 nVqeSampleRate;
    AX_F32 fVqeVolume;
    AX_BOOL bResample;
    AX_U32 nResRate;
} SAMPLE_AI_DEV_ATTR_T;

typedef struct {
    const AX_AEC_CONFIG_T* pstAecCfg;
    const AX_NS_CONFIG_T* pstNsCfg;
    const AX_AGC_CONFIG_T* pstAgcCfg;
    const AX_VAD_CONFIG_T* pstVadCfg;
} SAMPLE_AP_UPTALKVQE_ATTR_T;

typedef struct {
    const AX_ACODEC_EQ_ATTR_T* pstEqCfg;
    const AX_ACODEC_FREQ_ATTR_T* pstHpfCfg;
    const AX_ACODEC_FREQ_ATTR_T* pstLpfCfg;
} SAMPLE_ACODEC_ATTR_T;

typedef struct {
    AX_U32 nEncChn;
    AX_U32 nEncSampleRate;
    AX_U32 nEncBitRate;
    AX_PAYLOAD_TYPE_E eEncType;
} SAMPLE_AENC_CHN_ATTR_T;

typedef struct {
    const SAMPLE_AI_DEV_ATTR_T* pstDevAttr;
    const SAMPLE_AP_UPTALKVQE_ATTR_T* pstUpVqeAttr;
    const SAMPLE_ACODEC_ATTR_T* pstACodecAttr;
    const SAMPLE_AENC_CHN_ATTR_T* pstAencChnAttr;
} SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T;

typedef struct {
    AX_BOOL bEnable;
    AX_PAYLOAD_TYPE_E ePt;
    AX_S32 nBitrate;
    AX_U32 nSampleRate;
    AX_U8 nChnCnt;
    AX_S32 nAOT;  // audio object type
} SAMPLE_AUDIO_ENCODER_INFO_T;

// audio play
typedef struct {
    AX_U32 nCardNum;
    AX_U32 nDeviceNum;
    AX_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_AUDIO_SOUND_MODE_E enSoundmode;
    AX_U32 nSampleRate;
    AX_U32 nVqeSampleRate;
    AX_F32 fVqeVolume;
    AX_BOOL bResample;
    AX_U32 nResRate;
} SAMPLE_AO_DEV_ATTR_T;

typedef struct {
    const AX_NS_CONFIG_T* pstNsCfg;
    const AX_AGC_CONFIG_T* pstAgcCfg;
} SAMPLE_DnVQE_ATTR_T;

typedef struct {
    AX_U32 nDecChn;
    AX_PAYLOAD_TYPE_E eDecType;
} SAMPLE_ADEC_CHN_ATTR_T;

typedef struct {
    AX_BOOL bKeepAlive; // whether keep ao dev already enable
    const SAMPLE_AO_DEV_ATTR_T* pstDevAttr;
    const SAMPLE_DnVQE_ATTR_T* pstDnVqeAttr;
    const SAMPLE_ADEC_CHN_ATTR_T* pstAdecChnAttr;
} SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T;

// audio file play status
typedef enum {
    SAMPLE_AUDIO_PLAY_FILE_STATUS_COMPLETE,
    SAMPLE_AUDIO_PLAY_FILE_STATUS_STOP,
    SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR,
    SAMPLE_AUDIO_PLAY_FILE_STATUS_BUTT
} SAMPLE_AUDIO_PLAY_FILE_STATUS_E;

// audio file paly result
typedef struct sampleAUDIO_PLAY_FILE_RESULT_T {
    AX_PAYLOAD_TYPE_E eType;
    SAMPLE_AUDIO_PLAY_FILE_STATUS_E eStatus;
    AX_CHAR *pstrFileName;
    AX_VOID *pUserData;
} SAMPLE_AUDIO_PLAY_FILE_RESULT_T, *SAMPLE_AUDIO_PLAY_FILE_RESULT_PTR;

typedef AX_VOID (*SAMPLE_AUDIO_PLAYFILERESULT_CALLBACK)(const SAMPLE_AUDIO_PLAY_FILE_RESULT_PTR pstResult);

typedef AX_VOID (*SAMPLE_AUDIO_STREAM_CALLBACK)(const AX_AUDIO_STREAM_T *ptStream, AX_VOID *pUserData);

#ifdef __cplusplus
extern "C"
{
#endif

AX_S32 COMMON_AUDIO_Init(const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pstCapEntryParam,
                                const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T* pstPlayEntryParam,
                                const SAMPLE_AUDIO_STREAM_CALLBACK callback, AX_VOID *pUserData);
AX_S32 COMMON_AUDIO_Deinit(AX_VOID);
AX_S32 COMMON_AUDIO_GetEncoderInfo(SAMPLE_AUDIO_ENCODER_INFO_T* pstEncoderInfo);
AX_S32 COMMON_AUDIO_Play(AX_PAYLOAD_TYPE_E eType, const AX_U8 *pData, AX_U32 nDataSize);
AX_S32 COMMON_AUDIO_StopPlay(AX_VOID);
AX_S32 COMMON_AUDIO_PlayFile(AX_PAYLOAD_TYPE_E eType,
                                    const AX_CHAR *pstrFileName,
                                    AX_S32 nLoop,
                                    SAMPLE_AUDIO_PLAYFILERESULT_CALLBACK callback,
                                    AX_VOID *pUserData);
AX_S32 COMMON_AUDIO_StopPlayFile(AX_VOID);
AX_S32 COMMON_AUDIO_GetCaptureAttr(AX_BOOL *pbEnable);
AX_S32 COMMON_AUDIO_SetCaptureAttr(AX_BOOL bEnable);
AX_S32 COMMON_AUDIO_SetCaptureVolume(AX_F32 fVqeVolume);
AX_S32 COMMON_AUDIO_SetPlayVolume(AX_F32 fVqeVolume);

#ifdef __cplusplus
}
#endif

#endif
