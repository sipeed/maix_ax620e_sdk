/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#pragma once
#include <string.h>
#include <string>
#include <vector>
#include "ax_isp_api.h"
#include "ax_mipi_rx_api.h"
#include "ax_vin_api.h"
#include "ax_isp_3a_plus.h"
#include "GlobalDef.h"

#define IS_SNS_LINEAR_MODE(eSensorMode) (((eSensorMode == AX_SNS_LINEAR_MODE) || (eSensorMode == AX_SNS_LINEAR_ONLY_MODE)) ? AX_TRUE : AX_FALSE)
#define IS_SNS_HDR_MODE(eSensorMode) (((eSensorMode >= AX_SNS_HDR_2X_MODE) && (eSensorMode <= AX_SNS_HDR_4X_MODE)) ? AX_TRUE : AX_FALSE)

#define IS_SWITCHED_SNS(eSwitchSnsType) (((eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN) || (eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SUB)) ? AX_TRUE : AX_FALSE)

#define MAX_PIPE_PER_DEVICE (3)

#define SNS_MODE_SDR       (1)
#define SNS_MODE_HDR       (2)
#define SNS_MODE_LFHDR_SDR (3)
#define SNS_MODE_LFHDR_HDR (4)

#define GET_SNS_HDR_TYPE(nSnsMode) (((nSnsMode == SNS_MODE_LFHDR_SDR) || (nSnsMode == SNS_MODE_LFHDR_HDR)) ? E_SNS_HDR_TYPE_LONGFRAME : E_SNS_HDR_TYPE_ClASSICAL)

#define APP_INVALID_SENSOR_ID (-1)
#define APP_INVALID_PIPE_ID   (-1)
#define COORDINATION_DETECT_MAX_FRAME_NOSHOW (15)

using std::string;
using std::vector;

typedef enum {
    /* New sensors should be append to the last */
    E_SNS_TYPE_OS04A10 = 0,
    E_SNS_TYPE_SC450AI = 1,
    E_SNS_TYPE_IMX678 = 2,
    E_SNS_TYPE_SC200AI = 3,
    E_SNS_TYPE_SC500AI = 4,
    E_SNS_TYPE_SC850SL = 5,
    E_SNS_TYPE_C4395 = 6,
    E_SNS_TYPE_GC4653 = 7,
    E_SNS_TYPE_MIS2032 = 8,
    E_SNS_TYPE_OS12D40 = 9,
    E_SNS_TYPE_IMX258 = 10,
    /* HDR Split Mode: Long Frame */
    E_SNS_TYPE_LF_START = 100,
    E_SNS_TYPE_OS04A10_LF = 101,
    E_SNS_TYPE_SC450AI_LF = 102,
    E_SNS_TYPE_IMX678_LF = 103,
    E_SNS_TYPE_SC200AI_LF = 104,
    E_SNS_TYPE_SC500AI_LF = 105,
    E_SNS_TYPE_SC850SL_LF = 106,
    E_SNS_TYPE_C4395_LF = 107,
    E_SNS_TYPE_GC4653_LF = 108,
    E_SNS_TYPE_MIS2032_LF = 109,
    E_SNS_TYPE_OS12D40_LF = 110,
    E_SNS_TYPE_IMX258_LF = 111,
    E_SNS_TYPE_LF_END = 199,
    /* HDR Split Mode: Short Frame */
    E_SNS_TYPE_SF_START = 200,
    E_SNS_TYPE_OS04A10_SF = 201,
    E_SNS_TYPE_SC450AI_SF = 202,
    E_SNS_TYPE_IMX678_SF = 203,
    E_SNS_TYPE_SC200AI_SF = 204,
    E_SNS_TYPE_SC500AI_SF = 205,
    E_SNS_TYPE_SC850SL_SF = 206,
    E_SNS_TYPE_C4395_SF = 207,
    E_SNS_TYPE_GC4653_SF = 208,
    E_SNS_TYPE_MIS2032_SF = 209,
    E_SNS_TYPE_OS12D40_SF = 220,
    E_SNS_TYPE_IMX258_SF = 221,
    E_SNS_TYPE_SF_END = 299,
    E_SNS_TYPE_MAX,
} SNS_TYPE_E;

typedef enum {
    E_SNS_HDR_TYPE_ClASSICAL = 0,
    E_SNS_HDR_TYPE_LONGFRAME
} SNS_HDR_TYPE_E;

