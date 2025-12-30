/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "ax_vin_api.h"
#include "ax_isp_api.h"
#include "ax_isp_3a_api.h"
#include "common_isp.h"
#include "common_sys.h"
#include "qs_log.h"


static AX_ISP_AF_REGFUNCS_T s_tAfFuncs = {0};

/* TODO user need config device node number */
static AX_S8 COMMON_ISP_GetI2cDevNode(AX_U8 nDevId)
{

    if (!IS_AX620Q) {
        if (nDevId == 0)
            return 0;
        else
            return 2;
    } else {
        return 0;
    }
}

AX_SNS_CONNECT_TYPE_E COMMON_ISP_GetSnsBusType(SAMPLE_SNS_TYPE_E eSnsType)
{
    AX_SNS_CONNECT_TYPE_E enBusType;

    switch (eSnsType) {
    case OMNIVISION_OS04A10:
    case SAMPLE_SNS_TYPE_BUTT:
        enBusType = ISP_SNS_CONNECT_I2C_TYPE;
        break;
    default:
        enBusType = ISP_SNS_CONNECT_I2C_TYPE;
        break;
    }

    return enBusType;
}

AX_S32 COMMON_ISP_RegisterSns(AX_U8 nPipeId, AX_U8 nDevId, AX_SNS_CONNECT_TYPE_E eBusType,
                            AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl, AX_U8 nI2cAddr)
{
    AX_S32 axRet = 0;
    AX_SNS_COMMBUS_T tSnsBusInfo = {0};

    if (AX_NULL == ptSnsHdl) {
        COMM_ISP_PRT("AX_ISP Sensor Object NULL!\n");
        return -1;
    }

    /* ISP Register Sensor */
    axRet = AX_ISP_RegisterSensor(nPipeId, ptSnsHdl);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    /* confige i2c/spi dev id */
    if (ISP_SNS_CONNECT_I2C_TYPE == eBusType) {
        tSnsBusInfo.I2cDev = COMMON_ISP_GetI2cDevNode(nDevId);
        tSnsBusInfo.busType = ISP_SNS_CONNECT_I2C_TYPE;
    } else {
        tSnsBusInfo.SpiDev.bit4SpiDev = COMMON_ISP_GetI2cDevNode(nDevId);
        tSnsBusInfo.SpiDev.bit4SpiCs  = 0;
        tSnsBusInfo.busType = ISP_SNS_CONNECT_SPI_TYPE;
    }

    if (nDevId == 0) {
        tSnsBusInfo.nPwdnGpio = 35;
    }
    else {
        tSnsBusInfo.nPwdnGpio = 99;
#if defined(SAMPLE_SNS_OS04A10_DOUBLE) && defined(AX620E_NAND)
        if(!IS_AX620Q) {
            tSnsBusInfo.nPwdnGpio = 34;
        }
#endif
    }

#if defined(SAMPLE_SNS_SC200AI_TRIPLE) && defined(AX620E_NAND)
    if (nDevId == 0) {
        tSnsBusInfo.nPwdnGpio = 27;
    }
#endif

    if (NULL != ptSnsHdl->pfn_sensor_set_bus_info) {
        axRet = ptSnsHdl->pfn_sensor_set_bus_info(nPipeId, tSnsBusInfo);
        if (0 != axRet) {
            COMM_ISP_PRT("set sensor bus info failed with %#x!\n", axRet);
            return axRet;
        }
        // COMM_ISP_PRT("set sensor bus idx %d\n", tSnsBusInfo.I2cDev);
    } else {
        COMM_ISP_PRT("not support set sensor bus info!\n");
        return -1;
    }

    if (NULL != ptSnsHdl->pfn_sensor_set_slaveaddr) {
        axRet = ptSnsHdl->pfn_sensor_set_slaveaddr(nPipeId, nI2cAddr);
        if (0 != axRet) {
            COMM_ISP_PRT("set sensor slave addr failed with %#x!\n", axRet);
            return axRet;
        }
        //COMM_ISP_PRT("set sensor slave addr %d\n", nI2cAddr);
    }

    return 0;
}

