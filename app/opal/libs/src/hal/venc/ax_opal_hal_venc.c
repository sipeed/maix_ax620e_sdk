
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_venc.h"
#include "ax_opal_utils.h"
#include "ax_opal_log.h"

#include <unistd.h>

#define LOG_TAG  ("HAL_VENC")

#define VENC_DEFAULT_INCREASE_FRAME_OUTDEPTH (2)

static AX_BOOL g_stSvcEnableParam[AX_MAX_VENC_CHN_NUM][AX_VENC_SVC_RECT_TYPE_BUTT] = {{AX_FALSE}};
static AX_BOOL g_stSvcEnable[AX_MAX_VENC_CHN_NUM] = {AX_FALSE};
static AX_OPAL_VIDEO_SVC_PARAM_T g_stSvcParams[AX_MAX_VENC_CHN_NUM] = {0};

static AX_VENC_RC_MODE_E cvtOpalRcMode2RcMode(AX_VENC_RC_MODE_E eOrigRcMode, AX_OPAL_VIDEO_RC_MODE_E eSrcRcMode) {
    AX_VENC_RC_MODE_E eRcMode = AX_VENC_RC_MODE_BUTT;
    if (AX_VENC_RC_MODE_H264CBR <= eOrigRcMode && eOrigRcMode <= AX_VENC_RC_MODE_H264QPMAP) {
        switch (eSrcRcMode)
        {
        case AX_OPAL_VIDEO_RC_MODE_CBR:
            eRcMode = AX_VENC_RC_MODE_H264CBR;
            break;
        case AX_OPAL_VIDEO_RC_MODE_VBR:
            eRcMode = AX_VENC_RC_MODE_H264VBR;
            break;
        case AX_OPAL_VIDEO_RC_MODE_FIXQP:
            eRcMode = AX_VENC_RC_MODE_H264FIXQP;
            break;
        case AX_OPAL_VIDEO_RC_MODE_AVBR:
            eRcMode = AX_VENC_RC_MODE_H264AVBR;
            break;
        case AX_OPAL_VIDEO_RC_MODE_CVBR:
            eRcMode = AX_VENC_RC_MODE_H264CVBR;
            break;
        default:
            break;
        }
    }
    else if (AX_VENC_RC_MODE_MJPEGCBR <= eOrigRcMode && eOrigRcMode <= AX_VENC_RC_MODE_MJPEGFIXQP) {
        switch (eSrcRcMode)
        {
        case AX_OPAL_VIDEO_RC_MODE_CBR:
            eRcMode = AX_VENC_RC_MODE_MJPEGCBR;
            break;
        case AX_OPAL_VIDEO_RC_MODE_VBR:
            eRcMode = AX_VENC_RC_MODE_MJPEGVBR;
            break;
        case AX_OPAL_VIDEO_RC_MODE_FIXQP:
            eRcMode = AX_VENC_RC_MODE_MJPEGFIXQP;
            break;
        default:
            break;
        }
    }
    else if (AX_VENC_RC_MODE_H265CBR <= eOrigRcMode && eOrigRcMode <= AX_VENC_RC_MODE_H265QPMAP) {
        switch (eSrcRcMode)
        {
        case AX_OPAL_VIDEO_RC_MODE_CBR:
            eRcMode = AX_VENC_RC_MODE_H265CBR;
            break;
        case AX_OPAL_VIDEO_RC_MODE_VBR:
            eRcMode = AX_VENC_RC_MODE_H265VBR;
            break;
        case AX_OPAL_VIDEO_RC_MODE_FIXQP:
            eRcMode = AX_VENC_RC_MODE_H265FIXQP;
            break;
        case AX_OPAL_VIDEO_RC_MODE_AVBR:
            eRcMode = AX_VENC_RC_MODE_H265AVBR;
            break;
        case AX_OPAL_VIDEO_RC_MODE_CVBR:
            eRcMode = AX_VENC_RC_MODE_H265CVBR;
            break;
        default:
            break;
        }
    }
    return eRcMode;
}

static AX_VOID cvtOpalRcAttr2H264CBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H264_CBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H264_CBR_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    // pstDstRcAttr->u32StatTime
    pstDstRcAttr->u32BitRate = pstSrcRcAttr->nBitrate;

    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxIQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxIQp, 0, 51);
    pstDstRcAttr->u32MinIQp = ADAPTER_RANGE(pstSrcRcAttr->nMinIQp, 0, 51);
    pstDstRcAttr->u32MaxIprop = ADAPTER_RANGE(pstSrcRcAttr->nMaxIprop, 1, 100);
    pstDstRcAttr->u32MinIprop = ADAPTER_RANGE(pstSrcRcAttr->nMinIprop, 1, pstSrcRcAttr->nMaxIprop);
    pstDstRcAttr->s32IntraQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nIntraQpDelta, -51, 51);
    pstDstRcAttr->s32DeBreathQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nDeBreathQpDelta, -51, 51);
    pstDstRcAttr->u32IdrQpDeltaRange = ADAPTER_RANGE(pstSrcRcAttr->nIdrQpDeltaRange, 2, 10);
    // stQpmapInfo
}

static AX_VOID cvtOpalRcAttr2H264VBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H264_VBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H264_VBR_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    // u32StatTime
    pstDstRcAttr->u32MaxBitRate = pstSrcRcAttr->nBitrate;

    // enVQ
    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxIQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxIQp, 0, 51);
    pstDstRcAttr->u32MinIQp = ADAPTER_RANGE(pstSrcRcAttr->nMinIQp, 0, 51);
    pstDstRcAttr->s32IntraQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nIntraQpDelta, -51, 51);
    pstDstRcAttr->s32DeBreathQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nDeBreathQpDelta, -51, 51);
    pstDstRcAttr->u32IdrQpDeltaRange = ADAPTER_RANGE(pstSrcRcAttr->nIdrQpDeltaRange, 2, 10);
    // stQpmapInfo

    // u32SceneChgThr
    // u32ChangePos
}

static AX_VOID cvtOpalRcAttr2H264FIXQP(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H264_FIXQP_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H264_FIXQP_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    pstDstRcAttr->u32IQp = 25;
    pstDstRcAttr->u32PQp = 30;
    pstDstRcAttr->u32BQp = 32;
}

static AX_VOID cvtOpalRcAttr2H264AVBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H264_AVBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H264_AVBR_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    // u32StatTime
    pstDstRcAttr->u32MaxBitRate = pstSrcRcAttr->nBitrate;

    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxIQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxIQp, 0, 51);
    pstDstRcAttr->u32MinIQp = ADAPTER_RANGE(pstSrcRcAttr->nMinIQp, 0, 51);
    pstDstRcAttr->s32IntraQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nIntraQpDelta, -51, 51);
    pstDstRcAttr->s32DeBreathQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nDeBreathQpDelta, -51, 51);
    pstDstRcAttr->u32IdrQpDeltaRange = ADAPTER_RANGE(pstSrcRcAttr->nIdrQpDeltaRange, 2, 10);
    // stQpmapInfo
    // u32SceneChgThr
    // u32ChangePos
    // u32MinStillPercent
    // u32MaxStillQp
}