typedef enum {
    E_DEPURPLE_MODE_NOCHANGE = -1,
    E_DEPURPLE_MODE_OFF,
    E_DEPURPLE_MODE_ON,
} DEPURPLE_MODE_E;

typedef enum {
    E_SNS_TISP_MODE_E = 0,
    E_SNS_AIISP_DEFAULT_SCENE_MODE_E = 1,
    E_SNS_AIISP_MANUAL_SCENE_AIISP_MODE_E = 2,
    E_SNS_AIISP_MANUAL_SCENE_TISP_MODE_E = 3,
    E_SNS_AIISP_AUTO_SCENE_MODE_E = 4,
    E_SNS_AIISP_BUTT_MODE_E,
} SNS_AIISP_MODE_E;

typedef enum {
    E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E = 0,
    E_SNS_SHUTTER_SLOW_SHUTTER_MODE_E = 1,
    E_SNS_SHUTTER_BUTT_MODE_E,
} SNS_SHUTTER_MODE_E;

// sensor soft photosensitivity status
typedef enum _SNS_SOFT_PHOTOSENSITIVITY_STATUS_E {
    SNS_SOFT_PHOTOSENSITIVITY_STATUS_DAY,
    SNS_SOFT_PHOTOSENSITIVITY_STATUS_NIGHT,
    SNS_SOFT_PHOTOSENSITIVITY_STATUS_UNKOWN,
    SNS_SOFT_PHOTOSENSITIVITY_STATUS_BUTT
} SNS_SOFT_PHOTOSENSITIVITY_STATUS_E;

// sensor soft photosensitivity type
typedef enum {
    SNS_SOFT_PHOTOSENSITIVITY_NONE,
    SNS_SOFT_PHOTOSENSITIVITY_IR,
    SNS_SOFT_PHOTOSENSITIVITY_WARMLIGHT,
    SNS_SOFT_PHOTOSENSITIVITY_TYPE_BUTT
} SNS_SOFT_PHOTOSENSITIVITY_TYPE_E;

typedef enum {
    COORDINATION_ALGO_DETECT_MODE_SKEL,
    COORDINATION_ALGO_DETECT_MODE_MD,
    COORDINATION_ALGO_DETECT_MODE_MD_SKEL,
    COORDINATION_ALGO_DETECT_MODE_BUTT
} COORDINATION_ALGO_DETECT_MODE_E;

typedef enum {
    COORDINATION_SKEL_CHECK_TYPE_BODY,
    COORDINATION_SKEL_CHECK_TYPE_VEHICLE,
    COORDINATION_SKEL_CHECK_TYPE_CYCLE,
    COORDINATION_SKEL_CHECK_TYPE_BODY_VEHICLE,
    COORDINATION_SKEL_CHECK_TYPE_BODY_VEHICLE_CYCLE
} COORDINATION_SKEL_CHECK_TYPE_E;

typedef enum {
    COORDINATION_CAM_TYPE_NONE,
    COORDINATION_CAM_TYPE_FIXED,
    COORDINATION_CAM_TYPE_PTZ
} COORDINATION_CAM_TYPE_E;

typedef enum {
    E_SNS_SWITCH_SNS_TYPE_MAIN = 0,
    E_SNS_SWITCH_SNS_TYPE_SUB,
    E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE,
    E_SNS_SWITCH_SNS_TYPE_BUTT
} SNS_SWITCH_SNS_TYPE_E;

typedef struct axSNS_ABILITY_T {
    AX_BOOL bSupportHDR;
    AX_BOOL bSupportHDRSplit;
    AX_F32 fShutterSlowFpsThr;

    axSNS_ABILITY_T(AX_VOID) {
        bSupportHDR = AX_TRUE;
        bSupportHDRSplit = AX_TRUE;
        fShutterSlowFpsThr = -1;
    }
} SNS_ABILITY_T;

typedef struct axSNS_CLK_ATTR_T {
    AX_U8 nSnsClkIdx;
    AX_SNS_CLK_RATE_E eSnsClkRate;

    axSNS_CLK_ATTR_T(AX_VOID) {
        nSnsClkIdx = 0;
        eSnsClkRate = AX_SNS_CLK_24M;
    }
} SNS_CLK_ATTR_T;

