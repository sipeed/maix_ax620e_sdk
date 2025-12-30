/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "SensorMgr.h"
#include <math.h>
#include <dlfcn.h>
#include <sys/prctl.h>
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "FramerateCtrlHelper.h"
#include "GlobalDef.h"
#include "OptionHelper.h"
#include "SensorFactory.hpp"
#include "SensorOptionHelper.h"
#ifdef TUNING_CTRL
#include "ax_nt_ctrl_api.h"
#include "ax_nt_stream_api.h"
#endif

#define SNS_MGR "SNS_MGR"

#define SNS_NT_CTRL_SONAME "libax_nt_ctrl.so"
#define SNS_NT_STREAM_SONAME "libax_nt_stream.so"

#define APP_STEP_EZOOM_STEPS (10)

///////////////////////////////////////////////////////////////////////////////////////////
AX_BOOL CSensorMgr::Init() {
    AX_U32 nSensorCount = APP_SENSOR_COUNT();
    for (AX_U32 i = 0; i < nSensorCount; i++) {
        SENSOR_CONFIG_T tSensorCfg;
        if (!APP_SENSOR_CONFIG(i, tSensorCfg)) {
            LOG_M_E(SNS_MGR, "Failed to get sensor config %d", i);
            return AX_FALSE;
        }

        if (tSensorCfg.eCoordCamType == COORDINATION_CAM_TYPE_FIXED) {
            m_tCoordinationInfo.nFixedSnsId = tSensorCfg.nSnsID;
            m_tCoordinationInfo.eAlgoDectctMode = tSensorCfg.eCoordAlgoDetMode;
            m_tCoordinationInfo.eSkelCheckType = tSensorCfg.eSkelCheckType;
            m_tCoordinationInfo.nMaxFrameNoShow = tSensorCfg.nMaxFrameNoShow;
        } else if (tSensorCfg.eCoordCamType == COORDINATION_CAM_TYPE_PTZ) {
            m_tCoordinationInfo.nPtzSnsId = tSensorCfg.nSnsID;
            m_tCoordinationInfo.fMaxFrameRate = tSensorCfg.fFrameRate;
            m_fLastFps = tSensorCfg.fFrameRate;
        }

        CBaseSensor* pSensor = (CBaseSensor*)(CSensorFactory::GetInstance()->CreateSensor(tSensorCfg));
        if (nullptr == pSensor) {
            LOG_M_E(SNS_MGR, "Failed to create sensor instance %d", i);
            return AX_FALSE;
        } else {
            LOG_M(SNS_MGR, "Create sensor instance %d ok.", i);
        }

        pSensor->RegAttrUpdCallback(UpdateAttrCB);

        if (!pSensor->Init()) {
            LOG_M_E(SNS_MGR, "Failed to initial sensor instance %d", i);
            return AX_FALSE;
        } else {
            LOG_M(SNS_MGR, "Initailize sensor %d ok.", i);
        }

        m_vecSensorIns.emplace_back(pSensor);

        m_mapStepEZoomThreadParams[i].nSnsID = i;
    }

    if (m_tCoordinationInfo.nFixedSnsId != APP_INVALID_SENSOR_ID
        && m_tCoordinationInfo.nPtzSnsId != APP_INVALID_SENSOR_ID
        && m_tCoordinationInfo.eAlgoDectctMode != COORDINATION_ALGO_DETECT_MODE_BUTT) {
        m_tCoordinationInfo.bEnable = AX_TRUE;
    }

    LOG_MM_D(SNS_MGR, "fixed_ptz_coord info-(enable: %d, fixed_sns: %d, ptz_sns: %d, algo_detect_mode: %d, skel_check_type: %d, nMaxFrameNoShow: %d, fMaxFrameRate: %f, fMinFrameRate: %f)",
                       m_tCoordinationInfo.bEnable, m_tCoordinationInfo.nFixedSnsId, m_tCoordinationInfo.nPtzSnsId,
                       m_tCoordinationInfo.eAlgoDectctMode, m_tCoordinationInfo.eSkelCheckType,
                       m_tCoordinationInfo.nMaxFrameNoShow, m_tCoordinationInfo.fMaxFrameRate, m_tCoordinationInfo.fMinFrameRate);

    return AX_TRUE;
}

AX_BOOL CSensorMgr::DeInit() {
    LOG_MM_C(SNS_MGR, "+++");

    for (ISensor* pSensor : m_vecSensorIns) {
        CSensorFactory::GetInstance()->DestorySensor(pSensor);
    }

    LOG_MM_C(SNS_MGR, "---");
    return AX_TRUE;
}

AX_BOOL CSensorMgr::Start() {
    AX_BOOL bSnsSync = AX_FALSE;

    vector<CBaseSensor*> vecSensorOpenIns;
    CBaseSensor* pSwitchSubSns = nullptr;
    if (m_vecSensorIns.size() > 2) { // make sure none-switch sensor open first for triple sensor
        for (auto pSensor : m_vecSensorIns) {
            if (!IS_SWITCHED_SNS(pSensor->GetSnsConfig().eSwitchSnsType)) {
                vecSensorOpenIns.insert(vecSensorOpenIns.begin(), pSensor);
            } else {
                vecSensorOpenIns.push_back(pSensor);
                if (pSensor->GetSnsConfig().eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SUB) {
                    pSwitchSubSns = pSensor;
                }
            }
        }
    } else {
        vecSensorOpenIns = m_vecSensorIns;
    }

    for (auto pSensor : vecSensorOpenIns) {
        if (pSensor->GetSnsConfig().eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN) {
            pSensor->SetSubSwitchSnsData(pSwitchSubSns);
        }

        if (!pSensor->Open()) {
            return AX_FALSE;
        }

        if (pSensor->IsSnsSync()) {
            bSnsSync = AX_TRUE;
        }
    }

    if (bSnsSync) {
        SetMuliSns3ASync(AX_TRUE);
    }

    StartNtCtrl();
    StartYuvGetThread();
    StartDispatchRawThread();
    StartCoordFpsCtrlThread();

    return AX_TRUE;
}

AX_BOOL CSensorMgr::Stop() {
    LOG_MM_C(SNS_MGR, "+++");

    StopStepEZoomThread();
    StopCoordFpsCtrlThread();
    StopNtCtrl();
    StopYuvGetThread();
    StopDispatchRawThread();
    for (auto pSensor : m_vecSensorIns) {
        if (!pSensor->Close()) {
            return AX_FALSE;
        }
    }

    LOG_MM_C(SNS_MGR, "---");
    return AX_TRUE;
}