static AX_VOID cvtOpalRcAttr2H264CVBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H264_CVBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H264_CVBR_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    // u32StatTime

    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxIQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxIQp, 0, 51);
    pstDstRcAttr->u32MinIQp = ADAPTER_RANGE(pstSrcRcAttr->nMinIQp, 0, 51);
    pstDstRcAttr->u32MinQpDelta = 0;
    pstDstRcAttr->u32MaxQpDelta = 0;
    pstDstRcAttr->s32DeBreathQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nDeBreathQpDelta, -51, 51);
    pstDstRcAttr->u32IdrQpDeltaRange = ADAPTER_RANGE(pstSrcRcAttr->nIdrQpDeltaRange, 2, 10);
    pstDstRcAttr->u32MaxIprop = ADAPTER_RANGE(pstSrcRcAttr->nMaxIprop, 1, 100);
    pstDstRcAttr->u32MinIprop = ADAPTER_RANGE(pstSrcRcAttr->nMinIprop, 1, pstSrcRcAttr->nMaxIprop);

    pstDstRcAttr->u32MaxBitRate = pstSrcRcAttr->nBitrate;
    pstDstRcAttr->u32ShortTermStatTime = 2;
    pstDstRcAttr->u32LongTermStatTime = 60;
    pstDstRcAttr->u32LongTermMaxBitrate = (pstSrcRcAttr->nBitrate >> 1);
    pstDstRcAttr->u32LongTermMinBitrate = (pstSrcRcAttr->nBitrate >> 2);

    // u32ExtraBitPercent
    // u32LongTermStatTimeUnit
    pstDstRcAttr->s32IntraQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nIntraQpDelta, -51, 51);
    // stQpmapInfo
}

static AX_VOID cvtOpalRcAttr2H265CBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H265_CBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H265_CBR_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    // pstDstRcAttr->u32StatTime
    pstDstRcAttr->u32BitRate = pstSrcRcAttr->nBitrate;

    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxIQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxIQp, 0, 51);
    pstDstRcAttr->u32MinIQp = ADAPTER_RANGE(pstSrcRcAttr->nMinIQp, 0, 51);
    pstDstRcAttr->u32MaxIprop = ADAPTER_RANGE(pstSrcRcAttr->nMaxIprop, 1, 100);
    pstDstRcAttr->u32MinIprop = ADAPTER_RANGE(pstSrcRcAttr->nMinIprop, 1, pstSrcRcAttr->nMaxIprop);
    pstDstRcAttr->s32IntraQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nIntraQpDelta, -51, 51);
    pstDstRcAttr->s32DeBreathQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nDeBreathQpDelta, -51, 51);
    pstDstRcAttr->u32IdrQpDeltaRange = ADAPTER_RANGE(pstSrcRcAttr->nIdrQpDeltaRange, 2, 10);
    // stQpmapInfo
}

static AX_VOID cvtOpalRcAttr2H265VBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H265_VBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H264_VBR_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    // u32StatTime
    pstDstRcAttr->u32MaxBitRate = pstSrcRcAttr->nBitrate;

    // enVQ
    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxIQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxIQp, 0, 51);
    pstDstRcAttr->u32MinIQp = ADAPTER_RANGE(pstSrcRcAttr->nMinIQp, 0, 51);
    pstDstRcAttr->s32IntraQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nIntraQpDelta, -51, 51);
    pstDstRcAttr->s32DeBreathQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nDeBreathQpDelta, -51, 51);
    pstDstRcAttr->u32IdrQpDeltaRange = ADAPTER_RANGE(pstSrcRcAttr->nIdrQpDeltaRange, 2, 10);
    // stQpmapInfo

    // u32SceneChgThr
    // u32ChangePos
}

static AX_VOID cvtOpalRcAttr2H265FIXQP(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H265_FIXQP_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H265_FIXQP_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    pstDstRcAttr->u32IQp = 25;
    pstDstRcAttr->u32PQp = 30;
    pstDstRcAttr->u32BQp = 32;
}

static AX_VOID cvtOpalRcAttr2H265AVBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H265_AVBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H265_AVBR_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    // u32StatTime
    pstDstRcAttr->u32MaxBitRate = pstSrcRcAttr->nBitrate;

    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxIQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxIQp, 0, 51);
    pstDstRcAttr->u32MinIQp = ADAPTER_RANGE(pstSrcRcAttr->nMinIQp, 0, 51);
    pstDstRcAttr->s32IntraQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nIntraQpDelta, -51, 51);
    pstDstRcAttr->s32DeBreathQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nDeBreathQpDelta, -51, 51);
    pstDstRcAttr->u32IdrQpDeltaRange = ADAPTER_RANGE(pstSrcRcAttr->nIdrQpDeltaRange, 2, 10);
    // stQpmapInfo
    // u32SceneChgThr
    // u32ChangePos
    // u32MinStillPercent
    // u32MaxStillQp
}

static AX_VOID cvtOpalRcAttr2H265CVBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_H265_CVBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_H265_CVBR_T));
    pstDstRcAttr->u32Gop = pstSrcRcAttr->nGop;
    // u32StatTime

    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxIQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxIQp, 0, 51);
    pstDstRcAttr->u32MinIQp = ADAPTER_RANGE(pstSrcRcAttr->nMinIQp, 0, 51);
    pstDstRcAttr->u32MinQpDelta = 0;
    pstDstRcAttr->u32MaxQpDelta = 0;
    pstDstRcAttr->s32DeBreathQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nDeBreathQpDelta, -51, 51);
    pstDstRcAttr->u32IdrQpDeltaRange = ADAPTER_RANGE(pstSrcRcAttr->nIdrQpDeltaRange, 2, 10);
    pstDstRcAttr->u32MaxIprop = ADAPTER_RANGE(pstSrcRcAttr->nMaxIprop, 1, 100);
    pstDstRcAttr->u32MinIprop = ADAPTER_RANGE(pstSrcRcAttr->nMinIprop, 1, pstSrcRcAttr->nMaxIprop);

    pstDstRcAttr->u32MaxBitRate = pstSrcRcAttr->nBitrate;
    pstDstRcAttr->u32ShortTermStatTime = 2;
    pstDstRcAttr->u32LongTermStatTime = 60;
    pstDstRcAttr->u32LongTermMaxBitrate = (pstSrcRcAttr->nBitrate >> 1);
    pstDstRcAttr->u32LongTermMinBitrate = (pstSrcRcAttr->nBitrate >> 2);

    // u32ExtraBitPercent
    // u32LongTermStatTimeUnit
    pstDstRcAttr->s32IntraQpDelta = ADAPTER_RANGE(pstSrcRcAttr->nIntraQpDelta, -51, 51);
    // stQpmapInfo
}

static AX_U32 GetVencBufSize(AX_S32 nWidth, AX_S32 nHeight) {
    // 4M
    if (nWidth >= 2560) {
        return (AX_U32)(nWidth * nHeight / 2);
    }

    return (AX_U32)(nWidth * nHeight * 3 / 4);
}