typedef struct axISP_ALGO_INFO_T {
    AX_BOOL bActiveAe;
    AX_BOOL bUserAe;
    AX_BOOL bActiveAwb;
    AX_BOOL bUserAwb;
    AX_BOOL bActiveLsc;
    AX_BOOL bUserLsc;
    AX_ISP_AE_REGFUNCS_T tAeFns;
    AX_ISP_AWB_REGFUNCS_T tAwbFns;
    AX_ISP_LSC_REGFUNCS_T tLscFns;

    axISP_ALGO_INFO_T(AX_VOID) {
        memset(this, 0, sizeof(*this));
    }
} ISP_ALGO_INFO_T;

typedef struct _SNS_COLOR_ATTR_T {
    AX_BOOL bColorManual;
    AX_F32 fBrightness;
    AX_F32 fSharpness;
    AX_F32 fContrast;
    AX_F32 fSaturation;

    _SNS_COLOR_ATTR_T() {
        bColorManual = AX_FALSE;
        fBrightness = 0;
        fSharpness = 0;
        fContrast = 0;
        fSaturation = 0;
    }
} SENSOR_COLOR_ATTR_T;

// sensor ldc attribute
typedef struct _SENSOR_LDC_ATTR_T {
    // LDC
    AX_BOOL bLdcEnable;
    AX_BOOL bLdcAspect;
    AX_S16 nLdcXRatio;
    AX_S16 nLdcYRatio;
    AX_S16 nLdcXYRatio;
    AX_S16 nLdcDistortionRatio;

    _SENSOR_LDC_ATTR_T() {
        bLdcEnable = AX_FALSE;
        bLdcAspect = AX_FALSE;
        nLdcXRatio = 0;
        nLdcYRatio = 0;
        nLdcXYRatio = 0;
        nLdcDistortionRatio = 0;
    }
} SENSOR_LDC_ATTR_T;

// sensor DIS attribute
typedef struct _SENSOR_DIS_ATTR_T {
    // DIS
    AX_BOOL bDisEnable;
    AX_BOOL bMotionShare;
    AX_BOOL bMotionEst;
    AX_U8 nDelayFrameNum;
    _SENSOR_DIS_ATTR_T() {
        memset(this, 0, sizeof(*this));
    }
} SENSOR_DIS_ATTR_T;

// sensor ircut attribute
typedef struct _SENSOR_IRCUT_ATTR_T {
    AX_F32 fIrCalibR;
    AX_F32 fIrCalibG;
    AX_F32 fIrCalibB;
    AX_F32 fNight2DayIrStrengthTh;
    AX_F32 fNight2DayIrDetectTh;
    AX_U32 nInitDayNightMode;
    AX_F32 fDay2NightLuxTh;
    AX_F32 fNight2DayLuxTh;
    AX_F32 fNight2DayBrightTh;
    AX_F32 fNight2DayDarkTh;
    AX_F32 fNight2DayUsefullWpRatio;
    AX_U32 nCacheTime;
} SENSOR_IRCUT_ATTR_T;

// sensor warm light attribute
typedef struct _SENSOR_WARMLIGHT_ATTR_T {
    AX_U64 nOnLightSensitivity;
    AX_U64 nOnLightExpValMax;
    AX_U64 nOnLightExpValMid;
    AX_U64 nOnLightExpValMin;
    AX_U64 nOffLightSensitivity;
    AX_U64 nOffLightExpValMax;
    AX_U64 nOffLightExpValMid;
    AX_U64 nOffLightExpValMin;
} SENSOR_WARMLIGHT_ATTR_T;

// sensor soft photo sensitivity
typedef struct _SENSOR_SOFT_PHOTOSENSITIVITY_ATTR_T {
    AX_BOOL bAutoCtrl;
    SNS_SOFT_PHOTOSENSITIVITY_TYPE_E eType;

    union {
        SENSOR_IRCUT_ATTR_T tIrAttr;
        SENSOR_WARMLIGHT_ATTR_T tWarmAttr;
    };

    _SENSOR_SOFT_PHOTOSENSITIVITY_ATTR_T() {
        bAutoCtrl = AX_FALSE;
        eType = SNS_SOFT_PHOTOSENSITIVITY_NONE;
        memset(&tIrAttr, 0x00, sizeof(tIrAttr));
        memset(&tWarmAttr, 0x00, sizeof(tWarmAttr));
    }
} SENSOR_SOFT_PHOTOSENSITIVITY_ATTR_T;

