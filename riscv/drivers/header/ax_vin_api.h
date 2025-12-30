/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_VIN_API_H__
#define __AX_VIN_API_H__

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_isp_common.h"
// #include "ax_pool_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define AX_SHUTTER_SEQ_NUM                                  (10)

typedef enum _AX_VIN_DEV_MODE_E_ {
    AX_VIN_DEV_MODE_INVALID = -1,
    AX_VIN_DEV_ONLINE       = 0,
    AX_VIN_DEV_OFFLINE      = 1,
    AX_VIN_DEV_MODE_MAX
} AX_VIN_DEV_MODE_E;

typedef enum _AX_VIN_DEV_WORK_MODE_E_ {
    AX_VIN_DEV_WORK_MODE_1MULTIPLEX         = 0,
    AX_VIN_DEV_WORK_MODE_2MULTIPLEX         = 1,
    AX_VIN_DEV_WORK_MODE_3MULTIPLEX         = 2,
    AX_VIN_DEV_WORK_MODE_4MULTIPLEX         = 3,
    AX_VIN_DEV_WORK_MODE_MAX,
} AX_VIN_DEV_WORK_MODE_E;

typedef enum _AX_VIN_PIPE_WORK_MODE_E_ {
    AX_VIN_PIPE_NORMAL_MODE0                 = 0,    /* normal mode: One dev matches one pipe */
    AX_VIN_PIPE_NORMAL_MODE1                 = 1,    /* normal mode: One dev matches one pipe */
    AX_VIN_PIPE_FUSION_MODE                  = 2,
    AX_VIN_PIPE_ISP_BYPASS_MODE              = 3,
    AX_VIN_PIPE_YUV_MODE                     = 4
} AX_VIN_PIPE_WORK_MODE_E;

typedef enum _AX_VIN_CHN_ID_ {
    AX_VIN_CHN_ID_INVALID                = -1,
    AX_VIN_CHN_ID_MAIN                   = 0,   /* main path */
    AX_VIN_CHN_ID_MAX
} AX_VIN_CHN_ID_E;

typedef enum {
    AX_ISP_FUSION_NONE = 0,
    AX_ISP_FUSION_DUAL_VISION,
} AX_FUSION_MODE_E;

typedef struct _AX_FRAME_EXP_INFO_T_ {
    AX_U32 nIntTime;                            /* ExposeTime(us). Accuracy: U32 Range: [0x0, 0xFFFFFFFF] */
    AX_U32 nAgain;                              /* Total Again value. Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]
                                                 * Total Again = Sensor Register Again x HCG Ratio
                                                 * LCG Mode: HCG Ratio = 1.0
                                                 * HCG Mode: HCG Ratio = Refer to Sensor Spec */
    AX_U32 nDgain;                              /* Sensor Dgain value. Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]
                                                 * Not Used, should be set to 0x400. AX Platform Use ISP DGain */
    AX_U32 nIspGain;                            /* ISP Dgain value. Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF] */
    AX_U32 nTotalGain;                          /* Total Gain value. Accuracy: U22.10 Range: [0x400, 0xFFFFFFFF]
                                                 * Total Gain value = SensorRegisterAgain * SensorDgain * CurrHcgRatio * IspGain */
    AX_U32 nHcgLcgMode;                         /* 0:HCG 1:LCG 2:Not Support */
    AX_U32 nHcgLcgRatio;                        /* Accuracy: U10.10 Range: [0x400, 0x2800] */
    AX_U32 nHdrRatio;                           /* Accuracy: U7.10 Range: [0x400, 0x1FC00] */
    AX_U32 nLux;                                /* Accuracy: U22.10 Range: [0, 0xFFFFFFFF]
                                                 * fLux = (MeanLuma*LuxK) / (AGain*Dgain*IspGain)
                                                 * where LuxK is a calibrated factor */
} AX_FRAME_EXP_INFO_T;

typedef struct _AX_ISP_FRAME_T_ {
    AX_RAW_TYPE_E       eRawType;
    AX_SNS_HDR_MODE_E   eHdrMode;
    AX_SNS_HDR_FRAME_E  nHdrFrame;
    AX_BAYER_PATTERN_E  eBayerPattern;
    AX_FRAME_EXP_INFO_T tExpInfo;
} AX_ISP_FRAME_T;

typedef struct _AX_IMG_INFO_T_ {
    AX_VIDEO_FRAME_INFO_T   tFrameInfo;
    AX_ISP_FRAME_T          tIspInfo;
} AX_IMG_INFO_T;

typedef enum _AX_VIN_FRAME_SOURCE_TYPE_E_ {
    AX_VIN_FRAME_SOURCE_TYPE_MIN  = -1,
    AX_VIN_FRAME_SOURCE_TYPE_DEV  = 0,
    AX_VIN_FRAME_SOURCE_TYPE_USER = 1,
    AX_VIN_FRAME_SOURCE_TYPE_MAX
} AX_VIN_FRAME_SOURCE_TYPE_E;

typedef enum _AX_VIN_FRAME_SOURCE_ID_E_ {
    AX_VIN_FRAME_SOURCE_ID_MIN  = -1,
    AX_VIN_FRAME_SOURCE_ID_IFE  = 1,
    AX_VIN_FRAME_SOURCE_ID_ITP  = 2,
    AX_VIN_FRAME_SOURCE_ID_YUV  = 3,
    AX_VIN_FRAME_SOURCE_ID_MAX
} AX_VIN_FRAME_SOURCE_ID_E;

typedef enum {
    AX_VIN_DEV_IRQ_SNS_FSOF     = 0,
    AX_VIN_DEV_IRQ_SNS_FEOF     = 1,    /* Currently not support */
    AX_VIN_DEV_IRQ_MAX,
} AX_VIN_DEV_IRQ_TYPE_E;

