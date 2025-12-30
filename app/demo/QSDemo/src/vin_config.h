/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _VIN_CONFIG__
#define _VIN_CONFIG__

#include "ax_global_type.h"
#include "ax_vin_api.h"
#include "ax_mipi_rx_api.h"
#include "qsdemo.h"

#define SNS_FPS_DEFAULT  15

#if defined(SAMPLE_SNS_OS04A10_SINGLE) || defined(SAMPLE_SNS_SC850SL_OS04A10) || defined(SAMPLE_SNS_OS04A10_DOUBLE)

AX_MIPI_RX_ATTR_T gOs04a10MipiAttr = {
    .ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .eLaneNum = AX_MIPI_DATA_LANE_4,
    .nDataRate =  80,
    .nDataLaneMap[0] = 0,
    .nDataLaneMap[1] = 1,
    .nDataLaneMap[2] = 3,
    .nDataLaneMap[3] = 4,
    .nClkLane[0]     = 2,
    .nClkLane[1]     = 5,
};

AX_SNS_ATTR_T gOs04a10SnsAttr = {
    .nWidth = 2688,
    .nHeight = 1520,
    .fFrameRate = SNS_FPS_DEFAULT,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    // .nSettingIndex = 12,
};

AX_SNS_CLK_ATTR_T gOs04a10SnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gOs04a10DevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_ONLINE,
    .eDevWorkMode = AX_VIN_DEV_WORK_MODE_1MULTIPLEX,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 2688, 1520},
    .tDevImgRgn[1] = {0, 0, 2688, 1520},
    .tDevImgRgn[2] = {0, 0, 2688, 1520},
    .tDevImgRgn[3] = {0, 0, 2688, 1520},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    //.tMipiIntfAttr.szImgVc[0] = 0,
    //.tMipiIntfAttr.szImgDt[0] = MIPI_CSI_DT_RAW10,
    //.tMipiIntfAttr.szImgVc[1] = 1,
    //.tMipiIntfAttr.szImgDt[1] = MIPI_CSI_DT_RAW10,

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl= {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gOs04a10PipeAttr = {
    .ePipeWorkMode = AX_VIN_PIPE_NORMAL_MODE1,
    .tPipeImgRgn = {0, 0, 2688, 1520},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_LOSSLESS, 0}}, {0, {AX_COMPRESS_MODE_NONE, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gOs04a10Chn0Attr = {
    .nWidth = 2688,
    .nHeight = 1520,
    .nWidthStride = 2688,
    .eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 0,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 4},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};
#endif

#if defined(SAMPLE_SNS_OS04D10_SINGLE)

AX_MIPI_RX_ATTR_T gOs04d10MipiAttr = {
    .ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .eLaneNum = AX_MIPI_DATA_LANE_2,
    .nDataRate = 720,
    .nDataLaneMap[0] = 0,
    .nDataLaneMap[1] = 1,
    .nDataLaneMap[2] = 3,
    .nDataLaneMap[3] = 4,
    .nClkLane[0]     = 2,
    .nClkLane[1]     = 5,
};

AX_SNS_ATTR_T gOs04d10SnsAttr = {
    .nWidth = 2560,
    .nHeight = 1440,
    .fFrameRate = SNS_FPS_DEFAULT,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    // .nSettingIndex = 12,
};

AX_SNS_CLK_ATTR_T gOs04d10SnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gOs04d10DevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_ONLINE,
    .eDevWorkMode = AX_VIN_DEV_WORK_MODE_1MULTIPLEX,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 2560, 1440},
    .tDevImgRgn[1] = {0, 0, 2560, 1440},
    .tDevImgRgn[2] = {0, 0, 2560, 1440},
    .tDevImgRgn[3] = {0, 0, 2560, 1440},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    //.tMipiIntfAttr.szImgVc[0] = 0,
    //.tMipiIntfAttr.szImgDt[0] = MIPI_CSI_DT_RAW10,
    //.tMipiIntfAttr.szImgVc[1] = 1,
    //.tMipiIntfAttr.szImgDt[1] = MIPI_CSI_DT_RAW10,

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl= {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gOs04d10PipeAttr = {
    .ePipeWorkMode = AX_VIN_PIPE_NORMAL_MODE1,
    .tPipeImgRgn = {0, 0, 2560, 1440},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_LOSSLESS, 0}}, {0, {AX_COMPRESS_MODE_NONE, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gOs04d10Chn0Attr = {
    .nWidth = 2560,
    .nHeight = 1440,
    .nWidthStride = 2560,
    .eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 0,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 4},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};
#endif

#if defined(SAMPLE_SNS_SC200AI_SINGLE) || defined(SAMPLE_SNS_SC200AI_DOUBLE) || defined(SAMPLE_SNS_SC200AI_TRIPLE)

AX_MIPI_RX_ATTR_T gSc200aiMipiAttr = {
    .ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .eLaneNum = AX_MIPI_DATA_LANE_2,
    .nDataRate =  400,
    .nDataLaneMap[0] = 0,
    .nDataLaneMap[1] = 1,
    .nDataLaneMap[2] = 3,
    .nDataLaneMap[3] = 4,
    .nClkLane[0]     = 2,
    .nClkLane[1]     = 5,
};

AX_SNS_ATTR_T gSc200aiSnsAttr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .fFrameRate = SNS_FPS_DEFAULT,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    // .nSettingIndex = 12,
};

