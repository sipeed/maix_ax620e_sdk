/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "BaseSensor.h"
#include <dlfcn.h>
#include <sys/prctl.h>
#include <math.h>
#include "AppLogApi.h"
#include "CommonUtils.hpp"
#include "ElapsedTimer.hpp"
#include "AXTypeConverter.hpp"
#include "SensorOptionHelper.h"
#include "SoftPhotoSensitivity.h"
#include "HotNoiseBalance.h"
#include "IspAlgoAeRoi.h"

#ifdef SWITCH_SENSOR_SUPPORT
#include "mipi_switch.h"
#include "CmdLineParser.h"
#endif

#define SENSOR "SENSOR"
#define SDRKEY "_sdr"
#define HDRKEY "_hdr"
#define LFSDRKEY "_lfhdr_long_frame"
#define LFHDRKEY "_lfhdr_normal"
#define BOARD_ID_LEN 128

#define SENSOR_BRIGHTNESS_LOW 0
#define SENSOR_BRIGHTNESS_MEDIUM 0x100
#define SENSOR_BRIGHTNESS_HIGH 4095

#define SENSOR_CONTRAST_LOW -4096
#define SENSOR_CONTRAST_MEDIUM 0x100
#define SENSOR_CONTRAST_HIGH 4095

#define SENSOR_SATURATION_LOW 0
#define SENSOR_SATURATION_MEDIUM 0x1000
#define SENSOR_SATURATION_HIGH 65535

#define SENSOR_SHARPNESS_LOW 16
#define SENSOR_SHARPNESS_MEDIUM 0x20
#define SENSOR_SHARPNESS_HIGH 255

#define SENSOR_ACTIVE_SENSOR (1)

CBaseSensor::CBaseSensor(SENSOR_CONFIG_T tSensorConfig)
    : m_tSnsCfg(tSensorConfig), m_eImgFormatSDR(AX_FORMAT_BAYER_RAW_10BPP), m_eImgFormatHDR(AX_FORMAT_BAYER_RAW_10BPP) {
}

AX_BOOL CBaseSensor::InitISP(AX_VOID) {
    memset(&m_tSnsAttr, 0, sizeof(AX_SNS_ATTR_T));
    memset(&m_tDevAttr, 0, sizeof(AX_VIN_DEV_ATTR_T));
    memset(&m_tMipiRxDev, 0, sizeof(AX_MIPI_RX_DEV_T));

    InitMipiRxAttr();
    InitSnsAttr();
    InitSnsClkAttr();
    InitDevAttr();
    InitPipeAttr();
    InitChnAttr();
    InitSnsLibraryInfo();
    InitAbilities();

    if (m_cbAttrUpd) {
        m_cbAttrUpd(this);
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::Init() {
    if (!InitISP()) {
        LOG_M_E(SENSOR, "Sensor %d init failed.", m_tSnsCfg.nSnsID);
        return AX_FALSE;
    }

    if (!InitEZoom()) {
        return AX_FALSE;
    }

#ifndef USE_STATIC_LIBS
    if (!m_pSnsLib) {
        AX_DLOPEN(m_pSnsLib, m_tSnsCfg.m_tSnsLibInfo.strLibName.c_str());
    }

    if (!m_pSnsLib) {
        LOG_M_E(SENSOR, "Load %s fail, %s", m_tSnsCfg.m_tSnsLibInfo.strLibName.c_str(), strerror(errno));
        return AX_FALSE;
    }

    AX_DLVAR_LOAD(m_pSnsObj, m_pSnsLib, m_tSnsCfg.m_tSnsLibInfo.strObjName.c_str(), AX_SENSOR_REGISTER_FUNC_T);

    if (!m_pSnsObj) {
        LOG_M_E(SENSOR, "Find symbol(%s) fail, %s", m_tSnsCfg.m_tSnsLibInfo.strObjName.c_str(), strerror(errno));
        return AX_FALSE;
    }
#endif

    if (!m_pSps) {
        m_pSps = new CSoftPhotoSensitivity(this);
    }

    if (!m_pHnb) {
        m_pHnb = new CHotNoiseBalance(this);
    }

    if (!m_pAeRoi) {
        m_pAeRoi = new CIspAlgoAeRoi(this);
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::DeInit() {
    LOG_MM_C(SENSOR, "[Dev:%d] +++", m_tSnsCfg.nDevID);

    memset(&m_tSnsAttr, 0, sizeof(AX_SNS_ATTR_T));
    memset(&m_tDevAttr, 0, sizeof(AX_VIN_DEV_ATTR_T));
    memset(&m_tMipiRxDev, 0, sizeof(AX_MIPI_RX_DEV_T));

    m_cbAttrUpd = nullptr;

#ifndef USE_STATIC_LIBS
    AX_DLCLOSE(m_pSnsLib);
#endif

    m_pSnsObj = nullptr;

    SAFE_DELETE_PTR(m_pSps);

    SAFE_DELETE_PTR(m_pHnb);

    SAFE_DELETE_PTR(m_pAeRoi);

    LOG_MM_C(SENSOR, "[Dev:%d] ---", m_tSnsCfg.nDevID);
    return AX_TRUE;
}

AX_BOOL CBaseSensor::Open() {
    if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SUB) {
        return AX_TRUE;
    }

    if (!OpenAll()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::Close() {
    if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SUB) {
        return AX_TRUE;
    }

    if (!CloseAll()) {
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::OpenAll() {
    LOG_MM_C(SENSOR, "[Dev:%d] +++", m_tSnsCfg.nDevID);

    LOG_MM(SENSOR, "Sensor Attr => w:%d h:%d framerate:%.2f sensor mode:%d rawType:%d", m_tSnsAttr.nWidth, m_tSnsAttr.nHeight,
           m_tSnsAttr.fFrameRate, m_tSnsAttr.eSnsMode, m_tSnsAttr.eRawType);

    AX_S32 nRet = 0;
    AX_U8 nDevID = m_tSnsCfg.nDevID;
    AX_U8 nRxDevID = m_tSnsCfg.nRxDevID;

    // open sensor mclk
    nRet = AX_ISP_OpenSnsClk(m_tSnsClkAttr.nSnsClkIdx, m_tSnsClkAttr.eSnsClkRate);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_ISP_OpenSnsClk[%d] failed, ret=0x%x.", m_tSnsClkAttr.nSnsClkIdx, nRet);
        return AX_FALSE;
    } else {
        LOG_M_C(SENSOR, "AX_ISP_OpenSnsClk[%d]", m_tSnsClkAttr.nSnsClkIdx);
    }

    for (auto &m : m_mapPipe2Attr) {
        AX_U8 nPipeID = m.first;
        AX_U8 nResetGpioNum = (nPipeID == m_tSwitchSubSnsInfo.nPipeID) ? m_tSwitchSubSnsInfo.nResetGpioNum : m_tSnsCfg.nResetGpioNum;
        AX_SENSOR_REGISTER_FUNC_T* pSnsObj = (nPipeID == m_tSwitchSubSnsInfo.nPipeID) ? m_tSwitchSubSnsInfo.pSnsObj : m_pSnsObj;

        // SNS reset sensor obj
        if ((AX_U8)(-1) != nResetGpioNum) {
            if (m_tSnsCfg.bNeedResetSensor) {
                if (!ResetSensorObj(nPipeID, pSnsObj, nResetGpioNum)) {
                    LOG_M_E(SENSOR, "[%d]ResetSensorObj with nResetGpioNum(%d) failed, ret=0x%x.", nPipeID, nResetGpioNum, nRet);
                    return AX_FALSE;
                }

                if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE) {
                    if (!ResetSensorObj((nPipeID + 1), pSnsObj, m_tSnsCfg.tSinglePipeSwitchInfo.nResetGpioNum)) {
                        LOG_M_E(SENSOR, "[%d]ResetSensorObj with switched pipe nResetGpioNum(%d) failed, ret=0x%x.",
                                         (nPipeID + 1), m_tSnsCfg.tSinglePipeSwitchInfo.nResetGpioNum, nRet);
                        return AX_FALSE;
                    }
                }
            } else {
                CElapsedTimer::GetInstance()->mSleep(15);
            }
        }
    }
    m_tSnsCfg.bNeedResetSensor = AX_FALSE;

    // step 1: AX_MIPI_RX_SetLaneCombo
    if (m_tMipiRxDev.eInputMode == AX_INPUT_MODE_MIPI && m_tMipiRxDev.tMipiAttr.eLaneNum == AX_MIPI_DATA_LANE_4)
        nRet = AX_MIPI_RX_SetLaneCombo(AX_LANE_COMBO_MODE_0);
    else
        nRet = AX_MIPI_RX_SetLaneCombo(AX_LANE_COMBO_MODE_1);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_MIPI_RX_SetLaneCombo failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    // step 2: AX_MIPI_RX_SetAttr
    nRet = AX_MIPI_RX_SetAttr(nRxDevID, &m_tMipiRxDev);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "RxDev[%d] AX_MIPI_RX_SetAttr failed, ret=0x%x.", nRxDevID, nRet);
        return AX_FALSE;
    }

    // step 3: AX_MIPI_RX_Reset
    nRet = AX_MIPI_RX_Reset(nRxDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "RxDev[%d] AX_MIPI_RX_Reset failed, ret=0x%x.", nRxDevID, nRet);
        return AX_FALSE;
    }

    // step 4: AX_MIPI_RX_Start
    nRet = AX_MIPI_RX_Start(nRxDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "RxDev[%d] AX_MIPI_RX_Start failed, ret=0x%x.", nRxDevID, nRet);
        return AX_FALSE;
    } else {
        LOG_M_C(SENSOR, "RxDev[%d] AX_MIPI_RX_Start", nRxDevID);
    }

    // step 5: AX_VIN_CreateDev
    if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN) {
        m_tDevAttr.tDevImgRgn[1].nWidth = m_tSwitchSubSnsInfo.nSnsOutWidth;
        m_tDevAttr.tDevImgRgn[1].nHeight = m_tSwitchSubSnsInfo.nSnsOutHeight;
        m_tDevAttr.eSnsOutputMode = AX_SNS_NORMAL;
    }

    nRet = AX_VIN_CreateDev(nDevID, &m_tDevAttr);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "[%d] AX_VIN_CreateDev failed, ret=0x%x.", nDevID, nRet);
        return AX_FALSE;
    } else {
        LOG_M_C(SENSOR, "[%d] AX_VIN_CreateDev", nDevID);
    }

    // step 6: AX_VIN_SetDevAttr
    nRet = AX_VIN_SetDevAttr(nDevID, &m_tDevAttr);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "[%d] AX_VIN_SetDevAttr failed, ret=0x%x.", nDevID, nRet);
        return AX_FALSE;
    }

    // if (AX_VIN_DEV_OFFLINE == m_tDevAttr.eDevMode) {
    //     AX_VIN_DUMP_ATTR_T tDumpAttr;
    //     tDumpAttr.bEnable = AX_TRUE;
    //     tDumpAttr.nDepth = 3;
    //     nRet = AX_VIN_SetDevDumpAttr(nDevID, AX_VIN_DUMP_QUEUE_TYPE_DEV, &tDumpAttr);
    //     if (0 != nRet) {
    //         LOG_M_E(SENSOR, "AX_VIN_SetDevDumpAttr failed, ret=0x%x.", nRet);
    //         return AX_FALSE;
    //     }
    // }

    AX_VIN_DEV_BIND_PIPE_T tDevBindPipe = {0};
    for (AX_U8 i = 0; i < GetPipeCount(); i++) {
        AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        tDevBindPipe.nPipeId[i] = nPipeID;
        tDevBindPipe.nNum += 1;

        switch (m_tSnsAttr.eSnsMode) {
            case AX_SNS_LINEAR_MODE:
            case AX_SNS_LINEAR_ONLY_MODE:
                tDevBindPipe.nHDRSel[i] = 0x1;
                break;
            case AX_SNS_HDR_2X_MODE:
                tDevBindPipe.nHDRSel[i] = 0x1 | 0x2;
                break;
            case AX_SNS_HDR_3X_MODE:
                tDevBindPipe.nHDRSel[i] = 0x1 | 0x2 | 0x4;
                break;
            case AX_SNS_HDR_4X_MODE:
                tDevBindPipe.nHDRSel[i] = 0x1 | 0x2 | 0x4 | 0x8;
                break;
            default:
                tDevBindPipe.nHDRSel[i] = 0x1;
                break;
        }
    }

    if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN) {
        tDevBindPipe.nHDRSel[0] = 1;
        tDevBindPipe.nHDRSel[1] = 2;
    }
    // step 7: AX_VIN_SetDevBindPipe
    nRet = AX_VIN_SetDevBindPipe(nDevID, &tDevBindPipe);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_SetDevBindPipe failed, ret=0x%x.", nRet);
        return AX_FALSE;
    }

    nRet = AX_VIN_SetDevBindMipi(nDevID, nRxDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_VIN_SetDevBindMipi(%d-%d) failed, ret=0x%x", nDevID, nRxDevID, nRet);
        return AX_FALSE;
    }

    for (auto &m : m_mapPipe2Attr) {
        AX_U8 nPipeID = m.first;
        AX_VIN_PIPE_ATTR_T &tPipeAttr = m.second;
        AX_U8 i = 0;
        for (; i < GetPipeCount(); i++) {
            if (m_tSnsCfg.arrPipeAttr[i].nPipeID == nPipeID) {
                break;
            }
        }

        // step 8: AX_VIN_CreatePipe
        if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN) {
            tPipeAttr.eSnsMode = AX_SNS_HDR_2X_MODE;
            if (m_tSwitchSubSnsInfo.nPipeID == nPipeID) {
                tPipeAttr.eSnsMode = AX_SNS_LINEAR_MODE;
                tPipeAttr.tPipeImgRgn.nWidth = m_tSwitchSubSnsInfo.nSnsOutWidth;
                tPipeAttr.tPipeImgRgn.nHeight = m_tSwitchSubSnsInfo.nSnsOutHeight;
            }
        }
        nRet = AX_VIN_CreatePipe(nPipeID, &tPipeAttr);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_VIN_CreatePipe failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_VIN_CreatePipe", nPipeID);
        }

        // step 9: AX_VIN_SetPipeAttr
        nRet = AX_VIN_SetPipeAttr(nPipeID, &tPipeAttr);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_VIN_SetPipeAttr failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        }

        // step 10: AX_VIN_SetPipeFrameSource
        if (tPipeAttr.ePipeWorkMode == AX_VIN_PIPE_NORMAL_MODE1) {
            if (m_tSnsCfg.eSwitchSnsType != E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE) {
                nRet = AX_VIN_SetPipeFrameSource(nPipeID, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_DEV);

                if (0 != nRet) {
                    LOG_M_E(SENSOR, "[%d] AX_VIN_SetPipeFrameSource failed, ret=0x%x.", nPipeID, nRet);
                    return AX_FALSE;
                }

                // AX_VIN_DUMP_ATTR_T tPipeDumpAttr;
                // memset(&tPipeDumpAttr, 0, sizeof(tPipeDumpAttr));

                // tPipeDumpAttr.bEnable = AX_TRUE;
                // tPipeDumpAttr.nDepth = 3;
                // nRet = AX_VIN_SetPipeDumpAttr(nPipeID, AX_VIN_PIPE_DUMP_NODE_IFE, AX_VIN_DUMP_QUEUE_TYPE_DEV, &tPipeDumpAttr);
                // if (nRet) {
                //     LOG_M_E(SENSOR, "pipe %d node %d dump attr set failed....\n", nPipeID, AX_VIN_PIPE_DUMP_NODE_IFE);
                // }

                // nRet = AX_VIN_SetPipeFrameSource(nPipeID, AX_VIN_FRAME_SOURCE_ID_ITP, AX_VIN_FRAME_SOURCE_TYPE_USER);
                // if (nRet) {
                //     LOG_M_E(SENSOR, "pipe %d src %d  frame source set failed....\n", nPipeID, AX_VIN_FRAME_SOURCE_ID_ITP);
                // }
                // tPipeDumpAttr.bEnable = AX_TRUE;
                // tPipeDumpAttr.nDepth = 3;
                // nRet = AX_VIN_SetPipeDumpAttr(nPipeID, AX_VIN_PIPE_DUMP_NODE_MAIN, AX_VIN_DUMP_QUEUE_TYPE_DEV, &tPipeDumpAttr);
                // if (nRet) {
                //     LOG_M_E(SENSOR, "pipe %d src %d  dump attr set failed....\n", nPipeID, AX_VIN_PIPE_DUMP_NODE_MAIN);
                // }
            } else { // E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE
                AX_VIN_DUMP_ATTR_T tPipeDumpAttr;
                memset(&tPipeDumpAttr, 0, sizeof(tPipeDumpAttr));
                tPipeDumpAttr.bEnable = AX_TRUE;
                tPipeDumpAttr.nDepth = 1;

                nRet = AX_VIN_SetPipeDumpAttr(nPipeID, AX_VIN_PIPE_DUMP_NODE_IFE, AX_VIN_DUMP_QUEUE_TYPE_DEV, &tPipeDumpAttr);
                if (nRet) {
                    LOG_MM_E(SENSOR, "pipe %d node %d dump attr set failed!", nPipeID, AX_VIN_PIPE_DUMP_NODE_IFE);
                    return AX_FALSE;
                }

                nRet = AX_VIN_SetPipeFrameSource(nPipeID, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_DEV);
                if (nRet) {
                    LOG_MM_E(SENSOR, "pipe %d src (id:%d, type:%d) frame source set failed!",
                                      nPipeID, AX_VIN_PIPE_DUMP_NODE_IFE, AX_VIN_FRAME_SOURCE_TYPE_DEV);
                    return AX_FALSE;
                }
                nRet = AX_VIN_SetPipeFrameSource(nPipeID, AX_VIN_FRAME_SOURCE_ID_ITP, AX_VIN_FRAME_SOURCE_TYPE_USER);
                if (nRet) {
                    LOG_MM_E(SENSOR, "pipe %d src (id:%d, type:%d) frame source set failed!",
                                      nPipeID, AX_VIN_FRAME_SOURCE_ID_ITP, AX_VIN_FRAME_SOURCE_TYPE_USER);
                    return AX_FALSE;
                }
            }
        }

        // step 11: RegisterSensor
        AX_U8 nBusType = (nPipeID == m_tSwitchSubSnsInfo.nPipeID) ? m_tSwitchSubSnsInfo.nBusType : m_tSnsCfg.nBusType;
        AX_U8 nDevNode = (nPipeID == m_tSwitchSubSnsInfo.nPipeID) ? m_tSwitchSubSnsInfo.nDevNode : m_tSnsCfg.nDevNode;
        AX_U8 nI2cAddr = (nPipeID == m_tSwitchSubSnsInfo.nPipeID) ? m_tSwitchSubSnsInfo.nI2cAddr : m_tSnsCfg.nI2cAddr;
        AX_SENSOR_REGISTER_FUNC_T* pSnsObj = (nPipeID == m_tSwitchSubSnsInfo.nPipeID) ? m_tSwitchSubSnsInfo.pSnsObj : m_pSnsObj;
        if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE) {
            if (!RegisterSinglePipeSensors(nPipeID)) {
                LOG_M_E(SENSOR, "Pipe[%d] RegisterSinglePipeSensors failed, ret=0x%x.", nPipeID, nRet);
                return AX_FALSE;
            }
        } else {
            LOG_M_C(SENSOR, "Pipe[%d] nBusType: %d, nDevNode: %d, nI2cAddr: %d.", nPipeID, nBusType, nDevNode, nI2cAddr);
            if (!RegisterSensor(nPipeID, pSnsObj, nBusType, nDevNode, nI2cAddr)) {
                LOG_M_E(SENSOR, "Pipe[%d] RegisterSensor failed, ret=0x%x.", nPipeID, nRet);
                return AX_FALSE;
            }
        }

        /* Todo: SDR=4 ; HDR=5*/
        if (IS_SNS_LINEAR_MODE(m_tSnsCfg.eSensorMode)) {
            m_tSnsAttr.nSettingIndex = m_tSnsCfg.arrSettingIndex[AX_SNS_LINEAR_MODE];
        } else {
            m_tSnsAttr.nSettingIndex = m_tSnsCfg.arrSettingIndex[m_tSnsCfg.eSensorMode];
        }

        LOG_M_C(SENSOR, "Pipe[%d] sensor setting index: %d", nPipeID, m_tSnsAttr.nSettingIndex);

