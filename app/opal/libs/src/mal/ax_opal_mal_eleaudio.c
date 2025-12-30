/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_eleaudio.h"
#include "ax_opal_hal_audio_cap.h"
#include "ax_opal_hal_audio_play.h"

#include "ax_opal_log.h"
#include "ax_opal_utils.h"

#define LOG_TAG ("ELE_AUDIO")

// interface vtable
AX_OPAL_MAL_ELE_Interface audio_interface = {
    .start = AX_OPAL_MAL_ELEAUDIO_Start,
    .stop = AX_OPAL_MAL_ELEAUDIO_Stop,
    .event_proc = AX_OPAL_MAL_ELEAUDIO_Process,
};

static AX_VOID AencGetStreamProc(AX_VOID *arg) {
    LOG_M_N(LOG_TAG, "+++");

    AX_OPAL_THREAD_T *pThread = (AX_OPAL_THREAD_T*)arg;
    if (!pThread) {
        goto EXIT;
    }

    AX_OPAL_MAL_ELEAUDIO_T* pEle = (AX_OPAL_MAL_ELEAUDIO_T*)pThread->arg;
    if (pEle == NULL) {
        return;
    }

    AX_S32 s32Ret = 0;
    AX_U32 nChannel = 0;

    AX_CHAR thread_name[16];
    sprintf(thread_name, "aenc_%d", nChannel);
    prctl(PR_SET_NAME, thread_name);

    AX_AUDIO_STREAM_T tStream;
    memset(&tStream, 0x0, sizeof(AX_AUDIO_STREAM_T));
	while (pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING) {
        s32Ret = AX_AENC_GetStream(nChannel, &tStream, AX_ERR_OPAL_GENERIC);
        if (AX_SUCCESS != s32Ret) {
            if (AX_ERR_AENC_UNEXIST == s32Ret) {
                break;
            }
            LOG_M_E(LOG_TAG, "AX_AENC_GetStream failed, ret 0x%X", s32Ret);
            break;
        }

        if (0 == tStream.u32Len || !tStream.pStream) {
            AX_AENC_ReleaseStream(nChannel, &tStream);
            continue;
        }

        if (pEle->stAudioPktCb.callback) {
            AX_OPAL_AUDIO_PKT_T tCapPkt;
            memset(&tCapPkt, 0x0, sizeof(tCapPkt));
            tCapPkt.nBitRate = pEle->stEncoderAttr.nBitRate;
            tCapPkt.eType = pEle->stEncoderAttr.eType;
            tCapPkt.eBitWidth = pEle->stEncoderAttr.eBitWidth;
            tCapPkt.eSampleRate = pEle->stEncoderAttr.eSampleRate;
            tCapPkt.eSoundMode = pEle->stEncoderAttr.eSoundMode; // PT_AAC == tCapPkt.eType ? AX_AUDIO_SOUND_MODE_STEREO : AX_AUDIO_SOUND_MODE_MONO;
            tCapPkt.pData = tStream.pStream;
            tCapPkt.nDataSize = tStream.u32Len;
            tCapPkt.u64Pts = tStream.u64TimeStamp;
            tCapPkt.pUserData = pEle->stAudioPktCb.pUserData;
            tCapPkt.pPrivateData = NULL;
            pEle->stAudioPktCb.callback(nChannel, &tCapPkt);
        }
        AX_AENC_ReleaseStream(nChannel, &tStream);
    }

EXIT:
    LOG_M_N(LOG_TAG, "---");
}

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELEAUDIO_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr) {

    AX_OPAL_MAL_ELE_HANDLE handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_ELEAUDIO_T));
    if (handle == AX_NULL) {
        LOG_M_E(LOG_TAG, "malloc ele audio failed.");
        return AX_NULL;
    }

    AX_OPAL_MAL_ELEAUDIO_T* pEle = (AX_OPAL_MAL_ELEAUDIO_T*)handle;
    memset(pEle, 0x0, sizeof(AX_OPAL_MAL_ELEAUDIO_T));
    pEle->stBase.nId = pEleAttr->nId;
    pEle->stBase.pParent = parent;
    pEle->stBase.vTable = &audio_interface;
    memcpy(&pEle->stBase.stAttr, pEleAttr, sizeof(AX_OPAL_ELEMENT_ATTR_T));

    AX_OPAL_MAL_SUBPPL_T *pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pEle->stBase.pParent;
    AX_OPAL_ATTR_T *pOpalAttr = &((AX_OPAL_MAL_PPL_T*)pSubPipeline->pParent)->stOpalAttr;
    memcpy(&pEle->stAttr, &(pOpalAttr->stAudioAttr), sizeof(AX_OPAL_AUDIO_ATTR_T));

    return handle;
}

