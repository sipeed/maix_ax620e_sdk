
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_vin.h"

#include <string.h>

#include "ax_opal_log.h"
#include "ax_opal_utils.h"

#define LOG_TAG "HAL_VIN"

static AX_BOOL g_bSensorReset[AX_OPAL_SNS_ID_BUTT];

AX_S32 AX_OPAL_HAL_VIN_CreateMipi(AX_S32 nRxDevId, AX_S32 nDevId, AX_S32 nPipeId, AX_VOID* pSensorHandle, AX_OPAL_CAM_MIPI_CFG_T* ptMipiCfg) {

    AX_SENSOR_REGISTER_FUNC_T* ptSnsHdl = (AX_SENSOR_REGISTER_FUNC_T*)pSensorHandle;
    AX_S32 nRet = 0;
    /* Step 2.1. Reset Sensor */
    {
        AX_S32 nResetGpio = ptMipiCfg->nResetGpio;
        if (nResetGpio == -1) {
            nResetGpio = (nDevId == 0) ? 97 : 40;
        }

        if (nPipeId < AX_OPAL_SNS_ID_BUTT && !g_bSensorReset[nPipeId]) {
            if (AX_NULL != ptSnsHdl->pfn_sensor_reset) {
                nRet = ptSnsHdl->pfn_sensor_reset(nPipeId, nResetGpio);
                if (0 != nRet) {
                    LOG_M_E(LOG_TAG, "reset sensor failed, ret=0x%x", nRet);
                    return nRet;
                }
            } else {
                LOG_M_E(LOG_TAG, "not support reset sensor.");
                return -1;
            }

            g_bSensorReset[nPipeId] = AX_TRUE;
        }
    }

    /* Step 2.2. AX_MIPI_RX_SetLaneCombo */
    AX_LANE_COMBO_MODE_E eLanComboMode = ((AX_INPUT_MODE_E)ptMipiCfg->eInputMode == AX_INPUT_MODE_MIPI
                && (AX_MIPI_LANE_NUM_E)ptMipiCfg->eLaneNum == AX_MIPI_DATA_LANE_4) ?
                AX_LANE_COMBO_MODE_0 : AX_LANE_COMBO_MODE_1;
    nRet = AX_MIPI_RX_SetLaneCombo(eLanComboMode);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_MIPI_RX_SetLaneCombo failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 2.3. AX_MIPI_RX_SetAttr */
    AX_MIPI_RX_DEV_T tMipiAttr;
    memset(&tMipiAttr, 0, sizeof(tMipiAttr));
    tMipiAttr.eInputMode = (AX_INPUT_MODE_E)ptMipiCfg->eInputMode;
    tMipiAttr.tMipiAttr.ePhyMode = (AX_MIPI_PHY_TYPE_E)ptMipiCfg->ePhyMode;
    tMipiAttr.tMipiAttr.eLaneNum = (AX_MIPI_LANE_NUM_E)ptMipiCfg->eLaneNum;
    tMipiAttr.tMipiAttr.nDataRate = ptMipiCfg->nDataRate;
    tMipiAttr.tMipiAttr.nDataLaneMap[0] = ptMipiCfg->nDataLaneMap0;
    tMipiAttr.tMipiAttr.nDataLaneMap[1] = ptMipiCfg->nDataLaneMap1;
    tMipiAttr.tMipiAttr.nDataLaneMap[2] = ptMipiCfg->nDataLaneMap2;
    tMipiAttr.tMipiAttr.nDataLaneMap[3] = ptMipiCfg->nDataLaneMap3;
    tMipiAttr.tMipiAttr.nClkLane[0] = ptMipiCfg->nClkLane0;
    tMipiAttr.tMipiAttr.nClkLane[1] = ptMipiCfg->nClkLane1;
    nRet = AX_MIPI_RX_SetAttr(nRxDevId, &tMipiAttr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_MIPI_RX_SetAttr failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 2.4. AX_MIPI_RX_Reset */
    nRet = AX_MIPI_RX_Reset(nRxDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_MIPI_RX_Reset failed, ret=0x%x", nRet);
        return -1;
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_CreateDev(AX_S32 nRxDevId, AX_S32 nDevId, AX_S32 nPipeId, AX_S32 nLaneNum, AX_OPAL_CAM_DEV_CFG_T* ptDevCfg) {
    AX_S32 nRet = 0;

    AX_VIN_DEV_ATTR_T tDevAttr;
    memset(&tDevAttr, 0x0, sizeof(tDevAttr));
    tDevAttr.bImgDataEnable = (AX_BOOL)ptDevCfg->bImgDataEnable;
    tDevAttr.eDevMode = (AX_VIN_DEV_MODE_E)ptDevCfg->eDevMode;
    tDevAttr.eSnsIntfType = (AX_SNS_INTF_TYPE_E)ptDevCfg->eSnsIntfType;
    for (int i = 0; i < AX_HDR_CHN_NUM; i++) {
        tDevAttr.tDevImgRgn[i].nStartX = ptDevCfg->tDevImgRgnStartX;
        tDevAttr.tDevImgRgn[i].nStartY = ptDevCfg->tDevImgRgnStartY;
        tDevAttr.tDevImgRgn[i].nWidth = ptDevCfg->tDevImgRgnWidth;
        tDevAttr.tDevImgRgn[i].nHeight = ptDevCfg->tDevImgRgnHeight;
    }
    tDevAttr.ePixelFmt = (AX_IMG_FORMAT_E)ptDevCfg->ePixelFmt;
    tDevAttr.eBayerPattern = (AX_BAYER_PATTERN_E)ptDevCfg->eBayerPattern;
    //tDevAttr.eSkipFrame = (AX_SNS_SKIP_FRAME_E)ptDevCfg->eSkipFrame;
    tDevAttr.eSnsMode = (AX_SNS_HDR_MODE_E)ptDevCfg->eSnsMode;
    tDevAttr.eSnsOutputMode = (AX_SNS_OUTPUT_MODE_E)ptDevCfg->eSnsOutputMode;
    tDevAttr.bNonImgDataEnable = (AX_BOOL)ptDevCfg->bNonImgEnable;
    if (ptDevCfg->tMipiIntfAttrImgVc == 0) {
        tDevAttr.tMipiIntfAttr.szImgVc[0] = 0;
        tDevAttr.tMipiIntfAttr.szImgVc[1] = 1;
    }
    tDevAttr.tMipiIntfAttr.szImgDt[0] = ptDevCfg->tMipiIntfAttrImgDt;
    tDevAttr.tMipiIntfAttr.szImgDt[1] = ptDevCfg->tMipiIntfAttrImgDt;
    tDevAttr.tMipiIntfAttr.szInfoVc[0] = ptDevCfg->tMipiIntfAttrInfoVc;
    tDevAttr.tMipiIntfAttr.szInfoVc[1] = ptDevCfg->tMipiIntfAttrInfoVc;
    tDevAttr.tMipiIntfAttr.szInfoDt[0] = ptDevCfg->tMipiIntfAttrInfoDt;
    tDevAttr.tMipiIntfAttr.szInfoDt[1] = ptDevCfg->tMipiIntfAttrInfoDt;

    // HDR & 8M & 4lane, should use 2 multiplex
    if (IS_SNS_HDR_MODE(tDevAttr.eSnsMode)
        && ptDevCfg->tDevImgRgnWidth >= 3840
        && nLaneNum >= AX_MIPI_DATA_LANE_4) {
        tDevAttr.eDevWorkMode = AX_VIN_DEV_WORK_MODE_2MULTIPLEX;
    } else {
        tDevAttr.eDevWorkMode = AX_VIN_DEV_WORK_MODE_1MULTIPLEX;
    }

    /* Step 3.1. AX_VIN_CreateDev */
    nRet = AX_VIN_CreateDev(nDevId, &tDevAttr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_CreateDev failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 3.2. AX_VIN_SetDevAttr */
    nRet = AX_VIN_SetDevAttr(nDevId, &tDevAttr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetDevAttr failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 3.3. AX_VIN_SetDevBindPipe */
    AX_VIN_DEV_BIND_PIPE_T tDevBindPipe = {0};
    tDevBindPipe.nNum = 1;
    tDevBindPipe.nPipeId[0] = nPipeId;
    switch (tDevAttr.eSnsMode) {
    case AX_SNS_LINEAR_MODE:
        tDevBindPipe.nHDRSel[0] = 0x1;
        break;

    case AX_SNS_HDR_2X_MODE:
        tDevBindPipe.nHDRSel[0] = 0x1 | 0x2;
        break;

    case AX_SNS_HDR_3X_MODE:
        tDevBindPipe.nHDRSel[0] = 0x1 | 0x2 | 0x4;
        break;

    case AX_SNS_HDR_4X_MODE:
        tDevBindPipe.nHDRSel[0] = 0x1 | 0x2 | 0x4 | 0x8;
        break;

    default:
        tDevBindPipe.nHDRSel[0] = 0x1;
        break;
    }
    nRet = AX_VIN_SetDevBindPipe(nDevId, &tDevBindPipe);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetDevBindPipe failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 3.4. AX_VIN_SetDevBindPipe */
    nRet = AX_VIN_SetDevBindMipi(nDevId, nRxDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetDevBindMipi failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_CreatePipe(AX_S32 nPipeId, AX_OPAL_CAM_PIPE_CFG_T* ptPipeCfg) {
    AX_S32 nRet = 0;

    AX_VIN_PIPE_ATTR_T tPipeAttr;
    memset(&tPipeAttr, 0x0, sizeof(AX_VIN_PIPE_ATTR_T));
    tPipeAttr.tPipeImgRgn.nWidth = ptPipeCfg->nWidth;
    tPipeAttr.tPipeImgRgn.nHeight = ptPipeCfg->nHeight;
    tPipeAttr.nWidthStride = ptPipeCfg->nWidthStride;
    tPipeAttr.eBayerPattern = (AX_BAYER_PATTERN_E)ptPipeCfg->eBayerPattern;
    tPipeAttr.ePixelFmt = (AX_IMG_FORMAT_E)ptPipeCfg->ePixelFmt;
    tPipeAttr.eSnsMode = (AX_SNS_HDR_MODE_E)ptPipeCfg->eSnsMode;
    tPipeAttr.tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)ptPipeCfg->enCompressMode;
    tPipeAttr.bAiIspEnable = (AX_BOOL)ptPipeCfg->bAiIspEnable;
    tPipeAttr.ePipeWorkMode = (AX_VIN_PIPE_WORK_MODE_E)ptPipeCfg->eWorkMode;
    // nLoadRawNode
    // uDumpNodeMask
    // bHMirror
    // tPipeAttr.bMotionComp = (AX_BOOL)ptPipeCfg->bMotionComp;
    tPipeAttr.tMotionAttr.bMotionComp = (AX_BOOL)ptPipeCfg->bMotionComp;
    tPipeAttr.tMotionAttr.bMotionEst = (AX_BOOL)ptPipeCfg->bMotionEst;
    tPipeAttr.tMotionAttr.bMotionShare = (AX_BOOL)ptPipeCfg->bMotionShare;
    tPipeAttr.tNrAttr.t3DnrAttr.tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)ptPipeCfg->b3DnrIsCompress;
    tPipeAttr.tNrAttr.tAinrAttr.tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)ptPipeCfg->bAinrIsCompress;
    tPipeAttr.tFrameRateCtrl.fSrcFrameRate = ptPipeCfg->fSrcFrameRate;
    tPipeAttr.tFrameRateCtrl.fDstFrameRate = ptPipeCfg->fDstFrameRate;

    /* Step 4.1. AX_VIN_CreatePipe */
    nRet = AX_VIN_CreatePipe(nPipeId, &tPipeAttr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_CreatePipe failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 4.2. AX_VIN_SetPipeAttr */
    nRet = AX_VIN_SetPipeAttr(nPipeId, &tPipeAttr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetPipeAttr failed, ret=0x%x", nRet);
        return -1;
    }

    // if (AX_TRUE == m_IniSensorPrams.bHMirror) {
    //     AX_VIN_SetPipeMirror(nPipeId, AX_TRUE);
    // }

    /* Step 4.3. AX_VIN_SetPipeFrameSource */
    nRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_DEV);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetPipeFrameSource failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_CreateChn(AX_S32 nPipeId, AX_S32 nChnId, AX_OPAL_CAM_CHN_CFG_T* ptChnCfg) {
    AX_S32 nRet = 0;

    AX_VIN_CHN_ID_E eChnId = AX_VIN_CHN_ID_MAIN;

    AX_VIN_CHN_ATTR_T tChnAttr;
    memset(&tChnAttr, 0x0, sizeof(AX_VIN_CHN_ATTR_T));
    tChnAttr.nWidth = ptChnCfg->nWidth;
    tChnAttr.nHeight = ptChnCfg->nHeight;
    tChnAttr.nWidthStride = ptChnCfg->nWidthStride;
    tChnAttr.nDepth = ptChnCfg->nDepth;
    tChnAttr.eImgFormat = (AX_IMG_FORMAT_E)ptChnCfg->ePixelFmt;
    tChnAttr.tCompressInfo.enCompressMode = (AX_COMPRESS_MODE_E)ptChnCfg->enCompressMode;
    tChnAttr.tCompressInfo.u32CompressLevel = ptChnCfg->u32CompressLevel;
    // bChnEnable[chn]
    // bChnFlip[chn]

    /* Step 7.1. AX_VIN_SetChnAttr */
    nRet = AX_VIN_SetChnAttr(nPipeId, eChnId, &tChnAttr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "[%d] AX_VIN_SetChnAttr[%d] failed, ret=0x%x", nPipeId, eChnId, nRet);
        return -1;
    }

    // frame mode
    /* Step 7.2. AX_VIN_SetChnFrameMode */
    {
        AX_VIN_FRAME_MODE_E eFrameMode = AX_VIN_FRAME_MODE_OFF;
        const char *envValue = getenv("VIN_FRAME_MODE");

        if (envValue != NULL) {
            eFrameMode = (AX_VIN_FRAME_MODE_E)atoi(envValue);
        } else {
            if (IS_AX620Q) {
                eFrameMode = AX_VIN_FRAME_MODE_RING;
            }
        }

        if (eFrameMode < AX_VIN_FRAME_MODE_BUTT) {
            nRet = AX_VIN_SetChnFrameMode(nPipeId, eChnId, eFrameMode);
            if (0 != nRet) {
                LOG_M_E(LOG_TAG, "[%d] AX_VIN_SetChnFrameMode[%d] failed, ret=0x%x.", nPipeId, eChnId, nRet);
            } else {
                LOG_M_C(LOG_TAG, "[%d] AX_VIN_SetChnFrameMode[%d] mode[%d]", nPipeId, eChnId, eFrameMode);
            }
        }
    }

    nRet = AX_VIN_SetDiscardYuvFrameNumbers(nPipeId, eChnId, 15);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "[%d][%d] AX_VIN_SetDiscardYuvFrameNumbers(15) failed, ret=0x%x.", nPipeId, eChnId, nRet);
    }

    // /* Step 7.3. AX_VIN_EnableChn */
    // nRet = AX_VIN_EnableChn(nPipeId, eChnId);
    // if (0 != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_VIN_EnableChn failed, ret=0x%x", nRet);
    //     return -1;
    // }

    // if (AX_TRUE == m_IniSensorPrams.bChnFlip[i]) {
    //     AX_VIN_SetChnFlip(nPipeId, (AX_VIN_CHN_ID_E)i, AX_TRUE);
    // }

    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_StartMipi(AX_S32 nRxDevId) {
    AX_S32 nRet = 0;
    /* Step 2.5. AX_MIPI_RX_Start */
    nRet = AX_MIPI_RX_Start(nRxDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_MIPI_RX_Start failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_StartDev(AX_S32 nDevId) {
    AX_S32 nRet = 0;
    /* Step 8.3. AX_VIN_EnableDev */
    nRet = AX_VIN_EnableDev(nDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_EnableDev failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_StartPipe(AX_S32 nPipeId) {
    AX_S32 nRet = 0;
    /* Step 8.1. AX_VIN_StartPipe */
    nRet = AX_VIN_StartPipe(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_StartPipe failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_StartChn(AX_S32 nPipeId, AX_S32 nChnId) {
    AX_S32 nRet = 0;
    /* Step 7.3. AX_VIN_EnableChn */
    nRet = AX_VIN_EnableChn(nPipeId, (AX_VIN_CHN_ID_E)nChnId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_EnableChn failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_StopMipi(AX_S32 nRxDevId) {
    AX_S32 nRet = 0;
    nRet = AX_MIPI_RX_Stop(nRxDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_MIPI_RX_Start failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_StopDev(AX_S32 nDevId) {
    AX_S32 nRet = 0;
    nRet = AX_VIN_DisableDev(nDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_DisableDev failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_StopPipe(AX_S32 nPipeId) {
    AX_S32 nRet = 0;
    nRet = AX_VIN_StopPipe(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_StopPipe failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_StopChn(AX_S32 nPipeId, AX_S32 nChnId) {
    AX_S32 nRet = 0;
    nRet = AX_VIN_DisableChn(nPipeId, (AX_VIN_CHN_ID_E)nChnId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_DisableChn failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_DestroyDev(AX_S32 nDevId) {
    AX_S32 nRet = 0;
    nRet = AX_VIN_DestroyDev(nDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_DestroyDev failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_VIN_DestroyPipe(AX_S32 nPipeId) {
    AX_S32 nRet = 0;
    nRet = AX_VIN_DestroyPipe(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_DestroyPipe failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}
