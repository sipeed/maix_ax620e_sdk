/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_audio_cap.h"

#include "ax_opal_log.h"
#define LOG_TAG  ("HAL_AI")

#define AX_OPAL_AI_DEFAULT_DEPTH                (30)
#define AX_OPAL_AI_STEREO_AAC_PERIOD_SIZE       (1024)
#define AX_OPAL_AI_PERIOD_COUNT                 (8)
#define AX_OPAL_AI_DEFAULT_TINYALSA_CHNCNT      (2)
#define AX_OPAL_AI_DEFAULT_BLK_SIZE             (4 * 1024)
#define AX_OPAL_AI_STEREO_AAC_BLK_SIZE          (8 * 1024)
#define AX_OPAL_AI_DEFAULT_BLK_CNT              (12)  /* 4(ai ) + 8(aenc) */
#define AX_OPAL_AENC_DEFAULT_OUT_DEPTH          (8)
#define AX_OPAL_AENC_CHN_ID_0                   (0)   // only one aenc chn

static AX_POOL g_ax_opal_audio_cap_poolid = AX_INVALID_POOLID;

static AX_BOOL IsUpTalkVqeEnabled(const AX_AP_UPTALKVQE_ATTR_T *pstVqeAttr) {
    return (AX_BOOL)((pstVqeAttr->stAecCfg.enAecMode != AX_AEC_MODE_DISABLE) ||
                            (pstVqeAttr->stNsCfg.bNsEnable != AX_FALSE) ||
                            (pstVqeAttr->stAgcCfg.bAgcEnable != AX_FALSE) ||
                            (pstVqeAttr->stVadCfg.bVadEnable != AX_FALSE));
}

