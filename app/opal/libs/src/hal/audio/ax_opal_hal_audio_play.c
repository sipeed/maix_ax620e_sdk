/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_audio_play.h"

#include <string.h>
#include "ax_opal_utils.h"

#include "ax_opal_log.h"
#define LOG_TAG  ("HAL_AO")

static AX_POOL g_ax_opal_audio_play_poolid = AX_INVALID_POOLID;

#define AX_OPAL_AO_DEFAULT_DEPTH                (30)
#define AX_OPAL_AO_STEREO_AAC_PERIOD_SIZE       (1024)
#define AX_OPAL_AO_PERIOD_COUNT                 (8)
#define AX_OPAL_AO_DEFAULT_TINYALSA_CHNCNT      (2)
#define AX_OPAL_ADEC_DEFAULT_IN_DEPTH           (8)
#define AX_OPAL_ADEC_DEFAULT_BLK_SIZE           (4 * 1024)
#define AX_OPAL_ADEC_STEREO_AAC_BLK_SIZE        (8 * 1024)
#define AX_OPAL_ADEC_DEFAULT_BLK_CNT            (34)

#define AX_OPAL_ADEC_CHN_ID_0                   (0)  // only one adec chn
#define AX_OPAL_AO_FILE_MAX                     (5)

#define AUDIO_AAC_HEADER_SIZE                   (7)
#define AUDIO_AAC_HEADER_WITH_CRC_SIZE          (AUDIO_AAC_HEADER_SIZE + 2)
#define AUDIO_ID_RIFF                           0x46464952
#define AUDIO_ID_WAVE                           0x45564157
#define AUDIO_ID_FMT                            0x20746d66
#define AUDIO_ID_DATA                           0x61746164

static AX_BOOL IsDnVqeEnabled(const AX_AP_DNVQE_ATTR_T *pstVqeAttr) {
    return (AX_BOOL)((pstVqeAttr->stNsCfg.bNsEnable != AX_FALSE) ||
                            (pstVqeAttr->stAgcCfg.bAgcEnable != AX_FALSE));
}