typedef struct _SENSOR_AE_ROI_ATTR_T {
    AX_BOOL bFaceAeRoiEnable;
    AX_BOOL bVehicleAeRoiEnable;

    _SENSOR_AE_ROI_ATTR_T() {
        bFaceAeRoiEnable = AX_FALSE;
        bVehicleAeRoiEnable = AX_FALSE;
    }
} SENSOR_AE_ROI_ATTR_T;

typedef struct _SENSOR_EZOOM_ATTR_T {
    AX_F32 fEZoomRatio;
    AX_F32 fEZoomMaxRatio;
    AX_S16 nCenterOffsetX;  // offset x from horizental center
    AX_S16 nCenterOffsetY;  // offset y from veritcal center

    _SENSOR_EZOOM_ATTR_T() {
        fEZoomRatio = 0.0;
        fEZoomMaxRatio = AX_EZOOM_MAX;
        nCenterOffsetX = 0;
        nCenterOffsetY = 0;
    }
} SENSOR_EZOOM_ATTR_T;

typedef struct _SENSOR_HOTNOISEBALANCE_T {
    AX_BOOL bEnable;
    AX_F32 fBalanceThreshold;
    AX_F32 fNormalThreshold;
    string strSdrHotNoiseNormalModeBin;
    string strSdrHotNoiseBalanceModeBin;
    string strHdrHotNoiseNormalModeBin;
    string strHdrHotNoiseBalanceModeBin;

    _SENSOR_HOTNOISEBALANCE_T() {
        bEnable = AX_FALSE;
        fBalanceThreshold = 0;
        fNormalThreshold = 0;
    }
} SENSOR_HOTNOISEBALANCE_T;

typedef struct _SENSOR_HDR_RATIO_T {
    AX_BOOL bEnable;
    AX_U8 nRatio; // 0: default; 1: 1:1 fusion
    string strHdrRatioDefaultBin;
    string strHdrRatioModeBin;

    _SENSOR_HDR_RATIO_T() {
        bEnable = AX_FALSE;
        nRatio = 0;
    }
} SENSOR_HDR_RATIO_T;

typedef struct _SENSOR_PIPE_MAPPING {
    AX_U8 nPipeSeq;
    AX_U8 nPipeID;

    _SENSOR_PIPE_MAPPING() {
        nPipeSeq = 0;
        nPipeID = 0;
    }
} SENSOR_PIPE_MAPPING_T;

typedef struct _COORDINATION_SKEL_RESULT_T {
    AX_BOOL bBodyFound{AX_FALSE};
    AX_BOOL bVehicleFound{AX_FALSE};
    AX_BOOL bCycleFound{AX_FALSE};
} COORDINATION_SKEL_RESULT_T;

typedef struct _FIXED_PTZ_COORDINATION_INFO_T {
    AX_BOOL bEnable;
    AX_U16  nFixedSnsId;
    AX_U16  nPtzSnsId;
    COORDINATION_ALGO_DETECT_MODE_E eAlgoDectctMode;
    COORDINATION_SKEL_CHECK_TYPE_E  eSkelCheckType;
    AX_U16 nMaxFrameNoShow;  // threshold for lowering frame rate
    AX_F32 fMaxFrameRate;
    AX_F32 fMinFrameRate;
    _FIXED_PTZ_COORDINATION_INFO_T() {
        bEnable = AX_FALSE;
        nFixedSnsId = APP_INVALID_SENSOR_ID;
        nPtzSnsId   = APP_INVALID_SENSOR_ID;
        eAlgoDectctMode = COORDINATION_ALGO_DETECT_MODE_BUTT;
        eSkelCheckType  = COORDINATION_SKEL_CHECK_TYPE_BODY;
        nMaxFrameNoShow = COORDINATION_DETECT_MAX_FRAME_NOSHOW;
        fMaxFrameRate = 15;
        fMinFrameRate = 1;
    }
} FIXED_PTZ_COORDINATION_INFO_T, *FIXED_PTZ_COORDINATION_INFO_PTR;

typedef struct _SENSOR_SWITCH_REG_DATA_T {
    AX_U32 nRegAddr{0};
    AX_U32 nData{0};
} SENSOR_SWITCH_REG_DATA_T;