AX_S32 COMMON_ISP_RegisterSnsMipiSwitch(AX_U8 nPipeId, AX_U8 nDevId, AX_SNS_CONNECT_TYPE_E eBusType,
                              AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl0, AX_U8 nI2cAddr0,
                              AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl1, AX_U8 nI2cAddr1) {
    AX_S32 axRet = 0;
    AX_SNS_COMMBUS_T tSnsBusInfo = {0};
    AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl = AX_NULL;
    AX_U8 nI2cAddr = 0;

    if (AX_NULL == ptSnsHdl0 || AX_NULL == ptSnsHdl1) {
        COMM_ISP_PRT("AX_ISP Sensor Object NULL!\n");
        return -1;
    }

    /* ISP Register Sensor */
    axRet = AX_ISP_RegisterSensorExt(nPipeId, 1, ptSnsHdl0);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP_RegisterSensor(pipe=%d, hdlid=1) Failed, ret=0x%x.\n", nPipeId, axRet);
        return axRet;
    }

    /* active sensor */
    axRet = AX_ISP_SetSnsActive(nPipeId, 1, 1);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP_SetSnsActive(pipe=%d, hdlid=1, active=1) Failed, ret=0x%x.\n", nPipeId, axRet);
        return axRet;
    }


    axRet = AX_ISP_RegisterSensorExt(nPipeId, 2, ptSnsHdl1);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP_RegisterSensor(pipe=%d, hdlid=2) Failed, ret=0x%x.\n", nPipeId, axRet);
        return axRet;
    }

    /* active sensor */
    axRet = AX_ISP_SetSnsActive(nPipeId, 2, 1);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP_SetSnsActive(pipe=%d, hdlid=2, active=1) Failed, ret=0x%x.\n", nPipeId, axRet);
        return axRet;
    }

    /* confige i2c/spi dev id */
    tSnsBusInfo.busType = ISP_SNS_CONNECT_I2C_TYPE;

    if (nDevId == 0) {
        tSnsBusInfo.nPwdnGpio = 35;
    } else {
        tSnsBusInfo.nPwdnGpio = 99;
    }

    for (int i = 0; i < 2; i++) {
        tSnsBusInfo.I2cDev = COMMON_ISP_GetI2cDevNode(nDevId);
        if (i == 0) {
            ptSnsHdl = ptSnsHdl0;
            nI2cAddr = nI2cAddr1;
#if defined(SAMPLE_SNS_SC200AI_TRIPLE) && defined(AX620E_NAND)
            tSnsBusInfo.I2cDev = 6;
            tSnsBusInfo.nPwdnGpio = 99;
#endif
        } else {
            ptSnsHdl = ptSnsHdl1;
            nI2cAddr = nI2cAddr0;
#if defined(SAMPLE_SNS_SC200AI_TRIPLE) && defined(AX620E_NAND)
            tSnsBusInfo.nPwdnGpio = 35;
#endif
        }

        COMM_ISP_PRT("[%d], i2cdev %d, nI2cAddr 0x%x\n", nPipeId + i, tSnsBusInfo.I2cDev, nI2cAddr);

        if (NULL != ptSnsHdl->pfn_sensor_set_bus_info) {
            axRet = ptSnsHdl->pfn_sensor_set_bus_info(nPipeId + i, tSnsBusInfo);
            if (0 != axRet) {
                COMM_ISP_PRT("set sensor bus info failed with %#x!\n", axRet);
                return axRet;
            }
            // COMM_ISP_PRT("set sensor bus idx %d\n", tSnsBusInfo.I2cDev);
        } else {
            COMM_ISP_PRT("not support set sensor bus info!\n");
            return -1;
        }

        if (NULL != ptSnsHdl->pfn_sensor_set_slaveaddr) {
            axRet = ptSnsHdl->pfn_sensor_set_slaveaddr(nPipeId + i, nI2cAddr);
            if (0 != axRet) {
                COMM_ISP_PRT("set sensor slave addr failed with %#x!\n", axRet);
                return axRet;
            }
            //COMM_ISP_PRT("set sensor slave addr %d\n", nI2cAddr);
        }
    }

    return 0;

}

