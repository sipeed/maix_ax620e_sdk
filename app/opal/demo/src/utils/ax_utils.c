/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "ax_opal_type.h"
#include "ax_utils.h"
#include "GlobalDef.h"
#include "ax_timer.h"
#include "ax_thread.h"
#include "ax_log.h"

#define PRINT_INTERVAL 10 //second

typedef struct _FPS_STAT_ITEM_T {
    AX_U64 nRcvFrmCount;  // Received Frames
    AX_U64 nPrdFrmCount;  // Period Received Frames
    AX_F32 fFinalFps;
}FPS_STAT_ITEM_T;

typedef struct _FPS_STAT_INFO_T{
    FPS_STAT_ITEM_T stFpsStat[AX_OPAL_SNS_ID_BUTT][AX_OPAL_VIDEO_CHN_BUTT];
    OPAL_THREAD_T* pThead;
    AX_U8 nSnsCount;
    AX_U8 bGotFrame;
}FPS_STAT_INFO_T;

static FPS_STAT_INFO_T g_stFpsStatInfo = {0};

AX_VOID FpsStatThreadFunc(AX_VOID *Param) {
    OPAL_THREAD_T *pThis = (OPAL_THREAD_T *) (g_stFpsStatInfo.pThead);

    AX_U64 nTickStart = OPAL_GetTickCount();
    AX_U64 nTickEnd = OPAL_GetTickCount();

    while (pThis->is_running) {

        if (!g_stFpsStatInfo.bGotFrame) {
            nTickStart = OPAL_GetTickCount();
            OPAL_mSleep(100);
            continue;
        }

        nTickEnd = OPAL_GetTickCount();
        if ((nTickEnd - nTickStart) >= PRINT_INTERVAL * 1000) {
            for (AX_S32 nSnsId = 0; nSnsId < g_stFpsStatInfo.nSnsCount; nSnsId++) {
                for (AX_S32 nSrcChn = 0; nSrcChn < AX_OPAL_VIDEO_CHN_BUTT; nSrcChn++) {
                    FPS_STAT_ITEM_T *pItem = &g_stFpsStatInfo.stFpsStat[nSnsId][nSrcChn];
                    if (pItem->nRcvFrmCount > 0) {
                        AX_F32 fps = pItem->nPrdFrmCount * 1.0 / PRINT_INTERVAL;
                        if (!pItem->fFinalFps) {
                            pItem->fFinalFps = fps;
                        }
                        pItem->fFinalFps = (pItem->fFinalFps + fps) / 2;
                        pItem->nPrdFrmCount = 0;
                        LOG_M_C("PRINT", "VENC[%d][%d] fps %5.2f, recv %lld", nSnsId, nSrcChn, fps, pItem->nRcvFrmCount > 0 ? (pItem->nRcvFrmCount - 1) : 0); /* Ignore the header frame */
                    }
                }
            }
            nTickStart = nTickEnd;
        }
        OPAL_mSleep(1000);
    }

    for (AX_S32 nSnsId = 0; nSnsId < g_stFpsStatInfo.nSnsCount; nSnsId++) {
        for (AX_S32 nSrcChn = 0; nSrcChn < AX_OPAL_VIDEO_CHN_BUTT; nSrcChn++) {
            FPS_STAT_ITEM_T *pItem = &g_stFpsStatInfo.stFpsStat[nSnsId][nSrcChn];
            if (pItem->nRcvFrmCount > 0) {
                LOG_M_C("PRINT", "Final VENC[%d][%d] fps %5.2f", nSnsId, nSrcChn, pItem->fFinalFps); /* Ignore the header frame */
            }
        }
    }
}

AX_S32 FpsStatInit(AX_U8 nSnsCount) {
    g_stFpsStatInfo.nSnsCount = nSnsCount;
    g_stFpsStatInfo.pThead = OPAL_CreateThread(FpsStatThreadFunc, NULL);
    OPAL_StartThread(g_stFpsStatInfo.pThead);
    return 0;
}

AX_S32 FpsStatDeinit() {
    OPAL_StopThread(g_stFpsStatInfo.pThead);
    OPAL_DestroyThread(g_stFpsStatInfo.pThead);
    return 0;
}

