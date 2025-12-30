/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <list>
#include <map>
#include <vector>
#include <mutex>
#include "condition_variable.hpp"
#include "GlobalDef.h"
#include "AXFrame.hpp"
#include "BaseLinkage.h"
#include "BaseSensor.h"
#include "IModule.h"
#include "IObserver.h"

#define MAX_SENSOR_COUNT (3)
#define MAX_PIPE_PER_DEV (3)

typedef struct _RAW_DISPATCH_THREAD_PARAM {
    AX_U8 nSnsID;
    AX_U8 nDevID;
    AX_U8 nPipeID;
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_BOOL bThreadRunning;
    std::thread hThread;
    CBaseSensor* pSensor;

    _RAW_DISPATCH_THREAD_PARAM() {
        nSnsID = 0;
        nDevID = 0;
        nPipeID = 0;
        eHdrMode = AX_SNS_LINEAR_MODE;
        bThreadRunning = AX_FALSE;
        pSensor = nullptr;
    }
} RAW_DISPATCH_THREAD_PARAM_T, *RAW_DISPATCH_THREAD_PARAM_PTR;

typedef struct _YUV_THREAD_PARAM {
    AX_U8 nSnsID;
    AX_U8 nPipeID;
    AX_U8 nIspChn;
    AX_BOOL bMultiplex;
    AX_BOOL bThreadRunning;
    std::thread hThread;

    _YUV_THREAD_PARAM() {
        nPipeID = 0;
        nIspChn = 0;
        bMultiplex = AX_FALSE;
        bThreadRunning = AX_FALSE;
    }
} YUV_THREAD_PARAM_T, *YUV_THREAD_PARAM_PTR;

typedef struct _STEP_EZOOM_THREAD_PARAM {
    AX_U8  nSnsID;
    AX_F32 fEZoomRatio;
    AX_F32 fLastEZoomRatio;
    AX_BOOL bThreadRunning;
    std::thread hThread;

    _STEP_EZOOM_THREAD_PARAM() {
        nSnsID = 0;
        fEZoomRatio = 1.0;
        fLastEZoomRatio = 1.0;
        bThreadRunning = AX_FALSE;
    }
} STEP_EZOOM_THREAD_PARAM_T, *STEP_EZOOM_THREAD_PARAM_PTR;

#define APP_COORD_DETECT_RESULT_ARR_SIZE (10)
typedef struct _COORD_DETECT_RESULT_T {
    AX_U64 nSeqNum{0};
    AX_BOOL bResult{AX_FALSE};
} COORD_DETECT_RESULT_T;
typedef struct _COORD_DETECT_DATA_T {
    AX_U8 nFront{0};
    AX_U8 nRear{0};
    COORD_DETECT_RESULT_T arrDetectResult[APP_COORD_DETECT_RESULT_ARR_SIZE];
} COORD_DETECT_DATA_T;

class CSensorMgr : public IModule, public IFrameRelease {
public:
    CSensorMgr() = default;
    virtual ~CSensorMgr() = default;

public:
    virtual AX_BOOL Init() override;
    virtual AX_BOOL DeInit() override;
    virtual AX_BOOL Start() override;
    virtual AX_BOOL Stop() override;

    virtual AX_VOID VideoFrameRelease(CAXFrame* pFrame) override;

    AX_BOOL Start(CBaseSensor* pSensor);
    AX_BOOL Stop(CBaseSensor* pSensor);
    CBaseSensor* GetSnsInstance(AX_U32 nIndex);
    AX_U32 GetSensorCount() {
        return m_vecSensorIns.size();
    }

    AX_S8 PipeFromSns(AX_U8 nPipeID);

    AX_VOID SetLoadType(AX_U32 nLoadType) {
        m_nLoadType = nLoadType;
    }

    AX_VOID SetYuvThreadParams(AX_U32 nSnsID, AX_U32 nPipeID, AX_U32 nChannel, AX_BOOL bMultiplex);
    AX_BOOL RestartWithScenario(AX_S32 nScenario);

    AX_VOID RegObserver(AX_U32 nPipeID, AX_U32 nChannel, IObserver* pObserver);
    AX_VOID UnregObserver(AX_U32 nPipeID, AX_U32 nChannel, IObserver* pObserver);

    AX_BOOL ChangeDaynightMode(AX_U32 nSnsID, AX_DAYNIGHT_MODE_E eDaynightMode);

    /*
        @brief: callback function intended to update sensor attributes, which will be called after initialization done and before opening.
    */
    static AX_BOOL UpdateAttrCB(ISensor* pInstance);
    AX_VOID SwitchSnsMode(AX_U32 nSnsID, AX_U32 nSnsMode, AX_U8 nHdrRatio = 0);
    AX_VOID SwitchSnsResolution(AX_U32 nSnsID, AX_U32 nWidth, AX_U32 nHeight, const AX_POOL_FLOORPLAN_T& tPrivPoolFloorPlan, const AX_VIN_LOW_MEM_MODE_E& eLowMemMode);
    AX_VOID ChangeSnsFps(AX_U32 nSnsID, AX_F32 fFrameRate);
    AX_VOID ChangeSnsMirrorFlip(AX_U32 nSnsID, AX_BOOL bMirror, AX_BOOL bFlip);
    AX_VOID UpdateAeRoi(AX_U32 nSnsID, AX_BOOL bFaceAeRoiEnable, AX_BOOL bVehicleAeRoiEnable);
    AX_VOID UpdateAeRoi(AX_U32 nSnsID, const std::vector<AX_APP_ALGO_AE_ROI_ITEM_T>& stVecItem);
    AX_BOOL UpdateLDC(AX_U32 nSnsID, AX_BOOL bLdcEnable, AX_BOOL bAspect, AX_S16 nXRatio, AX_S16 nYRatio, AX_S16 nXYRatio,
                      AX_S16 nDistorRatio);
    AX_BOOL UpdateDIS(AX_U32 nSnsID, AX_BOOL bDisEnable);
    AX_VOID UpdateSps(AX_U32 nSnsID, const SENSOR_SOFT_PHOTOSENSITIVITY_ATTR_T& tAttr);
    AX_VOID UpdateHnb(AX_U32 nSnsID, const SENSOR_HOTNOISEBALANCE_T& tAttr);
    AX_BOOL UpdateRotation(AX_U32 nSnsID, AX_VIN_ROTATION_E eRotation);
    AX_BOOL EnableChn(AX_U32 nSnsID, AX_BOOL bEnable);
    AX_BOOL EZoom(AX_U32 nSnsID, AX_F32 fEZoomRatio);
    AX_BOOL GetResolution(AX_U32 nSnsID, AX_U32 &nWidth, AX_U32 &nHeight);

