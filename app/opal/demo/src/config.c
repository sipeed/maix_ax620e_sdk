/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "config.h"
#include <stddef.h>

const AX_OPAL_VIDEO_ENCODER_ATTR_T g_stOpalEncoderList[AX_OPAL_VIDEO_RC_MODE_BUTT] = {
    {
        .eRcMode = AX_OPAL_VIDEO_RC_MODE_CBR,
        .nQpLevel = 0,
        .nGop = 60, // setting as system default
        .nBitrate = 2048,  // setting as system default
        .nMinQp = 10,
        .nMaxQp = 51,
        .nMinIQp = 10,
        .nMaxIQp = 51,
        .nMinIprop = 10,
        .nMaxIprop = 40,
        .nFixQp = 0,
        .nIntraQpDelta = -2,
        .nDeBreathQpDelta = -2,
        .nIdrQpDeltaRange = 10
    },
    {
        .eRcMode = AX_OPAL_VIDEO_RC_MODE_VBR,
        .nQpLevel = 0,
        .nGop = 60, // setting as system default
        .nBitrate = 2048, // setting as system default
        .nMinQp = 31,
        .nMaxQp = 46,
        .nMinIQp = 31,
        .nMaxIQp = 46,
        .nMinIprop = 10,
        .nMaxIprop = 40,
        .nFixQp = 0,
        .nIntraQpDelta = -2,
        .nDeBreathQpDelta = -2,
        .nIdrQpDeltaRange = 10
    },
    {
        .eRcMode = AX_OPAL_VIDEO_RC_MODE_FIXQP,
        .nQpLevel = 0,
        .nGop = 60, // setting as system default
        .nBitrate = 2048,  // setting as system default
        .nMinQp = 0,
        .nMaxQp = 51,
        .nMinIQp = 0,
        .nMaxIQp = 0,
        .nMinIprop = 10,
        .nMaxIprop = 40,
        .nFixQp = 40,
        .nIntraQpDelta = 0,
        .nDeBreathQpDelta = 0,
        .nIdrQpDeltaRange = 0
    },
    {
        .eRcMode = AX_OPAL_VIDEO_RC_MODE_AVBR,
        .nQpLevel = 0,
        .nGop = 60, // setting as system default
        .nBitrate = 2048, // setting as system default
        .nMinQp = 31,
        .nMaxQp = 46,
        .nMinIQp = 31,
        .nMaxIQp = 46,
        .nMinIprop = 10,
        .nMaxIprop = 40,
        .nFixQp = 0,
        .nIntraQpDelta = -2,
        .nDeBreathQpDelta = -2,
        .nIdrQpDeltaRange = 10
    },
    {
        .eRcMode = AX_OPAL_VIDEO_RC_MODE_CVBR,
        .nQpLevel = 0,
        .nGop = 60, // setting as system default
        .nBitrate = 2048, // setting as system default
        .nMinQp = 0,
        .nMaxQp = 51,
        .nMinIQp = 0,
        .nMaxIQp = 51,
        .nMinIprop = 10,
        .nMaxIprop = 40,
        .nFixQp = 0,
        .nIntraQpDelta = -2,
        .nDeBreathQpDelta = -2,
        .nIdrQpDeltaRange = 10
    }
};