static AX_S32 StartAi(const AX_OPAL_AUDIO_ATTR_T *pstAttr, AX_OPAL_AUDIO_ENCODER_ATTR_T *pstEncoderAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_U32 card = pstAttr->stCapAttr.stDevAttr.nCardId;
    AX_U32 device = pstAttr->stCapAttr.stDevAttr.nDeviceId;

    /* Step1.1: link mode */
    AENC_CHN nAencChnId = AX_OPAL_AENC_CHN_ID_0;
    {
        AX_MOD_INFO_T Ai_Mod = {AX_ID_AI, card, device};
        AX_MOD_INFO_T Aenc_Mod = {AX_ID_AENC, 0, nAencChnId};

        nRet = AX_SYS_Link(&Ai_Mod, &Aenc_Mod);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_SYS_Link failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    // /* Step1.2: AI init */
    // nRet = AX_AI_Init();
    // if (AX_SUCCESS != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_AI_Init failed, ret 0x%X", nRet);
    //     return nRet;
    // }

    /* Step1.3: set AI attribute */
    AX_AI_ATTR_T stAttr;
    memset(&stAttr, 0x00, sizeof(stAttr));
    {
        stAttr.enBitwidth = pstAttr->stDevCommAttr.eBitWidth;
        stAttr.enLinkMode = AX_LINK_MODE;
        stAttr.enSamplerate = (AX_AUDIO_SAMPLE_RATE_E)pstAttr->stDevCommAttr.eSampleRate;  // TODO: convert
        stAttr.enLayoutMode = (AX_AI_LAYOUT_MODE_E)pstAttr->stCapAttr.stDevAttr.eLayoutMode; // TODO: convert
        stAttr.U32Depth = AX_OPAL_AI_DEFAULT_DEPTH;
        if (pstAttr->stCapAttr.stPipeAttr.stAudioChnAttr[nAencChnId].eType == PT_AAC
            && stAttr.enLayoutMode == AX_AI_MIC_MIC) {
            stAttr.u32PeriodSize = AX_OPAL_AI_STEREO_AAC_PERIOD_SIZE;
        } else {
            stAttr.u32PeriodSize = pstAttr->stDevCommAttr.eSampleRate / 100;
        }
        stAttr.u32PeriodCount = AX_OPAL_AI_PERIOD_COUNT;
        stAttr.u32ChnCnt = AX_OPAL_AI_DEFAULT_TINYALSA_CHNCNT;

        nRet = AX_AI_SetPubAttr(card,device,&stAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_AI_SetPubAttr failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    /* Step1.4: attach AI pool */
    if (AX_INVALID_POOLID == g_ax_opal_audio_cap_poolid) {
        AX_U32 nFrameSize = AX_OPAL_AI_DEFAULT_BLK_SIZE;
        if (pstAttr->stCapAttr.stPipeAttr.stAudioChnAttr[nAencChnId].eType == PT_AAC
            && stAttr.enLayoutMode == AX_AI_MIC_MIC) {
            nFrameSize = AX_OPAL_AI_STEREO_AAC_BLK_SIZE;
        }
        g_ax_opal_audio_cap_poolid = AX_OPAL_HAL_SYS_CreatePool(nFrameSize, AX_OPAL_AI_DEFAULT_BLK_CNT, "AUDIO_CAP");
    }
    if (g_ax_opal_audio_cap_poolid == AX_INVALID_POOLID) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_SYS_CreatePool failed");
        return -1;
    }

    nRet = AX_AI_AttachPool(card, device, g_ax_opal_audio_cap_poolid);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_AttachPool failed, ret 0x%X", nRet);
        return nRet;
    }

    /* Step1.5: set AI VQE */
    AX_AP_UPTALKVQE_ATTR_T stVqeAttr;
    memset(&stVqeAttr, 0, sizeof(stVqeAttr));
    {
        stVqeAttr.s32SampleRate = pstAttr->stDevCommAttr.eSampleRate;
        stVqeAttr.u32FrameSamples = pstAttr->stDevCommAttr.eSampleRate / 100;

        if (pstAttr->stDevCommAttr.eSampleRate == AX_OPAL_AUDIO_SAMPLE_RATE_8000 ||
            pstAttr->stDevCommAttr.eSampleRate == AX_OPAL_AUDIO_SAMPLE_RATE_16000) {
            stVqeAttr.s32SampleRate = pstAttr->stDevCommAttr.eSampleRate;
            stVqeAttr.u32FrameSamples = pstAttr->stDevCommAttr.eSampleRate / 100;
        } else {
            stVqeAttr.s32SampleRate = (AX_S32)AX_AUDIO_SAMPLE_RATE_16000;
            stVqeAttr.u32FrameSamples = stVqeAttr.s32SampleRate / 100;
        }

        memcpy(&stVqeAttr.stNsCfg, &pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAnsAttr, sizeof(AX_NS_CONFIG_T));
        memcpy(&stVqeAttr.stAgcCfg, &pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAgcAttr, sizeof(AX_AGC_CONFIG_T));
        memcpy(&stVqeAttr.stAecCfg, &pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAecAttr, sizeof(AX_AEC_CONFIG_T));
        memcpy(&stVqeAttr.stVadCfg, &pstAttr->stCapAttr.stDevAttr.stVqeAttr.stVadAttr, sizeof(AX_VAD_CONFIG_T));

        if (IsUpTalkVqeEnabled(&stVqeAttr)) {
            nRet = AX_AI_SetUpTalkVqeAttr(card, device, &stVqeAttr);
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AX_AI_SetUpTalkVqeAttr failed, ret 0x%X", nRet);
                return nRet;
            }
        }
    }

#if 0  // codec
    /* Step1.6: set ACODEC HPF */
    if (pstACodecAttr->tHpfCfg.bEnable) {
        AX_ACODEC_FREQ_ATTR_T stHpfAttr;
        memset(&stHpfAttr, 0x00, sizeof(stHpfAttr));
        stHpfAttr.s32Freq = pstACodecAttr->tHpfCfg.s32Freq;
        stHpfAttr.s32GainDb = pstACodecAttr->tHpfCfg.s32GainDb;
        stHpfAttr.s32Samplerate = pstDevAttr->nSampleRate;
        nRet = AX_ACODEC_RxHpfSetAttr(card, &stHpfAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxHpfSetAttr failed, ret 0x%X", nRet);
            return nRet;
        }

        nRet = AX_ACODEC_RxHpfEnable(card);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxHpfEnable failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    /* Step1.7: set ACODEC LPF */
    if (pstACodecAttr->tLpfCfg.bEnable) {
        AX_ACODEC_FREQ_ATTR_T stLpfAttr;
        memset(&stLpfAttr, 0x00, sizeof(stLpfAttr));
        stLpfAttr.s32Freq = pstACodecAttr->tLpfCfg.s32Freq;
        stLpfAttr.s32GainDb = pstACodecAttr->tLpfCfg.s32GainDb;
        stLpfAttr.s32Samplerate = pstDevAttr->nSampleRate;
        nRet = AX_ACODEC_RxLpfSetAttr(card, &stLpfAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxLpfSetAttr failed, ret 0x%X", nRet);
            return nRet;
        }

        nRet = AX_ACODEC_RxLpfEnable(card);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxLpfEnable failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    /* Step1.8: set ACODEC EQ */
    if (pstACodecAttr->tEqCfg.bEnable) {
        AX_ACODEC_EQ_ATTR_T stEqAttr;
        memset(&stEqAttr, 0x00, sizeof(stEqAttr));
        memcpy(stEqAttr.s32GainDb, &pstACodecAttr->tEqCfg.s32GainDb, sizeof(stEqAttr.s32GainDb));
        stEqAttr.s32Samplerate = pstDevAttr->nSampleRate;
        nRet = AX_ACODEC_RxEqSetAttr(card, &stEqAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxEqSetAttr failed, ret 0x%X", nRet);
            return nRet;
        }

        nRet = AX_ACODEC_RxEqEnable(card);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxEqEnable failed, ret 0x%X", nRet);
            return nRet;
        }
    }
#endif

    /* Step1.9: enable AI device */
    nRet = AX_AI_EnableDev(card, device);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_EnableDev failed, ret 0x%X", nRet);
        return nRet;
    }

    /* Step1.10: enable AI resample */
    if (pstAttr->stDevCommAttr.eSampleRate != stVqeAttr.s32SampleRate) {
        nRet = AX_AI_EnableResample(card, device, stVqeAttr.s32SampleRate);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_AI_EnableResample failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    /* Step1.11: set AI VQE volume */
    nRet = AX_AI_SetVqeVolume(card, device, pstAttr->stCapAttr.stDevAttr.fVolume);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_SetVqeVolume failed, ret 0x%X", nRet);
        return nRet;
    }

    pstEncoderAttr->eSampleRate = stVqeAttr.s32SampleRate;
    pstEncoderAttr->eBitWidth = pstAttr->stDevCommAttr.eBitWidth;

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 StartAenc(const AX_OPAL_AUDIO_ATTR_T *pstAttr, AX_OPAL_AUDIO_ENCODER_ATTR_T *pstEncoderAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AENC_CHN aeChn = AX_OPAL_AENC_CHN_ID_0;
    AX_U32 u32BitRate = pstAttr->stCapAttr.stPipeAttr.stAudioChnAttr[aeChn].nBitRate;
    AX_PAYLOAD_TYPE_E eEncType = pstAttr->stCapAttr.stPipeAttr.stAudioChnAttr[aeChn].eType;
    AX_OPAL_AUDIO_SOUND_MODE_E eSoundMode = pstAttr->stCapAttr.stPipeAttr.stAudioChnAttr[aeChn].eSoundMode; // AX_OPAL_AUDIO_SOUND_MODE_MONO;
    AX_U32 nAOT = 0;

    AX_AENC_CHN_ATTR_T tAttr;
    memset(&tAttr, 0x0, sizeof(AX_AENC_CHN_ATTR_T));

    AX_AENC_AAC_ENCODER_ATTR_T aacEncoderAttr = {
        .enAacType = AX_AAC_TYPE_AAC_LC,
        .enTransType = AX_AAC_TRANS_TYPE_ADTS,
        .enChnMode = ((AX_AI_LAYOUT_MODE_E)pstAttr->stCapAttr.stDevAttr.eLayoutMode == AX_AI_MIC_MIC) ? AX_AAC_CHANNEL_MODE_2 : AX_AAC_CHANNEL_MODE_1,
        .u32GranuleLength = 1024,
        .u32SampleRate = pstEncoderAttr->eSampleRate,
        .u32BitRate = u32BitRate
    };

    AX_AENC_G726_ENCODER_ATTR_T stG726EncoderAttr = {
        .u32BitRate = u32BitRate
    };

    AX_AENC_G723_ENCODER_ATTR_T stG723EncoderAttr = {
        .u32BitRate = u32BitRate
    };

    /* Step2.2: encoder init */
    tAttr.enType = eEncType;
    tAttr.u32BufSize = AX_OPAL_AENC_DEFAULT_OUT_DEPTH;
    tAttr.enLinkMode = AX_LINK_MODE;

    if (tAttr.enType == PT_AAC) {
        // eSoundMode = AX_OPAL_AUDIO_SOUND_MODE_STEREO;
        tAttr.u32PtNumPerFrm = 1024;
        tAttr.pValue = &aacEncoderAttr;
#if defined(APP_FAAC_SUPPORT)
        nRet = AX_AENC_FaacInit();
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_AENC_FaacInit failed, ret=0x%x", nRet);
            return nRet;
        }
#elif defined(APP_FDK_SUPPORT)
        nRet = AX_AENC_FdkInit();
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_AENC_FdkInit failed, ret=0x%x", nRet);
            return nRet;
        }
#endif
        nAOT = AX_AAC_TYPE_AAC_LC;
    } else if (tAttr.enType == PT_G726) {
        tAttr.u32PtNumPerFrm = 480;
        tAttr.pValue = &stG726EncoderAttr;
    } else if (tAttr.enType == PT_G723) {
        tAttr.u32PtNumPerFrm = 480;
        tAttr.pValue = &stG723EncoderAttr;
    } else {
        tAttr.u32PtNumPerFrm = 1024;
        tAttr.pValue = NULL;
    }

    /* Step2.3: create channel */
    {
        nRet = AX_AENC_CreateChn(aeChn, &tAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_AENC_CreateChn failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    // encoder info
    pstEncoderAttr->nAOT = nAOT;
    pstEncoderAttr->nBitRate = u32BitRate;
    pstEncoderAttr->eType = eEncType;
    pstEncoderAttr->eSoundMode = eSoundMode;

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}


static AX_S32 StopAi(const AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_U32 card = pstAttr->stCapAttr.stDevAttr.nCardId;
    AX_U32 device = pstAttr->stCapAttr.stDevAttr.nDeviceId;
    AENC_CHN aeChn = AX_OPAL_AENC_CHN_ID_0;

    /* Step2.1: disable AI device */
    nRet = AX_AI_DisableDev(card, device);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_DisableDev failed, ret 0x%X", nRet);
        return nRet;
    }

#if 0 // codec
    /* Step2.1: disable EQ */
    if (pstACodecAttr->tEqCfg.bEnable) {
        nRet = AX_ACODEC_RxEqDisable(card);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxEqDisable failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    /* Step2.2: disable LPF */
    if (pstACodecAttr->tLpfCfg.bEnable) {
        nRet = AX_ACODEC_RxLpfDisable(card);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxLpfDisable failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    /* Step2.3: disable HPF */
    if (pstACodecAttr->tHpfCfg.bEnable) {
        nRet = AX_ACODEC_RxHpfDisable(card);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_ACODEC_RxHpfDisable failed, ret 0x%X", nRet);
            return nRet;
        }
    }
#endif

    /* Step2.4: detach pool */
    nRet = AX_AI_DetachPool(card, device);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_DetachPool failed, ret 0x%X", nRet);
        return nRet;
    }

    if (g_ax_opal_audio_cap_poolid != AX_INVALID_POOLID) {
        AX_OPAL_HAL_SYS_DestroyPool(g_ax_opal_audio_cap_poolid);
        g_ax_opal_audio_cap_poolid = AX_INVALID_POOLID;
    }

    // /* Step2.5: AI deinit */
    // nRet = AX_AI_DeInit();
    // if (AX_SUCCESS != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_AI_DeInit failed, ret 0x%X", nRet);
    // }

    /* Step2.6: unlink */
    {
        AX_MOD_INFO_T Ai_Mod = {AX_ID_AI, card, device};
        AX_MOD_INFO_T Aenc_Mod = {AX_ID_AENC, 0, aeChn};
        nRet = AX_SYS_UnLink(&Ai_Mod, &Aenc_Mod);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_SYS_UnLink failed, ret 0x%X", nRet);
            return nRet;
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 StopAenc(const AX_OPAL_AUDIO_ENCODER_ATTR_T *pstEncoderAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AENC_CHN aeChn = AX_OPAL_AENC_CHN_ID_0;

    /* Step1.1: destroy chn */
    nRet = AX_AENC_DestroyChn(aeChn);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_DestroyChn failed, ret 0x%X", nRet);
        return nRet;
    }

    /* Step1.2: encoder deinit */
    switch (pstEncoderAttr->eType) {
    case PT_AAC:
        {
#if defined(APP_FAAC_SUPPORT)
            nRet = AX_AENC_FaacDeInit();
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AX_AENC_FaacDeInit failed, ret=0x%x", nRet);
                return nRet;
            }
#elif defined(APP_FDK_SUPPORT)
            nRet = AX_AENC_FdkDeInit();
            if (AX_SUCCESS != nRet) {
                LOG_M_E(LOG_TAG, "AX_AENC_FdkDeInit failed, ret=0x%x", nRet);
                return nRet;
            }
#endif
        }
        break;
    default:
        break;
    }

    // /* Step1.3: adec deinit */
    // nRet = AX_AENC_DeInit();
    // if (AX_SUCCESS != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_AENC_DeInit failed, ret 0x%X", nRet);
    // }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_CAP_Init(AX_VOID) {
    LOG_M_D(LOG_TAG, "+++");

    AX_S32 nRet = AX_SUCCESS;

    nRet = AX_AI_Init();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_Init failed, ret=0x%x", nRet);
        return nRet;
    }

    nRet = AX_AENC_Init();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_Init failed, ret=0x%x", nRet);
        return nRet;
    }

