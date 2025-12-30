/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifdef QSDEMO_AUDIO_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/prctl.h>
#include <sys/stat.h>

#include "common_audio.h"
#include "common_sys.h"
#include "axfifo.h"
#include "qs_utils.h"

// capture
#define SAMPLE_AI_DEFAULT_TINYALSA_CHNCNT (2)
#define SAMPLE_AI_PERIOD_COUNT (8)
#define SAMPLE_AI_STEREO_AAC_PERIOD_SIZE (1280) // stereo aac + vqe: 1280; stereo aac: 1024
#define SAMPLE_AI_DEFAULT_BLK_SIZE (4 * 1024)
#define SAMPLE_AI_STEREO_AAC_BLK_SIZE (12800) // stereo aac + vqe: 12800; stereo aac: 8192
#define SAMPLE_AI_DEFAULT_BLK_CNT (12) // 4(ai) + 8(aenc)
#define SAMPLE_AI_DEFAULT_DEPTH (30)
#define SAMPLE_AENC_DEFAULT_OUT_DEPTH (8)

// play
#define SAMPLE_AO_DEFAULT_TINYALSA_CHNCNT (2)
#define SAMPLE_AO_DEFAULT_DEPTH (30)
#define SAMPLE_AO_PERIOD_COUNT (8)
#define SAMPLE_AO_STEREO_AAC_PERIOD_SIZE (1024)
#define SAMPLE_ADEC_DEFAULT_BLK_SIZE (4 * 1024)
#define SAMPLE_ADEC_STEREO_AAC_BLK_SIZE (8 * 1024)
#define SAMPLE_ADEC_DEFAULT_BLK_CNT (34)
#define SAMPLE_ADEC_DEFAULT_IN_DEPTH (8)

typedef struct {
    AX_BOOL bEnable;
    AX_BOOL bInited;
    SAMPLE_AI_DEV_ATTR_T stDevAttr;
    AX_AEC_CONFIG_T stAecCfg;
    AX_NS_CONFIG_T stNsCfg;
    AX_AGC_CONFIG_T stAgcCfg;
    AX_VAD_CONFIG_T stVadCfg;
    AX_ACODEC_EQ_ATTR_T stEqCfg;
    AX_ACODEC_FREQ_ATTR_T stHpfCfg;
    AX_ACODEC_FREQ_ATTR_T stLpfCfg;
    SAMPLE_AENC_CHN_ATTR_T stAencChnAttr;
    SAMPLE_AUDIO_ENCODER_INFO_T stEncoderInfo;
} SAMPLE_AUDIO_CAPTURE_PARAM_T;

typedef struct {
    AX_BOOL bEnable;
    AX_BOOL bKeepAlive;
    AX_BOOL bInited;
    SAMPLE_AO_DEV_ATTR_T stDevAttr;
    AX_NS_CONFIG_T stNsCfg;
    AX_AGC_CONFIG_T stAgcCfg;
    SAMPLE_ADEC_CHN_ATTR_T stAdecChnAttr;
} SAMPLE_AUDIO_PLAY_PARAM_T;

// audio get
typedef struct _SAMPLE_AUDIO_GET_THREAD_PARAM {
    AX_BOOL bExit;
    AX_U32 nCardNum;
    AX_U32 nDeviceNum;
    AX_U32 nChn;
    pthread_t nTid;
    SAMPLE_AUDIO_STREAM_CALLBACK callback;
    AX_VOID *pUserData;
} SAMPLE_AUDIO_GET_THREAD_PARAM_T;

// playe file
#define FILE_NAME_LEN (256)
#define SAMPLE_AUDIO_PLAY_FILE_DEPTH (5)

#define NOTIFY_AFPLAY_FILE(pstPlayFileAttr)              \
    do {                                                 \
        pthread_mutex_lock(&g_sample_audio_play_file_info.mtxPlayFile); \
        axfifo_put(&g_sample_audio_play_file_info.qPlayFile, pstPlayFileAttr, sizeof(SAMPLE_AUDIO_PLAY_FILE_ATTR_T)); \
        pthread_cond_broadcast(&g_sample_audio_play_file_info.cvPlayFile); \
        pthread_mutex_unlock(&g_sample_audio_play_file_info.mtxPlayFile); \
    } while (0)

#define NOTIFY_AFPLAY_RESULT(pstResult)                \
    do {                                               \
        pthread_mutex_lock(&g_sample_audio_play_file_info.mtxResult); \
        axfifo_put(&g_sample_audio_play_file_info.qResult, pstResult, sizeof(SAMPLE_AUDIO_PLAY_FILE_ATTR_T)); \
        pthread_cond_broadcast(&g_sample_audio_play_file_info.cvResult); \
        pthread_mutex_unlock(&g_sample_audio_play_file_info.mtxResult); \
    } while (0)

#define AUDIO_ID_RIFF 0x46464952
#define AUDIO_ID_WAVE 0x45564157
#define AUDIO_ID_FMT 0x20746d66
#define AUDIO_ID_DATA 0x61746164

typedef struct audioWAV_RIFF_CHUNK_T {
    AX_U32 nChunkID;  //'R','I','F','F'
    AX_U32 nChunkSize;
    AX_U32 nFormat;  //'W','A','V','E'
} AUDIO_WAV_RIFF_CHUNK_T, *AUDIO_WAV_RIFF_CHUNK_PTR;

typedef struct audioWAV_FMT_CHUNK_T {
    AX_U32 nFmtID;
    AX_U32 nFmtSize;
    AX_U16 nFmtTag;
    AX_U16 nFmtChannels;
    AX_U32 nSampleRate;
    AX_U32 nByteRate;
    AX_U16 nBlockAilgn;
    AX_U16 nBitsPerSample;
} AUDIO_WAV_FMT_CHUNK_T, *AUDIO_WAV_FMT_CHUNK_PTR;

typedef struct audioWAV_DATA_CHUNK_T {
    AX_U32 nDataID;  //'d','a','t','a'
    AX_U32 nDataSize;
} AUDIO_WAV_DATA_CHUNK_T, *AUDIO_WAV_DATA_CHUNK_PTR;

typedef struct audioWAV_STRUCT_T {
    AUDIO_WAV_RIFF_CHUNK_T stRiffRegion;
    AUDIO_WAV_FMT_CHUNK_T stFmtRegion;
    AUDIO_WAV_DATA_CHUNK_T stDataRegion;
} AUDIO_WAV_STRUCT_T, *AUDIO_WAV_STRUCT_PTR;

#define AUDIO_AAC_HEADER_SIZE (7)
#define AUDIO_AAC_HEADER_WITH_CRC_SIZE (AUDIO_AAC_HEADER_SIZE + 2)

typedef struct {
    AX_S32 nLoop;
    AX_PAYLOAD_TYPE_E eType;
    AX_CHAR strFileName[FILE_NAME_LEN];
    AX_VOID *pUserData;
    SAMPLE_AUDIO_PLAYFILERESULT_CALLBACK callback;
    SAMPLE_AUDIO_PLAY_FILE_STATUS_E eStatus;
} SAMPLE_AUDIO_PLAY_FILE_ATTR_T;

typedef struct {
    AX_BOOL bInited;

    // play
    AX_BOOL bPlayingFile;
    AX_BOOL bStopPlayFile;
    AX_BOOL bPlayFileThreadRunning;
    SAMPLE_AUDIO_PLAY_FILE_STATUS_E ePlayFileStatus;
    pthread_t hPlayFileThread;
    axfifo_t qPlayFile;
    pthread_mutex_t mtxPlayFile;
    pthread_cond_t cvPlayFile;

    // wait play
    pthread_mutex_t mtxWaitPlayFile;
    pthread_cond_t cvWaitPlayFile;

    // wait stop
    pthread_mutex_t mtxWaitStopPlayFile;
    pthread_cond_t cvWaitStopPlayFile;

    // result callback
    AX_BOOL bResultThreadRunning;
    axfifo_t qResult;
    pthread_mutex_t mtxResult;
    pthread_cond_t cvResult;
    pthread_t hResultThread;
} SAMPLE_AUDIO_PLAY_FILE_INFO_T;

static AX_POOL g_sample_ai_PoolId = AX_INVALID_POOLID;
static SAMPLE_AUDIO_CAPTURE_PARAM_T g_sample_audio_capture_param;

static SAMPLE_AUDIO_GET_THREAD_PARAM_T g_sample_audio_get_thread_param;

static AX_POOL g_sample_adec_PoolId = AX_INVALID_POOLID;
static SAMPLE_AUDIO_PLAY_PARAM_T g_sample_audio_play_param;

static SAMPLE_AUDIO_PLAY_FILE_INFO_T g_sample_audio_play_file_info;

static AX_S32 SampleAudioPlayStop();

static AX_BOOL IsUpTalkVqeEnabled(const AX_AP_UPTALKVQE_ATTR_T *pstVqeAttr) {
    return (AX_BOOL)((pstVqeAttr->stAecCfg.enAecMode != AX_AEC_MODE_DISABLE) ||
                            (pstVqeAttr->stNsCfg.bNsEnable != AX_FALSE) ||
                            (pstVqeAttr->stAgcCfg.bAgcEnable != AX_FALSE) ||
                            (pstVqeAttr->stVadCfg.bVadEnable != AX_FALSE));
}

static AX_BOOL IsDnVqeEnabled(const AX_AP_DNVQE_ATTR_T *pstVqeAttr) {
    return (AX_BOOL)((pstVqeAttr->stNsCfg.bNsEnable != AX_FALSE) ||
                            (pstVqeAttr->stAgcCfg.bAgcEnable != AX_FALSE));
}

static AX_BOOL IsAudioSupport(AX_PAYLOAD_TYPE_E eType) {
    return (AX_BOOL)((PT_G711A == eType) ||
                        (PT_G711U == eType) ||
                        (PT_LPCM == eType) ||
                        (PT_G726 == eType) ||
                        (PT_AAC == eType));

}