#ifdef SWITCH_SENSOR_SUPPORT
        AX_S32 nScenario = 0;
        if (!CCmdLineParser::GetInstance()->GetScenario(nScenario)) {
            LOG_M_W(SENSOR, "Load default scenario %d", nScenario);
        }
        if ((nScenario == E_APP_SCENARIO_TRIPLE_SENSOR_SWITCH_SINGLE_PIPE)
            && (m_tSnsCfg.eSensorType == E_SNS_TYPE_SC850SL)) {
            if (m_tSnsCfg.eSensorMode == AX_SNS_LINEAR_MODE) {
                m_tSnsAttr.fFrameRate = 20.0;
            } else {
                m_tSnsAttr.fFrameRate = 15.0;
            }
            m_tSnsCfg.fFrameRate = m_tSnsAttr.fFrameRate;
            LOG_MM_I(SENSOR, "Sensor(type:%d) fps: %f for sensor mode: %d",
                              m_tSnsCfg.eSensorType, m_tSnsAttr.fFrameRate, m_tSnsCfg.eSensorMode);
        }
#endif
        // step 12: SetSnsAttr
        if (!SetSnsAttr(nPipeID, m_tSnsAttr)) {
            return AX_FALSE;
        }

        // step 13: AX_ISP_Create
        nRet = AX_ISP_Create(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_ISP_Create failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_ISP_Create", nPipeID);
        }

        // step 14: RegisterAlgo Callback
        if (!m_algWrapper.RegisterAlgoToSensor(pSnsObj, nPipeID)) {
            LOG_M_E(SENSOR, "[%d] RegisterAlgoToSensor failed.", nPipeID);
            return AX_FALSE;
        }

        // step 15: AX_ISP_LoadBinParams
        LoadBinParams(nPipeID, m_tSnsCfg.arrPipeAttr[i].vecTuningBin);

        // step 16: AX_ISP_Open
        nRet = AX_ISP_Open(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_ISP_Open failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_ISP_Open", nPipeID);
        }

        for (AX_U8 j = AX_VIN_CHN_ID_MAIN; j < AX_VIN_CHN_ID_MAX; j++) {
            AX_VIN_CHN_ATTR_T &tChnAttr = m_mapPipe2ChnAttr[nPipeID][j];

            AX_VIN_CHN_ATTR_T tChnAttr2 = tChnAttr;
            // step 17: AX_VIN_SetChnAttr
            nRet = AX_VIN_SetChnAttr(nPipeID, (AX_VIN_CHN_ID_E)j, &tChnAttr2);
            if (0 != nRet) {
                LOG_M_E(SENSOR, "[%d] AX_VIN_SetChnAttr failed, ret=0x%x.", nPipeID, nRet);
                return AX_FALSE;
            }

            if (m_tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].eFrmMode == AX_VIN_FRAME_MODE_RING) {
                nRet = AX_VIN_SetChnFrameMode(nPipeID, (AX_VIN_CHN_ID_E)j, m_tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].eFrmMode);
                if (0 != nRet) {
                    LOG_M_E(SENSOR, "[%d] AX_VIN_SetChnFrameMode[%d] failed, ret=0x%x.", nPipeID, j, nRet);
                }
            }

            if (m_tSnsCfg.nDiscardYUVFrmNum > 0) {
                nRet = AX_VIN_SetDiscardYuvFrameNumbers(nPipeID, (AX_VIN_CHN_ID_E)j, m_tSnsCfg.nDiscardYUVFrmNum);
                if (0 != nRet) {
                    LOG_M_E(SENSOR, "[%d] AX_VIN_SetDiscardYuvFrameNumbers[%d] failed, frm-num=%d, ret=0x%x.", nPipeID, j, m_tSnsCfg.nDiscardYUVFrmNum, nRet);
                }
            }

            if (m_tSnsCfg.bMirror) {
                nRet = AX_VIN_SetPipeMirror(nPipeID, AX_TRUE);
                if (0 != nRet) {
                    LOG_M_E(SENSOR, "[%d] AX_VIN_SetPipeMirror failed, ret=0x%x.", nPipeID, nRet);
                    return AX_FALSE;
                }
            }

            if (m_tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].bChnEnable) {
                if (m_tSnsCfg.bFlip) {
                    nRet = AX_VIN_SetChnFlip(nPipeID, (AX_VIN_CHN_ID_E)j, AX_TRUE);
                    if (0 != nRet) {
                        LOG_M_E(SENSOR, "[%d] AX_VIN_SetChnFlip failed, ret=0x%x.", nPipeID, nRet);
                        return AX_FALSE;
                    }
                }

                // step 18: AX_VIN_EnableChn
                std::map<AX_U8, SENSOR_VINIVPS_CFG_T> mapVinIvps = CSensorOptionHelper::GetInstance()->GetMapVinIvps();
                for (auto& m : mapVinIvps) {
                    AX_U8 nVinId = m.first;
                    AX_U8 nIvpsId = m.second.nIvpsGrp;
                    AX_S8 nVinIvpsMode = m.second.nVinIvpsMode;
                    if ((nVinId == nPipeID && AX_ITP_ONLINE_VPP == (AX_VIN_IVPS_MODE_E)nVinIvpsMode)
                        && !m_tSnsCfg.arrPipeAttr[i].bTuning) {
                        LOG_M_C(SENSOR, "[%d] ITP_ONLINE_VPP[%d] ...", nPipeID, nIvpsId);
                    } else if (nVinId == nPipeID) {
                        nRet = AX_VIN_EnableChn(nPipeID, (AX_VIN_CHN_ID_E)j);
                        if (0 != nRet) {
                            LOG_M_E(SENSOR, "[%d] AX_VIN_EnableChn[%d] failed, ret=0x%x.", nPipeID, j, nRet);
                            return AX_FALSE;
                        } else {
                            LOG_M_C(SENSOR, "Pipe[%d] AX_VIN_EnableChn[%d]", nPipeID, j);
                        }
                    }
                }
            }
        }

        // step 19: AX_VIN_StartPipe
        nRet = AX_VIN_StartPipe(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "AX_VIN_StartPipe failed, ret=0x%x.", nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_VIN_StartPipe", nPipeID);
        }

        // step 20: AX_ISP_Start
        nRet = AX_ISP_Start(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_ISP_Start failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_ISP_Start", nPipeID);
        }
    }

    // step 21: AX_VIN_EnableDev
    nRet = AX_VIN_EnableDev(nDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "[%d] AX_VIN_EnableDev failed, ret=0x%x.", nDevID, nRet);
        return AX_FALSE;
    } else {
        LOG_M_C(SENSOR, "[%d] AX_VIN_EnableDev", nDevID);
    }

    if (IsSnsSync()) {
        SetMultiSnsSync(AX_TRUE);
    }

#ifdef SWITCH_SENSOR_SUPPORT
    if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN || m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE) {
        if (!EnableMipiSwitch()) {
            return AX_FALSE;
        }
    }
#endif

    {
        std::lock_guard<std::mutex> _ApiLck(m_mtx);
        m_bSensorStarted = AX_TRUE;
    }

    if (!RestoreSnsAttr()) {
        return AX_FALSE;
    }

    // step 22: AX_ISP_StreamOn
    for (AX_U8 i = 0; i < GetPipeCount(); i++) {
        AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;
        nRet = AX_ISP_StreamOn(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_ISP_StreamOn failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_ISP_StreamOn", nPipeID);
        }

        if (E_SNS_HDR_TYPE_LONGFRAME == m_tSnsCfg.eHdrType) { // long frame hdr
            AX_ISP_LFHDR_MODE_E eLFHDRMode = (SNS_MODE_LFHDR_SDR == m_tSnsCfg.nSensorMode) ? AX_ISP_LFHDR_LONG_FRAME_MODE : AX_ISP_LFHDR_NORMAL_MODE;
            nRet = AX_ISP_IQ_SetAeLongFrameMode(nPipeID, eLFHDRMode);
            if (0 != nRet) {
                LOG_M_E(SENSOR, "Sns[%d] Pipe[%d] AX_ISP_IQ_SetAeLongFrameMode(%d) faile, ret: 0x%x",
                                    m_tSnsCfg.nSnsID, nPipeID, eLFHDRMode, nRet);
                return AX_FALSE;
            }
        }
    }

    if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN && m_tSwitchSubSnsInfo.pSwitchSns) {
        CBaseSensor* pSwitchSns = (CBaseSensor*)m_tSwitchSubSnsInfo.pSwitchSns;
        pSwitchSns->SetSnsStartStatus(AX_TRUE);
    }

    // get shutter mode
    m_tSnsCfg.eShutterMode = GetShutterMode();

    LOG_MM_C(SENSOR, "[Dev:%d] ---", m_tSnsCfg.nDevID);
    return AX_TRUE;
}

AX_BOOL CBaseSensor::CloseAll() {
    LOG_MM_C(SENSOR, "[Dev:%d] +++", m_tSnsCfg.nDevID);
    // stop sensor application
    {
        // stop sensor soft photo sensitivity
        if (m_pSps) {
            m_pSps->Stop();
        }

        // stop hot noise balance
        if (m_pHnb) {
            m_pHnb->Stop();
        }

        // stop ae roi
        if (m_pAeRoi) {
            m_pAeRoi->Stop();
        }
    }

    std::lock_guard<std::mutex> _ApiLck(m_mtx);
    AX_S32 nRet = AX_SUCCESS;
    m_bSensorStarted = AX_FALSE;
    if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN && m_tSwitchSubSnsInfo.pSwitchSns) {
        CBaseSensor* pSwitchSns = (CBaseSensor*)m_tSwitchSubSnsInfo.pSwitchSns;
        pSwitchSns->SetSnsStartStatus(AX_FALSE);
    }

    // step 1: AX_ISP_Stop
    for (AX_U8 i = 0; i < GetPipeCount(); i++) {
        AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;
        nRet = AX_ISP_Stop(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_ISP_Stop failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_ISP_Stop", nPipeID);
        }
    }

    // step 2: AX_VIN_DisableDev
    AX_U8 nDevID = m_tSnsCfg.nDevID;
    AX_U8 nRxDevID = m_tSnsCfg.nRxDevID;
    nRet = AX_VIN_DisableDev(nDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "[%d] AX_VIN_DisableDev failed, ret=0x%x.", nDevID, nRet);
        return AX_FALSE;
    } else {
        LOG_M_C(SENSOR, "[%d] AX_VIN_DisableDev", nDevID);
    }

    // if (AX_VIN_DEV_OFFLINE == m_tSnsCfg.eDevMode) {
    //     AX_VIN_DUMP_ATTR_T tDumpAttr;
    //     tDumpAttr.bEnable = AX_FALSE;
    //     nRet = AX_VIN_SetDevDumpAttr(nDevID, AX_VIN_DUMP_QUEUE_TYPE_DEV, &tDumpAttr);
    //     if (0 != nRet) {
    //         LOG_M_E(SENSOR, "AX_VIN_SetDevDumpAttr failed, ret=0x%x.", nRet);
    //         return AX_FALSE;
    //     }
    // }

    // step 3: AX_ISP_StreamOff
    for (AX_U8 i = 0; i < GetPipeCount(); i++) {
        AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;
        nRet = AX_ISP_StreamOff(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_ISP_StreamOff failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_ISP_StreamOff", nPipeID);
        }
    }

    // step 4: AX_ISP_CloseSnsClk
    nRet = AX_ISP_CloseSnsClk(m_tSnsClkAttr.nSnsClkIdx);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "AX_ISP_CloseSnsClk[%d] failed, ret=0x%x.", m_tSnsClkAttr.nSnsClkIdx, nRet);
        return AX_FALSE;
    } else {
        LOG_M_C(SENSOR, "AX_ISP_CloseSnsClk[%d]", m_tSnsClkAttr.nSnsClkIdx);
    }

    for (AX_U8 i = 0; i < GetPipeCount(); i++) {
        AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        // step 5: AX_VIN_StopPipe
        nRet = AX_VIN_StopPipe(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_VIN_StopPipe failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_VIN_StopPipe", nPipeID);
        }

        // step 6: AX_VIN_DisableChn
        for (AX_U8 j = AX_VIN_CHN_ID_MAIN; j < AX_VIN_CHN_ID_MAX; j++) {
            if (m_tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].bChnEnable) {
                std::map<AX_U8, SENSOR_VINIVPS_CFG_T> mapVinIvps = CSensorOptionHelper::GetInstance()->GetMapVinIvps();
                for (auto& m : mapVinIvps) {
                    AX_U8 nVinId = m.first;
                    AX_U8 nIvpsId = m.second.nIvpsGrp;
                    AX_S8 nVinIvpsMode = m.second.nVinIvpsMode;
                    if ((nVinId == nPipeID && AX_ITP_ONLINE_VPP == (AX_VIN_IVPS_MODE_E)nVinIvpsMode)
                        && !m_tSnsCfg.arrPipeAttr[i].bTuning) {
                        LOG_M_C(SENSOR, "[%d] ITP_ONLINE_VPP[%d] ...", nPipeID, nIvpsId);
                    } else {
                        nRet = AX_VIN_DisableChn(nPipeID, (AX_VIN_CHN_ID_E)j);
                        if (0 != nRet) {
                            LOG_M_E(SENSOR, "[%d] AX_VIN_DisableChn[%d] failed, ret=0x%x.", nPipeID, j, nRet);
                            return AX_FALSE;
                        } else {
                            LOG_M_C(SENSOR, "[%d] AX_VIN_DisableChn[%d]", nPipeID, j);
                        }
                    }
                }
            }
        }

        // step 6: AX_ISP_Close
        nRet = AX_ISP_Close(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_ISP_Close failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_ISP_Close", nPipeID);
        }

        // step 7: UnRegister Algo
        if (!m_algWrapper.UnRegisterAlgoFromSensor(nPipeID)) {
            LOG_M_E(SENSOR, "UnRegisterAlgoFromSensor failed.");
            return AX_FALSE;
        }

        // step 8: AX_ISP_Destroy
        nRet = AX_ISP_Destroy(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_ISP_Destroy failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_ISP_Destroy", nPipeID);
        }

        // step 9: UnRegisterSensor
        if (!UnRegisterSensor(nPipeID)) {
            LOG_M_E(SENSOR, "UnRegisterSensor failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }

        // step 10: AX_VIN_DestroyPipe
        nRet = AX_VIN_DestroyPipe(nPipeID);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d] AX_VIN_DestroyPipe failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "[%d] AX_VIN_DestroyPipe", nPipeID);
        }
    }

    // step 11: AX_VIN_DestroyPipe
    nRet = AX_MIPI_RX_Stop(nRxDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "RxDev[%d] AX_MIPI_RX_Stop failed, ret=0x%x.", nRxDevID, nRet);
        return AX_FALSE;
    } else {
        LOG_M_C(SENSOR, "RxDev[%d] AX_MIPI_RX_Stop", nRxDevID);
    }

    // step 12: AX_VIN_DestroyPipe
    nRet = AX_VIN_DestroyDev(nDevID);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "[%d] AX_VIN_DestroyDev failed, ret=0x%x.", nDevID, nRet);
        return AX_FALSE;
    } else {
        LOG_M_C(SENSOR, "[%d] AX_VIN_DestroyDev", nDevID);
    }

    LOG_MM_C(SENSOR, "[Dev:%d] ---", m_tSnsCfg.nDevID);
    return AX_TRUE;
}