static AX_S32 StartAo(const AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_U32 card = pstAttr->stPlayAttr.stDevAttr.nCardId;
    AX_U32 device = pstAttr->stPlayAttr.stDevAttr.nDeviceId;
    AX_U32 nDecChn = AX_OPAL_ADEC_CHN_ID_0;

    /* Step1.1: link mode */
    {
        AX_MOD_INFO_T Adec_Mod = {AX_ID_ADEC, 0, nDecChn};
        AX_MOD_INFO_T Ao_Mod = {AX_ID_AO, card, device};

        nRet = AX_SYS_Link(&Adec_Mod, &Ao_Mod);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_SYS_Link failed, ret=0x%x", nRet);
            return -1;
        }
    }

    // /* Step1.2: AO init */
    // nRet = AX_AO_Init();
    // if (AX_SUCCESS != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_AO_Init failed, ret=0x%x", nRet);
    //     return nRet;
    // }

    /* Step1.3: set AO attribute */
    {
        AX_AO_ATTR_T stAttr;
        memset(&stAttr, 0x00, sizeof(stAttr));

        stAttr.enBitwidth = (AX_AUDIO_BIT_WIDTH_E)pstAttr->stDevCommAttr.eBitWidth; // TODO: convert
        stAttr.enSoundmode = (AX_AUDIO_SOUND_MODE_E)pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[nDecChn].eSoundMode; // TODO: convert
        stAttr.enLinkMode = AX_LINK_MODE;
        stAttr.enSamplerate = (AX_AUDIO_SAMPLE_RATE_E)pstAttr->stDevCommAttr.eSampleRate;
        stAttr.U32Depth = AX_OPAL_AO_DEFAULT_DEPTH;

        if (pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[nDecChn].eType == PT_AAC
            && stAttr.enSoundmode == AX_AUDIO_SOUND_MODE_STEREO) {
            stAttr.u32PeriodSize = AX_OPAL_AO_STEREO_AAC_PERIOD_SIZE;
        } else {
            stAttr.u32PeriodSize = pstAttr->stDevCommAttr.eSampleRate / 100;
        }
        stAttr.u32PeriodCount = AX_OPAL_AO_PERIOD_COUNT;
        stAttr.bInsertSilence = AX_FALSE;
        stAttr.u32ChnCnt = AX_OPAL_AO_DEFAULT_TINYALSA_CHNCNT;

        nRet = AX_AO_SetPubAttr(card, device,&stAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_AO_SetPubAttr failed, ret=0x%x", nRet);
            return nRet;
        }
    }

    /* Step1.4: set AO VQE */
    AX_AP_DNVQE_ATTR_T stVqeAttr;
    memset(&stVqeAttr, 0, sizeof(stVqeAttr));
    {
        if (pstAttr->stDevCommAttr.eSampleRate == AX_OPAL_AUDIO_SAMPLE_RATE_8000 ||
            pstAttr->stDevCommAttr.eSampleRate == AX_OPAL_AUDIO_SAMPLE_RATE_16000) {
            stVqeAttr.s32SampleRate = pstAttr->stDevCommAttr.eSampleRate;
            stVqeAttr.u32FrameSamples = pstAttr->stDevCommAttr.eSampleRate / 100;
        } else {
            stVqeAttr.s32SampleRate = (AX_S32)AX_AUDIO_SAMPLE_RATE_16000;
            stVqeAttr.u32FrameSamples = stVqeAttr.s32SampleRate / 100;
        }

        memcpy(&stVqeAttr.stNsCfg, &pstAttr->stPlayAttr.stDevAttr.stVqeAttr.stAnsAttr, sizeof(AX_NS_CONFIG_T));
        memcpy(&stVqeAttr.stAgcCfg, &pstAttr->stPlayAttr.stDevAttr.stVqeAttr.stAgcAttr, sizeof(AX_AGC_CONFIG_T));

        if (IsDnVqeEnabled(&stVqeAttr)) {
            nRet = AX_AO_SetDnVqeAttr(card, device, &stVqeAttr);
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AX_AO_SetDnVqeAttr failed, ret=0x%x", nRet);
                return nRet;
            }
        }
    }

    /* Step1.5: enable AO device */
    nRet = AX_AO_EnableDev(card, device);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AO_EnableDev failed, ret=0x%x", nRet);
        return nRet;
    }

    /* Step1.6: enable AO resample */
    if (pstAttr->stDevCommAttr.eSampleRate != stVqeAttr.s32SampleRate) {
        nRet = AX_AO_EnableResample(card, device, stVqeAttr.s32SampleRate);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_AO_EnableResample failed, ret=0x%x", nRet);
            return nRet;
        }
    }

    /* Step1.6: set AO VQE volume */
    nRet = AX_AO_SetVqeVolume(card, device, pstAttr->stPlayAttr.stDevAttr.fVolume);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AO_SetVqeVolume failed, ret=0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 StartAdec(const AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    /* Step2.0: play stop */
    // SampleAudioPlayStop();

    /* Step2.1: adec init */
    // nRet = AX_ADEC_Init();
    // if (AX_SUCCESS != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_ADEC_Init failed, ret=0x%x", nRet);
    //     return nRet;
    // }

    {
        AX_ADEC_AAC_DECODER_ATTR_T aacDecoderAttr = {
            .enTransType = AX_AAC_TRANS_TYPE_ADTS,
        };

        AX_ADEC_G726_DECODER_ATTR_T stG726DecoderAttr ={
            .u32BitRate = 32000
        };

        AX_ADEC_G723_DECODER_ATTR_T stG723DecoderAttr ={
            .u32BitRate = 24000
        };

        ADEC_CHN adChn = AX_OPAL_ADEC_CHN_ID_0;

        AX_ADEC_CHN_ATTR_T stAdecChnAttr;
        memset(&stAdecChnAttr, 0x0, sizeof(AX_ADEC_CHN_ATTR_T));
        stAdecChnAttr.enType = pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[adChn].eType;
        stAdecChnAttr.u32BufSize = AX_OPAL_ADEC_DEFAULT_IN_DEPTH;
        stAdecChnAttr.enLinkMode = AX_LINK_MODE;
        stAdecChnAttr.u32MaxFrameSize = 0;

        /* Step2.2: decoder init */
        switch (stAdecChnAttr.enType) {
        case PT_AAC:
            {
                stAdecChnAttr.pValue = &aacDecoderAttr;
#if defined(APP_FAAC_SUPPORT)
                nRet = AX_ADEC_FaacInit(); // faac not support aac decoder
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_ADEC_FaacInit failed, ret=0x%x", nRet);
                    return nRet;
                }
#elif defined(APP_FDK_SUPPORT)
                nRet = AX_ADEC_FdkInit();
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_ADEC_FdkInit failed, ret=0x%x", nRet);
                    return nRet;
                }
#endif
            }
            break;

        case PT_G726:
            stAdecChnAttr.pValue = &stG726DecoderAttr;
            break;

        case PT_G723:
            stAdecChnAttr.pValue = &stG723DecoderAttr;
            break;

        default:
            stAdecChnAttr.pValue = NULL;
            break;
        }

        /* Step2.3: create channel */
        nRet = AX_ADEC_CreateChn(adChn, &stAdecChnAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ADEC_CreateChn failed, ret=0x%x", nRet);
            return nRet;
        }

        /* Step2.4: attach AO pool */
        if (AX_INVALID_POOLID == g_ax_opal_audio_play_poolid) {

            AX_U32 nFrameSize = AX_OPAL_ADEC_DEFAULT_BLK_SIZE;
            AX_AUDIO_SOUND_MODE_E enSoundmode = (AX_AUDIO_SOUND_MODE_E)pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[adChn].eSoundMode;
            if (pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[adChn].eType == PT_AAC
                &&  enSoundmode == AX_AUDIO_SOUND_MODE_STEREO) {
                nFrameSize = AX_OPAL_ADEC_STEREO_AAC_BLK_SIZE;
            }
            g_ax_opal_audio_play_poolid = AX_OPAL_HAL_SYS_CreatePool(nFrameSize, AX_OPAL_ADEC_DEFAULT_BLK_CNT, "AUDIO_PLAY");
        }

        if (AX_INVALID_POOLID == g_ax_opal_audio_play_poolid) {
            LOG_M_E(LOG_TAG, "AX_OPAL_HAL_SYS_CreatePool failed");
            return -1;
        }
        nRet = AX_ADEC_AttachPool(adChn, g_ax_opal_audio_play_poolid);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ADEC_AttachPool failed, ret=0x%x", nRet);
            return nRet;
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 StopAo(const AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_U32 card = pstAttr->stPlayAttr.stDevAttr.nCardId;
    AX_U32 device = pstAttr->stPlayAttr.stDevAttr.nDeviceId;

    /* Step2.1: disable AO device */
    nRet = AX_AO_DisableDev(card, device);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AO_DisableDev failed, ret=0x%x", nRet);
        return nRet;
    }

    // /* Step2.1: AO deinit */
    // nRet = AX_AO_DeInit();
    // if (AX_SUCCESS != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_AO_DeInit failed, ret=0x%x", nRet);
    // }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 StopAdec(const AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    ADEC_CHN adChn = AX_OPAL_ADEC_CHN_ID_0;

    /* Step1.1: dencoder deinit */
    switch (pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[adChn].eType) {
    case PT_AAC:
        {
#if defined(APP_FAAC_SUPPORT)
            nRet = AX_ADEC_FaacDeInit();
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AX_ADEC_FaacDeInit failed, ret=0x%x", nRet);
                return nRet;
            }
#elif defined(APP_FDK_SUPPORT)
            nRet = AX_ADEC_FdkDeInit();
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AX_ADEC_FdkDeInit failed, ret=0x%x", nRet);
                return nRet;
            }