AX_SNS_CLK_ATTR_T gSc200aiSnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gSc200aiDevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_ONLINE,
    .eDevWorkMode = AX_VIN_DEV_WORK_MODE_1MULTIPLEX,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 1920, 1080},
    .tDevImgRgn[1] = {0, 0, 1920, 1080},
    .tDevImgRgn[2] = {0, 0, 1920, 1080},
    .tDevImgRgn[3] = {0, 0, 1920, 1080},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    // .tMipiIntfAttr.szImgVc[0] = 0;
    // .tMipiIntfAttr.szImgVc[1] = 1;
    // .tMipiIntfAttr.szImgDt[0] = 44;
    // .tMipiIntfAttr.szImgDt[1] = 44;
    // .tMipiIntfAttr.szInfoVc[0] = 31;
    // .tMipiIntfAttr.szInfoVc[1] = 31;
    // .tMipiIntfAttr.szInfoDt[0] = 63;
    // .tMipiIntfAttr.szInfoDt[1] = 63;

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl= {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gSc200aiPipeAttr = {
    .ePipeWorkMode = AX_VIN_PIPE_NORMAL_MODE1,
    .tPipeImgRgn = {0, 0, 1920, 1080},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_LOSSLESS, 0}}, {0, {AX_COMPRESS_MODE_NONE, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gSc200aiChn0Attr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .nWidthStride = 1920,
    .eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 0,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 4},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};
#endif

#if defined(SAMPLE_SNS_SC850SL_SINGLE) || defined(SAMPLE_SNS_SC850SL_OS04A10)

AX_MIPI_RX_ATTR_T gSc850slMipiAttr = {
    .ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .eLaneNum = AX_MIPI_DATA_LANE_2,
    .nDataRate =  1080,
    .nDataLaneMap[0] = 0,
    .nDataLaneMap[1] = 1,
    .nClkLane[0]     = 2,
    .nClkLane[1]     = 5,
};

AX_SNS_ATTR_T gSc850slSnsAttr = {
    .nWidth = 3840,
    .nHeight = 2160,
    .fFrameRate = SNS_FPS_DEFAULT,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    // .nSettingIndex = 12,
};

AX_SNS_CLK_ATTR_T gSc850slSnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gSc850slDevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_ONLINE,
    .eDevWorkMode = AX_VIN_DEV_WORK_MODE_1MULTIPLEX,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 3840, 2160},
    .tDevImgRgn[1] = {0, 0, 3840, 2160},
    .tDevImgRgn[2] = {0, 0, 3840, 2160},
    .tDevImgRgn[3] = {0, 0, 3840, 2160},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    //.tMipiIntfAttr.szImgVc[0] = 0,
    //.tMipiIntfAttr.szImgDt[0] = MIPI_CSI_DT_RAW10,
    //.tMipiIntfAttr.szImgVc[1] = 1,
    //.tMipiIntfAttr.szImgDt[1] = MIPI_CSI_DT_RAW10,

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl= {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gSc850slPipeAttr = {
    .ePipeWorkMode = AX_VIN_PIPE_NORMAL_MODE1,
    .tPipeImgRgn = {0, 0, 3840, 2160},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_LOSSLESS, 0}}, {0, {AX_COMPRESS_MODE_NONE, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gSc850slChn0Attr = {
    .nWidth = 3840,
    .nHeight = 2160,
    .nWidthStride = 3840,
    .eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 0,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 4},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};


AX_MIPI_RX_ATTR_T gSc850sl2MMipiAttr = {
    .ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .eLaneNum = AX_MIPI_DATA_LANE_2,
    .nDataRate =  1080,
    .nDataLaneMap[0] = 0,
    .nDataLaneMap[1] = 1,
    .nClkLane[0]     = 2,
    .nClkLane[1]     = 5,
};

AX_SNS_ATTR_T gSc850sl2MSnsAttr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .fFrameRate = SNS_FPS_DEFAULT,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    // .nSettingIndex = 12,
};

