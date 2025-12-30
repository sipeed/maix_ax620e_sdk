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
#include "audio_config.h"

// audio capture config
// audio input
#define SAMPLE_AI_DEFAULT_CARD_ID 0
#define SAMPLE_AI_DEFAULT_DEVICE_ID 0
#define SAMPLE_AI_DEFAULT_LAYOUT_MODE AX_AI_MIC_REF // depend on hardware design; // mono: AX_AI_MIC_REF/AX_AI_REF_MIC; stereo: AX_AI_MIC_MIC
#define SAMPLE_AI_DEFAULT_BIT_WIDTH AX_AUDIO_BIT_WIDTH_16
#define SAMPLE_AI_DEFAULT_SAMPLE_RATE 16000
#define SAMPLE_AI_DEFAULT_VQE_VOLUME 1.0

// audio encoder
#define SAMPLE_AENC_DEFAULT_CHANNEL 0
#define SAMPLE_AENC_DEFAULT_SAMPLERATE 16000
#define SAMPLE_AENC_DEFAULT_BITRATE 48000
#ifdef QSDEMO_AUDIO_AAC_SUPPORT
#define SAMPLE_AENC_DEFAULT_ENCODER PT_AAC // PT_AAC // PT_G711A // PT_G711U
#else
#define SAMPLE_AENC_DEFAULT_ENCODER PT_G711A // PT_AAC // PT_G711A // PT_G711U
#endif

static const SAMPLE_AI_DEV_ATTR_T g_sample_ai_devAttr = {
    .nCardNum = SAMPLE_AI_DEFAULT_CARD_ID,
    .nDeviceNum = SAMPLE_AI_DEFAULT_DEVICE_ID,
    .eLayoutMode = SAMPLE_AI_DEFAULT_LAYOUT_MODE,
    .eBitWidth = SAMPLE_AI_DEFAULT_BIT_WIDTH,
    .nSampleRate = SAMPLE_AI_DEFAULT_SAMPLE_RATE,
    .nVqeSampleRate = SAMPLE_AI_DEFAULT_SAMPLE_RATE,
    .fVqeVolume = SAMPLE_AI_DEFAULT_VQE_VOLUME,
    .bResample = AX_FALSE,
    .nResRate = SAMPLE_AI_DEFAULT_SAMPLE_RATE,
};

static const AX_AEC_CONFIG_T g_sample_ai_AecCfg = {
    .enAecMode = AX_AEC_MODE_DISABLE,
};

static const AX_NS_CONFIG_T g_sample_ai_NsCfg = {
    .bNsEnable = AX_FALSE,
    .enAggressivenessLevel = 2,
};

static const AX_AGC_CONFIG_T g_sample_ai_AgcCfg = {
    .bAgcEnable = AX_FALSE,
    .enAgcMode = AX_AGC_MODE_FIXED_DIGITAL,
    .s16TargetLevel = -3,
    .s16Gain = 9,
};

static AX_VAD_CONFIG_T g_sample_ai_VadCfg = {
    .bVadEnable = AX_FALSE,
    .u32VadLevel = 2,
};

static const SAMPLE_AP_UPTALKVQE_ATTR_T g_sample_ai_UpVeqAttr = {
    .pstAecCfg = &g_sample_ai_AecCfg,
    .pstNsCfg = &g_sample_ai_NsCfg,
    .pstAgcCfg = &g_sample_ai_AgcCfg,
    .pstVadCfg = &g_sample_ai_VadCfg,
};

static const AX_ACODEC_EQ_ATTR_T g_sample_EqCfg = {
    .bEnable = AX_FALSE,
    .s32GainDb = {-10,-3,3,5,10},
    .s32Samplerate = 16000,
};

static const AX_ACODEC_FREQ_ATTR_T g_sample_HpfCfg = {
    .bEnable = AX_FALSE,
    .s32GainDb = -3,
    .s32Samplerate = 16000,
    .s32Freq = 200,
};

static const AX_ACODEC_FREQ_ATTR_T g_sample_LpfCfg = {
    .bEnable = AX_FALSE,
    .s32GainDb = 0,
    .s32Samplerate = 16000,
    .s32Freq = 3000,
};

static const SAMPLE_ACODEC_ATTR_T g_sample_AcodecAttr = {
    .pstEqCfg = &g_sample_EqCfg,
    .pstHpfCfg = &g_sample_HpfCfg,
    .pstLpfCfg = &g_sample_LpfCfg,
};