AX_BOOL CSensorMgr::Start(CBaseSensor* pSensor) {
    if (!pSensor->Open()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CSensorMgr::Stop(CBaseSensor* pSensor) {
    StopStepEZoomThread(pSensor);

    if (!pSensor->Close()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_VOID CSensorMgr::StartDispatchRawThread() {
    for (auto pSensor : m_vecSensorIns) {
        SENSOR_CONFIG_T tSnsCfg = pSensor->GetSnsConfig();
        if (tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE) {
            m_mapDev2ThreadParam[tSnsCfg.nDevID].nSnsID = tSnsCfg.nSnsID;
            m_mapDev2ThreadParam[tSnsCfg.nDevID].nDevID = tSnsCfg.nDevID;
            m_mapDev2ThreadParam[tSnsCfg.nDevID].nPipeID = tSnsCfg.arrPipeAttr[0].nPipeID;
            m_mapDev2ThreadParam[tSnsCfg.nDevID].eHdrMode = tSnsCfg.eSensorMode;
            m_mapDev2ThreadParam[tSnsCfg.nDevID].pSensor = pSensor;
        }
    }

    for (auto& m : m_mapDev2ThreadParam) {
        RAW_DISPATCH_THREAD_PARAM_T& tParams = m.second;
        tParams.hThread = std::thread(&CSensorMgr::RawDispatchThreadFunc, this, &tParams);
    }
}

AX_VOID CSensorMgr::StopDispatchRawThread() {
    for (auto& mapDev2Param : m_mapDev2ThreadParam) {
        RAW_DISPATCH_THREAD_PARAM_T& tParams = mapDev2Param.second;
        if (tParams.hThread.joinable()) {
            tParams.bThreadRunning = AX_FALSE;
        }
    }

    for (auto& mapDev2Param : m_mapDev2ThreadParam) {
        RAW_DISPATCH_THREAD_PARAM_T& tParams = mapDev2Param.second;
        if (tParams.hThread.joinable()) {
            tParams.hThread.join();
        }
    }
}

AX_VOID CSensorMgr::StartYuvGetThread() {
    for (auto& mapChn2Param : m_mapYuvThreadParams) {
        for (auto& param : mapChn2Param.second) {
            YUV_THREAD_PARAM_T& tParams = param.second;
            tParams.hThread = std::thread(&CSensorMgr::YuvGetThreadFunc, this, &tParams);
        }
    }
}

AX_VOID CSensorMgr::StopYuvGetThread() {
    for (auto& mapChn2Param : m_mapYuvThreadParams) {
        for (auto& param : mapChn2Param.second) {
            YUV_THREAD_PARAM_T& tParams = param.second;
            if (tParams.hThread.joinable()) {
                tParams.bThreadRunning = AX_FALSE;
            }
        }
    }

    for (auto& mapChn2Param : m_mapYuvThreadParams) {
        for (auto& param : mapChn2Param.second) {
            YUV_THREAD_PARAM_T& tParams = param.second;
            if (tParams.hThread.joinable()) {
                tParams.hThread.join();
            }
        }
    }
}

AX_VOID CSensorMgr::StartNtCtrl() {
#ifdef TUNING_CTRL
    AX_S32 nRet = 0;
    AX_BOOL bInit = AX_FALSE;
    for (auto pSensor : m_vecSensorIns) {
        SENSOR_CONFIG_T tSnsCfg = pSensor->GetSnsConfig();
        for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
            if (tSnsCfg.arrPipeAttr[i].bTuning) {
#ifndef USE_STATIC_LIBS
                if (!m_pNtStreamLib) {
                    AX_DLOPEN(m_pNtStreamLib, SNS_NT_STREAM_SONAME);
                    if (!m_pNtStreamLib) {
                        LOG_M_E(SNS_MGR, "Load %s fail, %s", SNS_NT_STREAM_SONAME, strerror(errno));
                        return;
                    }

                    AX_DLAPI_LOAD(m_pNtStreamLib, AX_NT_StreamInit, AX_S32, (AX_U32 nStreamPort));
                    AX_DLAPI_LOAD(m_pNtStreamLib, AX_NT_StreamDeInit, AX_S32, (AX_VOID));
                    AX_DLAPI_LOAD(m_pNtStreamLib, AX_NT_SetStreamSource, AX_S32, (AX_U8 pipe));
                }

                nRet = -1;
                if (AX_DLAPI(AX_NT_StreamInit)) {
                    nRet = AX_DLAPI(AX_NT_StreamInit)(6000);
                }
#else
                nRet = AX_NT_StreamInit(6000);
#endif

                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_NT_StreamInit failed, ret=0x%x.", nRet);
                    return;
                }

#ifndef USE_STATIC_LIBS
                if (!m_pNtCrtlLib) {
                    AX_DLOPEN(m_pNtCrtlLib, SNS_NT_CTRL_SONAME);
                    if (!m_pNtCrtlLib) {
                        LOG_M_E(SNS_MGR, "Load %s fail, %s", SNS_NT_CTRL_SONAME, strerror(errno));
                        return;
                    }

                    AX_DLAPI_LOAD(m_pNtCrtlLib, AX_NT_CtrlInit, AX_S32, (AX_U32 nPort));
                    AX_DLAPI_LOAD(m_pNtCrtlLib, AX_NT_CtrlDeInit, AX_S32, (AX_VOID));
                }

                nRet = -1;
                if (AX_DLAPI(AX_NT_CtrlInit)) {
                    nRet = AX_DLAPI(AX_NT_CtrlInit)(tSnsCfg.arrPipeAttr[i].nTuningPort);
                }
#else
                nRet = AX_NT_CtrlInit(tSnsCfg.arrPipeAttr[i].nTuningPort);
#endif

                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_NT_CtrlInit failed, ret=0x%x.", nRet);
                    return;
                } else {
                    LOG_MM(SNS_MGR, "Enable tunning on port: %d", tSnsCfg.arrPipeAttr[i].nTuningPort);
                }

                bInit = AX_TRUE;
                break;
            }
        }
        if (bInit) break;
    }

    for (auto pSensor : m_vecSensorIns) {
        SENSOR_CONFIG_T tSnsCfg = pSensor->GetSnsConfig();

        for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
            AX_U8 nPipeID = tSnsCfg.arrPipeAttr[i].nPipeID;
            if (tSnsCfg.arrPipeAttr[i].bTuning) {
                for (AX_U8 j = 0; j < AX_VIN_CHN_ID_MAX; j++) {
                    if (tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].tChnCompressInfo.enCompressMode != 0) {
                        LOG_M_W(SNS_MGR, "Pipe %d Channel %d is in compress mode, does not support nt streaming.", nPipeID, j);
                    }
                }

#ifndef USE_STATIC_LIBS
                if (AX_DLAPI(AX_NT_SetStreamSource)) {
                    nRet = AX_DLAPI(AX_NT_SetStreamSource)(nPipeID);
                }
#else
                AX_NT_SetStreamSource(nPipeID);
#endif
            }
        }
    }
#endif
}