AX_S32 COMMON_ISP_UnRegisterSns(AX_U8 nPipeId)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_UnRegisterSensor(nPipeId);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}

AX_S32 COMMON_ISP_UnRegisterSnsMipiSwitch(AX_U8 nPipeId)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_SetSnsActive(nPipeId, 1, 0);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP_SetSnsActive(pipe=%d, hdlid=1, active=0) Failed, ret=0x%x.\n", nPipeId, axRet);
        return axRet;
    }

    axRet = AX_ISP_SetSnsActive(nPipeId, 2, 0);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP_SetSnsActive(pipe=%d, hdlid=2, active=0) Failed, ret=0x%x.\n", nPipeId, axRet);
        return axRet;
    }

    axRet = AX_ISP_UnRegisterSensorExt(nPipeId, 1);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor (pipe=%d, hdlid=1) Failed, ret=0x%x.\n", nPipeId, axRet);
        return axRet;
    }

    axRet = AX_ISP_UnRegisterSensorExt(nPipeId, 2);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor (pipe=%d, hdlid=2  Failed, ret=0x%x.\n", nPipeId, axRet);
        return axRet;
    }

    return axRet;
}

AX_S32 COMMON_ISP_ResetSnsObj(AX_U8 nPipeId, AX_U8 nDevId, AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl)
{
    AX_S32 axRet = 0;
    AX_U32 nResetGpio;

    if (AX_NULL == ptSnsHdl) {
        COMM_ISP_PRT("AX_ISP Sensor Object NULL!\n");
        return -1;
    }

    /* reset sensor */
    if (nDevId == 0) {
        nResetGpio = 97;
#if defined(SAMPLE_SNS_SC200AI_TRIPLE) && defined(AX620E_NAND)
        if(IS_AX620Q) {
            nResetGpio = 10;
        }
#endif
    } else {
        nResetGpio = 40;
    }

    if (NULL != ptSnsHdl->pfn_sensor_reset) {
        axRet = ptSnsHdl->pfn_sensor_reset(nPipeId, nResetGpio);
        if (0 != axRet) {
            COMM_ISP_PRT("sensor reset failed with %#x!\n", axRet);
            return axRet;
        }
    }

    return 0;
}

AX_S32 COMMON_ISP_ResetSnsObjMipiSwitch(AX_U8 nPipeId, AX_U8 nDevId,
                                AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl0,
                                AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl1)
{
    AX_S32 axRet = 0;
    AX_U32 nResetGpio;
    AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl = AX_NULL;

    if (AX_NULL == ptSnsHdl0 || AX_NULL == ptSnsHdl1) {
        COMM_ISP_PRT("AX_ISP Sensor Object NULL!\n");
        return -1;
    }

    /* reset sensor */
    if (nDevId == 0) {
        nResetGpio = 97;
    } else {
        nResetGpio = 40;
    }

    for (int i = 0; i < 2; i++) {
        if (i == 0) {
            ptSnsHdl = ptSnsHdl0;
#if defined(SAMPLE_SNS_SC200AI_TRIPLE) && defined(AX620E_NAND)
            if(IS_AX620Q) {
                nResetGpio = 4;
            }
#endif
        } else {
            ptSnsHdl = ptSnsHdl1;
#if defined(SAMPLE_SNS_SC200AI_TRIPLE) && defined(AX620E_NAND)
            if(IS_AX620Q) {
                nResetGpio = 97;
            }
#endif
        }

        if (NULL != ptSnsHdl && NULL != ptSnsHdl->pfn_sensor_reset) {
            axRet = ptSnsHdl->pfn_sensor_reset(nPipeId + i, nResetGpio);
            if (0 != axRet) {
                COMM_ISP_PRT("sensor reset failed with %#x!\n", axRet);
                return axRet;
            }
        }
    }

    return 0;
}