static AX_VOID cvtOpalRcAttr2MJPEGCBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_MJPEG_CBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_MJPEG_CBR_T));
    pstDstRcAttr->u32BitRate = pstSrcRcAttr->nBitrate;
    pstDstRcAttr->u32StatTime = 1;
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
}

static AX_VOID cvtOpalRcAttr2MJPEGVBR(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_MJPEG_VBR_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_MJPEG_VBR_T));
    pstDstRcAttr->u32MaxBitRate = pstSrcRcAttr->nBitrate;
    pstDstRcAttr->u32StatTime = 1;
    pstDstRcAttr->u32MinQp = ADAPTER_RANGE(pstSrcRcAttr->nMinQp, 0, 51);
    pstDstRcAttr->u32MaxQp = ADAPTER_RANGE(pstSrcRcAttr->nMaxQp, 0, 51);
}

static AX_VOID cvtOpalRcAttr2MJPEGFIXQP(const AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr, AX_VENC_MJPEG_FIXQP_T *pstDstRcAttr) {
    memset(pstDstRcAttr, 0, sizeof(AX_VENC_MJPEG_FIXQP_T));
    pstDstRcAttr->s32FixedQp = 25;
}

AX_S32 AX_OPAL_HAL_VENC_Init(AX_VOID) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_VENC_MOD_ATTR_T stModAttr = {
        .enVencType = AX_VENC_MULTI_ENCODER,
        .stModThdAttr.u32TotalThreadNum = 2,
        .stModThdAttr.bExplicitSched = AX_FALSE,
    };

    nRet = AX_VENC_Init(&stModAttr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VENC_Init failed, ret=0x%x", nRet);
        return -1;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_Deinit(AX_VOID) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    nRet = AX_VENC_Deinit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VENC_Deinit failed, ret=0x%x", nRet);
        return -1;
    }

    LOG_M_D(LOG_TAG, "---");
    return 0;
}

AX_S32 AX_OPAL_HAL_VENC_CreateChn(AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation, AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_OPAL_VIDEO_RC_MODE_E eRcMode = pstChnAttr->stEncoderAttr.eRcMode;
    AX_OPAL_VIDEO_CHN_TYPE_E eType = pstChnAttr->eType;
    AX_OPAL_VIDEO_ENCODER_ATTR_T *pstSrcRcAttr = &pstChnAttr->stEncoderAttr;

    AX_VENC_CHN_ATTR_T stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    stVencChnAttr.stVencAttr.enMemSource = AX_MEMORY_SOURCE_CMM;
    stVencChnAttr.stVencAttr.u32MaxPicWidth = pstChnAttr->nMaxWidth;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = pstChnAttr->nMaxHeight;
    stVencChnAttr.stVencAttr.u8InFifoDepth = 1;
    stVencChnAttr.stVencAttr.u8OutFifoDepth = 1;
    stVencChnAttr.stVencAttr.u32PicWidthSrc = pstChnAttr->nWidth;
    stVencChnAttr.stVencAttr.u32PicHeightSrc = pstChnAttr->nHeight;
    if (AX_OPAL_SNS_ROTATION_90 == eRotation || AX_OPAL_SNS_ROTATION_270 == eRotation) {
       AX_SWAP(stVencChnAttr.stVencAttr.u32PicWidthSrc, stVencChnAttr.stVencAttr.u32PicHeightSrc);
    }
    stVencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate  = pstChnAttr->nFramerate;
    stVencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = pstChnAttr->nFramerate;
    stVencChnAttr.stVencAttr.enLinkMode = AX_LINK_MODE;

    if (IS_AX620Q) {
        stVencChnAttr.stVencAttr.bDeBreathEffect = AX_FALSE;
        stVencChnAttr.stVencAttr.bRefRingbuf = AX_TRUE;
    }
    else {
        stVencChnAttr.stVencAttr.bDeBreathEffect = AX_FALSE;
        stVencChnAttr.stVencAttr.bRefRingbuf = AX_FALSE;
    }

    stVencChnAttr.stVencAttr.u32BufSize = GetVencBufSize(pstChnAttr->nMaxWidth, pstChnAttr->nMaxHeight);

    switch (eType) {
        case AX_OPAL_VIDEO_CHN_TYPE_H264: {
            stVencChnAttr.stVencAttr.enType = PT_H264;
            stVencChnAttr.stVencAttr.enProfile = AX_VENC_H264_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enLevel = AX_VENC_H264_LEVEL_5_2;
            stVencChnAttr.stVencAttr.enTier = AX_VENC_HEVC_MAIN_TIER;

            if (eRcMode == AX_OPAL_VIDEO_RC_MODE_CBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H264_CBR_T stH264Cbr;
                cvtOpalRcAttr2H264CBR(pstSrcRcAttr, &stH264Cbr);
                memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_VBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264VBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H264_VBR_T stH264Vbr;
                cvtOpalRcAttr2H264VBR(pstSrcRcAttr, &stH264Vbr);
                memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_FIXQP) {
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264FIXQP;

                AX_VENC_H264_FIXQP_T stH264FixQp;
                cvtOpalRcAttr2H264FIXQP(pstSrcRcAttr, &stH264FixQp);
                memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_AVBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264AVBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H264_AVBR_T stH264Avbr;
                cvtOpalRcAttr2H264AVBR(pstSrcRcAttr, &stH264Avbr);
                memcpy(&stVencChnAttr.stRcAttr.stH264AVbr, &stH264Avbr, sizeof(AX_VENC_H264_AVBR_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_CVBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CVBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H264_CVBR_T stH264Cvbr;
                cvtOpalRcAttr2H264CVBR(pstSrcRcAttr, &stH264Cvbr);
                memcpy(&stVencChnAttr.stRcAttr.stH264CVbr, &stH264Cvbr, sizeof(AX_VENC_H264_CVBR_T));
            }
            else {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H264_CBR_T stH264Cbr;
                cvtOpalRcAttr2H264CBR(pstSrcRcAttr, &stH264Cbr);
                memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_T));
            }
        }
        break;
        case AX_OPAL_VIDEO_CHN_TYPE_H265: {
            stVencChnAttr.stVencAttr.enType = PT_H265;
            stVencChnAttr.stVencAttr.enProfile = AX_VENC_HEVC_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enLevel = AX_VENC_HEVC_LEVEL_5_1;
            stVencChnAttr.stVencAttr.enTier = AX_VENC_HEVC_MAIN_TIER;

            if (eRcMode == AX_OPAL_VIDEO_RC_MODE_CBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H265_CBR_T stH265Cbr;
                cvtOpalRcAttr2H265CBR(pstSrcRcAttr, &stH265Cbr);
                memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_VBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265VBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;

                AX_VENC_H264_VBR_T stH265Vbr;
                cvtOpalRcAttr2H265VBR(pstSrcRcAttr, &stH265Vbr);
                memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_FIXQP) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265FIXQP;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H265_FIXQP_T stH265FixQp;
                cvtOpalRcAttr2H265FIXQP(pstSrcRcAttr, &stH265FixQp);
                memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_AVBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265AVBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H265_AVBR_T stH265Avbr;
                cvtOpalRcAttr2H265AVBR(pstSrcRcAttr, &stH265Avbr);
                memcpy(&stVencChnAttr.stRcAttr.stH265AVbr, &stH265Avbr, sizeof(AX_VENC_H265_AVBR_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_CVBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CVBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H265_CVBR_T stH265Cvbr;
                cvtOpalRcAttr2H265CVBR(pstSrcRcAttr, &stH265Cvbr);
                memcpy(&stVencChnAttr.stRcAttr.stH265CVbr, &stH265Cvbr, sizeof(AX_VENC_H265_CVBR_T));
            }
            else {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_H265_CBR_T stH265Cbr;
                cvtOpalRcAttr2H265CBR(pstSrcRcAttr, &stH265Cbr);
                memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_T));
            }
        }
        break;
        case AX_OPAL_VIDEO_CHN_TYPE_MJPEG: {
            stVencChnAttr.stVencAttr.enType = PT_MJPEG;
            if (eRcMode == AX_OPAL_VIDEO_RC_MODE_CBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGCBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_MJPEG_CBR_T stMjpegCbr;
                cvtOpalRcAttr2MJPEGCBR(pstSrcRcAttr, &stMjpegCbr);
                memcpy(&stVencChnAttr.stRcAttr.stMjpegCbr, &stMjpegCbr, sizeof(AX_VENC_MJPEG_CBR_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_VBR) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGVBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_MJPEG_VBR_T stMjpegVbr;
                cvtOpalRcAttr2MJPEGVBR(pstSrcRcAttr, &stMjpegVbr);
                memcpy(&stVencChnAttr.stRcAttr.stMjpegVbr, &stMjpegVbr, sizeof(AX_VENC_MJPEG_VBR_T));
            }
            else if (eRcMode == AX_OPAL_VIDEO_RC_MODE_FIXQP) {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGFIXQP;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_MJPEG_FIXQP_T stMjpegFixQp;
                cvtOpalRcAttr2MJPEGFIXQP(pstSrcRcAttr, &stMjpegFixQp);
                memcpy(&stVencChnAttr.stRcAttr.stMjpegFixQp, &stMjpegFixQp, sizeof(AX_VENC_MJPEG_FIXQP_T));
            }
            else {
                stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGCBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                AX_VENC_MJPEG_CBR_T stMjpegCbr;
                cvtOpalRcAttr2MJPEGCBR(pstSrcRcAttr, &stMjpegCbr);
                memcpy(&stVencChnAttr.stRcAttr.stMjpegCbr, &stMjpegCbr, sizeof(AX_VENC_MJPEG_CBR_T));
            }
        }
        break;
        case AX_OPAL_VIDEO_CHN_TYPE_JPEG: {
            stVencChnAttr.stVencAttr.enType = PT_JPEG;
        }
        break;
        default:
            LOG_M_E(LOG_TAG, "Unrecognized payload type: %d.", eType);
        break;
    }

    if (eType != AX_OPAL_VIDEO_CHN_TYPE_JPEG){
        stVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_NORMALP;
    }

    nRet = AX_VENC_CreateChn(nChnId, &stVencChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_CreateChn failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    if (eType == AX_OPAL_VIDEO_CHN_TYPE_JPEG) {
        AX_VENC_JPEG_PARAM_T stJpegParam;
        memset(&stJpegParam, 0, sizeof(AX_VENC_JPEG_PARAM_T));
        nRet = AX_VENC_GetJpegParam(nChnId, &stJpegParam);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "[%d] AX_VENC_GetJpegParam failed, ret=0x%x", nChnId, nRet);
            return nRet;
        }

        stJpegParam.u32Qfactor = 90;
        nRet = AX_VENC_SetJpegParam(nChnId, &stJpegParam);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "[%d] AX_VENC_SetJpegParam failed, ret=0x%x", nChnId, nRet);
            return nRet;
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_DestroyChn(AX_S32 nChnId) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_S32 nRetry = 5;
    do {
        nRet = AX_VENC_DestroyChn(nChnId);
        if (AX_ERR_VENC_BUSY == nRet) {
            usleep(100 * 1000);
        }
        else {
            break;
        }
        nRetry -= 1;
    } while (nRetry > 0);

    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_DestroyChn failed, ret=0x%x", nChnId, nRet);
    }

    LOG_M_D(LOG_TAG, "---");
    return AX_SUCCESS;
}

