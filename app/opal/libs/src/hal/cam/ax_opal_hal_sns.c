
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_sns.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "ax_opal_log.h"
#define LOG_TAG  ("HAL_SNS")

#ifdef USING_OPAL_STATIC_FLAG
#ifdef OS04A10_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnsos04a10Obj;
#endif
#ifdef SC450AI_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnssc450aiObj;
#endif
#ifdef IMX678_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnsimx678Obj;
#endif
#ifdef SC200AI_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnssc200aiObj;
#endif
#ifdef SC500AI_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnssc500aiObj;
#endif
#ifdef SC850SL_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnssc850slObj;
#endif
#ifdef C4395_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnsc4395Obj;
#endif
#ifdef GC4653_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnsGc4653Obj;
#endif
#ifdef MIS2032_SUPPORT
extern AX_SENSOR_REGISTER_FUNC_T gSnsmis2032Obj;
#endif

typedef struct {
    char name[32];
    AX_SENSOR_REGISTER_FUNC_T *objHandle;
} objHandleMap;

objHandleMap objSnsHandleArr[] = {
#ifdef OS04A10_SUPPORT
    {"gSnsos04a10Obj", &gSnsos04a10Obj},
#endif
#ifdef SC450AI_SUPPORT
    {"gSnssc450aiObj", &gSnssc450aiObj},
#endif
#ifdef IMX678_SUPPORT
    {"gSnsimx678Obj", &gSnsimx678Obj},
#endif
#ifdef SC200AI_SUPPORT
    {"gSnssc200aiObj", &gSnssc200aiObj},
#endif
#ifdef SC500AI_SUPPORT
    {"gSnssc500aiObj", &gSnssc500aiObj},
#endif
#ifdef SC850SL_SUPPORT
    {"gSnssc850slObj", &gSnssc850slObj},
#endif
#ifdef C4395_SUPPORT
    {"gSnsc4395Obj", &gSnsc4395Obj},
#endif
#ifdef GC4653_SUPPORT
    {"gSnsGc4653Obj", &gSnsGc4653Obj},
#endif
#ifdef MIS2032_SUPPORT
    {"gSnsmis2032Obj", &gSnsmis2032Obj},
#endif
};
#endif

AX_S32 AX_OPAL_HAL_SNS_Init(const AX_CHAR* pDriverFileName, const AX_CHAR* pSensorObjName, AX_OPAL_MAL_SNSLIB_ATTR_T* ptSnsLibAttr) {
    AX_S32 nRet = 0;
    /* Step 1. Sesnor Driver */
#ifndef USING_OPAL_STATIC_FLAG
    /* Step 1.1. dlopen Sensor Driver */
    ptSnsLibAttr->pSensorLib = dlopen(pDriverFileName, RTLD_LAZY);
    if (AX_NULL == ptSnsLibAttr->pSensorLib) {
        LOG_M_E(LOG_TAG, "Sensor Driver %s dlopen failed, err=%s.", pDriverFileName, dlerror());
        return -1;
    }

    /* Step 1.2. dlsym Sensor Driver */
    ptSnsLibAttr->pSensorHandle = dlsym(ptSnsLibAttr->pSensorLib, pSensorObjName);
    if (AX_NULL == ptSnsLibAttr->pSensorHandle) {
        LOG_M_E(LOG_TAG, "Sensor symbols %s dlsym failed, err=%s.", pSensorObjName, dlerror());
        return -1;
    }
#else
    ptSnsLibAttr->pSensorLib = AX_NULL;

    for (int i = 0; i < (AX_S32)(sizeof(objSnsHandleArr)/sizeof(objSnsHandleArr[0])); i++) {
        if (0 == strncmp(objSnsHandleArr[i].name, pSensorObjName, strlen(objSnsHandleArr[i].name))) {
            ptSnsLibAttr->pSensorHandle = (AX_VOID *)objSnsHandleArr[i].objHandle;
            if (!ptSnsLibAttr->pSensorHandle) {
                LOG_M_E(LOG_TAG, "Sensor symbols %s static failed.", pSensorObjName);
                return -1;
            }
            break;
        }
    }
#endif
    return nRet;
}

AX_S32 AX_OPAL_HAL_SNS_Deinit(AX_OPAL_MAL_SNSLIB_ATTR_T* ptSnsLibAttr) {
#ifndef USING_OPAL_STATIC_FLAG
    dlclose(ptSnsLibAttr->pSensorLib);
#endif
    ptSnsLibAttr->pSensorLib = AX_NULL;
    ptSnsLibAttr->pSensorHandle = AX_NULL;
    return 0;
}