static const SAMPLE_AENC_CHN_ATTR_T g_sample_aenc_chn_attr = {
    .nEncChn = SAMPLE_AENC_DEFAULT_CHANNEL,
    .nEncSampleRate = SAMPLE_AENC_DEFAULT_SAMPLERATE,
    .nEncBitRate = SAMPLE_AENC_DEFAULT_BITRATE,
    .eEncType = SAMPLE_AENC_DEFAULT_ENCODER
};

static const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T g_sample_audio_capture_entry_param = {
    .pstDevAttr = &g_sample_ai_devAttr,
    .pstUpVqeAttr = &g_sample_ai_UpVeqAttr,
    .pstACodecAttr = &g_sample_AcodecAttr,
    .pstAencChnAttr = &g_sample_aenc_chn_attr,
};

// audio play config
// audio output
#define SAMPLE_AO_DEFAULT_CARD_ID 0
#define SAMPLE_AO_DEFAULT_DEVICE_ID 1
#define SAMPLE_AO_DEFAULT_SOUND_MODE AX_AUDIO_SOUND_MODE_MONO // mono: AX_AUDIO_SOUND_MODE_MONO; stereo: AX_AUDIO_SOUND_MODE_STEREO
#define SAMPLE_AO_DEFAULT_BIT_WIDTH AX_AUDIO_BIT_WIDTH_16
#define SAMPLE_AO_DEFAULT_SAMPLE_RATE 16000
#define SAMPLE_AO_DEFAULT_VQE_VOLUME 1.0
#define SAMPLE_PLAY_DEFAULT_KEEP_ALIVE AX_FALSE

// audio decoder
#define SAMPLE_ADEC_DEFAULT_CHANNEL 0
#define SAMPLE_ADEC_DEFAULT_DECODER PT_G711A

static const SAMPLE_AO_DEV_ATTR_T g_sample_ao_devAttr = {
    .nCardNum = SAMPLE_AO_DEFAULT_CARD_ID,
    .nDeviceNum = SAMPLE_AO_DEFAULT_DEVICE_ID,
    .enSoundmode = SAMPLE_AO_DEFAULT_SOUND_MODE,
    .eBitWidth = SAMPLE_AO_DEFAULT_BIT_WIDTH,
    .nSampleRate = SAMPLE_AO_DEFAULT_SAMPLE_RATE,
    .nVqeSampleRate = SAMPLE_AO_DEFAULT_SAMPLE_RATE,
    .fVqeVolume = SAMPLE_AO_DEFAULT_VQE_VOLUME,
    .bResample = AX_FALSE,
    .nResRate = SAMPLE_AO_DEFAULT_SAMPLE_RATE,
};

static const AX_NS_CONFIG_T g_sample_ao_NsCfg = {
    .bNsEnable = AX_FALSE,
    .enAggressivenessLevel = 2,
};

static const AX_AGC_CONFIG_T g_sample_ao_AgcCfg = {
    .bAgcEnable = AX_FALSE,
    .enAgcMode = AX_AGC_MODE_FIXED_DIGITAL,
    .s16TargetLevel = -3,
    .s16Gain = 9,
};

static const SAMPLE_DnVQE_ATTR_T g_sample_ao_DnVeqAttr = {
    .pstNsCfg = &g_sample_ao_NsCfg,
    .pstAgcCfg = &g_sample_ao_AgcCfg,
};

static const SAMPLE_ADEC_CHN_ATTR_T g_sample_adec_chn_attr = {
    .nDecChn = SAMPLE_ADEC_DEFAULT_CHANNEL,
    .eDecType = SAMPLE_ADEC_DEFAULT_DECODER
};

static const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T g_sample_audio_play_entry_param = {
    .bKeepAlive = SAMPLE_PLAY_DEFAULT_KEEP_ALIVE,
    .pstDevAttr = &g_sample_ao_devAttr,
    .pstDnVqeAttr = &g_sample_ao_DnVeqAttr,
    .pstAdecChnAttr = &g_sample_adec_chn_attr,
};

const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T *qs_audio_capture_config(AX_VOID) {
    return &g_sample_audio_capture_entry_param;
}

const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T *qs_audio_play_config(AX_VOID) {
    return &g_sample_audio_play_entry_param;
}
#endif