AX_S32 AX_OPAL_HAL_VENC_StartRecv(AX_S32 nChnId) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_VENC_RECV_PIC_PARAM_T stRecvParam;
    memset(&stRecvParam, 0, sizeof(AX_VENC_RECV_PIC_PARAM_T));
    stRecvParam.s32RecvPicNum = -1;

    nRet = AX_VENC_StartRecvFrame(nChnId, &stRecvParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VENC_StartRecvFrame failed, ret=0x%x", nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_StopRecv(AX_S32 nChnId) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    nRet = AX_VENC_StopRecvFrame(nChnId);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_StopRecvFrame failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    nRet = AX_VENC_ResetChn(nChnId);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_ResetChn failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_SetJpegQf(AX_S32 nChnId, AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");


    AX_VENC_JPEG_PARAM_T stJpegParam;
    memset(&stJpegParam, 0, sizeof(AX_VENC_JPEG_PARAM_T));

    nRet = AX_VENC_GetJpegParam(nChnId, &stJpegParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_GetJpegParam failed,  nRet=0x%x", nChnId, nRet);
        return nRet;
    }

    stJpegParam.u32Qfactor = pstChnAttr->stEncoderAttr.nQpLevel;
    nRet = AX_VENC_SetJpegParam(nChnId, &stJpegParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_SetJpegParam failed,  nRet=0x%x", nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_SetResolution(AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation, AX_S32 nWidth, AX_S32 nHeight) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_VENC_CHN_ATTR_T tAttr;
    memset(&tAttr, 0x0, sizeof(AX_VENC_CHN_ATTR_T));
    nRet = AX_VENC_GetChnAttr(nChnId, &tAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_GetChnAttr failed,  nRet=0x%x", nChnId, nRet);
        return nRet;
    }

    tAttr.stVencAttr.u32PicWidthSrc = ALIGN_UP(nWidth, 2);;
    tAttr.stVencAttr.u32PicHeightSrc = nHeight;
    if (AX_OPAL_SNS_ROTATION_90 == eRotation || AX_OPAL_SNS_ROTATION_270 == eRotation) {
       AX_SWAP(tAttr.stVencAttr.u32PicWidthSrc, tAttr.stVencAttr.u32PicHeightSrc);
    }
    nRet = AX_VENC_SetChnAttr(nChnId, &tAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_SetChnAttr failed,  nRet=0x%x", nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_SetRotation(AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    // if (AX_OPAL_SNS_ROTATION_0 == eRotation || AX_OPAL_SNS_ROTATION_180 == eRotation || AX_OPAL_SNS_ROTATION_BUTT == eRotation) {
    //     return nRet;
    // }

    AX_VENC_CHN_ATTR_T tAttr;
    memset(&tAttr, 0x0, sizeof(AX_VENC_CHN_ATTR_T));
    nRet = AX_VENC_GetChnAttr(nChnId, &tAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_GetChnAttr failed,  nRet=0x%x", nChnId, nRet);
        return nRet;
    }

    AX_U32 nNewWidth = ALIGN_UP(tAttr.stVencAttr.u32PicHeightSrc, 2);
    AX_U32 nNewHeight = tAttr.stVencAttr.u32PicWidthSrc;
    tAttr.stVencAttr.u32PicWidthSrc = nNewWidth;
    tAttr.stVencAttr.u32PicHeightSrc = nNewHeight;
    nRet = AX_VENC_SetChnAttr(nChnId, &tAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_SetChnAttr failed,  nRet=0x%x", nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_SetFps(AX_S32 nChnId, AX_F32 fSrcFps, AX_F32 fDstFps) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_BOOL bFrcUpdate = AX_FALSE;

    AX_VENC_RC_PARAM_T stRcParam;
    nRet = AX_VENC_GetRcParam(nChnId, &stRcParam);

    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_GetRcParam failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    if (fSrcFps != -1) {
        stRcParam.stFrameRate.fSrcFrameRate = fSrcFps;
        bFrcUpdate = AX_TRUE;
    }

    if (fDstFps != -1) {
        stRcParam.stFrameRate.fDstFrameRate = fDstFps;
        bFrcUpdate = AX_TRUE;
    }

    if (bFrcUpdate) {
        nRet = AX_VENC_SetRcParam(nChnId, &stRcParam);

        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "[%d] AX_VENC_SetRcParam failed, ret=0x%x", nChnId, nRet);
            return nRet;
        }

        LOG_M_C(LOG_TAG, "[%d] Framerate[%f:%f]", nChnId, stRcParam.stFrameRate.fSrcFrameRate, stRcParam.stFrameRate.fDstFrameRate);

        AX_VENC_CHN_ATTR_T tAttr;
        nRet = AX_VENC_GetChnAttr(nChnId, &tAttr);

        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "[%d] AX_VENC_GetChnAttr failed, ret=0x%x", nChnId, nRet);
            return nRet;
        }

        if (stRcParam.stFrameRate.fDstFrameRate > stRcParam.stFrameRate.fSrcFrameRate
            && tAttr.stVencAttr.u8OutFifoDepth < VENC_DEFAULT_INCREASE_FRAME_OUTDEPTH) {
            tAttr.stVencAttr.u8OutFifoDepth = VENC_DEFAULT_INCREASE_FRAME_OUTDEPTH;
        }

        nRet = AX_VENC_SetChnAttr(nChnId, &tAttr);

        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "[%d] AX_VENC_SetChnAttr failed, ret=0x%x", nChnId, nRet);
            return nRet;
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AXOP_HAL_VENC_StopRecv(AX_S32 nChnId) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    nRet = AX_VENC_StopRecvFrame(nChnId);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_StopRecvFrame failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    nRet = AX_VENC_ResetChn(nChnId);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_ResetChn failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_RequestIDR(AX_S32 nChnId) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    nRet = AX_VENC_RequestIDR(nChnId, AX_TRUE);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d]AX_VENC_RequestIDR failed, ret=0x%x", nChnId, nRet);
        return nRet;
    } else {
        LOG_M_N(LOG_TAG, "[%d]AX_VENC_RequestIDR success.", nChnId);
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_VENC_SetRcAttr(AX_S32 nChnId, AX_OPAL_VIDEO_ENCODER_ATTR_T *pstRcAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_VENC_RC_PARAM_T stRcAttr;
    memset(&stRcAttr, 0x0, sizeof(AX_VENC_RC_PARAM_T));
    nRet = AX_VENC_GetRcParam(nChnId, &stRcAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_GetChnAttr failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    AX_VENC_RC_MODE_E eRcMode = cvtOpalRcMode2RcMode(stRcAttr.enRcMode, pstRcAttr->eRcMode);
    if (AX_VENC_RC_MODE_BUTT == eRcMode) {
        LOG_M_E(LOG_TAG, "[%d] Invalid or not supported RcMdoe %d", nChnId, stRcAttr.enRcMode);
        return -1;
    }

    stRcAttr.enRcMode = eRcMode;
    stRcAttr.s32FirstFrameStartQp = -1;
    if (AX_VENC_RC_MODE_H264CBR == eRcMode) {;
        AX_VENC_H264_CBR_T stH264Cbr;
        cvtOpalRcAttr2H264CBR(pstRcAttr, &stH264Cbr);
        memcpy(&stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_T));
    } else if (AX_VENC_RC_MODE_H264VBR == eRcMode) {
        AX_VENC_H264_VBR_T stH264Vbr;
        cvtOpalRcAttr2H264VBR(pstRcAttr, &stH264Vbr);
        memcpy(&stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_T));

    } else if (AX_VENC_RC_MODE_H264FIXQP == eRcMode) {
        AX_VENC_H264_FIXQP_T stH264FixQp;
        cvtOpalRcAttr2H264FIXQP(pstRcAttr, &stH264FixQp);
        memcpy(&stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
    } else if (AX_VENC_RC_MODE_H264AVBR == eRcMode) {
        AX_VENC_H264_AVBR_T stH264Avbr;
        cvtOpalRcAttr2H264AVBR(pstRcAttr, &stH264Avbr);
        memcpy(&stRcAttr.stH264AVbr, &stH264Avbr, sizeof(AX_VENC_H264_AVBR_T));
    } else if (AX_VENC_RC_MODE_H264CVBR == eRcMode) {
        AX_VENC_H264_CVBR_T stH264Cvbr;
        cvtOpalRcAttr2H264CVBR(pstRcAttr, &stH264Cvbr);
        memcpy(&stRcAttr.stH264CVbr, &stH264Cvbr, sizeof(AX_VENC_H264_CVBR_T));
    } else if (AX_VENC_RC_MODE_H265CBR == eRcMode) {
        AX_VENC_H265_CBR_T stH265Cbr;
        cvtOpalRcAttr2H265CBR(pstRcAttr, &stH265Cbr);
        memcpy(&stRcAttr.stH264Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_T));
    } else if (AX_VENC_RC_MODE_H265VBR == eRcMode) {
        AX_VENC_H265_VBR_T stH265Vbr;
        cvtOpalRcAttr2H265VBR(pstRcAttr, &stH265Vbr);
        memcpy(&stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_T));
    } else if (AX_VENC_RC_MODE_H265FIXQP == eRcMode) {
        AX_VENC_H265_FIXQP_T stH265FixQp;
        cvtOpalRcAttr2H265FIXQP(pstRcAttr, &stH265FixQp);
        memcpy(&stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
    } else if (AX_VENC_RC_MODE_H265AVBR == eRcMode) {
        AX_VENC_H265_AVBR_T stH265Avbr;
        cvtOpalRcAttr2H265AVBR(pstRcAttr, &stH265Avbr);
        memcpy(&stRcAttr.stH265AVbr, &stH265Avbr, sizeof(AX_VENC_H265_AVBR_T));
    } else if (AX_VENC_RC_MODE_H265CVBR == eRcMode) {
        AX_VENC_H265_CVBR_T stH265Cvbr;
        cvtOpalRcAttr2H265CVBR(pstRcAttr, &stH265Cvbr);
        memcpy(&stRcAttr.stH265CVbr, &stH265Cvbr, sizeof(AX_VENC_H265_CVBR_T));
    } else if (AX_VENC_RC_MODE_MJPEGCBR == eRcMode) {
        AX_VENC_MJPEG_CBR_T stMjpegCbr;
        cvtOpalRcAttr2MJPEGCBR(pstRcAttr, &stMjpegCbr);
        memcpy(&stRcAttr.stMjpegCbr, &stMjpegCbr, sizeof(AX_VENC_MJPEG_CBR_T));
    } else if (AX_VENC_RC_MODE_MJPEGVBR == eRcMode) {
        AX_VENC_MJPEG_VBR_T stMjpegVbr;
        cvtOpalRcAttr2MJPEGVBR(pstRcAttr, &stMjpegVbr);
        memcpy(&stRcAttr.stMjpegVbr, &stMjpegVbr, sizeof(AX_VENC_MJPEG_VBR_T));
    } else if (AX_VENC_RC_MODE_MJPEGFIXQP == eRcMode) {
        AX_VENC_MJPEG_FIXQP_T stMjpegFixQp;
        cvtOpalRcAttr2MJPEGFIXQP(pstRcAttr, &stMjpegFixQp);
        memcpy(&stRcAttr.stMjpegFixQp, &stMjpegFixQp, sizeof(AX_VENC_MJPEG_FIXQP_T));
    }

    nRet = AX_VENC_SetRcParam(nChnId, &stRcAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_SetRcParam failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

#if 0
AX_S32 AX_OPAL_HAL_VENC_GetRcAttr(AX_U32 nChnId, AX_OPAL_VIDEO_ENCODER_ATTR_T *pstRcAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_VENC_RC_PARAM_T tRcParam;
    memset(&tRcParam, 0, sizeof(AX_VENC_RC_PARAM_T));
    nRet = AX_VENC_GetRcParam(nChnId, &tRcParam);
    if (AX_SUCCESS != nRet){
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_GetRcParam failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }

    pstRcAttr->eRcMode = tRcParam.enRcMode;
    if (tRcParam.enRcMode == AX_VENC_RC_MODE_H264CBR){
        pstRcAttr->nGop = tRcParam.stH264Cbr.u32Gop;
        pstRcAttr->nBitrate = tRcParam.stH264Cbr.u32BitRate;
        pstRcAttr->nMaxIQp = tRcParam.stH264Cbr.u32MaxIQp;
        pstRcAttr->nMinIQp = tRcParam.stH264Cbr.u32MinIQp;
        pstRcAttr->nMaxQp = tRcParam.stH264Cbr.u32MaxQp;
        pstRcAttr->nMinQp = tRcParam.stH264Cbr.u32MinQp;
    } else if (tRcParam.enRcMode == AX_VENC_RC_MODE_H264VBR){
        pstRcAttr->nGop = tRcParam.stH264Vbr.u32Gop;
        pstRcAttr->nBitrate = tRcParam.stH264Vbr.u32MaxBitRate;
        pstRcAttr->nMaxIQp = tRcParam.stH264Vbr.u32MaxIQp;
        pstRcAttr->nMinIQp = tRcParam.stH264Vbr.u32MinIQp;
        pstRcAttr->nMaxQp = tRcParam.stH264Vbr.u32MaxQp;
        pstRcAttr->nMinQp = tRcParam.stH264Vbr.u32MinQp;
    } else if (tRcParam.enRcMode == AX_VENC_RC_MODE_H264AVBR){
        pstRcAttr->nGop = tRcParam.stH264AVbr.u32Gop;
        pstRcAttr->nBitrate = tRcParam.stH264AVbr.u32MaxBitRate;
        pstRcAttr->nMaxIQp = tRcParam.stH264AVbr.u32MaxIQp;
        pstRcAttr->nMinIQp = tRcParam.stH264AVbr.u32MinIQp;
        pstRcAttr->nMaxQp = tRcParam.stH264AVbr.u32MaxQp;
        pstRcAttr->nMinQp = tRcParam.stH264AVbr.u32MinQp;
    } else if (tRcParam.enRcMode == AX_VENC_RC_MODE_H264CVBR){
        pstRcAttr->nGop = tRcParam.stH264CVbr.u32Gop;
        pstRcAttr->nBitrate = tRcParam.stH264CVbr.u32MaxBitRate;
        pstRcAttr->nMaxIQp = tRcParam.stH264CVbr.u32MaxIQp;
        pstRcAttr->nMinIQp = tRcParam.stH264CVbr.u32MinIQp;
        pstRcAttr->nMaxQp = tRcParam.stH264CVbr.u32MaxQp;
        pstRcAttr->nMinQp = tRcParam.stH264CVbr.u32MinQp;
    } else if (tRcParam.enRcMode == AX_VENC_RC_MODE_H265CBR){
        pstRcAttr->nGop = tRcParam.stH265Cbr.u32Gop;
        pstRcAttr->nBitrate = tRcParam.stH265Cbr.u32BitRate;
        pstRcAttr->nMaxIQp = tRcParam.stH265Cbr.u32MaxIQp;
        pstRcAttr->nMinIQp = tRcParam.stH265Cbr.u32MinIQp;
        pstRcAttr->nMaxQp = tRcParam.stH265Cbr.u32MaxQp;
        pstRcAttr->nMinQp = tRcParam.stH265Cbr.u32MinQp;
    } else if (tRcParam.enRcMode == AX_VENC_RC_MODE_H265VBR){
        pstRcAttr->nGop = tRcParam.stH265Vbr.u32Gop;
        pstRcAttr->nBitrate = tRcParam.stH265Vbr.u32MaxBitRate;
        pstRcAttr->nMaxIQp = tRcParam.stH265Vbr.u32MaxIQp;
        pstRcAttr->nMinIQp = tRcParam.stH265Vbr.u32MinIQp;
        pstRcAttr->nMaxQp = tRcParam.stH265Vbr.u32MaxQp;
        pstRcAttr->nMinQp = tRcParam.stH265Vbr.u32MinQp;
    } else if (tRcParam.enRcMode == AX_VENC_RC_MODE_H265AVBR){
        pstRcAttr->nGop = tRcParam.stH265AVbr.u32Gop;
        pstRcAttr->nBitrate = tRcParam.stH265AVbr.u32MaxBitRate;
        pstRcAttr->nMaxIQp = tRcParam.stH265AVbr.u32MaxIQp;
        pstRcAttr->nMinIQp = tRcParam.stH265AVbr.u32MinIQp;
        pstRcAttr->nMaxQp = tRcParam.stH265AVbr.u32MaxQp;
        pstRcAttr->nMinQp = tRcParam.stH265AVbr.u32MinQp;
    }else if (tRcParam.enRcMode == AX_VENC_RC_MODE_H265CVBR){
        pstRcAttr->nGop = tRcParam.stH265CVbr.u32Gop;
        pstRcAttr->nBitrate = tRcParam.stH265CVbr.u32MaxBitRate;
        pstRcAttr->nMaxIQp = tRcParam.stH265CVbr.u32MaxIQp;
        pstRcAttr->nMinIQp = tRcParam.stH265CVbr.u32MinIQp;
        pstRcAttr->nMaxQp = tRcParam.stH265CVbr.u32MaxQp;
        pstRcAttr->nMinQp = tRcParam.stH265CVbr.u32MinQp;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}
#endif

AX_S32 AX_OPAL_HAL_VENC_SetChnAttr(AX_S32 nChnId, AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_VENC_CHN_ATTR_T stChnAttr;
    memset(&stChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));
    nRet = AX_VENC_GetChnAttr(nChnId, &stChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_GetChnAttr failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }else{
        if (stChnAttr.stVencAttr.u32PicWidthSrc != pstChnAttr->nWidth
            || stChnAttr.stVencAttr.u32PicHeightSrc != pstChnAttr->nHeight)

        stChnAttr.stVencAttr.u32PicWidthSrc = ALIGN_UP(pstChnAttr->nWidth, 128);
        stChnAttr.stVencAttr.u32PicHeightSrc = pstChnAttr->nHeight;
        nRet = AX_VENC_SetChnAttr(nChnId, &stChnAttr);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "[%d] AX_VENC_SetChnAttr failed, ret=0x%x", nChnId, nRet);
            return nRet;
        }
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

#if 0
static AX_OPAL_VIDEO_CHN_TYPE_E cvtChnType2Opal(AX_PAYLOAD_TYPE_E eType) {
    switch (eType) {
        case PT_H264:
            return AX_OPAL_VIDEO_CHN_TYPE_H264;
        case PT_H265:
            return AX_OPAL_VIDEO_CHN_TYPE_H265;
        case PT_MJPEG:
            return AX_OPAL_VIDEO_CHN_TYPE_MJPEG;
        case PT_JPEG:
            return AX_OPAL_VIDEO_CHN_TYPE_JPEG;
        default:
            break;
    }

    return AX_OPAL_VIDEO_CHN_TYPE_BUTT;
}

AX_S32 AX_OPAL_HAL_VENC_GetChnAttr(AX_S32 nChnId, AX_OPAL_VIDEO_CHN_ATTR_T *pstChnAttr) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_VENC_CHN_ATTR_T stChnAttr;
    memset(&stChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    nRet = AX_VENC_GetChnAttr(nChnId, &stChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VENC_GetChnAttr failed, ret=0x%x", nChnId, nRet);
        return nRet;
    }else{
        pstChnAttr->eType = cvtChnType2Opal(stChnAttr.stVencAttr.enType);
        pstChnAttr->nWidth = stChnAttr.stVencAttr.u32PicWidthSrc;
        pstChnAttr->nHeight = stChnAttr.stVencAttr.u32PicHeightSrc;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}
#endif

AX_OPAL_VIDEO_NALU_TYPE_E AX_OPAL_HAL_VENC_CvtNaluType(AX_PAYLOAD_TYPE_E eType, AX_VENC_DATA_TYPE_U nVencNaluType, AX_BOOL bIFrame) {
    if (eType == PT_H264) {
        if (bIFrame) {
            return AX_OPAL_VIDEO_H264_NALU_ISLICE;
        } else {
            switch (nVencNaluType.enH264EType) {
                case AX_H264E_NALU_BSLICE:
                    return AX_OPAL_VIDEO_H264_NALU_BSLICE;
                case AX_H264E_NALU_PSLICE:
                    return AX_OPAL_VIDEO_H264_NALU_PSLICE;
                case AX_H264E_NALU_ISLICE:
                    return AX_OPAL_VIDEO_H264_NALU_ISLICE;
                case AX_H264E_NALU_IDRSLICE:
                    return AX_OPAL_VIDEO_H264_NALU_IDRSLICE;
                case AX_H264E_NALU_SEI:
                    return AX_OPAL_VIDEO_VIDEO_H264_NALU_SEI;
                case AX_H264E_NALU_SPS:
                    return AX_OPAL_VIDEO_H264_NALU_SPS;
                case AX_H264E_NALU_PPS:
                    return AX_OPAL_VIDEO_H264_NALU_PPS;
                case AX_H264E_NALU_BUTT:
                    return AX_OPAL_VIDEO_NALU_TYPE_BUTT;
                default:
                    return AX_OPAL_VIDEO_NALU_TYPE_BUTT;
            }
        }
    } else if (eType == PT_H265) {
        if (bIFrame) {
            return AX_OPAL_VIDEO_H265_NALU_ISLICE;
        } else {
        }
        switch (nVencNaluType.enH265EType) {
            case AX_H265E_NALU_BSLICE:
                return AX_OPAL_VIDEO_H265_NALU_BSLICE;
            case AX_H265E_NALU_PSLICE:
                return AX_OPAL_VIDEO_H265_NALU_PSLICE;
            case AX_H265E_NALU_ISLICE:
                return AX_OPAL_VIDEO_H265_NALU_ISLICE;
            case AX_H265E_NALU_TSA_R:
                return AX_OPAL_VIDEO_H265_NALU_TSA_R;
            case AX_H265E_NALU_IDRSLICE:
                return AX_OPAL_VIDEO_H265_NALU_IDRSLICE;
            case AX_H265E_NALU_VPS:
                return AX_OPAL_VIDEO_H265_NALU_VPS;
            case AX_H265E_NALU_SPS:
                return AX_OPAL_VIDEO_H265_NALU_SPS;
            case AX_H265E_NALU_PPS:
                return AX_OPAL_VIDEO_H265_NALU_PPS;
            case AX_H265E_NALU_SEI:
                return AX_OPAL_VIDEO_H265_NALU_SEI;
            default:
                return AX_OPAL_VIDEO_NALU_TYPE_BUTT;
        }
    }

    return AX_OPAL_VIDEO_NALU_TYPE_BUTT;
}

AX_S32 AX_OPAL_HAL_VENC_SetSvcParam(AX_S32 nChnId, const AX_OPAL_VIDEO_SVC_PARAM_T* pstParam) {
    AX_S32 s32Ret = AX_SUCCESS;
    if (nChnId >= AX_MAX_VENC_CHN_NUM) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }

    if (!pstParam) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    s32Ret = AX_VENC_EnableSvc(nChnId, pstParam->bEnable);
    if (s32Ret != 0) {
        LOG_M_E(LOG_TAG, "AX_VENC_EnableSvc[%d] failed, ret=0x%x", nChnId, s32Ret);
        return s32Ret;
    }

    g_stSvcEnable[nChnId] = pstParam->bEnable;


    AX_VENC_SVC_PARAM_T tSvcParam = {0};
    // s32Ret = AX_VENC_GetSvcParam(nChnId, &tSvcParam);
    // if (s32Ret != 0) {
    //     LOG_M_E(LOG_TAG, "AX_VENC_GetSvcParam[%d] failed, ret=0x%x", nChnId, s32Ret);
    //     return s32Ret;
    // }
    tSvcParam.bAbsQp = pstParam->bAbsQp;
    tSvcParam.bSync = pstParam->bSync;
    tSvcParam.stBgQpCfg.iQp = pstParam->stBgQpCfg.nIQp;
    tSvcParam.stBgQpCfg.pQp = pstParam->stBgQpCfg.nPQp;
    tSvcParam.u32RectTypeNum = AX_OPAL_VIDEO_SVC_REGION_TYPE_BUTT;
    if (pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE0].bEnable) {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE0].iQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE0].stQpMap.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE0].pQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE0].stQpMap.nPQp;
    } else {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE0].iQp = pstParam->stBgQpCfg.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE0].pQp = pstParam->stBgQpCfg.nPQp;
    }
    g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE0] = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE0].bEnable;

    if (pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE1].bEnable) {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE1].iQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE1].stQpMap.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE1].pQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE1].stQpMap.nPQp;
    } else {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE1].iQp = pstParam->stBgQpCfg.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE1].pQp = pstParam->stBgQpCfg.nPQp;
    }
    g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE1] = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE1].bEnable;

    if (pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE2].bEnable) {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE2].iQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE2].stQpMap.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE2].pQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE2].stQpMap.nPQp;
    } else {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE2].iQp = pstParam->stBgQpCfg.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE2].pQp = pstParam->stBgQpCfg.nPQp;
    }
    g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE2] = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE2].bEnable;

    if (pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE3].bEnable) {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE3].iQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE3].stQpMap.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE3].pQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE3].stQpMap.nPQp;
    } else {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE3].iQp = pstParam->stBgQpCfg.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE3].pQp = pstParam->stBgQpCfg.nPQp;
    }
    g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE3] = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE3].bEnable;

    if (pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE4].bEnable) {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE4].iQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE4].stQpMap.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE4].pQp = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE4].stQpMap.nPQp;
    } else {
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE4].iQp = pstParam->stBgQpCfg.nIQp;
        tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE4].pQp = pstParam->stBgQpCfg.nPQp;
    }
    g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE4] = pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE4].bEnable;

    g_stSvcParams[nChnId] = *pstParam;

    if (pstParam->bEnable) {
        s32Ret = AX_VENC_SetSvcParam(nChnId, &tSvcParam);
        if (s32Ret != 0) {
            LOG_M_E(LOG_TAG, "AX_VENC_SetSvcParam[%d] failed, ret=0x%x", nChnId, s32Ret);
        }
    }
    return s32Ret;
}

