/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_ELECAM_H_
#define _AX_OPAL_MAL_ELECAM_H_

#include "ax_opal_mal_def.h"

#define AX_OPAL_MAX_NAME_SIZE       (64)
#define AX_OPAL_MAX_BINPATH_SIZE    (256)
#define AX_OPAL_MAX_BINPATH_CNT     (2) // 0: SDR bin; 1: HDR bin

typedef enum {
    AX_OPAL_RAW_TYPE_NONE    = 0,
    AX_OPAL_RAW_TYPE_RAW10   = 10,       // raw10, 10-bit per pixel
    AX_OPAL_RAW_TYPE_RAW12   = 12,       // raw12, 12-bit per pixel
    AX_OPAL_RAW_TYPE_BUTT
} AX_OPAL_RAW_TYPE_E;

typedef enum {
    AX_OPAL_BAYER_PATTERN_RGGB = 0,        // R Gr Gb B bayer pattern
    AX_OPAL_BAYER_PATTERN_BUTT
} AX_OPAL_BAYER_PATTERN_E;

typedef enum {
    AX_OPAL_INPUT_MODE_MIPI = 0,
    AX_OPAL_INPUT_MODE_BUTT
} AX_OPAL_INPUT_MODE_E;

typedef enum {
    AX_OPAL_MIPI_PHY_TYPE_DPHY = 0,
    AX_OPAL_MIPI_PHY_TYPE_BUTT,
} AX_OPAL_MIPI_PHY_TYPE_E;

typedef enum {
    AX_OPAL_FORMAT_INVALID = -1,
    /* YUV420 8 bit */
    AX_OPAL_FORMAT_YUV420_SEMIPLANAR         = 3,        /* 0x03 */ /* YYYY... UVUVUV... NV12 */
    /* BAYER RAW */
    AX_OPAL_FORMAT_BAYER_RAW_10BPP_PACKED    = 133,      /* 0x85 */
    AX_OPAL_FORMAT_BAYER_RAW_12BPP_PACKED    = 134,      /* 0x86 */
    AX_OPAL_FORMAT_BUTT
} AX_OPAL_IMG_FORMAT_E;

typedef struct _AX_OPAL_CAM_SENSOR_CFG_T {
    AX_CHAR cObjName[AX_OPAL_MAX_NAME_SIZE];
    AX_CHAR cLibName[AX_OPAL_MAX_NAME_SIZE];
    AX_S32 nWidth;
    AX_S32 nHeight;
    AX_F32 fFrameRate;
    AX_OPAL_SNS_MODE_E eSnsMode;
    AX_OPAL_RAW_TYPE_E eRawType;
    AX_OPAL_BAYER_PATTERN_E eBayerPattern;
    AX_S32 bTestPatternEnable;
    AX_S32 eMasterSlaveSel;
    AX_S32 nSettingIndex[2]; // SDR/HDR
    AX_S32 nBusType;
    AX_S32 nDevNode;
    AX_S32 nI2cAddr;
    AX_S32 nSnsClkIdx;
    AX_S32 eSnsClkRate;
} AX_OPAL_CAM_SENSOR_CFG_T;

typedef struct _AXOP_CAM_MIPI_CFG_T {
    AX_S32 nResetGpio;
    AX_S32 eLaneComboMode;
    AX_S32 nMipiRxID;
    AX_OPAL_INPUT_MODE_E eInputMode;
    AX_OPAL_MIPI_PHY_TYPE_E ePhyMode;
    AX_S32 eLaneNum;
    AX_S32 nDataRate;
    AX_S32 nDataLaneMap0;
    AX_S32 nDataLaneMap1;
    AX_S32 nDataLaneMap2;
    AX_S32 nDataLaneMap3;
    AX_S32 nClkLane0;
    AX_S32 nClkLane1;
} AX_OPAL_CAM_MIPI_CFG_T;

typedef struct _AXOP_CAM_DEV_CFG_T {
    AX_OPAL_INPUT_MODE_E eSnsIntfType;
    AX_S32 tDevImgRgnStartX;
    AX_S32 tDevImgRgnStartY;
    AX_S32 tDevImgRgnWidth;
    AX_S32 tDevImgRgnHeight;
    AX_OPAL_IMG_FORMAT_E ePixelFmt;
    AX_OPAL_BAYER_PATTERN_E eBayerPattern;
    AX_S32 eSkipFrame;
    AX_OPAL_SNS_MODE_E eSnsMode;
    AX_S32 eDevMode;
    AX_S32 bImgDataEnable;
    AX_S32 bNonImgEnable;
    AX_S32 tMipiIntfAttrImgVc;
    AX_S32 tMipiIntfAttrImgDt;
    AX_S32 tMipiIntfAttrInfoVc;
    AX_S32 tMipiIntfAttrInfoDt;
    AX_S32 eSnsOutputMode;
} AX_OPAL_CAM_DEV_CFG_T;