#endif
        }
        break;

    default:
        break;
    }

    /* Step1.2: detach pool */
    nRet = AX_ADEC_DetachPool(adChn);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_DetachPool failed, ret=0x%x", nRet);
        return nRet;
    }

    if (g_ax_opal_audio_play_poolid != AX_INVALID_POOLID) {
        AX_OPAL_HAL_SYS_DestroyPool(g_ax_opal_audio_play_poolid);
        g_ax_opal_audio_play_poolid = AX_INVALID_POOLID;
    }

    /* Step1.3: destroy chn */
    nRet = AX_ADEC_DestroyChn(adChn);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_DestroyChn failed, ret=0x%x", nRet);
        return nRet;
    }

    // /* Step1.4: adec deinit */
    // nRet = AX_ADEC_DeInit();
    // if (AX_SUCCESS != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_ADEC_DeInit failed, ret=0x%x", nRet);
    //     return nRet;
    // }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_BOOL IsAudioSupport(AX_PAYLOAD_TYPE_E eType) {
    return (AX_BOOL)((PT_G711A == eType) ||
                        (PT_G711U == eType) ||
                        (PT_LPCM == eType) ||
                        (PT_G726 == eType) ||
                        (PT_AAC == eType));

}

static AX_S32 PlayAudio(AX_AUDIO_STREAM_T *pstStream, AX_S32 nDecChn) {
    static AX_U32 u32SeqNum = 0;
    pstStream->u32Seq = ++u32SeqNum;
    return AX_ADEC_SendStream(nDecChn, pstStream, AX_TRUE);
}