AX_BOOL CBaseSensor::RestoreSnsAttr() {
    // set hdr ratio
    if (IS_SNS_HDR_MODE(m_tSnsCfg.eSensorMode)
        && m_tSnsCfg.tHdrRatioAttr.bEnable) {
        SetHdrRatio(m_tSnsCfg.tHdrRatioAttr.nRatio);
    }

    // scene mode
    UpdateSceneMode();

    // ldc
    if (m_tSnsCfg.arrPipeAttr[0].tLdcAttr.bLdcEnable) {
        UpdateLDC(m_tSnsCfg.arrPipeAttr[0].tLdcAttr.bLdcEnable,
                    m_tSnsCfg.arrPipeAttr[0].tLdcAttr.bLdcAspect,
                    m_tSnsCfg.arrPipeAttr[0].tLdcAttr.nLdcXRatio,
                    m_tSnsCfg.arrPipeAttr[0].tLdcAttr.nLdcYRatio,
                    m_tSnsCfg.arrPipeAttr[0].tLdcAttr.nLdcXYRatio,
                    m_tSnsCfg.arrPipeAttr[0].tLdcAttr.nLdcDistortionRatio);
    }

    // DIS
    if (m_tSnsCfg.arrPipeAttr[0].tDisAttr.bDisEnable) {
        UpdateDIS(m_tSnsCfg.arrPipeAttr[0].tDisAttr.bDisEnable);
    }

    {
        // rotation
        if (m_tSnsCfg.eRotation != AX_ROTATION_0) {
            UpdateRotation((AX_VIN_ROTATION_E)m_tSnsCfg.eRotation);
        } else {
            // zoom
            {
                if (m_tSnsCfg.tEZoomAttr.fEZoomRatio > 1.0) {
                    memset(m_tPreCropRect, 0, sizeof(m_tPreCropRect));
                    EZoom(m_tSnsCfg.tEZoomAttr.fEZoomRatio);
                }
            }
        }
    }

    // fps
    // fps.2: update initial fps
    {
        if (m_tSnsCfg.fFrameRate != m_tSnsAttr.fFrameRate
            || m_tSnsAttr.fFrameRate < m_tAbilities.fShutterSlowFpsThr) {
            m_tSnsAttr.fFrameRate = m_tSnsCfg.fFrameRate;

            UpdateSnsAttr();
        }
    }

    // day night
    {
        if (m_tSnsCfg.eDayNight == AX_DAYNIGHT_MODE_NIGHT) {
            ChangeDaynightMode(m_tSnsCfg.eDayNight);
        }

        if (m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN && m_tSwitchSubSnsInfo.pSwitchSns) {
            CBaseSensor* pSwitchSns = (CBaseSensor*)m_tSwitchSubSnsInfo.pSwitchSns;
            AX_DAYNIGHT_MODE_E eDayNightMode = pSwitchSns->GetSnsConfig().eDayNight;
            if (eDayNightMode == AX_DAYNIGHT_MODE_NIGHT) {
                pSwitchSns->ChangeDaynightMode(eDayNightMode);
            }
        }
    }

    // color
    {
        if (m_tSnsCfg.tColorAttr.bColorManual) {
            APP_ISP_IMAGE_ATTR_T tAttr;

            tAttr.nAutoMode = m_tSnsCfg.tColorAttr.bColorManual ? 0 : 1;
            tAttr.nBrightness = (AX_U8)m_tSnsCfg.tColorAttr.fBrightness;
            tAttr.nSharpness = (AX_U8)m_tSnsCfg.tColorAttr.fSharpness;
            tAttr.nContrast = (AX_U8)m_tSnsCfg.tColorAttr.fContrast;
            tAttr.nSaturation = (AX_U8)m_tSnsCfg.tColorAttr.fSaturation;

            SetIspImageAttr(tAttr);
        }
    }

    // start sensor soft photo sensitivity
    if (m_pSps) {
        m_pSps->Start(m_tSnsCfg.tSoftPhotoSensitivityAttr);
    }

    // start hot noise balance
    if (m_pHnb) {
        m_pHnb->Start(m_tSnsCfg.tHotNoiseBalanceAttr);
    }

    // ae roi
    if (m_tSnsCfg.tAeRoiAttr.bFaceAeRoiEnable
        || m_tSnsCfg.tAeRoiAttr.bVehicleAeRoiEnable) {
        SetAeRoiAttr(m_tSnsCfg.tAeRoiAttr);
    }

    // start ae roi
    if (m_pAeRoi) {
        m_pAeRoi->Start(m_tSnsCfg.tAeRoiAttr);
    }

    return AX_TRUE;
}

const AX_SNS_ATTR_T &CBaseSensor::GetSnsAttr(AX_VOID) const {
    return m_tSnsAttr;
}

AX_VOID CBaseSensor::SetSnsAttr(const AX_SNS_ATTR_T &tSnsAttr) {
    // fps.3: update fps
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    // restore shutter mode
    if (m_tSnsAttr.fFrameRate < m_tAbilities.fShutterSlowFpsThr
        && tSnsAttr.fFrameRate >= m_tAbilities.fShutterSlowFpsThr
        && m_tSnsCfg.eShutterMode == E_SNS_SHUTTER_SLOW_SHUTTER_MODE_E) {
        SNS_SHUTTER_MODE_E eShutterMode = GetShutterMode();

        if (eShutterMode != E_SNS_SHUTTER_SLOW_SHUTTER_MODE_E) {
            SetShutterMode(E_SNS_SHUTTER_SLOW_SHUTTER_MODE_E);
        }
    }

    m_tSnsAttr = tSnsAttr;
    m_tSnsCfg.fFrameRate = tSnsAttr.fFrameRate;

    UpdateSnsAttr();
}

const SNS_CLK_ATTR_T &CBaseSensor::GetSnsClkAttr(AX_VOID) const {
    return m_tSnsClkAttr;
}

AX_VOID CBaseSensor::SetSnsClkAttr(const SNS_CLK_ATTR_T &tClkAttr) {
    m_tSnsClkAttr = tClkAttr;
}

const AX_VIN_DEV_ATTR_T &CBaseSensor::GetDevAttr(AX_VOID) const {
    return m_tDevAttr;
}

AX_VOID CBaseSensor::SetDevAttr(const AX_VIN_DEV_ATTR_T &tDevAttr) {
    m_tDevAttr = tDevAttr;
}

const AX_MIPI_RX_DEV_T &CBaseSensor::GetMipiRxAttr(AX_VOID) const {
    return m_tMipiRxDev;
}

AX_VOID CBaseSensor::SetMipiRxAttr(const AX_MIPI_RX_DEV_T &tMipiRxAttr) {
    m_tMipiRxDev = tMipiRxAttr;
}

const AX_VIN_PIPE_ATTR_T &CBaseSensor::GetPipeAttr(AX_U8 nPipe) const {
    std::map<AX_U8, AX_VIN_PIPE_ATTR_T>::const_iterator itFinder = m_mapPipe2Attr.find(nPipe);
    if (itFinder == m_mapPipe2Attr.end()) {
        LOG_MM_E(SENSOR, "[Dev:%d] Pipe %d not configured.", m_tSnsCfg.nDevID, nPipe);
    }

    return itFinder->second;
}

AX_VOID CBaseSensor::SetPipeAttr(AX_U8 nPipe, const AX_VIN_PIPE_ATTR_T &tPipeAttr) {
    m_mapPipe2Attr[nPipe] = tPipeAttr;
}

const AX_VIN_CHN_ATTR_T &CBaseSensor::GetChnAttr(AX_U8 nPipe, AX_U8 nChannel) const {
    std::map<AX_U8, std::map<AX_U8, AX_VIN_CHN_ATTR_T>>::const_iterator itFinder = m_mapPipe2ChnAttr.find(nPipe);
    if (itFinder == m_mapPipe2ChnAttr.end()) {
        LOG_MM_E(SENSOR, "[Dev:%d] Pipe %d not configured.", m_tSnsCfg.nDevID, nPipe);
    }

    return itFinder->second.find(nChannel)->second;
}

AX_VOID CBaseSensor::SetChnAttr(AX_U8 nPipe, AX_U8 nChannel, const AX_VIN_CHN_ATTR_T &tChnAttr) {
    m_mapPipe2ChnAttr[nPipe][nChannel] = tChnAttr;
}

const SNS_ABILITY_T &CBaseSensor::GetAbilities(AX_VOID) const {
    return m_tAbilities;
}

const SENSOR_CONFIG_T &CBaseSensor::GetSnsConfig(AX_VOID) const {
    return m_tSnsCfg;
}

AX_U32 CBaseSensor::GetPipeCount() {
    return m_mapPipe2Attr.size();
}

AX_VOID CBaseSensor::RegisterIspAlgo(const ISP_ALGO_INFO_T &tAlg) {
    m_algWrapper.SetupAlgoInfo(tAlg);
}

AX_BOOL CBaseSensor::RegisterSensor(AX_U8 nPipe, AX_SENSOR_REGISTER_FUNC_T* pSnsObj, AX_U8 nBusType, AX_U8 nDevNode, AX_U8 nI2cAddr)  {
    AX_S32 ret;
    AX_SNS_COMMBUS_T tSnsBusInfo;
    memset(&tSnsBusInfo, 0, sizeof(tSnsBusInfo));

    ret = AX_ISP_RegisterSensor(nPipe, pSnsObj);
    if (ret) {
        LOG_M_E(SENSOR, "AX_ISP_RegisterSensor(pipe: %d), ret = 0x%08X", nPipe, ret);
        goto __FAIL0__;
    }

    if (ISP_SNS_CONNECT_I2C_TYPE == nBusType) {
        tSnsBusInfo.I2cDev = nDevNode;
    } else {
        tSnsBusInfo.SpiDev.bit4SpiDev = nDevNode;
        tSnsBusInfo.SpiDev.bit4SpiCs = 0;
    }

    if (pSnsObj->pfn_sensor_set_bus_info) {
        ret = pSnsObj->pfn_sensor_set_bus_info(nPipe, tSnsBusInfo);
        if (0 != ret) {
            LOG_M_E(SENSOR, "Sensor set bus info fail, ret = 0x%08X.", ret);
            goto __FAIL1__;
        } else {
            LOG_M_I(SENSOR, "set sensor bus idx %d", tSnsBusInfo.I2cDev);
        }
    } else {
        LOG_M_E(SENSOR, "sensor set buf info is not supported!");
        goto __FAIL1__;
    }

    if (NULL != pSnsObj->pfn_sensor_set_slaveaddr && (AX_U8)(-1) != nI2cAddr) {
        ret = pSnsObj->pfn_sensor_set_slaveaddr(nPipe, nI2cAddr);
        if (0 != ret) {
            LOG_M_E(SENSOR, "set sensor slave addr failed with %#x!\n", ret);
            goto __FAIL1__;
        }
        LOG_M_I(SENSOR, "set sensor slave addr %d\n", nI2cAddr);
    }

    return AX_TRUE;

__FAIL1__:
    AX_ISP_UnRegisterSensor(nPipe);

__FAIL0__:
    return AX_FALSE;
}