AX_S32 AX_OPAL_MAL_ELEAUDIO_Destroy(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_FREE(self);
    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEAUDIO_Start(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }
    AX_OPAL_MAL_ELEAUDIO_T* pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;

    /* cap */
    nRet = AX_OPAL_HAL_AUDIO_CAP_Start(&pEle->stAttr, &pEle->stEncoderAttr);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    if (pEle->stAttr.stCapAttr.bEnable) {
        if (pEle->pThreadGetAencStream == AX_NULL) {
            pEle->pThreadGetAencStream = AX_OPAL_CreateThread(AencGetStreamProc, pEle);
            AX_OPAL_StartThread(pEle->pThreadGetAencStream);
        }
    }

    /* play */
    nRet = AX_OPAL_HAL_AUDIO_PLAY_Start(&pEle->stAttr);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    pEle->stBase.bStart = AX_TRUE;

    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEAUDIO_Stop(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELEAUDIO_T* pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;

    /* cap */
    if (pEle->stAttr.stCapAttr.bEnable) {
        if (pEle->pThreadGetAencStream != AX_NULL) {
            AX_OPAL_StopThread(pEle->pThreadGetAencStream);
            AX_OPAL_DestroyThread(pEle->pThreadGetAencStream);
            pEle->pThreadGetAencStream = AX_NULL;
        }
    }

    nRet = AX_OPAL_HAL_AUDIO_CAP_Stop(&pEle->stAttr, &pEle->stEncoderAttr);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* play */
    nRet = AX_OPAL_HAL_AUDIO_PLAY_Stop(&pEle->stAttr);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    pEle->stBase.bStart = AX_FALSE;
    return nRet;
}

AX_S32 get_audioattr(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_AUDIO_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    memcpy(pPorcessData->pData, &pEle->stAttr, sizeof(AX_OPAL_AUDIO_ATTR_T));

    return nRet;
}

AX_S32 set_audioattr(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_AUDIO_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_AUDIO_ATTR_T *pAudioData = (AX_OPAL_AUDIO_ATTR_T*)pPorcessData->pData;

    if (pEle->stAttr.stCapAttr.bEnable) {
        if (pEle->pThreadGetAencStream != AX_NULL) {
            AX_OPAL_StopThread(pEle->pThreadGetAencStream);
            AX_OPAL_DestroyThread(pEle->pThreadGetAencStream);
            pEle->pThreadGetAencStream = AX_NULL;
        }
    }

    /* cap */
    nRet = AX_OPAL_HAL_AUDIO_CAP_Stop(pAudioData, &pEle->stEncoderAttr);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* play */
    nRet = AX_OPAL_HAL_AUDIO_PLAY_Stop(pAudioData);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* cap */
    nRet = AX_OPAL_HAL_AUDIO_CAP_Start(pAudioData, &pEle->stEncoderAttr);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    if (pEle->stAttr.stCapAttr.bEnable) {
        if (pEle->pThreadGetAencStream == AX_NULL) {
            pEle->pThreadGetAencStream = AX_OPAL_CreateThread(AencGetStreamProc, pEle);
            AX_OPAL_StartThread(pEle->pThreadGetAencStream);
        }
    }

    /* play */
    nRet = AX_OPAL_HAL_AUDIO_PLAY_Start(pAudioData);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    memcpy(&pEle->stAttr, pAudioData, sizeof(AX_OPAL_AUDIO_ATTR_T));
    pEle->stBase.bStart = AX_TRUE;

    return nRet;
}

static AX_S32 play(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {

    AX_S32 nRet = AX_SUCCESS;

    AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_AUDIO_PLAY_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_AUDIO_PLAY_T *pStream = (AX_OPAL_AUDIO_PLAY_T*)pPorcessData->pData;

    nRet = AX_OPAL_HAL_AUDIO_Play(&pEle->stAttr, pStream->eType, pStream->pData, pStream->nDataSize);

    return nRet;
}

static AX_S32 playfile(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* verify data size */
    if (pPorcessData->nDataSize != sizeof(AX_OPAL_AUDIO_PLAYFILE_T)) {
        return AX_ERR_OPAL_GENERIC;
    }
    AX_OPAL_AUDIO_PLAYFILE_T *pPlayFileData = (AX_OPAL_AUDIO_PLAYFILE_T*)pPorcessData->pData;

    nRet = AX_OPAL_HAL_AUDIO_PlayFile(&pEle->stAttr, pPlayFileData, &pEle->stAudioFilePlay);
    return nRet;
}


static AX_S32 stop_playfile(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_S32 AX_OPAL_HAL_AUDIO_PlayFile(AX_OPAL_AUDIO_ATTR_T* pstAttr, AX_OPAL_AUDIO_PLAYFILE_T* pPlayFile, AX_OPAL_MAL_AUDIO_PLAY_T *pMalPlayFile);

    nRet = AX_OPAL_HAL_AUDIO_StopPlayFile(&pEle->stAttr, &pEle->stAudioFilePlay);

    return nRet;
}

static AX_S32 set_playpipe(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    if (pPorcessData->nDataSize != sizeof(AX_OPAL_AUDIO_PLAY_PIPE_ATTR_T)) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_OPAL_AUDIO_ATTR_T stAudioData;
    memcpy(&stAudioData, &pEle->stAttr, sizeof(AX_OPAL_AUDIO_ATTR_T));
    memcpy(&stAudioData.stPlayAttr.stPipeAttr, pPorcessData->pData, sizeof(AX_OPAL_AUDIO_PLAY_PIPE_ATTR_T));

    nRet = AX_OPAL_HAL_AUDIO_PLAY_Stop(&stAudioData);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    nRet = AX_OPAL_HAL_AUDIO_PLAY_Start(&stAudioData);
    if (0 != nRet) {
        return AX_ERR_OPAL_GENERIC;
    }

    memcpy(&pEle->stAttr.stPlayAttr.stPipeAttr, pPorcessData->pData, sizeof(AX_OPAL_AUDIO_PLAY_PIPE_ATTR_T));
    return nRet;
}

static AX_S32 get_vol(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData, AX_S32 nPlayCap) {
    AX_S32 nRet = AX_SUCCESS;

    AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    if (0 == nPlayCap) {
        nRet = AX_OPAL_HAL_AUDIO_CAP_GetVolume(&pEle->stAttr);
        if (nRet != AX_SUCCESS) {
            return AX_ERR_OPAL_GENERIC;
        }
        *((AX_OPAL_AUDIO_GETVOLUME_T*)pPorcessData->pData)->pfVol = pEle->stAttr.stCapAttr.stDevAttr.fVolume;
    }
    else {
        nRet = AX_OPAL_HAL_AUDIO_PLAY_GetVolume(&pEle->stAttr);
        if (nRet != AX_SUCCESS) {
            return AX_ERR_OPAL_GENERIC;
        }
        *((AX_OPAL_AUDIO_GETVOLUME_T*)pPorcessData->pData)->pfVol = pEle->stAttr.stPlayAttr.stDevAttr.fVolume;
    }
    return nRet;
}

static AX_S32 set_vol(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData, AX_S32 nPlayCap) {
    AX_S32 nRet = AX_SUCCESS;

    AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
    if (!pEle->stBase.bStart) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_OPAL_AUDIO_ATTR_T stAudioData;
    memcpy(&stAudioData, &pEle->stAttr, sizeof(AX_OPAL_AUDIO_ATTR_T));
    if (0 == nPlayCap) {
        stAudioData.stCapAttr.stDevAttr.fVolume = ((AX_OPAL_AUDIO_SETVOLUME_T*)pPorcessData->pData)->fVol;
        nRet = AX_OPAL_HAL_AUDIO_CAP_SetVolume(&stAudioData);
        if (nRet != AX_SUCCESS) {
            return AX_ERR_OPAL_GENERIC;
        }
        pEle->stAttr.stCapAttr.stDevAttr.fVolume = ((AX_OPAL_AUDIO_SETVOLUME_T*)pPorcessData->pData)->fVol;
    }
    else {
        stAudioData.stPlayAttr.stDevAttr.fVolume = ((AX_OPAL_AUDIO_SETVOLUME_T*)pPorcessData->pData)->fVol;
        nRet = AX_OPAL_HAL_AUDIO_PLAY_SetVolume(&stAudioData);
        if (nRet != AX_SUCCESS) {
            return AX_ERR_OPAL_GENERIC;
        }
        pEle->stAttr.stPlayAttr.stDevAttr.fVolume = ((AX_OPAL_AUDIO_SETVOLUME_T*)pPorcessData->pData)->fVol;
    }

    return nRet;
}

AX_S32 AX_OPAL_MAL_ELEAUDIO_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }
    AX_OPAL_MAIN_CMD_E eMainCmdType = pPorcessData->eMainCmdType;

    switch (eMainCmdType) {
        case AX_OPAL_MAINCMD_AUDIO_GETATTR:
            nRet = get_audioattr(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_AUDIO_SETATTR:
            nRet = set_audioattr(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_AUDIO_PLAY:
            nRet = play(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_AUDIO_STOPPLAY:
            nRet = stop_playfile(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_AUDIO_PLAYFILE:
            nRet = playfile(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_AUDIO_GETCAPVOLUME:
            nRet = get_vol(self, pPorcessData, 0);
            break;
        case AX_OPAL_MAINCMD_AUDIO_SETCAPVOLUME:
            nRet = set_vol(self, pPorcessData, 0);
            break;
        case AX_OPAL_MAINCMD_AUDIO_GETPLAYVOLUME:
            nRet = get_vol(self, pPorcessData, 1);
            break;
        case AX_OPAL_MAINCMD_AUDIO_SETPLAYVOLUME:
            nRet = set_vol(self, pPorcessData, 1);
            break;
        case AX_OPAL_MAINCMD_AUDIO_GETENCODERATTR:
            {
                AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
                if (!pEle->stBase.bStart) {
                    return AX_ERR_OPAL_GENERIC;
                }
                memcpy(pPorcessData->pData, &pEle->stEncoderAttr, sizeof(AX_OPAL_AUDIO_ENCODER_ATTR_T));
            }
            break;
        case AX_OPAL_MAINCMD_AUDIO_GETPLAYPIPEATTR:
            {
                AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
                if (!pEle->stBase.bStart) {
                    return AX_ERR_OPAL_GENERIC;
                }
                memcpy(pPorcessData->pData, &pEle->stAttr.stPlayAttr.stPipeAttr, sizeof(AX_OPAL_AUDIO_PLAY_PIPE_ATTR_T));
            }
            break;
        case AX_OPAL_MAINCMD_AUDIO_SETPLAYPIPEATTR:
            nRet = set_playpipe(self, pPorcessData);
            break;
        case AX_OPAL_MAINCMD_AUDIO_REGISTERPACKETCALLBACK:
            {
                AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
                memcpy(&pEle->stAudioPktCb, pPorcessData->pData, sizeof(AX_OPAL_AUDIO_PKT_CALLBACK_T));
            }
            break;
        case AX_OPAL_MAINCMD_AUDIO_UNREGISTERPACKETCALLBACK:
            {
                AX_OPAL_MAL_ELEAUDIO_T *pEle = (AX_OPAL_MAL_ELEAUDIO_T*)self;
                memset(&pEle->stAudioPktCb, 0x0, sizeof(AX_OPAL_AUDIO_PKT_CALLBACK_T));
            }
            break;
        default:
            LOG_M_E(LOG_TAG, "not support type %d", eMainCmdType);
            break;
    }
    return nRet;
}