AX_S32 COMMON_ISP_SetSnsAttr(AX_U8 nPipeId, AX_SNS_ATTR_T *ptSnsAttr, AX_SNS_CLK_ATTR_T *pstSnsClkAttr)
{
    AX_S32 axRet = 0;

    /* confige sensor attr */
    axRet = AX_ISP_SetSnsAttr(nPipeId, ptSnsAttr);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_SetSnsAttr failed, nRet=0x%x.\n", axRet);
        return -1;
    }

    return axRet;
}

AX_S32 COMMON_ISP_RegisterAeAlgLib(AX_U8 nPipeId, AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl, AX_BOOL bUser3a,
                                   AX_ISP_AE_REGFUNCS_T *pAeFuncs, AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl1)
{
    AX_S32 axRet = 0;
    AX_ISP_AE_REGFUNCS_T tAeFuncs = {0};

    if (AX_NULL == ptSnsHdl) {
        COMM_ISP_PRT("AX_ISP Sensor Object NULL!\n");
        return -1;
    }

    /* 3a get sensor config */
    //COMM_ISP_PRT("bUser3a %d\n", bUser3a);
    if (!bUser3a) {
        tAeFuncs.pfnAe_Init = AX_ISP_ALG_AeInit;
        tAeFuncs.pfnAe_Exit = AX_ISP_ALG_AeDeInit;
        tAeFuncs.pfnAe_Run  = AX_ISP_ALG_AeRun;
        tAeFuncs.pfnAe_Ctrl  = AX_ISP_ALG_AeCtrl;
    } else {
        tAeFuncs.pfnAe_Init = pAeFuncs->pfnAe_Init;
        tAeFuncs.pfnAe_Exit = pAeFuncs->pfnAe_Exit;
        tAeFuncs.pfnAe_Run  = pAeFuncs->pfnAe_Run;
        tAeFuncs.pfnAe_Ctrl  = pAeFuncs->pfnAe_Ctrl;
    }

    /* Register the sensor driver interface TO the AE library */
    if (ptSnsHdl1 == AX_NULL) {
        axRet = AX_ISP_ALG_AeRegisterSensor(nPipeId, ptSnsHdl);
        if (axRet) {
            COMM_ISP_PRT("AX_ISP ae register sensor Failed, ret=0x%x.\n", axRet);
            return axRet;
        }
    } else {
        axRet = AX_ISP_ALG_AeRegisterSensor(nPipeId, ptSnsHdl);
        if (axRet) {
            COMM_ISP_PRT("AX_ISP ae register sensor Failed, ret=0x%x.\n", axRet);
            return axRet;
        }
    }

    /* Register ae alg */
    axRet = AX_ISP_RegisterAeLibCallback(nPipeId, &tAeFuncs);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register ae callback Failed, ret=0x%x.\n", axRet);
        return axRet;

    }

    return axRet;
}

AX_S32 COMMON_ISP_UnRegisterAeAlgLib(AX_U8 nPipeId, AX_BOOL bMipiSwitch)
{
    AX_S32 axRet = 0;

    if (!bMipiSwitch) {
        axRet = AX_ISP_ALG_AeUnRegisterSensor(nPipeId);
        if (axRet) {
            COMM_ISP_PRT("AX_ISP ae un register sensor Failed, ret=0x%x.\n", axRet);
            return axRet;
        }
    } else {
        axRet = AX_ISP_ALG_AeUnRegisterSensor(nPipeId);
        if (axRet) {
            COMM_ISP_PRT("AX_ISP ae un register sensor Failed, ret=0x%x.\n", axRet);
            return axRet;
        }
    }

    axRet = AX_ISP_UnRegisterAeLibCallback(nPipeId);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}