static AX_VOID *GetAencStreamThread(AX_VOID *pThreadParam) {
    SAMPLE_AUDIO_GET_THREAD_PARAM_T *t = (SAMPLE_AUDIO_GET_THREAD_PARAM_T *)pThreadParam;
    AX_U32 nChannel = t->nChn;
    AX_S32 ret = AX_SUCCESS;

    AX_CHAR szName[50] = {0};
    sprintf(szName, "aenc_%d", nChannel);
    prctl(PR_SET_NAME, szName);

    while (!t->bExit) {
        {
            if (!g_sample_audio_capture_param.bInited) {
                MSSleep(50);
                continue;
            }
        }

        AX_AUDIO_STREAM_T tStream;
        ret = AX_AENC_GetStream(nChannel, &tStream, -1);

        if (ret != AX_SUCCESS) {
            if (!g_sample_audio_capture_param.bInited) {
                MSSleep(50);
                continue;
            } else {
                COMM_AUDIO_PRT("AX_AENC_GetStream, ret 0x%X", ret);
                continue;
            }
        }

        if (0 == tStream.u32Len || !tStream.pStream) {
            AX_AENC_ReleaseStream(nChannel, &tStream);
            continue;
        }

        if (t->callback) {
            t->callback(&tStream, t->pUserData);
        }

        AX_AENC_ReleaseStream(nChannel, &tStream);
    }

    return (AX_VOID *)0;
}

static AX_S32 GetAudioStreamThreadStart(const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pstCapEntryParam,
                                                    const SAMPLE_AUDIO_STREAM_CALLBACK callback, AX_VOID *pUserData) {
    if (pstCapEntryParam && callback) {
        const SAMPLE_AI_DEV_ATTR_T *pstDevAttr = pstCapEntryParam->pstDevAttr;
        const SAMPLE_AENC_CHN_ATTR_T *pstAencChnAttr = pstCapEntryParam->pstAencChnAttr;

        g_sample_audio_get_thread_param.bExit = AX_FALSE;
        g_sample_audio_get_thread_param.nCardNum = pstDevAttr->nCardNum;
        g_sample_audio_get_thread_param.nDeviceNum = pstDevAttr->nDeviceNum;
        g_sample_audio_get_thread_param.nChn = pstAencChnAttr->nEncChn;
        g_sample_audio_get_thread_param.callback = callback;
        g_sample_audio_get_thread_param.pUserData = pUserData;

        if (0 != pthread_create(&g_sample_audio_get_thread_param.nTid, NULL, GetAencStreamThread, (AX_VOID *)&g_sample_audio_get_thread_param)){
            return -1;
        }
    }

    return 0;
}

static AX_S32 SampleAiInit(const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pstCapEntryParam) {
    AX_S32 s32Ret = -1;

    const SAMPLE_AI_DEV_ATTR_T *pstDevAttr = pstCapEntryParam->pstDevAttr;
    const SAMPLE_AP_UPTALKVQE_ATTR_T *pstUpVqeAttr = pstCapEntryParam->pstUpVqeAttr;
    const SAMPLE_ACODEC_ATTR_T* pstACodecAttr = pstCapEntryParam->pstACodecAttr;
    const SAMPLE_AENC_CHN_ATTR_T *pstAencChnAttr = pstCapEntryParam->pstAencChnAttr;

    if (!pstDevAttr || !pstAencChnAttr) {
        COMM_AUDIO_PRT("input parameter empty");
        return -1;
    }

    AX_U32 card = pstDevAttr->nCardNum;
    AX_U32 device = pstDevAttr->nDeviceNum;
    AX_U32 nOutSampleRate = pstDevAttr->nSampleRate;

    /* Step1.1: link mode */
    {
        AX_MOD_INFO_T Ai_Mod = {AX_ID_AI, card, device};
        AX_MOD_INFO_T Aenc_Mod = {AX_ID_AENC, 0, pstAencChnAttr->nEncChn};

        s32Ret = AX_SYS_Link(&Ai_Mod, &Aenc_Mod);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_SYS_Link failed, ret 0x%X", s32Ret);
            return -1;
        }
    }

    /* Step1.2: AI init */
    s32Ret = AX_AI_Init();
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AI_Init failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

    /* Step1.3: set AI attribute */
    {
        AX_AI_ATTR_T stAttr;
        memset(&stAttr, 0x00, sizeof(stAttr));

        stAttr.enBitwidth = pstDevAttr->eBitWidth;
        stAttr.enLinkMode = AX_LINK_MODE;
        stAttr.enSamplerate = (AX_AUDIO_SAMPLE_RATE_E)pstDevAttr->nSampleRate;
        stAttr.enLayoutMode = pstDevAttr->eLayoutMode;
        stAttr.U32Depth = SAMPLE_AI_DEFAULT_DEPTH;
        if (pstAencChnAttr->eEncType == PT_AAC
            && pstDevAttr->eLayoutMode == AX_AI_MIC_MIC) {
            stAttr.u32PeriodSize = SAMPLE_AI_STEREO_AAC_PERIOD_SIZE;
        } else {
            stAttr.u32PeriodSize = pstDevAttr->nSampleRate / 100;
        }
        stAttr.u32PeriodCount = SAMPLE_AI_PERIOD_COUNT;
        stAttr.u32ChnCnt = SAMPLE_AI_DEFAULT_TINYALSA_CHNCNT;

        s32Ret = AX_AI_SetPubAttr(card,device,&stAttr);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_AI_SetPubAttr failed, ret 0x%X", s32Ret);
            goto EXIT;
        }
    }

    /* Step1.4: attach AI pool */
    if (AX_INVALID_POOLID == g_sample_ai_PoolId) {
        AX_U32 nFrameSize = SAMPLE_AI_DEFAULT_BLK_SIZE;

        if (pstAencChnAttr->eEncType == PT_AAC
            && pstDevAttr->eLayoutMode == AX_AI_MIC_MIC) {
            nFrameSize = SAMPLE_AI_STEREO_AAC_BLK_SIZE;
        }

        g_sample_ai_PoolId = COMMON_SYS_CreatePool(nFrameSize, SAMPLE_AI_DEFAULT_BLK_CNT, "AUDIO_CAP");
    }

    if (g_sample_ai_PoolId == AX_INVALID_POOLID) {
        COMM_AUDIO_PRT("COMMON_SYS_CreatePool failed");
        goto EXIT;
    }

    s32Ret = AX_AI_AttachPool(card, device, g_sample_ai_PoolId);
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AI_AttachPool failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

    /* Step1.5: set AI VQE */
    if (pstUpVqeAttr) {
        AX_AP_UPTALKVQE_ATTR_T stVqeAttr;
        memset(&stVqeAttr, 0, sizeof(stVqeAttr));

        stVqeAttr.s32SampleRate = pstDevAttr->nVqeSampleRate;
        stVqeAttr.u32FrameSamples = pstDevAttr->nVqeSampleRate / 100;
        memcpy(&stVqeAttr.stNsCfg, pstUpVqeAttr->pstNsCfg, sizeof(AX_NS_CONFIG_T));
        memcpy(&stVqeAttr.stAgcCfg, pstUpVqeAttr->pstAgcCfg, sizeof(AX_AGC_CONFIG_T));
        memcpy(&stVqeAttr.stAecCfg, pstUpVqeAttr->pstAecCfg, sizeof(AX_AEC_CONFIG_T));
        memcpy(&stVqeAttr.stVadCfg, pstUpVqeAttr->pstVadCfg, sizeof(AX_VAD_CONFIG_T));

        if (IsUpTalkVqeEnabled(&stVqeAttr)) {
            s32Ret = AX_AI_SetUpTalkVqeAttr(card, device, &stVqeAttr);
            if (s32Ret) {
                COMM_AUDIO_PRT("AX_AI_SetUpTalkVqeAttr failed, ret 0x%X", s32Ret);
                goto EXIT;
            }
            nOutSampleRate = pstDevAttr->nVqeSampleRate;
        }
    }

    /* Step1.6: set ACODEC HPF */
    if (pstACodecAttr && pstACodecAttr->pstHpfCfg && pstACodecAttr->pstHpfCfg->bEnable) {
        AX_ACODEC_FREQ_ATTR_T stHpfAttr;

        memset(&stHpfAttr, 0x00, sizeof(stHpfAttr));
        stHpfAttr.s32Freq = pstACodecAttr->pstHpfCfg->s32Freq;
        stHpfAttr.s32GainDb = pstACodecAttr->pstHpfCfg->s32GainDb;
        stHpfAttr.s32Samplerate = pstDevAttr->nSampleRate;

        s32Ret = AX_ACODEC_RxHpfSetAttr(card, &stHpfAttr);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxHpfSetAttr failed, ret 0x%X", s32Ret);
            goto EXIT;
        }

        s32Ret = AX_ACODEC_RxHpfEnable(card);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxHpfEnable failed, ret 0x%X", s32Ret);
            goto EXIT;
        }
    }

    /* Step1.7: set ACODEC LPF */
    if (pstACodecAttr && pstACodecAttr->pstLpfCfg && pstACodecAttr->pstLpfCfg->bEnable) {
        AX_ACODEC_FREQ_ATTR_T stLpfAttr;

        memset(&stLpfAttr, 0x00, sizeof(stLpfAttr));
        stLpfAttr.s32Freq = pstACodecAttr->pstLpfCfg->s32Freq;
        stLpfAttr.s32GainDb = pstACodecAttr->pstLpfCfg->s32GainDb;
        stLpfAttr.s32Samplerate = pstDevAttr->nSampleRate;

        s32Ret = AX_ACODEC_RxLpfSetAttr(card, &stLpfAttr);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxLpfSetAttr failed, ret 0x%X", s32Ret);
            goto EXIT;
        }

        s32Ret = AX_ACODEC_RxLpfEnable(card);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxLpfEnable failed, ret 0x%X", s32Ret);
            goto EXIT;
        }
    }

    /* Step1.8: set ACODEC EQ */
    if (pstACodecAttr && pstACodecAttr->pstEqCfg && pstACodecAttr->pstEqCfg->bEnable) {
        AX_ACODEC_EQ_ATTR_T stEqAttr;

        memset(&stEqAttr, 0x00, sizeof(stEqAttr));
        memcpy(stEqAttr.s32GainDb, pstACodecAttr->pstEqCfg->s32GainDb, sizeof(stEqAttr.s32GainDb));
        stEqAttr.s32Samplerate = pstDevAttr->nSampleRate;

        s32Ret = AX_ACODEC_RxEqSetAttr(card, &stEqAttr);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxEqSetAttr failed, ret 0x%X", s32Ret);
            goto EXIT;
        }

        s32Ret = AX_ACODEC_RxEqEnable(card);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxEqEnable failed, ret 0x%X", s32Ret);
            goto EXIT;
        }
    }

    /* Step1.9: enable AI device */
    s32Ret = AX_AI_EnableDev(card, device);
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AI_EnableDev failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

    /* Step1.10: enable AI resample */
    if (pstDevAttr->bResample) {
        AX_AUDIO_SAMPLE_RATE_E enOutSampleRate = pstDevAttr->nResRate;
        s32Ret = AX_AI_EnableResample(card, device, enOutSampleRate);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_AI_EnableResample failed, ret 0x%X", s32Ret);
            goto EXIT;
        }

        nOutSampleRate = pstDevAttr->nResRate;
    }

    /* Step1.11: set AI VQE volume */
    s32Ret = AX_AI_SetVqeVolume(card, device, pstDevAttr->fVqeVolume);

    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AI_SetVqeVolume failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

    g_sample_audio_capture_param.stEncoderInfo.nSampleRate = nOutSampleRate;