AX_S32 FpsStatReset(AX_U8 nSnsId, AX_U8 nSrcChn) {
    g_stFpsStatInfo.stFpsStat[nSnsId][nSrcChn].nRcvFrmCount = 0;
    g_stFpsStatInfo.stFpsStat[nSnsId][nSrcChn].nPrdFrmCount = 0;
    return 0;
}

AX_S32 FpsStatUpdate(AX_U8 nSnsId, AX_U8 nSrcChn) {
    g_stFpsStatInfo.bGotFrame = 1;
    if (g_stFpsStatInfo.stFpsStat[nSnsId][nSrcChn].nRcvFrmCount > 0) {
        g_stFpsStatInfo.stFpsStat[nSnsId][nSrcChn].nPrdFrmCount++;
    }
    g_stFpsStatInfo.stFpsStat[nSnsId][nSrcChn].nRcvFrmCount++;
    return 0;
}

AX_OPAL_VIDEO_CHN_TYPE_E Int2EncoderType(AX_U8 nEncodeType) {
    if (0 == nEncodeType) {
        return AX_OPAL_VIDEO_CHN_TYPE_H264;
    } else if (2 == nEncodeType) {
        return AX_OPAL_VIDEO_CHN_TYPE_H265;
    } else if (1 == nEncodeType) {
        return AX_OPAL_VIDEO_CHN_TYPE_MJPEG;
    }
    return AX_OPAL_VIDEO_CHN_TYPE_H264;
}

AX_U8 EncoderType2Int(AX_OPAL_VIDEO_CHN_TYPE_E eEncodeType) {
    if (AX_OPAL_VIDEO_CHN_TYPE_H264 == eEncodeType) {
        return 0;
    } else if (AX_OPAL_VIDEO_CHN_TYPE_H265 == eEncodeType) {
        return 2;
    } else if (AX_OPAL_VIDEO_CHN_TYPE_MJPEG == eEncodeType) {
        return 1;
    }
    return 0;
}

/* 2M: H264 2M, H265: 1M; 4M: H264 4M, H265: 2M; 4K: H264: 8M, H265:4M */
AX_U32 GetVencBitrate(AX_PAYLOAD_TYPE_E enType, AX_S32 nWidth, AX_S32 nHeight) {
    if (PT_H264 == enType || PT_MJPEG == enType) {
        // 4K
        if (nWidth >= 3840) {
            return 8192;
        }
        // 4M
        else if (nWidth >= 2560) {
            return 4096;
        }
        // 2M
        else if (nWidth >= 1920) {
            return 2048;
        }
        else {
            return ALIGN_COMM_DOWN(nWidth * nHeight / 1000, 100);
        }
    } else if (PT_H265 == enType) {
        // 4K
        if (nWidth >= 3840) {
            return 4096;
        }
        // 4M
        else if (nWidth >= 2560) {
            return 2048;
        }
        // 2M
        else if (nWidth >= 1920) {
            return 1024;
        }
        else {
            return ALIGN_COMM_DOWN(nWidth * nHeight / 1000, 100);
        }
    }
    return 4096;
}

AX_PAYLOAD_TYPE_E Int2PayloadType(AX_U32 nEncodeType) {
    if (0 == nEncodeType) {
        return PT_H264;
    } else if (2 == nEncodeType) {
        return PT_H265;
    } else if (1 == nEncodeType) {
        return PT_MJPEG;
    }

    return PT_H264;
}

AX_U32 PayloadType2Int(AX_PAYLOAD_TYPE_E eEncodeType) {
    if (PT_H264 == eEncodeType) {
        return 0;
    } else if (PT_H265 == eEncodeType) {
        return 2;
    } else if (PT_MJPEG == eEncodeType) {
        return 1;
    }

    return 0;
}

AX_PAYLOAD_TYPE_E EncoderType2PayloadType(AX_OPAL_VIDEO_CHN_TYPE_E eEncodeType) {
    if (AX_OPAL_VIDEO_CHN_TYPE_H264 == eEncodeType) {
        return PT_H264;
    } else if (AX_OPAL_VIDEO_CHN_TYPE_H265 == eEncodeType) {
        return PT_H265;
    } else if (AX_OPAL_VIDEO_CHN_TYPE_MJPEG == eEncodeType) {
        return PT_MJPEG;
    } else if (AX_OPAL_VIDEO_CHN_TYPE_JPEG == eEncodeType) {
        return PT_JPEG;
    }

    return PT_H264;
}