/* Sequence of YUV data */
typedef enum _AX_VIN_YUV_DATA_SEQ_E_ {
    AX_VIN_DATA_SEQ_VUVU = 0,   /* The input sequence of the second component(only contains u and v) in BT.1120 mode is VUVU */
    AX_VIN_DATA_SEQ_UVUV,       /* The input sequence of the second component(only contains u and v) in BT.1120 mode is UVUV */

    AX_VIN_DATA_SEQ_UYVY,       /* The input sequence of YUV is UYVY */
    AX_VIN_DATA_SEQ_VYUY,       /* The input sequence of YUV is VYUY */
    AX_VIN_DATA_SEQ_YUYV,       /* The input sequence of YUV is YUYV */
    AX_VIN_DATA_SEQ_YVYU,       /* The input sequence of YUV is YVYU */

    AX_VIN_DATA_SEQ_MAX
} AX_VIN_YUV_DATA_SEQ_E;

/* Clock edge mode */
typedef enum _AX_VIN_CLK_EDGE_E_ {
    AX_VIN_CLK_EDGE_SINGLE_UP = 0,         /* single-edge mode: rising edge */
    AX_VIN_CLK_EDGE_SINGLE_DOWN,           /* single-edge mode: falling edge */
    AX_VIN_CLK_EDGE_MAX
} AX_VIN_CLK_EDGE_E;

typedef enum _AX_VIN_SCAN_MODE_E_ {
    AX_VIN_SCAN_INTERLACED  = 0,        /* interlaced mode */
    AX_VIN_SCAN_PROGRESSIVE,            /* progressive mode */
    AX_VIN_SCAN_MAX
} AX_VIN_SCAN_MODE_E;

/* Polarity of the horizontal synchronization signal */
typedef enum _AX_VIN_HSYNC_POLARITY_E_ {
    AX_VIN_SYNC_POLARITY_HIGH = 0,        /* the valid horizontal/vertical synchronization signal is high-level */
    AX_VIN_SYNC_POLARITY_LOW,             /* the valid horizontal/vertical synchronization signal is low-level */
    AX_VIN_SYNC_POLARITY_MAX
} AX_VIN_SYNC_POLARITY_E;

typedef enum _AX_VIN_LVDS_SYNC_MODE_E_ {
    AX_VIN_LVDS_SYNC_MODE_SAV = 0,          /* SAV, EAV */
    AX_VIN_LVDS_SYNC_MODE_SOF,              /* sensor SOL, EOL, SOF, EOF */
    AX_VIN_LVDS_SYNC_MODE_MAX
} AX_VIN_LVDS_SYNC_MODE_E;

typedef enum _AX_VIN_LVDS_BIT_ENDIAN_E_ {
    AX_VIN_LVDS_ENDIAN_LITTLE  = 0,
    AX_VIN_LVDS_ENDIAN_BIG     = 1,
    AX_VIN_LVDS_ENDIAN_MAX
} AX_VIN_LVDS_BIT_ENDIAN_E;

typedef enum _AX_VIN_LINE_INFO_MODE_ {
    AX_VIN_LI_MODE_VC                   = 0,
    AX_VIN_LI_MODE_LINE_INFO_4PIXEL     = 1,
    AX_VIN_LI_MODE_LINE_INFO_8PIXEL     = 2,
    AX_VIN_LI_MODE_MAX
} AX_VIN_LINE_INFO_MODE_E;

typedef enum _AX_VIN_LVDS_SYNC_CODE_E_ {
    AX_VIN_LVDS_SYNC_MODE_SAV_EAV_NORMAL                    = 0,
    AX_VIN_LVDS_SYNC_MODE_SAV_EAV_LINE_INFO_MODE            = 1,
    AX_VIN_LVDS_SYNC_MODE_SAV_EAV_SYNC_CODE_INTERLEAVE_MODE = 2,
    AX_VIN_LVDS_SYNC_MODE_SOF_EOF_NORMAL                    = 3,
    AX_VIN_LVDS_SYNC_MODE_SOF_EOF_HDR_MODE                  = 4,
    AX_VIN_LVDS_SYNC_MODE_HISPI_PACKETIZED_SP_NORMAL_MODE   = 5,
    AX_VIN_LVDS_SYNC_MODE_HISPI_PACKETIZED_SP_HDR_MODE      = 6,
    AX_VIN_LVDS_SYNC_MODE_HISPI_STREAMING_SP_NORMAL_MODE    = 7,
    AX_VIN_LVDS_SYNC_MODE_HISPI_STREAMING_SP_HDR_MODE       = 8,
} AX_VIN_LVDS_SYNC_CODE_E;

typedef enum _AX_VIN_DVP_DATA_PIN_SELECT_E_ {
    AX_VIN_DVP_DATA_PIN_START_0 = 0,     /*7 8 10 12 14*/
    AX_VIN_DVP_DATA_PIN_START_2 = 1,     /*7 8 10 12*/
    AX_VIN_DVP_DATA_PIN_START_4 = 2,     /*7 8 10*/
    AX_VIN_DVP_DATA_PIN_START_6 = 3,     /*7 8*/
    AX_VIN_DVP_DATA_PIN_START_8 = 4,     /*7*/
} AX_VIN_DVP_DATA_PIN_SELECT_E;

typedef enum _AX_VIN_SYNC_SIGNAL_MODE_E_ {
    AX_VIN_SYNC_SIGNAL_MODE_HIGH_LEVEL = 0,          /* the synchronization signal is high-level valid */
    AX_VIN_SYNC_SIGNAL_MODE_RISING_EDGE,             /* the synchronization signal is rising-edge valid */
    AX_VIN_SYNC_SIGNAL_MODE_MAX
} AX_VIN_SYNC_SIGNAL_MODE_E;