static AX_S32 PlayFileProcess(AX_OPAL_AUDIO_ATTR_T* pstAttr, AX_OPAL_MAL_AUDIO_PLAY_T* pMalPlay, AX_OPAL_AUDIO_PLAYFILE_T* pAudioFile) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    FILE *pFile = NULL;
    AX_S32 nLoop = 0;
    AX_U8 *pBuffer = NULL;
    AX_S32 nReadSize = 960;
    AX_OPAL_AUDIO_PLAY_FILE_STATUS_E eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_BUTT;

    pFile = fopen(pAudioFile->pstrFileName, "rb");
    if (!pFile) {
        LOG_M_E(LOG_TAG, "open file (%s) failed", pAudioFile->pstrFileName);
        eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
        goto EXIT;
    }

    switch (pAudioFile->eType) {
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
        LOG_M_E(LOG_TAG, "alloc memory[%d] failed", nReadSize);
        eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
        goto EXIT;
    }

    do {
        AX_S32 nTotalSize = 0;

        do {
            AX_S32 nNumRead = 0;
            AX_S32 nBufferSize = 0;

            if (PT_AAC == pAudioFile->eType) {
                AX_U8 *pPacket = (AX_U8 *)pBuffer;
                AX_S32 nPacketSize = 0;

                {
                    nNumRead = fread(pPacket, 1, AUDIO_AAC_HEADER_SIZE, pFile);
                    if (nNumRead != AUDIO_AAC_HEADER_SIZE) {
                        break;
                    }

                    if (pPacket[0] != 0xff || (pPacket[1] & 0xf0) != 0xf0) {
                        eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
                        LOG_M_E(LOG_TAG, "Not an ADTS packet");
                        break;
                    }

                    /* Set to 1 if there is no CRC and 0 if there is CRC */
                    AX_BOOL bNoCRC = (1 == (pPacket[1] & 0x1)) ? AX_TRUE : AX_FALSE;

                    nPacketSize = ((pPacket[3] & 0x03) << 11) | (pPacket[4] << 3) | (pPacket[5] >> 5);

                    if (bNoCRC) {
                        if (nPacketSize < AUDIO_AAC_HEADER_SIZE || nPacketSize > nReadSize) {
                            eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
                            LOG_M_E(LOG_TAG, "Invalid packet size(%d)", nPacketSize);
                            break;
                        } else {
                            nNumRead = fread(pPacket + AUDIO_AAC_HEADER_SIZE, 1, nPacketSize - AUDIO_AAC_HEADER_SIZE, pFile);

                            if (nNumRead != nPacketSize - AUDIO_AAC_HEADER_SIZE) {
                                eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
                                LOG_M_E(LOG_TAG, "Partial packet");
                                break;
                            }
                        }
                    } else {
                        if (nPacketSize < AUDIO_AAC_HEADER_WITH_CRC_SIZE || nPacketSize > nReadSize) {
                            eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
                            LOG_M_E(LOG_TAG, "Invalid packet size(%d)", nPacketSize);
                            break;
                        } else {
                            nNumRead =
                                fread(pPacket + AUDIO_AAC_HEADER_WITH_CRC_SIZE, 1, nPacketSize - AUDIO_AAC_HEADER_WITH_CRC_SIZE, pFile);

                            if (nNumRead != nPacketSize - AUDIO_AAC_HEADER_WITH_CRC_SIZE) {
                                eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
                                LOG_M_E(LOG_TAG, "Partial packet");
                                break;
                            }
                        }
                    }
                }

                if (nNumRead > 0) {
                    nBufferSize = nPacketSize;
                    nTotalSize += nBufferSize;
                }
            } else if (PT_LPCM == pAudioFile->eType) {
                if (0 == nTotalSize) {
                    nNumRead = fread(pBuffer, 1, sizeof(AUDIO_WAV_STRUCT_T), pFile);

                    if (nNumRead != sizeof(AUDIO_WAV_STRUCT_T)) {
                        eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
                        LOG_M_E(LOG_TAG, "Error wav format file");
                        break;
                    }

                    AUDIO_WAV_STRUCT_T *pstWavHeader = (AUDIO_WAV_STRUCT_T*)pBuffer;

                    if (AUDIO_ID_RIFF != pstWavHeader->stRiffRegion.nChunkID ||
                        AUDIO_ID_WAVE != pstWavHeader->stRiffRegion.nFormat
                        // only support mono
                        || pstWavHeader->stFmtRegion.nFmtChannels > 1) {

                        eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
                        LOG_M_E(LOG_TAG,
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
                AX_AUDIO_STREAM_T stStream;
                memset(&stStream, 0x00, sizeof(stStream));
                stStream.pStream = (AX_U8 *)pBuffer;
                stStream.u64PhyAddr = 0;
                stStream.u32Len = nBufferSize;
                stStream.bEof = AX_FALSE;
                nRet = PlayAudio(&stStream, AX_OPAL_ADEC_CHN_ID_0);
                if (AX_SUCCESS != nRet) {
                    eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR;
                    LOG_M_E(LOG_TAG, "AX_ADEC_SendStream failed, ret=0x%x", nRet);
                    break;
                }

                // TODO: MSSleep(30);
                usleep(30 * 1000);

            } else {
                break;
            }
        } while (pMalPlay->pFileThread->is_running);

        // loop + 1
        ++nLoop;

        // rewind file
        rewind(pFile);

        // Check AO Status
        do {
            AX_U32 card = pstAttr->stPlayAttr.stDevAttr.nCardId;
            AX_U32 device = pstAttr->stPlayAttr.stDevAttr.nDeviceId;
            AX_AO_DEV_STATE_T stStatus;
            memset(&stStatus, 0x00, sizeof(stStatus));
            nRet = AX_AO_QueryDevStat(card, device, &stStatus);
            if (nRet) {
                LOG_M_E(LOG_TAG, "AX_AO_QueryDevStat[%d][%d] fail, ret:0x%x", card, device, nRet);
                break;
            }

            if (0 < stStatus.u32DevBusyNum) {
                // MSSleep(5);
                usleep(5 * 1000);
                continue;
            } else {
                break;
            }
        } while (1);

    } while (pMalPlay->pFileThread->is_running
            && (pAudioFile->nLoop < 0 || (nLoop < pAudioFile->nLoop))
            && eStatus != AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR);

    if (eStatus != AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR) {
        if (nLoop >= pAudioFile->nLoop) {
            eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_COMPLETE;
        } else {
            eStatus = AX_OPAL_AUDIO_PLAY_FILE_STATUS_STOP;
        }
    }

EXIT:
    if (pFile) {
        fclose(pFile);
    }

    if (pBuffer) {
        free(pBuffer);
    }

    if (pAudioFile->callback) {
        AX_OPAL_AUDIO_PLAY_FILE_RESULT_T tResult;
        memset(&tResult, 0x0, sizeof(AX_OPAL_AUDIO_PLAY_FILE_RESULT_T));
        tResult.eStatus = eStatus;
        tResult.eType = pAudioFile->eType;
        tResult.pstrFileName = pAudioFile->pstrFileName;
        tResult.pUserData = pAudioFile->pUserData;
        pAudioFile->callback(AX_OPAL_ADEC_CHN_ID_0, &tResult);
    }

    LOG_M_D(LOG_TAG, "---");
    return eStatus == AX_OPAL_AUDIO_PLAY_FILE_STATUS_ERROR ? -1 : 0;
}

static AX_VOID ProcAdecFileThread(AX_VOID *arg) {
    LOG_M_D(LOG_TAG, "+++");

    AX_S32 nRet = 0;
    AX_OPAL_THREAD_T* pThread = (AX_OPAL_THREAD_T*)arg;
    if (!pThread) {
        LOG_M_E(LOG_TAG, "Invalid thread arg, null ptr.");
        return;
    }

    AX_OPAL_MAL_AUDIO_PLAY_T *pMalPlayFile = (AX_OPAL_MAL_AUDIO_PLAY_T*)pThread->arg;
    if (pMalPlayFile == AX_NULL) {
        LOG_M_E(LOG_TAG, "Invalid play file arg, null ptr.");
        return;
    }

    AX_CHAR thread_name[16];
    sprintf(thread_name, "adec_file");
    prctl(PR_SET_NAME, thread_name);

    void *data = NULL;
    ADEC_CHN adChn = AX_OPAL_ADEC_CHN_ID_0;
	while (pThread->is_running && pThread->eState == AX_OPAL_THREAD_STATE_RUNNING) {

        if (opal_queue_get_wait(pMalPlayFile->pFileQueue, (void **)&data) == 0) {
            if (data == NULL) {
                break;
            }

            AX_OPAL_MAL_AUDIO_PLAY_PARAM_T* pFileParam = (AX_OPAL_MAL_AUDIO_PLAY_PARAM_T*)data;
            if (pFileParam->pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[adChn].eType != pFileParam->stPlayFile.eType) {
                nRet = AX_OPAL_HAL_AUDIO_PLAY_Stop(pFileParam->pstAttr);
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_OPAL_HAL_AUDIO_PLAY_Destroy failed, ret=0x%x", nRet);
                    AX_OPAL_FREE(pFileParam);
                    break;
                }
                pFileParam->pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[adChn].eType = pFileParam->stPlayFile.eType;
                nRet = AX_OPAL_HAL_AUDIO_PLAY_Start(pFileParam->pstAttr);
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_OPAL_HAL_AUDIO_PLAY_Create failed, ret=0x%x", nRet);
                    AX_OPAL_FREE(pFileParam);
                    break;
                }
            }

            nRet = PlayFileProcess(pFileParam->pstAttr, pFileParam->pMalPlayFile, &pFileParam->stPlayFile);
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AXOP_HAL_AUDIO_PLAY_Destroy failed, ret=0x%x", nRet);
                AX_OPAL_FREE(pFileParam);
                break;
            }

            AX_OPAL_FREE(pFileParam);
        } else {
            break;
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return;
}

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_Init(AX_VOID) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    nRet = AX_AO_Init();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AO_Init failed, ret=0x%x", nRet);
        return nRet;
    }

    nRet = AX_ADEC_Init();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_Init failed, ret=0x%x", nRet);
        return nRet;
    }