AX_OPAL_VIDEO_RC_MODE_E Str2RcMode(AX_CHAR* strRcModeName, AX_PAYLOAD_TYPE_E ePayloadType) {
    if (PT_H264 == ePayloadType) {
        if (strcmp(strRcModeName,"CBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_CBR;
        } else if (strcmp(strRcModeName,"VBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_VBR;
        } else if (strcmp(strRcModeName,"FIXQP") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_FIXQP;
        } else if (strcmp(strRcModeName,"AVBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_AVBR;
        } else if (strcmp(strRcModeName,"CVBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_CVBR;
        }
    } else if (PT_H265 == ePayloadType) {
        if (strcmp(strRcModeName,"CBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_CBR;
        } else if (strcmp(strRcModeName,"VBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_VBR;
        } else if (strcmp(strRcModeName,"FIXQP") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_FIXQP;
        } else if (strcmp(strRcModeName,"AVBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_AVBR;
        } else if (strcmp(strRcModeName,"CVBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_CVBR;
        }
    } else if (PT_MJPEG == ePayloadType) {
        if (strcmp(strRcModeName,"CBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_CBR;
        } else if (strcmp(strRcModeName,"VBR") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_VBR;
        } else if (strcmp(strRcModeName,"FIXQP") == 0) {
            return AX_OPAL_VIDEO_RC_MODE_FIXQP;
        }
    }

    return AX_OPAL_VIDEO_RC_MODE_CBR;
}

AX_U32 RcMode2Int(AX_OPAL_VIDEO_RC_MODE_E eRcMode) {
    AX_U32 nRcMode = 0;
    switch (eRcMode) {
        case AX_OPAL_VIDEO_RC_MODE_CBR:
            nRcMode = 0;
            break;
        case AX_OPAL_VIDEO_RC_MODE_VBR:
            nRcMode = 1;
            break;
        case AX_OPAL_VIDEO_RC_MODE_FIXQP:
            nRcMode = 2;
            break;
        case AX_OPAL_VIDEO_RC_MODE_AVBR:
            nRcMode = 3;
            break;
        case AX_OPAL_VIDEO_RC_MODE_CVBR:
            nRcMode = 4;
            break;
        default:
            break;
    }
    return nRcMode;
}

AX_OPAL_VIDEO_RC_MODE_E Int2RcMode(AX_U8 nEncoderType, AX_U8 nRcType) {
    switch (nEncoderType) {
        case 0: { /* H264 */
            switch (nRcType) {
                case 0: { /* CBR */
                    return AX_OPAL_VIDEO_RC_MODE_CBR;
                }
                case 1: { /* VBR */
                    return AX_OPAL_VIDEO_RC_MODE_VBR;
                }
                case 2: { /* FIXQP */
                    return AX_OPAL_VIDEO_RC_MODE_FIXQP;
                }
                case 3: { /* AVBR */
                    return AX_OPAL_VIDEO_RC_MODE_AVBR;
                }
                case 4: { /* CVBR */
                    return AX_OPAL_VIDEO_RC_MODE_CVBR;
                }
                default:
                    break;
            }
        }
        case 1: { /* MJPEG */
            switch (nRcType) {
                case 0: { /* CBR */
                    return AX_OPAL_VIDEO_RC_MODE_CBR;
                }
                case 1: { /* VBR */
                    return AX_OPAL_VIDEO_RC_MODE_VBR;
                }
                case 2: { /* FIXQP */
                    return AX_OPAL_VIDEO_RC_MODE_FIXQP;
                }
                default:
                    break;
            }
        }
        case 2: { /* H265 */
            switch (nRcType) {
                case 0: { /* CBR */
                    return AX_OPAL_VIDEO_RC_MODE_CBR;
                }
                case 1: { /* VBR */
                    return AX_OPAL_VIDEO_RC_MODE_VBR;
                }
                case 2: { /* FIXQP */
                    return AX_OPAL_VIDEO_RC_MODE_FIXQP;
                }
                case 3: { /* AVBR */
                    return AX_OPAL_VIDEO_RC_MODE_AVBR;
                }
                case 4: { /* CVBR */
                    return AX_OPAL_VIDEO_RC_MODE_CVBR;
                }
                default:
                    break;
            }
        }
        default:
            break;
    }
    return AX_OPAL_VIDEO_RC_MODE_BUTT;
}

