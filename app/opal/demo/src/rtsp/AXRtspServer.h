/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AXRTSPSERVER_H__
#define __AXRTSPSERVER_H__

#include <condition_variable>
#include <mutex>
#include <thread>
#include "ax_rtsp_def.h"
#include "AXLiveServerMediaSession.h"
#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"

using namespace std;

class CAXRtspServer {
public:
    CAXRtspServer(AX_VOID) = default;
    virtual ~CAXRtspServer(AX_VOID) = default;


    AX_BOOL AddSessionAttr(AX_U32 nChn, const AX_RTSP_SESS_ATTR_T& stSessAttr);
    AX_BOOL GetSessionAttr(AX_U32 nChn, AX_RTSP_SESS_ATTR_T& stSessAttr);
    AX_BOOL UpdateSessionAttr(AX_U32 nChn, const AX_RTSP_SESS_ATTR_T& stSessAttr);

    AX_BOOL Start();
    AX_BOOL Stop();
    AX_BOOL SendNalu(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts = 0, AX_BOOL bIFrame = AX_FALSE);
	AX_BOOL SendAudio(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts);

    AX_VOID RestartSessions(AX_VOID);

private:
    AX_VOID RtspServerThreadFunc();
    /* Call must in rtsp thread */
    AX_VOID ReleaseRtspResource();
    AX_VOID DoRestartSessions(AX_VOID);

private:
    vector<AX_RTSP_SESS_ATTR_T> m_vecSessAttr;
    /* Valid media session during progress may be not continously, based on VENC's channel id */
    AXLiveServerMediaSession* m_pMediaSession[RTSP_MAX_CHN_NUM][AX_RTSP_MEDIA_BUTT]{nullptr};
    UsageEnvironment* m_pUEnv{nullptr};
    RTSPServer* m_rtspServer{nullptr};
    AX_CHAR m_chStopEventLoop{0};

    thread* m_pServerThread{nullptr};
    AX_BOOL m_bServerThreadWorking{AX_FALSE};

    mutex m_mtxSessions;
    condition_variable m_cvSessions;
    AX_BOOL m_bNeedRestartSessions{AX_FALSE};
};

#endif /*__AXRTSPSERVER_H__*/