AX_VOID CSensorMgr::StopNtCtrl() {
#ifdef TUNING_CTRL
    AX_S32 nRet = 0;
    AX_BOOL bDeInit = AX_FALSE;
    for (auto pSensor : m_vecSensorIns) {
        SENSOR_CONFIG_T tSnsCfg = pSensor->GetSnsConfig();

        for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
            if (tSnsCfg.arrPipeAttr[i].bTuning) {
#ifndef USE_STATIC_LIBS
                nRet = -1;
                if (AX_DLAPI(AX_NT_CtrlDeInit)) {
                    nRet = AX_DLAPI(AX_NT_CtrlDeInit)();
                }
#else
                nRet = AX_NT_CtrlDeInit();
#endif
                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_NT_CtrlDeInit failed, ret=0x%x.", nRet);
                    return;
                }

#ifndef USE_STATIC_LIBS
                nRet = -1;
                if (AX_DLAPI(AX_NT_StreamDeInit)) {
                    nRet = AX_DLAPI(AX_NT_StreamDeInit)();
                }
#else
                nRet = AX_NT_StreamDeInit();
#endif
                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_NT_StreamDeInit failed, ret=0x%x.", nRet);
                    return;
                }

                bDeInit = AX_TRUE;
                break;
            }
        }

        if (bDeInit) break;
    }

#ifndef USE_STATIC_LIBS
    AX_DLCLOSE(m_pNtCrtlLib);
    AX_DLCLOSE(m_pNtStreamLib);
#endif
#endif
}

AX_VOID CSensorMgr::SetYuvThreadParams(AX_U32 nSnsID, AX_U32 nPipeID, AX_U32 nChannel, AX_BOOL bMultiplex) {
    if (nChannel >= AX_VIN_CHN_ID_MAX) {
        return;
    }
    m_mapYuvThreadParams[nPipeID][nChannel].nSnsID = nSnsID;
    m_mapYuvThreadParams[nPipeID][nChannel].nPipeID = nPipeID;
    m_mapYuvThreadParams[nPipeID][nChannel].nIspChn = nChannel;
    m_mapYuvThreadParams[nPipeID][nChannel].bMultiplex = bMultiplex;
    m_mapYuvThreadParams[nPipeID][nChannel].bThreadRunning = AX_FALSE;
}

AX_VOID CSensorMgr::RegObserver(AX_U32 nPipeID, AX_U32 nChannel, IObserver* pObserver) {
    if (nullptr != pObserver) {
        AX_S8 nSensorID = PipeFromSns(nPipeID);
        if (-1 == nSensorID) {
            LOG_MM_E(SNS_MGR, "Pipe %d does not configured in sensor.json", nPipeID);
            return;
        }

        AX_VIN_PIPE_ATTR_T tPipeAttr = m_vecSensorIns[nSensorID]->GetPipeAttr(nPipeID);
        AX_VIN_CHN_ATTR_T tChnAttr = m_vecSensorIns[nSensorID]->GetChnAttr(nPipeID, nChannel);

        OBS_TRANS_ATTR_T tTransAttr;
        tTransAttr.nGroup = nPipeID;
        tTransAttr.nChannel = nChannel;
        tTransAttr.fFramerate = tPipeAttr.tFrameRateCtrl.fDstFrameRate;
        tTransAttr.nWidth = tChnAttr.nWidth;
        tTransAttr.nHeight = tChnAttr.nHeight;
        tTransAttr.bEnableFBC = tChnAttr.tCompressInfo.enCompressMode == AX_COMPRESS_MODE_NONE ? AX_FALSE : AX_TRUE;
        tTransAttr.bLink = AX_FALSE;
        tTransAttr.nSnsSrc = PipeFromSns(nPipeID);

        if (pObserver->OnRegisterObserver(E_OBS_TARGET_TYPE_VIN, nPipeID, nChannel, &tTransAttr)) {
            m_mapObservers[nPipeID][nChannel].push_back(pObserver);
        }
    }
}

AX_VOID CSensorMgr::UnregObserver(AX_U32 nPipeID, AX_U32 nChannel, IObserver* pObserver) {
    if (nullptr == pObserver) {
        return;
    }

    for (vector<IObserver*>::iterator it = m_mapObservers[nPipeID][nChannel].begin(); it != m_mapObservers[nPipeID][nChannel].end(); it++) {
        if (*it == pObserver) {
            m_mapObservers[nPipeID][nChannel].erase(it);
            break;
        }
    }
}

AX_VOID CSensorMgr::NotifyAll(AX_S32 nPipe, AX_U32 nChannel, AX_VOID* pFrame) {
    if (nullptr == pFrame) {
        return;
    }

    if (m_mapObservers[nPipe][nChannel].size() == 0) {
        ((CAXFrame*)pFrame)->FreeMem();
        return;
    }

    for (vector<IObserver*>::iterator it = m_mapObservers[nPipe][nChannel].begin(); it != m_mapObservers[nPipe][nChannel].end(); it++) {
        (*it)->OnRecvData(E_OBS_TARGET_TYPE_VIN, nPipe, nChannel, pFrame);
    }
}

AX_VOID CSensorMgr::VideoFrameRelease(CAXFrame* pAXFrame) {
    if (pAXFrame) {
        AX_U32 nPipe = pAXFrame->nGrp;
        AX_U32 nChn = pAXFrame->nChn;

        m_mtxFrame[nPipe][nChn].lock();
        for (list<CAXFrame*>::iterator it = m_qFrame[nPipe][nChn].begin(); it != m_qFrame[nPipe][nChn].end(); it++) {
            if ((*it)->nGrp == pAXFrame->nGrp && (*it)->nChn == pAXFrame->nChn &&
                (*it)->stFrame.stVFrame.stVFrame.u64SeqNum == pAXFrame->stFrame.stVFrame.stVFrame.u64SeqNum) {
                if (!pAXFrame->bMultiplex || (*it)->DecFrmRef() == 0) {
                    AX_VIN_ReleaseYuvFrame(nPipe, (AX_VIN_CHN_ID_E)nChn, (AX_IMG_INFO_T*)(*it)->pUserDefine);
                    LOG_MM_D(SNS_MGR, "[%d][%d] AX_VIN_ReleaseYuvFrame, seq:%lld, addr:%p", nPipe, nChn,
                             pAXFrame->stFrame.stVFrame.stVFrame.u64SeqNum, pAXFrame->pUserDefine);

                    AX_IMG_INFO_T* pIspImg = (AX_IMG_INFO_T*)(*it)->pUserDefine;

                    SAFE_DELETE_PTR(pIspImg);
                    SAFE_DELETE_PTR((*it));

                    m_qFrame[nPipe][nChn].erase(it);
                }

                break;
            }
        }
        m_mtxFrame[nPipe][nChn].unlock();
    }
}