DEMO_SNS_OSD_CONFIG_T g_stOsdCfg[AX_OPAL_SNS_ID_BUTT] = {
    // sensor 0
    [0] = {
        .stOsd = {
            // chn0
            [0] = {
                // index: 0
                // time osd
                [0] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_LEFT_TOP,
                        .nXBoundary = 48,
                        .nYBoundary = 20,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_TIME,
                        {
                            .stDatetimeAttr = {
                                .nFontSize = 128,
                                .eFormat = AX_OPAL_OSD_DATETIME_FORMAT_YYMMDDHHMMSS1,
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_MAIN_ID,
                    .pHandle = NULL,
                    .szStr = {0},
                },
                // index: 1
                // picture osd
                [1] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM,
                        .nXBoundary = 32,
                        .nYBoundary = 48,
                        .nARGB = 0,
                        .eType = AX_OPAL_OSD_TYPE_PICTURE,
                        {
                            .stPicAttr = {
                                .nWidth = 256,
                                .nHeight = 64,
                                .eSource = AX_OPAL_OSD_PIC_SOURCE_FILE,
                                {
                                    .pstrFileName = (AX_CHAR *)"./res/axera_logo_256x64.argb1555",
                                }
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_MAIN_ID,
                    .pHandle = NULL,
                    .szStr = {0},
                },
                // index: 2
                // string osd
                [2] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_LEFT_BOTTOM,
                        .nXBoundary = 40,
                        .nYBoundary = 16,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_STRING,
                        {
                            .stStrAttr = {
                                .nFontSize = 48,
                                .pStr = (AX_CHAR *)"AXERA OPAL DEMO",
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_MAIN_ID,
                    .pHandle = NULL,
                    .szStr = "AXERA OPAL DEMO",
                },
                // index: 3
                // string osd
                [3] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_RIGHT_TOP,
                        .nXBoundary = 20,
                        .nYBoundary = 20,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_STRING_TOP,
                        {
                            .stStrAttr = {
                                .nFontSize = 48,
                                .pStr = (AX_CHAR *)"LOCATION-*",
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_MAIN_ID,
                    .pHandle = NULL,
                    .szStr = "LOCATION-*",
                },
            },
            // chn1
            [1] = {
                // index: 0
                // time osd
                [0] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_LEFT_TOP,
                        .nXBoundary = 14,
                        .nYBoundary = 8,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_TIME,
                        {
                            .stDatetimeAttr = {
                                .nFontSize = 20,
                                .eFormat = AX_OPAL_OSD_DATETIME_FORMAT_YYMMDDHHMMSS1,
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_SUB1_ID,
                    .pHandle = NULL,
                    .szStr = {0},
                },
                // index: 1
                // picture osd
                [1] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM,
                        .nXBoundary = 8,
                        .nYBoundary = 8,
                        .nARGB = 0,
                        .eType = AX_OPAL_OSD_TYPE_PICTURE,
                        {
                            .stPicAttr = {
                                .nWidth = 96,
                                .nHeight = 28,
                                .eSource = AX_OPAL_OSD_PIC_SOURCE_FILE,
                                {
                                    .pstrFileName = (AX_CHAR *)"./res/axera_logo_96x28.argb1555",
                                }
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_SUB1_ID,
                    .pHandle = NULL,
                    .szStr = "./res/axera_logo_96x28.argb1555",
                },
                // index: 2
                // string osd
                [2] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_LEFT_BOTTOM,
                        .nXBoundary = 20,
                        .nYBoundary = 8,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_STRING,
                        {
                            .stStrAttr = {
                                .nFontSize = 16,
                                .pStr = (AX_CHAR *)"AXERA OPAL DEMO",
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_SUB1_ID,
                    .pHandle = NULL,
                    .szStr = "AXERA OPAL DEMO",
                },
                // index: 3
                // string osd
                [3] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_RIGHT_TOP,
                        .nXBoundary = 20,
                        .nYBoundary = 20,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_STRING_TOP,
                        {
                            .stStrAttr = {
                                .nFontSize = 16,
                                .pStr = (AX_CHAR *)"LOCATION-*",
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_SUB1_ID,
                    .pHandle = NULL,
                    .szStr = "LOCATION-*",
                }
            },
        },
        .stPriv = {
            // privacy osd
            .stOsdAttr = {
                .bEnable = AX_FALSE,
                .eAlign = AX_OPAL_OSD_ALIGN_LEFT_TOP,
                .nXBoundary = 200,
                .nYBoundary = 200,
                .nARGB = 0xFFFFFFFF,
                .eType = AX_OPAL_OSD_TYPE_PRIVACY,
                {
                    .stPrivacyAttr = {
                        .eType = AX_OPAL_OSD_PRIVACY_TYPE_RECT,
                        .nLineWidth = 1,
                        .bSolid = AX_TRUE,
                        .stPoints = {{100, 100}, {400, 100}, {400, 300}, {100, 300}}
                    }
                }
            },
            .nSrcChn = 0,  // not used
            .pHandle = NULL,
            .szStr = {0},
        }
    },
    [1] = {
        .stOsd = {
            // chn0
            [0] = {
                // index: 0
                // time osd
                [0] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_LEFT_TOP,
                        .nXBoundary = 48,
                        .nYBoundary = 20,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_TIME,
                        {
                            .stDatetimeAttr = {
                                .nFontSize = 128,
                                .eFormat = AX_OPAL_OSD_DATETIME_FORMAT_YYMMDDHHMMSS1,
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_MAIN_ID,
                    .pHandle = NULL,
                    .szStr = {0},
                },
                // index: 1
                // picture osd
                [1] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM,
                        .nXBoundary = 32,
                        .nYBoundary = 48,
                        .nARGB = 0,
                        .eType = AX_OPAL_OSD_TYPE_PICTURE,
                        {
                            .stPicAttr = {
                                .nWidth = 256,
                                .nHeight = 64,
                                .eSource = AX_OPAL_OSD_PIC_SOURCE_FILE,
                                {
                                    .pstrFileName = (AX_CHAR *)"./res/axera_logo_256x64.argb1555",
                                }
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_MAIN_ID,
                    .pHandle = NULL,
                    .szStr = "./res/axera_logo_256x64.argb1555",
                },
                // index: 2
                // string osd
                [2] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_LEFT_BOTTOM,
                        .nXBoundary = 40,
                        .nYBoundary = 16,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_STRING,
                        {
                            .stStrAttr = {
                                .nFontSize = 48,
                                .pStr = (AX_CHAR *)"AXERA OPAL DEMO",
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_MAIN_ID,
                    .pHandle = NULL,
                    .szStr = "AXERA OPAL DEMO",
                },
                // index: 3
                // string osd
                [3] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_RIGHT_TOP,
                        .nXBoundary = 20,
                        .nYBoundary = 20,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_STRING_TOP,
                        {
                            .stStrAttr = {
                                .nFontSize = 48,
                                .pStr = (AX_CHAR *)"LOCATION-*",
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_MAIN_ID,
                    .pHandle = NULL,
                    .szStr = "LOCATION-*",
                },
            },
            // chn1
            [1] = {
                // index: 0
                // time osd
                [0] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_LEFT_TOP,
                        .nXBoundary = 14,
                        .nYBoundary = 8,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_TIME,
                        {
                            .stDatetimeAttr = {
                                .nFontSize = 20,
                                .eFormat = AX_OPAL_OSD_DATETIME_FORMAT_YYMMDDHHMMSS1,
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_SUB1_ID,
                    .pHandle = NULL,
                    .szStr = {0},
                },
                // index: 1
                // picture osd
                [1] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM,
                        .nXBoundary = 8,
                        .nYBoundary = 8,
                        .nARGB = 0,
                        .eType = AX_OPAL_OSD_TYPE_PICTURE,
                        {
                            .stPicAttr = {
                                .nWidth = 96,
                                .nHeight = 28,
                                .eSource = AX_OPAL_OSD_PIC_SOURCE_FILE,
                                {
                                    .pstrFileName = (AX_CHAR *)"./res/axera_logo_96x28.argb1555",
                                }
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_SUB1_ID,
                    .pHandle = NULL,
                    .szStr = "./res/axera_logo_96x28.argb1555",
                },
                // index: 2
                // string osd
                [2] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_LEFT_BOTTOM,
                        .nXBoundary = 20,
                        .nYBoundary = 8,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_STRING,
                        {
                            .stStrAttr = {
                                .nFontSize = 16,
                                .pStr = (AX_CHAR *)"AXERA OPAL DEMO",
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_SUB1_ID,
                    .pHandle = NULL,
                    .szStr = "AXERA OPAL DEMO",
                },
                // index: 3
                // string osd
                [3] = {
                    .stOsdAttr = {
                        .bEnable = AX_TRUE,
                        .eAlign = AX_OPAL_OSD_ALIGN_RIGHT_TOP,
                        .nXBoundary = 20,
                        .nYBoundary = 20,
                        .nARGB = 0xFFFFFFFF,
                        .eType = AX_OPAL_OSD_TYPE_STRING_TOP,
                        {
                            .stStrAttr = {
                                .nFontSize = 16,
                                .pStr = (AX_CHAR *)"LOCATION-*",
                                .bInvEnable = AX_FALSE,
                                .nColorInv = 0x0,
                            }
                        }
                    },
                    .nSrcChn = VIDEO_CHN_VENC_SUB1_ID,
                    .pHandle = NULL,
                    .szStr = "LOCATION-*",
                }
            },
        },
        .stPriv = {
            // privacy osd
            .stOsdAttr = {
                .bEnable = AX_FALSE,
                .eAlign = AX_OPAL_OSD_ALIGN_LEFT_TOP,
                .nXBoundary = 200,
                .nYBoundary = 200,
                .nARGB = 0xFFFFFFFF,
                .eType = AX_OPAL_OSD_TYPE_PRIVACY,
                {
                    .stPrivacyAttr = {
                        .eType = AX_OPAL_OSD_PRIVACY_TYPE_RECT,
                        .nLineWidth = 1,
                        .bSolid = AX_TRUE,
                        .stPoints = {{100, 100}, {400, 100}, {400, 300}, {100, 300}}
                    }
                }
            },
            .nSrcChn = 0,  // not used
            .pHandle = NULL,
            .szStr = {0},
        }
    }
};

AX_OPAL_VIDEO_SNS_ATTR_T g_stOpalSnsAttrList[AX_OPAL_SNS_ID_BUTT] = {
    [0] = {
        .bFlip = AX_FALSE,
        .bMirror = AX_FALSE,
        .fFrameRate = 30,
        .eMode = AX_OPAL_SNS_SDR_MODE,
        .eDayNight = AX_OPAL_SNS_DAY_MODE,
        .eRotation = AX_OPAL_SNS_ROTATION_0,
        .stColorAttr = {
            .bManual = AX_FALSE,
            .fBrightness = 50,
            .fSharpness = 0,
            .fContrast = 50,
            .fSaturation = 50,
        },
        .stLdcAttr = {
            .bEnable = AX_FALSE,
            .bAspect = AX_FALSE,
            .nXRatio = 0,
            .nYRatio = 0,
            .nXYRatio = 0,
            .nDistortionRatio = 1500,
        },
        .stDisAttr = {
            .bEnable = AX_FALSE,
            .bMotionEst = AX_FALSE,
            .bMotionShare = AX_FALSE,
            .nDelayFrameNum = 4,
        },
        .stEZoomAttr = {
            .fRatio = 1,
        },
    },
    [1] = {
        .bFlip = AX_FALSE,
        .bMirror = AX_FALSE,
        .fFrameRate = 25,
        .eMode = AX_OPAL_SNS_SDR_MODE,
        .eDayNight = AX_OPAL_SNS_DAY_MODE,
        .eRotation = AX_OPAL_SNS_ROTATION_0,
        .stColorAttr = {
            .bManual = AX_FALSE,
            .fBrightness = 50,
            .fSharpness = 0,
            .fContrast = 50,
            .fSaturation = 50,
        },
        .stLdcAttr = {
            .bEnable = AX_FALSE,
            .bAspect = AX_FALSE,
            .nXRatio = 0,
            .nYRatio = 0,
            .nXYRatio = 0,
            .nDistortionRatio = 1500,
        },
        .stDisAttr = {
            .bEnable = AX_FALSE,
            .bMotionEst = AX_FALSE,
            .bMotionShare = AX_FALSE,
            .nDelayFrameNum = 4,
        },
        .stEZoomAttr = {
            .fRatio = 1,
        },
    }
};

AX_OPAL_ATTR_T g_stOpalAttr = {
    .stVideoAttr = {
        // sensor 0
        [0] = {
            .bEnable = AX_TRUE,
            .stAlgoAttr = {
                .nAlgoType = AX_OPAL_ALGO_HVCP | AX_OPAL_ALGO_MD | AX_OPAL_ALGO_OD,
            },
            .stSnsAttr = {
                .bFlip = AX_FALSE,
                .bMirror = AX_FALSE,
                .fFrameRate = 30,
                .eMode = AX_OPAL_SNS_SDR_MODE,
                .eDayNight = AX_OPAL_SNS_DAY_MODE,
                .eRotation = AX_OPAL_SNS_ROTATION_0,
                .stColorAttr = {
                    .bManual = AX_FALSE,
                    .fBrightness = 50,
                    .fSharpness = 0,
                    .fContrast = 50,
                    .fSaturation = 50,
                },
                .stLdcAttr = {
                    .bEnable = AX_FALSE,
                    .bAspect = AX_FALSE,
                    .nXRatio = 0,
                    .nYRatio = 0,
                    .nXYRatio = 0,
                    .nDistortionRatio = 1500,
                },
                .stDisAttr = {
                    .bEnable = AX_FALSE,
                    .bMotionEst = AX_FALSE,
                    .bMotionShare = AX_FALSE,
                    .nDelayFrameNum = 4,
                },
                .stEZoomAttr = {
                    .fRatio = 0,
                },
            },
            .stPipeAttr = {
                .stVideoChnAttr = {
                    // video channel 0
                    [0] = {
                        .bEnable = AX_TRUE,
                        .nMaxWidth = -1,        // -1: same as sensor width
                        .nMaxHeight = -1,       // -1: same as sensor height
                        .nWidth = -1,           // -1: same as sensor width
                        .nHeight = -1,          // -1: same as sensor height
                        .nFramerate = 30,       // -1: same as sensor framerate
                        .eType = AX_OPAL_VIDEO_CHN_TYPE_H264,
                        .stEncoderAttr = {
                            .eRcMode = AX_OPAL_VIDEO_RC_MODE_CBR,
                            .nQpLevel = 0,
                            .nGop = 60,          // setting as system default
                            .nBitrate = 4096,      // setting as system default
                            .nMinQp = 10,
                            .nMaxQp = 51,
                            .nMinIQp = 10,
                            .nMaxIQp = 51,
                            .nMinIprop = 10,
                            .nMaxIprop = 40,
                            .nFixQp = 0,
                            .nIntraQpDelta = -2,
                            .nDeBreathQpDelta = -2,
                            .nIdrQpDeltaRange = 10
                        }
                    },
                    // video channel 1 for algorithm
                    [1] = {
                        .bEnable = AX_TRUE,
                        .nMaxWidth = 1280,
                        .nMaxHeight = 720,
                        .nWidth = 1280,
                        .nHeight = 720,
                        .nFramerate = 10,
                        .eType = AX_OPAL_VIDEO_CHN_TYPE_ALGO,
                    },
                    // video channel 2
                    [2] = {
                        .bEnable = AX_TRUE,
                        .nMaxWidth = 720,
                        .nMaxHeight = 576,
                        .nWidth = 720,
                        .nHeight = 576,
                        .nFramerate = 30,       // -1: same as sensor framerate
                        .eType = AX_OPAL_VIDEO_CHN_TYPE_H264,
                        .stEncoderAttr = {
                            .eRcMode = AX_OPAL_VIDEO_RC_MODE_CBR,
                            .nQpLevel = 0,
                            .nGop = 60,
                            .nBitrate = 400,
                            .nMinQp = 10,
                            .nMaxQp = 51,
                            .nMinIQp = 10,
                            .nMaxIQp = 51,
                            .nMinIprop = 10,
                            .nMaxIprop = 40,
                            .nFixQp =0,
                            .nIntraQpDelta = -2,
                            .nDeBreathQpDelta = -2,
                            .nIdrQpDeltaRange = 10
                        }
                    }
                }
            }
        },
        [1] = {
            .bEnable = AX_FALSE,
            .stAlgoAttr = {
                .nAlgoType = AX_OPAL_ALGO_HVCP | AX_OPAL_ALGO_MD | AX_OPAL_ALGO_OD,
            },
            .stSnsAttr = {
                .bFlip = AX_FALSE,
                .bMirror = AX_FALSE,
                .fFrameRate = 30,
                .eMode = AX_OPAL_SNS_SDR_MODE,
                .eDayNight = AX_OPAL_SNS_DAY_MODE,
                .eRotation = AX_OPAL_SNS_ROTATION_0,
                .stColorAttr = {
                    .bManual = AX_FALSE,
                    .fBrightness = 50,
                    .fSharpness = 0,
                    .fContrast = 50,
                    .fSaturation = 50,
                },
                .stLdcAttr = {
                    .bEnable = AX_FALSE,
                    .bAspect = AX_FALSE,
                    .nXRatio = 0,
                    .nYRatio = 0,
                    .nXYRatio = 0,
                    .nDistortionRatio = 1500,
                },
                .stDisAttr = {
                    .bEnable = AX_FALSE,
                    .bMotionEst = AX_FALSE,
                    .bMotionShare = AX_FALSE,
                    .nDelayFrameNum = 4,
                },
                .stEZoomAttr = {
                    .fRatio = 0,
                },
            },
            .stPipeAttr = {
                .stVideoChnAttr = {
                    // video channel 0
                    [0] = {
                        .bEnable = AX_TRUE,
                        .nMaxWidth = -1,        // -1: same as sensor width
                        .nMaxHeight = -1,       // -1: same as sensor height
                        .nWidth = -1,           // -1: same as sensor width
                        .nHeight = -1,          // -1: same as sensor height
                        .nFramerate = 30,       // -1: same as sensor framerate
                        .eType = AX_OPAL_VIDEO_CHN_TYPE_H264,
                        .stEncoderAttr = {
                            .eRcMode = AX_OPAL_VIDEO_RC_MODE_CBR,
                            .nQpLevel = 0,
                            .nGop = 60,          // setting as system default
                            .nBitrate = 4096,      // setting as system default
                            .nMinQp = 10,
                            .nMaxQp = 51,
                            .nMinIQp = 10,
                            .nMaxIQp = 51,
                            .nMinIprop = 10,
                            .nMaxIprop = 40,
                            .nFixQp = 0,
                            .nIntraQpDelta = -2,
                            .nDeBreathQpDelta = -2,
                            .nIdrQpDeltaRange = 10
                        }
                    },
                    // video channel 1 for algorithm
                    [1] = {
                        .bEnable = AX_TRUE,
                        .nMaxWidth = 1280,
                        .nMaxHeight = 720,
                        .nWidth = 1280,
                        .nHeight = 720,
                        .nFramerate = 10,
                        .eType = AX_OPAL_VIDEO_CHN_TYPE_ALGO,
                    },
                    // video channel 2
                    [2] = {
                        .bEnable = AX_TRUE,
                        .nMaxWidth = 720,
                        .nMaxHeight = 576,
                        .nWidth = 720,
                        .nHeight = 576,
                        .nFramerate = 30,       // -1: same as sensor framerate
                        .eType = AX_OPAL_VIDEO_CHN_TYPE_H264,
                        .stEncoderAttr = {
                            .eRcMode = AX_OPAL_VIDEO_RC_MODE_CBR,
                            .nQpLevel = 0,
                            .nGop = 60,
                            .nBitrate = 400,
                            .nMinQp = 10,
                            .nMaxQp = 51,
                            .nMinIQp = 10,
                            .nMaxIQp = 51,
                            .nMinIprop = 10,
                            .nMaxIprop = 40,
                            .nFixQp =0,
                            .nIntraQpDelta = -2,
                            .nDeBreathQpDelta = -2,
                            .nIdrQpDeltaRange = 10
                        }
                    }
                }
            }
        }
    },
    // audio attribute
    .stAudioAttr = {
        // audio dev common attribute
        .stDevCommAttr = {
            .eBitWidth = AX_OPAL_AUDIO_BIT_WIDTH_16,
            .eSampleRate = AX_OPAL_AUDIO_SAMPLE_RATE_16000,
            .nPeriodSize = 160
        },
        // audio pipe attribute
        .stCapAttr = {
            .bEnable = AX_TRUE,
            // audio capture device attribute
            .stDevAttr = {
                .nCardId = 0,
                .nDeviceId = 0,
                .fVolume = 1.0,
                .eLayoutMode = AX_OPAL_AUDIO_LAYOUT_MIC_REF, // AX620Q: eLayoutMode = AX_OPAL_AUDIO_LAYOUT_REF_MIC
                // audio 3A attribute
                .stVqeAttr = {
                    .stAecAttr = {
                        .eType = AX_OPAL_AUDIO_AEC_FIXED,
                        {
                            .stFixedAttr = {
                            .eMode = AX_OPAL_AUDIO_AEC_SPEAKERPHONE
                            }
                        }
                    },
                    .stAnsAttr = {
                        .bEnable = AX_TRUE,
                        .eLevel = AX_OPAL_AUDIO_ANS_AGGRESSIVENESS_LEVEL_HIGH,
                    },
                    .stAgcAttr = {
                        .bEnable = AX_TRUE,
                        .eAgcMode = AX_OPAL_AUDIO_AGC_MODE_FIXED_DIGITAL,
                        .nTargetLv = -3,
                        .nGain = 9
                    },
                    .stVadAttr = {
                        .bEnable = AX_FALSE,
                        .nVadLevel = 2
                    }
                }
            },
            // audio capture pipe attribute
            .stPipeAttr = {
                // audio pipe channel
                .stAudioChnAttr = {
                    // audio pipe channel 0
                    {
                        .bEnable = AX_TRUE,
                        .nBitRate = 48000,
                        .eSoundMode = AX_OPAL_AUDIO_SOUND_MODE_MONO,
                        .eType = PT_G711A,
                        // aac encoder
                        {
                            .stAacEncoder = {
                            .eAacType = AX_OPAL_AUDIO_AAC_TYPE_AAC_LC,
                            .eTransType =AX_OPAL_AUDIO_AAC_TRANS_TYPE_ADTS
                            }
                        }
                    },
                }
            }
        },
        // audio play attribute
        .stPlayAttr = {
            .bEnable = AX_TRUE,
            // audio play device attribute
            .stDevAttr = {
                .nCardId = 0,
                .nDeviceId = 1,
                .fVolume = 1.0,
                // audio 3A attribute
                .stVqeAttr = {
                    .stAnsAttr = {
                        .bEnable = AX_TRUE,
                        .eLevel = AX_OPAL_AUDIO_ANS_AGGRESSIVENESS_LEVEL_HIGH,
                    },
                    .stAgcAttr = {
                        .bEnable = AX_TRUE,
                        .eAgcMode = AX_OPAL_AUDIO_AGC_MODE_FIXED_DIGITAL,
                        .nTargetLv = -3,
                        .nGain = 9
                    }
                }
            },
            // audio play pipe attribute
            .stPipeAttr = {
                // audio pipe channel 0
                .stAudioChnAttr = {
                    // audio pipe channel 0
                    {
                        .bEnable = AX_TRUE,
                        .nBitRate = 48000,
                        .eSampleRate = AX_OPAL_AUDIO_SAMPLE_RATE_16000,
                        .eSoundMode = AX_OPAL_AUDIO_SOUND_MODE_MONO,
                        .eType = PT_G711A,
                        // default decoder
                        {
                            .stDefDecoder = {
                            .nReserved = 0
                            }
                        }
                    },
                }
            }
        }
    }
};


const AX_OPAL_VIDEO_SVC_PARAM_T g_stSvcParam[AX_OPAL_SNS_ID_BUTT] = {
    // sensor 0
    {
        .bEnable = AX_FALSE,
        .bAbsQp = AX_FALSE,
        .bSync = AX_FALSE,
        .stBgQpCfg = {
            .nIQp = 2,
            .nPQp = 2,
        },
        .stQpCfg = {
            // for body
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -1,
                    .nPQp = -1,
                }
            },
            // for vehicle
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -1,
                    .nPQp = -1,
                }
            },
            // for cycle
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -1,
                    .nPQp = -1,
                }
            },
            // for face
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -3,
                    .nPQp = -3,
                }
            },
            // for plate
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -3,
                    .nPQp = -3,
                }
            }
        }
    },
    // sensor 1
    {
        .bEnable = AX_FALSE,
        .bAbsQp = AX_FALSE,
        .bSync = AX_FALSE,
        .stBgQpCfg = {
            .nIQp = 2,
            .nPQp = 2,
        },
        .stQpCfg = {
            // for body
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -1,
                    .nPQp = -1,
                }
            },
            // for vehicle
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -1,
                    .nPQp = -1,
                }
            },
            // for cycle
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -1,
                    .nPQp = -1,
                }
            },
            // for face
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -3,
                    .nPQp = -3,
                }
            },
            // for plate
            {
                .bEnable = AX_FALSE,
                .stQpMap = {
                    .nIQp = -3,
                    .nPQp = -3,
                }
            }
        }
    }
};