#if defined(APP_FAAC_SUPPORT)
    nRet = AX_AENC_FaacInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_FaacInit failed, ret=0x%x", nRet);
        return nRet;
    }
#elif defined(APP_FDK_SUPPORT)
    nRet = AX_AENC_FdkInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_FdkInit failed, ret=0x%x", nRet);
        return nRet;
    }
#endif

#ifdef APP_OPUS_SUPPORT
    nRet = AX_AENC_OpusInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_OpusInit failed, ret=0x%x", nRet);
        return nRet;
    }
#endif

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_CAP_Deinit(AX_VOID) {
    AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

#if defined(APP_FAAC_SUPPORT)
    nRet = AX_AENC_FaacDeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_FaacDeInit failed, ret=0x%x", nRet);
        return nRet;
    }
#elif defined(APP_FDK_SUPPORT)
    nRet = AX_AENC_FdkDeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_FdkDeInit failed, ret=0x%x", nRet);
        return nRet;
    }
#endif

#ifdef APP_OPUS_SUPPORT
    nRet = AX_AENC_OpusDeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_OpusDeInit failed, ret=0x%x", nRet);
        return nRet;
    }
#endif
    nRet = AX_AI_DeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_DeInit failed, ret=0x%x", nRet);
        return nRet;
    }

    nRet = AX_AENC_DeInit();
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AENC_DeInit failed, ret=0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_CAP_Start(AX_OPAL_AUDIO_ATTR_T *pstAttr, AX_OPAL_AUDIO_ENCODER_ATTR_T *pstEncoderAttr) {
    AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

    if (!pstAttr->stCapAttr.bEnable) {
        return nRet;
    }

    /* Step1: Ai Init */
    nRet = StartAi(pstAttr, pstEncoderAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "Start Ai failed, ret:0x%x", nRet);
        return nRet;
    }

    /* Step2: Aenc Init */
    nRet = StartAenc(pstAttr, pstEncoderAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "Start Aenc failed, ret:0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_CAP_Stop(AX_OPAL_AUDIO_ATTR_T *pstAttr, const AX_OPAL_AUDIO_ENCODER_ATTR_T *pstEncoderAttr) {
    AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

    if (!pstAttr->stCapAttr.bEnable) {
        return nRet;
    }

    /* Step1: Aenc Deinit */
    nRet = StopAenc(pstEncoderAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "SampleAencDeinit fail, ret:0x%x", nRet);
        return nRet;
    }

    /* Step2: Ai Deinit */
    nRet = StopAi(pstAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "SampleAiDeinit fail, ret:0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_CAP_GetVolume(AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stCapAttr.bEnable) {
        return 0;
    }
    AX_U32 card = pstAttr->stCapAttr.stDevAttr.nCardId;
    AX_U32 device = pstAttr->stCapAttr.stDevAttr.nDeviceId;
    AX_F64 fVol = 0;
    AX_S32 nRet = AX_AI_GetVqeVolume(card, device, &fVol);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_GetVqeVolume[%d][%d] failed ret=%08X", card, device, nRet);
    }
    pstAttr->stCapAttr.stDevAttr.fVolume = fVol;

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_AUDIO_CAP_SetVolume(const AX_OPAL_AUDIO_ATTR_T *pstAttr) {
    LOG_M_D(LOG_TAG, "+++");
    if (!pstAttr->stCapAttr.bEnable) {
        return 0;
    }
    AX_U32 card = pstAttr->stCapAttr.stDevAttr.nCardId;
    AX_U32 device = pstAttr->stCapAttr.stDevAttr.nDeviceId;
    AX_S32 nRet = AX_AI_SetVqeVolume(card, device, (AX_F64)pstAttr->stCapAttr.stDevAttr.fVolume);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_AI_SetVqeVolume[%d][%d] failed ret=%08X", card, device, nRet);
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}