AX_VOID CSensorMgr::RawDispatchThreadFunc(RAW_DISPATCH_THREAD_PARAM_T* pThreadParam) {
    AX_U8 nPipeID = pThreadParam->nPipeID;
    AX_U8 nSnsID = pThreadParam->nSnsID;
    AX_SNS_HDR_MODE_E eHdrMode = pThreadParam->eHdrMode;
    CBaseSensor* pSensor = pThreadParam->pSensor;

    LOG_MM(SNS_MGR, "Pipe[%d] +++", nPipeID);

    AX_CHAR szName[32] = {0};
    sprintf(szName, "APP_RAW_DISP_%d", nPipeID);
    prctl(PR_SET_NAME, szName);

    AX_S32 nRet = 0;
    m_bGetFrameFlag[nSnsID] = AX_TRUE;
    pThreadParam->bThreadRunning = AX_TRUE;
    AX_IMG_INFO_T tImg[AX_SNS_HDR_FRAME_MAX] = {0};
    AX_U64 nSeq = 0;
    while (pThreadParam->bThreadRunning) {
        if (!m_bGetFrameFlag[nSnsID]) {
            CElapsedTimer::GetInstance()->mSleep(10);
            eHdrMode = pThreadParam->eHdrMode;
            continue;
        }

        for (AX_S32 i = 0; i < eHdrMode; i++) {
            nRet = AX_VIN_GetRawFrame(nPipeID, AX_VIN_PIPE_DUMP_NODE_IFE, (AX_SNS_HDR_FRAME_E)i, tImg + i, 500);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(SNS_MGR, "Pipe[%d] AX_VIN_GetRawFrame failed, ret=0x%x.", nPipeID, nRet);
                continue;
            } else {
                LOG_M_D(SNS_MGR, "[%lld] Get raw frame, hdrframe=%d, seq=%lld.", nSeq, i, tImg[i].tFrameInfo.stVFrame.u64SeqNum);
            }
        }

        if (tImg[0].tIspInfo.tSnsSWitchInfo.bFirstFrmFlag) {
            pSensor->LoadBinParams(nPipeID);
        }

        pSensor->SetEZoomAeAwbIQ(tImg[0].tIspInfo.tSnsSWitchInfo.nSnsId, nPipeID);

        if (tImg[0].tIspInfo.tSnsSWitchInfo.bFirstFrmFlag) {
            AX_ISP_RUNONCE_PARAM_T nIspParm;
            nIspParm.eCmdType = AX_ISP_EXT_CMD_SNS_SWITCH;
            nIspParm.bFirstFrmFlag = AX_TRUE;
            nIspParm.nSnsId = tImg[0].tIspInfo.tSnsSWitchInfo.nSnsId;
            nIspParm.eLensType = tImg[0].tIspInfo.tSnsSWitchInfo.eLensType;
            nRet = AX_ISP_RunOnceExt(nPipeID, &nIspParm);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(SNS_MGR, "Pipe[%d] AX_ISP_RunOnceExt failed, ret=0x%x.", nPipeID, nRet);
            }

            LOG_M_C(SNS_MGR, "Pipe[%d] eHdrMode[%d] AX_ISP_RunOnceExt for sns(%d) lenstype: %d",
                                        nPipeID, eHdrMode, nIspParm.nSnsId, nIspParm.eLensType);
        }

        nRet = AX_VIN_SendRawFrame(nPipeID, AX_VIN_FRAME_SOURCE_ID_ITP, (AX_S8)eHdrMode, (const AX_IMG_INFO_T**)&tImg, 200);
        if (AX_SUCCESS != nRet) {
            LOG_MM_E(SNS_MGR, "Pipe[%d] AX_VIN_SendRawFrame failed, ret=0x%x.", nPipeID, nRet);
        } else {
            LOG_M_D(SNS_MGR, "[%lld] Send raw to ITP pipe %d, frame num=%d", nSeq, nPipeID, eHdrMode);
        }

        for (AX_S32 i = 0; i < eHdrMode; i++) {
            nRet = AX_VIN_ReleaseRawFrame(nPipeID, AX_VIN_PIPE_DUMP_NODE_IFE, (AX_SNS_HDR_FRAME_E)i, tImg + i);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(SNS_MGR, "Pipe[%d] AX_VIN_ReleaseRawFrame failed, ret=0x%x.", nPipeID, nRet);
                continue;
            } else {
                LOG_M_D(SNS_MGR, "[%lld] Release raw frame, hdrframe=%d, seq=%lld.", nSeq, i, tImg[i].tFrameInfo.stVFrame.u64SeqNum);
            }
        }

        nSeq++;
    }
}

AX_VOID CSensorMgr::YuvGetThreadFunc(YUV_THREAD_PARAM_T* pThreadParam) {
    AX_U8 nPipe = pThreadParam->nPipeID;
    AX_U8 nChn = pThreadParam->nIspChn;
    AX_U8 nSnsID = pThreadParam->nSnsID;

    LOG_MM(SNS_MGR, "[%d][%d] +++", nPipe, nChn);

    AX_CHAR szName[32] = {0};
    sprintf(szName, "APP_YUV_Get_%d_%d", nPipe, nChn);
    prctl(PR_SET_NAME, szName);

    AX_S32 nRet = 0;
    m_bGetFrameFlag[nSnsID] = AX_TRUE;
    pThreadParam->bThreadRunning = AX_TRUE;
    while (pThreadParam->bThreadRunning) {
        if (!m_bGetFrameFlag[nSnsID]) {
            CElapsedTimer::GetInstance()->mSleep(10);
            continue;
        }
        AX_IMG_INFO_T* pVinImg = new (std::nothrow) AX_IMG_INFO_T();
        if (nullptr == pVinImg) {
            LOG_M_E(SNS_MGR, "Allocate buffer for YuvGetThread failed.");
            CElapsedTimer::GetInstance()->mSleep(10);
            continue;
        }

        nRet = AX_VIN_GetYuvFrame(nPipe, (AX_VIN_CHN_ID_E)nChn, pVinImg, 1000);
        if (AX_SUCCESS != nRet) {
            if (pThreadParam->bThreadRunning) {
                LOG_M_E(SNS_MGR, "[%d][%d] AX_VIN_GetYuvFrame failed, ret=0x%x, unreleased buffer=%d", nPipe, nChn, nRet,
                        m_qFrame[nPipe][nChn].size());
            }
            SAFE_DELETE_PTR(pVinImg);
            continue;
        }

        LOG_MM_D(SNS_MGR, "[%d][%d] Seq %llu, Size %d, w:%d, h:%d, PTS:%llu, [FramePhyAddr:0x%llx, FrameVirAddr:0x%llx], addr:%p", nPipe,
                 nChn, pVinImg->tFrameInfo.stVFrame.u64SeqNum, pVinImg->tFrameInfo.stVFrame.u32FrameSize,
                 pVinImg->tFrameInfo.stVFrame.u32Width, pVinImg->tFrameInfo.stVFrame.u32Height, pVinImg->tFrameInfo.stVFrame.u64PTS,
                 pVinImg->tFrameInfo.stVFrame.u64PhyAddr[0], pVinImg->tFrameInfo.stVFrame.u64VirAddr[0], pVinImg);

        ///////////////////////////// DEBUG DATA //////////////////////////////////
        // AX_VIN_ReleaseYuvFrame(nPipe, (AX_VIN_CHN_ID_E)nChn, pVinImg);
        // SAFE_DELETE_PTR(pVinImg);
        // continue;
        ///////////////////////////////////////////////////////////////

        CAXFrame* pAXFrame = new (std::nothrow) CAXFrame();
        pAXFrame->nGrp = nPipe;
        pAXFrame->nChn = nChn;
        pAXFrame->stFrame.stVFrame = pVinImg->tFrameInfo;
        pAXFrame->pFrameRelease = this;
        pAXFrame->pUserDefine = pVinImg;
        /* Here, we can not determine bMultiplex flag according to number of observers, because each observer must filter frames by target
         * pipe & channel */
        pAXFrame->bMultiplex = pThreadParam->bMultiplex;

        m_mtxFrame[nPipe][nChn].lock();
        if (m_qFrame[nPipe][nChn].size() >= 5) {
            LOG_MM_W(SNS_MGR, "[%d][%d] queue size is %d, drop this frame", nPipe, nChn, m_qFrame[nPipe][nChn].size());
            AX_VIN_ReleaseYuvFrame(nPipe, (AX_VIN_CHN_ID_E)nChn, pVinImg);
            SAFE_DELETE_PTR(pVinImg);
            SAFE_DELETE_PTR(pAXFrame);

            m_mtxFrame[nPipe][nChn].unlock();
            continue;
        }

        m_qFrame[nPipe][nChn].push_back(pAXFrame);
        m_mtxFrame[nPipe][nChn].unlock();

        NotifyAll(nPipe, nChn, pAXFrame);
    }

    LOG_MM(SNS_MGR, "[%d][%d] ---", nPipe, nChn);
}