/* synchronization information about the BT or DVP timing */
typedef struct _AX_VIN_SYNC_CFG_T_ {
    AX_VIN_SYNC_SIGNAL_MODE_E   eHsyncSignalMode;  /* the valid synchronization Mode Ctrl */
    AX_VIN_SYNC_POLARITY_E      eVsyncInv;         /* the valid vertical synchronization signal polarity */
    AX_VIN_SYNC_POLARITY_E      eHsyncInv;         /* the valid horizontal synchronization signal polarity */
} AX_VIN_SYNC_CFG_T;

typedef enum _AX_VIN_BT_TDM_MODE_E_ {
    AX_VIN_BT_TDM_MODE_1CH        = 0,        /* tdm mode 1 channel */
    AX_VIN_BT_TDM_MODE_2CH        = 1,        /* tdm mode 2 channel */
    AX_VIN_BT_TDM_MODE_3CH        = 2,        /* tdm mode 3 channel */
    AX_VIN_BT_TDM_MODE_4CH        = 3,        /* tdm mode 4 channel */
} AX_VIN_BT_TDM_MODE_E;

typedef enum _AX_VIN_ROTATION_ {
    AX_VIN_ROTATION_0   = 0,
    AX_VIN_ROTATION_90  = 1,
    AX_VIN_ROTATION_180 = 2,
    AX_VIN_ROTATION_270 = 3,
    AX_VIN_ROTATION_BUTT
} AX_VIN_ROTATION_E;

typedef enum _AX_VIN_COORD_ {
    AX_VIN_COORD_ABS = 0,
    AX_VIN_COORD_RATIO,           /* in ratio mode: nX,nY：[0, 999]; nW,nH:[1, 1000] */
    AX_VIN_COORD_BUTT
} AX_VIN_COORD_E;

/* Blank information of the input timing */
typedef struct _AX_VIN_INTF_TIMING_BLANK_T_ {
    AX_U32 nHsyncHfb;       /* Horizontal front blanking width, Unit: cycle */
    AX_U32 nHsyncHact;      /* Horizontal effetive width */
    AX_U32 nHsyncHbb;       /* Horizontal back blanking width, Unit: cycle */
    AX_U32 nVsyncVfb;       /* Vertical front blanking height of one frame, Unit: line */
    AX_U32 nVsyncVact;      /* Vertical effetive width of one frame */
    AX_U32 nVsyncVbb;       /* Vertical back blanking height of one frame, Unit: line */
} AX_VIN_INTF_TIMING_BLANK_T;

typedef enum _AX_VIN_BT_CTRL_MODE_E_ {
    AX_VIN_BT_CTRL_SYNC_CODE            = 0,        /* internal sync: Controlled by sync code */
    AX_VIN_BT_CTRL_VSYNC_HSYNC          = 1,        /* external sync: input control signal (vsync/hsync) enable */
} AX_VIN_BT_CTRL_MODE_E;

typedef enum _AX_VIN_INTF_DATA_CHN_MODE_E_ {
    AX_VIN_INTF_DATA_MODE_SINGLE_CHN    = 0,        /* Y and UV are transmitted alternately on the same data line */
    AX_VIN_INTF_DATA_MODE_DUAL_CHN      = 1,        /* Y and UV are transmitted on different data lines */
} AX_VIN_INTF_DATA_CHN_MODE_E;

typedef enum _AX_VIN_BT_SYNC_HEADER_TYPE_E_ {
    AX_VIN_BT_SYNC_HEADER_4_BYTE        = 0,        /* 4byte sync code: 0xFF 0x00  0x00  0xXY */
    AX_VIN_BT_SYNC_HEADER_8_BYTE        = 1,        /* 8byte sync code: 0xFF 0xFF 0x00 0x00 0x00 0x00 0xXY 0xXY */
} AX_VIN_BT_SYNC_HEADER_TYPE_E;


typedef enum _AX_VIN_SYNC_TRIGGER_MODE_E_ {
    AX_VIN_SYNC_OUTSIDE_ELEC_TRIGGER,
    AX_VIN_SYNC_INSIDE_COUNTER_TRIGGER,             /* not support */
} AX_VIN_SYNC_TRIGGER_MODE_E;

typedef enum _AX_VIN_SYNC_SHUTTER_TPYE_E_ {
    AX_VIN_SYNC_SHUTTER_NORMAL              = 0,
    AX_VIN_SYNC_SHUTTER_USER_FRAME_RATE     = 1,    /* use sync flash */
    AX_VIN_SYNC_SHUTTER_MAX,
} AX_VIN_SYNC_SHUTTER_TPYE_E;

typedef enum _AX_VIN_SHUTTER_MODE_E_ {
    AX_VIN_SHUTTER_MODE_VIDEO,
    AX_VIN_SHUTTER_MODE_PICTURE,
    AX_VIN_SHUTTER_MODE_FLASH_SNAP,      /* pipe shutter mode */
    AX_VIN_SHUTTER_MODE_MAX,
} AX_VIN_SHUTTER_MODE_E;

typedef struct _AX_VIN_LIGHT_SHUTTER_PARAM_T_ {
    AX_U32                          nShutterSeq;
    AX_VIN_SHUTTER_MODE_E           eShutterMode;
} AX_VIN_LIGHT_SHUTTER_PARAM_T;

typedef struct _AX_VIN_LIGHT_SYNC_INFO_T_ {
    AX_U32                          nVts;
    AX_U32                          nHts;
    AX_F32                          fFrameRate;
    AX_U32                          nIntTime;                           /* exposure time, unit: us */
    AX_U32                          nVfbTime;                           /* Vertical front blanking time, Unit: us */
    AX_VIN_LIGHT_SHUTTER_PARAM_T    szShutterParam[AX_SHUTTER_SEQ_NUM]; /* pipe id shutter seq */
} AX_VIN_LIGHT_SYNC_INFO_T;