EXIT:
    return s32Ret;
}

static AX_S32 SampleAiDeinit(const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pstCapEntryParam) {
    AX_S32 s32Ret = -1;

    const SAMPLE_AI_DEV_ATTR_T *pstDevAttr = pstCapEntryParam->pstDevAttr;
    const SAMPLE_ACODEC_ATTR_T* pstACodecAttr = pstCapEntryParam->pstACodecAttr;
    const SAMPLE_AENC_CHN_ATTR_T *pstAencChnAttr = pstCapEntryParam->pstAencChnAttr;

    if (!pstDevAttr || !pstAencChnAttr) {
        COMM_AUDIO_PRT("input parameter empty");
        return -1;
    }

    AX_U32 card = pstDevAttr->nCardNum;
    AX_U32 device = pstDevAttr->nDeviceNum;

    /* Step2.1: disable AI device */
    s32Ret = AX_AI_DisableDev(card, device);

    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AI_DisableDev failed, ret 0x%X", s32Ret);
    }

    /* Step2.1: disable EQ */
    if (pstACodecAttr && pstACodecAttr->pstEqCfg && pstACodecAttr->pstEqCfg->bEnable) {
        s32Ret = AX_ACODEC_RxEqDisable(card);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxEqDisable failed, ret 0x%X", s32Ret);
        }
    }

    /* Step2.2: disable LPF */
    if (pstACodecAttr && pstACodecAttr->pstLpfCfg && pstACodecAttr->pstLpfCfg->bEnable) {
        s32Ret = AX_ACODEC_RxLpfDisable(card);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxLpfDisable failed, ret 0x%X", s32Ret);
        }
    }

    /* Step2.3: disable HPF */
    if (pstACodecAttr && pstACodecAttr->pstHpfCfg && pstACodecAttr->pstHpfCfg->bEnable) {
        s32Ret = AX_ACODEC_RxHpfDisable(card);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ACODEC_RxHpfDisable failed, ret 0x%X", s32Ret);
        }
    }

    /* Step2.4: detach pool */
    s32Ret = AX_AI_DetachPool(card, device);
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AI_DetachPool failed, ret 0x%X", s32Ret);
    }

    /* Step2.5: AI deinit */
    s32Ret = AX_AI_DeInit();
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AI_DeInit failed, ret 0x%X", s32Ret);
    }

    /* Step2.6: unlink */
    {
        AX_MOD_INFO_T Ai_Mod = {AX_ID_AI, card, device};
        AX_MOD_INFO_T Aenc_Mod = {AX_ID_AENC, 0, pstAencChnAttr->nEncChn};

        s32Ret = AX_SYS_UnLink(&Ai_Mod, &Aenc_Mod);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_SYS_UnLink failed, ret 0x%X", s32Ret);
        }
    }

    return s32Ret;
}

static AX_S32 SampleAencInit(const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pstCapEntryParam) {
    AX_S32 s32Ret = -1;

    const SAMPLE_AI_DEV_ATTR_T *pstDevAttr = pstCapEntryParam->pstDevAttr;
    const SAMPLE_AENC_CHN_ATTR_T *pstAencChnAttr = pstCapEntryParam->pstAencChnAttr;

    if (!pstAencChnAttr) {
        COMM_AUDIO_PRT("input parameter empty");
        return -1;
    }

    /* Step2.1: aenc init */
    s32Ret = AX_AENC_Init();

    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AENC_Init failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

    {
        AX_U32 nAOT = 0;

        AX_AENC_CHN_ATTR_T tAttr;

#ifdef QSDEMO_AUDIO_AAC_SUPPORT
        AX_AENC_AAC_ENCODER_ATTR_T aacEncoderAttr = {
            .enAacType = AX_AAC_TYPE_AAC_LC,
            .enTransType = AX_AAC_TRANS_TYPE_ADTS,
            .enChnMode = (pstDevAttr->eLayoutMode == AX_AI_MIC_MIC) ? AX_AAC_CHANNEL_MODE_2 : AX_AAC_CHANNEL_MODE_1,
            .u32GranuleLength = 1024,
            .u32SampleRate = pstAencChnAttr->nEncSampleRate,
            .u32BitRate = pstAencChnAttr->nEncBitRate
        };
#endif

        AX_AENC_G726_ENCODER_ATTR_T stG726EncoderAttr = {
            .u32BitRate = pstAencChnAttr->nEncBitRate
        };

        AX_AENC_G723_ENCODER_ATTR_T stG723EncoderAttr = {
            .u32BitRate = pstAencChnAttr->nEncBitRate
        };

        /* Step2.2: encoder init */
        tAttr.enType = pstAencChnAttr->eEncType;
        tAttr.u32BufSize = SAMPLE_AENC_DEFAULT_OUT_DEPTH;
        tAttr.enLinkMode = AX_LINK_MODE;
        if (tAttr.enType == PT_G726) {
            tAttr.u32PtNumPerFrm = 480;
            tAttr.pValue = &stG726EncoderAttr;
        } else if (tAttr.enType == PT_G723) {
            tAttr.u32PtNumPerFrm = 480;
            tAttr.pValue = &stG723EncoderAttr;
        }
#ifdef QSDEMO_AUDIO_AAC_SUPPORT
        else if (tAttr.enType == PT_AAC) {
            tAttr.u32PtNumPerFrm = 1024;
            tAttr.pValue = &aacEncoderAttr;
            s32Ret = AX_AENC_FaacInit();

            if (s32Ret) {
                COMM_AUDIO_PRT("AX_AENC_AAC_Init failed, ret 0x%X", s32Ret);
                goto EXIT;
            }

            nAOT = AX_AAC_TYPE_AAC_LC;
        }
#endif
        else {
            tAttr.u32PtNumPerFrm = 1024;
            tAttr.pValue = NULL;
        }

        /* Step2.3: create channel */
        {
            AENC_CHN aeChn = pstAencChnAttr->nEncChn;

            s32Ret = AX_AENC_CreateChn(aeChn, &tAttr);

            if (s32Ret) {
                COMM_AUDIO_PRT("AX_AENC_CreateChn failed, ret 0x%X", s32Ret);
                goto EXIT;
            }
        }

        // encoder info
        g_sample_audio_capture_param.stEncoderInfo.bEnable = AX_TRUE;
        g_sample_audio_capture_param.stEncoderInfo.ePt = pstAencChnAttr->eEncType;
        g_sample_audio_capture_param.stEncoderInfo.nBitrate = pstAencChnAttr->nEncBitRate;
        if (tAttr.enType == PT_AAC) {
            g_sample_audio_capture_param.stEncoderInfo.nSampleRate = pstAencChnAttr->nEncSampleRate;
        }
        g_sample_audio_capture_param.stEncoderInfo.nChnCnt = (pstDevAttr->eLayoutMode == AX_AI_MIC_MIC) ? 2 : 1;
        g_sample_audio_capture_param.stEncoderInfo.nAOT = nAOT;
    }

EXIT:
    return s32Ret;
}

static AX_S32 SampleAencDeinit(const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pstCapEntryParam) {
    AX_S32 s32Ret = -1;

    const SAMPLE_AENC_CHN_ATTR_T *pstAencChnAttr = pstCapEntryParam->pstAencChnAttr;

    if (!pstAencChnAttr) {
        COMM_AUDIO_PRT("input parameter empty");
        return -1;
    }

    AENC_CHN aeChn = pstAencChnAttr->nEncChn;

    /* Step1.1: destroy chn */
    s32Ret = AX_AENC_DestroyChn(aeChn);
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AENC_DestroyChn failed, ret 0x%X", s32Ret);
    }

    /* Step1.2: encoder deinit */
    switch (pstAencChnAttr->eEncType) {
#ifdef QSDEMO_AUDIO_AAC_SUPPORT
    case PT_AAC:
        {
            s32Ret = AX_AENC_FaacDeInit();

            if (s32Ret) {
                COMM_AUDIO_PRT("AX_AENC_AAC_DeInit failed, ret 0x%X", s32Ret);
            }
        }
        break;
#endif

    default:
        break;
    }

    /* Step1.3: adec deinit */
    s32Ret = AX_AENC_DeInit();

    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AENC_DeInit failed, ret 0x%X", s32Ret);
    }

    return s32Ret;
}

