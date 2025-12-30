/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "vin_config.h"
#include "qsdemo.h"
#include "qs_timer.h"
#include <pthread.h>
#include "qs_utils.h"


#if defined(SAMPLE_SNS_SC200AI_SINGLE) || defined(SAMPLE_SNS_SC200AI_DOUBLE) || defined(SAMPLE_SNS_SC200AI_TRIPLE)
extern AX_SENSOR_REGISTER_FUNC_T gSnssc200aiObjQs;
extern AX_SENSOR_REGISTER_FUNC_T gSnssc200aiObj;
#elif defined(SAMPLE_SNS_SC235HAI_SINGLE)
extern AX_SENSOR_REGISTER_FUNC_T gSnssc235haiObj;
#elif defined(SAMPLE_SNS_SC850SL_SINGLE)
extern AX_SENSOR_REGISTER_FUNC_T gSnssc850slObjQs;
//extern AX_SENSOR_REGISTER_FUNC_T gSnssc850slObj;
#elif defined(SAMPLE_SNS_SC500AI_SINGLE)
extern AX_SENSOR_REGISTER_FUNC_T gSnssc500aiObjQs;
//extern AX_SENSOR_REGISTER_FUNC_T gSnssc500aiObj;
#elif defined(SAMPLE_SNS_SC450AI_SINGLE)
extern AX_SENSOR_REGISTER_FUNC_T gSnssc450aiObj;
#elif defined(SAMPLE_SNS_SC850SL_OS04A10)
extern AX_SENSOR_REGISTER_FUNC_T gSnssc850slObjQs;
extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10ObjQs;
//extern AX_SENSOR_REGISTER_FUNC_T gSnssc850slObj;
//extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10Obj;
#elif defined(SAMPLE_SNS_OS04D10_SINGLE)
extern AX_SENSOR_REGISTER_FUNC_T gSnsos04d10ObjQs;
//extern AX_SENSOR_REGISTER_FUNC_T gSnsos04d10Obj;
#elif defined(SAMPLE_SNS_OS04A10_DOUBLE)
extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10ObjQs;
//extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10Obj;
#else
extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10ObjQs;
//extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10Obj;
#endif