typedef struct _AXOP_CAM_ISP_CFG_T {
    AX_S32 nBinCnt;
    AX_CHAR cBinPathName[AX_OPAL_MAX_BINPATH_SIZE][AX_OPAL_MAX_BINPATH_CNT];
} AX_OPAL_CAM_ISP_CFG_T;

typedef struct _AX_OPAL_CAM_NT_CFG_T {
    AX_S32 bEnable;
    AX_S32 nStreamPort;
    AX_S32 nCtrlPort;
} AX_OPAL_CAM_NT_CFG_T;

typedef struct _AX_OPAL_CAM_PIPE_CFG_T {
    AX_S32 nWidth;
    AX_S32 nHeight;
    AX_S32 nWidthStride;
    AX_OPAL_BAYER_PATTERN_E eBayerPattern;
    AX_OPAL_IMG_FORMAT_E ePixelFmt;
    AX_OPAL_SNS_MODE_E eSnsMode;
    AX_S32 eWorkMode;
    AX_S32 bAiIspEnable;
    AX_S32 enCompressMode;
    AX_S32 bHMirror;
    AX_S32 b3DnrIsCompress;
    AX_S32 bAinrIsCompress;
    AX_F32 fSrcFrameRate;
    AX_F32 fDstFrameRate;
    AX_S32 nLoadRawNode;
    AX_S32 nDumpNodeMask;
    AX_S32 nScalerRatio;
    AX_S32 bMotionEst;
    AX_S32 eVinIvpsMode;
    AX_S32 bMotionShare;
    AX_S32 bMotionComp;

    AX_OPAL_CAM_ISP_CFG_T stCamIspCfg;
    AX_OPAL_CAM_NT_CFG_T stCamNTCfg;
} AX_OPAL_CAM_PIPE_CFG_T;

typedef struct _AXOP_CAM_CHN_CFG_T {
    AX_S32 nWidth;
    AX_S32 nHeight;
    AX_OPAL_IMG_FORMAT_E ePixelFmt;
    AX_S32 bEnable;
    AX_S32 nWidthStride;
    AX_S32 nDepth;
    AX_S32 bFlip;
    AX_S32 enCompressMode;
    AX_S32 u32CompressLevel;
} AX_OPAL_CAM_CHN_CFG_T;

typedef struct _AX_OPAL_CAM_CFG_T {
    AX_OPAL_CAM_SENSOR_CFG_T tSensorCfg;
    AX_OPAL_CAM_MIPI_CFG_T tMipiCfg;
    AX_OPAL_CAM_DEV_CFG_T tDevCfg;
    AX_OPAL_CAM_PIPE_CFG_T tPipeCfg;
    // AX_OPAL_CAM_ISP_CFG_T tIspCfg;
    AX_OPAL_CAM_CHN_CFG_T tChnCfg;
} AX_OPAL_CAM_CFG_T;

typedef struct {
    AX_VOID* pSensorLib;
    AX_VOID* pSensorHandle;
} AX_OPAL_MAL_SNSLIB_ATTR_T;

typedef struct _AX_OPAL_MAL_CAM_ATTR_T {
    AX_OPAL_CAM_CFG_T tCamCfg;
    AX_OPAL_MAL_SNSLIB_ATTR_T tSnsLibAttr;
} AX_OPAL_MAL_CAM_ATTR_T;

typedef struct _AX_OPAL_MAL_CAM_ID_T {
    AX_S32 nRxDevId;
    AX_S32 nDevId;
    AX_S32 nPipeId;
    AX_S32 nChnId;
} AX_OPAL_MAL_CAM_ID_T;

typedef struct _AX_OPAL_MAL_ELECAM_T {
    AX_OPAL_MAL_ELE_T stBase;
    AX_OPAL_MAL_CAM_ATTR_T stAttr;
    AX_OPAL_MAL_CAM_ID_T stId;
} AX_OPAL_MAL_ELECAM_T;

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELECAM_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr);
AX_S32 AX_OPAL_MAL_ELECAM_Destroy(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELECAM_Start(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELECAM_Stop(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELECAM_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);

#endif // _AX_OPAL_MAL_ELECAM_H_