typedef struct _AX_VIN_SYNC_SIGNAL_ATTR_T_ {
    AX_U32                      nSyncIdx;
    AX_VIN_SYNC_POLARITY_E      eSyncInv;           /* the valid sync signal polarity */
    AX_U32                      nSyncFreq;          /* the sync signal frequency */
    AX_U32                      nSyncDutyRatio;     /* the sync signal duty ratio */
} AX_VIN_SYNC_SIGNAL_ATTR_T;

typedef struct _AX_VIN_FLASH_LIGHT_SYNC_ATTR_T_ {
    AX_U32                      nFlashIdx;
    AX_VIN_SYNC_POLARITY_E      eFlashSyncInv;      /* the flash sync signal polarity */
    AX_U32                      nFlashDutyTime;     /* the active line */
    AX_U32                      nFlashDelayTime;    /* the delay line */
} AX_VIN_FLASH_LIGHT_SYNC_ATTR_T;

typedef struct _AX_VIN_STROBE_LIGHT_SYNC_ATTR_T_ {
    AX_U32                      nStrobeIdx;
    AX_VIN_SYNC_POLARITY_E      eStrobeSyncInv;     /* the strobe sync signal polarity */
    AX_U32                      nStrobeDutyTime;    /* the active line */
    AX_U32                      nStrobeDelayTime;   /* the delay line */
} AX_VIN_STROBE_LIGHT_SYNC_ATTR_T;

typedef struct _AX_VIN_BT_SYNC_CODE_CFG_T_ {
    AX_U8 nVldSav;
    AX_U8 nInvSav;
    AX_U8 nVldSavMask;
    AX_U8 nInvSavMask;
} AX_VIN_BT_SYNC_CODE_CFG_T;

typedef struct _AX_VIN_DEV_BT_ATTR_T_ {
    AX_VIN_INTF_DATA_CHN_MODE_E     eDataChnSel;                                  /* input data channel select */
    AX_VIN_BT_CTRL_MODE_E           eSyncCtrlMode;                                /* sync mode: internal sync and external sync */
    AX_VIN_SYNC_CFG_T               tSyncCfg;                                     /* configuration is only required for external sync */
    AX_VIN_INTF_TIMING_BLANK_T      tTimingBlank;                                 /* configuration is only required for external sync */
    AX_VIN_BT_SYNC_HEADER_TYPE_E    eSyncHeaderType;                              /* 0: single header, 1:double header */
    AX_U8                           nDataActiveMode;                              /* 0: 8bit data in [9:2], 1: 8bit data in [7:0] */
    AX_U8                           nYcOrder;                                     /* 0: c first, 1: y first */
    AX_VIN_BT_SYNC_CODE_CFG_T       tBtSyncCode[AX_VIN_SYNC_CODE_NUM];            /* sensor bt sync code cfg*/
    AX_S8                           nBtDataMux[AX_BT_DATA_MUX_NUM];               /* bt data mux sel configure */
    AX_S8                           nBtVsyncMux;                                  /* bt vsync mux sel configure */
    AX_S8                           nBtHsyncMux;                                  /* bt hsync mux sel configure */
    AX_VIN_BT_TDM_MODE_E            eBtTdmMode;                                   /* 0: normal mode ,1: tdm mode 2 channel,2: tdm mode 3 channel,3: tdm mode 4 channel */
} AX_VIN_DEV_BT_ATTR_T;

typedef struct _AX_VIN_DEV_DVP_ATTR_T_ {
    AX_VIN_DVP_DATA_PIN_SELECT_E    eLaneMapMode;
    AX_VIN_SYNC_CFG_T               tSyncCfg;
    AX_S8                           nDvpDataMux[AX_DVP_DATA_MUX_NUM];
    AX_S8                           nDvpVsyncMux;
    AX_S8                           nDvpHsyncMux;
} AX_VIN_DEV_DVP_ATTR_T;

typedef struct _AX_VIN_LINE_INFO_ATTR_T_ {
    AX_VIN_LINE_INFO_MODE_E     eLineInfoMode;
    AX_U16                      szLineMask[AX_HDR_CHN_NUM];
    AX_U16                      szLineSet[AX_HDR_CHN_NUM];
} AX_VIN_LINE_INFO_ATTR_T;

typedef struct _AX_VIN_DEV_LVDS_ATTR_T_ {
    AX_VIN_LINE_INFO_ATTR_T         tLineInfoAttr;
    AX_VIN_LVDS_SYNC_CODE_E         eSyncMode;
    AX_U8                           nLineNum;               /* 2: 2ch; 4: 4ch; 8:8ch; 10:10ch; 12: 12ch; 16: 16ch */
    AX_U8
    nContSyncCodeMatchEn;   /* 0: seek sync code only in first frame, 1: seek sync code in every frame */
    AX_U8                           nHispiFirCodeEn;
    AX_U32                          nInfoHeight;

    /* each vc has 4 params, sync code[i]:
       sync mode is SYNC_MODE_SOF: SOF, EOF, SOL, EOL
       sync mode is SYNC_MODE_SAV: invalid sav, invalid eav, valid sav, valid eav  */
    AX_U16                          szSyncCode[AX_VIN_LVDS_LANE_NUM][AX_HDR_CHN_NUM][AX_VIN_SYNC_CODE_NUM];
} AX_VIN_DEV_LVDS_ATTR_T;

