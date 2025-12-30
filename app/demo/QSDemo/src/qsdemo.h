/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _QSDEMO_H_
#define _QSDEMO_H_
#include "qs_log.h"

#include "ax_venc_api.h"
#include "ax_ivps_api.h"
#include "ax_isp_api.h"
#include "ax_buffer_tool.h"
#include "ax_isp_3a_api.h"
#include "common_sys.h"
#include "common_vin.h"
#include "common_cam.h"
#include "common_audio.h"
#include "ax_osd_type.h"

#define ALIGN_UP_16(value) ((value + 0xF) & (~0xF))
#define ALIGN_UP_64(value) ((value + 0x3F) & (~0x3F))
#define SAMPLE_VENC_CHN_NUM_MAX   (6)

#define SENSOR_MAX_FRAMERATE (30)
#define SENSOR_FRAMERATE (15)
#define DETECT_FRAMERATE (15)
#define JENC_FRAMERATE (15)
#define DET_MAX_FRAME_NOBODY (5)

#define MAX_HEIGHT_DEFAULT 16384
#define MAX_WIDTH_DEFAULT 16384
#define IVPSChannelNumber (3)

#define IS_SNS_LINEAR_MODE(eSensorMode) (((eSensorMode == AX_SNS_LINEAR_MODE) || (eSensorMode == QS_DEMO_DEFAULT_SNS_MODE)) ? AX_TRUE : AX_FALSE)
#define IS_SNS_HDR_MODE(eSensorMode) (((eSensorMode >= AX_SNS_HDR_2X_MODE) && (eSensorMode <= AX_SNS_HDR_4X_MODE)) ? AX_TRUE : AX_FALSE)

#define QS_DEMO_DEFAULT_SNS_MODE (AX_SNS_LINEAR_ONLY_MODE)

typedef struct _stVencGetStreamParam
{
    AX_S32 nSndId;
    AX_S32 nVeChn;
    AX_BOOL bThreadStart;
    AX_PAYLOAD_TYPE_E ePayloadType;
    pthread_t nTid;
    AX_U64 u64LastSeqNum;
    AX_U64 u64LastFramePts;
    AX_U64 u64GetVencFrmCnt;
} VENC_GETSTREAM_PARAM_T;

typedef enum
{
    SAMPLE_RC_CBR = 0,
    SAMPLE_RC_VBR = 1,
    SAMPLE_RC_AVBR = 2,
    SAMPLE_RC_QPMAP = 3,
    SAMPLE_RC_FIXQP = 4,
    SAMPLE_RC_CVBR = 5,
} SAMPLE_VENC_RC_E;

typedef struct _stRCInfo
{
    SAMPLE_VENC_RC_E eRCType;
    AX_U32 nMinQp;
    AX_U32 nMaxQp;
    AX_U32 nMinIQp;
    AX_U32 nMaxIQp;
    AX_S32 nIntraQpDelta;
} RC_INFO_T;

typedef struct _stVideoConfig
{
    AX_PAYLOAD_TYPE_E ePayloadType;
    AX_U32 nGOP;
    AX_U32 nSrcFrameRate;
    AX_U32 nDstFrameRate;
    AX_U32 nStride;
    AX_S32 nInWidth;
    AX_S32 nInHeight;
    AX_S32 nOutWidth;
    AX_S32 nOutHeight;
    AX_IMG_FORMAT_E eImgFormat;
    RC_INFO_T stRCInfo;
    AX_S32 nBitrate;
} VIDEO_CONFIG_T;

typedef struct {
    AX_U32 nStride;
    AX_S32 nWidth;
    AX_S32 nHeight;
    AX_F32 fFramerate;
    AX_IMG_FORMAT_E eImgFormat;
    AX_COMPRESS_MODE_E enCompressMode;
    AX_BOOL bVenc;
    AX_S32  nRtspChn;
    AX_F32  fDetectWidthRatio;
    AX_F32  fDetectHeightRatio;
    AX_OSD_REGION_PARAM_T *ptRgn; // RECT RGN
} SAMPLE_CHN_ATTR_T;