typedef struct _SENSOR_SWITCH_SUB_SNS_INFO_T {
    AX_U32 nSnsOutWidth;
    AX_U32 nSnsOutHeight;
    AX_U8  nPipeID;
    AX_U8  nBusType;
    AX_U8  nDevNode;
    AX_U8  nI2cAddr;
    AX_U8  nResetGpioNum;
    AX_SENSOR_REGISTER_FUNC_T* pSnsObj;
    AX_VOID* pSwitchSns;
    SENSOR_SWITCH_REG_DATA_T tSwitchRegData;

    _SENSOR_SWITCH_SUB_SNS_INFO_T() {
        nSnsOutWidth = 0;
        nSnsOutHeight = 0;
        nPipeID = APP_INVALID_PIPE_ID;
        nBusType = 0;
        nDevNode = 0;
        nI2cAddr = 0;
        nResetGpioNum = (AX_U8)(-1);
        pSnsObj = nullptr;
        pSwitchSns = nullptr;
    }
} SENSOR_SWITCH_SUB_SNS_INFO_T;

typedef struct _SNS_SINGLE_PIPE_SWITCH_INFO_T {
    AX_U8 nHdlId0;
    AX_U8 nHdlId1;
    AX_U8 nCurrentHdlId;
    AX_U8 nDevNode;
    AX_U8 nI2cAddr;
    AX_U8 nResetGpioNum;
    AX_F32 fSwitchEZoomRatio;  // ezoom ratio threshold to switch sensor
    AX_S16 nCenterOffsetX;     // offset x from horizental center
    AX_S16 nCenterOffsetY;     // offset y from veritcal center
    AX_WIN_AREA_T tSwitchZoomAreaThreshold;
    vector<string> vecTuningBin{};

    _SNS_SINGLE_PIPE_SWITCH_INFO_T() {
       nHdlId0 = 0;
       nHdlId1 = 0;
       nCurrentHdlId = 0;
       nDevNode = 0;
       nI2cAddr = -1;
       nResetGpioNum = 0;
       fSwitchEZoomRatio = -1;
       nCenterOffsetX = 0;
       nCenterOffsetY = 0;
       tSwitchZoomAreaThreshold.nStartX = 0;
       tSwitchZoomAreaThreshold.nStartY = 0;
       tSwitchZoomAreaThreshold.nWidth = 0;
       tSwitchZoomAreaThreshold.nHeight = 0;
    }
} SNS_SINGLE_PIPE_SWITCH_INFO_T;

typedef struct _CHANNEL_CONFIG_T {
    AX_S32 nWidth;
    AX_S32 nHeight;
    AX_U8 nYuvDepth;
    AX_BOOL bChnEnable;
    AX_F32 fFrameRate;
    AX_VIN_FRAME_MODE_E eFrmMode;
    AX_FRAME_COMPRESS_INFO_T tChnCompressInfo;

    _CHANNEL_CONFIG_T() {
        nWidth = 0;
        nHeight = 0;
        nYuvDepth = 0;
        fFrameRate = 0;
        bChnEnable = AX_FALSE;
        eFrmMode = AX_VIN_FRAME_MODE_OFF;
        tChnCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
        tChnCompressInfo.u32CompressLevel = 0;
    }
} CHANNEL_CONFIG_T, *CHANNEL_CONFIG_PTR;

typedef struct _SENSOR_RESOLUTION_T {
    AX_U32 nSnsAttrWidth;
    AX_U32 nSnsAttrHeight;
    AX_U32 nSnsOutWidth;
    AX_U32 nSnsOutHeight;

    _SENSOR_RESOLUTION_T() {
        nSnsAttrWidth = -1;
        nSnsAttrHeight = -1;
        nSnsOutWidth = -1;
        nSnsOutHeight = -1;
    }
} SENSOR_RESOLUTION_T, *SENSOR_RESOLUTION_PTR;