typedef struct _AX_VIN_DEV_MIPI_ATTR_T_ {
    AX_U8                   bImgFilterEn;
    AX_U8                   szImgVc[AX_HDR_CHN_NUM];
    AX_U8                   szImgDt[AX_HDR_CHN_NUM];
    AX_U8                   bInfoFilterEn;
    AX_U8                   szInfoVc[AX_HDR_CHN_NUM];
    AX_U8                   szInfoDt[AX_HDR_CHN_NUM];
    AX_VIN_LINE_INFO_ATTR_T tLineInfoAttr;
} AX_VIN_DEV_MIPI_ATTR_T;

typedef enum {
    AX_SNS_INTF_TYPE_INVALID                 = -1,
    AX_SNS_INTF_TYPE_MIPI_RAW                = 0,        /* MIPI sensor(data type: raw) */
    AX_SNS_INTF_TYPE_MIPI_YUV                = 1,        /* MIPI sensor(data type: yuv) */
    AX_SNS_INTF_TYPE_SUB_LVDS                = 2,        /* Sub-LVDS sensor */
    AX_SNS_INTF_TYPE_DVP                     = 3,        /* DVP sensor */
    AX_SNS_INTF_TYPE_HISPI                   = 4,        /* HiSpi sensor */
    AX_SNS_INTF_TYPE_BT601                   = 5,        /* BT.601 sensor */
    AX_SNS_INTF_TYPE_BT656                   = 6,        /* BT.656 sensor */
    AX_SNS_INTF_TYPE_BT1120                  = 7,        /* BT.1120 sensor */
    AX_SNS_INTF_TYPE_TPG                     = 8,        /* ISP TPG test pattern */
    AX_SNS_INTF_TYPE_MAX
} AX_SNS_INTF_TYPE_E;

typedef struct _AX_VIN_DEV_ATTR_T_ {
    AX_VIN_DEV_MODE_E                       eDevMode;
    AX_VIN_DEV_WORK_MODE_E                  eDevWorkMode;
    AX_BOOL                                 bImgDataEnable;     /* 1: image data enable, 0: disable */
    AX_BOOL                                 bNonImgDataEnable;  /* 1: nonimage data enable, 0: disable */
    AX_U32                                  nNonImgDataSize;
    AX_SNS_INTF_TYPE_E                      eSnsIntfType;
    AX_SNS_HDR_MODE_E                       eSnsMode;
    AX_BAYER_PATTERN_E                      eBayerPattern;
    AX_IMG_FORMAT_E                         ePixelFmt;          /* Pixel format */
    AX_WIN_AREA_T                           tDevImgRgn[AX_HDR_CHN_NUM];         /* image region acquired by dev */
    AX_U32                                  nWidthStride[AX_HDR_CHN_NUM];
    AX_SNS_OUTPUT_MODE_E                    eSnsOutputMode;
    AX_FRAME_RATE_CTRL_T                    tFrameRateCtrl;     /* suggest using macro AX_VIDEO_FRC_RATIO() */
    AX_FRAME_COMPRESS_INFO_T                tCompressInfo;
    AX_U8                                   nConvYuv422To420En;
    AX_U8                                   nConvFactor;

    union {
        AX_VIN_DEV_MIPI_ATTR_T              tMipiIntfAttr;      /* input interface attr: MIPI */
        AX_VIN_DEV_LVDS_ATTR_T              tLvdsIntfAttr;      /* input interface attr: LVDS */
        AX_VIN_DEV_DVP_ATTR_T               tDvpIntfAttr;       /* input interface attr: DVP */
        AX_VIN_DEV_BT_ATTR_T                tBtIntfAttr;        /* input interface attr: BT1120\BT656\BT601 */
    };
} AX_VIN_DEV_ATTR_T;

typedef enum {
    AX_PRIVATE_DATA_MODE_BACK           = 0,
    AX_PRIVATE_DATA_MODE_FRONT          = 1,
    AX_PRIVATE_DATA_MODE_MAX
} AX_PRIVATE_DATA_MODE_E;

typedef struct _AX_VIN_PRIVATE_DATA_ATTR_T_ {
    AX_BOOL                     bEnable;
    AX_PRIVATE_DATA_MODE_E      ePrivDataMode;
    AX_WIN_AREA_T               tPrivDataRoiRgn[AX_HDR_CHN_NUM];
} AX_VIN_PRIVATE_DATA_ATTR_T;

typedef struct {
    AX_BOOL                     bEnable;
    AX_U64                      nIntCnt;
    AX_U64                      nFrameSeqNum;
    AX_F32                      fFrameRate;
    AX_U32                      nLostFrameCnt;
    AX_U32                      nVbFailCnt;
    AX_U32                      nBlkId;
} AX_VIN_DEV_STATUS_T;

typedef struct {
    AX_BOOL                     bEnable;
    AX_U64                      nIntCnt;
    AX_U64                      nFrameSeqNum;
    AX_F32                      fFrameRate;
    AX_U32                      nLostFrameCnt;
    AX_U32                      nVbFailCnt;
    AX_U32                      nBlkId;
} AX_VIN_PIPE_STATUS_T;

typedef struct {
    AX_BOOL                     bEnable;
    AX_U64                      nIntCnt;
    AX_U64                      nFrameSeqNum;
    AX_F32                      fFrameRate;
    AX_U32                      nLostFrameCnt;
    AX_U32                      nVbFailCnt;
    AX_U32                      nBlkId;
} AX_VIN_CHN_STATUS_T;

typedef struct _AX_VIN_3DNR_ATTR_T_ {
    AX_BOOL                     bPwlEnable;         /* Currently not support */
    AX_FRAME_COMPRESS_INFO_T    tCompressInfo;
} AX_VIN_3DNR_ATTR_T;