AX_S32 AX_OPAL_HAL_SNS_Create(AX_S32 nPipeId, AX_VOID* pSensorHandle, AX_OPAL_CAM_SENSOR_CFG_T* ptSnsCfg) {
    AX_S32 nRet = 0;

    AX_SENSOR_REGISTER_FUNC_T* ptSnsHdl = (AX_SENSOR_REGISTER_FUNC_T*)pSensorHandle;

    AX_SNS_ATTR_T tSnsAttr;
    memset(&tSnsAttr, 0x0, sizeof(AX_SNS_ATTR_T));
    tSnsAttr.nWidth = ptSnsCfg->nWidth;
    tSnsAttr.nHeight = ptSnsCfg->nHeight;
    tSnsAttr.fFrameRate = (AX_F32)ptSnsCfg->fFrameRate;
    tSnsAttr.eSnsMode = (AX_SNS_HDR_MODE_E)ptSnsCfg->eSnsMode;
    tSnsAttr.eRawType = (AX_RAW_TYPE_E)ptSnsCfg->eRawType;
    tSnsAttr.eBayerPattern = (AX_BAYER_PATTERN_E)ptSnsCfg->eBayerPattern;
    tSnsAttr.bTestPatternEnable = (AX_BOOL)ptSnsCfg->bTestPatternEnable;
    tSnsAttr.eMasterSlaveSel = (AX_SNS_MASTER_SLAVE_E)ptSnsCfg->eMasterSlaveSel;

    if (AX_SNS_LINEAR_MODE == tSnsAttr.eSnsMode) {
        tSnsAttr.nSettingIndex = ptSnsCfg->nSettingIndex[0];
    } else if (AX_SNS_HDR_2X_MODE == tSnsAttr.eSnsMode) {
        tSnsAttr.nSettingIndex = ptSnsCfg->nSettingIndex[1];
    } else {
        tSnsAttr.nSettingIndex = ptSnsCfg->nSettingIndex[0];
    }

    /* Step 5.1. Sensor Register */
    AX_SNS_COMMBUS_T tSnsBusInfo = {0};;
    nRet = AX_ISP_RegisterSensor(nPipeId, ptSnsHdl);
    if (nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_RegisterSensor failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 5.2. Sensor Set Bus Info */
    {
        /* confige i2c/spi dev id */
        if (ptSnsCfg->nDevNode == -1) {
            if (ISP_SNS_CONNECT_I2C_TYPE == ptSnsCfg->nBusType) {
                tSnsBusInfo.I2cDev = 0;
                tSnsBusInfo.busType = ISP_SNS_CONNECT_I2C_TYPE;

            } else {
                tSnsBusInfo.SpiDev.bit4SpiDev = 0;
                tSnsBusInfo.SpiDev.bit4SpiCs  = 0;
                tSnsBusInfo.busType = ISP_SNS_CONNECT_SPI_TYPE;
            }
        } else {
            tSnsBusInfo.I2cDev = ptSnsCfg->nDevNode;
        }

        if (NULL != ptSnsHdl->pfn_sensor_set_bus_info) {
            nRet = ptSnsHdl->pfn_sensor_set_bus_info(nPipeId, tSnsBusInfo);
            if (0 != nRet) {
                LOG_M_E(LOG_TAG, "senort set bus info failed, ret=0x%x", nRet);
                return -1;
            }
        } else {
            LOG_M_E(LOG_TAG, "not support set sensor bus info");
            return -1;
        }
    }

    /* Step 5.3. AX_ISP_SetSnsAttr */
    nRet = AX_ISP_SetSnsAttr(nPipeId, &tSnsAttr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_SetSnsAttr failed, ret=0x%x", nRet);
        return -1;
    }

    /* Step 5.4. Sensor Set SlaveAddr */
    if (AX_NULL != ptSnsHdl->pfn_sensor_set_slaveaddr) {
        nRet = ptSnsHdl->pfn_sensor_set_slaveaddr(nPipeId, ptSnsCfg->nI2cAddr);
        if (0 != nRet) {
            LOG_M_E(LOG_TAG, "sensor set slaveaddr failed, ret=0x%x", nRet);
            return -1;
        }
    }

    /* Step 5.5. AX_ISP_OpenSnsClk */
    nRet = AX_ISP_OpenSnsClk(ptSnsCfg->nSnsClkIdx, (AX_SNS_CLK_RATE_E)ptSnsCfg->eSnsClkRate);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_OpenSnsClk failed, ret=0x%x", nRet);
        return -1;
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_SNS_Destroy(AX_S32 nPipeId, AX_OPAL_CAM_SENSOR_CFG_T* ptSnsCfg) {
    AX_S32 nRet = 0;
    nRet = AX_ISP_CloseSnsClk(ptSnsCfg->nSnsClkIdx);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_CloseSnsClk failed, ret=0x%x", nRet);
        return -1;
    }

    nRet = AX_ISP_UnRegisterSensor(nPipeId);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_UnRegisterSensor failed, ret=0x%x", nRet);
        return -1;
    }

    return nRet;
}