AX_SNS_CLK_ATTR_T gSc850sl2MSnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gSc850sl2MDevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_ONLINE,
    .eDevWorkMode = AX_VIN_DEV_WORK_MODE_1MULTIPLEX,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 1920, 1080},
    .tDevImgRgn[1] = {0, 0, 1920, 1080},
    .tDevImgRgn[2] = {0, 0, 1920, 1080},
    .tDevImgRgn[3] = {0, 0, 1920, 1080},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    //.tMipiIntfAttr.szImgVc[0] = 0,
    //.tMipiIntfAttr.szImgDt[0] = MIPI_CSI_DT_RAW10,
    //.tMipiIntfAttr.szImgVc[1] = 1,
    //.tMipiIntfAttr.szImgDt[1] = MIPI_CSI_DT_RAW10,

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl= {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gSc850sl2MPipeAttr = {
    .ePipeWorkMode = AX_VIN_PIPE_NORMAL_MODE1,
    .tPipeImgRgn = {0, 0, 1920, 1080},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_LOSSLESS, 0}}, {0, {AX_COMPRESS_MODE_NONE, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gSc850sl2MChn0Attr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .nWidthStride = 1920,
    .eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 0,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 4},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

#endif

#if defined(SAMPLE_SNS_SC500AI_SINGLE)

AX_MIPI_RX_ATTR_T gSc500aiMipiAttr = {
    .ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .eLaneNum = AX_MIPI_DATA_LANE_4,
    .nDataRate = 396,
    .nDataLaneMap[0] = 0,
    .nDataLaneMap[1] = 1,
    .nDataLaneMap[2] = 3,
    .nDataLaneMap[3] = 4,
    .nClkLane[0]     = 2,
    .nClkLane[1]     = 5,
};

AX_SNS_ATTR_T gSc500aiSnsAttr = {
    .nWidth = 2880,
    .nHeight = 1620,
    .fFrameRate = SNS_FPS_DEFAULT,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    // .nSettingIndex = 12,
};

AX_SNS_CLK_ATTR_T gSc500aiSnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gSc500aiDevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_ONLINE,
    .eDevWorkMode = AX_VIN_DEV_WORK_MODE_1MULTIPLEX,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 2880, 1620},
    .tDevImgRgn[1] = {0, 0, 2880, 1620},
    .tDevImgRgn[2] = {0, 0, 2880, 1620},
    .tDevImgRgn[3] = {0, 0, 2880, 1620},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    //.tMipiIntfAttr.szImgVc[0] = 0,
    //.tMipiIntfAttr.szImgDt[0] = MIPI_CSI_DT_RAW10,
    //.tMipiIntfAttr.szImgVc[1] = 1,
    //.tMipiIntfAttr.szImgDt[1] = MIPI_CSI_DT_RAW10,

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl= {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gSc500aiPipeAttr = {
    .ePipeWorkMode = AX_VIN_PIPE_NORMAL_MODE1,
    .tPipeImgRgn = {0, 0, 2880, 1620},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_LOSSLESS, 0}}, {0, {AX_COMPRESS_MODE_NONE, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gSc500aiChn0Attr = {
    .nWidth = 2880,
    .nHeight = 1620,
    .nWidthStride = 2944,
    .eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 0,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 4},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};
#endif

#if defined(SAMPLE_SNS_SC450AI_SINGLE)

AX_MIPI_RX_ATTR_T gSc450aiMipiAttr = {
    .ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .eLaneNum = AX_MIPI_DATA_LANE_4,
    .nDataRate =  80,
    .nDataLaneMap[0] = 0,
    .nDataLaneMap[1] = 1,
    .nDataLaneMap[2] = 3,
    .nDataLaneMap[3] = 4,
    .nClkLane[0]     = 2,
    .nClkLane[1]     = 5,
};

AX_SNS_ATTR_T gSc450aiSnsAttr = {
    .nWidth = 2688,
    .nHeight = 1520,
    .fFrameRate = SNS_FPS_DEFAULT,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    // .nSettingIndex = 12,
};

AX_SNS_CLK_ATTR_T gSc450aiSnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gSc450aiDevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_ONLINE,
    .eDevWorkMode = AX_VIN_DEV_WORK_MODE_1MULTIPLEX,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 2688, 1520},
    .tDevImgRgn[1] = {0, 0, 2688, 1520},
    .tDevImgRgn[2] = {0, 0, 2688, 1520},
    .tDevImgRgn[3] = {0, 0, 2688, 1520},

    /* When users transfer special data, they need to configure VC&DT for szImgVc/szImgDt/szInfoVc/szInfoDt */
    //.tMipiIntfAttr.szImgVc[0] = 0,
    //.tMipiIntfAttr.szImgDt[0] = MIPI_CSI_DT_RAW10,
    //.tMipiIntfAttr.szImgVc[1] = 1,
    //.tMipiIntfAttr.szImgDt[1] = MIPI_CSI_DT_RAW10,

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl= {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gSc450aiPipeAttr = {
    .ePipeWorkMode = AX_VIN_PIPE_NORMAL_MODE1,
    .tPipeImgRgn = {0, 0, 2688, 1520},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_LOSSLESS, 0}}, {0, {AX_COMPRESS_MODE_NONE, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gSc450aiChn0Attr = {
    .nWidth = 2688,
    .nHeight = 1520,
    .nWidthStride = 2688,
    .eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 0,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 4},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};
#endif

#if defined(SAMPLE_SNS_SC235HAI_SINGLE)
AX_MIPI_RX_ATTR_T gMipiAttr = {
    .ePhyMode = AX_MIPI_PHY_TYPE_DPHY,
    .eLaneNum = AX_MIPI_DATA_LANE_2,
    .nDataRate =  396,
    .nDataLaneMap[0] = 0,
    .nDataLaneMap[1] = 1,
    .nClkLane[0]     = 2,
    .nClkLane[1]     = 5,
};

AX_SNS_ATTR_T gSnsAttr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .fFrameRate = SNS_FPS_DEFAULT,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eRawType = AX_RT_RAW10,
    .eBayerPattern = AX_BP_RGGB,
    .bTestPatternEnable = AX_FALSE,
    // .nSettingIndex = 12,
};