CBaseSensor* CSensorMgr::GetSnsInstance(AX_U32 nIndex) {
    if (nIndex >= m_vecSensorIns.size()) {
        return nullptr;
    }

    return m_vecSensorIns[nIndex];
}

AX_S8 CSensorMgr::PipeFromSns(AX_U8 nPipeID) {
    for (AX_U8 i = 0; i < GetSensorCount(); i++) {
        CBaseSensor* pSensor = GetSnsInstance(i);
        AX_U32 nPipeCount = pSensor->GetSnsConfig().nPipeCount;
        for (AX_U8 j = 0; j < nPipeCount; j++) {
            if (nPipeID == pSensor->GetSnsConfig().arrPipeAttr[j].nPipeID) {
                return i;
            }
        }
    }

    return -1;
}

AX_BOOL CSensorMgr::UpdateAttrCB(ISensor* pInstance) {
    if (nullptr == pInstance) {
        return AX_FALSE;
    }

    /* Sample code to update attributes before sensor.Open */
    // SNS_ABILITY_T tSnsAbilities = pInstance->GetAbilities();

    // AX_VIN_PIPE_ATTR_T tPipeAttr = pInstance->GetPipeAttr();
    // tPipeAttr.tCompressInfo = AX_TRUE;
    // pInstance->SetPipeAttr(tPipeAttr);

    return AX_TRUE;
}

AX_BOOL CSensorMgr::ChangeDaynightMode(AX_U32 nSnsID, AX_DAYNIGHT_MODE_E eDaynightMode) {
    CBaseSensor* pSensor = GetSnsInstance(nSnsID);
    if (nullptr != pSensor) {
        return pSensor->ChangeDaynightMode(eDaynightMode);
    } else {
        return AX_FALSE;
   }
}

AX_VOID CSensorMgr::SwitchSnsMode(AX_U32 nSnsID, AX_U32 nSnsMode, AX_U8 nHdrRatio/* = 0 */) {
    LOG_MM_C(SNS_MGR, "+++");

    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return;
    }

    AX_BOOL bRestart = AX_TRUE;
    SNS_HDR_TYPE_E eHdrType = GET_SNS_HDR_TYPE(nSnsMode);

    // no need restart for switching between Long-Frame hdr normal mode and Long-Frame hdr long frame mode
    if ((eHdrType == E_SNS_HDR_TYPE_LONGFRAME) && (pCurSensor->GetSnsConfig().eHdrType == E_SNS_HDR_TYPE_LONGFRAME)) {
        bRestart = AX_FALSE;
    }

    if (!bRestart) {
        pCurSensor->ChangeHdrMode(nSnsMode, nHdrRatio);
    } else {
        m_bGetFrameFlag[nSnsID] = AX_FALSE;

        Stop(pCurSensor);
        {
            AX_U8 nDevID = pCurSensor->GetSnsConfig().nDevID;
            pCurSensor->ChangeHdrMode(nSnsMode, nHdrRatio);
            m_mapDev2ThreadParam[nDevID].eHdrMode = (eHdrType == E_SNS_HDR_TYPE_LONGFRAME) ? AX_SNS_HDR_2X_MODE : (AX_SNS_HDR_MODE_E)nSnsMode;
            pCurSensor->Init();
        }

        Start(pCurSensor);

        m_bGetFrameFlag[nSnsID] = AX_TRUE;
    }

    LOG_MM_C(SNS_MGR, "---");
    return;
}

AX_VOID CSensorMgr::SwitchSnsResolution(AX_U32 nSnsID, AX_U32 nWidth, AX_U32 nHeight, const AX_POOL_FLOORPLAN_T& tPrivPoolFloorPlan, const AX_VIN_LOW_MEM_MODE_E& eLowMemMode) {
    LOG_MM_C(SNS_MGR, "+++");

    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return;
    }

    AX_S32 nRet = 0;

    Stop(pCurSensor);

    // deinit vin
    {
        nRet = AX_MIPI_RX_DeInit();

        if (0 != nRet) {
            LOG_M_E(SNS_MGR, "AX_MIPI_RX_DeInit, ret=0x%x.", nRet);
        } else {
            LOG_M_C(SNS_MGR, "AX_MIPI_RX_DeInit");
        }

        nRet = AX_VIN_Deinit();

        if (0 != nRet) {
            LOG_M_E(SNS_MGR, "AX_VIN_Deinit, ret=0x%x.", nRet);
        } else {
            LOG_M_C(SNS_MGR, "AX_VIN_Deinit");
        }
    }

    {
        pCurSensor->ChangeSnsResolution(nWidth, nHeight);

        // init vin
        {
            nRet = AX_VIN_Init();

            if (0 != nRet) {
                LOG_M_E(SNS_MGR, "AX_VIN_Init, ret=0x%x.", nRet);
            } else {
                LOG_M_C(SNS_MGR, "AX_VIN_Init");
            }

            nRet = AX_MIPI_RX_Init();

            if (0 != nRet) {
                LOG_M_E(SNS_MGR, "AX_MIPI_RX_Init, ret=0x%x.", nRet);
            } else {
                LOG_M_C(SNS_MGR, "AX_MIPI_RX_Init");
            }

            if (AX_VIN_LOW_MEM_DISABLE != eLowMemMode) {
                AX_S32 nRet = AX_VIN_SetLowMemMode(eLowMemMode);

                if (nRet != 0) {
                    LOG_MM_E(SNS_MGR, "AX_VIN_SetLowMemMode mode[%d], ret=0x%x.", eLowMemMode, nRet);
                } else {
                    LOG_MM_C(SNS_MGR, "AX_VIN_SetLowMemMode mode[%d]", eLowMemMode);
                }
            }

            {
                /*initialize vin private pool*/
                nRet = AX_VIN_SetPoolAttr(&tPrivPoolFloorPlan);

                if (0 != nRet) {
                    LOG_MM_E(SNS_MGR, "AX_VIN_SetPoolAttr, ret=0x%x.", nRet);
                } else {
                    LOG_MM_C(SNS_MGR, "AX_VIN_SetPoolAttr");
                }
            }
        }

        pCurSensor->Init();
    }

    Start(pCurSensor);

    LOG_MM_C(SNS_MGR, "---");
    return;
}

