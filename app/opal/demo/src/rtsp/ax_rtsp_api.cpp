/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_rtsp_api.h"
#include "AXRtspServer.h"

AX_S32 AX_RTSP_Init(AX_RTSP_HANDLE *pHandle)
{
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = new CAXRtspServer();

    if (pHandle) {
        *pHandle = (AX_RTSP_HANDLE)rtspServer;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_RTSP_Start(AX_RTSP_HANDLE pHandle)
{
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer) {
        bRet = rtspServer->Start();
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_RTSP_Stop(AX_RTSP_HANDLE pHandle)
{
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer) {
        rtspServer->Stop();

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_RTSP_Deinit(AX_RTSP_HANDLE pHandle)
{
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer) {
        delete rtspServer;

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_RTSP_AddSessionAttr(AX_RTSP_HANDLE pHandle, AX_U32 nChn, const AX_RTSP_SESS_ATTR_T *pstSessAttr) {
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer && pstSessAttr) {
        rtspServer->AddSessionAttr(nChn, *pstSessAttr);

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_RTSP_GetSessionAttr(AX_RTSP_HANDLE pHandle, AX_U32 nChn, AX_RTSP_SESS_ATTR_T* pstSessAttr) {
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer && pstSessAttr) {
        rtspServer->GetSessionAttr(nChn, *pstSessAttr);
        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_RTSP_UpdateSessionAttr(AX_RTSP_HANDLE pHandle, AX_U32 nChn, const AX_RTSP_SESS_ATTR_T* pstSessAttr) {
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer && pstSessAttr) {
        rtspServer->UpdateSessionAttr(nChn, *pstSessAttr);
        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_RTSP_RestartSessions(AX_RTSP_HANDLE pHandle) {
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer) {
        rtspServer->RestartSessions();
        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}


AX_S32 AX_RTSP_SendVideo(AX_RTSP_HANDLE pHandle, AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts, AX_BOOL bIFrame)
{
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer) {
        rtspServer->SendNalu(nChn, pBuf, nLen, nPts, bIFrame);

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

AX_S32 AX_RTSP_SendAudio(AX_RTSP_HANDLE pHandle, AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts)
{
    AX_BOOL bRet = AX_FALSE;

    CAXRtspServer *rtspServer = (CAXRtspServer *)pHandle;

    if (rtspServer) {
        rtspServer->SendAudio(nChn, pBuf, nLen, nPts);

        bRet = AX_TRUE;
    }

    return bRet ? 0 : -1;
}

extern AX_S32 RTSP_GetIP(AX_CHAR* pIPAddr, AX_U32 nLen);
AX_S32 AX_RTSP_GetIP(AX_CHAR* pIPAddr, AX_U32 nLen) {
    return RTSP_GetIP(pIPAddr, nLen);
}