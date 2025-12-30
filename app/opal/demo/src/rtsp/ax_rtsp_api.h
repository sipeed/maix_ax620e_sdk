/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_RTSP_WRAPPER_H__
#define __AX_RTSP_WRAPPER_H__

#include "ax_rtsp_def.h"

#ifdef __cplusplus
extern "C" {
#endif

AX_S32 AX_RTSP_Init(AX_RTSP_HANDLE *pHandle);
AX_S32 AX_RTSP_Deinit(AX_RTSP_HANDLE pHandle);
AX_S32 AX_RTSP_AddSessionAttr(AX_RTSP_HANDLE pHandle, AX_U32 nChn, const AX_RTSP_SESS_ATTR_T *pstSessAttr);
AX_S32 AX_RTSP_GetSessionAttr(AX_RTSP_HANDLE pHandle, AX_U32 nChn, AX_RTSP_SESS_ATTR_T* pstSessAttr);
AX_S32 AX_RTSP_UpdateSessionAttr(AX_RTSP_HANDLE pHandle, AX_U32 nChn, const AX_RTSP_SESS_ATTR_T* pstSessAttr);
AX_S32 AX_RTSP_RestartSessions(AX_RTSP_HANDLE pHandle);
AX_S32 AX_RTSP_Start(AX_RTSP_HANDLE pHandle);
AX_S32 AX_RTSP_Stop(AX_RTSP_HANDLE pHandle);
AX_S32 AX_RTSP_SendVideo(AX_RTSP_HANDLE pHandle, AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts, AX_BOOL bIFrame);
AX_S32 AX_RTSP_SendAudio(AX_RTSP_HANDLE pHandle, AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts);
AX_S32 AX_RTSP_GetIP(AX_CHAR* pIPAddr, AX_U32 nLen);

#ifdef __cplusplus
}
#endif

#endif  // __AX_RTSP_WRAPPER_H__
