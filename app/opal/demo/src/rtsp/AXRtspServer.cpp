/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <dirent.h>
#include <vector>
#include <string>
#include "AXRtspServer.h"

#define RTSP_SRV "RTSP_SRV"

AX_S32 RTSP_GetIP(AX_CHAR* pIPAddr, AX_U32 nLen) {
    const char* vNetType[2] = {"eth", "usb"};
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j <= 9; ++j) {
            char strDevice[10] = {0};
            sprintf(strDevice,"%s%d",vNetType[i], j);
            int fd;
            int ret;
            struct ifreq ifr;
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            strcpy(ifr.ifr_name, strDevice);
            ret = ioctl(fd, SIOCGIFADDR, &ifr);
            close(fd);
            if (ret < 0) {
                continue;
            }

            char* pIP = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
            if (pIP) {
                strncpy((char *)pIPAddr, pIP, nLen - 1);
                pIPAddr[nLen - 1] = '\0';
                return 0;
            }
        }
    }
    return -1;
}

AX_BOOL CAXRtspServer::Start() {
    m_pServerThread = new thread(&CAXRtspServer::RtspServerThreadFunc, this);
    if (m_pServerThread) {
        m_bServerThreadWorking = AX_TRUE;
        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CAXRtspServer::Stop() {
    // LOG_MM_C(RTSP_SRV, "+++");

    m_bServerThreadWorking = AX_FALSE;
    m_chStopEventLoop = 1;

    if (m_pServerThread) {
        m_pServerThread->join();
        delete m_pServerThread;
        m_pServerThread = nullptr;
    }

    // LOG_MM_C(RTSP_SRV, "+++");
    return AX_TRUE;
}

AX_BOOL CAXRtspServer::SendNalu(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts, AX_BOOL bIFrame /*= AX_FALSE*/) {
    std::unique_lock<std::mutex> lck(m_mtxSessions);
    if (m_pMediaSession[nChn][AX_RTSP_MEDIA_VIDEO]) {
        m_pMediaSession[nChn][AX_RTSP_MEDIA_VIDEO]->SendNalu(nChn, pBuf, nLen, nPts, bIFrame);
    }

    return AX_TRUE;
}

AX_BOOL CAXRtspServer::SendAudio(AX_U8 nChn, const AX_U8* pBuf, AX_U32 nLen, AX_U64 nPts) {
    std::unique_lock<std::mutex> lck(m_mtxSessions);
    if (m_pMediaSession[nChn][AX_RTSP_MEDIA_AUDIO]) {
        m_pMediaSession[nChn][AX_RTSP_MEDIA_AUDIO]->SendNalu(nChn, pBuf, nLen, nPts, AX_TRUE);
    }

    return AX_TRUE;
}

AX_BOOL CAXRtspServer::AddSessionAttr(AX_U32 nChn, const AX_RTSP_SESS_ATTR_T& stSessAttr) {
    m_vecSessAttr.emplace_back(stSessAttr);
    return AX_TRUE;
}

AX_BOOL CAXRtspServer::GetSessionAttr(AX_U32 nChn, AX_RTSP_SESS_ATTR_T& stSessAttr) {
    for (auto& stAttr : m_vecSessAttr) {
        if (stAttr.nChannel == nChn) {
            stSessAttr = stAttr;
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

AX_BOOL CAXRtspServer::UpdateSessionAttr(AX_U32 nChannel, const AX_RTSP_SESS_ATTR_T& stSessAttr) {
    for (auto& stAttr : m_vecSessAttr) {
        if (stAttr.nChannel == nChannel) {
            stAttr.stVideoAttr = stSessAttr.stVideoAttr;
            stAttr.stAudioAttr = stSessAttr.stAudioAttr;
            break;
        }
    }

    return AX_TRUE;
}

AX_VOID CAXRtspServer::RtspServerThreadFunc() {
    prctl(PR_SET_NAME, "APP_RTSP_Server");

    OutPacketBuffer::maxSize = 700000;
    TaskScheduler* taskSchedular = BasicTaskScheduler::createNew();
    m_pUEnv = BasicUsageEnvironment::createNew(*taskSchedular);
    m_rtspServer = RTSPServer::createNew(*m_pUEnv, 8554, NULL);
    if (nullptr == m_rtspServer) {
        // LOG_M_E(RTSP_SRV, "Failed to create rtsp server :: %s", m_pUEnv->getResultMsg());
        return;
    }

    AX_CHAR szIP[64] = {0};
    AX_BOOL bGetIPRet = AX_FALSE;
    if (RTSP_GetIP(&szIP[0], sizeof(szIP)) == 0) {
        bGetIPRet = AX_TRUE;
    }

    AX_U32 nChannel = 0;
    AX_U32 nMaxFrmSize = 0;
    AX_PAYLOAD_TYPE_E ePt = PT_H264;
	AX_U32 nSessCount = m_vecSessAttr.size();
    for (AX_U32 i = 0; i < nSessCount; i++) {
        nChannel = m_vecSessAttr[i].nChannel;
        AX_CHAR strStream[32] = {0};
        sprintf(strStream, "axstream%d", nChannel);
        ServerMediaSession* sms = ServerMediaSession::createNew(*m_pUEnv, strStream, strStream, "Live Stream");
        if (m_vecSessAttr[i].stVideoAttr.bEnable) {
            ePt = m_vecSessAttr[i].stVideoAttr.ePt;
            m_pMediaSession[nChannel][AX_RTSP_MEDIA_VIDEO] = AXLiveServerMediaSession::createNewVideo(*m_pUEnv, true, ePt);
            sms->addSubsession(m_pMediaSession[nChannel][AX_RTSP_MEDIA_VIDEO]);
        }
        if (m_vecSessAttr[i].stAudioAttr.bEnable) {
            ePt = m_vecSessAttr[i].stAudioAttr.ePt;
            nMaxFrmSize = 8192;
            AX_U32 nSampleRate = m_vecSessAttr[i].stAudioAttr.nSampleRate;
            AX_U8 nChnCnt = m_vecSessAttr[i].stAudioAttr.nChnCnt;
            AX_S32 nAOT = m_vecSessAttr[i].stAudioAttr.nAOT;
            m_pMediaSession[nChannel][AX_RTSP_MEDIA_AUDIO] =
                AXLiveServerMediaSession::createNewAudio(*m_pUEnv, true, ePt, nMaxFrmSize,nSampleRate, nChnCnt, nAOT);
            sms->addSubsession(m_pMediaSession[nChannel][AX_RTSP_MEDIA_AUDIO]);
        }
        m_rtspServer->addServerMediaSession(sms);

        if (bGetIPRet) {
            char url[128] = {0};
            sprintf(url, "rtsp://%s:8554/%s", szIP, strStream);
            printf("Play the stream using url: <<<<< %s >>>>>\n", url);
        } else {
            char *url = m_rtspServer->rtspURL(sms);
            printf("Play the stream using url: <<<<< %s >>>>>\n", url);
        }
    }

    while (m_bServerThreadWorking) {
        m_chStopEventLoop = 0;
        taskSchedular->doEventLoop(&m_chStopEventLoop);
        if (m_bNeedRestartSessions) {
            DoRestartSessions();
        }
    }


    ReleaseRtspResource();

    delete (taskSchedular);
    taskSchedular = nullptr;

    // LOG_M_I(RTSP_SRV, "Quit rtsp server thread func.");

    return;
}

AX_VOID CAXRtspServer::ReleaseRtspResource() {
    AX_U32 nChannel = 0;
    AX_U32 nSessionCount = m_vecSessAttr.size();
    for (AX_U32 i = 0; i < nSessionCount; i++) {
        std::unique_lock<std::mutex> lck(m_mtxSessions);
        nChannel = m_vecSessAttr[i].nChannel;
        AX_CHAR strStream[32] = {0};
        sprintf(strStream, "axstream%d", nChannel);
        m_rtspServer->deleteServerMediaSession(strStream);

        for (AX_U32 j = 0; j < AX_RTSP_MEDIA_BUTT; j++) {
            m_pMediaSession[nChannel][j] = nullptr;
        }
    }

    RTSPServer::close(m_rtspServer);
    m_rtspServer = nullptr;
    m_pUEnv->reclaim();
    m_pUEnv = nullptr;
}

AX_VOID CAXRtspServer::RestartSessions(AX_VOID) {
    static constexpr AX_U32 nRestartSessionsTimeoutMilliseconds = 50;
    std::unique_lock<std::mutex> lck(m_mtxSessions);
    m_bNeedRestartSessions = AX_TRUE;
    m_chStopEventLoop = 1;
    m_cvSessions.wait_for(lck,
                          std::chrono::milliseconds(nRestartSessionsTimeoutMilliseconds),
                          [this] { return (m_bServerThreadWorking == AX_FALSE);});
}

AX_VOID CAXRtspServer::DoRestartSessions(AX_VOID) {
    std::unique_lock<std::mutex> lck(m_mtxSessions);
    m_bNeedRestartSessions = AX_FALSE;
    AX_U32 nMaxFrmSize = 0;
    AX_U32 nSessionCount = m_vecSessAttr.size();
    for (AX_U32 i = 0; i < nSessionCount; i++) {
        AX_U32 nChannel = m_vecSessAttr[i].nChannel;
        AX_CHAR strStream[32] = {0};
        sprintf(strStream, "axstream%d", nChannel);
        m_rtspServer->deleteServerMediaSession(strStream);
        m_pMediaSession[nChannel][AX_RTSP_MEDIA_VIDEO] = nullptr;
        m_pMediaSession[nChannel][AX_RTSP_MEDIA_AUDIO] = nullptr;

        ServerMediaSession* sms = ServerMediaSession::createNew(*m_pUEnv, strStream, strStream, "Live Stream");
        if (m_vecSessAttr[i].stVideoAttr.bEnable) {
            AX_PAYLOAD_TYPE_E ePt = m_vecSessAttr[i].stVideoAttr.ePt;
            m_pMediaSession[nChannel][AX_RTSP_MEDIA_VIDEO] =
                AXLiveServerMediaSession::createNewVideo(*m_pUEnv, true, ePt);
            sms->addSubsession(m_pMediaSession[nChannel][AX_RTSP_MEDIA_VIDEO]);
        }

        if (m_vecSessAttr[i].stAudioAttr.bEnable) {
            AX_PAYLOAD_TYPE_E ePt = m_vecSessAttr[i].stAudioAttr.ePt;
            nMaxFrmSize = 8192;
            AX_U32 nSampleRate = m_vecSessAttr[i].stAudioAttr.nSampleRate;
            AX_U8 nChnCnt = m_vecSessAttr[i].stAudioAttr.nChnCnt;
            AX_S32 nAOT = m_vecSessAttr[i].stAudioAttr.nAOT;
            m_pMediaSession[nChannel][AX_RTSP_MEDIA_AUDIO] =
                AXLiveServerMediaSession::createNewAudio(*m_pUEnv, true, ePt, nMaxFrmSize, nSampleRate, nChnCnt, nAOT);
            sms->addSubsession(m_pMediaSession[nChannel][AX_RTSP_MEDIA_AUDIO]);
        }

        m_rtspServer->addServerMediaSession(sms);
    }

    m_cvSessions.notify_one();
}