AX_BOOL CBaseSensor::RegisterSinglePipeSensors(AX_U8 nPipe)  {
    AX_S32 ret = -1;
    AX_SENSOR_REGISTER_FUNC_T* pSnsObj = m_pSnsObj;
    AX_U8 nBusType = m_tSnsCfg.nBusType;
    AX_BOOL bSnsRegistered = AX_FALSE;
    AX_SNS_COMMBUS_T tSnsBusInfo;
    memset(&tSnsBusInfo, 0, sizeof(tSnsBusInfo));

    AX_U8 nSnsNum = 2;
    AX_U8 arrHdlId[nSnsNum] = {m_tSnsCfg.tSinglePipeSwitchInfo.nHdlId0, m_tSnsCfg.tSinglePipeSwitchInfo.nHdlId1};
    AX_U8 arrDevNode[nSnsNum] = {m_tSnsCfg.tSinglePipeSwitchInfo.nDevNode, m_tSnsCfg.nDevNode};
    AX_U8 arrDevI2cAddr[nSnsNum] = {m_tSnsCfg.tSinglePipeSwitchInfo.nI2cAddr, m_tSnsCfg.nI2cAddr};

    LOG_M_C(SENSOR, "Pipe[%d] nBusType: %d, HdlId0: %d, HdlId1: %d, nDevNode: %d, nI2cAddr: %d, nDevNodeSwitch: %d, nI2cAddrSwitch: %d",
                    nPipe, nBusType, arrHdlId[0], arrHdlId[1], arrDevNode[1], arrDevI2cAddr[1], arrDevNode[0], arrDevI2cAddr[0]);

    for (AX_U8 i = 0; i < nSnsNum; i++) {
        ret = AX_ISP_RegisterSensorExt(nPipe, arrHdlId[i], pSnsObj);
        if (ret) {
            LOG_MM_E(SENSOR, "AX_ISP_RegisterSensorExt(pipe: %d, HdlId: %d), ret = 0x%08X", nPipe, arrHdlId[i], ret);
            break;
        }

        bSnsRegistered = AX_TRUE;

        ret = AX_ISP_SetSnsActive(nPipe, arrHdlId[i], SENSOR_ACTIVE_SENSOR);
        if (ret) {
            LOG_MM_E(SENSOR, "AX_ISP_SetSnsActive(pipe: %d, HdlId: %d), ret = 0x%08X", nPipe, arrHdlId[i], ret);
            break;
        }

        if (ISP_SNS_CONNECT_I2C_TYPE == nBusType) {
            tSnsBusInfo.I2cDev = arrDevNode[i];
        } else {
            tSnsBusInfo.SpiDev.bit4SpiDev = arrDevNode[i];
            tSnsBusInfo.SpiDev.bit4SpiCs = 0;
        }

        if (pSnsObj->pfn_sensor_set_bus_info) {
            ret = pSnsObj->pfn_sensor_set_bus_info(arrHdlId[i], tSnsBusInfo);
            if (0 != ret) {
                LOG_MM_E(SENSOR, "Sensor set bus info fail, ret = 0x%08X.", ret);
                break;
            } else {
                LOG_M_I(SENSOR, "set sensor bus idx %d", tSnsBusInfo.I2cDev);
            }
        } else {
            LOG_MM_E(SENSOR, "sensor set buf info is not supported!");
            ret = -1;
            break;
        }

        if (NULL != pSnsObj->pfn_sensor_set_slaveaddr && (AX_U8)(-1) != arrDevI2cAddr[i]) {
            ret = pSnsObj->pfn_sensor_set_slaveaddr(arrHdlId[i], arrDevI2cAddr[i]);
            if (0 != ret) {
                LOG_MM_E(SENSOR, "set sensor slave addr failed with %#x!", ret);
                break;
            }
            LOG_M_I(SENSOR, "set sensor slave addr %d", arrDevI2cAddr[i]);
        }

        LOG_MM_I(SENSOR, "HdlId: %d, I2cDev: %d, I2cAddr: %d", arrHdlId[i], tSnsBusInfo.I2cDev, arrDevI2cAddr[i]);
    }

    if (0 != ret) {
        if (bSnsRegistered) {
            AX_ISP_UnRegisterSensor(nPipe);
        }
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::UnRegisterSensor(AX_U8 nPipe) {
    AX_S32 ret = AX_ISP_UnRegisterSensor(nPipe);
    if (0 != ret) {
        LOG_M_E(SENSOR, "AX_ISP_UnRegisterSensor(pipe: %d) fail, ret = 0x%08X", nPipe, ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::ResetSensorObj(AX_U8 nPipe, AX_SENSOR_REGISTER_FUNC_T* pSnsObj, AX_U8 nResetGpio) {
    if (pSnsObj && pSnsObj->pfn_sensor_reset) {
        if (0 != pSnsObj->pfn_sensor_reset(nPipe, nResetGpio)) {
            LOG_M_E(SENSOR, "Pipe[%d] sensor reset fail.", nPipe);
            return AX_FALSE;
        }
    } else {
        LOG_M_E(SENSOR, "Pipe[%d] sensor reset is not supported!", nPipe);
        return AX_FALSE;
    }

    LOG_M_C(SENSOR, "Pipe[%d] sensor(%p) reset.", nPipe, pSnsObj);

    return AX_TRUE;
}

AX_VOID CBaseSensor::InitSensor(AX_U8 nPipe) {
    if (m_pSnsObj && m_pSnsObj->pfn_sensor_init) {
        m_pSnsObj->pfn_sensor_init(nPipe);
    }

    m_algWrapper.RegisterAlgoToSensor(m_pSnsObj, nPipe);
}

AX_VOID CBaseSensor::ExitSensor(AX_U8 nPipe) {
    m_algWrapper.UnRegisterAlgoFromSensor(nPipe);

    if (m_pSnsObj && m_pSnsObj->pfn_sensor_exit) {
        m_pSnsObj->pfn_sensor_exit(nPipe);
    }
}

AX_VOID CBaseSensor::RegAttrUpdCallback(SensorAttrUpdCallback callback) {
    if (nullptr == callback) {
        return;
    }

    m_cbAttrUpd = callback;
}

AX_IMG_FORMAT_E CBaseSensor::GetMaxImgFmt() {
    return m_eImgFormatSDR >= m_eImgFormatHDR ? m_eImgFormatSDR : m_eImgFormatHDR;
}

AX_SNS_HDR_MODE_E CBaseSensor::GetMaxHdrMode() {
    if (!GetAbilities().bSupportHDR) {
        return AX_SNS_LINEAR_MODE;
    }

    return AX_SNS_HDR_4X_MODE;
}


AX_BOOL CBaseSensor::ChangeDaynightMode(AX_DAYNIGHT_MODE_E eDaynightMode) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
    for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
        if (!m_bSensorStarted) {
            continue;
        }

        AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;
        for (AX_U8 j = 0; j < AX_VIN_CHN_ID_MAX; j++) {
            if (!tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].bChnEnable) {
                continue;
            }

            AX_S32 nRet = AX_VIN_SetChnDayNightMode(nPipeId, (AX_VIN_CHN_ID_E)j, eDaynightMode);
            if (0 != nRet) {
                LOG_M_E(SENSOR, "[%d][%d] AX_VIN_SetChnDayNightMode failed, ret=0x%x.", nPipeId, j, nRet);
                return AX_FALSE;
            } else {
                LOG_M_W(SENSOR, "[%d][%d] AX_VIN_SetChnDayNightMode OK.", nPipeId, j);
            }
        }
    }

    m_tSnsCfg.eDayNight = eDaynightMode;

    return AX_TRUE;
}

AX_BOOL CBaseSensor::ChangeSnsResolution(AX_U32 nWidth, AX_U32 nHeight) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    m_tSnsCfg.tSnsResolution.nSnsOutWidth = nWidth;
    m_tSnsCfg.tSnsResolution.nSnsOutHeight = nHeight;

    return AX_TRUE;
}

AX_BOOL CBaseSensor::ChangeHdrMode(AX_U32 nSnsMode, AX_U8 nHdrRatio) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    SNS_HDR_TYPE_E ePreHdrType = m_tSnsCfg.eHdrType;

    m_tSnsCfg.eHdrType = GET_SNS_HDR_TYPE(nSnsMode);
    m_tSnsCfg.nSensorMode = nSnsMode;
    m_tSnsCfg.tHdrRatioAttr.nRatio = nHdrRatio;

    if (E_SNS_HDR_TYPE_ClASSICAL == m_tSnsCfg.eHdrType) {
        m_tSnsCfg.eSensorMode = (SNS_MODE_HDR == nSnsMode) ? AX_SNS_HDR_2X_MODE : AX_SNS_LINEAR_MODE;
    } else { // Long-Frame HDR
        if (ePreHdrType == E_SNS_HDR_TYPE_LONGFRAME) { // do switch between Long-Frame hdr normal mode and Long-Frame hdr long frame mode
            AX_S32 nRet = -1;
            for (AX_U8 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
                AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;
                if (!LoadBinParams(nPipeID, m_tSnsCfg.arrPipeAttr[i].vecTuningBin)) {
                    return AX_FALSE;
                }
                AX_ISP_LFHDR_MODE_E eLFHDRMode = (SNS_MODE_LFHDR_HDR == nSnsMode) ? AX_ISP_LFHDR_NORMAL_MODE : AX_ISP_LFHDR_LONG_FRAME_MODE;
                nRet = AX_ISP_IQ_SetAeLongFrameMode(nPipeID, eLFHDRMode);
                if (0 != nRet) {
                    LOG_M_E(SENSOR, "Sns[%d] Pipe[%d] AX_ISP_IQ_SetAeLongFrameMode(%d) faile, ret: 0x%x",
                                    m_tSnsCfg.nSnsID, nPipeID, eLFHDRMode, nRet);
                    return AX_FALSE;
                }
            }
        } else { // do switch from classical hdr to Long-Frame hdr, it will do load bin and change mode in restart process and make sure eSensorMode is AX_SNS_HDR_2X_MODE
            m_tSnsCfg.eSensorMode = AX_SNS_HDR_2X_MODE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::ChangeSnsMirrorFlip(AX_BOOL bMirror, AX_BOOL bFlip) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    LOG_M_W(SENSOR, "SNS[%d] change mirror=%d, flip=%d.", m_tSnsCfg.nSnsID, bMirror, bFlip);
    AX_S32 nRet = 0;
    AX_BOOL bSucces = AX_TRUE;
    for (AX_U8 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        if (!m_bSensorStarted) {
            continue;
        }

        AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;
        AX_BOOL bValue = AX_FALSE;
        if ((m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN)
            && (nPipeID == m_tSwitchSubSnsInfo.nPipeID)) {
            continue;
        }

        nRet = AX_VIN_GetPipeMirror(nPipeID, &bValue);
        if (nRet == 0 && bMirror != bValue) {
            LOG_M_W(SENSOR, "SNS[%d] change pipe[%d] mirror=%d.", m_tSnsCfg.nSnsID, nPipeID, bMirror);
            nRet = AX_VIN_SetPipeMirror(nPipeID, bMirror);
        } else if (nRet != 0) {
            LOG_M_E(SENSOR, "AX_VIN_GetPipeMirror failed, ret=0x%x.", nRet);
            LOG_M_W(SENSOR, "SNS[%d] change pipe[%d] mirror=%d.", m_tSnsCfg.nSnsID, nPipeID, bMirror);
            nRet = AX_VIN_SetPipeMirror(nPipeID, bMirror);
        }
        if (nRet != 0) {
            bSucces = AX_FALSE;
        }

        for (AX_U8 j = AX_VIN_CHN_ID_MAIN; j < AX_VIN_CHN_ID_MAX; j++) {
            if (m_tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].bChnEnable) {
                AX_BOOL bValue = AX_FALSE;
                nRet = AX_VIN_GetChnFlip(nPipeID, (AX_VIN_CHN_ID_E)j, &bValue);
                if (nRet == 0 && bFlip != bValue) {
                    LOG_M_W(SENSOR, "SNS[%d] change pipe[%d] chn[%d] flip=%d.", m_tSnsCfg.nSnsID, nPipeID, j, bFlip);
                    nRet = AX_VIN_SetChnFlip(nPipeID, (AX_VIN_CHN_ID_E)j, bFlip);
                } else if (nRet != 0) {
                    LOG_M_E(SENSOR, "AX_VIN_GetChnFlip failed, ret=0x%x.", nRet);
                    LOG_M_W(SENSOR, "SNS[%d] change pipe[%d] chn[%d] flip=%d.", m_tSnsCfg.nSnsID, nPipeID, j, bFlip);
                    nRet = AX_VIN_SetChnFlip(nPipeID, (AX_VIN_CHN_ID_E)j, bFlip);
                }
                if (nRet != 0) {
                    bSucces = AX_FALSE;
                }
            }
        }
    }

    m_tSnsCfg.bMirror = bMirror;
    m_tSnsCfg.bFlip = bFlip;

    return bSucces;
}

AX_BOOL CBaseSensor::GetIspImageAttr(APP_ISP_IMAGE_ATTR_T &tAttr) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    AX_S32 ret = 0;
    // restore sharpness
    AX_ISP_IQ_SHARPEN_PARAM_T IspShpParam;
    SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
    for (AX_U32 i = 0; i < tSnsCfg.nPipeCount; i++) {
        if (!m_bSensorStarted) {
            continue;
        }

        AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;
        ret = AX_ISP_IQ_GetShpParam(nPipeId, &IspShpParam);

        tAttr.nSharpness =
            CalcIspIQToValue(IspShpParam.tManualParam.nShpGain[0], SENSOR_SHARPNESS_LOW, SENSOR_SHARPNESS_MEDIUM, SENSOR_SHARPNESS_HIGH);

        // restore Ycproc
        AX_ISP_IQ_YCPROC_PARAM_T IspYcprocParam;

        ret = AX_ISP_IQ_GetYcprocParam(nPipeId, &IspYcprocParam);

        if (0 != ret) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_GetYcprocParam failed, ret=0x%x.", ret);
            return AX_FALSE;
        }

        tAttr.nBrightness =
            CalcIspIQToValue(IspYcprocParam.nBrightness, SENSOR_BRIGHTNESS_LOW, SENSOR_BRIGHTNESS_MEDIUM, SENSOR_BRIGHTNESS_HIGH);
        tAttr.nContrast = CalcIspIQToValue(IspYcprocParam.nContrast, SENSOR_CONTRAST_LOW, SENSOR_CONTRAST_MEDIUM, SENSOR_CONTRAST_HIGH);
        tAttr.nSaturation =
            CalcIspIQToValue(IspYcprocParam.nSaturation, SENSOR_SATURATION_LOW, SENSOR_SATURATION_MEDIUM, SENSOR_SATURATION_HIGH);
        tAttr.nAutoMode = IspShpParam.nAutoMode;
        LOG_MM_I(SENSOR, "autoMode:%d, tAttr.nSharpness:%d, tAttr.nBrightness:%d, tAttr.nContrast:%d,tAttr.nSaturation:%d",
                 IspShpParam.nAutoMode, tAttr.nSharpness, tAttr.nBrightness, tAttr.nContrast, tAttr.nSaturation);
        memcpy(&m_tImageAttr, &tAttr, sizeof(APP_ISP_IMAGE_ATTR_T));

        m_tSnsCfg.tColorAttr.bColorManual = (tAttr.nAutoMode == 0) ? AX_TRUE : AX_FALSE;
        m_tSnsCfg.tColorAttr.fBrightness = (AX_F32)tAttr.nBrightness;
        m_tSnsCfg.tColorAttr.fSharpness = (AX_F32)tAttr.nSharpness;
        m_tSnsCfg.tColorAttr.fContrast = (AX_F32)tAttr.nContrast;
        m_tSnsCfg.tColorAttr.fSaturation = (AX_F32)tAttr.nSaturation;
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::SetIspImageAttr(const APP_ISP_IMAGE_ATTR_T &tAttr) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    LOG_MM_C(SENSOR, "+++");
    AX_S32 ret = 0;
    AX_S32 nShpGain = CalcValueToIspIQ(tAttr.nSharpness, SENSOR_SHARPNESS_LOW, SENSOR_SHARPNESS_MEDIUM, SENSOR_SHARPNESS_HIGH);
    AX_S32 nBrightness = CalcValueToIspIQ(tAttr.nBrightness, SENSOR_BRIGHTNESS_LOW, SENSOR_BRIGHTNESS_MEDIUM, SENSOR_BRIGHTNESS_HIGH);
    AX_S32 nContrast = CalcValueToIspIQ(tAttr.nContrast, SENSOR_CONTRAST_LOW, SENSOR_CONTRAST_MEDIUM, SENSOR_CONTRAST_HIGH);
    AX_S32 nSaturation = CalcValueToIspIQ(tAttr.nSaturation, SENSOR_SATURATION_LOW, SENSOR_SATURATION_MEDIUM, SENSOR_SATURATION_HIGH);
    // set sharpness
    AX_ISP_IQ_SHARPEN_PARAM_T IspShpParam;
    SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
    for (AX_U32 i = 0; i < tSnsCfg.nPipeCount; i++) {
        if (!m_bSensorStarted) {
            continue;
        }

        AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;
        ret = AX_ISP_IQ_GetShpParam(nPipeId, &IspShpParam);
        IspShpParam.nAutoMode = tAttr.nAutoMode;

        IspShpParam.tManualParam.nShpGain[0] = nShpGain;
        IspShpParam.tManualParam.nShpGain[1] = nShpGain;

        ret = AX_ISP_IQ_SetShpParam(nPipeId, &IspShpParam);

        if (0 != ret) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_SetShpParam failed, ret=0x%x.", ret);
            return AX_FALSE;
        }

        // set Ycproc
        AX_ISP_IQ_YCPROC_PARAM_T IspYcprocParam;

        ret = AX_ISP_IQ_GetYcprocParam(nPipeId, &IspYcprocParam);

        if (0 != ret) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_GetYcprocParam failed, ret=0x%x.", ret);
            return AX_FALSE;
        }
        LOG_MM_I(SENSOR, "nSharpness:%d, nBrightness:%d, nContrast:%d,nSaturation:%d", tAttr.nSharpness, tAttr.nBrightness, tAttr.nContrast,
                 tAttr.nSaturation);
        IspYcprocParam.nBrightness = nBrightness;
        IspYcprocParam.nContrast = nContrast;
        IspYcprocParam.nSaturation = nSaturation;
        IspYcprocParam.nYCprocEn = AX_TRUE;

        ret = AX_ISP_IQ_SetYcprocParam(nPipeId, &IspYcprocParam);

        if (0 != ret) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_SetYcprocParam failed, ret=0x%x.", ret);

            return AX_FALSE;
        }
    }

    {
        m_tSnsCfg.tColorAttr.bColorManual = (tAttr.nAutoMode == 0) ? AX_TRUE : AX_FALSE;
        m_tSnsCfg.tColorAttr.fBrightness = (AX_F32)tAttr.nBrightness;
        m_tSnsCfg.tColorAttr.fSharpness = (AX_F32)tAttr.nSharpness;
        m_tSnsCfg.tColorAttr.fContrast = (AX_F32)tAttr.nContrast;
        m_tSnsCfg.tColorAttr.fSaturation = (AX_F32)tAttr.nSaturation;
    }

    LOG_MM_C(SENSOR, "---");

    return AX_TRUE;
}

AX_BOOL CBaseSensor::RestoreIspImageAttr(APP_ISP_IMAGE_ATTR_T &tAttr) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    LOG_MM_C(SENSOR, "+++");

    AX_S32 ret = 0;
    // restore sharpness
    AX_ISP_IQ_SHARPEN_PARAM_T IspShpParam;
    SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
    for (AX_U32 i = 0; i < tSnsCfg.nPipeCount; i++) {
        if (!m_bSensorStarted) {
            continue;
        }

        AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;
        ret = AX_ISP_IQ_GetShpParam(nPipeId, &IspShpParam);

        // AX620 TODO: sharpness not support auto mode?
        // IspShpParam.nAutoMode = tAttr.nAutoMode;
        AX_S32 nShpGain = CalcValueToIspIQ(m_tImageAttr.nSharpness, SENSOR_SHARPNESS_LOW, SENSOR_SHARPNESS_MEDIUM, SENSOR_SHARPNESS_HIGH);

        IspShpParam.tManualParam.nShpGain[0] = nShpGain;
        IspShpParam.tManualParam.nShpGain[1] = nShpGain;

        ret = AX_ISP_IQ_SetShpParam(nPipeId, &IspShpParam);

        if (0 != ret) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_SetShpParam failed, ret=0x%x.", ret);
            return AX_FALSE;
        }

        // restore Ycproc
        AX_ISP_IQ_YCPROC_PARAM_T IspYcprocParam;
        ret = AX_ISP_IQ_GetYcprocParam(nPipeId, &IspYcprocParam);
        if (0 != ret) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_GetYcprocParam failed, ret=0x%x.", ret);
            return AX_FALSE;
        }
        IspYcprocParam.nBrightness =
            CalcValueToIspIQ(m_tImageAttr.nBrightness, SENSOR_BRIGHTNESS_LOW, SENSOR_BRIGHTNESS_MEDIUM, SENSOR_BRIGHTNESS_HIGH);
        IspYcprocParam.nContrast =
            CalcValueToIspIQ(m_tImageAttr.nContrast, SENSOR_CONTRAST_LOW, SENSOR_CONTRAST_MEDIUM, SENSOR_CONTRAST_HIGH);
        IspYcprocParam.nSaturation =
            CalcValueToIspIQ(m_tImageAttr.nSaturation, SENSOR_SATURATION_LOW, SENSOR_SATURATION_MEDIUM, SENSOR_SATURATION_HIGH);
        IspYcprocParam.nYCprocEn = AX_TRUE;
        m_tImageAttr.nAutoMode = tAttr.nAutoMode;

        tAttr = m_tImageAttr;

        ret = AX_ISP_IQ_SetYcprocParam(nPipeId, &IspYcprocParam);
        LOG_MM_I(SENSOR, "nPipeId:%d, nSharpness:%d, nBrightness:%d, nContrast:%d, nSaturation:%d", nPipeId, m_tImageAttr.nSharpness,
                 m_tImageAttr.nBrightness, m_tImageAttr.nContrast, m_tImageAttr.nSaturation);
    }

    {
        m_tSnsCfg.tColorAttr.bColorManual = (tAttr.nAutoMode == 0) ? AX_TRUE : AX_FALSE;
        m_tSnsCfg.tColorAttr.fBrightness = tAttr.nBrightness;
        m_tSnsCfg.tColorAttr.fSharpness = tAttr.nSharpness;
        m_tSnsCfg.tColorAttr.fContrast = tAttr.nContrast;
        m_tSnsCfg.tColorAttr.fSaturation = tAttr.nSaturation;
    }

    LOG_MM_C(SENSOR, "---");

    return AX_TRUE;
}

// Linear: ([low, 0], [medium, 50]); ([medium, 50], [max, 100])
AX_S32 CBaseSensor::CalcValueToIspIQ(AX_F32 fVal, AX_S32 nLow, AX_S32 nMedium, AX_S32 nHigh) {
    AX_S32 nIQ = 0;

    if (nIQ < 0.) {
        nIQ = nLow;
    } else if (fVal <= 50.) {
        nIQ = nLow + fVal * (nMedium - nLow) / 50;
    } else if (fVal <= 100.) {
        nIQ = 2 * nMedium - nHigh + fVal * (nHigh - nMedium) / 50;
    } else {
        nIQ = nHigh;
    }

    return nIQ;
}

AX_F32 CBaseSensor::CalcIspIQToValue(AX_S32 nIQ, AX_S32 nLow, AX_S32 nMedium, AX_S32 nHigh) {
    AX_F32 fVal = 0;

    if (nIQ < nLow) {
        fVal = 0.;
    } else if (nIQ <= nMedium) {
        fVal = 50. * (nIQ - nLow) / (nMedium - nLow);
    } else if (nIQ <= nHigh) {
        fVal = 50. * (nIQ + nHigh - 2 * nMedium) / (nHigh - nMedium);
    } else {
        fVal = 100.;
    }

    return fVal;
}