AX_S32 COMMON_ISP_RegisterAwbAlgLib(AX_U8 nPipeId, AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl, AX_BOOL bUser3a,
                                    AX_ISP_AWB_REGFUNCS_T *pAwbFuncs)
{
    AX_S32 axRet = 0;
    AX_ISP_AWB_REGFUNCS_T tAwbFuncs = {0};

    if (AX_NULL == ptSnsHdl) {
        COMM_ISP_PRT("AX_ISP Sensor Object NULL!\n");
        return -1;
    }

    /* 3a get sensor config */
    if (!bUser3a) {
        tAwbFuncs.pfnAwb_Init = AX_ISP_ALG_AwbInit;
        tAwbFuncs.pfnAwb_Exit = AX_ISP_ALG_AwbDeInit;
        tAwbFuncs.pfnAwb_Run  = AX_ISP_ALG_AwbRun;
        tAwbFuncs.pfnAwb_Ctrl  = AX_ISP_ALG_AwbCtrl;
    } else {
        tAwbFuncs.pfnAwb_Init = pAwbFuncs->pfnAwb_Init;
        tAwbFuncs.pfnAwb_Exit = pAwbFuncs->pfnAwb_Exit;
        tAwbFuncs.pfnAwb_Run  = pAwbFuncs->pfnAwb_Run;
        tAwbFuncs.pfnAwb_Ctrl  = pAwbFuncs->pfnAwb_Ctrl;
    }

    /* Register the sensor driver interface TO the AWB library */
    axRet = AX_ISP_ALG_AwbRegisterSensor(nPipeId, ptSnsHdl);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    /* Register awb alg */
    axRet = AX_ISP_RegisterAwbLibCallback(nPipeId, &tAwbFuncs);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register awb callback Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}

AX_S32 COMMON_ISP_UnRegisterAwbAlgLib(AX_U8 nPipeId)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_ALG_AwbUnRegisterSensor(nPipeId);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP awb unregister sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    axRet = AX_ISP_UnRegisterAwbLibCallback(nPipeId);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}


AX_S32 COMMON_ISP_RegisterLscAlgLib(AX_U8 nPipeId, AX_BOOL bUser3a, AX_ISP_LSC_REGFUNCS_T *pLscFuncs)
{
    AX_S32 axRet = 0;

    /* Register Lsc alg */
    axRet = AX_ISP_RegisterLscLibCallback(nPipeId, pLscFuncs);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Register Lsc callback Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}

AX_S32 COMMON_ISP_UnRegisterLscAlgLib(AX_U8 nPipeId)
{
    AX_S32 axRet = 0;

    axRet = AX_ISP_UnRegisterLscLibCallback(nPipeId);
    if (axRet) {
        COMM_ISP_PRT("AX_ISP Unregister Sensor Failed, ret=0x%x.\n", axRet);
        return axRet;
    }

    return axRet;
}