#if defined(APP_FAAC_SUPPORT)
    nRet = AX_ADEC_FaacInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_FaacInit failed, ret=0x%x", nRet);
        return nRet;
    }
#elif defined(APP_FDK_SUPPORT)
    nRet = AX_ADEC_FdkInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_FdkInit failed, ret=0x%x", nRet);
        return nRet;
    }
#endif

#ifdef APP_OPUS_SUPPORT
    nRet = AX_ADEC_OpusInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_OpusInit failed, ret=0x%x", nRet);
        return nRet;
    }
#endif

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_Deinit(AX_VOID) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

#if defined(APP_FAAC_SUPPORT)
    nRet = AX_ADEC_FaacDeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_FaacDeInit failed, ret=0x%x", nRet);
        return nRet;
    }
#elif defined(APP_FDK_SUPPORT)
    nRet = AX_ADEC_FdkDeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_FdkDeInit failed, ret=0x%x", nRet);
        return nRet;
    }
#endif

#ifdef APP_OPUS_SUPPORT
    nRet = AX_ADEC_OpusDeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_OpusDeInit failed, ret=0x%x", nRet);
        return nRet;
    }
#endif

    nRet = AX_ADEC_DeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_DeInit failed, ret=0x%x", nRet);
        return nRet;
    }

    nRet = AX_AO_DeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AO_DeInit failed, ret=0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_Start(AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stPlayAttr.bEnable) {
        return 0;
    }
    /* Step3: Ao Init */
    nRet = StartAo(pstAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AO init failed, ret=0x%x", nRet);
        return nRet;
    }

    /* Step4: Adec Init */
    nRet = StartAdec(pstAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "ADEC init failed, ret=0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_Stop(AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stPlayAttr.bEnable) {
        return 0;
    }
    /* Step1: Aenc Deinit */
    nRet = StopAdec(pstAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "ADEC deinit failed, ret=0x%x", nRet);
        return nRet;
    }

    /* Step2: Ao Deinit */
    nRet = StopAo(pstAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AO deinit failed, ret=0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_GetVolume(AX_OPAL_AUDIO_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stPlayAttr.bEnable) {
        return 0;
    }
    AX_U32 card = pstAttr->stPlayAttr.stDevAttr.nCardId;
    AX_U32 device = pstAttr->stPlayAttr.stDevAttr.nDeviceId;

    AX_F64 fVol = 0;
    nRet = AX_AO_GetVqeVolume(card, device, &fVol);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AO_GetVqeVolume[%d][%d] failed, ret=0x%x", card, device, nRet);
    } else {
        pstAttr->stPlayAttr.stDevAttr.fVolume = fVol;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_PLAY_SetVolume(const AX_OPAL_AUDIO_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stPlayAttr.bEnable) {
        return 0;
    }
    AX_U32 card = pstAttr->stPlayAttr.stDevAttr.nCardId;
    AX_U32 device = pstAttr->stPlayAttr.stDevAttr.nDeviceId;
    nRet = AX_AO_SetVqeVolume(card, device, (AX_F64)pstAttr->stPlayAttr.stDevAttr.fVolume);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AO_GetVqeVolume[%d][%d] failed, ret=0x%x", card, device, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_Play(AX_OPAL_AUDIO_ATTR_T* pstAttr, AX_PAYLOAD_TYPE_E eType, const AX_U8 *pData, AX_U32 nDataSize) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stPlayAttr.bEnable) {
        return 0;
    }
    // check type
    if (!IsAudioSupport(eType)) {
        LOG_M_E(LOG_TAG, "unsupport audio type(%d)", eType);
        return -1;
    }

    ADEC_CHN adChn = AX_OPAL_ADEC_CHN_ID_0;
    AX_PAYLOAD_TYPE_E eCurrentType = pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[adChn].eType;
    if (eCurrentType != eType) {
        nRet = AX_OPAL_HAL_AUDIO_PLAY_Stop(pstAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AXOP_HAL_AUDIO_PLAY_Destroy failed, ret=0x%x", nRet);
            return nRet;
        }
        pstAttr->stPlayAttr.stPipeAttr.stAudioChnAttr[adChn].eType = eType;
        nRet = AX_OPAL_HAL_AUDIO_PLAY_Start(pstAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AXOP_HAL_AUDIO_PLAY_Create failed, ret=0x%x", nRet);
            return nRet;
        }
    }

    // play
    AX_AUDIO_STREAM_T stStream;
    memset(&stStream, 0x00, sizeof(stStream));
    stStream.pStream    = (AX_U8*)pData;
    stStream.u64PhyAddr = 0;
    stStream.u32Len     = nDataSize;
    stStream.bEof       = AX_FALSE;

    nRet = PlayAudio(&stStream, adChn);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ADEC_SendStream failed, ret=0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_StopPlay(AX_OPAL_AUDIO_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stPlayAttr.bEnable) {
        return 0;
    }
    nRet = AX_OPAL_HAL_AUDIO_PLAY_Stop(pstAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_AUDIO_PLAY_Stop failed, ret=0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_PlayFile(AX_OPAL_AUDIO_ATTR_T* pstAttr, AX_OPAL_AUDIO_PLAYFILE_T* pPlayFile, AX_OPAL_MAL_AUDIO_PLAY_T *pMalPlayFile) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stPlayAttr.bEnable) {
        return 0;
    }
    if (pMalPlayFile->pFileQueue == NULL) {
        pMalPlayFile->pFileQueue = opal_queue_create_limited(AX_OPAL_AO_FILE_MAX);
    }

    if (pMalPlayFile->pFileThread == NULL) {
        pMalPlayFile->pFileThread = AX_OPAL_CreateThread(ProcAdecFileThread, pMalPlayFile);
        AX_OPAL_StartThread(pMalPlayFile->pFileThread);
    }

    if (!IsAudioSupport(pPlayFile->eType)) {
        LOG_M_E(LOG_TAG, "unsupport audio type(%d)", pPlayFile->eType);
        nRet = -1;
        return nRet;
    }

    if (access(pPlayFile->pstrFileName, F_OK) == -1) {
        LOG_M_E(LOG_TAG, "%s does not exist.", pPlayFile->pstrFileName);
        nRet = -1;
        return nRet;
    }

    AX_OPAL_MAL_AUDIO_PLAY_PARAM_T* pFileParam = (AX_OPAL_MAL_AUDIO_PLAY_PARAM_T*)AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_AUDIO_PLAY_PARAM_T));
    pFileParam->pstAttr = pstAttr;
    pFileParam->pMalPlayFile = pMalPlayFile;
    memcpy(&pFileParam->stPlayFile, pPlayFile, sizeof(AX_OPAL_AUDIO_PLAYFILE_T));

    nRet = opal_queue_put(pMalPlayFile->pFileQueue, pFileParam);
    if (nRet < 0) {
        LOG_M_E(LOG_TAG, "opal_queue_put failed, ret=%d.", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_StopPlayFile(AX_OPAL_AUDIO_ATTR_T* pstAttr, AX_OPAL_MAL_AUDIO_PLAY_T *pMalPlayFile) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stPlayAttr.bEnable) {
        return 0;
    }
    /* - file queue */
    if (pMalPlayFile->pFileQueue != AX_NULL) {
        AX_S32 nCnt = opal_queue_elements(pMalPlayFile->pFileQueue);
        for (AX_S32 i = 0; i < nCnt; i++) {
            AX_VOID* data;
            opal_queue_get(pMalPlayFile->pFileQueue, (AX_VOID **)&data);
            if (data != NULL) {
                AX_OPAL_FREE(data);
            }
        }
        opal_queue_set_new_data(pMalPlayFile->pFileQueue, 0);
        opal_queue_destroy(pMalPlayFile->pFileQueue);
        pMalPlayFile->pFileQueue = AX_NULL;
    }

    /* - file thread */
    if (pMalPlayFile->pFileThread) {
        AX_OPAL_StopThread(pMalPlayFile->pFileThread);
        AX_OPAL_DestroyThread(pMalPlayFile->pFileThread);
        pMalPlayFile->pFileThread = NULL;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}