AX_SNS_CLK_ATTR_T gSnsClkAttr = {
    .nSnsClkIdx = 0,
    .eSnsClkRate = AX_SNS_CLK_24M,
};

AX_VIN_DEV_ATTR_T gDevAttr = {
    .bImgDataEnable = AX_TRUE,
    .bNonImgDataEnable = AX_FALSE,
    .eDevMode = AX_VIN_DEV_ONLINE,
    .eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW,
    .tDevImgRgn[0] = {0, 0, 1920, 1080},
    .tDevImgRgn[1] = {0, 0, 1920, 1080},
    .tDevImgRgn[2] = {0, 0, 1920, 1080},
    .tDevImgRgn[3] = {0, 0, 1920, 1080},

    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eBayerPattern = AX_BP_RGGB,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .eSnsOutputMode = AX_SNS_NORMAL,
    .tCompressInfo = {AX_COMPRESS_MODE_NONE, 0},
    .tFrameRateCtrl= {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_PIPE_ATTR_T gPipeAttr = {
    .ePipeWorkMode = AX_VIN_PIPE_NORMAL_MODE1,
    .tPipeImgRgn = {0, 0, 1920, 1080},
    .eBayerPattern = AX_BP_RGGB,
    .ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP_PACKED,
    .eSnsMode = QS_DEMO_DEFAULT_SNS_MODE,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 0},
    .tNrAttr = {{0, {AX_COMPRESS_MODE_LOSSLESS, 0}}, {0, {AX_COMPRESS_MODE_NONE, 0}}},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};

AX_VIN_CHN_ATTR_T gChn0Attr = {
    .nWidth = 1920,
    .nHeight = 1080,
    .nWidthStride = 1920,
    .eImgFormat = AX_FORMAT_YUV420_SEMIPLANAR,
    .nDepth = 0,
    .tCompressInfo = {AX_COMPRESS_MODE_LOSSY, 4},
    .tFrameRateCtrl = {AX_INVALID_FRMRATE, AX_INVALID_FRMRATE},
};
#endif
#endif // _VIN_CONFIG__