typedef struct {
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_S32 nAiIspMode;
    AX_BOOL bDetectEnable;
    AX_BOOL bJencEnable;
    AX_BOOL bVencEnable;
    AX_S32 nSensorFrameRate;
    AX_S32 nDetectFrameRate;
    AX_S32 nDetectFrameMax;
    AX_S32 nJencFrameRate;
    AX_S32 nVencDumpNum;
    AX_BOOL bJencDump;
    AX_S32 nCamCnt;
    AX_S32 nIvpsChnCnt;
    AX_S32 nSleepTime; // ms
    AX_S32 nDetetAlgo; // 0: SKEL, 1: MD, 2: MD->SKEL
    AX_S32 nAudioFlag; // 0: NONE, 1: capture only, 2: play only, 3: capture + play
    AX_BOOL bSc850sl2M;
    AX_S32 nHdrModeTest;
    AX_S32 nVinIvpsMode;
    AX_S32 nGdcOnlineVppTest;
    AX_S32 nVinChnFrmMode;
    AX_S32 nRebootInterval;
    AX_BOOL bRandomCapNum;
    AX_S32 nCapNum;
    AX_BOOL bRtspEnable;
    AX_BOOL bUseTispInAov;
    AX_BOOL bStoreAudioInAov;
    AX_S32 nVencRcChangePolicy;
    AX_S32 nMaxRecordFileCount;
    AX_S32 nGopInAov;
    AX_S32 nSwitchSnsId;
    AX_BOOL bAutoZoomLoopOn;
    SAMPLE_EZOOM_SWITCH_INFO_T tEzoomSwitchInfo;
    AX_S32 nSlowShutter;
    AX_S32 nAeManualShutter;
} SAMPLE_ENTRY_PARAM_T;

typedef enum {
    E_SNS_TISP_MODE_E = 0,
    E_SNS_AIISP_DEFAULT_SCENE_MODE_E = 1,
    E_SNS_AIISP_MANUAL_SCENE_AIISP_MODE_E = 2,
    E_SNS_AIISP_MANUAL_SCENE_TISP_MODE_E = 3,
    E_SNS_AIISP_AUTO_SCENE_MODE_E = 4,
    E_SNS_AIISP_BUTT_MODE_E,
} SNS_AIISP_MODE_E;

typedef struct _SYS_LINK_MOD_PARAM
{
    AX_MOD_INFO_T srcMod;
    AX_MOD_INFO_T dstMod;
} SYS_LINK_MOD_PARAM_T;

typedef enum {
    E_STS_GET_FRAME_0       = 0,     // get 1st yuv frame from ivps for sns0
    E_STS_GET_SKEL_0        = 1,     // get result from skel for sns0
    E_STS_GET_VENC_0        = 2,     // get venc 1st stream for sns0
    E_STS_SYSSLEEP_INVOKE   = 3,     // invoke AX_SYS_SLEEP
    E_STS_SYSSLEEP_RETURN   = 4,     // AX_SYS_SLEEP return
    E_STS_GET_FRAME_1       = 5,     // get 1st yuv frame from ivps for sns1
    E_STS_GET_SKEL_1        = 6,     // get result from skel for sns1
    E_STS_GET_VENC_1        = 7,     // get venc 1st stream for sns1
    E_STS_NOT_USED1         = 8,     // NOT_USED1
    E_STS_NOT_USED2         = 9,     // NOT_USED2
    E_STS_BUT,
} QS_SLEEPTIME_STAT_T;

#define SAMPLE_PHY_MEM_ALIGN_SIZE (16)

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))


#define CONDITION0_MASK_NONE    0x0       //    0000
#define CONDITION_MASK_SKEL     0x1       //    0001
#define CONDITION_MASK_VENC0    0x2       //    0010
#define CONDITION_MASK_VENC1    0x4       //    0100
#define CONDITION_MASK_STOP     0x8       //    1000
#define CONDITION_MASK_AUDIO    0x10      //   10000
#define CONDITION_MASK_RECORD0  0x20      //  100000
#define CONDITION_MASK_RECORD1  0x40      // 1000000

void update_sleep_condition(int mask, bool bSetTrue);

AX_S32 qs_cam_pool_config(SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs, COMMON_SYS_ARGS_T *pPrivArgs, AX_CAMERA_T* pCamList);
AX_S32 qs_cam_change_hdrmode(SAMPLE_ENTRY_PARAM_T *pEntryParam, AX_CAMERA_T* pCamList);
#endif // _QSDEMO_H_