#if defined(SAMPLE_SNS_SC200AI_SINGLE)
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc200aiSdr[] = {
    {1920, 1080, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc200aiSdrRotation[] = {
    {1920, 1152, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleSc200aiSdr[] = {
    {1920, 1080, 1920, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 6, 2, 4},      /* vin raw10 use */
};
#elif defined(SAMPLE_SNS_SC200AI_DOUBLE) || defined(SAMPLE_SNS_SC200AI_TRIPLE)
COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleSc200aiSdr[] = {
    {1920, 1080, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 6, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 4, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 6, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleSc200aiSdrRotation[] = {
    {1920, 1152, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 8, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 4, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 6, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolDoubleSc200aiSdr[] = {
    {1920, 1080, 1920, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 12, 2, 4},      /* vin raw10 use */
};
#elif defined(SAMPLE_SNS_SC235HAI_SINGLE)
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc235haiSdr[] = {
    {1920, 1080, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc235haiSdrRotation[] = {
    {1920, 1152, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleSc235haiSdr[] = {
    {1920, 1080, 1920, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 6, 2, 4},      /* vin raw10 use */
};
#elif defined(SAMPLE_SNS_SC850SL_SINGLE)
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc850slSdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc850slSdrRotation[] = {
    {3840, 2176, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleSc850slSdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 6, 2, 4},      /* vin raw10 use */
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc850sl2MSdr[] = {
    {1920, 1080, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc850sl2MSdrRotation[] = {
    {1920, 1152, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleSc850sl2MSdr[] = {
    {1920, 1080, 1920, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 6, 2, 4},      /* vin raw10 use */
};
#elif defined(SAMPLE_SNS_SC850SL_OS04A10)
COMMON_SYS_POOL_CFG_T gtSysCommPoolSc850slOs04a10Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 6, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSc850slOs04a10SdrRotation[] = {
    {3840, 2176, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 4, 2, 4},    /* vin nv21/nv21 use */
    {2688, 1536, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 4, 2, 4},
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 6, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSc850slOs04a10Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 8, 2, 4},      /* vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 8, 2, 4},      /* vin raw10 use */
};
#elif defined(SAMPLE_SNS_SC500AI_SINGLE)
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc500aiSdr[] = {
    {2880, 1620, 2944, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 0, 0},     /* 5M(2880) not support 1280 fbc */
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc500aiSdrRotation[] = {
    {2880, 1664, 2944, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 0, 0},     /* 5M(2880) not support 1280 fbc */
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleSc500aiSdr[] = {
    {2880, 1620, 2880, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 6, 2, 4},      /* vin raw10 use */
};
#elif defined(SAMPLE_SNS_SC450AI_SINGLE)
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc450aiSdr[] = {
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 0, 0},     /* 4M(2688) not support 1280 fbc */
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc450aiSdrRotation[] = {
    {2688, 1536, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 0, 0},     /* 4M(2688) not support 1280 fbc */
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleSc450aiSdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 6, 2, 4},      /* vin raw10 use */
};
#elif defined(SAMPLE_SNS_OS04D10_SINGLE)
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04d10Sdr[] = {
    {2560, 1440, 2560, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 0, 0},     /* 4M(2688) not support 1280 fbc */
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04d10SdrRotation[] = {
    {2560, 1536, 2560, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 0, 0},     /* 4M(2688) not support 1280 fbc */
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleOs04d10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 8, 2, 4},      /* vin raw10 use */
};
#elif defined(SAMPLE_SNS_OS04A10_DOUBLE)
COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 6, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 4, 0, 0},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 6, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleOs04a10SdrRotation[] = {
    {2688, 1536, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 8, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 4, 0, 0},
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 6, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolDoubleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 12, 2, 4},      /* vin raw10 use */
};
#else
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 2, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 0, 0},     /* 4M(2688) not support 1280 fbc */
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10SdrRotation[] = {
    {2688, 1536, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 3, 2, 4},    /* vin nv21/nv21 use */
    {1280, 720, 1280, AX_FORMAT_YUV420_SEMIPLANAR, 2, 0, 0},     /* 4M(2688) not support 1280 fbc */
    {1024, 576, 1024, AX_FORMAT_YUV420_SEMIPLANAR, 3, 0, 0},
};
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 8, 2, 4},      /* vin raw10 use */
};
#endif

#if defined(SAMPLE_SNS_SC200AI_SINGLE)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 1920, .nHeight = 1080, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 1920.0/1024.0f, .fDetectHeightRatio = 1080.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#elif defined(SAMPLE_SNS_SC200AI_DOUBLE) || defined(SAMPLE_SNS_SC200AI_TRIPLE)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 1920, .nHeight = 1080, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 1920.0/1024.0f, .fDetectHeightRatio = 1080.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},

    {.nWidth = 1920, .nHeight = 1080, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 2, .fDetectWidthRatio = 1920.0/1024.0f, .fDetectHeightRatio = 1080.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 3, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#elif defined(SAMPLE_SNS_SC235HAI_SINGLE)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 1920, .nHeight = 1080, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 1920.0/1024.0f, .fDetectHeightRatio = 1080.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#elif defined(SAMPLE_SNS_SC850SL_SINGLE)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 3840, .nHeight = 2160, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 3840.0/1024.0f, .fDetectHeightRatio = 2160.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
SAMPLE_CHN_ATTR_T gOutChnAttr2M[] = {
    {.nWidth = 1920, .nHeight = 1080, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 1920.0/1024.0f, .fDetectHeightRatio = 1080.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#elif defined(SAMPLE_SNS_SC850SL_OS04A10)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 3840, .nHeight = 2160, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 3840.0/1024.0f, .fDetectHeightRatio = 2160.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},

    {.nWidth = 2688, .nHeight = 1520, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 2, .fDetectWidthRatio = 2688.0/1024.0f, .fDetectHeightRatio = 1520.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_TRUE, .nRtspChn = 3, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#elif defined(SAMPLE_SNS_SC500AI_SINGLE)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 2880, .nHeight = 1620, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 2880.0/1024.0f, .fDetectHeightRatio = 1620.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},     /* 5M(2880) not support 1280 fbc */
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#elif defined(SAMPLE_SNS_SC450AI_SINGLE)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 2688, .nHeight = 1520, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 2688.0/1024.0f, .fDetectHeightRatio = 1520.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},     /* 4M(2688) not support 1280 fbc */
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#elif defined(SAMPLE_SNS_OS04D10_SINGLE)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 2560, .nHeight = 1440, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 2560.0/1024.0f, .fDetectHeightRatio = 1440.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},     /* 4M(2560) not support 1280 fbc */
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#elif defined(SAMPLE_SNS_OS04A10_DOUBLE)
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 2688, .nHeight = 1520, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 2688.0/1024.0f, .fDetectHeightRatio = 1520.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},

    {.nWidth = 2688, .nHeight = 1520, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 2, .fDetectWidthRatio = 2688.0/1024.0f, .fDetectHeightRatio = 1520.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_TRUE, .nRtspChn = 3, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#else
SAMPLE_CHN_ATTR_T gOutChnAttr[] = {
    {.nWidth = 2688, .nHeight = 1520, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 2, .bVenc = AX_TRUE, .nRtspChn = 0, .fDetectWidthRatio = 2688.0/1024.0f, .fDetectHeightRatio = 1520.0/576.0f, .ptRgn = NULL},
    {.nWidth = 1280, .nHeight = 720, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_TRUE, .nRtspChn = 1, .fDetectWidthRatio = 1280.0/1024.0f, .fDetectHeightRatio = 720.0/576.0f, .ptRgn = NULL},     /* 4M(2688) not support 1280 fbc */
    {.nWidth = 1024, .nHeight = 576, .fFramerate = SENSOR_FRAMERATE, .enCompressMode = 0, .bVenc = AX_FALSE, .nRtspChn = -1, .fDetectWidthRatio = 1.0f, .fDetectHeightRatio = 1.0f, .ptRgn = NULL},
};
#endif

AX_S32 QSDEMO_VIN_GetSnsConfig(SAMPLE_SNS_TYPE_E eSnsType,
                               AX_MIPI_RX_ATTR_T *ptMipiAttr, AX_SNS_ATTR_T *ptSnsAttr,
                               AX_SNS_CLK_ATTR_T *ptSnsClkAttr, AX_VIN_DEV_ATTR_T *pDevAttr,
                               AX_VIN_PIPE_ATTR_T *pPipeAttr, AX_VIN_CHN_ATTR_T *pChnAttr)
{
#if defined(SAMPLE_SNS_SC200AI_SINGLE) || defined(SAMPLE_SNS_SC200AI_DOUBLE) ||  defined(SAMPLE_SNS_SC200AI_TRIPLE)
    if (eSnsType == SMARTSENS_SC200AI) {
        memcpy(ptMipiAttr, &gSc200aiMipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
        memcpy(ptSnsAttr, &gSc200aiSnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gSc200aiSnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gSc200aiDevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gSc200aiPipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gSc200aiChn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
    }
#endif
#if defined(SAMPLE_SNS_SC235HAI_SINGLE)
    if (eSnsType == SMARTSENS_SC235HAI) {
        memcpy(ptMipiAttr, &gMipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
        memcpy(ptSnsAttr, &gSnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gSnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gDevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gPipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gChn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
    }
#endif
#if defined(SAMPLE_SNS_SC850SL_SINGLE) || defined(SAMPLE_SNS_SC850SL_OS04A10)
    if (eSnsType == SMARTSENS_SC850SL) {
        memcpy(ptMipiAttr, &gSc850slMipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
        memcpy(ptSnsAttr, &gSc850slSnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gSc850slSnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gSc850slDevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gSc850slPipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gSc850slChn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
    } else if (eSnsType == SMARTSENS_SC850SL2M) {
        memcpy(ptMipiAttr, &gSc850sl2MMipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
        memcpy(ptSnsAttr, &gSc850sl2MSnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gSc850sl2MSnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gSc850sl2MDevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gSc850sl2MPipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gSc850sl2MChn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
    }
#endif
#if defined(SAMPLE_SNS_SC500AI_SINGLE)
    if (eSnsType == SMARTSENS_SC500AI) {
        memcpy(ptMipiAttr, &gSc500aiMipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
        memcpy(ptSnsAttr, &gSc500aiSnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gSc500aiSnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gSc500aiDevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gSc500aiPipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gSc500aiChn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
    }
#endif
#if defined(SAMPLE_SNS_SC450AI_SINGLE)
    if (eSnsType == SMARTSENS_SC450AI) {
        memcpy(ptMipiAttr, &gSc450aiMipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
        memcpy(ptSnsAttr, &gSc450aiSnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gSc450aiSnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gSc450aiDevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gSc450aiPipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gSc450aiChn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
    }
#endif
#if defined(SAMPLE_SNS_OS04D10_SINGLE)
    if (eSnsType == OMNIVISION_OS04D10) {
        memcpy(ptMipiAttr, &gOs04d10MipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
        memcpy(ptSnsAttr, &gOs04d10SnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gOs04d10SnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gOs04d10DevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gOs04d10PipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gOs04d10Chn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
    }
#endif
#if defined(SAMPLE_SNS_OS04A10_SINGLE) || defined(SAMPLE_SNS_SC850SL_OS04A10) || defined(SAMPLE_SNS_OS04A10_DOUBLE)
    if (eSnsType == OMNIVISION_OS04A10) {
        memcpy(ptMipiAttr, &gOs04a10MipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
        memcpy(ptSnsAttr, &gOs04a10SnsAttr, sizeof(AX_SNS_ATTR_T));
        memcpy(ptSnsClkAttr, &gOs04a10SnsClkAttr, sizeof(AX_SNS_CLK_ATTR_T));
        memcpy(pDevAttr, &gOs04a10DevAttr, sizeof(AX_VIN_DEV_ATTR_T));
        memcpy(pPipeAttr, &gOs04a10PipeAttr, sizeof(AX_VIN_PIPE_ATTR_T));
        memcpy(&pChnAttr[0], &gOs04a10Chn0Attr, sizeof(AX_VIN_CHN_ATTR_T));
    }
#endif
    return 0;
}

static AX_VOID __set_pipe_hdr_mode(AX_U32 *pHdrSel, AX_SNS_HDR_MODE_E eHdrMode)
{
    if (NULL == pHdrSel) {
        return;
    }

    switch (eHdrMode) {
    case AX_SNS_LINEAR_MODE:
    case AX_SNS_LINEAR_ONLY_MODE:
        *pHdrSel = 0x1;
        break;

    case AX_SNS_HDR_2X_MODE:
        *pHdrSel = 0x1 | 0x2;
        break;

    default:
        *pHdrSel = 0x1;
        break;
    }
}

static AX_VOID __set_vin_attr(AX_CAMERA_T *pCam, SAMPLE_SNS_TYPE_E eSnsType, AX_SNS_HDR_MODE_E eHdrMode,
                              COMMON_VIN_MODE_E eSysMode, AX_BOOL bAiispEnable, SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    pCam->eSnsType = eSnsType;
    pCam->tSnsAttr.eSnsMode = eHdrMode;
    pCam->tDevAttr.eSnsMode = eHdrMode;
    pCam->eHdrMode = eHdrMode;
    pCam->eSysMode = eSysMode;
    pCam->tPipeAttr.eSnsMode = eHdrMode;
    pCam->tPipeAttr.bAiIspEnable = bAiispEnable;
    if (IS_SNS_HDR_MODE(eHdrMode)) {
        pCam->tDevAttr.eSnsOutputMode = AX_SNS_DOL_HDR;
    }

    if (COMMON_VIN_TPG == eSysMode) {
        pCam->tDevAttr.eSnsIntfType = AX_SNS_INTF_TYPE_TPG;
    }
    pCam->bChnEn[0] = (pEntryParam->nVinIvpsMode == AX_ITP_ONLINE_VPP) ? AX_FALSE : AX_TRUE;
    pCam->nChnFrmMode[0] = (pEntryParam->nVinIvpsMode == AX_ITP_ONLINE_VPP) ? 0 : pEntryParam->nVinChnFrmMode;
    pCam->bEnableDev = AX_TRUE;
    pCam->bRegisterSns = AX_TRUE;
}

#if defined(SAMPLE_SNS_SC200AI_SINGLE)
static AX_U32 __sample_case_single_sc200ai(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pEntryParam->nCamCnt = 1;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    pCam = &pCamList[0];
    QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = &gSnssc200aiObjQs;
    pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
    }

    // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
    // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;

    return 0;
}
#elif defined(SAMPLE_SNS_SC200AI_DOUBLE)
static AX_U32 __sample_case_double_sc200ai(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    AX_S32 j = 0, i = 0;
    pCommonArgs->nCamCnt = 2;
    pEntryParam->nCamCnt = 2;

    for (i = 0; i < pCommonArgs->nCamCnt; i++) {
        pCam = &pCamList[i];
        pCam->nNumber = i;
        QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        pCam->nDevId = i;
        pCam->nRxDev = i;
        pCam->nPipeId = i;
        pCam->nI2cAddr = 0x30;
        if (i == 1 && IS_AX620Q) {
            pCam->nI2cAddr = 0x32;  //620Q: 0x32, 630C: 0x30
        }
        pCam->tSnsClkAttr.nSnsClkIdx = 0;
        pCam->tDevBindPipe.nNum = 1;
        pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
        pCam->ptSnsHdl = &gSnssc200aiObjQs;
        pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
        pCam->tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_2;

        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
        }

        // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
        // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;
    }

    return 0;
}
#elif defined(SAMPLE_SNS_SC200AI_TRIPLE)
static AX_U32 __sample_case_triple_sc200ai(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    AX_S32 j = 0, i = 0;
    pCommonArgs->nCamCnt = 2;
    pEntryParam->nCamCnt = 2;

    for (i = 0; i < pCommonArgs->nCamCnt; i++) {
        pCam = &pCamList[i];
        pCam->nNumber = i;
        QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        pCam->nDevId = i;
        pCam->nRxDev = i;
        pCam->nPipeId = i;
        pCam->nI2cAddr = 0x30;

        pCam->tSnsClkAttr.nSnsClkIdx = 0;
        pCam->tDevBindPipe.nNum = 1;
        pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
        pCam->ptSnsHdl = &gSnssc200aiObjQs;
        pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
        pCam->tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_2;

        if (i == 1) {
            if (pEntryParam->nSwitchSnsId == 1) {
                pCam->ptSnsHdl1 = &gSnssc200aiObj;
            } else {
                pCam->ptSnsHdl1 = &gSnssc200aiObjQs;
                pCam->ptSnsHdl = &gSnssc200aiObj;
            }
            pCam->nSwitchSnsId = pEntryParam->nSwitchSnsId;
            pCam->nI2cAddr = 0x32;
            pCam->nI2cAddr1 = 0x32;
            pCam->tSnsAttr.eMasterSlaveSel = AX_SNS_SYNC_SLAVE;
            pCam->bMipiSwitchEnable = AX_TRUE;
            pCam->eLoadRawNode = LOAD_RAW_ITP;
            pCam->eSysMode = COMMON_VIN_LOADRAW;
            pCam->tEzoomSwitchInfo = pEntryParam->tEzoomSwitchInfo;
            if (pEntryParam->tEzoomSwitchInfo.nWidth > 0) {
                pCam->tEzoomSwitchInfo.fSwitchRatio = pCam->tSnsAttr.nWidth * 1.0f / pEntryParam->tEzoomSwitchInfo.nWidth;
            } else {
                pCam->tEzoomSwitchInfo.fSwitchRatio = pEntryParam->tEzoomSwitchInfo.fSwitchRatio;
            }
            pEntryParam->tEzoomSwitchInfo.fSwitchRatio = pCam->tEzoomSwitchInfo.fSwitchRatio;
            ALOGI("fSwitchRatio: %f", pEntryParam->tEzoomSwitchInfo.fSwitchRatio);
            eSysMode = COMMON_VIN_LOADRAW;
        }

        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
        }

        if (pCam->bMipiSwitchEnable) {
            strcpy(pCam->tPipeInfo[0].szBinPath, "nulll.bin"); // wide bin
            strcpy(pCam->tPipeInfo[1].szBinPath, "nulll.bin"); // tele bin
        }

        // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
        // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;
    }

    return 0;
}
#elif defined(SAMPLE_SNS_SC235HAI_SINGLE)
static AX_U32 __sample_case_single_sc235hai(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pEntryParam->nCamCnt = 1;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    pCam = &pCamList[0];
    QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = &gSnssc235haiObj;
    pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
    }

    // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
    // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;

    return 0;
}
#elif defined(SAMPLE_SNS_SC850SL_SINGLE)
static AX_U32 __sample_case_single_sc850sl(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pEntryParam->nCamCnt = 1;

    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    pCam = &pCamList[0];
    pCam->nNumber = 0;
    QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    //pCam->nI2cAddr = 0x30;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = &gSnssc850slObjQs;
    pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;

    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
        if (pEntryParam->bSc850sl2M) {
            strcpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/sc850sl_sdr_2m.bin");
        }
    }

    // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
    // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;

    return 0;
}
#elif defined(SAMPLE_SNS_SC850SL_OS04A10)
static AX_U32 __sample_case_sc850sl_os04a10(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    AX_S32 j = 0, i = 0;
    pCommonArgs->nCamCnt = 2;
    pEntryParam->nCamCnt = 2;

    for (i = 0; i < pCommonArgs->nCamCnt; i++) {
        if (i == 1) {
            eSnsType = OMNIVISION_OS04A10;
        }
        pCam = &pCamList[i];
        pCam->nNumber = i;
        QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        pCam->ptSnsHdl = (i == 0) ? &gSnssc850slObjQs : &gSnsos04a10ObjQs;
        pCam->tSnsAttr.nSettingIndex = (i == 1) ? 35 : 0;
        pCam->nDevId = i;
        pCam->nRxDev = i;
        pCam->nPipeId = i;
        pCam->nI2cAddr = 0x30;
        if (i == 1 && IS_AX620Q) {
            pCam->nI2cAddr = 0x32;  //620Q: 0x32, 630C: 0x30
        }
        pCam->tSnsClkAttr.nSnsClkIdx = 0;
        pCam->tDevBindPipe.nNum = 1;
        pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;

        pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
        pCam->tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_2;

        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
        }

        // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
        // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;
    }

    return 0;
}
#elif defined(SAMPLE_SNS_SC500AI_SINGLE)
static AX_U32 __sample_case_single_sc500ai(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pEntryParam->nCamCnt = 1;

    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    pCam = &pCamList[0];
    pCam->nNumber = 0;
    QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    //pCam->nI2cAddr = 0x30;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = &gSnssc500aiObjQs;
    pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;

    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
    }

    // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
    // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;


    return 0;
}
#elif defined(SAMPLE_SNS_SC450AI_SINGLE)
static AX_U32 __sample_case_single_sc450ai(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pEntryParam->nCamCnt = 1;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    pCam = &pCamList[0];
    QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = &gSnssc450aiObj;
    pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
    }

    // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
    // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;

    return 0;
}
#elif defined(SAMPLE_SNS_OS04D10_SINGLE)
static AX_U32 __sample_case_single_os04d10(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pEntryParam->nCamCnt = 1;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    pCam = &pCamList[0];
    QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = &gSnsos04d10ObjQs;
    pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
    }

    // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
    // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;

    return 0;
}
#elif defined(SAMPLE_SNS_OS04A10_DOUBLE)
static AX_U32 __sample_case_double_os04a10(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    AX_S32 j = 0, i = 0;
    pCommonArgs->nCamCnt = 2;
    pEntryParam->nCamCnt = 2;

    for (i = 0; i < pCommonArgs->nCamCnt; i++) {
        pCam = &pCamList[i];
        pCam->nNumber = i;
        QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        pCam->ptSnsHdl = &gSnsos04a10ObjQs;
        pCam->tSnsAttr.nSettingIndex = 35;
        pCam->nDevId = i;
        pCam->nRxDev = i;
        pCam->nPipeId = i;
        pCam->nI2cAddr = 0x36;

        pCam->tSnsClkAttr.nSnsClkIdx = 0;
        pCam->tDevBindPipe.nNum = 1;
        pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;

        pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
        pCam->tMipiAttr.eLaneNum = AX_MIPI_DATA_LANE_2;

        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
        }

        // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
        // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;
    }

    return 0;
}
#else
static AX_U32 __sample_case_single_os04a10(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = COMMON_VIN_SENSOR;
    AX_SNS_HDR_MODE_E eHdrMode = pEntryParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pEntryParam->nCamCnt = 1;
    AX_BOOL bAiispEnable = (pEntryParam->nAiIspMode == E_SNS_TISP_MODE_E) ? AX_FALSE : AX_TRUE;
    pCam = &pCamList[0];
    QSDEMO_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    if (IS_AX620Q) {
        AX_S32 nSingleSnsPipeId = QS_GetSingleSnsPipeId(-1);
        /* rx0:0 rx1:1 */
        if (nSingleSnsPipeId >= 0) {
            pCam->nDevId = nSingleSnsPipeId;
            pCam->nRxDev = nSingleSnsPipeId;
            pCam->nPipeId = nSingleSnsPipeId;
            /* only for os04a 620q rx1 */
            if (nSingleSnsPipeId == 1) {
                pCam->nI2cAddr = 0x10;
            }
            pCam->tSnsAttr.nSettingIndex = 35;
            pCam->tMipiAttr.eLaneNum = 2;
        }
    } else {
#if defined(AX620E_NAND)
    pCam->tMipiAttr.eLaneNum = 2;
    pCam->tSnsAttr.nSettingIndex = 35;
#endif
    }
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = &gSnsos04a10ObjQs;
    pCam->eBusType = ISP_SNS_CONNECT_I2C_TYPE;
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, bAiispEnable, pEntryParam);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = bAiispEnable;
    }

    // pCam->tChnAttr[0].tCompressInfo.enCompressMode = gOutChnAttr[0].enCompressMode;
    // pCam->tChnAttr[0].tCompressInfo.u32CompressLevel = 0;

    return 0;
}
#endif

AX_S32 qs_cam_pool_config(SAMPLE_ENTRY_PARAM_T *pEntryParam, COMMON_SYS_ARGS_T *pCommonArgs, COMMON_SYS_ARGS_T *pPrivArgs, AX_CAMERA_T* pCamList)
{
    SAMPLE_SNS_TYPE_E   eSnsType = SAMPLE_SNS_TYPE_NONE;

#if defined(SAMPLE_SNS_SC200AI_SINGLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC200AI);

    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc200aiSdrRotation) / sizeof(gtSysCommPoolSingleSc200aiSdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc200aiSdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc200aiSdr) / sizeof(gtSysCommPoolSingleSc200aiSdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc200aiSdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleSc200aiSdr) / sizeof(gtPrivatePoolSingleSc200aiSdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolSingleSc200aiSdr;

    /* cams config */
    __sample_case_single_sc200ai(pCamList, eSnsType, pEntryParam, pCommonArgs);
#elif defined(SAMPLE_SNS_SC200AI_DOUBLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC200AI);

    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleSc200aiSdrRotation) / sizeof(gtSysCommPoolDoubleSc200aiSdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolDoubleSc200aiSdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleSc200aiSdr) / sizeof(gtSysCommPoolDoubleSc200aiSdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolDoubleSc200aiSdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolDoubleSc200aiSdr) / sizeof(gtPrivatePoolDoubleSc200aiSdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolDoubleSc200aiSdr;

    /* cams config */
    __sample_case_double_sc200ai(pCamList, eSnsType, pEntryParam, pCommonArgs);
#elif defined(SAMPLE_SNS_SC200AI_TRIPLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC200AI);

    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleSc200aiSdrRotation) / sizeof(gtSysCommPoolDoubleSc200aiSdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolDoubleSc200aiSdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleSc200aiSdr) / sizeof(gtSysCommPoolDoubleSc200aiSdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolDoubleSc200aiSdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolDoubleSc200aiSdr) / sizeof(gtPrivatePoolDoubleSc200aiSdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolDoubleSc200aiSdr;

    /* cams config */
    __sample_case_triple_sc200ai(pCamList, eSnsType, pEntryParam, pCommonArgs);
#elif defined(SAMPLE_SNS_SC235HAI_SINGLE)
eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC235HAI);

    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc235haiSdrRotation) / sizeof(gtSysCommPoolSingleSc235haiSdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc235haiSdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc235haiSdr) / sizeof(gtSysCommPoolSingleSc235haiSdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc235haiSdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleSc235haiSdr) / sizeof(gtPrivatePoolSingleSc235haiSdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolSingleSc235haiSdr;

    /* cams config */
    __sample_case_single_sc235hai(pCamList, eSnsType, pEntryParam, pCommonArgs);
#elif defined(SAMPLE_SNS_SC850SL_SINGLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC850SL);
    if (pEntryParam->bSc850sl2M) {
        eSnsType = SMARTSENS_SC850SL2M;
        memcpy(gOutChnAttr, gOutChnAttr2M, sizeof(gOutChnAttr));
    }

    if (pEntryParam->bSc850sl2M) {
        /* comm pool config */
        if (pEntryParam->nGdcOnlineVppTest) {
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc850sl2MSdrRotation) / sizeof(gtSysCommPoolSingleSc850sl2MSdrRotation[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc850sl2MSdrRotation;
        } else {
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc850sl2MSdr) / sizeof(gtSysCommPoolSingleSc850sl2MSdr[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc850sl2MSdr;
        }

        /* private pool config */
        pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleSc850sl2MSdr) / sizeof(gtPrivatePoolSingleSc850sl2MSdr[0]);
        pPrivArgs->pPoolCfg = gtPrivatePoolSingleSc850sl2MSdr;

    } else {
        /* comm pool config */
        if (pEntryParam->nGdcOnlineVppTest) {
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc850slSdrRotation) / sizeof(gtSysCommPoolSingleSc850slSdrRotation[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc850slSdrRotation;
        } else {
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc850slSdr) / sizeof(gtSysCommPoolSingleSc850slSdr[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc850slSdr;
        }

        /* private pool config */
        pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleSc850slSdr) / sizeof(gtPrivatePoolSingleSc850slSdr[0]);
        pPrivArgs->pPoolCfg = gtPrivatePoolSingleSc850slSdr;
    }

    /* cams config */
    __sample_case_single_sc850sl(pCamList, eSnsType, pEntryParam, pCommonArgs);
#elif defined(SAMPLE_SNS_SC850SL_OS04A10)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC850SL);

    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSc850slOs04a10SdrRotation) / sizeof(gtSysCommPoolSc850slOs04a10SdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSc850slOs04a10SdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSc850slOs04a10Sdr) / sizeof(gtSysCommPoolSc850slOs04a10Sdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSc850slOs04a10Sdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSc850slOs04a10Sdr) / sizeof(gtPrivatePoolSc850slOs04a10Sdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolSc850slOs04a10Sdr;

    /* cams config */
    __sample_case_sc850sl_os04a10(pCamList, eSnsType, pEntryParam, pCommonArgs);
#elif defined(SAMPLE_SNS_SC500AI_SINGLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC500AI);

    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc500aiSdrRotation) / sizeof(gtSysCommPoolSingleSc500aiSdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc500aiSdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc500aiSdr) / sizeof(gtSysCommPoolSingleSc500aiSdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc500aiSdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleSc500aiSdr) / sizeof(gtPrivatePoolSingleSc500aiSdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolSingleSc500aiSdr;

    /* cams config */
    __sample_case_single_sc500ai(pCamList, eSnsType, pEntryParam, pCommonArgs);
#elif defined(SAMPLE_SNS_SC450AI_SINGLE)
    eSnsType = SMARTSENS_SC450AI;
    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc450aiSdrRotation) / sizeof(gtSysCommPoolSingleSc450aiSdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc450aiSdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc450aiSdr) / sizeof(gtSysCommPoolSingleSc450aiSdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc450aiSdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleSc450aiSdr) / sizeof(gtPrivatePoolSingleSc450aiSdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolSingleSc450aiSdr;

    /* cams config */
    __sample_case_single_sc450ai(pCamList, eSnsType, pEntryParam, pCommonArgs);

#elif defined(SAMPLE_SNS_OS04D10_SINGLE)
    eSnsType = OMNIVISION_OS04D10;
    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04d10SdrRotation) / sizeof(gtSysCommPoolSingleOs04d10SdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04d10SdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04d10Sdr) / sizeof(gtSysCommPoolSingleOs04d10Sdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04d10Sdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleOs04d10Sdr) / sizeof(gtPrivatePoolSingleOs04d10Sdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolSingleOs04d10Sdr;

    /* cams config */
    __sample_case_single_os04d10(pCamList, eSnsType, pEntryParam, pCommonArgs);
#elif defined(SAMPLE_SNS_OS04A10_DOUBLE)
    eSnsType = OMNIVISION_OS04A10;

    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleOs04a10SdrRotation) / sizeof(gtSysCommPoolDoubleOs04a10SdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolDoubleOs04a10SdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleOs04a10Sdr) / sizeof(gtSysCommPoolDoubleOs04a10Sdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolDoubleOs04a10Sdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolDoubleOs04a10Sdr) / sizeof(gtPrivatePoolDoubleOs04a10Sdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolDoubleOs04a10Sdr;

    /* cams config */
    __sample_case_double_os04a10(pCamList, eSnsType, pEntryParam, pCommonArgs);
#else
    eSnsType = OMNIVISION_OS04A10;

    /* comm pool config */
    if (pEntryParam->nGdcOnlineVppTest) {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10SdrRotation) / sizeof(gtSysCommPoolSingleOs04a10SdrRotation[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04a10SdrRotation;
    } else {
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10Sdr) / sizeof(gtSysCommPoolSingleOs04a10Sdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04a10Sdr;
    }

    /* private pool config */
    pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleOs04a10Sdr) / sizeof(gtPrivatePoolSingleOs04a10Sdr[0]);
    pPrivArgs->pPoolCfg = gtPrivatePoolSingleOs04a10Sdr;

    /* cams config */
    __sample_case_single_os04a10(pCamList, eSnsType, pEntryParam, pCommonArgs);
#endif
    return 0;
}

AX_S32 qs_cam_change_hdrmode(SAMPLE_ENTRY_PARAM_T *pEntryParam, AX_CAMERA_T* pCamList)
{
    SAMPLE_SNS_TYPE_E   eSnsType = SAMPLE_SNS_TYPE_NONE;
    COMMON_SYS_ARGS_T   commonArgs = {0};

#if defined(SAMPLE_SNS_SC200AI_SINGLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC200AI);

    /* cams config */
    __sample_case_single_sc200ai(pCamList, eSnsType, pEntryParam, &commonArgs);
    pCamList[0].ptSnsHdl = &gSnssc200aiObj;
#elif defined(SAMPLE_SNS_SC200AI_DOUBLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC200AI);

    /* cams config */
    __sample_case_double_sc200ai(pCamList, eSnsType, pEntryParam, &commonArgs);
    pCamList[0].ptSnsHdl = &gSnssc200aiObj;
    pCamList[1].ptSnsHdl = &gSnssc200aiObj;
#elif defined(SAMPLE_SNS_SC200AI_TRIPLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC200AI);

    /* cams config */
    __sample_case_triple_sc200ai(pCamList, eSnsType, pEntryParam, &commonArgs);
    pCamList[0].ptSnsHdl = &gSnssc200aiObj;
    pCamList[1].ptSnsHdl = &gSnssc200aiObj;
    pCamList[1].ptSnsHdl1 = &gSnssc200aiObj;
#elif defined(SAMPLE_SNS_SC235HAI_SINGLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC235HAI);

    /* cams config */
    __sample_case_single_sc235hai(pCamList, eSnsType, pEntryParam, &commonArgs);
    pCamList[0].ptSnsHdl = &gSnssc235haiObj;
#elif defined(SAMPLE_SNS_SC850SL_SINGLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC850SL);

    /* cams config */
    __sample_case_single_sc850sl(pCamList, eSnsType, pEntryParam, &commonArgs);
    //pCamList[0].ptSnsHdl = &gSnssc850slObj;
#elif defined(SAMPLE_SNS_SC850SL_OS04A10)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC850SL);

    /* cams config */
    __sample_case_sc850sl_os04a10(pCamList, eSnsType, pEntryParam, &commonArgs);
    //pCamList[0].ptSnsHdl = &gSnssc850slObj;
    //pCamList[1].ptSnsHdl = &gSnsos04a10Obj;
#elif defined(SAMPLE_SNS_SC500AI_SINGLE)
    eSnsType = (SAMPLE_SNS_TYPE_E)(SMARTSENS_SC500AI);

    /* cams config */
    __sample_case_single_sc500ai(pCamList, eSnsType, pEntryParam, &commonArgs);
    //pCamList[0].ptSnsHdl = &gSnssc500aiObj;
#elif defined(SAMPLE_SNS_SC450AI_SINGLE)
    eSnsType = SMARTSENS_SC450AI;

    /* cams config */
    __sample_case_single_sc450ai(pCamList, eSnsType, pEntryParam, &commonArgs);
    //pCamList[0].ptSnsHdl = &gSnssc450aiObj;
#elif defined(SAMPLE_SNS_OS04D10_SINGLE)
    eSnsType = OMNIVISION_OS04D10;

    /* cams config */
    __sample_case_single_os04d10(pCamList, eSnsType, pEntryParam, &commonArgs);
    //pCamList[0].ptSnsHdl = &gSnsos04d10Obj;
#elif defined(SAMPLE_SNS_OS04A10_DOUBLE)
    eSnsType = OMNIVISION_OS04A10;

    /* cams config */
    __sample_case_double_os04a10(pCamList, eSnsType, pEntryParam, &commonArgs);
    //pCamList[0].ptSnsHdl = &gSnsos04a10Obj;
#else
    eSnsType = OMNIVISION_OS04A10;

    /* cams config */
    __sample_case_single_os04a10(pCamList, eSnsType, pEntryParam, &commonArgs);
    //pCamList[0].ptSnsHdl = &gSnsos04a10Obj;
#endif
    return 0;
}