typedef struct _PIPE_CONFIG_T {
    AX_U8 nPipeID;
    AX_F32 fPipeFramerate;
    CHANNEL_CONFIG_T arrChannelAttr[AX_VIN_CHN_ID_MAX];
    SNS_AIISP_MODE_E eAiIspMode;
    AX_FRAME_COMPRESS_INFO_T tIfeCompress[AX_SNS_HDR_MODE_MAX];
    AX_FRAME_COMPRESS_INFO_T tAiNrCompress[AX_SNS_HDR_MODE_MAX];
    AX_FRAME_COMPRESS_INFO_T t3DNrCompress[AX_SNS_HDR_MODE_MAX];
    AX_BOOL bTuning;
    AX_U32 nTuningPort;
    SENSOR_LDC_ATTR_T tLdcAttr;
    SENSOR_DIS_ATTR_T tDisAttr;
    AX_S8 nVinIvpsMode;
    AX_U8 nIvpsGrp;

    vector<string> vecTuningBin{};

    _PIPE_CONFIG_T() {
        nPipeID = 0;
        fPipeFramerate = 30;
        eAiIspMode = E_SNS_AIISP_DEFAULT_SCENE_MODE_E;
        memset(tIfeCompress, 0, sizeof(tIfeCompress));
        memset(tAiNrCompress, 0, sizeof(tAiNrCompress));
        memset(t3DNrCompress, 0, sizeof(t3DNrCompress));
        bTuning = AX_TRUE;
        nTuningPort = 8082;
        nVinIvpsMode = -1;
        nIvpsGrp = 0;
    }
} PIPE_CONFIG_T, *PIPE_CONFIG_PTR;

typedef struct axSNS_LIB_INFO_T {
    std::string strLibName;
    std::string strObjName;
} SNS_LIB_INFO_T;

typedef struct _SENSOR_CONFIG_T {
    AX_U8 nSnsID;
    AX_U8 nDevID;
    AX_U8 nRxDevID;
    AX_U8 nSensorMode;
    AX_BOOL bMirror;
    AX_BOOL bFlip;
    AX_BOOL bLFHdrSupport; // Long-Frame HDR Support
    AX_BOOL bNeedResetSensor;
    SNS_TYPE_E eSensorType;
    AX_ROTATION_E eRotation;
    SNS_HDR_TYPE_E eHdrType;
    AX_SNS_HDR_MODE_E eSensorMode; // for sensor config
    AX_SNS_MASTER_SLAVE_E nMasterSlaveSel;
    AX_DAYNIGHT_MODE_E eDayNight;
    AX_VIN_DEV_MODE_E eDevMode;
    AX_SNS_OUTPUT_MODE_E eSnsOutputMode;
    AX_VIN_IVPS_MODE_E eVinIvpsMode;
    AX_F32 fFrameRate;
    AX_F32 fShutterSlowFpsThr;
    AX_U32 nPipeCount;
    SENSOR_RESOLUTION_T tSnsResolution;
    PIPE_CONFIG_T arrPipeAttr[MAX_PIPE_PER_DEVICE];
    SENSOR_COLOR_ATTR_T tColorAttr;
    SENSOR_AE_ROI_ATTR_T tAeRoiAttr;
    SENSOR_SOFT_PHOTOSENSITIVITY_ATTR_T tSoftPhotoSensitivityAttr;
    SENSOR_EZOOM_ATTR_T tEZoomAttr;
    SENSOR_HOTNOISEBALANCE_T tHotNoiseBalanceAttr;
    SENSOR_HDR_RATIO_T tHdrRatioAttr;
    AX_U8 nResetGpioNum;
    AX_U8 nClkId;
    AX_U8 nLaneNum;
    AX_U8 nBusType;
    AX_U8 nDevNode;
    AX_U8 nI2cAddr;
    AX_U8 nDevNodeSwitch;
    AX_U8 nI2cAddrSwitch;
    AX_S8 arrSettingIndex[3];
    AX_VIN_DEV_WORK_MODE_E arrDevWorkMode[3];
    SNS_SHUTTER_MODE_E eShutterMode;
    AX_U32 nDiscardYUVFrmNum;
    COORDINATION_CAM_TYPE_E eCoordCamType;
    COORDINATION_ALGO_DETECT_MODE_E eCoordAlgoDetMode;
    COORDINATION_SKEL_CHECK_TYPE_E  eSkelCheckType;
    AX_U16 nMaxFrameNoShow;
    SNS_SWITCH_SNS_TYPE_E  eSwitchSnsType;
    SENSOR_SWITCH_REG_DATA_T tSwitchRegData;
    SNS_LIB_INFO_T m_tSnsLibInfo;
    SNS_SINGLE_PIPE_SWITCH_INFO_T tSinglePipeSwitchInfo;

    _SENSOR_CONFIG_T() {
        nSnsID = 0;
        nDevID = 0;
        nRxDevID = 0;
        nSensorMode = SNS_MODE_SDR;
        bMirror = AX_FALSE;
        bFlip = AX_FALSE;
        bLFHdrSupport = AX_FALSE;
        bNeedResetSensor = AX_TRUE;
        eRotation = AX_ROTATION_0;
        fFrameRate = 30;
        fShutterSlowFpsThr = -1;
        eDevMode = AX_VIN_DEV_OFFLINE;
        eSnsOutputMode = AX_SNS_NORMAL;
        eSensorType = E_SNS_TYPE_MAX;
        eHdrType = E_SNS_HDR_TYPE_ClASSICAL;
        eSensorMode = AX_SNS_LINEAR_MODE;
        eDayNight = AX_DAYNIGHT_MODE_DAY;
        nMasterSlaveSel = AX_SNS_MASTER;
        nPipeCount = 0;
        nResetGpioNum = 0;
        nClkId = 0;
        nLaneNum = 0;
        nBusType = 0; // i2c
        nDevNode = 0;
        nI2cAddr = -1;
        nDevNodeSwitch = 0;
        nI2cAddrSwitch = -1;
        eVinIvpsMode = AX_GDC_ONLINE_VPP;
        eShutterMode = E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E;
        nDiscardYUVFrmNum = 0;
        eCoordCamType = COORDINATION_CAM_TYPE_NONE;
        eCoordAlgoDetMode = COORDINATION_ALGO_DETECT_MODE_BUTT;
        eSkelCheckType = COORDINATION_SKEL_CHECK_TYPE_BODY;
        nMaxFrameNoShow = COORDINATION_DETECT_MAX_FRAME_NOSHOW;
        eSwitchSnsType = E_SNS_SWITCH_SNS_TYPE_BUTT;
        arrDevWorkMode[0] = AX_VIN_DEV_WORK_MODE_1MULTIPLEX;
        arrDevWorkMode[1] = AX_VIN_DEV_WORK_MODE_1MULTIPLEX;
        arrDevWorkMode[2] = AX_VIN_DEV_WORK_MODE_1MULTIPLEX;
    }
} SENSOR_CONFIG_T, *SENSOR_CONFIG_PTR;