typedef struct _AX_VIN_AINR_ATTR_T_ {
    AX_BOOL                     bPwlEnable;         /* Currently not support */
    AX_FRAME_COMPRESS_INFO_T    tCompressInfo;      /* Currently not support */
} AX_VIN_AINR_ATTR_T;

typedef struct _AX_VIN_NR_ATTR_T_ {
    AX_VIN_3DNR_ATTR_T      t3DnrAttr;
    AX_VIN_AINR_ATTR_T      tAinrAttr;
} AX_VIN_NR_ATTR_T;

typedef struct _AX_VIN_PIPE_ATTR_T_ {
    AX_VIN_PIPE_WORK_MODE_E     ePipeWorkMode;      /* pipe work mode */
    AX_WIN_AREA_T               tPipeImgRgn;        /* image region acquired by pipe */
    AX_U32                      nWidthStride;
    AX_BAYER_PATTERN_E          eBayerPattern;
    AX_IMG_FORMAT_E             ePixelFmt;
    AX_SNS_HDR_MODE_E           eSnsMode;
    AX_BOOL                     bAiIspEnable;       /* AIISP master enable */
    AX_FRAME_COMPRESS_INFO_T    tCompressInfo;
    AX_BOOL                     bMotionComp;        /* motion compensation */
    AX_VIN_NR_ATTR_T            tNrAttr;
    AX_FRAME_RATE_CTRL_T        tFrameRateCtrl;     /* suggest using macro AX_VIDEO_FRC_RATIO() */
} AX_VIN_PIPE_ATTR_T;

typedef struct _AX_VIN_CHN_ATTR_T_ {
    AX_U32                      nWidth;
    AX_U32                      nHeight;
    AX_U32                      nWidthStride;
    AX_IMG_FORMAT_E             eImgFormat;
    AX_U32                      nDepth;
    AX_FRAME_COMPRESS_INFO_T    tCompressInfo;  /* lossless: u32CompressLevel = 0;lossy: 8bitmode 1 <=u32CompressLevel <=8,10bitmode 1 <=u32CompressLevel <=10*/
    AX_FRAME_RATE_CTRL_T        tFrameRateCtrl; /* suggest using macro AX_VIDEO_FRC_RATIO() */
} AX_VIN_CHN_ATTR_T;

typedef enum _AX_VIN_PIPE_DUMP_NODE_E_ {
    AX_VIN_PIPE_DUMP_NODE_MIN   = -1,
    AX_VIN_PIPE_DUMP_NODE_IFE   = 0,    /* write data from ife to ddr */
    AX_VIN_PIPE_DUMP_NODE_MAIN,         /* write data from yuv chn */
    AX_VIN_PIPE_DUMP_NODE_MAX,
} AX_VIN_PIPE_DUMP_NODE_E;

typedef enum _AX_VIN_DUMP_QUEUE_TYPE_E_ {
    AX_VIN_DUMP_QUEUE_TYPE_DEV      = 0,
    AX_VIN_DUMP_QUEUE_TYPE_EXT      = 1,
    AX_VIN_DUMP_QUEUE_TYPE_FIND     = 2,
} AX_VIN_DUMP_QUEUE_TYPE_E;

typedef struct _AX_VIN_DEV_DUMP_ATTR_T_ {
    AX_VIN_DUMP_QUEUE_TYPE_E    eDumpType;          /* RW; dump queue type */
    AX_BOOL                     bEnable;            /* RW; Whether dump is enable */
    AX_U32                      nDepth;             /* RW; frame buffer queue depth */
} AX_VIN_DEV_DUMP_ATTR_T;

typedef struct _AX_VIN_DUMP_ATTR_T_ {
    AX_VIN_DUMP_QUEUE_TYPE_E    eDumpType;          /* RW; dump queue type */
    AX_BOOL                     bEnable;            /* RW; Whether dump is enable */
    AX_U32                      nDepth;             /* RW; frame buffer depth */
} AX_VIN_DUMP_ATTR_T;

/* List of pipe bind to dev */
typedef struct _AX_VIN_DEV_BIND_PIPE_T_ {
    AX_U32  nNum;                           /* RW; Range [1, AX_VIN_MAX_PIPE_NUM] */
    AX_U32  nPipeId[AX_VIN_MAX_PIPE_NUM];   /* RW; Array of pipe id */
    AX_U32  nHDRSel[AX_VIN_MAX_PIPE_NUM];   /* 0x1:long | 0x2: middle | 0x4:short | 0x8:veryshort */
} AX_VIN_DEV_BIND_PIPE_T;

typedef enum _AX_DAYNIGHT_MODE_E_ {
    AX_DAYNIGHT_MODE_DAY     = 0,
    AX_DAYNIGHT_MODE_NIGHT,
} AX_DAYNIGHT_MODE_E;

typedef struct _AX_VIN_CROP_INFO_ {
    AX_BOOL bEnable;
    AX_VIN_COORD_E eCoordMode;
    AX_WIN_AREA_T tCropRect;
} AX_VIN_CROP_INFO_T;

typedef struct  _AX_VIN_SCALER_BUFFER_ {
    AX_U32              nWidth;
    AX_U32              nHeight;
    AX_RAW_TYPE_E       eRawType;
    AX_U32              lineStride;
    AX_U64              phy_addr;
    AX_U32              size;
    AX_U64              pts;
    AX_U64              seq_num;
} AX_VIN_SCALER_BUFFER_T;
/************************************************************************************
 *  VIN API
 ************************************************************************************/

/* Global API */
AX_S32 AX_VIN_Init(AX_VOID);
AX_S32 AX_VIN_Deinit(AX_VOID);
//AX_S32 AX_VIN_SetPoolAttr(const AX_POOL_FLOORPLAN_T *pPoolFloorPlan);