AX_S32 AX_OPAL_HAL_VENC_GetSvcParam(AX_S32 nChnId, AX_OPAL_VIDEO_SVC_PARAM_T* pstParam) {
    AX_S32 s32Ret = AX_SUCCESS;
    if (nChnId >= AX_MAX_VENC_CHN_NUM) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }

    if (!pstParam) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    pstParam->bEnable = g_stSvcEnable[nChnId];

    if (!pstParam->bEnable) {
        *pstParam = g_stSvcParams[nChnId];
        return s32Ret;
    }

    AX_VENC_SVC_PARAM_T tSvcParam = {0};
    s32Ret = AX_VENC_GetSvcParam(nChnId, &tSvcParam);
    if (s32Ret != 0) {
        LOG_M_E(LOG_TAG, "AX_VENC_GetSvcParam[%d] failed, ret=0x%x", nChnId, s32Ret);
        return s32Ret;
    }

    pstParam->bAbsQp = tSvcParam.bAbsQp;
    pstParam->bSync = tSvcParam.bSync;

    pstParam->stBgQpCfg.nIQp = tSvcParam.stBgQpCfg.iQp;
    pstParam->stBgQpCfg.nPQp = tSvcParam.stBgQpCfg.pQp;

    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE0].stQpMap.nIQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE0].iQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE0].stQpMap.nPQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE0].pQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE0].bEnable = g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE0];


    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE1].stQpMap.nIQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE1].iQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE1].stQpMap.nPQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE1].pQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE1].bEnable = g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE1];


    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE2].stQpMap.nIQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE2].iQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE2].stQpMap.nPQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE2].pQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE2].bEnable = g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE2];


    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE3].stQpMap.nIQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE3].iQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE3].stQpMap.nPQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE3].pQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE3].bEnable = g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE3];


    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE4].stQpMap.nIQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE4].iQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE4].stQpMap.nPQp = tSvcParam.stQpCfg[AX_VENC_SVC_RECT_TYPE4].pQp;
    pstParam->stQpCfg[AX_OPAL_VIDEO_SVC_REGION_TYPE4].bEnable = g_stSvcEnableParam[nChnId][AX_VENC_SVC_RECT_TYPE4];

    return s32Ret;
}

