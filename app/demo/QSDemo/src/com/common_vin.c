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

#include "ax_sensor_struct.h"
#include "ax_buffer_tool.h"
#include "ax_vin_api.h"
#include "common_vin.h"


AX_S32 COMMON_VIN_StartMipi(AX_U8 nRxDev, AX_INPUT_MODE_E eInputMode, AX_MIPI_RX_ATTR_T *ptMipiAttr)
{
    AX_S32 nRet = 0;
    AX_MIPI_RX_DEV_T  tMipiDev = {0};

    tMipiDev.eInputMode = eInputMode;
    memcpy(&tMipiDev.tMipiAttr, ptMipiAttr, sizeof(AX_MIPI_RX_ATTR_T));
    if (tMipiDev.tMipiAttr.eLaneNum == AX_MIPI_DATA_LANE_4)
        AX_MIPI_RX_SetLaneCombo(AX_LANE_COMBO_MODE_0);
    else
        AX_MIPI_RX_SetLaneCombo(AX_LANE_COMBO_MODE_1);

    nRet = AX_MIPI_RX_SetAttr(nRxDev, &tMipiDev);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_MIPI_RX_SetAttr failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_MIPI_RX_Reset(nRxDev);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_MIPI_RX_Reset, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_MIPI_RX_Start(nRxDev);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_MIPI_RX_Start failed, ret=0x%x.\n", nRet);
        return -1;
    }

    return 0;
}

AX_S32 COMMON_VIN_StopMipi(AX_U8 nRxDev)
{
    AX_S32 axRet;

    axRet = AX_MIPI_RX_Stop(nRxDev);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_MIPI_RX_Stop failed, ret=0x%x.\n", axRet);
    }

    return 0;
}


AX_S32 COMMON_VIN_StartDev(AX_U8 devId, AX_BOOL bEnableDev, AX_VIN_DEV_ATTR_T *pDevAttr)
{
    AX_S32 nRet = 0;
    AX_VIN_DUMP_ATTR_T  tDumpAttr = {0};

    if (bEnableDev) {
        if (AX_VIN_DEV_OFFLINE == pDevAttr->eDevMode) {
            tDumpAttr.bEnable = AX_TRUE;
            tDumpAttr.nDepth = 3;
            nRet = AX_VIN_SetDevDumpAttr(devId, AX_VIN_DUMP_QUEUE_TYPE_DEV, &tDumpAttr);
            if (0 != nRet) {
                COMM_VIN_PRT(" AX_VIN_SetDevDumpAttr failed, ret=0x%x.\n", nRet);
                return -1;
            }
        }

        nRet = AX_VIN_EnableDev(devId);
        if (0 != nRet) {
            COMM_VIN_PRT("AX_VIN_EnableDev failed, ret=0x%x.\n", nRet);
            return -1;
        }
    }

    return 0;
}

AX_S32 COMMON_VIN_StopDev(AX_U8 devId, AX_BOOL bEnableDev)
{
    AX_S32 axRet;
    AX_VIN_DEV_ATTR_T tDevAttr = {0};
    AX_VIN_DUMP_ATTR_T tDumpAttr = {0};

    AX_VIN_GetDevAttr(devId, &tDevAttr);

    if (bEnableDev) {
        axRet = AX_VIN_DisableDev(devId);
        if (0 != axRet) {
            COMM_VIN_PRT("AX_VIN_DisableDev failed, devId=%d, ret=0x%x.\n", devId, axRet);
        }

        if (AX_VIN_DEV_OFFLINE == tDevAttr.eDevMode) {
            tDumpAttr.bEnable = AX_FALSE;
            axRet = AX_VIN_SetDevDumpAttr(devId, AX_VIN_DUMP_QUEUE_TYPE_DEV, &tDumpAttr);
            if (0 != axRet) {
                COMM_VIN_PRT(" AX_VIN_SetSnsDumpAttr failed, ret=0x%x.\n", axRet);
            }
        }

    }

    return 0;
}

AX_S32 COMMON_VIN_CreateDev(AX_U8 devId, AX_U32 nRxDev, AX_VIN_DEV_ATTR_T *pDevAttr,
                            AX_VIN_DEV_BIND_PIPE_T *ptDevBindPipe)
{
    AX_S32 nRet = 0;

    nRet = AX_VIN_CreateDev(devId, pDevAttr);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_VIN_CreateDev failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_VIN_SetDevAttr(devId, pDevAttr);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_VIN_CreateDev failed, ret=0x%x.\n", nRet);
        return -1;
    }

    nRet = AX_VIN_SetDevBindPipe(devId, ptDevBindPipe);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_VIN_SetDevBindPipe failed, ret=0x%x\n", nRet);
        return -1;
    }

    nRet = AX_VIN_SetDevBindMipi(devId, nRxDev);
    if (0 != nRet) {
        COMM_VIN_PRT("AX_VIN_SetDevBindMipi failed, ret=0x%x\n", nRet);
        return -1;
    }

    return 0;
}

AX_S32 COMMON_VIN_DestroyDev(AX_U8 devId)
{
    AX_S32 axRet;

    axRet = AX_VIN_DestroyDev(devId);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_DestroyDev failed, devId=%d, ret=0x%x.\n", devId, axRet);
    }

    return 0;
}