/* DEV  API */
AX_S32 AX_VIN_CreateDev(AX_U8 nDevId, const AX_VIN_DEV_ATTR_T *pDevAttr);
AX_S32 AX_VIN_DestroyDev(AX_U8 nDevId);

AX_S32 AX_VIN_SetDevAttr(AX_U8 nDevId, const AX_VIN_DEV_ATTR_T *pDevAttr);
AX_S32 AX_VIN_GetDevAttr(AX_U8 nDevId, AX_VIN_DEV_ATTR_T *pDevAttr);

AX_S32 AX_VIN_EnableDev(AX_U8 nDevId);
AX_S32 AX_VIN_DisableDev(AX_U8 nDevId);

AX_S32 AX_VIN_SetDevBindMipi(AX_U8 nDevId, AX_U8 nMipiId);
AX_S32 AX_VIN_GetDevBindMipi(AX_U8 nDevId, AX_U8 *pMipiId);

AX_S32 AX_VIN_SetDevBindPipe(AX_U8 nDevId, const AX_VIN_DEV_BIND_PIPE_T *ptDevBindPipe);
AX_S32 AX_VIN_GetDevBindPipe(AX_U8 nDevId, AX_VIN_DEV_BIND_PIPE_T *ptDevBindPipe);

AX_S32 AX_VIN_SetDevDumpAttr(AX_U8 nDevId, const AX_VIN_DEV_DUMP_ATTR_T *ptDumpAttr);
AX_S32 AX_VIN_GetDevDumpAttr(AX_U8 nDevId, AX_VIN_DEV_DUMP_ATTR_T *ptDumpAttr);

AX_S32 AX_VIN_SetDevPrivateDataAttr(AX_U8 nDevId, const AX_VIN_PRIVATE_DATA_ATTR_T *ptPrivDataAttr);
AX_S32 AX_VIN_GetDevPrivateDataAttr(AX_U8 nDevId, AX_VIN_PRIVATE_DATA_ATTR_T *ptPrivDataAttr);

AX_S32 AX_VIN_GetDevFrame(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, AX_IMG_INFO_T *pImgInfo, AX_S32 nTimeOutMs);
AX_S32 AX_VIN_ReleaseDevFrame(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, const AX_IMG_INFO_T *pImgInfo);

AX_S32 AX_VIN_FindCachedDevFrame(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, AX_U64 nSeqNum, AX_IMG_INFO_T *pImgInfo);
AX_S32 AX_VIN_ReleaseCachedDevFrame(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, const AX_IMG_INFO_T *pImgInfo);

AX_S32 AX_VIN_GetDevFrameExt(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, AX_IMG_INFO_T *pImgInfo, AX_S32 nTimeOutMs);
AX_S32 AX_VIN_ReleaseDevFrameExt(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, const AX_IMG_INFO_T *pImgInfo);

AX_S32 AX_VIN_GetNonImageData(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, AX_IMG_INFO_T *pImgInfo, AX_S32 nTimeOutMs);
AX_S32 AX_VIN_ReleaseNonImageData(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, const AX_IMG_INFO_T *pImgInfo);

AX_S32 AX_VIN_QueryDevStatus(AX_U8 nDevId, AX_VIN_DEV_STATUS_T *pDevStatus);

AX_S32 AX_VIN_GetDevIrqTimeOut(AX_U8 nDevId, AX_SNS_HDR_FRAME_E eSnsFrame, AX_VIN_DEV_IRQ_TYPE_E eIrqType, AX_S32 nTimeOutMs);

/* FLASH API */
AX_S32 AX_VIN_SetSyncTriggerMode(AX_U8 nDevId, AX_VIN_SYNC_TRIGGER_MODE_E eSnapTriggerMode);

AX_S32 AX_VIN_SetVSyncAttr(AX_U8 nDevId, const AX_VIN_SYNC_SIGNAL_ATTR_T *ptVsyncAttr);
AX_S32 AX_VIN_GetVSyncAttr(AX_U8 nDevId, AX_VIN_SYNC_SIGNAL_ATTR_T *ptVsyncAttr);
AX_S32 AX_VIN_EnableVSync(AX_U8 nDevId);
AX_S32 AX_VIN_DisableVSync(AX_U8 nDevId);

AX_S32 AX_VIN_SetHSyncAttr(AX_U8 nDevId, const AX_VIN_SYNC_SIGNAL_ATTR_T *ptHsyncAttr);
AX_S32 AX_VIN_GetHSyncAttr(AX_U8 nDevId, AX_VIN_SYNC_SIGNAL_ATTR_T *ptHsyncAttr);
AX_S32 AX_VIN_EnableHSync(AX_U8 nDevId);
AX_S32 AX_VIN_DisableHSync(AX_U8 nDevId);

AX_S32 AX_VIN_SetLightSyncInfo(AX_U8 nDevId, const AX_VIN_LIGHT_SYNC_INFO_T *ptLightSyncInfo);
AX_S32 AX_VIN_GetLightSyncInfo(AX_U8 nDevId, AX_VIN_LIGHT_SYNC_INFO_T *ptLightSyncInfo);

AX_S32 AX_VIN_SetStrobeSyncAttr(AX_U8 nDevId, const AX_VIN_STROBE_LIGHT_SYNC_ATTR_T *ptSnapStrobeAttr);
AX_S32 AX_VIN_GetStrobeSyncAttr(AX_U8 nDevId, AX_VIN_STROBE_LIGHT_SYNC_ATTR_T *ptSnapStrobeAttr);
AX_S32 AX_VIN_EnableStrobe(AX_U8 nDevId);
AX_S32 AX_VIN_DisableStrobe(AX_U8 nDevId);

