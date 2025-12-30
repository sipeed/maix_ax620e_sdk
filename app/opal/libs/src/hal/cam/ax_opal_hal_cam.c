/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_cam.h"

#include "ax_opal_hal_isp.h"
#include "ax_opal_hal_nt.h"
#include "ax_opal_hal_sns.h"
#include "ax_opal_hal_vin.h"
#include "ax_opal_hal_sps.h"
#include "ax_opal_hal_hnb.h"


#include "ax_opal_log.h"
#define LOG_TAG ("HAL_CAM")

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

#define AX_EZOOM_MAX  8

extern AX_S32 cvt_value2iq(AX_F32 fVal, AX_S32 nLow, AX_S32 nMedium, AX_S32 nHigh);
extern AX_F32 cvt_iq2value(AX_S32 nIQ, AX_S32 nLow, AX_S32 nMedium, AX_S32 nHigh);

static AX_OPAL_SNS_COLOR_ATTR_T g_tDefaultColorAttr[AX_OPAL_SNS_ID_BUTT];

AX_S32 AX_OPAL_HAL_CAM_Init(AX_VOID) {
    AX_S32 nRet = 0;

    nRet = AX_VIN_Init();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_Init failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_MIPI_RX_Init();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_MIPI_RX_Init failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_SPS_Init();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_SPS_Init failed, ret=0x%x.", nRet);
        return -1;
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_CAM_Deinit(AX_VOID) {
    AX_S32 nRet = 0;

    nRet = AX_OPAL_HAL_SPS_Deinit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_SPS_Deinit failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_MIPI_RX_DeInit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_MIPI_RX_DeInit failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_VIN_Deinit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_DeInit failed, ret=0x%x.", nRet);
        return -1;
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_CAM_Open(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_MAL_CAM_ATTR_T *ptCamAttr) {

    AX_S32 nRet = 0;

    AX_S32 nRxDevId = ptCamId->nRxDevId;
    AX_S32 nDevId = ptCamId->nDevId;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = (AX_VIN_CHN_ID_E)ptCamId->nChnId;
    AX_SENSOR_REGISTER_FUNC_T* ptSnsHdl = (AX_SENSOR_REGISTER_FUNC_T*)ptCamAttr->tSnsLibAttr.pSensorHandle;

    AX_OPAL_CAM_MIPI_CFG_T* ptMipiCfg = &ptCamAttr->tCamCfg.tMipiCfg;
    AX_OPAL_CAM_DEV_CFG_T* ptDevCfg = &ptCamAttr->tCamCfg.tDevCfg;
    AX_OPAL_CAM_PIPE_CFG_T* ptPipeCfg = &ptCamAttr->tCamCfg.tPipeCfg;
    AX_OPAL_CAM_SENSOR_CFG_T* ptSnsCfg = &ptCamAttr->tCamCfg.tSensorCfg;
    AX_OPAL_CAM_ISP_CFG_T* ptIspCfg = &ptCamAttr->tCamCfg.tPipeCfg.stCamIspCfg;
    AX_OPAL_CAM_CHN_CFG_T* ptChnCfg = &ptCamAttr->tCamCfg.tChnCfg;

    /* Step 2. Mipi */
    nRet = AX_OPAL_HAL_VIN_CreateMipi(nRxDevId, nDevId, nPipeId, ptSnsHdl, ptMipiCfg);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_CreateMipi failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 3. Dev */
    nRet = AX_OPAL_HAL_VIN_CreateDev(nRxDevId, nDevId, nPipeId, ptMipiCfg->eLaneNum, ptDevCfg);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_CreateDev failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 4. Pipe */
    nRet = AX_OPAL_HAL_VIN_CreatePipe(nPipeId, ptPipeCfg);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_CreatePipe failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 5. Sensor */
    nRet = AX_OPAL_HAL_SNS_Create(nPipeId, ptSnsHdl, ptSnsCfg);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_SNS_Create failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 6. ISP */
    nRet = AX_OPAL_HAL_ISP_Create(nPipeId, ptSnsHdl, ptIspCfg, ptSnsCfg->eSnsMode);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_ISP_Create failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 7. CHN */
    nRet = AX_OPAL_HAL_VIN_CreateChn(nPipeId, eChnId, ptChnCfg);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_CreateChn failed, ret=0x%x.", nRet);
        return -1;
    }

    if (ptPipeCfg->stCamNTCfg.bEnable) {
        nRet = AX_NT_CtrlInit(ptPipeCfg->stCamNTCfg.nCtrlPort);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_NT_CtrlInit failed, ret=0x%x.", nRet);
            return -1;
        }

        nRet =  AX_NT_StreamInit(ptPipeCfg->stCamNTCfg.nStreamPort);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_NT_CtrlInit failed, ret=0x%x.", nRet);
            return -1;
        }

        nRet = AX_NT_SetStreamSource(nPipeId);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_NT_SetStreamSource failed, ret=0x%x.", nRet);
            return -1;
        }
    }

    // get default color attribute
    if (nPipeId < AX_OPAL_SNS_ID_BUTT) {
        // set default to manual and then get from iq
        g_tDefaultColorAttr[nPipeId].bManual = AX_TRUE;

        AX_OPAL_VIDEO_SNS_ATTR_T tDefAttr;
        memset(&tDefAttr, 0x00, sizeof(tDefAttr));
        nRet = AX_OPAL_HAL_CAM_GetColor(ptCamId, &tDefAttr);
        if (nRet == 0) {
            g_tDefaultColorAttr[nPipeId] = tDefAttr.stColorAttr;
        }
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_Close(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_MAL_CAM_ATTR_T *ptCamAttr) {
    AX_S32 nRet = 0;

    AX_S32 nDevId = ptCamId->nDevId;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_OPAL_CAM_SENSOR_CFG_T* ptSnsCfg = &ptCamAttr->tCamCfg.tSensorCfg;
    AX_OPAL_CAM_PIPE_CFG_T* ptPipeCfg = &ptCamAttr->tCamCfg.tPipeCfg;

    nRet = AX_OPAL_HAL_ISP_Destroy(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_ISP_Destroy failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_SNS_Destroy(nPipeId, ptSnsCfg);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_SNS_Destroy failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_VIN_DestroyPipe(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_DestroyPipe failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_VIN_DestroyDev(nDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_DestroyDev failed, ret=0x%x.", nRet);
        return -1;
    }

    if (ptPipeCfg->stCamNTCfg.bEnable) {
        nRet = AX_NT_CtrlDeInit();
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_NT_CtrlDeInit failed, ret=0x%x.", nRet);
            return -1;
        }

        nRet =  AX_NT_StreamDeInit();
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_NT_StreamDeInit failed, ret=0x%x.", nRet);
            return -1;
        }
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_Start(const AX_OPAL_MAL_CAM_ID_T* ptCamId) {

    AX_S32 nRet = 0;

    AX_S32 nRxDevId = ptCamId->nRxDevId;
    AX_S32 nDevId = ptCamId->nDevId;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = (AX_VIN_CHN_ID_E)ptCamId->nChnId;

    /* Step 8. Start */
    /* Step 2.5. AX_MIPI_RX_Start */
    nRet = AX_OPAL_HAL_VIN_StartMipi(nRxDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_StartMipi failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 7.3. AX_VIN_EnableChn */
    nRet = AX_OPAL_HAL_VIN_StartChn(nPipeId, eChnId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_StartChn failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 8.1. AX_VIN_StartPipe */
    nRet = AX_OPAL_HAL_VIN_StartPipe(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_StartPipe failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 8.3. AX_VIN_EnableDev */
    nRet = AX_OPAL_HAL_VIN_StartDev(nDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_StartDev failed, ret=0x%x.", nRet);
        return -1;
    }
    /* Step 8.2. AX_ISP_Start */
    /* Step 8.4. AX_ISP_StreamOn */
    nRet = AX_OPAL_HAL_ISP_Start(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_ISP_Start failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_SPS_Start(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_SPS_Start failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_HNB_Start(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_HNB_Start failed, ret=0x%x.", nRet);
        return -1;
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_Stop(const AX_OPAL_MAL_CAM_ID_T* ptCamId) {

    AX_S32 nRet = 0;

    AX_S32 nRxDevId = ptCamId->nRxDevId;
    AX_S32 nDevId = ptCamId->nDevId;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = (AX_VIN_CHN_ID_E)ptCamId->nChnId;

    nRet = AX_OPAL_HAL_SPS_Stop(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_SPS_Stop failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_HNB_Stop(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_HNB_Stop failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_ISP_Stop(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_ISP_Stop failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_VIN_StopDev(nDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_StopDev failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_VIN_StopPipe(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_StopPipe failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_VIN_StopChn(nPipeId, eChnId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_StopChn failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_OPAL_HAL_VIN_StopMipi(nRxDevId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_OPAL_HAL_VIN_StopMipi failed, ret=0x%x.", nRet);
        return -1;
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetRotation(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = AX_SUCCESS;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = ptCamId->nChnId;
    nRet = AX_VIN_GetChnRotation(nPipeId, eChnId, (AX_VIN_ROTATION_E*)&pData->eRotation);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnRotation failed, dev=%d pipe=%d chn=%d ret=0x%x.", ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }
    return (AX_S32)nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetRotation(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");

    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = ptCamId->nChnId;
    AX_VIN_ROTATION_E eValue = AX_OPAL_SNS_ROTATION_0;
    nRet = AX_VIN_GetChnRotation(nPipeId, eChnId, &eValue);
    if (AX_SUCCESS == nRet && (AX_VIN_ROTATION_E)pData->eRotation == eValue) {
        LOG_M_W(LOG_TAG, "current rotation state %d is the same as input state %d, dev=%d pipe=%d chn=%d.",
                eValue, pData->eRotation, ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId);
    } else if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnRotation failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }
    nRet = AX_VIN_SetChnRotation(nPipeId, eChnId, (AX_VIN_ROTATION_E)pData->eRotation);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetChnRotation failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    } else {
        // if (pData->stEZoomAttr.nRatio != 0) {
        //     AX_OPAL_HAL_CAM_SetEZoom(ptCamId, pData);
        // }
        LOG_M_D(LOG_TAG, "AX_VIN_SetChnRotation %d success, dev=%d pipe=%d chn=%d ret=0x%x.",
                pData->eRotation, ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }
    LOG_M_D(LOG_TAG, "---");
    return (AX_S32)nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetMirror(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = AX_SUCCESS;
    AX_S32 nPipeId = ptCamId->nPipeId;
    nRet = AX_VIN_GetPipeMirror(nPipeId, (AX_BOOL*)&pData->bMirror);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetPipeMirror failed, dev=%d pipe=%d ret=0x%x.", ptCamId->nDevId, ptCamId->nPipeId, nRet);
    }
    return (AX_S32)nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetMirror(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = AX_SUCCESS;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_BOOL bValue = AX_FALSE;
    nRet = AX_VIN_GetPipeMirror(nPipeId, &bValue);
    if (AX_SUCCESS == nRet && (AX_BOOL)pData->bMirror == bValue) {
        LOG_M_W(LOG_TAG, "current mirror state %d is the same as input state %d, dev=%d pipe=%d.",
                bValue, pData->bMirror, ptCamId->nDevId, ptCamId->nPipeId);
    } else if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetPipeMirror failed, dev=%d pipe=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, nRet);
    }
    nRet = AX_VIN_SetPipeMirror(nPipeId, (AX_BOOL)pData->bMirror);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetPipeMirror failed, dev=%d pipe=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, nRet);
    }
    return (AX_S32)nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetFlip(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = AX_SUCCESS;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = ptCamId->nChnId;
    nRet = AX_VIN_GetChnFlip(nPipeId, eChnId, (AX_BOOL*)&pData->bFlip);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnFlip failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                    ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }
    return (AX_S32)nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetFlip(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = AX_SUCCESS;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = ptCamId->nChnId;
    AX_BOOL bValue = AX_FALSE;
    nRet = AX_VIN_GetChnFlip(nPipeId, eChnId, &bValue);
    if (AX_SUCCESS == nRet && (AX_BOOL)pData->bFlip == bValue) {
        LOG_M_W(LOG_TAG, "current flip state %d is the same as input state %d, dev=%d pipe=%d chn=%d.",
                bValue, pData->bFlip, ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId);
    } else if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnFlip failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }
    nRet = AX_VIN_SetChnFlip(nPipeId, eChnId, (AX_BOOL)pData->bFlip);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetChnFlip failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }
    return (AX_S32)nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetDayNight(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = ptCamId->nChnId;
    nRet = AX_VIN_GetChnDayNightMode(nPipeId, eChnId, (AX_DAYNIGHT_MODE_E*)&pData->eDayNight);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnDayNightMode failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                    ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }
    return (AX_S32)nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetDayNight(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = ptCamId->nChnId;
    AX_DAYNIGHT_MODE_E eValue = AX_DAYNIGHT_MODE_DAY;
    nRet = AX_VIN_GetChnDayNightMode(nPipeId, eChnId, &eValue);
    if (AX_SUCCESS == nRet && eValue == (AX_DAYNIGHT_MODE_E)pData->eDayNight) {
        return nRet;
    } else if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetPipeMirror failed, dev=%d pipe=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, nRet);
    }
    nRet = AX_VIN_SetChnDayNightMode(nPipeId, eChnId, (AX_DAYNIGHT_MODE_E)pData->eDayNight);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_SetChnDayNightMode failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }
    return (AX_S32)nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetFps(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    AX_S32 nPipeId = ptCamId->nPipeId;

    AX_SNS_ATTR_T tSnsAttr;
    memset(&tSnsAttr, 0x0, sizeof(AX_SNS_ATTR_T));
    nRet = AX_ISP_GetSnsAttr(nPipeId, &tSnsAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_GetSnsAttr failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }
    pData->fFrameRate = tSnsAttr.fFrameRate;
    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetFps(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;

    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_SNS_ATTR_T tSnsAttr;
    memset(&tSnsAttr, 0x0, sizeof(AX_SNS_ATTR_T));
    nRet = AX_ISP_GetSnsAttr(nPipeId, &tSnsAttr);
    if (AX_SUCCESS == nRet && tSnsAttr.fFrameRate == pData->fFrameRate) {
        LOG_M_W(LOG_TAG, "current sensor fps %f is the same as input %f, dev=%d pipe=%d chn=%d.",
                tSnsAttr.fFrameRate, pData->fFrameRate, ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId);
    } else if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_GetSnsAttr failed, dev=%d pipe=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, nRet);
    }

    tSnsAttr.fFrameRate = pData->fFrameRate;
    nRet = AX_ISP_SetSnsAttr(nPipeId, &tSnsAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_SetSnsAttr failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetColor(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_OPAL_SNS_COLOR_ATTR_T* pColorAttr = &pData->stColorAttr;

    /* sharpness */
    AX_ISP_IQ_SHARPEN_PARAM_T tShpParam;
    memset(&tShpParam, 0x0, sizeof(AX_ISP_IQ_SHARPEN_PARAM_T));
    nRet = AX_ISP_IQ_GetShpParam(nPipeId, &tShpParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetShpParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }
    pColorAttr->fSharpness = cvt_iq2value(tShpParam.tManualParam.nShpGain[0], SENSOR_SHARPNESS_LOW, SENSOR_SHARPNESS_MEDIUM, SENSOR_SHARPNESS_HIGH);
    pColorAttr->bManual = (tShpParam.nAutoMode == 0) ? AX_TRUE : AX_FALSE;

    /* ycproc */
    AX_ISP_IQ_YCPROC_PARAM_T tYcprocParam;
    memset(&tYcprocParam, 0x0, sizeof(AX_ISP_IQ_YCPROC_PARAM_T));
    nRet = AX_ISP_IQ_GetYcprocParam(nPipeId, &tYcprocParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetShpParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }
    pColorAttr->fBrightness = cvt_iq2value(tYcprocParam.nBrightness, SENSOR_BRIGHTNESS_LOW, SENSOR_BRIGHTNESS_MEDIUM, SENSOR_BRIGHTNESS_HIGH);
    pColorAttr->fContrast = cvt_iq2value(tYcprocParam.nContrast, SENSOR_CONTRAST_LOW, SENSOR_CONTRAST_MEDIUM, SENSOR_CONTRAST_HIGH);
    pColorAttr->fSaturation = cvt_iq2value(tYcprocParam.nSaturation, SENSOR_SATURATION_LOW, SENSOR_SATURATION_MEDIUM, SENSOR_SATURATION_HIGH);
    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetColor(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

    AX_S32 nPipeId = ptCamId->nPipeId;
    // AX_VIN_CHN_ID_E eChnId = ptCamId->nChnId;
    AX_OPAL_SNS_COLOR_ATTR_T* pColorAttr = &pData->stColorAttr;

    if (!pColorAttr->bManual
        && nPipeId < AX_OPAL_SNS_ID_BUTT
        && !g_tDefaultColorAttr[nPipeId].bManual) {
        pColorAttr = &g_tDefaultColorAttr[nPipeId];
    }

    /* sharpness */
    AX_S32 nShpGain = cvt_value2iq(pColorAttr->fSharpness, SENSOR_SHARPNESS_LOW, SENSOR_SHARPNESS_MEDIUM, SENSOR_SHARPNESS_HIGH);
    AX_ISP_IQ_SHARPEN_PARAM_T tShpParam;
    memset(&tShpParam, 0x0, sizeof(AX_ISP_IQ_SHARPEN_PARAM_T));
    nRet = AX_ISP_IQ_GetShpParam(nPipeId, &tShpParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetShpParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }
    tShpParam.nAutoMode = pColorAttr->bManual ? 0 : 1;
    tShpParam.tManualParam.nShpGain[0] = nShpGain;
    tShpParam.tManualParam.nShpGain[1] = nShpGain;
    nRet = AX_ISP_IQ_SetShpParam(nPipeId, &tShpParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_SetShpParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    /* ycproc */
    AX_S32 nBrightness = cvt_value2iq(pColorAttr->fBrightness, SENSOR_BRIGHTNESS_LOW, SENSOR_BRIGHTNESS_MEDIUM, SENSOR_BRIGHTNESS_HIGH);
    AX_S32 nContrast = cvt_value2iq(pColorAttr->fContrast, SENSOR_CONTRAST_LOW, SENSOR_CONTRAST_MEDIUM, SENSOR_CONTRAST_HIGH);
    AX_S32 nSaturation = cvt_value2iq(pColorAttr->fSaturation, SENSOR_SATURATION_LOW, SENSOR_SATURATION_MEDIUM, SENSOR_SATURATION_HIGH);
    AX_ISP_IQ_YCPROC_PARAM_T tYcprocParam;
    memset(&tYcprocParam, 0x0, sizeof(AX_ISP_IQ_YCPROC_PARAM_T));
    nRet = AX_ISP_IQ_GetYcprocParam(nPipeId, &tYcprocParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetYcprocParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }
    tYcprocParam.nYCprocEn = AX_TRUE;
    tYcprocParam.nBrightness = nBrightness;
    tYcprocParam.nContrast = nContrast;
    tYcprocParam.nSaturation = nSaturation;
    nRet = AX_ISP_IQ_SetYcprocParam(nPipeId, &tYcprocParam);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_SetYcprocParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetMode(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    LOG_M_E(LOG_TAG, "Switch SNS Mode not support.");
    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetMode(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    LOG_M_E(LOG_TAG, "Switch SNS Mode not support.");
    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetLdc(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {

    AX_S32 nRet = 0;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_OPAL_SNS_LDC_ATTR_T* pLdcAttr = &pData->stLdcAttr;

    AX_ISP_IQ_LDC_PARAM_T tLdcAttr;
    memset(&tLdcAttr, 0x0, sizeof(AX_ISP_IQ_LDC_PARAM_T));
    nRet = AX_ISP_IQ_GetLdcParam(nPipeId, &tLdcAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetLdcParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    pLdcAttr->bEnable = tLdcAttr.nLdcEnable;
    pLdcAttr->bAspect = tLdcAttr.tLdcV1Param.bAspect;
    pLdcAttr->nXRatio = tLdcAttr.tLdcV1Param.nXRatio;
    pLdcAttr->nYRatio = tLdcAttr.tLdcV1Param.nYRatio;
    pLdcAttr->nXYRatio = tLdcAttr.tLdcV1Param.nXYRatio;
    pLdcAttr->nDistortionRatio = tLdcAttr.tLdcV1Param.nDistortionRatio;

    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetLdc(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_OPAL_SNS_LDC_ATTR_T* pLdcAttr = &pData->stLdcAttr;

    AX_ISP_IQ_LDC_PARAM_T tLdcAttr;
    memset(&tLdcAttr, 0x0, sizeof(AX_ISP_IQ_LDC_PARAM_T));
    nRet = AX_ISP_IQ_GetLdcParam(nPipeId, &tLdcAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetLdcParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    tLdcAttr.nLdcEnable = pLdcAttr->bEnable,
    tLdcAttr.nType = AX_ISP_IQ_LDC_TYPE_V1,
    tLdcAttr.tLdcV1Param.bAspect = pLdcAttr->bAspect;
    tLdcAttr.tLdcV1Param.nXRatio = pLdcAttr->nXRatio;
    tLdcAttr.tLdcV1Param.nYRatio = pLdcAttr->nYRatio;
    tLdcAttr.tLdcV1Param.nXYRatio = pLdcAttr->nXYRatio;
    tLdcAttr.tLdcV1Param.nCenterXOffset = 0;
    tLdcAttr.tLdcV1Param.nCenterYOffset = 0;
    tLdcAttr.tLdcV1Param.nDistortionRatio = pLdcAttr->nDistortionRatio;
    tLdcAttr.tLdcV1Param.nSpreadCoef = 0;
    memset(&tLdcAttr.tLdcV2Param, 0x0, sizeof(AX_ISP_IQ_LDC_V2_PARAM_T));
    tLdcAttr.tLdcV2Param.nMatrix[2][2] = 1;

    nRet = AX_ISP_IQ_SetLdcParam(nPipeId, &tLdcAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_SetLdcParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetEZoom(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    // return the saved nEZoomRatio value
    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetEZoom(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

    AX_OPAL_SNS_EZOOM_ATTR_T* pEZoomAttr = &pData->stEZoomAttr;
    AX_F32 fEZoomRatio = pEZoomAttr->fRatio;
    if (fEZoomRatio < 1 || fEZoomRatio > AX_EZOOM_MAX) {
        return -1;
    }

    AX_S32 nPipeId = ptCamId->nPipeId;
    AX_VIN_CHN_ID_E eChnId = ptCamId->nChnId;

    AX_VIN_CROP_INFO_T tCropInfo;
    memset(&tCropInfo, 0x0, sizeof(AX_VIN_CROP_INFO_T));
    nRet = AX_VIN_GetChnCrop(nPipeId, eChnId, &tCropInfo);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnCrop failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }
    AX_VIN_ROTATION_E eRotation = AX_VIN_ROTATION_BUTT;
    nRet = AX_VIN_GetChnRotation(nPipeId, eChnId, &eRotation);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnRotation failed, dev=%d pipe=%d chn=%d ret=0x%x.",
            ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    AX_VIN_CHN_ATTR_T tChnAttr;
    memset(&tChnAttr, 0x0, sizeof(AX_VIN_CHN_ATTR_T));
    nRet = AX_VIN_GetChnAttr(nPipeId, eChnId, &tChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnAttr failed, dev=%d pipe=%d chn=%d ret=0x%x.",
            ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }
    AX_U32 nWidth = tChnAttr.nWidth;
    AX_U32 nHeight = tChnAttr.nHeight;

    if (AX_VIN_ROTATION_90 == eRotation || AX_VIN_ROTATION_270 == eRotation) {
        nWidth = tChnAttr.nHeight;
        nHeight = tChnAttr.nWidth;
    }

    if (fEZoomRatio == 1) {
        tCropInfo.bEnable = AX_FALSE;
        tCropInfo.eCoordMode = AX_VIN_COORD_ABS;
        tCropInfo.tCropRect.nWidth = nWidth;
        tCropInfo.tCropRect.nHeight = nHeight;
        tCropInfo.tCropRect.nStartX = 0;
        tCropInfo.tCropRect.nStartY = 0;
        nRet = AX_VIN_SetChnCrop(nPipeId, eChnId, &tCropInfo);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_VIN_SetChnCrop failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
            return nRet;
        }
    } else {

        tCropInfo.bEnable = AX_TRUE;
        tCropInfo.eCoordMode = AX_VIN_COORD_ABS;
        tCropInfo.tCropRect.nWidth = nWidth / fEZoomRatio;
        tCropInfo.tCropRect.nHeight = nHeight / fEZoomRatio;
        tCropInfo.tCropRect.nStartX = (nWidth - tCropInfo.tCropRect.nWidth) / 2;
        tCropInfo.tCropRect.nStartY = (nHeight - tCropInfo.tCropRect.nHeight) / 2;
        nRet = AX_VIN_SetChnCrop(nPipeId, eChnId, &tCropInfo);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_VIN_SetChnCrop failed, dev=%d pipe=%d chn=%d ret=0x%x.",
                ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
            return nRet;
        }
    }
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_GetDis(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;

    AX_OPAL_SNS_DIS_ATTR_T* pDisAttr = &pData->stDisAttr;
    AX_S32 nPipeId = ptCamId->nPipeId;

    AX_ISP_IQ_DIS_PARAM_T tDisAttr;
    memset(&tDisAttr, 0x0, sizeof(AX_ISP_IQ_DIS_PARAM_T));
    nRet = AX_ISP_IQ_GetDisParam(nPipeId, &tDisAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetDisParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
            ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    pDisAttr->bEnable = tDisAttr.bDisEnable;
    pDisAttr->nDelayFrameNum = tDisAttr.tDisV1Param.nDelayFrameNum;
    // pDisAttr->bMotionEst, no such parameter
    // pDisAttr->bMotionShare, no such parameter
    return nRet;
}

AX_S32 AX_OPAL_HAL_CAM_SetDis(const AX_OPAL_MAL_CAM_ID_T* ptCamId, AX_OPAL_VIDEO_SNS_ATTR_T* pData) {
    AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

    AX_OPAL_SNS_DIS_ATTR_T* pDisAttr = &pData->stDisAttr;
    AX_S32 nPipeId = ptCamId->nPipeId;

    AX_ISP_IQ_DIS_PARAM_T tDisAttr;
    memset(&tDisAttr, 0x0, sizeof(AX_ISP_IQ_DIS_PARAM_T));
    nRet = AX_ISP_IQ_GetDisParam(nPipeId, &tDisAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetDisParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
            ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    tDisAttr.bDisEnable = pDisAttr->bEnable;
    tDisAttr.tDisV1Param.nDelayFrameNum = pDisAttr->nDelayFrameNum;
    nRet = AX_ISP_IQ_SetDisParam(nPipeId, &tDisAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_SetDisParam failed, dev=%d pipe=%d chn=%d ret=0x%x.",
            ptCamId->nDevId, ptCamId->nPipeId, ptCamId->nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 cvt_value2iq(AX_F32 fVal, AX_S32 nLow, AX_S32 nMedium, AX_S32 nHigh) {
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

AX_F32 cvt_iq2value(AX_S32 nIQ, AX_S32 nLow, AX_S32 nMedium, AX_S32 nHigh) {
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