static AX_S32 SampleCaptureInit(const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pstCapEntryParam) {
    AX_S32 s32Ret = -1;

    /* Step1: Ai Init */
    s32Ret = SampleAiInit(pstCapEntryParam);
    if (s32Ret) {
        COMM_AUDIO_PRT("SampleAiInit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }

    /* Step2: Aenc Init */
    s32Ret = SampleAencInit(pstCapEntryParam);
    if (s32Ret) {
        COMM_AUDIO_PRT("SampleAencInit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }

    g_sample_audio_capture_param.bInited = AX_TRUE;

EXIT:
    return s32Ret;
}

static AX_S32 SampleCaptureDeinit(AX_VOID) {
    AX_S32 s32Ret = -1;

    g_sample_audio_capture_param.bInited = AX_FALSE;

    SAMPLE_AI_DEV_ATTR_T *pstDevAttr = &g_sample_audio_capture_param.stDevAttr;
    SAMPLE_AP_UPTALKVQE_ATTR_T stUpVqeAttr = {
            .pstAecCfg = &g_sample_audio_capture_param.stAecCfg,
            .pstNsCfg = &g_sample_audio_capture_param.stNsCfg,
            .pstAgcCfg = &g_sample_audio_capture_param.stAgcCfg,
            .pstVadCfg = &g_sample_audio_capture_param.stVadCfg
        };
    SAMPLE_ACODEC_ATTR_T stAcodecAttr = {
            .pstEqCfg = &g_sample_audio_capture_param.stEqCfg,
            .pstHpfCfg = &g_sample_audio_capture_param.stHpfCfg,
            .pstLpfCfg = &g_sample_audio_capture_param.stLpfCfg,
        };
    SAMPLE_AENC_CHN_ATTR_T *pstAencChnAttr = &g_sample_audio_capture_param.stAencChnAttr;

    SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T tEntryParam = {
            .pstDevAttr = pstDevAttr,
            .pstUpVqeAttr = &stUpVqeAttr,
            .pstACodecAttr = &stAcodecAttr,
            .pstAencChnAttr = pstAencChnAttr,
        };

    /* Step1: Aenc Deinit */
    s32Ret = SampleAencDeinit(&tEntryParam);
    if (s32Ret) {
        COMM_AUDIO_PRT("SampleAencDeinit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }

    /* Step2: Ai Deinit */
    s32Ret = SampleAiDeinit(&tEntryParam);
    if (s32Ret) {
        COMM_AUDIO_PRT("SampleAiDeinit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }

EXIT:
    return s32Ret;
}

static AX_S32 SampleAoInit(const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T* pstPlayEntryParam) {
    AX_S32 s32Ret = -1;

    const SAMPLE_AO_DEV_ATTR_T *pstDevAttr = pstPlayEntryParam->pstDevAttr;
    const SAMPLE_DnVQE_ATTR_T *pstDnVqeAttr = pstPlayEntryParam->pstDnVqeAttr;
    const SAMPLE_ADEC_CHN_ATTR_T *pstAdecChnAttr = pstPlayEntryParam->pstAdecChnAttr;

    if (!pstDevAttr || !pstAdecChnAttr) {
        COMM_AUDIO_PRT("input parameter empty");
        return -1;
    }

    AX_U32 card = pstDevAttr->nCardNum;
    AX_U32 device = pstDevAttr->nDeviceNum;

    /* Step1.1: link mode */
    {
        AX_MOD_INFO_T Adec_Mod = {AX_ID_ADEC, 0, pstAdecChnAttr->nDecChn};
        AX_MOD_INFO_T Ao_Mod = {AX_ID_AO, card, device};

        s32Ret = AX_SYS_Link(&Adec_Mod, &Ao_Mod);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_SYS_Link failed, ret 0x%X", s32Ret);
            return -1;
        }
    }

    /* Step1.2: AO init */
    s32Ret = AX_AO_Init();
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AO_Init failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

    /* Step1.3: set AO attribute */
    {
        AX_AO_ATTR_T stAttr;
        memset(&stAttr, 0x00, sizeof(stAttr));

        stAttr.enBitwidth = pstDevAttr->eBitWidth;
        stAttr.enSoundmode = pstDevAttr->enSoundmode;
        stAttr.enLinkMode = AX_LINK_MODE;
        stAttr.enSamplerate = (AX_AUDIO_SAMPLE_RATE_E)pstDevAttr->nSampleRate;
        stAttr.U32Depth = SAMPLE_AO_DEFAULT_DEPTH;
        if (pstAdecChnAttr->eDecType == PT_AAC
            && pstDevAttr->enSoundmode == AX_AUDIO_SOUND_MODE_STEREO) {
            stAttr.u32PeriodSize = SAMPLE_AO_STEREO_AAC_PERIOD_SIZE;
        } else {
            stAttr.u32PeriodSize = pstDevAttr->nSampleRate / 100;
        }
        stAttr.u32PeriodCount = SAMPLE_AO_PERIOD_COUNT;
        stAttr.bInsertSilence = AX_FALSE;
        stAttr.u32ChnCnt = SAMPLE_AO_DEFAULT_TINYALSA_CHNCNT;

        s32Ret = AX_AO_SetPubAttr(card, device,&stAttr);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_AO_SetPubAttr failed, ret 0x%X", s32Ret);
            goto EXIT;
        }
    }

    /* Step1.4: set AO VQE */
    if (pstDnVqeAttr) {
        AX_AP_DNVQE_ATTR_T stVqeAttr;
        memset(&stVqeAttr, 0, sizeof(stVqeAttr));

        stVqeAttr.s32SampleRate = pstDevAttr->nVqeSampleRate;
        stVqeAttr.u32FrameSamples = pstDevAttr->nVqeSampleRate / 100;
        memcpy(&stVqeAttr.stNsCfg, pstDnVqeAttr->pstNsCfg, sizeof(AX_NS_CONFIG_T));
        memcpy(&stVqeAttr.stAgcCfg, pstDnVqeAttr->pstAgcCfg, sizeof(AX_AGC_CONFIG_T));

        if (IsDnVqeEnabled(&stVqeAttr)) {
            s32Ret = AX_AO_SetDnVqeAttr(card, device, &stVqeAttr);
            if (s32Ret) {
                COMM_AUDIO_PRT("AX_AO_SetDnVqeAttr failed, ret 0x%X", s32Ret);
                goto EXIT;
            }
        }
    }

    /* Step1.5: enable AO device */
    s32Ret = AX_AO_EnableDev(card, device);
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AO_EnableDev failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

    /* Step1.6: enable AO resample */
    if (pstDevAttr->bResample) {
        AX_AUDIO_SAMPLE_RATE_E enInSampleRate = pstDevAttr->nResRate;
        s32Ret = AX_AO_EnableResample(card, device, enInSampleRate);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_AO_EnableResample failed, ret 0x%X", s32Ret);
            goto EXIT;
        }
    }

    /* Step1.6: set AO VQE volume */
    s32Ret = AX_AO_SetVqeVolume(card, device, pstDevAttr->fVqeVolume);

    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AO_SetVqeVolume failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

EXIT:
    return s32Ret;
}

static AX_S32 SampleAoDeinit(const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T* pstPlayEntryParam) {
    AX_S32 s32Ret = -1;

    const SAMPLE_AO_DEV_ATTR_T *pstDevAttr = pstPlayEntryParam->pstDevAttr;

    if (!pstDevAttr) {
        COMM_AUDIO_PRT("input parameter empty");
        return -1;
    }

    AX_U32 card = pstDevAttr->nCardNum;
    AX_U32 device = pstDevAttr->nDeviceNum;

    /* Step2.1: disable AO device */
    s32Ret = AX_AO_DisableDev(card, device);

    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AO_DisableDev failed, ret 0x%X", s32Ret);
    }

    /* Step2.1: AO deinit */
    s32Ret = AX_AO_DeInit();
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_AO_DeInit failed, ret 0x%X", s32Ret);
    }

    return s32Ret;
}

static AX_S32 SampleAdecInit(const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T* pstPlayEntryParam) {
    AX_S32 s32Ret = -1;

    const SAMPLE_AO_DEV_ATTR_T *pstDevAttr = pstPlayEntryParam->pstDevAttr;
    const SAMPLE_ADEC_CHN_ATTR_T *pstAdecChnAttr = pstPlayEntryParam->pstAdecChnAttr;

    if (!pstDevAttr || !pstAdecChnAttr) {
        COMM_AUDIO_PRT("input parameter empty");
        return -1;
    }

    /* Step2.0: play stop */
    SampleAudioPlayStop();

    /* Step2.1: adec init */
    s32Ret = AX_ADEC_Init();
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_ADEC_Init failed, ret 0x%X", s32Ret);
        goto EXIT;
    }

    {
#ifdef QSDEMO_AUDIO_AAC_SUPPORT
        AX_ADEC_AAC_DECODER_ATTR_T aacDecoderAttr = {
            .enTransType = AX_AAC_TRANS_TYPE_ADTS,
        };
#endif

        AX_ADEC_G726_DECODER_ATTR_T stG726DecoderAttr ={
            .u32BitRate = 32000
        };

        AX_ADEC_G723_DECODER_ATTR_T stG723DecoderAttr ={
            .u32BitRate = 24000
        };

        ADEC_CHN adChn = pstAdecChnAttr->nDecChn;

        AX_ADEC_CHN_ATTR_T pstAttr;
        pstAttr.enType = pstAdecChnAttr->eDecType;
        pstAttr.u32BufSize = SAMPLE_ADEC_DEFAULT_IN_DEPTH;
        pstAttr.enLinkMode = AX_LINK_MODE;
        pstAttr.u32MaxFrameSize = 0;

        /* Step2.2: decoder init */
        switch (pstAttr.enType) {
#ifdef QSDEMO_AUDIO_AAC_SUPPORT
        case PT_AAC:
            {
                pstAttr.pValue = &aacDecoderAttr;
                s32Ret = AX_ADEC_FaacInit(); // faac not support aac decoder

                if (s32Ret) {
                    COMM_AUDIO_PRT("AX_ADEC_FaacInit failed, ret 0x%X", s32Ret);
                    goto EXIT;
                }
            }
            break;
#endif

        case PT_G726:
            pstAttr.pValue = &stG726DecoderAttr;
            break;

        case PT_G723:
            pstAttr.pValue = &stG723DecoderAttr;
            break;

        default:
            pstAttr.pValue = NULL;
            break;
        }

        /* Step2.3: create channel */
        s32Ret = AX_ADEC_CreateChn(adChn, &pstAttr);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ADEC_FaacInit failed, ret 0x%X", s32Ret);
            goto EXIT;
        }

        /* Step2.4: attach AO pool */
        if (AX_INVALID_POOLID == g_sample_adec_PoolId) {
            AX_U32 nFrameSize = SAMPLE_ADEC_DEFAULT_BLK_SIZE;

            if (pstAdecChnAttr->eDecType == PT_AAC
                && pstDevAttr->enSoundmode == AX_AUDIO_SOUND_MODE_STEREO) {
                nFrameSize = SAMPLE_ADEC_STEREO_AAC_BLK_SIZE;
            }

            g_sample_adec_PoolId = COMMON_SYS_CreatePool(nFrameSize, SAMPLE_ADEC_DEFAULT_BLK_CNT, "AUDIO_PLAY");
        }

        if (AX_INVALID_POOLID == g_sample_adec_PoolId) {
            COMM_AUDIO_PRT("COMMON_SYS_CreatePool failed");
            goto EXIT;
        }

        s32Ret = AX_ADEC_AttachPool(adChn, g_sample_adec_PoolId);
        if (s32Ret) {
            COMM_AUDIO_PRT("AX_ADEC_AttachPool failed, ret 0x%X", s32Ret);
            goto EXIT;
        }
    }

EXIT:
    return s32Ret;
}

static AX_S32 SampleAdecDeinit(const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T* pstPlayEntryParam) {
    AX_S32 s32Ret = -1;

    const SAMPLE_ADEC_CHN_ATTR_T *pstAdecChnAttr = pstPlayEntryParam->pstAdecChnAttr;

    if (!pstAdecChnAttr) {
        COMM_AUDIO_PRT("input parameter empty");
        return -1;
    }

    /* Step1.1: dencoder deinit */
    switch (pstAdecChnAttr->eDecType) {
#ifdef QSDEMO_AUDIO_AAC_SUPPORT
    case PT_AAC:
        {
            s32Ret = AX_ADEC_FaacDeInit();

            if (s32Ret) {
                COMM_AUDIO_PRT("AX_ADEC_FaacDeInit failed, ret 0x%X", s32Ret);
            }
        }
        break;
#endif

    default:
        break;
    }

    ADEC_CHN adChn = pstAdecChnAttr->nDecChn;

    /* Step1.2: detach pool */
    s32Ret = AX_ADEC_DetachPool(adChn);
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_ADEC_DetachPool failed, ret 0x%X", s32Ret);
    }

    /* Step1.3: destroy chn */
    s32Ret = AX_ADEC_DestroyChn(adChn);
    if (s32Ret) {
        COMM_AUDIO_PRT("AX_ADEC_DestroyChn failed, ret 0x%X", s32Ret);
    }

    /* Step1.4: adec deinit */
    s32Ret = AX_ADEC_DeInit();

    if (s32Ret) {
        COMM_AUDIO_PRT("AX_ADEC_DeInit failed, ret 0x%X", s32Ret);
    }

    return s32Ret;
}