AX_BOOL CBaseSensor::SetSnsAttr(AX_U8 nPipeId, const AX_SNS_ATTR_T &tSnsAttr) {
    AX_SNS_ATTR_T tAttr = tSnsAttr;

    if (tAttr.fFrameRate < m_tAbilities.fShutterSlowFpsThr) {
        tAttr.fFrameRate = m_tAbilities.fShutterSlowFpsThr;
    }

    AX_S32 nRet = AX_ISP_SetSnsAttr(nPipeId, &tAttr);

    if (0 != nRet) {
        LOG_MM_E(SENSOR, "pipe:[%d] AX_ISP_SetSnsAttr failed, ret=0x%x.", nPipeId, nRet);
        return AX_FALSE;
    } else {
        LOG_MM_I(SENSOR, "pipe:[%d] AX_ISP_SetSnsAttr OK.", nPipeId);
    }

    if (m_bSensorStarted) {
        if (tSnsAttr.fFrameRate != tAttr.fFrameRate) {
            SNS_SHUTTER_MODE_E eShutterMode = GetShutterMode();

            if (eShutterMode != E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E) {
                SetShutterMode(E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E);
            }

            if (m_pSnsObj && m_pSnsObj->pfn_sensor_set_slow_fps) {
                GetShutterMode();
                m_pSnsObj->pfn_sensor_set_slow_fps(nPipeId, tSnsAttr.fFrameRate);
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::UpdateSnsAttr() {
    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        if (!SetSnsAttr(nPipeId, m_tSnsAttr)) {
            return AX_FALSE;
        }
    }
    return AX_TRUE;
}

AX_BOOL CBaseSensor::SetHdrRatio(AX_U8 nHdrRatio) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    m_tSnsCfg.tHdrRatioAttr.nRatio = nHdrRatio;

    if (IS_SNS_HDR_MODE(m_tSnsCfg.eSensorMode)
        && m_tSnsCfg.tHdrRatioAttr.bEnable) {
        if (!m_bSensorStarted) {
            return AX_TRUE;
        }

        LOG_MM_C(SENSOR, "Ratio[%d]", nHdrRatio);

        string strBinName;

        if (nHdrRatio == 0) {
            strBinName = m_tSnsCfg.tHdrRatioAttr.strHdrRatioDefaultBin;
        } else {
            strBinName = m_tSnsCfg.tHdrRatioAttr.strHdrRatioModeBin;
        }

        return LoadParams(strBinName);
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::SetAeRoiAttr(const SENSOR_AE_ROI_ATTR_T& tAttr) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
    for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
        if (!m_bSensorStarted) {
            continue;
        }

        AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;

        AX_ISP_IQ_AE_PARAM_T tAeParam;
        memset(&tAeParam, 0x00, sizeof(tAeParam));

        AX_S32 nRet = AX_ISP_IQ_GetAeParam(nPipeId, &tAeParam);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "AX_ISP_IQ_GetAeParam failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }

        tAeParam.tAeAlgAuto.tFaceUIParam.nEnable = tAttr.bFaceAeRoiEnable;

        tAeParam.tAeAlgAuto.tVehicleUIParam.nEnable = tAttr.bVehicleAeRoiEnable;

        nRet = AX_ISP_IQ_SetAeParam(nPipeId, &tAeParam);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "AX_ISP_IQ_SetAeParam failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }
    }

    m_tSnsCfg.tAeRoiAttr = tAttr;

    return AX_TRUE;
}

AX_BOOL CBaseSensor::SetAeRoi(const std::vector<AX_APP_ALGO_AE_ROI_ITEM_T>& stVecItem) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    if (!m_tSnsCfg.tAeRoiAttr.bFaceAeRoiEnable
        && !m_tSnsCfg.tAeRoiAttr.bVehicleAeRoiEnable) {
        return AX_TRUE;
    }

    AX_BOOL bUpdate = AX_FALSE;
    AX_ISP_AE_DETECT_OBJECT_PARAM_T tAeParam;

    if (stVecItem.size() > 0) {
        bUpdate = AX_TRUE;
        memset(&tAeParam, 0x00, sizeof(tAeParam));

        AX_U32 nAeRoiCnt = stVecItem.size();

        auto& nObjectceIndex = tAeParam.nObjectNum;
        for (AX_U32 i = 0; i < nAeRoiCnt && nObjectceIndex < AX_AE_DETECT_OBJECT_MAX_NUM; i ++) {
            tAeParam.nObjectID[nObjectceIndex] = stVecItem[i].u64TrackId;
            tAeParam.nObjectCategory[nObjectceIndex] = (AX_U32)AX_APP_ALGO_GET_AE_ROI_OBJECT_CATEGORY(stVecItem[i].eType);
            tAeParam.nObjectConfidence[nObjectceIndex] = CAXTypeConverter::AeFloat2Int(stVecItem[i].fConfidence, 1, 10, AX_FALSE);
            tAeParam.tObjectInfos[nObjectceIndex].nObjectStartX = CAXTypeConverter::AeFloat2Int(stVecItem[i].tBox.fX, 1, 10, AX_FALSE);
            tAeParam.tObjectInfos[nObjectceIndex].nObjectStartY = CAXTypeConverter::AeFloat2Int(stVecItem[i].tBox.fY, 1, 10, AX_FALSE);
            tAeParam.tObjectInfos[nObjectceIndex].nObjectWidth = CAXTypeConverter::AeFloat2Int(stVecItem[i].tBox.fW, 1, 10, AX_FALSE);
            tAeParam.tObjectInfos[nObjectceIndex].nObjectHeight = CAXTypeConverter::AeFloat2Int(stVecItem[i].tBox.fH, 1, 10, AX_FALSE);
            nObjectceIndex ++;
        }

        m_bAeRoiManual = AX_TRUE;
    } else {
        if (m_bAeRoiManual) {
            bUpdate = AX_TRUE;
            m_bAeRoiManual = AX_FALSE;
            memset(&tAeParam, 0x00, sizeof(tAeParam));
        }
    }

    if (bUpdate) {
        SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
        for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
            if (!m_bSensorStarted) {
                continue;
            }

            AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;

            AX_S32 nRet = AX_ISP_IQ_SetAeDetectObjectROI(nPipeId, &tAeParam);
            if (0 != nRet) {
                LOG_M_E(SENSOR, "AX_ISP_IQ_SetAeDetectObjectROI failed, ret=0x%08X.", nRet);
                return AX_FALSE;
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::UpdateLDC(AX_BOOL bLdcEnable, AX_BOOL bAspect, AX_S16 nXRatio, AX_S16 nYRatio, AX_S16 nXYRatio, AX_S16 nDistorRatio) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    LOG_MM_I(SENSOR, "UpdateLDC+++");
    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;
        AX_ISP_IQ_LDC_PARAM_T tLdcAttr;
        AX_ISP_IQ_GetLdcParam(nPipeId, &tLdcAttr);
        tLdcAttr = {.nLdcEnable = bLdcEnable,
                    .nType = AX_ISP_IQ_LDC_TYPE_V1,
                    .tLdcV1Param =
                        {
                            .bAspect = bAspect,
                            .nXRatio = nXRatio,
                            .nYRatio = nYRatio,
                            .nXYRatio = nXYRatio,
                            .nCenterXOffset = 0,
                            .nCenterYOffset = 0,
                            .nDistortionRatio = nDistorRatio,
                            .nSpreadCoef = 0,
                        },
                    .tLdcV2Param = {.nMatrix =
                                        {
                                            {0, 0, 0, /*0 - 2*/},
                                            {0, 0, 0, /*0 - 2*/},
                                            {0, 0, 1, /*0 - 2*/},
                                        },
                                    .nDistortionCoeff = {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/}}};

        if (m_bSensorStarted) {
            AX_S32 nRet = AX_ISP_IQ_SetLdcParam(nPipeId, &tLdcAttr);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(SENSOR, "AX_ISP_IQ_SetLdcParam failed ret=0x%x", nRet);
                return AX_FALSE;
            }
        }

        m_tSnsCfg.arrPipeAttr[i].tLdcAttr.bLdcEnable = bLdcEnable;
        m_tSnsCfg.arrPipeAttr[i].tLdcAttr.bLdcAspect = bAspect;
        m_tSnsCfg.arrPipeAttr[i].tLdcAttr.nLdcXRatio = nXRatio;
        m_tSnsCfg.arrPipeAttr[i].tLdcAttr.nLdcYRatio = nYRatio;
        m_tSnsCfg.arrPipeAttr[i].tLdcAttr.nLdcXYRatio = nXYRatio;
        m_tSnsCfg.arrPipeAttr[i].tLdcAttr.nLdcDistortionRatio = nDistorRatio;
    }

    LOG_MM_I(SENSOR, "UpdateLDC---");
    return AX_TRUE;
}

AX_BOOL CBaseSensor::UpdateDIS(AX_BOOL bDisEnable) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);
    LOG_MM_I(SENSOR, "UpdateDIS+++");
    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;
        AX_ISP_IQ_DIS_PARAM_T tDisAttr;
        AX_ISP_IQ_GetDisParam(nPipeId, &tDisAttr);

        if (m_bSensorStarted) {
            tDisAttr.bDisEnable = bDisEnable;
            if (tDisAttr.bDisEnable &&
                m_tSnsCfg.arrPipeAttr[0].tDisAttr.nDelayFrameNum > 0) {
                tDisAttr.tDisV1Param.nDelayFrameNum = m_tSnsCfg.arrPipeAttr[0].tDisAttr.nDelayFrameNum;
            }
            AX_S32 nRet = AX_ISP_IQ_SetDisParam(nPipeId, &tDisAttr);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(SENSOR, "AX_ISP_IQ_SetDisParam failed ret=0x%x", nRet);
                return AX_FALSE;
            }
        }

        m_tSnsCfg.arrPipeAttr[i].tDisAttr.bDisEnable = bDisEnable;
    }

    LOG_MM_I(SENSOR, "UpdateDIS---");

    return AX_TRUE;
}

AX_BOOL CBaseSensor::UpdateRotation(AX_VIN_ROTATION_E eRotation) {
    LOG_MM_I(SENSOR, "UpdateRotation+++");
    AX_S32 nRet = 0;
    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;
        AX_VIN_ROTATION_E eRotionOld = AX_VIN_ROTATION_0;

        nRet = AX_VIN_GetChnRotation(nPipeId, AX_VIN_CHN_ID_MAIN, &eRotionOld);
        if (AX_SUCCESS != nRet) {
            LOG_MM_E(SENSOR, "AX_VIN_GetChnRotation failed ret=0x%x", nRet);
        }

        if (eRotation != eRotionOld) {
            nRet = AX_VIN_SetChnRotation(nPipeId, AX_VIN_CHN_ID_MAIN, eRotation);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(SENSOR, "AX_VIN_SetChnRotation failed ret=0x%x", nRet);
                return AX_FALSE;
            }
        } else {
            LOG_MM_I(SENSOR, "Ratation %d not changed", eRotation);
        }
    }

    m_tSnsCfg.eRotation = (AX_ROTATION_E)eRotation;

    if (m_tSnsCfg.tEZoomAttr.fEZoomRatio) {
        EZoom(m_tSnsCfg.tEZoomAttr.fEZoomRatio);
    }

    LOG_MM_I(SENSOR, "UpdateRotation---");
    return AX_TRUE;
}

AX_BOOL CBaseSensor::EnableChn(AX_BOOL bEnable)  {
    LOG_MM_I(SENSOR, "EnableChn +++, enable=%d", bEnable);
    AX_S32 nRet = 0;
    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        if (bEnable) {
            nRet = AX_VIN_EnableChn(nPipeId, AX_VIN_CHN_ID_MAIN);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(SENSOR, "AX_VIN_EnableChn failed ret=0x%x", nRet);
                return AX_FALSE;
            }
        } else {
            nRet = AX_VIN_DisableChn(nPipeId, AX_VIN_CHN_ID_MAIN);
            if (AX_SUCCESS != nRet) {
                LOG_MM_E(SENSOR, "AX_VIN_DisableChn failed ret=0x%x", nRet);
                return AX_FALSE;
            }
        }
    }

    LOG_MM_I(SENSOR, "EnableChn ---, enable=%d", bEnable);
    return AX_TRUE;
}

AX_BOOL CBaseSensor::UpdateSnsFps(AX_S32 nFps) {
    if (0 >= nFps || 30 < nFps) {
        return AX_TRUE;
    }

    SNS_SHUTTER_MODE_E eShutterMode = GetShutterMode();

    if (eShutterMode != E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E) {
        SetShutterMode(E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E);
    }

    if (m_pSnsObj && m_pSnsObj->pfn_sensor_set_slow_fps) {
        for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
            AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;
            m_pSnsObj->pfn_sensor_set_slow_fps(nPipeID, nFps);
        }
    }
    return AX_TRUE;
}

AX_BOOL CBaseSensor::GetAeStatus(AX_ISP_IQ_AE_STATUS_T& tAeStatus) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    if (!m_bSensorStarted) {
        return AX_FALSE;
    }

    SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
    for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;

        AX_S32 nRet = AX_ISP_IQ_GetAeStatus(nPipeId, &tAeStatus);

        if (0 != nRet) {
            LOG_M_E(SENSOR, "AX_ISP_IQ_GetAeStatus failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }

        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_BOOL CBaseSensor::GetDayNightStatus(AX_DAYNIGHT_MODE_E& eDayNightStatus) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    if (!m_bSensorStarted) {
        return AX_FALSE;
    }

    SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
    for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;

        for (AX_U8 j = AX_VIN_CHN_ID_MAIN; j < AX_VIN_CHN_ID_MAX; j++) {
            if (tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].bChnEnable) {
                AX_S32 nRet = AX_VIN_GetChnDayNightMode(nPipeId, (AX_VIN_CHN_ID_E)j, &eDayNightStatus);

                if (0 != nRet) {
                    LOG_M_E(SENSOR, "AX_VIN_GetChnDayNightMode failed, ret=0x%x.", nRet);
                    return AX_FALSE;
                }

                return AX_TRUE;
            }
        }
    }

    return AX_FALSE;
}

AX_BOOL CBaseSensor::SetIrAttr(const SENSOR_IRCUT_ATTR_T& tIrAttr) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    SENSOR_CONFIG_T tSnsCfg = GetSnsConfig();
    for (AX_U8 i = 0; i < tSnsCfg.nPipeCount; i++) {
        if (!m_bSensorStarted) {
            continue;
        }

        AX_DAYNIGHT_MODE_E eDayNightStatus = AX_DAYNIGHT_MODE_DAY;
        AX_ISP_IQ_IR_PARAM_T tIrParam;
        memset(&tIrParam, 0x00, sizeof(tIrParam));

        AX_U8 nPipeId = tSnsCfg.arrPipeAttr[i].nPipeID;

        for (AX_U8 j = AX_VIN_CHN_ID_MAIN; j < AX_VIN_CHN_ID_MAX; j++) {
            if (tSnsCfg.arrPipeAttr[i].arrChannelAttr[j].bChnEnable) {
                AX_S32 nRet = AX_VIN_GetChnDayNightMode(nPipeId, (AX_VIN_CHN_ID_E)j, &eDayNightStatus);

                if (0 != nRet) {
                    LOG_M_E(SENSOR, "AX_VIN_GetChnDayNightMode failed, ret=0x%x.", nRet);
                    return AX_FALSE;
                }

                return AX_TRUE;
            }
        }

        tIrParam.nIrCalibR                = CAXTypeConverter::AeFloat2Int(tIrAttr.fIrCalibR, 8, 10, AX_FALSE);
        tIrParam.nIrCalibG                = CAXTypeConverter::AeFloat2Int(tIrAttr.fIrCalibG, 8, 10, AX_FALSE);
        tIrParam.nIrCalibB                = CAXTypeConverter::AeFloat2Int(tIrAttr.fIrCalibB, 8, 10, AX_FALSE);
        tIrParam.nNight2DayIrStrengthTh   = CAXTypeConverter::AeFloat2Int(tIrAttr.fNight2DayIrStrengthTh, 8, 10, AX_FALSE);
        tIrParam.nNight2DayIrDetectTh     = CAXTypeConverter::AeFloat2Int(tIrAttr.fNight2DayIrDetectTh, 8, 10, AX_FALSE);
        tIrParam.nDay2NightLuxTh          = CAXTypeConverter::AeFloat2Int(tIrAttr.fDay2NightLuxTh, 22, 10, AX_FALSE);
        tIrParam.nNight2DayLuxTh          = CAXTypeConverter::AeFloat2Int(tIrAttr.fNight2DayLuxTh, 22, 10, AX_FALSE);
        tIrParam.nNight2DayBrightTh       = CAXTypeConverter::AeFloat2Int(tIrAttr.fNight2DayBrightTh, 8, 10, AX_FALSE);
        tIrParam.nNight2DayDarkTh         = CAXTypeConverter::AeFloat2Int(tIrAttr.fNight2DayDarkTh, 8, 10, AX_FALSE);
        tIrParam.nNight2DayUsefullWpRatio = CAXTypeConverter::AeFloat2Int(tIrAttr.fNight2DayUsefullWpRatio, 8, 10, AX_FALSE);
        tIrParam.nCacheTime               = tIrAttr.nCacheTime;
        tIrParam.nInitDayNightMode        = eDayNightStatus;

        AX_S32 nRet = AX_ISP_IQ_SetIrParam(nPipeId, &tIrParam);

        if (0 != nRet) {
            LOG_M_E(SENSOR, "AX_ISP_IQ_SetIrParam failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }

        return AX_TRUE;
    }

    return AX_FALSE;
}

AX_VOID CBaseSensor::UpdateSps(const SENSOR_SOFT_PHOTOSENSITIVITY_ATTR_T& tAttr) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    m_tSnsCfg.tSoftPhotoSensitivityAttr = tAttr;

    if (m_pSps) {
        m_pSps->Update(tAttr);
    }
}

AX_VOID CBaseSensor::UpdateHnb(const SENSOR_HOTNOISEBALANCE_T& tAttr) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    m_tSnsCfg.tHotNoiseBalanceAttr = tAttr;

    if (m_pHnb) {
        m_pHnb->Update(tAttr);
    }
}

AX_VOID CBaseSensor::UpdateAeRoi(const SENSOR_AE_ROI_ATTR_T& tAttr) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    m_tSnsCfg.tAeRoiAttr = tAttr;

    if (m_pAeRoi) {
        m_pAeRoi->Update(tAttr);
    }
}

AX_VOID CBaseSensor::UpdateAeRoi(const std::vector<AX_APP_ALGO_AE_ROI_ITEM_T>& stVecItem) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    if (m_pAeRoi) {
        m_pAeRoi->UpdateAeRoi(stVecItem);
    }
}