AX_VOID CSensorMgr::ChangeSnsFps(AX_U32 nSnsID, AX_F32 fFrameRate) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return;
    }
    AX_SNS_ATTR_T stSnsAttr = pCurSensor->GetSnsAttr();
    stSnsAttr.fFrameRate = fFrameRate;
    pCurSensor->SetSnsAttr(stSnsAttr);
}

AX_VOID CSensorMgr::ChangeSnsMirrorFlip(AX_U32 nSnsID, AX_BOOL bMirror, AX_BOOL bFlip) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return;
    }

    pCurSensor->ChangeSnsMirrorFlip(bMirror, bFlip);
}

AX_BOOL CSensorMgr::UpdateLDC(AX_U32 nSnsID, AX_BOOL bLdcEnable, AX_BOOL bAspect, AX_S16 nXRatio, AX_S16 nYRatio, AX_S16 nXYRatio,
                              AX_S16 nDistorRatio) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return AX_FALSE;
    }

    AX_BOOL ret = pCurSensor->UpdateLDC(bLdcEnable, bAspect, nXRatio, nYRatio, nXYRatio, nDistorRatio);
    if (!ret) {
        LOG_MM_E(SNS_MGR, "sensor%d UpdateLDC failed.[%d,%d,%d,%d,%d,%d]", nSnsID, bLdcEnable, bAspect, nXRatio, nYRatio, nXYRatio,
                 nDistorRatio);
    }
    return ret;
}

AX_BOOL CSensorMgr::UpdateDIS(AX_U32 nSnsID, AX_BOOL bDisEnable) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);
    if (nullptr == pCurSensor) {
        return AX_FALSE;
    }

    AX_BOOL ret = pCurSensor->UpdateDIS(bDisEnable);
    if (!ret) {
        LOG_MM_E(SNS_MGR, "sensor%d UpdateDIS failed.[%d]", nSnsID, bDisEnable);
    }
    return ret;
}

AX_VOID CSensorMgr::UpdateAeRoi(AX_U32 nSnsID, AX_BOOL bFaceAeRoiEnable, AX_BOOL bVehicleAeRoiEnable) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);

    if (nullptr == pCurSensor) {
        return;
    }

    SENSOR_AE_ROI_ATTR_T tAttr;
    tAttr.bFaceAeRoiEnable = bFaceAeRoiEnable;
    tAttr.bVehicleAeRoiEnable = bVehicleAeRoiEnable;
    pCurSensor->UpdateAeRoi(tAttr);
}

AX_VOID CSensorMgr::UpdateAeRoi(AX_U32 nSnsID, const std::vector<AX_APP_ALGO_AE_ROI_ITEM_T>& stVecItem) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);

    if (nullptr == pCurSensor) {
        return;
    }

    pCurSensor->UpdateAeRoi(stVecItem);
}

AX_BOOL CSensorMgr::UpdateRotation(AX_U32 nSnsID, AX_VIN_ROTATION_E eRotation)
{
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);

    if (nullptr == pCurSensor) {
        return AX_FALSE;
    }

    return pCurSensor->UpdateRotation(eRotation);
}


AX_BOOL CSensorMgr::EnableChn(AX_U32 nSnsID, AX_BOOL bEnable)
{
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);

    if (nullptr == pCurSensor) {
        return AX_FALSE;
    }

    return pCurSensor->EnableChn(bEnable);
}

AX_BOOL CSensorMgr::EZoom(AX_U32 nSnsID, AX_F32 fEZoomRatio)
{
    AX_BOOL bRet = AX_FALSE;
    if (m_mapStepEZoomThreadParams.find(nSnsID) != m_mapStepEZoomThreadParams.end()) {
        if (m_mapStepEZoomThreadParams[nSnsID].hThread.joinable()) {
            m_mapStepEZoomThreadParams[nSnsID].bThreadRunning = AX_FALSE;
            m_mapStepEZoomThreadParams[nSnsID].hThread.join();
        }
        if (m_mapStepEZoomThreadParams[nSnsID].fLastEZoomRatio == fEZoomRatio) {
            return AX_TRUE;
        }
        LOG_MM_I(SNS_MGR, "+++ [%d]zoom ratio: %f", nSnsID, fEZoomRatio);
        m_mapStepEZoomThreadParams[nSnsID].fEZoomRatio = fEZoomRatio;
        m_mapStepEZoomThreadParams[nSnsID].hThread = std::thread(&CSensorMgr::StepEZoomThreadFunc, this, &m_mapStepEZoomThreadParams[nSnsID]);
        bRet = AX_TRUE;
    }

    return bRet;
}


AX_VOID CSensorMgr::UpdateSps(AX_U32 nSnsID, const SENSOR_SOFT_PHOTOSENSITIVITY_ATTR_T& tAttr) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);

    if (nullptr == pCurSensor) {
        return;
    }

    pCurSensor->UpdateSps(tAttr);
}

AX_VOID CSensorMgr::UpdateHnb(AX_U32 nSnsID, const SENSOR_HOTNOISEBALANCE_T& tAttr) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);

    if (nullptr == pCurSensor) {
        return;
    }

    pCurSensor->UpdateHnb(tAttr);
}

AX_BOOL CSensorMgr::GetResolution(AX_U32 nSnsID, AX_U32 &nWidth, AX_U32 &nHeight) {
    CBaseSensor* pCurSensor = GetSnsInstance(nSnsID);

    if (nullptr == pCurSensor) {
        return AX_FALSE;
    }

    pCurSensor->GetResolution(nWidth, nHeight);

    return AX_TRUE;
}

AX_VOID CSensorMgr::SetAeSyncRatio(const AX_ISP_IQ_AE_SYNC_RATIO_T& tAeSyncRatio) {
    m_tAeSyncRatio = tAeSyncRatio;
}