AX_S32 COMMON_VIN_SetPipeAttr(COMMON_VIN_MODE_E eSysMode, SAMPLE_LOAD_RAW_NODE_E eLoadRawNode, AX_U8 nPipeId,
                              AX_VIN_PIPE_ATTR_T *pPipeAttr)
{
    AX_S32 axRet;
    AX_VIN_DUMP_ATTR_T stDumpAttr;
    AX_U8 nPipeWorkMode = 0;
    memset(&stDumpAttr, 0, sizeof(stDumpAttr));

    axRet = AX_VIN_CreatePipe(nPipeId, pPipeAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VIN_CreatePipe failed, ret=0x%x.\n", axRet);
        return -1;
    }
    axRet = AX_VIN_SetPipeAttr(nPipeId, pPipeAttr);
    if (0 != axRet) {
        COMM_VIN_PRT("AX_VI_SetPipeAttr failed, nRet = 0x%x.\n", axRet);
        return -1;
    }

    nPipeWorkMode = (AX_U8)pPipeAttr->ePipeWorkMode;
    switch (nPipeWorkMode) {
    case AX_VIN_PIPE_NORMAL_MODE1:
        if (COMMON_VIN_LOADRAW == eSysMode && LOAD_RAW_ITP == eLoadRawNode) {
            stDumpAttr.bEnable = AX_TRUE;
            stDumpAttr.nDepth = 1;
            axRet = AX_VIN_SetPipeDumpAttr(nPipeId, AX_VIN_PIPE_DUMP_NODE_IFE, AX_VIN_DUMP_QUEUE_TYPE_DEV, &stDumpAttr);
            if (axRet) {
                COMM_VIN_PRT("pipe %d src %d dump attr set failed....\n", nPipeId, AX_VIN_PIPE_DUMP_NODE_IFE);
            }

            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_DEV);
            if (axRet) {
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE);
            }

            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_ITP, AX_VIN_FRAME_SOURCE_TYPE_USER);  // IFE + ITP RAW+RGB+YUV DOMAIN
            if (axRet) {
                COMM_VIN_PRT("pipe %d src %d  frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_ITP);
            }
            break;
        } else if (COMMON_VIN_LOADRAW == eSysMode) {
            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_USER);
            if (axRet) {
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE);
            }
        } else if (LOAD_RAW_ITP == eLoadRawNode) {
            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_ITP, AX_VIN_FRAME_SOURCE_TYPE_USER);
            if (axRet)
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_ITP);
            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_DEV);
            if (axRet)
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE);
        } else {
            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_DEV);
            if (axRet)
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE);
        }
        break;
    case AX_VIN_PIPE_SUB_YUV_MODE:
        if (COMMON_VIN_LOADRAW == eSysMode) {
            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_USER);
            if (axRet)
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE);
            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_YUV, AX_VIN_FRAME_SOURCE_TYPE_USER);
            if (axRet)
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE);
        } else {
            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_VIN_FRAME_SOURCE_TYPE_DEV);
            if (axRet)
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE);
            axRet = AX_VIN_SetPipeFrameSource(nPipeId, AX_VIN_FRAME_SOURCE_ID_YUV, AX_VIN_FRAME_SOURCE_TYPE_DEV);
            if (axRet)
                COMM_VIN_PRT("pipe %d src %d frame source set failed....\n", nPipeId, AX_VIN_FRAME_SOURCE_ID_IFE);
        }
        break;
    default:
        COMM_VIN_PRT("pipe work mode%x.\n", pPipeAttr->ePipeWorkMode);
        break;
    }

    return 0;
}

AX_S32 COMMON_VIN_StartChn(AX_U8 pipe, AX_VIN_CHN_ATTR_T *ptChnAttr, AX_BOOL *pChnEn, AX_S32 *pChnFrmMode)
{
    AX_S32 nRet = 0;
    AX_S32 chn = 0;

    for (chn = 0; chn < AX_VIN_CHN_ID_MAX; chn++) {
        nRet = AX_VIN_SetChnAttr(pipe, chn, &ptChnAttr[chn]);
        if (0 != nRet) {
            COMM_VIN_PRT("AX_VIN_SetChnAttr failed, nRet=0x%x.\n", nRet);
            return -1;
        }

        if(pChnEn[chn] == AX_TRUE) {
            nRet = AX_VIN_SetChnFrameMode(pipe, (AX_VIN_CHN_ID_E)chn, (AX_VIN_FRAME_MODE_E)(pChnFrmMode[chn]));
            if (0 != nRet) {
                COMM_VIN_PRT("AX_VIN_SetChnFrameMode failed, nRet=0x%x.\n", nRet);
            }

            nRet = AX_VIN_EnableChn(pipe, (AX_VIN_CHN_ID_E)chn);
            if (0 != nRet) {
                COMM_VIN_PRT("AX_VIN_EnableChn failed, nRet=0x%x.\n", nRet);
                return -1;
            }
        }
    }

    return 0;
}

AX_S32 COMMON_VIN_StopChn(AX_U8 pipe)
{
    AX_S32 nRet = 0;
    AX_S32 chn = 0;

    for (chn = 0; chn < AX_VIN_CHN_ID_MAX; chn++) {
        nRet = AX_VIN_DisableChn(pipe, chn);
        if (0 != nRet) {
            COMM_VIN_PRT("AX_VIN_DisableChn failed, nRet=0x%x.\n", nRet);
            return -1;
        }
    }

    return 0;
}