    AX_VOID SetAeSyncRatio(const AX_ISP_IQ_AE_SYNC_RATIO_T& tAeSyncRatio);
    AX_VOID SetAwbSyncRatio(const AX_ISP_IQ_AWB_SYNC_RATIO_T& tAwbSyncRatio);
    AX_BOOL SetMuliSns3ASync(AX_BOOL bSync);

    AX_VOID SetSkelDetectResult(AX_U32 nSnsID, AX_U64 nSeqNum, const COORDINATION_SKEL_RESULT_T& tSkelResult);
    AX_VOID SetMDResult(AX_U32 nSnsID, AX_U64 nSeqNum, AX_BOOL bMDResult);

protected:
    AX_VOID RawDispatchThreadFunc(RAW_DISPATCH_THREAD_PARAM_T* pThreadParam);
    AX_VOID YuvGetThreadFunc(YUV_THREAD_PARAM_T* pThreadParam);
    AX_VOID CoordFpsCtrlThreadFun();
    AX_VOID StepEZoomThreadFunc(STEP_EZOOM_THREAD_PARAM_T* pThreadParam);

private:
    AX_VOID StartDispatchRawThread();
    AX_VOID StopDispatchRawThread();
    AX_VOID StartYuvGetThread();
    AX_VOID StopYuvGetThread();
    AX_VOID StartNtCtrl();
    AX_VOID StopNtCtrl();

    AX_VOID StartCoordFpsCtrlThread();
    AX_VOID StopCoordFpsCtrlThread();

    AX_VOID StopStepEZoomThread();
    AX_VOID StopStepEZoomThread(CBaseSensor* pSensor);

    AX_VOID NotifyAll(AX_S32 nPipe, AX_U32 nChannel, AX_VOID* pFrame);

    AX_BOOL GetCoordDetectResult(AX_BOOL& bDetected);

private:
    /*
        @brief: Load sensor configurations with specified types.
    */
    AX_U32 m_nLoadType{0};
    vector<CBaseSensor*> m_vecSensorIns;
    std::map<AX_U32, std::map<AX_U32, std::vector<IObserver*>>> m_mapObservers;
    std::map<AX_U8, RAW_DISPATCH_THREAD_PARAM_T> m_mapDev2ThreadParam;
    std::map<AX_U32, std::map<AX_U32, YUV_THREAD_PARAM_T>> m_mapYuvThreadParams;
    std::map<AX_U32, std::map<AX_U32, AX_BOOL>> m_mapChnLinkable;

    list<CAXFrame*> m_qFrame[MAX_SENSOR_COUNT * MAX_PIPE_PER_DEVICE][AX_VIN_CHN_ID_MAX];
    mutex m_mtxFrame[MAX_SENSOR_COUNT * MAX_PIPE_PER_DEVICE][AX_VIN_CHN_ID_MAX];
    AX_BOOL m_bGetFrameFlag[MAX_SENSOR_COUNT]{AX_FALSE};

    AX_VOID* m_pNtCrtlLib{nullptr};
    AX_DLAPI_DEF(AX_NT_CtrlInit, AX_S32, (AX_U32 nPort));
    AX_DLAPI_DEF(AX_NT_CtrlDeInit, AX_S32, (AX_VOID));

    AX_VOID* m_pNtStreamLib{nullptr};
    AX_DLAPI_DEF(AX_NT_StreamInit, AX_S32, (AX_U32 nStreamPort));
    AX_DLAPI_DEF(AX_NT_StreamDeInit, AX_S32, (AX_VOID));
    AX_DLAPI_DEF(AX_NT_SetStreamSource, AX_S32, (AX_U8 pipe));

    AX_ISP_IQ_AE_SYNC_RATIO_T  m_tAeSyncRatio{1 << 20};
    AX_ISP_IQ_AWB_SYNC_RATIO_T m_tAwbSyncRatio{1 << 20, 1 << 20};

    FIXED_PTZ_COORDINATION_INFO_T m_tCoordinationInfo;
    AX_U16 m_nCoordDetectFrameNoShow{0};
    std::mutex m_mtxSetDetectResult;
    std::mutex m_mtxStatusCheck;
    std::condition_variable m_cvStatusCheck;
    AX_BOOL m_bCoordFpsCtrlRunning{AX_FALSE};
    std::thread m_hCoordFpsCtrlThread;
    AX_F32 m_fLastFps{0};
    COORD_DETECT_DATA_T m_tSkelDetectData;
    COORD_DETECT_DATA_T m_tMDDetectData;

    std::map<AX_U8, STEP_EZOOM_THREAD_PARAM_T> m_mapStepEZoomThreadParams;
};