AX_S32 AX_OPAL_HAL_VENC_SetSvcRegion(AX_S32 nChnId, const AX_OPAL_VIDEO_SVC_REGION_T* pstRegion) {
    AX_S32 s32Ret = AX_SUCCESS;
    if (nChnId >= AX_MAX_VENC_CHN_NUM) {
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }

    if (!pstRegion) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    if (!g_stSvcEnable[nChnId]) {
        return s32Ret;
    }

    AX_VENC_SVC_REGION_T stRegion = {0};
    stRegion.u64Pts = pstRegion->u64Pts;
    stRegion.u32RectNum = pstRegion->nItemSize <= AX_VENC_SVC_MAX_RECT_NUM ? pstRegion->nItemSize : AX_VENC_SVC_MAX_RECT_NUM;

    AX_U32 nRectNum = 0;
    for (AX_S32 i = 0; i < stRegion.u32RectNum; i++) {
        if (pstRegion->pstItems[i].eRegionType < AX_OPAL_VIDEO_SVC_REGION_TYPE_BUTT && g_stSvcEnableParam[nChnId][pstRegion->pstItems[i].eRegionType] == AX_TRUE) {
            stRegion.stRect[nRectNum].fX = pstRegion->pstItems[i].stRect.fX;
            stRegion.stRect[nRectNum].fY = pstRegion->pstItems[i].stRect.fY;
            stRegion.stRect[nRectNum].fWidth = pstRegion->pstItems[i].stRect.fW;
            stRegion.stRect[nRectNum].fHeight = pstRegion->pstItems[i].stRect.fH;
            stRegion.enRectType[nRectNum] = (AX_VENC_SVC_RECT_TYPE_E) pstRegion->pstItems[i].eRegionType;
            nRectNum ++;
        }
    }
    stRegion.u32RectNum = nRectNum;

    s32Ret = AX_VENC_SetSvcRegion(nChnId, &stRegion);

    return s32Ret;
}