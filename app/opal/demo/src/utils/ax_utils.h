/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_UTILS_H
#define _AX_UTILS_H

#include "ax_opal_type.h"

#ifdef __cplusplus
extern "C" {
#endif

AX_S32                   FpsStatInit(AX_U8 nSnsCount);
AX_S32                   FpsStatDeinit();
AX_S32                   FpsStatReset(AX_U8 nSnsId, AX_U8 nSrcChn);
AX_S32                   FpsStatUpdate(AX_U8 nSnsId, AX_U8 nSrcChn);

AX_OPAL_VIDEO_CHN_TYPE_E Int2EncoderType(AX_U8 nEncodeType);
AX_U8                    EncoderType2Int(AX_OPAL_VIDEO_CHN_TYPE_E eEncodeType);
/* 2M: H264 2M, H265: 1M; 4M: H264 4M, H265: 2M; 4K: H264: 8M, H265:4M */
AX_U32                   GetVencBitrate(AX_PAYLOAD_TYPE_E enType, AX_S32 nWidth, AX_S32 nHeight);
AX_PAYLOAD_TYPE_E        Int2PayloadType(AX_U32 nEncodeType);
AX_U32                   PayloadType2Int(AX_PAYLOAD_TYPE_E eEncodeType);
AX_PAYLOAD_TYPE_E        EncoderType2PayloadType(AX_OPAL_VIDEO_CHN_TYPE_E eEncodeType);
AX_OPAL_VIDEO_RC_MODE_E  Str2RcMode(AX_CHAR* strRcModeName, AX_PAYLOAD_TYPE_E ePayloadType);
AX_U32                   RcMode2Int(AX_OPAL_VIDEO_RC_MODE_E eRcMode);
AX_OPAL_VIDEO_RC_MODE_E  Int2RcMode(AX_U8 nEncoderType, AX_U8 nRcType);

#ifdef __cplusplus
}
#endif

#endif // #define _AX_UTILS_H