static AX_S32 SamplePlayInit(const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T* pstPlayEntryParam) {
    AX_S32 s32Ret = -1;

    /* Step3: Ao Init */
    s32Ret = SampleAoInit(pstPlayEntryParam);
    if (s32Ret) {
        COMM_AUDIO_PRT("SampleAoInit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }

    /* Step4: Adec Init */
    s32Ret = SampleAdecInit(pstPlayEntryParam);
    if (s32Ret) {
        COMM_AUDIO_PRT("SampleAdecInit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }

    g_sample_audio_play_param.bInited = AX_TRUE;

EXIT:
    return s32Ret;
}

static AX_S32 SamplePlayDeinit(AX_VOID) {
    AX_S32 s32Ret = -1;

    g_sample_audio_play_param.bInited = AX_FALSE;

    SAMPLE_AO_DEV_ATTR_T *pstDevAttr = &g_sample_audio_play_param.stDevAttr;
    SAMPLE_DnVQE_ATTR_T stDnVqeAttr = {
            .pstNsCfg = &g_sample_audio_play_param.stNsCfg,
            .pstAgcCfg = &g_sample_audio_play_param.stAgcCfg,
        };
    SAMPLE_ADEC_CHN_ATTR_T *pstAdecChnAttr = &g_sample_audio_play_param.stAdecChnAttr;

    SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T tEntryParam = {
            .pstDevAttr = pstDevAttr,
            .pstDnVqeAttr = &stDnVqeAttr,
            .pstAdecChnAttr = pstAdecChnAttr,
        };

    /* Step3: Adec Deinit */
    s32Ret = SampleAdecDeinit(&tEntryParam);
    if (s32Ret) {
        COMM_AUDIO_PRT("SampleAdecDeinit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }

    /* Step3: Ao Deinit */
    s32Ret = SampleAoDeinit(&tEntryParam);
    if (s32Ret) {
        COMM_AUDIO_PRT("SampleAoDeinit fail, ret:0x%x", s32Ret);
        goto EXIT;
    }

EXIT:
    return s32Ret;
}

AX_S32 COMMON_AUDIO_Init(const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pstCapEntryParam,
                                const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T* pstPlayEntryParam,
                                const SAMPLE_AUDIO_STREAM_CALLBACK callback, AX_VOID *pUserData) {
    AX_S32 s32Ret = -1;

    if (pstCapEntryParam) {
        s32Ret = SampleCaptureInit(pstCapEntryParam);

        if (s32Ret) {
            goto EXIT;
        }

        g_sample_audio_capture_param.bEnable = AX_TRUE;

        if (pstCapEntryParam->pstDevAttr) {
            memcpy(&g_sample_audio_capture_param.stDevAttr,
                    pstCapEntryParam->pstDevAttr, sizeof(g_sample_audio_capture_param.stDevAttr));
        }

        if (pstCapEntryParam->pstUpVqeAttr && pstCapEntryParam->pstUpVqeAttr->pstAecCfg) {
            memcpy(&g_sample_audio_capture_param.stAecCfg,
                    pstCapEntryParam->pstUpVqeAttr->pstAecCfg, sizeof(g_sample_audio_capture_param.stAecCfg));
        }

        if (pstCapEntryParam->pstUpVqeAttr && pstCapEntryParam->pstUpVqeAttr->pstNsCfg) {
            memcpy(&g_sample_audio_capture_param.stNsCfg,
                    pstCapEntryParam->pstUpVqeAttr->pstNsCfg, sizeof(g_sample_audio_capture_param.stNsCfg));
        }

        if (pstCapEntryParam->pstUpVqeAttr && pstCapEntryParam->pstUpVqeAttr->pstAgcCfg) {
            memcpy(&g_sample_audio_capture_param.stAgcCfg,
                    pstCapEntryParam->pstUpVqeAttr->pstAgcCfg, sizeof(g_sample_audio_capture_param.stAgcCfg));
        }

        if (pstCapEntryParam->pstUpVqeAttr && pstCapEntryParam->pstUpVqeAttr->pstVadCfg) {
            memcpy(&g_sample_audio_capture_param.stVadCfg,
                    pstCapEntryParam->pstUpVqeAttr->pstVadCfg, sizeof(g_sample_audio_capture_param.stVadCfg));
        }

        if (pstCapEntryParam->pstACodecAttr && pstCapEntryParam->pstACodecAttr->pstEqCfg) {
            memcpy(&g_sample_audio_capture_param.stEqCfg,
                    pstCapEntryParam->pstACodecAttr->pstEqCfg, sizeof(g_sample_audio_capture_param.stEqCfg));
        }

        if (pstCapEntryParam->pstACodecAttr && pstCapEntryParam->pstACodecAttr->pstHpfCfg) {
            memcpy(&g_sample_audio_capture_param.stHpfCfg,
                    pstCapEntryParam->pstACodecAttr->pstHpfCfg, sizeof(g_sample_audio_capture_param.stHpfCfg));
        }

        if (pstCapEntryParam->pstACodecAttr && pstCapEntryParam->pstACodecAttr->pstLpfCfg) {
            memcpy(&g_sample_audio_capture_param.stLpfCfg,
                    pstCapEntryParam->pstACodecAttr->pstLpfCfg, sizeof(g_sample_audio_capture_param.stLpfCfg));
        }

        if (pstCapEntryParam->pstAencChnAttr) {
            memcpy(&g_sample_audio_capture_param.stAencChnAttr,
                    pstCapEntryParam->pstAencChnAttr, sizeof(g_sample_audio_capture_param.stAencChnAttr));
        }

        if (callback) {
            GetAudioStreamThreadStart(pstCapEntryParam, callback, pUserData);
        }
    }

    if (pstPlayEntryParam) {
        if (pstPlayEntryParam->bKeepAlive) {
            s32Ret = SamplePlayInit(pstPlayEntryParam);

            if (s32Ret) {
                goto EXIT;
            }
        } else {
            s32Ret = 0;
        }

        g_sample_audio_play_param.bEnable = AX_TRUE;
        g_sample_audio_play_param.bKeepAlive = pstPlayEntryParam->bKeepAlive;

        if (pstPlayEntryParam->pstDevAttr) {
            memcpy(&g_sample_audio_play_param.stDevAttr,
                    pstPlayEntryParam->pstDevAttr, sizeof(g_sample_audio_play_param.stDevAttr));
        }

        if (pstPlayEntryParam->pstDnVqeAttr && pstPlayEntryParam->pstDnVqeAttr->pstNsCfg) {
            memcpy(&g_sample_audio_play_param.stNsCfg,
                    pstPlayEntryParam->pstDnVqeAttr->pstNsCfg, sizeof(g_sample_audio_play_param.stNsCfg));
        }

        if (pstPlayEntryParam->pstDnVqeAttr && pstPlayEntryParam->pstDnVqeAttr->pstAgcCfg) {
            memcpy(&g_sample_audio_play_param.stAgcCfg,
                    pstPlayEntryParam->pstDnVqeAttr->pstAgcCfg, sizeof(g_sample_audio_play_param.stAgcCfg));
        }

        if (pstPlayEntryParam->pstAdecChnAttr) {
            memcpy(&g_sample_audio_play_param.stAdecChnAttr,
                    pstPlayEntryParam->pstAdecChnAttr, sizeof(g_sample_audio_play_param.stAdecChnAttr));
        }
    }

EXIT:
    return s32Ret;
}

AX_S32 COMMON_AUDIO_Deinit(AX_VOID) {
    AX_S32 s32Ret = -1;

    if (g_sample_audio_capture_param.bEnable) {
        /* Step1: stop aenc stream thread */
        g_sample_audio_get_thread_param.bExit = AX_TRUE;

        if (g_sample_audio_get_thread_param.nTid) {
            pthread_join(g_sample_audio_get_thread_param.nTid, NULL);
            g_sample_audio_get_thread_param.nTid = 0;
        }

        /* Step2: capture Deinit */
        if (g_sample_audio_capture_param.bInited) {
            s32Ret = SampleCaptureDeinit();
        }

        if (g_sample_ai_PoolId != AX_INVALID_POOLID) {
            COMMON_SYS_DestroyPool(g_sample_ai_PoolId);
            g_sample_ai_PoolId = AX_INVALID_POOLID;
        }

        g_sample_audio_capture_param.bEnable = AX_FALSE;
    }

    if (g_sample_audio_play_param.bEnable) {
        /* Step3: play Deinit */
        if (g_sample_audio_play_param.bInited) {
            s32Ret = SamplePlayDeinit();
        }

        if (g_sample_adec_PoolId != AX_INVALID_POOLID) {
            COMMON_SYS_DestroyPool(g_sample_adec_PoolId);
            g_sample_adec_PoolId = AX_INVALID_POOLID;
        }

        g_sample_audio_play_param.bEnable = AX_FALSE;
    }

    return s32Ret;
}

AX_S32 COMMON_AUDIO_GetEncoderInfo(SAMPLE_AUDIO_ENCODER_INFO_T* pstEncoderInfo) {
    if (g_sample_audio_capture_param.stEncoderInfo.bEnable
        && pstEncoderInfo) {
        memcpy(pstEncoderInfo,
                &g_sample_audio_capture_param.stEncoderInfo,
                sizeof(g_sample_audio_capture_param.stEncoderInfo));

        return 0;
    }

    return -1;
}

static AX_S32 SampleAudioPlay(AX_AUDIO_STREAM_T *pstStream) {
    static AX_U32 u32SeqNum = 0;

    pstStream->u32Seq = ++u32SeqNum;

    return AX_ADEC_SendStream(g_sample_audio_play_param.stAdecChnAttr.nDecChn, pstStream, AX_TRUE);
}

static AX_VOID *ResultCallbackThread(AX_VOID* param) {
    AX_CHAR szName[50] = {0};
    sprintf(szName, "APP_AFPLAY_Res");
    prctl(PR_SET_NAME, szName);

    SAMPLE_AUDIO_PLAY_FILE_ATTR_T stPlayResult;

    g_sample_audio_play_file_info.bResultThreadRunning = AX_TRUE;

    while (g_sample_audio_play_file_info.bResultThreadRunning) {
        // wait result
        pthread_mutex_lock(&g_sample_audio_play_file_info.mtxResult);
        while (axfifo_size(&g_sample_audio_play_file_info.qResult) == 0
                && g_sample_audio_play_file_info.bResultThreadRunning) {
            pthread_cond_wait(&g_sample_audio_play_file_info.cvResult, &g_sample_audio_play_file_info.mtxResult);
        }
        pthread_mutex_unlock(&g_sample_audio_play_file_info.mtxResult);

        if (axfifo_size(&g_sample_audio_play_file_info.qResult) > 0) {
            memset(&stPlayResult, 0x00, sizeof(stPlayResult));
            axfifo_pop(&g_sample_audio_play_file_info.qResult, &stPlayResult, sizeof(stPlayResult));

            if (stPlayResult.callback) {
                SAMPLE_AUDIO_PLAY_FILE_RESULT_T stResult;
                stResult.eType = stPlayResult.eType;
                stResult.pstrFileName = (AX_CHAR *)stPlayResult.strFileName;
                stResult.eStatus = stPlayResult.eStatus;
                stResult.pUserData = stPlayResult.pUserData;

                stPlayResult.callback(&stResult);
            }
        }
    }

    return (AX_VOID *)0;
}

static AX_VOID ProcessPlayFile(SAMPLE_AUDIO_PLAY_FILE_ATTR_T* pstPlayFileAttr) {
    //COMM_AUDIO_PRT("+++");

    AX_S32 s32Ret = -1;
    FILE *pFile = NULL;
    AX_S32 nLoop = 0;
    AX_U8 *pBuffer = NULL;
    AX_S32 nReadSize = 960;

    pFile = fopen(pstPlayFileAttr->strFileName, "rb");

    if (!pFile) {
        pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
        COMM_AUDIO_PRT("open file (%s) failed", pstPlayFileAttr->strFileName);
        goto EXIT;
    }

    //COMM_AUDIO_PRT("start play file: %s", pstPlayFileAttr->strFileName);

    switch (pstPlayFileAttr->eType) {
        case PT_G711A:
        case PT_G711U:
            nReadSize = 960;
            break;

        case PT_AAC:
            nReadSize = 10240;
            break;

        default:
            nReadSize = 960;
            break;
    }

    pBuffer = (AX_U8 *)malloc(nReadSize);

    if (!pBuffer) {
        pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
        COMM_AUDIO_PRT("alloc memory[%d] failed", nReadSize);
        goto EXIT;
    }

    do {
        AX_S32 nTotalSize = 0;

        do {
            AX_S32 nNumRead = 0;
            AX_S32 nBufferSize = 0;

            if (PT_AAC == pstPlayFileAttr->eType) {
                AX_U8 *pPacket = (AX_U8 *)pBuffer;
                AX_S32 nPacketSize = 0;

                {
                    nNumRead = fread(pPacket, 1, AUDIO_AAC_HEADER_SIZE, pFile);
                    if (nNumRead != AUDIO_AAC_HEADER_SIZE) {
                        break;
                    }

                    if (pPacket[0] != 0xff || (pPacket[1] & 0xf0) != 0xf0) {
                        pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
                        COMM_AUDIO_PRT("Not an ADTS packet");
                        break;
                    }

                    /* Set to 1 if there is no CRC and 0 if there is CRC */
                    AX_BOOL bNoCRC = (1 == (pPacket[1] & 0x1)) ? AX_TRUE : AX_FALSE;

                    nPacketSize = ((pPacket[3] & 0x03) << 11) | (pPacket[4] << 3) | (pPacket[5] >> 5);

                    if (bNoCRC) {
                        if (nPacketSize < AUDIO_AAC_HEADER_SIZE || nPacketSize > nReadSize) {
                            pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
                            COMM_AUDIO_PRT("Invalid packet size(%d)", nPacketSize);
                            break;
                        } else {
                            nNumRead = fread(pPacket + AUDIO_AAC_HEADER_SIZE, 1, nPacketSize - AUDIO_AAC_HEADER_SIZE, pFile);

                            if (nNumRead != nPacketSize - AUDIO_AAC_HEADER_SIZE) {
                                pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
                                COMM_AUDIO_PRT("Partial packet");
                                break;
                            }
                        }
                    } else {
                        if (nPacketSize < AUDIO_AAC_HEADER_WITH_CRC_SIZE || nPacketSize > nReadSize) {
                            pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
                            COMM_AUDIO_PRT("Invalid packet size(%d)", nPacketSize);
                            break;
                        } else {
                            nNumRead =
                                fread(pPacket + AUDIO_AAC_HEADER_WITH_CRC_SIZE, 1, nPacketSize - AUDIO_AAC_HEADER_WITH_CRC_SIZE, pFile);

                            if (nNumRead != nPacketSize - AUDIO_AAC_HEADER_WITH_CRC_SIZE) {
                                pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
                                COMM_AUDIO_PRT("Partial packet");
                                break;
                            }
                        }
                    }
                }

                if (nNumRead > 0) {
                    nBufferSize = nPacketSize;
                    nTotalSize += nBufferSize;
                }
            } else if (PT_LPCM == pstPlayFileAttr->eType) {
                if (0 == nTotalSize) {
                    nNumRead = fread(pBuffer, 1, sizeof(AUDIO_WAV_STRUCT_T), pFile);

                    if (nNumRead != sizeof(AUDIO_WAV_STRUCT_T)) {
                        pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
                        COMM_AUDIO_PRT("Error wav format file");
                        break;
                    }

                    AUDIO_WAV_STRUCT_PTR pstWavHeader = (AUDIO_WAV_STRUCT_PTR)pBuffer;

                    if (AUDIO_ID_RIFF != pstWavHeader->stRiffRegion.nChunkID ||
                        AUDIO_ID_WAVE != pstWavHeader->stRiffRegion.nFormat
                        // only support mono
                        || pstWavHeader->stFmtRegion.nFmtChannels > 1) {
                        pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
                        COMM_AUDIO_PRT(
                            "Invalid wav file: %c%c%c%c %c%c%c%c SampleRate: %d, ByteRate: %d, FmtChannels: %d, BitsPerSample: %d, "
                            "DataSize: %d",
                            (AX_U8)(pstWavHeader->stRiffRegion.nChunkID & 0xFF), (AX_U8)((pstWavHeader->stRiffRegion.nChunkID >> 8) & 0xFF),
                            (AX_U8)((pstWavHeader->stRiffRegion.nChunkID >> 16) & 0xFF),
                            (AX_U8)((pstWavHeader->stRiffRegion.nChunkID >> 24) & 0xFF), (AX_U8)(pstWavHeader->stRiffRegion.nFormat & 0xFF),
                            (AX_U8)((pstWavHeader->stRiffRegion.nFormat >> 8) & 0xFF),
                            (AX_U8)((pstWavHeader->stRiffRegion.nFormat >> 16) & 0xFF),
                            (AX_U8)((pstWavHeader->stRiffRegion.nFormat >> 24) & 0xFF), pstWavHeader->stFmtRegion.nSampleRate,
                            pstWavHeader->stFmtRegion.nByteRate, pstWavHeader->stFmtRegion.nFmtChannels,
                            pstWavHeader->stFmtRegion.nBitsPerSample, pstWavHeader->stDataRegion.nDataSize);
                        break;
                    }
                }

                nNumRead = fread(pBuffer, 1, nReadSize, pFile);

                if (nNumRead > 0) {
                    nBufferSize = nNumRead;
                    nTotalSize += nBufferSize;
                }
            } else {
                nNumRead = fread(pBuffer, 1, nReadSize, pFile);

                if (nNumRead > 0) {
                    nBufferSize = nNumRead;  // nPcmSize != nNumRead memleak
                    nTotalSize += nBufferSize;
                }
            }

            if (nBufferSize > 0) {
                {
                    AX_AUDIO_STREAM_T stStream;
                    memset(&stStream, 0x00, sizeof(stStream));

                    stStream.pStream = (AX_U8 *)pBuffer;
                    stStream.u64PhyAddr = 0;
                    stStream.u32Len = nBufferSize;
                    stStream.bEof = AX_FALSE;

                    s32Ret = SampleAudioPlay(&stStream);

                    if (s32Ret) {
                        pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR;
                        break;
                    }
                }

                // TODO:
                MSSleep(30);
            } else {
                break;
            }
        } while (g_sample_audio_play_file_info.bPlayFileThreadRunning
                            && !g_sample_audio_play_file_info.bStopPlayFile);

        // loop + 1
        ++nLoop;

        // rewind file
        rewind(pFile);

        // Check AO Status
        do {
            AX_U32 card = g_sample_audio_play_param.stDevAttr.nCardNum;
            AX_U32 device = g_sample_audio_play_param.stDevAttr.nDeviceNum;
            AX_AO_DEV_STATE_T stStatus;
            memset(&stStatus, 0x00, sizeof(stStatus));

            s32Ret = AX_AO_QueryDevStat(card, device, &stStatus);

            if (s32Ret) {
                COMM_AUDIO_PRT("AX_AO_QueryDevStat[%d][%d] fail, ret:0x%x", card, device, s32Ret);
                break;
            }

            if (0 < stStatus.u32DevBusyNum) {
                MSSleep(5);
                continue;
            } else {
                break;
            }
        } while (1);
    } while (g_sample_audio_play_file_info.bPlayFileThreadRunning
                        && !g_sample_audio_play_file_info.bStopPlayFile
                        && (pstPlayFileAttr->nLoop < 0 || (nLoop < pstPlayFileAttr->nLoop))
                        && pstPlayFileAttr->eStatus != SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR);

    if (g_sample_audio_play_file_info.bStopPlayFile) {
        pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_STOP;
    } else if (nLoop >= pstPlayFileAttr->nLoop && pstPlayFileAttr->eStatus != SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR) {
        pstPlayFileAttr->eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_COMPLETE;
    }

EXIT:
    if (pFile) {
        fclose(pFile);
    }

    if (pBuffer) {
        free(pBuffer);
    }

    //COMM_AUDIO_PRT("---");
}

static AX_VOID* ProcessFileThread(AX_VOID *param) {
    AX_CHAR szName[50] = {0};
    sprintf(szName, "APP_AFPLAY");
    prctl(PR_SET_NAME, szName);

    SAMPLE_AUDIO_PLAY_FILE_ATTR_T stPlayFileAttr;

    g_sample_audio_play_file_info.bPlayFileThreadRunning = AX_TRUE;

    while (g_sample_audio_play_file_info.bPlayFileThreadRunning) {
        // wait for play
        pthread_mutex_lock(&g_sample_audio_play_file_info.mtxPlayFile);
        while (axfifo_size(&g_sample_audio_play_file_info.qPlayFile) == 0
                && g_sample_audio_play_file_info.bPlayFileThreadRunning) {
            pthread_cond_wait(&g_sample_audio_play_file_info.cvPlayFile, &g_sample_audio_play_file_info.mtxPlayFile);
        }
        pthread_mutex_unlock(&g_sample_audio_play_file_info.mtxPlayFile);

        if (axfifo_size(&g_sample_audio_play_file_info.qPlayFile) > 0) {
            memset(&stPlayFileAttr, 0x00, sizeof(stPlayFileAttr));
            axfifo_pop(&g_sample_audio_play_file_info.qPlayFile, &stPlayFileAttr, sizeof(stPlayFileAttr));

            g_sample_audio_play_file_info.bPlayingFile = AX_TRUE;
            g_sample_audio_play_file_info.bStopPlayFile = AX_FALSE;

            ProcessPlayFile(&stPlayFileAttr);

            g_sample_audio_play_file_info.ePlayFileStatus = stPlayFileAttr.eStatus;

            // sync
            if (!stPlayFileAttr.callback) {
                pthread_mutex_lock(&g_sample_audio_play_file_info.mtxWaitPlayFile);
                pthread_cond_broadcast(&g_sample_audio_play_file_info.cvWaitPlayFile);
                pthread_mutex_unlock(&g_sample_audio_play_file_info.mtxWaitPlayFile);
            } else {
                NOTIFY_AFPLAY_RESULT(&stPlayFileAttr);
            }

            if ((axfifo_size(&g_sample_audio_play_file_info.qPlayFile) == 0 || g_sample_audio_play_file_info.bStopPlayFile)
                    && !g_sample_audio_play_param.bKeepAlive) {
                SamplePlayDeinit();
            }

            g_sample_audio_play_file_info.bPlayingFile = AX_FALSE;

            if (g_sample_audio_play_file_info.bStopPlayFile) {
                pthread_mutex_lock(&g_sample_audio_play_file_info.mtxWaitStopPlayFile);
                pthread_cond_broadcast(&g_sample_audio_play_file_info.cvWaitStopPlayFile);
                pthread_mutex_unlock(&g_sample_audio_play_file_info.mtxWaitStopPlayFile);
            }
        }
    }

    return (AX_VOID *)0;
}

static AX_S32 SampleInitAudioPlayFileInfo() {
    // play
    g_sample_audio_play_file_info.bPlayingFile = AX_FALSE;
    g_sample_audio_play_file_info.bStopPlayFile = AX_FALSE;
    g_sample_audio_play_file_info.bPlayFileThreadRunning = AX_FALSE;
    g_sample_audio_play_file_info.ePlayFileStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_BUTT;
    axfifo_create(&g_sample_audio_play_file_info.qPlayFile, SAMPLE_AUDIO_PLAY_FILE_DEPTH * sizeof(SAMPLE_AUDIO_PLAY_FILE_ATTR_T));
    pthread_mutex_init(&g_sample_audio_play_file_info.mtxPlayFile, NULL);
    pthread_cond_init(&g_sample_audio_play_file_info.cvPlayFile, NULL);

    pthread_create(&g_sample_audio_play_file_info.hPlayFileThread, NULL, ProcessFileThread, AX_NULL);

    // wait play
    pthread_mutex_init(&g_sample_audio_play_file_info.mtxWaitPlayFile, NULL);
    pthread_cond_init(&g_sample_audio_play_file_info.cvWaitPlayFile, NULL);

    // wait stop
    pthread_mutex_init(&g_sample_audio_play_file_info.mtxWaitStopPlayFile, NULL);
    pthread_cond_init(&g_sample_audio_play_file_info.cvWaitStopPlayFile, NULL);

    // result callback
    g_sample_audio_play_file_info.bResultThreadRunning = AX_FALSE;
    axfifo_create(&g_sample_audio_play_file_info.qResult, SAMPLE_AUDIO_PLAY_FILE_DEPTH * sizeof(SAMPLE_AUDIO_PLAY_FILE_ATTR_T));
    pthread_mutex_init(&g_sample_audio_play_file_info.mtxResult, NULL);
    pthread_cond_init(&g_sample_audio_play_file_info.cvResult, NULL);
    pthread_create(&g_sample_audio_play_file_info.hResultThread, NULL, ResultCallbackThread, AX_NULL);

    g_sample_audio_play_file_info.bInited = AX_TRUE;

    return 0;
}

static AX_S32 SampleAudioPlayFileStop() {
    if (g_sample_audio_play_file_info.bPlayingFile) {
        g_sample_audio_play_file_info.bStopPlayFile = AX_TRUE;

        // wait stop complete
        pthread_mutex_lock(&g_sample_audio_play_file_info.mtxWaitStopPlayFile);
        while (g_sample_audio_play_file_info.bPlayingFile) {
            pthread_cond_wait(&g_sample_audio_play_file_info.cvWaitStopPlayFile, &g_sample_audio_play_file_info.mtxWaitStopPlayFile);
        }
        pthread_mutex_unlock(&g_sample_audio_play_file_info.mtxWaitStopPlayFile);
    }

    return 0;
}

static AX_S32 SampleAudioPlayStop() {
    /* Step1: stop play */
    SampleAudioPlayFileStop();

    /* Step2: stop play thread */
    if (g_sample_audio_play_file_info.bPlayFileThreadRunning) {
        g_sample_audio_play_file_info.bPlayFileThreadRunning = AX_FALSE;
        pthread_cond_broadcast(&g_sample_audio_play_file_info.cvPlayFile);

        pthread_join(g_sample_audio_play_file_info.hPlayFileThread, NULL);
    }

    /* Step3: stop result */
    if (g_sample_audio_play_file_info.bResultThreadRunning) {
        g_sample_audio_play_file_info.bResultThreadRunning = AX_FALSE;
        pthread_cond_broadcast(&g_sample_audio_play_file_info.cvResult);

        pthread_join(g_sample_audio_play_file_info.hResultThread, NULL);
    }

    if (g_sample_audio_play_file_info.bInited) {
        // play
        pthread_mutex_destroy(&g_sample_audio_play_file_info.mtxPlayFile);
        pthread_cond_destroy(&g_sample_audio_play_file_info.cvPlayFile);

        // wait play
        pthread_mutex_destroy(&g_sample_audio_play_file_info.mtxWaitPlayFile);
        pthread_cond_destroy(&g_sample_audio_play_file_info.cvWaitPlayFile);

        // wait stop
        pthread_mutex_destroy(&g_sample_audio_play_file_info.mtxWaitStopPlayFile);
        pthread_cond_destroy(&g_sample_audio_play_file_info.cvWaitStopPlayFile);

        // result callback
        pthread_mutex_destroy(&g_sample_audio_play_file_info.mtxResult);
        pthread_cond_destroy(&g_sample_audio_play_file_info.cvResult);

        g_sample_audio_play_file_info.bInited = AX_FALSE;
    }

    return 0;
}

static AX_S32 SampleAudioPlayRebuild(AX_PAYLOAD_TYPE_E eType) {
    AX_S32 s32Ret = -1;

    // deinit
    if (g_sample_audio_play_param.bInited) {
        s32Ret = SamplePlayDeinit();

        if (s32Ret) {
            goto EXIT;
        }
    }

    // init
    {
        g_sample_audio_play_param.stAdecChnAttr.eDecType = eType;

        SAMPLE_AO_DEV_ATTR_T *pstDevAttr = &g_sample_audio_play_param.stDevAttr;
        SAMPLE_DnVQE_ATTR_T stDnVqeAttr = {
                .pstNsCfg = &g_sample_audio_play_param.stNsCfg,
                .pstAgcCfg = &g_sample_audio_play_param.stAgcCfg,
            };
        SAMPLE_ADEC_CHN_ATTR_T *pstAdecChnAttr = &g_sample_audio_play_param.stAdecChnAttr;

        SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T tEntryParam = {
                .pstDevAttr = pstDevAttr,
                .pstDnVqeAttr = &stDnVqeAttr,
                .pstAdecChnAttr = pstAdecChnAttr,
            };

        s32Ret = SamplePlayInit(&tEntryParam);
    }

    if (s32Ret) {
        goto EXIT;
    }

EXIT:
    return s32Ret;
}

AX_S32 COMMON_AUDIO_Play(AX_PAYLOAD_TYPE_E eType, const AX_U8 *pData, AX_U32 nDataSize) {
    AX_S32 s32Ret = -1;

    if (!g_sample_audio_play_param.bEnable) {
        COMM_AUDIO_PRT("play dev disabled");
        return s32Ret;
    }

    // check type
    if (!IsAudioSupport(eType)) {
        COMM_AUDIO_PRT("unsupport audio type(%d)", eType);
        return s32Ret;
    }

    // check type
    if (!g_sample_audio_play_param.bInited
        || eType != g_sample_audio_play_param.stAdecChnAttr.eDecType) {
        s32Ret = SampleAudioPlayRebuild(eType);

        if (s32Ret) {
            COMM_AUDIO_PRT("SampleAudioPlayRebuild fail, ret:0x%x", s32Ret);
            return s32Ret;
        }
    }

    // play
    {
        AX_AUDIO_STREAM_T stStream;
        memset(&stStream, 0x00, sizeof(stStream));

        stStream.pStream = (AX_U8 *)pData;
        stStream.u64PhyAddr = 0;
        stStream.u32Len = nDataSize;
        stStream.bEof = AX_FALSE;

        s32Ret = SampleAudioPlay(&stStream);
    }

    return s32Ret;
}

AX_S32 COMMON_AUDIO_StopPlay(AX_VOID) {
    if (!g_sample_audio_play_param.bEnable) {
        return 0;
    }

    if (!g_sample_audio_play_param.bKeepAlive) {
        SamplePlayDeinit();
    }

    return 0;
}

AX_S32 COMMON_AUDIO_PlayFile(AX_PAYLOAD_TYPE_E eType,
                                    const AX_CHAR *pstrFileName,
                                    AX_S32 nLoop,
                                    SAMPLE_AUDIO_PLAYFILERESULT_CALLBACK callback,
                                    AX_VOID *pUserData) {
    AX_S32 s32Ret = -1;

    if (!g_sample_audio_play_param.bEnable) {
        COMM_AUDIO_PRT("play dev disabled");
        return s32Ret;
    }

    // check filename
    if (!pstrFileName) {
        COMM_AUDIO_PRT("file name empty");
        return s32Ret;
    }

    // check file exist
    {
        struct stat st;
        if (stat(pstrFileName, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                COMM_AUDIO_PRT("%s is not a valid file", pstrFileName);
                return -1;
            }
        } else {
            COMM_AUDIO_PRT("%s not exist", pstrFileName);
            return -1;
        }
    }

    // check type
    if (!IsAudioSupport(eType)) {
        COMM_AUDIO_PRT("unsupport audio type(%d)", eType);
        return s32Ret;
    }

    // check type
    if (!g_sample_audio_play_param.bInited
        || eType != g_sample_audio_play_param.stAdecChnAttr.eDecType) {
        s32Ret = SampleAudioPlayRebuild(eType);

        if (s32Ret) {
            COMM_AUDIO_PRT("SampleAudioPlayRebuild fail, ret:0x%x", s32Ret);
            return s32Ret;
        }
    }

    if (!g_sample_audio_play_file_info.bInited) {
        SampleInitAudioPlayFileInfo();
    }

    {
        SAMPLE_AUDIO_PLAY_FILE_ATTR_T tPlayFileAttr;
        memset(&tPlayFileAttr, 0x00, sizeof(SAMPLE_AUDIO_PLAY_FILE_ATTR_T));

        tPlayFileAttr.nLoop = (0 == nLoop) ? 1 : nLoop;
        tPlayFileAttr.eType = eType;
        strncpy(tPlayFileAttr.strFileName, pstrFileName, FILE_NAME_LEN - 1);
        tPlayFileAttr.pUserData = pUserData;
        tPlayFileAttr.callback = callback;
        tPlayFileAttr.eStatus = SAMPLE_AUDIO_PLAY_FILE_STATUS_BUTT;

        NOTIFY_AFPLAY_FILE(&tPlayFileAttr);

        // sync
        if (!callback) {
            pthread_mutex_lock(&g_sample_audio_play_file_info.mtxWaitPlayFile);
            pthread_cond_wait(&g_sample_audio_play_file_info.cvWaitPlayFile, &g_sample_audio_play_file_info.mtxWaitPlayFile);
            pthread_mutex_unlock(&g_sample_audio_play_file_info.mtxWaitPlayFile);

            if (SAMPLE_AUDIO_PLAY_FILE_STATUS_ERROR != g_sample_audio_play_file_info.ePlayFileStatus) {
                s32Ret = 0;
            }
        } else {
            s32Ret = 0;
        }
    }

    return s32Ret;
}

AX_S32 COMMON_AUDIO_StopPlayFile(AX_VOID) {
    if (!g_sample_audio_play_param.bEnable) {
        return 0;
    }

    return SampleAudioPlayFileStop();
}

AX_S32 COMMON_AUDIO_GetCaptureAttr(AX_BOOL *pbEnable) {
    AX_S32 s32Ret = -1;

    if (!g_sample_audio_capture_param.bEnable || !pbEnable) {
        return s32Ret;
    }

    *pbEnable = g_sample_audio_capture_param.bInited;

    return 0;
}

AX_S32 COMMON_AUDIO_SetCaptureAttr(AX_BOOL bEnable) {
    AX_S32 s32Ret = -1;

    if (!g_sample_audio_capture_param.bEnable) {
        return s32Ret;
    }

    if (g_sample_audio_capture_param.bInited != bEnable) {
        if (bEnable) {
            SAMPLE_AI_DEV_ATTR_T *pstDevAttr = &g_sample_audio_capture_param.stDevAttr;
            SAMPLE_AP_UPTALKVQE_ATTR_T stUpVqeAttr = {
                    .pstAecCfg = &g_sample_audio_capture_param.stAecCfg,
                    .pstNsCfg = &g_sample_audio_capture_param.stNsCfg,
                    .pstAgcCfg = &g_sample_audio_capture_param.stAgcCfg,
                    .pstVadCfg = &g_sample_audio_capture_param.stVadCfg
                };
            SAMPLE_ACODEC_ATTR_T stAcodecAttr = {
                    .pstEqCfg = &g_sample_audio_capture_param.stEqCfg,
                    .pstHpfCfg = &g_sample_audio_capture_param.stHpfCfg,
                    .pstLpfCfg = &g_sample_audio_capture_param.stLpfCfg
                };
            SAMPLE_AENC_CHN_ATTR_T *pstAencChnAttr = &g_sample_audio_capture_param.stAencChnAttr;

            SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T tEntryParam = {
                    .pstDevAttr = pstDevAttr,
                    .pstUpVqeAttr = &stUpVqeAttr,
                    .pstACodecAttr = &stAcodecAttr,
                    .pstAencChnAttr = pstAencChnAttr,
                };

            s32Ret = SampleCaptureInit(&tEntryParam);
        } else {
            s32Ret = SampleCaptureDeinit();
        }
    } else {
        s32Ret = 0;
    }

    return s32Ret;
}

AX_S32 COMMON_AUDIO_SetCaptureVolume(AX_F32 fVqeVolume) {
    AX_S32 s32Ret = -1;

    /*set AI VQE volume */
    AX_U32 card = g_sample_audio_capture_param.stDevAttr.nCardNum;
    AX_U32 device = g_sample_audio_capture_param.stDevAttr.nDeviceNum;

    s32Ret = AX_AI_SetVqeVolume(card, device, (AX_F64)fVqeVolume);

    return s32Ret;
}

AX_S32 COMMON_AUDIO_SetPlayVolume(AX_F32 fVqeVolume) {
    AX_S32 s32Ret = -1;

    /*set AO VQE volume */
    AX_U32 card = g_sample_audio_play_param.stDevAttr.nCardNum;
    AX_U32 device = g_sample_audio_play_param.stDevAttr.nDeviceNum;

    if (0 == fVqeVolume) {
        AX_AUDIO_FADE_T stFade;
        memset(&stFade, 0x00, sizeof(stFade));
        stFade.bFade = AX_FALSE;
        stFade.enFadeInRate = AX_AUDIO_FADE_RATE_128;
        stFade.enFadeOutRate = AX_AUDIO_FADE_RATE_128;

        s32Ret = AX_AO_SetVqeVolume(card, device, (AX_F64)fVqeVolume);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_AO_SetVqeVolume failed, ret:0x%x", s32Ret);
            return s32Ret;
        }

        s32Ret = AX_AO_SetVqeMute(card, device, AX_TRUE, &stFade);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_AO_SetVqeMute failed, ret:0x%x", s32Ret);
        }
    } else {
        AX_BOOL bEnable = AX_FALSE;
        AX_AUDIO_FADE_T stFade;

        s32Ret = AX_AO_GetVqeMute(card, device, &bEnable, &stFade);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_AO_GetVqeMute failed, ret:0x%x", s32Ret);
            return s32Ret;
        }

        if (bEnable) {
            s32Ret = AX_AO_SetVqeMute(card, device, AX_FALSE, &stFade);

            if (s32Ret) {
                COMM_AUDIO_PRT("AX_AO_SetVqeMute failed, ret:0x%x", s32Ret);
                return s32Ret;
            }
        }

        s32Ret = AX_AO_SetVqeVolume(card, device, (AX_F64)fVqeVolume);

        if (s32Ret) {
            COMM_AUDIO_PRT("AX_AO_SetVqeVolume failed, ret:0x%x", s32Ret);
        }
    }

    return s32Ret;
}

#endif