AX_BOOL CBaseSensor::GetEZoomCropRect(AX_U32 nWidth, AX_U32 nHeight, AX_S16 nCenterOffsetX, AX_S16 nCenterOffsetY, AX_F32 fEZoomRatio, AX_F32 fEZoomMaxRatio, AX_WIN_AREA_T& tCropRect) {
    if ((0 == nWidth) || (0 == nHeight) || (fEZoomRatio <= 1) || (fEZoomMaxRatio <= 1)) {
        LOG_MM_E(SENSOR, "Invalid params(nWidth: %d, nHeight: %d, fEZoomRatio: %f, fEZoomMaxRatio: %f)", nWidth, nHeight, fEZoomRatio, fEZoomMaxRatio);
        return AX_FALSE;
    }

    // Cal Scaled width
    tCropRect.nWidth  = static_cast<AX_U32>(nWidth  * 1.0f / fEZoomRatio);
    tCropRect.nHeight = static_cast<AX_U32>(nHeight * 1.0f / fEZoomRatio);

    // Cal x y
    tCropRect.nStartX = AX_MAX(static_cast<AX_S32>((nWidth / 2) - (tCropRect.nWidth / 2) + nCenterOffsetX * fEZoomRatio / fEZoomMaxRatio), 0);
    tCropRect.nStartY = AX_MAX(static_cast<AX_S32>((nHeight / 2) - (tCropRect.nHeight / 2) + nCenterOffsetY * fEZoomRatio / fEZoomMaxRatio), 0);

    if (tCropRect.nStartX + tCropRect.nWidth > nWidth) {
        tCropRect.nWidth = nWidth - tCropRect.nStartX;
    }

    if (tCropRect.nStartY + tCropRect.nHeight > nHeight) {
        tCropRect.nHeight = nHeight - tCropRect.nStartY;
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::EZoom(AX_F32 fEZoomRatio) {
    LOG_MM_I(SENSOR, "EZoom +++, fEZoomRatio: %f, fEZoomMaxRatio: %f",
                      fEZoomRatio, m_tSnsCfg.tEZoomAttr.fEZoomMaxRatio);

    if (fEZoomRatio < 1.0 || fEZoomRatio > m_tSnsCfg.tEZoomAttr.fEZoomMaxRatio) {
        LOG_MM_E(SENSOR, "Invalid fEZoomRatio(%f)", fEZoomRatio);
        return AX_FALSE;
    }

    AX_F32 fRealEZoomRatio = fEZoomRatio;
    AX_F32 fRealEZoomMaxRatio = m_tSnsCfg.tEZoomAttr.fEZoomMaxRatio;
    AX_U8 nSnsId = 0;

    AX_S16 nCenterOffsetX = m_tSnsCfg.tEZoomAttr.nCenterOffsetX;
    AX_S16 nCenterOffsetY = m_tSnsCfg.tEZoomAttr.nCenterOffsetY;
#ifdef SWITCH_SENSOR_SUPPORT
    if ((m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE)) {
        if (fEZoomRatio >= m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio) {
            nSnsId = m_tSnsCfg.tSinglePipeSwitchInfo.nHdlId1;

            nCenterOffsetX = m_tSnsCfg.tSinglePipeSwitchInfo.nCenterOffsetX;
            nCenterOffsetY = m_tSnsCfg.tSinglePipeSwitchInfo.nCenterOffsetX;

            fRealEZoomRatio = fEZoomRatio - m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio + 1.0;

            if (fRealEZoomRatio == 1.0) {
                memset(m_tPreCropRect, 0, sizeof(m_tPreCropRect));
            }

            fRealEZoomMaxRatio = m_tSnsCfg.tEZoomAttr.fEZoomMaxRatio - m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio + 1.0;
        } else {
            nSnsId = m_tSnsCfg.tSinglePipeSwitchInfo.nHdlId0;
            fRealEZoomMaxRatio = m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio;
        }
    }
#endif

    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        AX_VIN_CROP_INFO_T tCropInfo;
        AX_U32 nWidth = m_tSnsCfg.tSnsResolution.nSnsOutWidth;
        AX_U32 nHeight = m_tSnsCfg.tSnsResolution.nSnsOutHeight;
        if (!GetChnCrop(nPipeId, nSnsId, &tCropInfo)) {
            LOG_MM_E(SENSOR, "GetChnCrop failed!");
            return AX_FALSE;
        }

        AX_VIN_ROTATION_E eRotation = AX_VIN_ROTATION_0;
        AX_VIN_GetChnRotation(nPipeId, AX_VIN_CHN_ID_MAIN, &eRotation);
        switch (eRotation) {
            case AX_VIN_ROTATION_90:
                std::swap(nWidth, nHeight);
                std::swap(nCenterOffsetX, nCenterOffsetY);
                nCenterOffsetX = -nCenterOffsetX;
                break;
            case AX_VIN_ROTATION_180:
                nCenterOffsetX = -nCenterOffsetX;
                nCenterOffsetY = -nCenterOffsetY;
                break;
            case AX_VIN_ROTATION_270:
                std::swap(nWidth, nHeight);
                std::swap(nCenterOffsetX, nCenterOffsetY);
                nCenterOffsetY = -nCenterOffsetY;
                break;
            default:
                break;
        }

        if (fRealEZoomRatio <= 1.0) {
            tCropInfo.bEnable = AX_FALSE;
            tCropInfo.tCropRect.nStartX = 0;
            tCropInfo.tCropRect.nStartY = 0;
            tCropInfo.tCropRect.nWidth = nWidth;
            tCropInfo.tCropRect.nHeight = nHeight;
            if (!SetChnCrop(nPipeId, nSnsId, &tCropInfo)) {
                LOG_MM_E(SENSOR, "SetChnCrop failed!");
                return AX_FALSE;
            }
        } else {
            LOG_MM_I(SENSOR, "EZoomRatio=%f, fRealEZoomRatio=%f, CenterOffset(%d, %d)",
                             fEZoomRatio, fRealEZoomRatio, nCenterOffsetX, nCenterOffsetY);
            tCropInfo.bEnable = AX_TRUE;

            GetDstCenterOffset(m_tSnsCfg.arrPipeAttr[i].nIvpsGrp, nWidth, nHeight, nCenterOffsetX, nCenterOffsetY);

            if (!GetEZoomCropRect(nWidth, nHeight, nCenterOffsetX, nCenterOffsetY, fRealEZoomRatio, fRealEZoomMaxRatio, tCropInfo.tCropRect)) {
                LOG_MM_E(SENSOR, "Get ezoom crop rect failed for ezoom ratio: %f", fEZoomRatio);
                return AX_FALSE;
            }

            LOG_MM_I(SENSOR, "crop info enable=%d, coord_mode=%d, rect=[%d, %d, %d, %d]", tCropInfo.bEnable, tCropInfo.eCoordMode,
              tCropInfo.tCropRect.nStartX, tCropInfo.tCropRect.nStartY, tCropInfo.tCropRect.nWidth, tCropInfo.tCropRect.nHeight);

            if ((AX_MAX(m_tPreCropRect[i].nStartX, tCropInfo.tCropRect.nStartX) - AX_MIN(m_tPreCropRect[i].nStartX, tCropInfo.tCropRect.nStartX)) <= 1
                || (AX_MAX(m_tPreCropRect[i].nStartY, tCropInfo.tCropRect.nStartY) - AX_MIN(m_tPreCropRect[i].nStartY, tCropInfo.tCropRect.nStartY)) <= 0) {
                LOG_MM_I(SENSOR, "Skip zoom ratio: %f", fEZoomRatio);
                continue;
            }

            tCropInfo.eCoordMode = AX_VIN_COORD_ABS;
            if (!SetChnCrop(nPipeId, nSnsId, &tCropInfo)) {
                LOG_MM_E(SENSOR, "SetChnCrop failed!");
                return AX_FALSE;
            }

            m_tPreCropRect[i] = tCropInfo.tCropRect;
        }

#ifdef SWITCH_SENSOR_SUPPORT
        if ((m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE)) {
            SetEZoomCropRect(nSnsId, m_tSnsCfg.tSnsResolution.nSnsOutWidth, m_tSnsCfg.tSnsResolution.nSnsOutHeight, &tCropInfo.tCropRect, eRotation);

            AX_S32 nRet = -1;
            if (nSnsId != m_tSnsCfg.tSinglePipeSwitchInfo.nCurrentHdlId) {
                nRet = ax_mipi_switch_change(nSnsId);
                if (0 != nRet) {
                    LOG_M_E(SENSOR, "ax_mipi_switch_change to hdl(%d) failed with ret: 0x%x", nSnsId, nRet);
                    return AX_FALSE;
                }
                m_tSnsCfg.tSinglePipeSwitchInfo.nCurrentHdlId = nSnsId;
            }
        }
#endif
        m_tSnsCfg.tEZoomAttr.fEZoomRatio = fEZoomRatio;
    }

    LOG_MM_I(SENSOR, "EZoom ---, fZoom=%f", fEZoomRatio);
    return AX_TRUE;
}

AX_BOOL CBaseSensor::InitEZoom() {
    LOG_MM_C(SENSOR, "+++");

    if (m_tSnsCfg.tEZoomAttr.fEZoomMaxRatio <= 0) {
        LOG_MM_E(SENSOR, "Invalid fEZoomMaxRatio=%f", m_tSnsCfg.tEZoomAttr.fEZoomMaxRatio);
        return AX_FALSE;
    }

#ifdef SWITCH_SENSOR_SUPPORT
    if (m_tSnsCfg.eSwitchSnsType != E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE) {
        return AX_TRUE;
    }

    if (m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio > 1
        && m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio < m_tSnsCfg.tEZoomAttr.fEZoomMaxRatio) {
        return AX_TRUE;
    }

    AX_U32 nSwitchWidth = m_tSnsCfg.tSinglePipeSwitchInfo.tSwitchZoomAreaThreshold.nWidth;
    if (nSwitchWidth == 0) {
        return AX_FALSE;
    }

    AX_U32 nWidth = m_tSnsCfg.tSnsResolution.nSnsOutWidth;
    m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio = nWidth * 1.0f / nSwitchWidth;
    LOG_MM_C(SENSOR, "fSwitchEZoomRatio: %.2f", m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio);

    if (m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio <= 1) {
        LOG_MM_E(SENSOR, "Get fSwitchEZoomRatio(%f) failed for SwitchZoomAreaThreshold(%d, %d, %d, %d)",
                          m_tSnsCfg.tSinglePipeSwitchInfo.fSwitchEZoomRatio,
                          m_tSnsCfg.tSinglePipeSwitchInfo.tSwitchZoomAreaThreshold.nStartX,
                          m_tSnsCfg.tSinglePipeSwitchInfo.tSwitchZoomAreaThreshold.nStartY,
                          m_tSnsCfg.tSinglePipeSwitchInfo.tSwitchZoomAreaThreshold.nWidth,
                          m_tSnsCfg.tSinglePipeSwitchInfo.tSwitchZoomAreaThreshold.nHeight);
        return AX_FALSE;
    }
#endif

    LOG_MM_C(SENSOR, "---");
    return AX_TRUE;
}

AX_BOOL CBaseSensor::UpdateSceneMode() {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    if (!m_bSensorStarted) {
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        if (E_SNS_TISP_MODE_E == m_tSnsCfg.arrPipeAttr[i].eAiIspMode
            || E_SNS_AIISP_DEFAULT_SCENE_MODE_E == m_tSnsCfg.arrPipeAttr[i].eAiIspMode) {
            continue;
        }

        AX_ISP_IQ_SCENE_PARAM_T tIspSceneParam;
        memset(&tIspSceneParam, 0x00, sizeof(tIspSceneParam));

        AX_S32 nRet = AX_ISP_IQ_GetSceneParam(nPipeId, &tIspSceneParam);

        if (0 != nRet) {
            LOG_MM_E(SENSOR, "pipe:[%d] AX_ISP_IQ_GetSceneParam failed, ret=0x%x.", nPipeId, nRet);
            return AX_FALSE;
        }

        switch (m_tSnsCfg.arrPipeAttr[i].eAiIspMode) {
            case E_SNS_AIISP_MANUAL_SCENE_AIISP_MODE_E:
                // manual mode
                tIspSceneParam.nAutoMode = 0;
                tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_ENABLE;
                break;
            case E_SNS_AIISP_MANUAL_SCENE_TISP_MODE_E:
                // manual mode
                tIspSceneParam.nAutoMode = 0;
                tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_DISABLE;
                break;
            case E_SNS_AIISP_AUTO_SCENE_MODE_E:
                // auto mode
                tIspSceneParam.nAutoMode = 1;
                // AX620E TODO for auto mode param settings
                break;
            default:
                break;
        }

        nRet = AX_ISP_IQ_SetSceneParam(nPipeId, &tIspSceneParam);

        if (0 != nRet) {
            LOG_MM_E(SENSOR, "pipe:[%d] AX_ISP_IQ_SetSceneParam failed, ret=0x%x.", nPipeId, nRet);
            return AX_FALSE;
        } else {
            LOG_MM_I(SENSOR, "pipe:[%d] AX_ISP_IQ_SetSceneParam OK.", nPipeId);
        }
    }

    return AX_TRUE;
}

SNS_SHUTTER_MODE_E CBaseSensor::GetShutterMode(AX_VOID) {
    SNS_SHUTTER_MODE_E eShutterMode = E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E;

    if (!m_bSensorStarted) {
        return eShutterMode;
    }

    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        AX_ISP_IQ_AE_PARAM_T tIspAeParam;
        memset(&tIspAeParam, 0x00, sizeof(tIspAeParam));

        AX_S32 nRet = AX_ISP_IQ_GetAeParam(nPipeId, &tIspAeParam);

        if (0 != nRet) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_GetAeParam failed, ret=0x%x.", nRet);
            return eShutterMode;
        }

        /* 0: FIX FRAME RATE MODE; 1: SLOW SHUTTER MODE */
        switch(tIspAeParam.tAeAlgAuto.tSlowShutterParam.nFrameRateMode) {
        case 0:
            eShutterMode = E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E;
            break;
        case 1:
            eShutterMode = E_SNS_SHUTTER_SLOW_SHUTTER_MODE_E;
            break;
        default:
            break;
        }

        return eShutterMode;
    }

    return eShutterMode;
}

AX_BOOL CBaseSensor::SetShutterMode(SNS_SHUTTER_MODE_E eShutterMode) {
    if (!m_bSensorStarted) {
        return AX_FALSE;
    }

    LOG_MM_C(SENSOR, "set shutter mode: %d", eShutterMode);

    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        AX_ISP_IQ_AE_PARAM_T tIspAeParam;
        memset(&tIspAeParam, 0x00, sizeof(tIspAeParam));

        AX_S32 nRet = AX_ISP_IQ_GetAeParam(nPipeId, &tIspAeParam);

        if (0 != nRet) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_GetAeParam failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }

        /* 0: FIX FRAME RATE MODE; 1: SLOW SHUTTER MODE */
        AX_U8 nFrameRateMode = 0;

        switch(eShutterMode) {
        case E_SNS_SHUTTER_FIX_FRAMERATE_MODE_E:
            nFrameRateMode = 0;
            break;
        case E_SNS_SHUTTER_SLOW_SHUTTER_MODE_E:
            nFrameRateMode = 1;
            break;
        default:
            break;
        }

        /* 0: FIX FRAME RATE MODE; 1: SLOW SHUTTER MODE */
        tIspAeParam.tAeAlgAuto.tSlowShutterParam.nFrameRateMode = nFrameRateMode;

        nRet = AX_ISP_IQ_SetAeParam(nPipeId, &tIspAeParam);

        if (0 != nRet) {
            LOG_MM_E(SENSOR, "AX_ISP_IQ_SetAeParam failed, ret=0x%x.", nRet);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::LoadBinParams(const std::string &strBinName) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    return LoadParams(strBinName);
}