AX_S32 COMMON_ISP_Init(AX_U8 nPipeId, AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl, AX_BOOL bRegisterSns,
                       AX_BOOL bUser3a, AX_ISP_AE_REGFUNCS_T *tAeFuncs, AX_ISP_AWB_REGFUNCS_T *tAwbFuncs,
                       AX_ISP_AF_REGFUNCS_T *tAfFuncs, AX_ISP_LSC_REGFUNCS_T *tLscFuncs, AX_CHAR *pIspParamsFile,
                       AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl1)
{
    AX_S32 axRet = 0;

    if (AX_NULL == ptSnsHdl) {
        COMM_ISP_PRT("AX_ISP Sensor Object NULL!\n");
        return -1;
    }

    ALOGD("pipe[%d] AX_ISP_Create ++", nPipeId);
    axRet = AX_ISP_Create(nPipeId);
    if (0 != axRet) {
        COMM_ISP_PRT("[%u]:AX_ISP_Create failed, ret=0x%x\n", nPipeId, axRet);
        return -1;
    }
    ALOGD("pipe[%d] AX_ISP_Create --", nPipeId);

    if (bRegisterSns) {
        /* ae alg register*/
        axRet = COMMON_ISP_RegisterAeAlgLib(nPipeId, ptSnsHdl, bUser3a, tAeFuncs, ptSnsHdl1);
        if (0 != axRet) {
            COMM_ISP_PRT("[%u]:RegisterAeAlgLib failed, ret=0x%x.\n", nPipeId, axRet);
            return -1;
        }

        /* awb alg register*/
        axRet = COMMON_ISP_RegisterAwbAlgLib(nPipeId, ptSnsHdl, bUser3a, tAwbFuncs);
        if (0 != axRet) {
            COMM_ISP_PRT("[%u]:RegisterAwbAlgLib failed, ret=0x%x.\n", nPipeId, axRet);
            return -1;
        }
    }

    if (strcmp(pIspParamsFile, "null.bin")) {
        axRet = AX_ISP_LoadBinParams(nPipeId, pIspParamsFile);
        if (0 != axRet) {
            COMM_ISP_PRT("AX_ISP_LoadBinParams ret=0x%x %s. The parameters in sensor.h will be used\n", axRet, pIspParamsFile);
        }
    }

    if(AX_NULL != tAfFuncs) {
        s_tAfFuncs = *tAfFuncs;
        if (AX_NULL != s_tAfFuncs.pfnCAf_Init) {
            s_tAfFuncs.pfnCAf_Init(nPipeId);
        }
    }
    ALOGD("pipe[%d] AX_ISP_Open ++", nPipeId);
    axRet = AX_ISP_Open(nPipeId);
    ALOGD("pipe[%d] AX_ISP_Open --", nPipeId);

    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_Open failed, ret=0x%x\n", axRet);
        return -1;
    }

    return axRet;
}

AX_S32 COMMON_ISP_DeInit(AX_U8 nPipeId, AX_BOOL bRegisterSns, AX_BOOL bMipiSwitch)
{
    AX_S32 axRet = 0;

    axRet |= AX_ISP_Close(nPipeId);
    if (0 != axRet) {
        COMM_ISP_PRT("[%u]:AX_ISP_Close failed, ret=0x%x.\n", nPipeId, axRet);
    }

    if (bRegisterSns) {
        COMMON_ISP_UnRegisterAeAlgLib(nPipeId, bMipiSwitch);
        COMMON_ISP_UnRegisterAwbAlgLib(nPipeId);
    }

    if (AX_NULL != s_tAfFuncs.pfnCAf_Exit) {
        s_tAfFuncs.pfnCAf_Exit(nPipeId);
        memset(&s_tAfFuncs, 0, sizeof(AX_ISP_AF_REGFUNCS_T));
    }

    axRet |= AX_ISP_Destroy(nPipeId);
    if (0 != axRet) {
        COMM_ISP_PRT("[%u]:AX_ISP_Exit failed, ret=0x%x.\n", nPipeId, axRet);
    }

    return axRet;
}

AX_S32 COMMON_ISP_SetAeToManual(AX_U8 nPipeId)
{
    AX_S32 axRet = 0;
    AX_ISP_IQ_AE_PARAM_T tIspAeParam = {0};

    axRet = AX_ISP_IQ_GetAeParam(nPipeId, &tIspAeParam);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_IQ_GetAeParam failed, ret=0x%x.\n", axRet);
        return -1;
    }

    tIspAeParam.nEnable = AX_FALSE;

    axRet = AX_ISP_IQ_SetAeParam(nPipeId, &tIspAeParam);
    if (0 != axRet) {
        COMM_ISP_PRT("AX_ISP_IQ_SetAeParam failed, nRet = 0x%x.\n", axRet);
        return -1;
    }

    return 0;
}
