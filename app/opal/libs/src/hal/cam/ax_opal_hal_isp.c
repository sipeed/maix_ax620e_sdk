/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_isp.h"
#include "ax_opal_log.h"

#define LOG_TAG "HAL_ISP"

AX_S32 AX_OPAL_HAL_ISP_Create(AX_S32 nPipeId, AX_VOID* pSensorHandle, AX_OPAL_CAM_ISP_CFG_T* ptIspCfg, AX_OPAL_SNS_MODE_E eSnsMode) {
    AX_S32 nRet = 0;

    AX_SENSOR_REGISTER_FUNC_T* ptSnsHdl = (AX_SENSOR_REGISTER_FUNC_T*)pSensorHandle;

    /* Step 6.1. AX_ISP_Create */
    nRet = AX_ISP_Create(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_Create failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 6.2. ISP AE Alg register */
    {
        AX_ISP_AE_REGFUNCS_T tAeFuncs = {0};
        tAeFuncs.pfnAe_Init = AX_ISP_ALG_AeInit;
        tAeFuncs.pfnAe_Exit = AX_ISP_ALG_AeDeInit;
        tAeFuncs.pfnAe_Run  = AX_ISP_ALG_AeRun;
        /* Register the sensor driven interface TO the AE library */
        nRet = AX_ISP_ALG_AeRegisterSensor(nPipeId, ptSnsHdl);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_ISP_ALG_AeRegisterSensor failed, ret=0x%x", nRet);
            return -1;
        }

        /* Register AE alg TO ISP firmware*/
        nRet = AX_ISP_RegisterAeLibCallback(nPipeId, &tAeFuncs);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_ISP_RegisterAeLibCallback failed, ret=0x%x", nRet);
            return -1;
        }
    }

    /* Step 6.3. ISP AWB Alg register */
    {
        AX_ISP_AWB_REGFUNCS_T tAwbFuncs = {0};
        tAwbFuncs.pfnAwb_Init = AX_ISP_ALG_AwbInit;
        tAwbFuncs.pfnAwb_Exit = AX_ISP_ALG_AwbDeInit;
        tAwbFuncs.pfnAwb_Run  = AX_ISP_ALG_AwbRun;

        /* Register the sensor driven interface TO the AWB library */
        nRet = AX_ISP_ALG_AwbRegisterSensor(nPipeId, ptSnsHdl);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_ISP_ALG_AwbRegisterSensor failed, ret=0x%x", nRet);
            return -1;
        }

        /* Register AWB alg TO ISP firmware*/
        nRet = AX_ISP_RegisterAwbLibCallback(nPipeId, &tAwbFuncs);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "AX_ISP_RegisterAwbLibCallback failed, ret=0x%x.", nRet);
            return -1;
        }
    }

    // TODO: Step 6.4. ISP AF Alg register

    /* Step 6.5. AX_ISP_LoadBinParams */
    // if (ptIspCfg->nBinCnt != 0) {
    char *sub = "null.bin";
    char *result = AX_NULL;
    if (eSnsMode == AX_OPAL_SNS_HDR_MODE) {
        result = strstr(ptIspCfg->cBinPathName[1], sub);
        if (result == NULL) {
            nRet = AX_ISP_LoadBinParams(nPipeId, ptIspCfg->cBinPathName[1]);
            if (0 != nRet) {
                LOG_M_E(LOG_TAG, "AX_ISP_LoadBinParams failed, ret=0x%x.", nRet);
                return -1;
            }
        }
    } else {
        result = strstr(ptIspCfg->cBinPathName[0], sub);
        if (result == NULL) {
            nRet = AX_ISP_LoadBinParams(nPipeId, ptIspCfg->cBinPathName[0]);
            if (0 != nRet) {
                LOG_M_E(LOG_TAG, "AX_ISP_LoadBinParams failed, ret=0x%x.", nRet);
                return -1;
            }
        }
    }

    /* Step 6.6. AX_ISP_Open */
    nRet = AX_ISP_Open(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_Open failed, ret=0x%x", nRet);
        return -1;
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_ISP_Start(AX_S32 nPipeId) {
    AX_S32 nRet = 0;

    /* Step 8.2. AX_ISP_Start */
    nRet = AX_ISP_Start(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_Start failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 8.4. AX_ISP_StreamOn */
    nRet = AX_ISP_StreamOn(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_StreamOn failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_ISP_Stop(AX_S32 nPipeId) {
    AX_S32 nRet = 0;
    /* Step 1.1. AX_ISP_StreamOff */
    nRet = AX_ISP_StreamOff(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_StreamOff failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 1.3. AX_ISP_Stop */
    nRet = AX_ISP_Stop(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_Stop failed, ret=0x%x", nRet);
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_ISP_Destroy(AX_S32 nPipeId) {
    AX_S32 nRet = 0;

    nRet = AX_ISP_Close(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_Close failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_ISP_ALG_AeUnRegisterSensor(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_ALG_AeUnRegisterSensor failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_ISP_UnRegisterAeLibCallback(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_UnRegisterAeLibCallback failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_ISP_ALG_AwbUnRegisterSensor(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_ALG_AwbUnRegisterSensor failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_ISP_UnRegisterAwbLibCallback(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_UnRegisterAwbLibCallback failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_ISP_Destroy(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_Destroy failed, ret=0x%x.\n", nRet);
        return -1;
    }

    return nRet;
}