AX_BOOL CBaseSensor::LoadParams(const std::string &strBinName) {
    if (access(strBinName.c_str(), F_OK) != 0) {
        LOG_MM_C(SENSOR, "(%s) not exist.", strBinName.c_str());
    } else {
        for (AX_U8 i = 0; i < GetPipeCount(); i++) {
            AX_U8 nPipeID = m_tSnsCfg.arrPipeAttr[i].nPipeID;

            AX_S32 nRet = AX_ISP_LoadBinParams(nPipeID, strBinName.c_str());

            if (0 != nRet) {
                LOG_M_E(SENSOR, "(%s) loaded failed, ret=0x%x.", strBinName.c_str(), nRet);

                return AX_FALSE;
            } else {
                LOG_MM_C(SENSOR, "(%s) loaded.", strBinName.c_str());
            }
        }
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::GetSnsTemperature(AX_F32 &fTemperature) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    if (!m_bSensorStarted) {
        return AX_FALSE;
    }

    for (AX_U32 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        AX_U8 nPipeId = m_tSnsCfg.arrPipeAttr[i].nPipeID;

        if (m_pSnsObj && m_pSnsObj->pfn_sensor_get_temperature_info) {
            AX_S32 nTemperature = 0;

            AX_S32 nRet = m_pSnsObj->pfn_sensor_get_temperature_info(nPipeId, &nTemperature);

            if (0 == nRet) {
                fTemperature = (AX_F32)nTemperature/1000.0;

                return AX_TRUE;
            }
        }
    }

    return AX_FALSE;
}

AX_VOID CBaseSensor::GetResolution(AX_U32 &nWidth, AX_U32 &nHeight) {
    std::lock_guard<std::mutex> _ApiLck(m_mtx);

    nWidth = m_tSnsCfg.tSnsResolution.nSnsOutWidth;
    nHeight = m_tSnsCfg.tSnsResolution.nSnsOutHeight;
}

AX_BOOL CBaseSensor::SetMultiSnsSync(AX_BOOL bSync)
{
    AX_S32 nRet = 0;

    for (auto &m : m_mapPipe2Attr) {
        AX_U8 nPipeID = m.first;

        AX_ISP_IQ_AWB_PARAM_T stIspAwbParam = {0};
        nRet = AX_ISP_IQ_GetAwbParam(nPipeID, &stIspAwbParam);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d]AX_ISP_IQ_GetAwbParam failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        }
        stIspAwbParam.nEnable = 1;
        stIspAwbParam.tAutoParam.nMultiCamSyncMode = bSync ? 1 : 0; /* 0: INDEPEND MODE; 1: MASTER SLAVE MODE; 2: OVERLAP MODE */
        nRet = AX_ISP_IQ_SetAwbParam(nPipeID, &stIspAwbParam);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d]AX_ISP_IQ_SetAwbParam failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        }

        AX_ISP_IQ_AE_PARAM_T stIspAeParam = {0};
        nRet = AX_ISP_IQ_GetAeParam(nPipeID, &stIspAeParam);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d]AX_ISP_IQ_GetAeParam failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        }
        stIspAeParam.nEnable = 1;
        stIspAeParam.tAeAlgAuto.nMultiCamSyncMode = bSync ? 3 : 0; /* 0: INDEPEND MODE; 1: MASTER SLAVE MODE; 2: OVERLAP MODE; 3: SPLICE MODE */
        nRet = AX_ISP_IQ_SetAeParam(nPipeID, &stIspAeParam);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "[%d]AX_ISP_IQ_SetAeParam failed, ret=0x%x.", nPipeID, nRet);
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::IsSnsSync() {
    if ((m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_BUTT)
        && (AX_SNS_SYNC_MASTER == (AX_SNS_MASTER_SLAVE_E)m_tSnsCfg.nMasterSlaveSel
            || AX_SNS_SYNC_SLAVE == (AX_SNS_MASTER_SLAVE_E)m_tSnsCfg.nMasterSlaveSel)) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

std::string CBaseSensor::GetTuningBinKey() {
    std::string strKey = "";

    switch (m_tSnsCfg.nSensorMode) {
        case SNS_MODE_SDR:
            strKey = SDRKEY;
            break;
        case SNS_MODE_HDR:
            strKey = HDRKEY;
            break;
        case SNS_MODE_LFHDR_SDR:
            strKey = LFSDRKEY;
            break;
        case SNS_MODE_LFHDR_HDR:
            strKey = LFHDRKEY;
            break;
        default:
            strKey = SDRKEY;
            LOG_M_W(SENSOR, "nSensorMode(%d) is invalid, use classical sdr key(%s) by default.", m_tSnsCfg.nSensorMode, strKey.c_str());
            break;
    }

    return strKey;
}

AX_BOOL CBaseSensor::LoadBinParams(AX_U8 nPipeId, const vector<string>& vecPipeTuningBin) {
    AX_S32 nRet = 0;

    vector<string> vecBinModeMatched;
    std::string strKey = GetTuningBinKey();

    for (AX_U8 j = 0; j < vecPipeTuningBin.size(); j++) {
        if (!vecPipeTuningBin[j].empty()) {
            if (std::string::npos == vecPipeTuningBin[j].find(strKey)) {
                continue;
            }

            vecBinModeMatched.push_back(vecPipeTuningBin[j]);
        }
    }

    vector<string> vecBinResolutionMatched;
    strKey = std::to_string(m_tSnsCfg.tSnsResolution.nSnsOutWidth) + "x" + std::to_string(m_tSnsCfg.tSnsResolution.nSnsOutHeight);

    for (AX_U8 j = 0; j < vecBinModeMatched.size(); j++) {
        if (std::string::npos == vecBinModeMatched[j].find(strKey)) {
            continue;
        }

        vecBinResolutionMatched.push_back(vecBinModeMatched[j]);
    }

    vector<string> vecBinMatched;

    if (vecBinResolutionMatched.size() > 0) {
        vecBinMatched = vecBinResolutionMatched;
    } else {
        vecBinMatched = vecBinModeMatched;
    }

    for (AX_U8 j = 0; j < vecBinMatched.size(); j++) {
        LOG_MM_I(SENSOR, "Try loading %s",vecBinMatched[j].c_str());
        if (0 != access(vecBinMatched[j].data(), F_OK)) {
            LOG_MM_E(SENSOR, "%s is not exist.",vecBinMatched[j].data());
            continue;
        }

        nRet = AX_ISP_LoadBinParams(nPipeId, vecBinMatched[j].c_str());
        if (AX_SUCCESS != nRet) {
            LOG_MM_E(SENSOR, "pipe[%d], %d tuning bin , AX_ISP_LoadBinParams ret=0x%x %s. The parameters in sensor.h will be used.\n",
                    nPipeId, j, nRet, vecBinMatched[j].c_str());
            return AX_FALSE;
        } else {
            LOG_MM_C(SENSOR, "pipe[%d] (%s) loaded.", nPipeId, vecBinMatched[j].c_str());
        }
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::LoadBinParams(AX_U8 nPipeId) {
    vector<string> vecPipeTuningBin;
    AX_BOOL bPipeFound = AX_FALSE;
    for (AX_U8 i = 0; i < m_tSnsCfg.nPipeCount; i++) {
        if (nPipeId == m_tSnsCfg.arrPipeAttr[i].nPipeID) {
            bPipeFound = AX_TRUE;
            vecPipeTuningBin = m_tSnsCfg.arrPipeAttr[i].vecTuningBin;
            if ((m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE)
                && (m_tSnsCfg.tSinglePipeSwitchInfo.nHdlId1 == m_tSnsCfg.tSinglePipeSwitchInfo.nCurrentHdlId)) {
                vecPipeTuningBin = m_tSnsCfg.tSinglePipeSwitchInfo.vecTuningBin;
            }
            break;
        }
    }

    if (!bPipeFound) {
        LOG_MM_E(SENSOR, "Can not find pipe: %d.", nPipeId);
        return AX_FALSE;
    }

    if (!vecPipeTuningBin.empty()) {
        return LoadBinParams(nPipeId, vecPipeTuningBin);
    }

    return AX_TRUE;
}

AX_VOID CBaseSensor::SetDiscardYuvFrameNum(AX_U32 nFrmNum) {
    m_tSnsCfg.nDiscardYUVFrmNum = nFrmNum;
}

AX_VOID CBaseSensor::SetSubSwitchSnsData(CBaseSensor* pSwitchSubSns) {
    if (!pSwitchSubSns) {
        LOG_MM_E(SENSOR, "pSwitchSubSns is null.");
        return;
    }

    pSwitchSubSns->GetResolution(m_tSwitchSubSnsInfo.nSnsOutWidth, m_tSwitchSubSnsInfo.nSnsOutHeight);
    m_tSwitchSubSnsInfo.nPipeID = pSwitchSubSns->GetSnsConfig().arrPipeAttr[0].nPipeID;
    m_tSwitchSubSnsInfo.nBusType = pSwitchSubSns->GetSnsConfig().nBusType;
    m_tSwitchSubSnsInfo.nDevNode = pSwitchSubSns->GetSnsConfig().nDevNode;
    m_tSwitchSubSnsInfo.nI2cAddr = pSwitchSubSns->GetSnsConfig().nI2cAddr;
    m_tSwitchSubSnsInfo.tSwitchRegData.nRegAddr = pSwitchSubSns->GetSnsConfig().tSwitchRegData.nRegAddr;
    m_tSwitchSubSnsInfo.tSwitchRegData.nData = pSwitchSubSns->GetSnsConfig().tSwitchRegData.nData;
    m_tSwitchSubSnsInfo.nResetGpioNum = pSwitchSubSns->GetSnsConfig().nResetGpioNum;
    m_tSwitchSubSnsInfo.pSnsObj = pSwitchSubSns->GetSensorObj();
    m_tSwitchSubSnsInfo.pSwitchSns = (AX_VOID*)pSwitchSubSns;

    m_mapPipe2Attr[m_tSwitchSubSnsInfo.nPipeID] = pSwitchSubSns->GetPipeAttr(m_tSwitchSubSnsInfo.nPipeID);
    for (AX_U8 nChn = 0; nChn < AX_VIN_CHN_ID_MAX; nChn++) {
        m_mapPipe2ChnAttr[m_tSwitchSubSnsInfo.nPipeID][nChn] = pSwitchSubSns->GetChnAttr(m_tSwitchSubSnsInfo.nPipeID, nChn);
    }

    // Note: Do not update m_tSnsCfg.nPipeCount which ref to current sensor own pipe count
    m_tSnsCfg.arrPipeAttr[m_tSnsCfg.nPipeCount] = pSwitchSubSns->GetSnsConfig().arrPipeAttr[0];
}

AX_VOID CBaseSensor::SetSnsStartStatus(AX_BOOL bStarted) {
    m_bSensorStarted = bStarted;

    LOG_M_C(SENSOR, "Sensor(%d) %s.", m_tSnsCfg.nSnsID, m_bSensorStarted ? "Started" : "Stopped");
}

AX_BOOL CBaseSensor::EnableMipiSwitch() {
#ifdef SWITCH_SENSOR_SUPPORT
    AX_S32 nRet = -1;
    AX_SWITCH_SNS_INFO_T arrSwitchSnsInfo[AX_MIPI_SWITCH_PIPE_NUM] = {0};
    AX_MIPI_SWITCH_WORK_MODE_E eSwitchWorkMode = AX_MIPI_SWITCH_STAY_LOW;
    AX_S32 nPipeNum = 1;

    switch (m_tSnsCfg.eSwitchSnsType) {
        case E_SNS_SWITCH_SNS_TYPE_MAIN:
            nPipeNum = 2;
            eSwitchWorkMode = AX_MIPI_SWITCH_SWITCH_PERIODIC;
            arrSwitchSnsInfo[0].nSnsId = m_tSnsCfg.arrPipeAttr[0].nPipeID;
            arrSwitchSnsInfo[0].nPipeId = m_tSnsCfg.arrPipeAttr[0].nPipeID;
            arrSwitchSnsInfo[0].eLensType = AX_LENS_TYPE_WIDE_FIELD;
            arrSwitchSnsInfo[0].eWorkMode = AX_MIPI_SWITCH_STAY_LOW;
            arrSwitchSnsInfo[0].eVsyncType = AX_MIPI_SWITCH_FSYNC_FLASH;
            arrSwitchSnsInfo[1].nSnsId = m_tSwitchSubSnsInfo.nPipeID;
            arrSwitchSnsInfo[1].nPipeId = m_tSwitchSubSnsInfo.nPipeID;
            arrSwitchSnsInfo[1].eLensType = AX_LENS_TYPE_WIDE_FIELD;
            arrSwitchSnsInfo[1].eWorkMode = AX_MIPI_SWITCH_STAY_HIGH;
            arrSwitchSnsInfo[1].eVsyncType = AX_MIPI_SWITCH_FSYNC_VSYNC;
            break;
        case E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE:
            if (m_tSnsCfg.tSinglePipeSwitchInfo.nCurrentHdlId == m_tSnsCfg.tSinglePipeSwitchInfo.nHdlId1) {
                eSwitchWorkMode = AX_MIPI_SWITCH_STAY_HIGH;
            }
            arrSwitchSnsInfo[0].nSnsId = m_tSnsCfg.tSinglePipeSwitchInfo.nHdlId0;
            arrSwitchSnsInfo[0].nPipeId = m_tSnsCfg.arrPipeAttr[0].nPipeID;
            arrSwitchSnsInfo[0].eLensType = AX_LENS_TYPE_WIDE_FIELD;
            arrSwitchSnsInfo[0].eWorkMode = AX_MIPI_SWITCH_STAY_LOW;
            arrSwitchSnsInfo[0].eVsyncType = AX_MIPI_SWITCH_FSYNC_FLASH;
            arrSwitchSnsInfo[1].nSnsId = m_tSnsCfg.tSinglePipeSwitchInfo.nHdlId1;
            arrSwitchSnsInfo[1].nPipeId = m_tSnsCfg.arrPipeAttr[0].nPipeID;
            arrSwitchSnsInfo[1].eLensType = AX_LENS_TYPE_LONG_FOCAL;
            arrSwitchSnsInfo[1].eWorkMode = AX_MIPI_SWITCH_STAY_HIGH;
            arrSwitchSnsInfo[1].eVsyncType = AX_MIPI_SWITCH_FSYNC_VSYNC;
            break;
        default:
            LOG_MM_W(SENSOR, "Do not do mipi switch enable for eSwitchSnsType: %d", m_tSnsCfg.eSwitchSnsType);
            return AX_TRUE;
    }

    AX_SWITCH_INFO_T switch_info = {0};
    switch_info.nFps = m_tSnsAttr.fFrameRate;
    switch_info.eWorkMode = eSwitchWorkMode;
    switch_info.nPipeNum = nPipeNum;
    switch_info.tSnsInfo[0] = arrSwitchSnsInfo[0];
    switch_info.tSnsInfo[1] = arrSwitchSnsInfo[1];
    LOG_M_C(SENSOR, "RxDev[%d] mipi_switch, pipe num: %d, sns0: (id: %d, pipe: %d), sns_1: (id: %d, pipe: %d), switch work mode: %d",
                    m_tSnsCfg.nRxDevID,
                    switch_info.nPipeNum,
                    switch_info.tSnsInfo[0].nSnsId,
                    switch_info.tSnsInfo[0].nPipeId,
                    switch_info.tSnsInfo[1].nSnsId,
                    switch_info.tSnsInfo[1].nPipeId,
                    switch_info.eWorkMode);

    nRet = ax_mipi_switch_init(&switch_info);
    if (0 != nRet) {
        LOG_M_E(SENSOR, "ax_mipi_switch_init failed with ret: 0x%x", nRet);
        return AX_FALSE;
    }
    nRet = ax_mipi_switch_start();
    if (0 != nRet) {
        LOG_M_E(SENSOR, "ax_mipi_switch_start failed with ret: 0x%x", nRet);
        return AX_FALSE;
    }

    if ((m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_MAIN) && m_tSwitchSubSnsInfo.pSnsObj) {
        AX_S32 nRet = m_tSwitchSubSnsInfo.pSnsObj->pfn_sensor_write_register(
                                                                m_tSwitchSubSnsInfo.nPipeID,
                                                                m_tSwitchSubSnsInfo.tSwitchRegData.nRegAddr,
                                                                m_tSwitchSubSnsInfo.tSwitchRegData.nData);
        if (0 != nRet) {
            LOG_M_E(SENSOR, "Pipe[%d] write data(%d) to reg_addr(%d) failed with ret: 0x%x",
                            m_tSwitchSubSnsInfo.nPipeID,
                            m_tSwitchSubSnsInfo.tSwitchRegData.nData,
                            m_tSwitchSubSnsInfo.tSwitchRegData.nRegAddr,
                            nRet);
            return AX_FALSE;
        } else {
            LOG_M_C(SENSOR, "RxDev[%d] mipi_switch, Pipe[%d] write data(%d) to reg_addr(%d)",
                            m_tSnsCfg.nRxDevID, m_tSwitchSubSnsInfo.nPipeID,
                            m_tSwitchSubSnsInfo.tSwitchRegData.nData,
                            m_tSwitchSubSnsInfo.tSwitchRegData.nRegAddr);
        }
    }
#endif

    return AX_TRUE;
}

AX_BOOL CBaseSensor::Check3ARoi(AX_U32 nImgWidth, AX_U32 nImgHeight, const AX_ISP_IQ_AE_STAT_ROI_T *pRoi)
{
    AX_U16 i = 0;
    AX_U16 nBaseWidth = 0;
    AX_U16 nBaseNum = 0;

    AX_U16 nInvalidBaseL = 0;
    AX_U16 nInvalidBaseR = 0;
    AX_U16 nOffsetL = 0;
    AX_U16 nOffsetR = 0;

    AX_U16 nValidBaseWidth = 0;
    AX_U16 nGridWidth = 0;
    AX_U16 nGridNum = 0;
    AX_U16 nGridL = 0;
    AX_U16 nGridR = 0;

    if (nImgHeight > 2160) {
        nBaseWidth = 896;
    } else {
        nBaseWidth = 768;
    }
    nBaseNum = (nImgWidth + nBaseWidth - 1) / nBaseWidth;

    nOffsetL = pRoi->nRoiOffsetH;
    LOG_MM_D(SENSOR, "nOffsetL: %d", nOffsetL);
    for (i = 0; i < nBaseNum; i++) {
        nValidBaseWidth = ((nImgWidth - i * nBaseWidth) > nBaseWidth) ? nBaseWidth : (nImgWidth - i * nBaseWidth);
        if (nOffsetL < nValidBaseWidth) {
            break;
        }
        nOffsetL -= nValidBaseWidth;
        nInvalidBaseL++;
    }

    nOffsetR = nImgWidth - pRoi->nRoiOffsetH - pRoi->nRoiRegionW * pRoi->nRoiRegionNumH;
    LOG_MM_D(SENSOR, "nOffsetR: %d", nOffsetR);
    for (i = nBaseNum - 1; i >= 0; i--) {
        nValidBaseWidth = ((nImgWidth - i * nBaseWidth) > nBaseWidth) ? nBaseWidth : (nImgWidth - i * nBaseWidth);
        if (nOffsetR < nValidBaseWidth) {
            break;
        }
        nOffsetR -= nValidBaseWidth;
        nInvalidBaseR++;
    }
    LOG_MM_D(SENSOR, "[%d %d]", nOffsetL, nOffsetR);

    for (i = 0; i < nBaseNum; i++) {
        if (i < nInvalidBaseL || i > nBaseNum - 1 - nInvalidBaseR) {
            LOG_MM_W(SENSOR, "Base [%2d]: Invalid.", i);
            continue;
        }

        if (i == 0) {
            nValidBaseWidth = nBaseWidth - nOffsetL;
            nGridWidth = (pRoi->nRoiRegionNumH == 1) ? nValidBaseWidth : pRoi->nRoiRegionW;
            nGridL = nOffsetL;
            nGridNum = nValidBaseWidth / nGridWidth;
            nGridR = nValidBaseWidth - nGridNum * nGridWidth;
            if ((nGridR > 0 && nGridR < 8) || nGridWidth < 8) {
                LOG_MM_W(SENSOR, "[%d]tRoi illegal for nGridL(%d) nGridR(%d) nGridWidth(%d)!", i, nGridL, nGridR, nGridWidth);
                return AX_FALSE;
            }
        }
        else if (i == (nBaseNum - 1)) {
            nValidBaseWidth = (((nImgWidth - i * nBaseWidth) > nBaseWidth) ? nBaseWidth : (nImgWidth - i * nBaseWidth)) - nOffsetR;
            nGridWidth = (pRoi->nRoiRegionNumH == 1) ? nValidBaseWidth : pRoi->nRoiRegionW;
            nGridL = nGridWidth - (nGridR ? nGridR : nGridWidth);
            nGridNum = (nValidBaseWidth - nGridL) / nGridWidth;
            nGridR = nOffsetR;
            if ((nGridL > 0 && nGridL < 8) || nGridWidth < 8) {
                LOG_MM_W(SENSOR, "[%d]tRoi illegal for nGridL(%d) nGridR(%d) nGridWidth(%d)!", i, nGridL, nGridR, nGridWidth);
                return AX_FALSE;
            }
        }
        else {
            nValidBaseWidth = nBaseWidth;
            nGridWidth = (pRoi->nRoiRegionNumH == 1) ? nValidBaseWidth : pRoi->nRoiRegionW;
            nGridL = nGridWidth - (nGridR ? nGridR : nGridWidth);
            nGridNum = (nValidBaseWidth - nGridL) / nGridWidth;
            nGridR = (nValidBaseWidth - nGridL) - nGridNum * nGridWidth;

            if (nInvalidBaseL && i == nInvalidBaseL) {
                nValidBaseWidth -= nOffsetL;
                nGridWidth = (pRoi->nRoiRegionNumH == 1) ? nValidBaseWidth : pRoi->nRoiRegionW;
                nGridNum = (nValidBaseWidth - nGridL) / nGridWidth;
                nGridR = (nValidBaseWidth - nGridL) - nGridNum * nGridWidth;
            }
            if (nInvalidBaseR && i == nBaseNum - 1 - nInvalidBaseR) {
                nValidBaseWidth -= nOffsetR;
                nGridWidth = (pRoi->nRoiRegionNumH == 1) ? nValidBaseWidth : pRoi->nRoiRegionW;
                nGridNum = (nValidBaseWidth - nGridL) / nGridWidth;
                nGridR = (nValidBaseWidth - nGridL) - nGridNum * nGridWidth;
            }

            if ((nGridL > 0 && nGridL < 8) || (nGridR > 0 && nGridR < 8) || nGridWidth < 8) {
                LOG_MM_W(SENSOR, "[%d]tRoi illegal for nGridL(%d) nGridR(%d) nGridWidth(%d)!", i, nGridL, nGridR, nGridWidth);
                return AX_FALSE;
            }
        }

        LOG_MM_I(SENSOR, "Base [%2d]: valid width %4d [%3d + %4d * %2d + %3d]",
                          i, nValidBaseWidth, nGridL, nGridWidth, nGridNum, nGridR);
    }

    return AX_TRUE;
}

AX_BOOL CBaseSensor::Get3ARoi(AX_U32 nImgWidth, AX_U32 nImgHeight, const AX_WIN_AREA_T *pCropRect, AX_ISP_IQ_AE_STAT_PARAM_T *ptAeParam, AX_ISP_IQ_AWB_STAT_PARAM_T *ptAwbParam)
{
    AX_U32 nAlignNum = 8;
    AX_U32 nCropWidth = 0, nCropHeight = 0;
    AX_ISP_IQ_AE_STAT_ROI_T tRoi = {0};

    nCropWidth = pCropRect->nWidth / 2 * 2;
    nCropHeight = pCropRect->nHeight / 2 * 2;

    /* AE Grid0/AWB Grid */
    tRoi.nRoiRegionNumH = ptAeParam->tGridRoi[0].nRoiRegionNumH;
    tRoi.nRoiRegionNumV = ptAeParam->tGridRoi[0].nRoiRegionNumV;
    tRoi.nRoiRegionW = ALIGN_DOWN((nCropWidth / tRoi.nRoiRegionNumH), nAlignNum);
    tRoi.nRoiRegionH = ALIGN_DOWN((nCropHeight / tRoi.nRoiRegionNumV), 2);
    tRoi.nRoiOffsetV = ALIGN_DOWN(((nCropHeight - tRoi.nRoiRegionH * tRoi.nRoiRegionNumV) >> 1) + pCropRect->nStartY, 2);
    do {
        tRoi.nRoiOffsetH = ALIGN_DOWN(((nCropWidth - tRoi.nRoiRegionW * tRoi.nRoiRegionNumH) >> 1) + pCropRect->nStartX, nAlignNum);

        if (Check3ARoi(nImgWidth, nImgHeight, &tRoi)) {
            break;
        }

        if (tRoi.nRoiRegionW >= nAlignNum * 2) {
            tRoi.nRoiRegionW -= nAlignNum;
        } else {
            LOG_M_E(SENSOR, "Get AE Grid0/AWB Grid failed.");
            return AX_FALSE;
        }
    } while (0);
    memcpy(&ptAeParam->tGridRoi[0], &tRoi, sizeof(AX_ISP_IQ_AE_STAT_ROI_T));
    memcpy(&ptAwbParam->tGridRoi, &tRoi, sizeof(AX_ISP_IQ_AE_STAT_ROI_T));

    /* AE Grid1 */
    tRoi.nRoiRegionNumH = ptAeParam->tGridRoi[1].nRoiRegionNumH;
    tRoi.nRoiRegionNumV = ptAeParam->tGridRoi[1].nRoiRegionNumV;
    tRoi.nRoiRegionW = ALIGN_DOWN((nCropWidth / tRoi.nRoiRegionNumH), nAlignNum);
    tRoi.nRoiRegionH = ALIGN_DOWN((nCropHeight / tRoi.nRoiRegionNumV), 2);
    tRoi.nRoiOffsetV = ALIGN_DOWN(((nCropHeight - tRoi.nRoiRegionH * tRoi.nRoiRegionNumV) >> 1) + pCropRect->nStartY, 2);
    do {
        tRoi.nRoiOffsetH = ALIGN_DOWN(((nCropWidth - tRoi.nRoiRegionW * tRoi.nRoiRegionNumH) >> 1) + pCropRect->nStartX, nAlignNum);

        if (Check3ARoi(nImgWidth, nImgHeight, &tRoi)) {
            break;
        }

        if (tRoi.nRoiRegionW >= nAlignNum * 2) {
            tRoi.nRoiRegionW -= nAlignNum;
        } else {
            LOG_M_E(SENSOR, "Get AE Grid1 failed.");
            return AX_FALSE;
        }
    } while (0);
    memcpy(&ptAeParam->tGridRoi[1], &tRoi, sizeof(AX_ISP_IQ_AE_STAT_ROI_T));

    /* AE Hist */
    tRoi.nRoiRegionNumH = 16;
    tRoi.nRoiRegionNumV = 16;
    tRoi.nRoiRegionW = ALIGN_DOWN((nCropWidth / tRoi.nRoiRegionNumH), nAlignNum);
    tRoi.nRoiRegionH = ALIGN_DOWN((nCropHeight / tRoi.nRoiRegionNumV), 2);
    tRoi.nRoiOffsetV = ALIGN_DOWN(((nCropHeight - tRoi.nRoiRegionH * tRoi.nRoiRegionNumV) >> 1) + pCropRect->nStartY, 2);
    do {
        tRoi.nRoiOffsetH = ALIGN_DOWN(((nCropWidth - tRoi.nRoiRegionW * tRoi.nRoiRegionNumH) >> 1) + pCropRect->nStartX, nAlignNum);

        if (Check3ARoi(nImgWidth, nImgHeight, &tRoi)) {
            break;
        }

        if (tRoi.nRoiRegionW >= nAlignNum * 2) {
            tRoi.nRoiRegionW -= nAlignNum;
        } else {
            LOG_M_E(SENSOR, "Get AE Hist failed.");
            return AX_FALSE;
        }
    } while (0);
    ptAeParam->tHistRoi.nRoiOffsetH = tRoi.nRoiOffsetH;
    ptAeParam->tHistRoi.nRoiOffsetV = tRoi.nRoiOffsetV;
    ptAeParam->tHistRoi.nRoiWidth = tRoi.nRoiRegionW;
    ptAeParam->tHistRoi.nRoiHeight = tRoi.nRoiRegionH;

    return AX_TRUE;
}

AX_BOOL CBaseSensor::GetChnCrop(AX_U8 nPipeId, AX_U8 nSnsId, AX_VIN_CROP_INFO_T* pCropInfo) {
    AX_S32 nRet = -1;

#ifdef SWITCH_SENSOR_SUPPORT
    if ((m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE)) {
        nRet = AX_VIN_GetChnCropExt(nPipeId, AX_VIN_CHN_ID_MAIN, nSnsId, pCropInfo);
        if (AX_SUCCESS != nRet) {
            LOG_MM_E(SENSOR, "AX_VIN_GetChnCropExt(pipe: %d, sns: %d) failed ret=0x%x", nPipeId, nSnsId, nRet);
            return AX_FALSE;
        }
    } else {
#endif
        nRet = AX_VIN_GetChnCrop(nPipeId, AX_VIN_CHN_ID_MAIN, pCropInfo);
        if (AX_SUCCESS != nRet) {
            LOG_MM_E(SENSOR, "AX_VIN_GetChnCrop failed ret=0x%x", nRet);
            return AX_FALSE;
        }
#ifdef SWITCH_SENSOR_SUPPORT
    }
#endif

    return AX_TRUE;
}

AX_BOOL CBaseSensor::SetChnCrop(AX_U8 nPipeId, AX_U8 nSnsId, const AX_VIN_CROP_INFO_T* pCropInfo) {
    AX_S32 nRet = -1;

#ifdef SWITCH_SENSOR_SUPPORT
    if ((m_tSnsCfg.eSwitchSnsType == E_SNS_SWITCH_SNS_TYPE_SINGLE_PIPE)) {
        nRet = AX_VIN_SetChnCropExt(nPipeId, AX_VIN_CHN_ID_MAIN, nSnsId, pCropInfo);
        if (AX_SUCCESS != nRet) {
            LOG_MM_E(SENSOR, "AX_VIN_SetChnCropExt(pipe: %d, sns: %d) failed ret=0x%x", nPipeId, nSnsId, nRet);
            return AX_FALSE;
        }
    } else {
#endif
        nRet = AX_VIN_SetChnCrop(nPipeId, AX_VIN_CHN_ID_MAIN, pCropInfo);
        if (AX_SUCCESS != nRet) {
            LOG_MM_E(SENSOR, "[%d] AX_VIN_SetChnCrop failed ret=0x%x", nPipeId, nRet);
            return AX_FALSE;
        }
#ifdef SWITCH_SENSOR_SUPPORT
    }
#endif

    return AX_TRUE;
}

AX_BOOL CBaseSensor::SetEZoomAeAwbIQ(AX_U8 nSnsId, AX_U8 nPipeId) {
    std::lock_guard<std::mutex> _ApiLck(m_mtxCrop);

    if (nSnsId != m_tCropRectInfo.nSnsId || !m_tCropRectInfo.bValid) {
        return AX_FALSE;
    }

    LOG_MM_I(SENSOR, "Sns[%d]Pipe[%d] +++", nSnsId, nPipeId);

    m_tCropRectInfo.bValid = 0;

    AX_S32 nRet = -1;
    AX_U32 nWidth = m_tSnsCfg.tSnsResolution.nSnsOutWidth;
    AX_U32 nHeight = m_tSnsCfg.tSnsResolution.nSnsOutHeight;
    AX_ISP_IQ_AE_STAT_PARAM_T stIspAeStatParam{0};
    nRet = AX_ISP_IQ_GetAeStatParam(nPipeId, &stIspAeStatParam);
    if (AX_SUCCESS != nRet) {
        LOG_MM_E(SENSOR, "AX_ISP_IQ_GetAeStatParam failed ret=0x%x", nRet);
        return AX_FALSE;
    }
    AX_ISP_IQ_AWB_STAT_PARAM_T stAwbStatParam{0};
    nRet = AX_ISP_IQ_GetAwbStatParam(nPipeId, &stAwbStatParam);
    if (AX_SUCCESS != nRet) {
        LOG_MM_E(SENSOR, "AX_ISP_IQ_GetAwbStatParam failed ret=0x%x", nRet);
        return AX_FALSE;
    }

    LOG_MM_I(SENSOR, "Sns[%d]Pipe[%d] EZoom CropRect(%d, %d, %d, %d)",
                      nSnsId, nPipeId,
                      m_tCropRectInfo.tCropRect.nStartX, m_tCropRectInfo.tCropRect.nStartY,
                      m_tCropRectInfo.tCropRect.nWidth, m_tCropRectInfo.tCropRect.nHeight);

    if (!Get3ARoi(nWidth, nHeight, &m_tCropRectInfo.tCropRect, &stIspAeStatParam, &stAwbStatParam)) {
        LOG_MM_E(SENSOR, "Get 3A Roi failed frome EZoom crop[%d, %d, %d, %d]!",
                                                m_tCropRectInfo.tCropRect.nStartX,
                                                m_tCropRectInfo.tCropRect.nStartY,
                                                m_tCropRectInfo.tCropRect.nWidth,
                                                m_tCropRectInfo.tCropRect.nHeight);
        return AX_FALSE;
    }

    nRet = AX_ISP_IQ_SetAeStatParam(nPipeId, &stIspAeStatParam);
    if (AX_SUCCESS != nRet) {
        LOG_MM_E(SENSOR, "AX_ISP_IQ_SetAeStatParam failed ret=0x%x", nRet);
        return AX_FALSE;
    }
    nRet = AX_ISP_IQ_SetAwbStatParam(nPipeId, &stAwbStatParam);
    if (AX_SUCCESS != nRet) {
        LOG_MM_E(SENSOR, "AX_ISP_IQ_SetAwbStatParam failed ret=0x%x", nRet);
        return AX_FALSE;
    }

    LOG_MM_I(SENSOR, "Sns[%d]Pipe[%d] ---", nSnsId, nPipeId);

    return AX_TRUE;
}

AX_VOID CBaseSensor::SetEZoomCropRect(AX_U8 nSnsId, AX_U32 nImgWidth, AX_U32 nImgHeight, AX_WIN_AREA_T* pCropRect, AX_VIN_ROTATION_E eRotation) {
    std::lock_guard<std::mutex> _ApiLck(m_mtxCrop);

    m_tCropRectInfo.nSnsId = nSnsId;
    m_tCropRectInfo.bValid = 1;

    switch (eRotation) {
        case AX_VIN_ROTATION_0:
            m_tCropRectInfo.tCropRect = *pCropRect;
            break;
        case AX_VIN_ROTATION_90:
            m_tCropRectInfo.tCropRect.nStartX = nImgWidth - pCropRect->nStartY - pCropRect->nHeight;
            m_tCropRectInfo.tCropRect.nStartY = pCropRect->nStartX;
            m_tCropRectInfo.tCropRect.nWidth  = pCropRect->nHeight;
            m_tCropRectInfo.tCropRect.nHeight = pCropRect->nWidth;
            break;
        case AX_VIN_ROTATION_180:
            m_tCropRectInfo.tCropRect.nStartX = nImgWidth - pCropRect->nStartX - pCropRect->nWidth;
            m_tCropRectInfo.tCropRect.nStartY = nImgHeight - pCropRect->nStartY - pCropRect->nHeight;
            m_tCropRectInfo.tCropRect.nWidth  = pCropRect->nWidth;
            m_tCropRectInfo.tCropRect.nHeight = pCropRect->nHeight;
            break;
        case AX_VIN_ROTATION_270:
            m_tCropRectInfo.tCropRect.nStartX = pCropRect->nStartY;
            m_tCropRectInfo.tCropRect.nStartY = nImgHeight - pCropRect->nStartX - pCropRect->nWidth;
            m_tCropRectInfo.tCropRect.nWidth  = pCropRect->nHeight;
            m_tCropRectInfo.tCropRect.nHeight = pCropRect->nWidth;
            break;
        default:
            break;
    }
    LOG_MM_I(SENSOR, "[%d] SetEZoomCropRect(%d, %d, %d, %d) with rotation: %d",
                      nSnsId, pCropRect->nStartX, pCropRect->nStartY, pCropRect->nWidth, pCropRect->nHeight, eRotation);
}

AX_BOOL CBaseSensor::GetDstCenterOffset(AX_U8 nIvpsGrp, AX_U32 nWidth, AX_U32 nHeight, AX_S16 &nCenterOffsetX, AX_S16 &nCenterOffsetY)
{
    AX_S32 nRet = IVPS_SUCC;
    AX_IVPS_POINT_NICE_T tOrgCenterPoint = {(AX_F32)(nWidth / 2), (AX_F32)(nHeight / 2)};
    AX_IVPS_POINT_NICE_T tDstCenterPoint = {0};
    nRet= AX_IVPS_PointQuerySrc2Dst(nIvpsGrp, &tOrgCenterPoint, &tDstCenterPoint);
    if (nRet != IVPS_SUCC) {
        LOG_MM_E(SENSOR, "Get dst point for (%d, %d) failed, ret: 0x%x", (AX_S16)tOrgCenterPoint.fX, (AX_S16)tOrgCenterPoint.fY, nRet);
        return AX_FALSE;
    }

    AX_IVPS_POINT_NICE_T tDstOffsetCenterPoint = {0};
    tOrgCenterPoint = {(AX_F32)(tOrgCenterPoint.fX + nCenterOffsetX), (AX_F32)(tOrgCenterPoint.fY + nCenterOffsetY)};
    nRet= AX_IVPS_PointQuerySrc2Dst(nIvpsGrp, &tOrgCenterPoint, &tDstOffsetCenterPoint);
    if (nRet != IVPS_SUCC) {
        LOG_MM_E(SENSOR, "Get dst point for (%d, %d) failed, ret: 0x%x", (AX_S16)tOrgCenterPoint.fX, (AX_S16)tOrgCenterPoint.fY, nRet);
        return AX_FALSE;
    }

    nCenterOffsetX = tDstOffsetCenterPoint.fX - tDstCenterPoint.fX;
    nCenterOffsetY = tDstOffsetCenterPoint.fY - tDstCenterPoint.fY;

    LOG_MM_I(SENSOR, "dst center point(%d, %d), dst offset-center point(%d, %d), dst center offset(%d, %d)",
                    (AX_S16)tDstCenterPoint.fX, (AX_S16)tDstCenterPoint.fY,
                    (AX_S16)tDstOffsetCenterPoint.fX, (AX_S16)tDstOffsetCenterPoint.fY,
                    nCenterOffsetX, nCenterOffsetY);

    return AX_TRUE;
}