class ISensor {
public:
    virtual ~ISensor(AX_VOID) = default;

    /*
        @brief: initialize and lanuch ISP pipe line
    */
    virtual AX_BOOL Open(AX_VOID) = 0;
    virtual AX_BOOL Close(AX_VOID) = 0;

    /*
        @brief: ISP algorithm including AE, AF, AWB, LSC ...
                by default, AE and AWB is actived with Axera algorithm.
    */
    virtual AX_VOID RegisterIspAlgo(const ISP_ALGO_INFO_T& tAlg) = 0;

    /*
        @brief: sensor and ISP properties set and get
                NOTE: property should be set before Start()
     */
    virtual const AX_SNS_ATTR_T& GetSnsAttr(AX_VOID) const = 0;
    virtual AX_VOID SetSnsAttr(const AX_SNS_ATTR_T& tSnsAttr) = 0;

    virtual const SNS_CLK_ATTR_T& GetSnsClkAttr(AX_VOID) const = 0;
    virtual AX_VOID SetSnsClkAttr(const SNS_CLK_ATTR_T& tClkAttr) = 0;

    virtual const AX_VIN_DEV_ATTR_T& GetDevAttr(AX_VOID) const = 0;
    virtual AX_VOID SetDevAttr(const AX_VIN_DEV_ATTR_T& tDevAttr) = 0;

    virtual const AX_MIPI_RX_DEV_T& GetMipiRxAttr(AX_VOID) const = 0;
    virtual AX_VOID SetMipiRxAttr(const AX_MIPI_RX_DEV_T& tMipiRxAttr) = 0;

    virtual const AX_VIN_PIPE_ATTR_T& GetPipeAttr(AX_U8 nPipe) const = 0;
    virtual AX_VOID SetPipeAttr(AX_U8 nPipe, const AX_VIN_PIPE_ATTR_T& tPipeAttr) = 0;

    virtual const AX_VIN_CHN_ATTR_T& GetChnAttr(AX_U8 nPipe, AX_U8 nChannel) const = 0;
    virtual AX_VOID SetChnAttr(AX_U8 nPipe, AX_U8 nChannel, const AX_VIN_CHN_ATTR_T& tChnAttr) = 0;

    virtual const SNS_ABILITY_T& GetAbilities(AX_VOID) const = 0;
};