AX_S32 AX_VIN_SetFlashSyncAttr(AX_U8 nDevId, const AX_VIN_FLASH_LIGHT_SYNC_ATTR_T *ptSnapFlashAttr);
AX_S32 AX_VIN_GetFlashSyncAttr(AX_U8 nDevId, AX_VIN_FLASH_LIGHT_SYNC_ATTR_T *ptSnapFlashAttr);
AX_S32 AX_VIN_TriggerFlash(AX_U8 nDevId, AX_BOOL bEnable);

/* PIPE API */
AX_S32 AX_VIN_CreatePipe(AX_U8 nPipeId, const AX_VIN_PIPE_ATTR_T *ptPipeAttr);
AX_S32 AX_VIN_DestroyPipe(AX_U8 nPipeId);

AX_S32 AX_VIN_SetPipeAttr(AX_U8 nPipeId, const AX_VIN_PIPE_ATTR_T *ptPipeAttr);
AX_S32 AX_VIN_GetPipeAttr(AX_U8 nPipeId, AX_VIN_PIPE_ATTR_T *ptPipeAttr);

AX_S32 AX_VIN_SetPipeMirror(AX_U8 nPipeId, AX_BOOL bMirror);
AX_S32 AX_VIN_GetPipeMirror(AX_U8 nPipeId, AX_BOOL *bMirror);

AX_S32 AX_VIN_StartPipe(AX_U8 nPipeId);
AX_S32 AX_VIN_StopPipe(AX_U8 nPipeId);

AX_S32 AX_VIN_SetPipeFrameSource(AX_U8 nPipeId, AX_VIN_FRAME_SOURCE_ID_E eSrcId, AX_VIN_FRAME_SOURCE_TYPE_E  eSrcType);
AX_S32 AX_VIN_GetPipeFrameSource(AX_U8 nPipeId, AX_VIN_FRAME_SOURCE_ID_E eSrcId, AX_VIN_FRAME_SOURCE_TYPE_E *eSrcType);

AX_S32 AX_VIN_SetPipeDumpAttr(AX_U8 nPipeId, AX_VIN_PIPE_DUMP_NODE_E eDumpNode, const AX_VIN_DUMP_ATTR_T *ptDumpAttr);
AX_S32 AX_VIN_GetPipeDumpAttr(AX_U8 nPipeId, AX_VIN_PIPE_DUMP_NODE_E eDumpNode, AX_VIN_DUMP_ATTR_T *ptDumpAttr);

AX_S32 AX_VIN_GetRawFrame(AX_U8 nPipeId, AX_VIN_PIPE_DUMP_NODE_E eRawId, AX_SNS_HDR_FRAME_E eSnsFrame,
                          AX_IMG_INFO_T *pImgInfo, AX_S32 nTimeOutMs);
AX_S32 AX_VIN_ReleaseRawFrame(AX_U8 nPipeId, AX_VIN_PIPE_DUMP_NODE_E eRawId, AX_SNS_HDR_FRAME_E eSnsFrame,
                              const AX_IMG_INFO_T *pImgInfo);

AX_S32 AX_VIN_SendRawFrame(AX_U8 nPipeId, AX_VIN_FRAME_SOURCE_ID_E eSrcId, AX_S8 nFrameNum,
                           const AX_IMG_INFO_T *pImgInfo[],
                           AX_S32 nTimeOutMs);

AX_S32 AX_VIN_GetExtRawFrame(AX_U8 nPipeId, AX_SNS_HDR_FRAME_E eSnsFrame, AX_VIN_SCALER_BUFFER_T *pImgInfo, AX_S32 TimeoutMs);
AX_S32 AX_VIN_SendYuvFrame(AX_U8 nPipeId, const AX_VIDEO_FRAME_T *pImgInfo, AX_S32 nTimeOutMs);

AX_S32 AX_VIN_QueryPipeStatus(AX_U8 nPipeId, AX_VIN_PIPE_STATUS_T *pPipeStatus);

/* CHN */
AX_S32 AX_VIN_SetChnAttr(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_VIN_CHN_ATTR_T *ptChnAttr);
AX_S32 AX_VIN_GetChnAttr(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_VIN_CHN_ATTR_T *ptChnAttr);

AX_S32 AX_VIN_EnableChn(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId);
AX_S32 AX_VIN_DisableChn(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId);

AX_S32 AX_VIN_SetChnFlip(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_BOOL bFlip);
AX_S32 AX_VIN_GetChnFlip(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_BOOL *bFlip);

AX_S32 AX_VIN_GetYuvFrame(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_IMG_INFO_T *pImgInfo, AX_S32 nTimeOutMs);
AX_S32 AX_VIN_ReleaseYuvFrame(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, const AX_IMG_INFO_T *pImgInfo);

AX_S32 AX_VIN_SetChnDayNightMode(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_DAYNIGHT_MODE_E eNightMode);
AX_S32 AX_VIN_GetChnDayNightMode(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_DAYNIGHT_MODE_E *eNightMode);

AX_S32 AX_VIN_SetDiscardYuvFrameNumbers(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_U32 nFrameNum);

AX_S32 AX_VIN_QueryChnStatus(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_VIN_CHN_STATUS_T *pChnStatus);

AX_S32 AX_VIN_SetChnRotation(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, const AX_VIN_ROTATION_E eRotation);
AX_S32 AX_VIN_GetChnRotation(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_VIN_ROTATION_E *eRotation);

AX_S32 AX_VIN_SetChnCrop(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, const AX_VIN_CROP_INFO_T *pCropInfo);
AX_S32 AX_VIN_GetChnCrop(AX_U8 nPipeId, AX_VIN_CHN_ID_E eChnId, AX_VIN_CROP_INFO_T *pCropInfo);

#ifdef __cplusplus
}
#endif

#endif //__AX_VIN_API_H__