AX_VOID CSensorMgr::SetAwbSyncRatio(const AX_ISP_IQ_AWB_SYNC_RATIO_T& tAwbSyncRatio) {
    m_tAwbSyncRatio = tAwbSyncRatio;
}

AX_BOOL CSensorMgr::SetMuliSns3ASync(AX_BOOL bSync) {
    AX_S32 nRet = 0;
    AX_U32 n3ARatioOffValue = 1 << 20;

    AX_ISP_IQ_AE_SYNC_RATIO_T  tAeSyncRatio{0};
    AX_ISP_IQ_AWB_SYNC_RATIO_T tAwbSyncRatio{0};

    tAeSyncRatio.nAeSyncRatio = bSync ? m_tAeSyncRatio.nAeSyncRatio : n3ARatioOffValue;
    tAwbSyncRatio.nRGainRatio = bSync ? m_tAwbSyncRatio.nRGainRatio : n3ARatioOffValue;
    tAwbSyncRatio.nBGainRatio = bSync ? m_tAwbSyncRatio.nBGainRatio : n3ARatioOffValue;

    LOG_MM_I(SNS_MGR, "bSync: %d, nAeSyncRatio: %d, nRGainRatio: %d, nBGainRatio: %d",
                    bSync,
                    tAeSyncRatio.nAeSyncRatio,
                    tAwbSyncRatio.nRGainRatio,
                    tAwbSyncRatio.nBGainRatio);

    nRet = AX_ISP_IQ_SetAeSyncParam(&tAeSyncRatio);
    if (0 != nRet) {
        LOG_MM_E(SNS_MGR, "AX_ISP_IQ_SetAeSyncParam fail, nRet:0x%x", nRet);
        return AX_FALSE;
    }

    nRet = AX_ISP_IQ_SetAwbSyncParam(&tAwbSyncRatio);
    if (0 != nRet) {
        LOG_MM_E(SNS_MGR, "AX_ISP_IQ_SetAwbSyncParam fail, nRet:0x%x", nRet);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_VOID CSensorMgr::SetSkelDetectResult(AX_U32 nSnsID, AX_U64 nSeqNum, const COORDINATION_SKEL_RESULT_T& tSkelResult) {
    if (!m_tCoordinationInfo.bEnable
         || m_tCoordinationInfo.eAlgoDectctMode == COORDINATION_ALGO_DETECT_MODE_MD
         || m_tCoordinationInfo.nFixedSnsId != nSnsID) {
        return;
    }

    AX_BOOL bDetectResult = AX_FALSE;
    switch (m_tCoordinationInfo.eSkelCheckType) {
        case COORDINATION_SKEL_CHECK_TYPE_BODY:
            bDetectResult = tSkelResult.bBodyFound;
            break;
        case COORDINATION_SKEL_CHECK_TYPE_VEHICLE:
            bDetectResult = tSkelResult.bVehicleFound;
            break;
        case COORDINATION_SKEL_CHECK_TYPE_CYCLE:
            bDetectResult = tSkelResult.bCycleFound;
            break;
        case COORDINATION_SKEL_CHECK_TYPE_BODY_VEHICLE:
            bDetectResult = (AX_BOOL)(tSkelResult.bBodyFound || tSkelResult.bVehicleFound);
            break;
        case COORDINATION_SKEL_CHECK_TYPE_BODY_VEHICLE_CYCLE:
            bDetectResult = (AX_BOOL)(tSkelResult.bBodyFound || tSkelResult.bVehicleFound || tSkelResult.bCycleFound);
            break;
        default:
            bDetectResult = tSkelResult.bBodyFound;
            break;
    }

    {
        std::lock_guard<std::mutex> guard(m_mtxSetDetectResult);
        m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nRear].nSeqNum = nSeqNum;
        m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nRear].bResult = bDetectResult;
        m_tSkelDetectData.nRear = (m_tSkelDetectData.nRear + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
        if (m_tSkelDetectData.nRear == m_tSkelDetectData.nFront) { // full, overwrite oldest data
            m_tSkelDetectData.nFront = (m_tSkelDetectData.nFront + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
        }
    }

    {
        std::unique_lock<std::mutex> lck(m_mtxStatusCheck);
        m_cvStatusCheck.notify_one();
    }
}

AX_VOID CSensorMgr::SetMDResult(AX_U32 nSnsID, AX_U64 nSeqNum, AX_BOOL bMDResult) {
    if (!m_tCoordinationInfo.bEnable
        || m_tCoordinationInfo.eAlgoDectctMode == COORDINATION_ALGO_DETECT_MODE_SKEL
        || m_tCoordinationInfo.nFixedSnsId != nSnsID) {
        return;
    }

    {
        std::lock_guard<std::mutex> guard(m_mtxSetDetectResult);
        m_tMDDetectData.arrDetectResult[m_tMDDetectData.nRear].nSeqNum = nSeqNum;
        m_tMDDetectData.arrDetectResult[m_tMDDetectData.nRear].bResult = bMDResult;
        m_tMDDetectData.nRear = (m_tMDDetectData.nRear + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
        if (m_tMDDetectData.nRear == m_tMDDetectData.nFront) { // full, overwrite oldest data
            m_tMDDetectData.nFront = (m_tMDDetectData.nFront + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
        }
    }

    {
        std::unique_lock<std::mutex> lck(m_mtxStatusCheck);
        m_cvStatusCheck.notify_one();
    }
}

AX_VOID CSensorMgr::CoordFpsCtrlThreadFun() {
    LOG_MM_C(SNS_MGR, "+++");

    AX_CHAR szName[32] = {0};
    sprintf(szName, "APP_COORD_FPS_CTRL");
    prctl(PR_SET_NAME, szName);

    CBaseSensor* pPtzSensor = GetSnsInstance(m_tCoordinationInfo.nPtzSnsId);

    while(m_bCoordFpsCtrlRunning) {
        {
            std::unique_lock<std::mutex> lck(m_mtxStatusCheck);
            m_cvStatusCheck.wait(lck);
        }

        if (!m_bCoordFpsCtrlRunning) {
            break;
        }

        AX_BOOL bDetected = AX_FALSE;
        if (GetCoordDetectResult(bDetected)) {
            if (bDetected) {
                m_nCoordDetectFrameNoShow = 0;
                if (m_fLastFps == m_tCoordinationInfo.fMinFrameRate) {
                    pPtzSensor->UpdateSnsFps(m_tCoordinationInfo.fMaxFrameRate);
                    m_fLastFps = m_tCoordinationInfo.fMaxFrameRate;
                }
            } else {
                m_nCoordDetectFrameNoShow++;

                if (m_nCoordDetectFrameNoShow >= m_tCoordinationInfo.nMaxFrameNoShow) {
                    m_nCoordDetectFrameNoShow = 0;
                    if (m_fLastFps == m_tCoordinationInfo.fMaxFrameRate) {
                        pPtzSensor->UpdateSnsFps(m_tCoordinationInfo.fMinFrameRate);
                        m_fLastFps = m_tCoordinationInfo.fMinFrameRate;
                    }
                }
            }
        }
    }
}

AX_VOID CSensorMgr::StartCoordFpsCtrlThread() {
    if (m_tCoordinationInfo.bEnable) {
        m_bCoordFpsCtrlRunning = AX_TRUE;
        m_hCoordFpsCtrlThread = std::thread(&CSensorMgr::CoordFpsCtrlThreadFun, this);
    }
}

AX_VOID CSensorMgr::StopCoordFpsCtrlThread() {
    if (m_tCoordinationInfo.bEnable) {
        m_bCoordFpsCtrlRunning = AX_FALSE;

        {
            std::unique_lock<std::mutex> lck(m_mtxStatusCheck);
            m_cvStatusCheck.notify_one();
        }

        if (m_hCoordFpsCtrlThread.joinable()) {
            m_hCoordFpsCtrlThread.join();
        }
    }
}

AX_BOOL CSensorMgr::GetCoordDetectResult(AX_BOOL& bDetected) {
    std::lock_guard<std::mutex> guard(m_mtxSetDetectResult);
    switch (m_tCoordinationInfo.eAlgoDectctMode) {
        case COORDINATION_ALGO_DETECT_MODE_SKEL:
            if (m_tSkelDetectData.nFront == m_tSkelDetectData.nRear) {
                return AX_FALSE;
            }

            bDetected = m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nFront].bResult;
            m_tSkelDetectData.nFront = (m_tSkelDetectData.nFront + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
            break;
        case COORDINATION_ALGO_DETECT_MODE_MD:
            if (m_tMDDetectData.nFront == m_tMDDetectData.nRear) {
                return AX_FALSE;
            }

            bDetected = m_tMDDetectData.arrDetectResult[m_tMDDetectData.nFront].bResult;
            m_tMDDetectData.nFront = (m_tMDDetectData.nFront + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
            break;
        case COORDINATION_ALGO_DETECT_MODE_MD_SKEL:
            if (m_tSkelDetectData.nFront == m_tSkelDetectData.nRear || m_tMDDetectData.nFront == m_tMDDetectData.nRear) {
                return AX_FALSE;
            }

            if (m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nFront].nSeqNum < m_tMDDetectData.arrDetectResult[m_tMDDetectData.nFront].nSeqNum) {
                do {
                    m_tSkelDetectData.nFront = (m_tSkelDetectData.nFront + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
                    if (m_tSkelDetectData.nFront == m_tSkelDetectData.nRear) {
                        return AX_FALSE;
                    }
                } while(m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nFront].nSeqNum < m_tMDDetectData.arrDetectResult[m_tMDDetectData.nFront].nSeqNum);
            } else if (m_tMDDetectData.arrDetectResult[m_tMDDetectData.nFront].nSeqNum < m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nFront].nSeqNum) {
                do {
                    m_tMDDetectData.nFront = (m_tMDDetectData.nFront + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
                    if (m_tMDDetectData.nFront == m_tMDDetectData.nRear) {
                        return AX_FALSE;
                    }
                } while(m_tMDDetectData.arrDetectResult[m_tMDDetectData.nFront].nSeqNum < m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nFront].nSeqNum);
            }

            if (m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nFront].nSeqNum == m_tMDDetectData.arrDetectResult[m_tMDDetectData.nFront].nSeqNum) {
                bDetected = (AX_BOOL)(m_tSkelDetectData.arrDetectResult[m_tSkelDetectData.nFront].bResult && m_tMDDetectData.arrDetectResult[m_tMDDetectData.nFront].bResult);
                m_tSkelDetectData.nFront = (m_tSkelDetectData.nFront + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
                m_tMDDetectData.nFront = (m_tMDDetectData.nFront + 1) % APP_COORD_DETECT_RESULT_ARR_SIZE;
            }
            break;
        default:
            return AX_FALSE;
    }

    return AX_TRUE;
}

AX_VOID CSensorMgr::StepEZoomThreadFunc(STEP_EZOOM_THREAD_PARAM_T* pThreadParam) {
    AX_U8 nSnsId = pThreadParam->nSnsID;
    AX_F32 fEZoomRatio = pThreadParam->fEZoomRatio;
    AX_F32 fBaseEZoomRatio = pThreadParam->fLastEZoomRatio;

    // LOG_MM_I(SNS_MGR, "+++ [%d]", nSnsId);

    AX_CHAR szName[32] = {0};
    sprintf(szName, "APP_STEP_EZOOM_%d", nSnsId);
    prctl(PR_SET_NAME, szName);

    AX_F32 fStep = (fEZoomRatio - fBaseEZoomRatio) / APP_STEP_EZOOM_STEPS;
    m_mapStepEZoomThreadParams[nSnsId].bThreadRunning = AX_TRUE;
    CBaseSensor* pSensor = GetSnsInstance(nSnsId);
    if (!pSensor) {
        LOG_MM_E(SNS_MGR, "Can not find sensor instance for sns index: %d", nSnsId);
        return;
    }

    AX_U8 i = 1;
    for (; i < APP_STEP_EZOOM_STEPS; i++) {
        if (!m_mapStepEZoomThreadParams[nSnsId].bThreadRunning) {
            // LOG_MM_E(SNS_MGR, "bThreadRunning false, index: %d", i);
            break;
        }

        pSensor->EZoom(fBaseEZoomRatio + i * fStep);
    }

    pSensor->EZoom(fEZoomRatio);

    m_mapStepEZoomThreadParams[nSnsId].fLastEZoomRatio = fEZoomRatio;

    LOG_MM_I(SNS_MGR, "--- [%d], ratio: %f", nSnsId, fEZoomRatio);
}

AX_VOID CSensorMgr::StopStepEZoomThread() {
    for (auto& it : m_mapStepEZoomThreadParams) {
        if (it.second.hThread.joinable()) {
            it.second.bThreadRunning = AX_FALSE;
            it.second.hThread.join();
        }
    }
}

AX_VOID CSensorMgr::StopStepEZoomThread(CBaseSensor* pSensor) {
    AX_U8 nSnsId = AX_U8(-1);
    for (AX_U8 i = 0; i < GetSensorCount(); i++) {
        CBaseSensor* pSensorIns = GetSnsInstance(i);
        if (pSensorIns == pSensor) {
            nSnsId = i;
            break;
        }
    }

    if (nSnsId == AX_U8(-1)) {
        return;
    }

    if (m_mapStepEZoomThreadParams[nSnsId].hThread.joinable()) {
        m_mapStepEZoomThreadParams[nSnsId].bThreadRunning = AX_FALSE;
        m_mapStepEZoomThreadParams[nSnsId].hThread.join();
    }
}
