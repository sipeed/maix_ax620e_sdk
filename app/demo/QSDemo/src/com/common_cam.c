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
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>

#include "ax_vin_api.h"
#include "ax_isp_api.h"
#include "ax_vin_error_code.h"
#include "ax_mipi_rx_api.h"
#include "common_cam.h"
#include "common_sys.h"
#include "ax_isp_3a_api.h"
#include "mipi_switch.h"
#include "qs_log.h"
#include "ax_isp_iq_api.h"


extern AX_BOOL g_bIvpsInited;
extern pthread_mutex_t g_mtxIvpsInit;
extern pthread_cond_t g_condIvpsInit;
pthread_mutex_t g_mtxCropRect;

static pthread_t g_DispatchThread[MAX_CAMERAS] = {0};
static AX_S32 g_DispatcherLoopExit[MAX_CAMERAS] = {0};
static SAMPLE_CROP_RECT_INFO_T g_tCropRectInfo = {0};
static pthread_t g_tidCheckI2CReady = 0;
static void *DispatchThread(void *args);
static AX_S32 RawFrameDispatch(AX_U8 nPipeId, AX_CAMERA_T *pCam, AX_SNS_HDR_MODE_E eHdrMode);
static void *CheckI2CReadyThread(void *args);

#if defined(AX_RISCV_LOAD_MODEL_SUPPORT)
extern AX_VOID SampleSetReleaseModelImageMemFlag(AX_S32 nCam);
#endif

#ifndef COMM_SWAP_U32
#define COMM_SWAP_U32(a, b) {AX_U32 temp = a; a = b; b = temp;}
#endif
#ifndef COMM_SWAP_S16
#define COMM_SWAP_S16(a, b) {AX_S16 temp = a; a = b; b = temp;}
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif
#ifndef AX_MAX
#define AX_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

AX_S32 COMMON_NPU_Init()
{
    AX_S32 axRet = 0;

    /* NPU Init */
    AX_ENGINE_NPU_ATTR_T attr;
    memset(&attr, 0, sizeof(AX_ENGINE_NPU_ATTR_T));
    attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_ENABLE;
    axRet = AX_ENGINE_Init(&attr);
    if (0 != axRet) {
        COMM_CAM_PRT("AX_ENGINE_Init failed, ret=0x%x.\n", axRet);
        return -1;
    }
    return 0;
}

AX_S32 COMMON_CAM_PrivPoolInit(COMMON_SYS_ARGS_T *pPrivPoolArgs)
{
    AX_S32 axRet = 0;
    AX_POOL_FLOORPLAN_T tPoolFloorPlan = {0};

    if (pPrivPoolArgs == NULL) {
        return -1;
    }

    /* Calc Pool BlkSize/BlkCnt */
    axRet = COMMON_SYS_CalcPool(pPrivPoolArgs->pPoolCfg, pPrivPoolArgs->nPoolCfgCnt, &tPoolFloorPlan);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_SYS_CalcPool failed, ret=0x%x.\n", axRet);
        return -1;
    }

    axRet = AX_VIN_SetPoolAttr(&tPoolFloorPlan);
    if (0 != axRet) {
        COMM_CAM_PRT("AX_VIN_SetPoolAttr fail!Error Code:0x%X\n", axRet);
        return -1;
    }

    return 0;
}

AX_S32 COMMON_CAM_Init(AX_VOID)
{
    AX_S32 axRet = 0;

    /* VIN Init */
    axRet = AX_VIN_Init();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_VIN_Init failed, ret=0x%x.\n", axRet);
        return -1;
    }

    /* Stitch Init */
    // axRet = COMMON_CAM_StitchAttrInit();
    // if (0 != axRet) {
    //     COMM_CAM_PRT("COMMON_CAM_StitchAttrInit failed, ret=0x%x.\n", axRet);
    //     return -1;
    // }

    /* MIPI Init */
    axRet = AX_MIPI_RX_Init();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_MIPI_RX_Init failed, ret=0x%x.\n", axRet);
        return -1;
    }

    return 0;
}

AX_S32 COMMON_CAM_Deinit(AX_VOID)
{
    AX_S32 axRet = 0;

    axRet = AX_MIPI_RX_DeInit();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_MIPI_RX_DeInit failed, ret=0x%x.\n", axRet);
        return -1 ;
    }

    axRet = AX_VIN_Deinit();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_VIN_DeInit failed, ret=0x%x.\n", axRet);
        return -1 ;
    }
    axRet = AX_ENGINE_Deinit();
    if (0 != axRet) {
        COMM_CAM_PRT("AX_ENGINE_Deinit failed, ret=0x%x.\n", axRet);
        return -1 ;
    }
    return axRet;
}

static AX_S32 __common_cam_open(AX_CAMERA_T *pCam)
{
    AX_S32 i = 0;
    AX_S32 axRet = 0;
    AX_U8 nPipeId = 0;
    AX_U8 nDevId = pCam->nDevId;
    AX_U32 nRxDev = pCam->nRxDev;
    AX_INPUT_MODE_E eInputMode = pCam->eInputMode;

    /* confige sensor clk [qs_mode no need call]*/
    // axRet = AX_ISP_OpenSnsClk(pstSnsClkAttr->nSnsClkIdx, pstSnsClkAttr->eSnsClkRate);
    // if (0 != axRet) {
    //     COMM_ISP_PRT("AX_ISP_OpenSnsClk failed, nRet=0x%x.\n", axRet);
    //     return -1;
    // }

    if (!pCam->bMipiSwitchEnable) {
        axRet = COMMON_ISP_ResetSnsObj(nPipeId, nDevId, pCam->ptSnsHdl);
        if (0 != axRet) {
            COMM_CAM_PRT("COMMON_ISP_ResetSnsObj failed, ret=0x%x.\n", axRet);
            return -1;
        }
    } else {
        axRet = COMMON_ISP_ResetSnsObjMipiSwitch(nPipeId, nDevId, pCam->ptSnsHdl, pCam->ptSnsHdl1);
        if (0 != axRet) {
            COMM_CAM_PRT("COMMON_ISP_ResetSnsObjMipiSwitch failed, ret=0x%x.\n", axRet);
            return -1;
        }
    }

    axRet = COMMON_VIN_StartMipi(nRxDev, eInputMode, &pCam->tMipiAttr);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_StartMipi failed, r-et=0x%x.\n", axRet);
        return -1;
    }

    axRet = COMMON_VIN_CreateDev(nDevId, nRxDev, &pCam->tDevAttr, &pCam->tDevBindPipe);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_CreateDev failed, ret=0x%x.\n", axRet);
        return -1;
    }

    for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
        nPipeId = pCam->tDevBindPipe.nPipeId[i];
        pCam->tPipeAttr.bAiIspEnable = pCam->tPipeInfo[i].bAiispEnable;
        axRet = COMMON_VIN_SetPipeAttr(pCam->eSysMode, pCam->eLoadRawNode, nPipeId, &pCam->tPipeAttr);
        if (0 != axRet) {
            COMM_CAM_PRT("COMMON_ISP_SetPipeAttr failed, ret=0x%x.\n", axRet);
            return -1;
        }
        if (pCam->bRegisterSns) {
            if (!pCam->bMipiSwitchEnable) {
                axRet = COMMON_ISP_RegisterSns(nPipeId, nDevId, pCam->eBusType, pCam->ptSnsHdl, pCam->nI2cAddr);
                if (0 != axRet) {
                    COMM_CAM_PRT("COMMON_ISP_RegisterSns failed, ret=0x%x.\n", axRet);
                    return -1;
                }

            } else {
                axRet = COMMON_ISP_RegisterSnsMipiSwitch(nPipeId, nDevId, pCam->eBusType, pCam->ptSnsHdl, pCam->nI2cAddr, pCam->ptSnsHdl1, pCam->nI2cAddr1);
                if (0 != axRet) {
                    COMM_CAM_PRT("COMMON_ISP_RegisterSnsMipiSwitch failed, ret=0x%x.\n", axRet);
                    return -1;
                }
            }

            axRet = COMMON_ISP_SetSnsAttr(nPipeId, &pCam->tSnsAttr, &pCam->tSnsClkAttr);
            if (0 != axRet) {
                COMM_CAM_PRT("COMMON_ISP_SetSnsAttr failed, ret=0x%x.\n", axRet);
                return -1;
            }

            // make sure above api is invoked
            if (pCam->bMipiSwitchEnable) {
                g_tidCheckI2CReady = 0;
                axRet = pthread_create(&g_tidCheckI2CReady, NULL, CheckI2CReadyThread, (AX_VOID *)(pCam));
                if (0 != axRet) {
                    COMM_CAM_PRT("pthread_create failed, ret=0x%x.\n", axRet);
                    return -1;
                }
            }
        }
        axRet = COMMON_ISP_Init(nPipeId, pCam->ptSnsHdl, pCam->bRegisterSns, pCam->bUser3a,
                                &pCam->tAeFuncs, &pCam->tAwbFuncs, &pCam->tAfFuncs, &pCam->tLscFuncs,
                                pCam->tPipeInfo[i].szBinPath, pCam->ptSnsHdl1);
        if (0 != axRet) {
            COMM_CAM_PRT("COMMON_ISP_StartIsp failed, axRet = 0x%x.\n", axRet);
            return -1;
        }
        axRet = COMMON_VIN_StartChn(nPipeId, pCam->tChnAttr, pCam->bChnEn, pCam->nChnFrmMode);
        if (0 != axRet) {
            COMM_CAM_PRT("COMMON_ISP_StartChn failed, nRet = 0x%x.\n", axRet);
            return -1;
        }

        pthread_mutex_lock(&g_mtxIvpsInit);
        while (!g_bIvpsInited) {
            pthread_cond_wait(&g_condIvpsInit, &g_mtxIvpsInit);
        }
        pthread_mutex_unlock(&g_mtxIvpsInit);

#if defined(AX_RISCV_LOAD_MODEL_SUPPORT)
        SampleSetReleaseModelImageMemFlag(pCam->nPipeId);
#endif

        axRet = AX_VIN_StartPipe(nPipeId);
        if (0 != axRet) {
            COMM_CAM_PRT("AX_VIN_StartPipe failed, ret=0x%x\n", axRet);
            return -1;
        }

        ALOGD("pipe[%d] AX_ISP_Start ++", pCam->nPipeId);
        axRet = AX_ISP_Start(nPipeId);
        if (0 != axRet) {
            COMM_CAM_PRT("AX_ISP_Open failed, ret=0x%x\n", axRet);
            return -1;
        }
        ALOGD("pipe[%d] AX_ISP_Start --", pCam->nPipeId);

        /* When there are multiple pipe, only the first pipe needs AE */
        // if (0 < i) {
        //     axRet = COMMON_ISP_SetAeToManual(nPipeId);
        //     if (0 != axRet) {
        //         COMM_CAM_PRT("COMMON_ISP_SetAeToManual failed, ret=0x%x\n", axRet);
        //         return -1;
        //     }
        // }
    }

    axRet = COMMON_VIN_StartDev(nDevId, pCam->bEnableDev, &pCam->tDevAttr);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_StartDev failed, ret=0x%x.\n", axRet);
        return -1;
    }

    if (pCam->bRegisterSns && pCam->bEnableDev) {
        for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
            nPipeId = pCam->tDevBindPipe.nPipeId[i];
            axRet = AX_ISP_StreamOn(nPipeId);
            if (0 != axRet) {
                COMM_CAM_PRT("AX_ISP_StreamOn failed, ret=0x%x.\n", axRet);
                return -1;
            }

            // if (pCam->bMipiSwitchEnable) {
            //     // stream on another sns
            //     if (pCam->nSwitchSnsId == 1) {
            //         if (pCam->ptSnsHdl1 && pCam->ptSnsHdl1->pfn_sensor_streaming_ctrl) {
            //             pCam->ptSnsHdl1->pfn_sensor_streaming_ctrl(nPipeId, 1);
            //         }
            //     } else {
            //         if (pCam->ptSnsHdl && pCam->ptSnsHdl->pfn_sensor_streaming_ctrl) {
            //             pCam->ptSnsHdl->pfn_sensor_streaming_ctrl(nPipeId, 1);
            //         }
            //     }
            // }
        }
    }
    ALOGD("pipe[%d] AX_ISP_StreamOn done", pCam->nPipeId);

    if (pCam->bMipiSwitchEnable) {
        AX_SWITCH_INFO_T switch_info = {0};
        switch_info.nFps = pCam->tSnsAttr.fFrameRate;
        switch_info.nPipeNum = 1;
        switch_info.tSnsInfo[0].nSnsId = 1;
        switch_info.tSnsInfo[0].nPipeId = pCam->nPipeId;
        switch_info.tSnsInfo[0].eLensType = AX_LENS_TYPE_WIDE_FIELD;
        switch_info.tSnsInfo[0].eWorkMode = AX_MIPI_SWITCH_STAY_LOW;
        switch_info.tSnsInfo[0].eVsyncType = AX_MIPI_SWITCH_FSYNC_FLASH;
        switch_info.tSnsInfo[1].nSnsId = 2;
        switch_info.tSnsInfo[1].nPipeId = pCam->nPipeId;
        switch_info.tSnsInfo[1].eLensType = AX_LENS_TYPE_LONG_FOCAL;
        switch_info.tSnsInfo[1].eWorkMode = AX_MIPI_SWITCH_STAY_HIGH;
        switch_info.tSnsInfo[1].eVsyncType = AX_MIPI_SWITCH_FSYNC_VSYNC;

        if (pCam->nSwitchSnsId == 1) {
            switch_info.eWorkMode = AX_MIPI_SWITCH_STAY_LOW;
        } else {
            switch_info.eWorkMode = AX_MIPI_SWITCH_STAY_HIGH;
        }

        ax_mipi_switch_init(&switch_info);
        ax_mipi_switch_start();
    }

    if (pCam->bEnableDev && pCam->eLoadRawNode == LOAD_RAW_ITP) {
        if (pCam->nNumber >= MAX_CAMERAS) {
            COMM_CAM_PRT("Access g_dispatcher_loop_exit[%d] out of bounds \n", pCam->nNumber);
            return -1;
        }
        g_DispatcherLoopExit[pCam->nNumber] = 0;
        axRet = pthread_create(&g_DispatchThread[pCam->nNumber], NULL, DispatchThread, (AX_VOID *)(pCam));
        if (0 != axRet) {
            COMM_CAM_PRT("pthread_create failed, ret=0x%x.\n", axRet);
            return -1;
        }
    }

    return 0;
}

static AX_S32 __common_cam_close(AX_CAMERA_T *pCam)
{
    AX_U8 i = 0;
    AX_S32 axRet = 0;
    AX_U8 nPipeId = pCam->nPipeId;
    AX_U8 nDevId = pCam->nDevId;
    AX_U32 nRxDev = pCam->nRxDev;

    if (pCam->nNumber < MAX_CAMERAS) {
        g_DispatcherLoopExit[pCam->nNumber] = 1;
        if (g_DispatchThread[pCam->nNumber] != 0) {
            axRet = pthread_join(g_DispatchThread[pCam->nNumber], NULL);
            if (axRet < 0) {
                COMM_CAM_PRT("raw dispacher thread exit failed, ret=0x%x.\n", axRet);
            }
            g_DispatchThread[pCam->nNumber] = 0;
        }
    }

    for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
        nPipeId = pCam->tDevBindPipe.nPipeId[i];
        axRet |= AX_ISP_Stop(nPipeId);
        if (0 != axRet) {
            COMM_ISP_PRT("AX_ISP_Stop failed, ret=0x%x.\n", axRet);
        }
    }

    axRet = COMMON_VIN_StopDev(nDevId, pCam->bEnableDev);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_StopDev failed, ret=0x%x.\n", axRet);
    }

    if (pCam->bRegisterSns && pCam->bEnableDev) {
        for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
            AX_ISP_StreamOff(pCam->tDevBindPipe.nPipeId[i]);
        }
    }

    /* confige sensor clk [qs_mode no need call]*/
    // axRet = AX_ISP_CloseSnsClk(pCam->tSnsClkAttr.nSnsClkIdx);
    // if (0 != axRet) {
    //     COMM_CAM_PRT("AX_VIN_CloseSnsClk failed, ret=0x%x.\n", axRet);
    // }

    for (i = 0; i < pCam->tDevBindPipe.nNum; i++) {
        nPipeId = pCam->tDevBindPipe.nPipeId[i];
        axRet = AX_VIN_StopPipe(nPipeId);
        if (0 != axRet) {
            COMM_CAM_PRT("AX_VIN_StopPipe failed, ret=0x%x.\n", axRet);
        }

        COMMON_VIN_StopChn(nPipeId);

        COMMON_ISP_DeInit(nPipeId, pCam->bRegisterSns, pCam->bMipiSwitchEnable);

        if (!pCam->bMipiSwitchEnable) {
            COMMON_ISP_UnRegisterSns(nPipeId);
        } else {
            COMMON_ISP_UnRegisterSnsMipiSwitch(nPipeId);
        }

        AX_VIN_DestroyPipe(nPipeId);
    }

    axRet = COMMON_VIN_StopMipi(nRxDev);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_StopMipi failed, ret=0x%x.\n", axRet);
    }

    axRet = COMMON_VIN_DestroyDev(nDevId);
    if (0 != axRet) {
        COMM_CAM_PRT("COMMON_VIN_DestroyDev failed, ret=0x%x.\n", axRet);
    }

    // COMM_CAM_PRT("%s: nDevId %d: exit.\n", __func__, nDevId);

    return AX_SUCCESS;
}

AX_S32 COMMON_CAM_Open(AX_CAMERA_T *pCamList, AX_U8 Num)
{
    AX_U16 i = 0;
    if (pCamList == NULL) {
        return -1;
    }

    for (i = 0; i < Num; i++) {
        if (AX_SUCCESS == __common_cam_open(&pCamList[i])) {
            pCamList[i].bOpen = AX_TRUE;
        } else {
            goto EXIT;
        }
    }
    return 0;
EXIT:
    for (i = 0; i < Num; i++) {
        if (!pCamList[i].bOpen) {
            continue;
        }
        __common_cam_close(&pCamList[i]);
    }
    return -1;
}

AX_S32 COMMON_CAM_Close(AX_CAMERA_T *pCamList, AX_U8 Num)
{
    ALOGI("COMMON_CAM_Close ++");
    AX_U16 i = 0;
    if (pCamList == NULL) {
        return -1;
    }

    for (i = 0; i < Num; i++) {
        if (!pCamList[i].bOpen) {
            continue;
        }
        if (AX_SUCCESS == __common_cam_close(&pCamList[i])) {
            ALOGI("camera %d is close", i);
            pCamList[i].bOpen = AX_FALSE;
        } else {
            return -1;
        }
    }
    ALOGI("COMMON_CAM_Close --");
    return 0;
}

static AX_BOOL Check3ARoi(AX_U32 nImgWidth, AX_U32 nImgHeight, const AX_ISP_IQ_AE_STAT_ROI_T *pRoi)
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
    for (i = 0; i < nBaseNum; i++) {
        nValidBaseWidth = ((nImgWidth - i * nBaseWidth) > nBaseWidth) ? nBaseWidth : (nImgWidth - i * nBaseWidth);
        if (nOffsetL < nValidBaseWidth) {
            break;
        }
        nOffsetL -= nValidBaseWidth;
        nInvalidBaseL++;
    }

    nOffsetR = nImgWidth - pRoi->nRoiOffsetH - pRoi->nRoiRegionW * pRoi->nRoiRegionNumH;
    for (i = nBaseNum - 1; i >= 0; i--) {
        nValidBaseWidth = ((nImgWidth - i * nBaseWidth) > nBaseWidth) ? nBaseWidth : (nImgWidth - i * nBaseWidth);
        if (nOffsetR < nValidBaseWidth) {
            break;
        }
        nOffsetR -= nValidBaseWidth;
        nInvalidBaseR++;
    }

    for (i = 0; i < nBaseNum; i++) {
        if (i < nInvalidBaseL || i > nBaseNum - 1 - nInvalidBaseR) {
            continue;
        }

        if (i == 0) {
            nValidBaseWidth = nBaseWidth - nOffsetL;
            nGridWidth = (pRoi->nRoiRegionNumH == 1) ? nValidBaseWidth : pRoi->nRoiRegionW;
            nGridL = nOffsetL;
            nGridNum = nValidBaseWidth / nGridWidth;
            nGridR = nValidBaseWidth - nGridNum * nGridWidth;
            if ((nGridR > 0 && nGridR < 8) || nGridWidth < 8) {
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
                return AX_FALSE;
            }
        }
    }

    return AX_TRUE;
}

static AX_BOOL Get3ARoi(AX_U32 nImgWidth, AX_U32 nImgHeight, AX_WIN_AREA_T *pCropRect, AX_ISP_IQ_AE_STAT_PARAM_T *ptAeParam, AX_ISP_IQ_AWB_STAT_PARAM_T *ptAwbParam)
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
            COMM_ISP_PRT("Get AE Grid0/AWB Grid failed.\n");
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
            COMM_ISP_PRT("Get AE Grid1 failed.\n");
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
            COMM_ISP_PRT("Get AE Hist failed.\n");
            return AX_FALSE;
        }
    } while (0);
    ptAeParam->tHistRoi.nRoiOffsetH = tRoi.nRoiOffsetH;
    ptAeParam->tHistRoi.nRoiOffsetV = tRoi.nRoiOffsetV;
    ptAeParam->tHistRoi.nRoiWidth = tRoi.nRoiRegionW;
    ptAeParam->tHistRoi.nRoiHeight = tRoi.nRoiRegionH;

    return AX_TRUE;
}

static AX_S32 SetEZoomAeAwbIQ(AX_U8 nSnsId, AX_U8 nPipeId, AX_U32 nWidth, AX_U32 nHeight) {
    pthread_mutex_lock(&g_mtxCropRect);

    AX_S32 nRet = -1;

    if (nSnsId != g_tCropRectInfo.nSnsId || !g_tCropRectInfo.bValid) {
        goto _set_ezoom_iq_out;
    }

    g_tCropRectInfo.bValid = 0;

    AX_ISP_IQ_AE_STAT_PARAM_T stIspAeStatParam = {0};
    nRet = AX_ISP_IQ_GetAeStatParam(nPipeId, &stIspAeStatParam);
    if (AX_SUCCESS != nRet) {
        COMM_ISP_PRT("AX_ISP_IQ_GetAeStatParam failed ret=0x%x\n", nRet);
        goto _set_ezoom_iq_out;
    }
    AX_ISP_IQ_AWB_STAT_PARAM_T stAwbStatParam = {0};
    nRet = AX_ISP_IQ_GetAwbStatParam(nPipeId, &stAwbStatParam);
    if (AX_SUCCESS != nRet) {
        COMM_ISP_PRT("AX_ISP_IQ_GetAwbStatParam failed ret=0x%x\n", nRet);
        goto _set_ezoom_iq_out;
    }

    // COMM_ISP_PRT("Sns[%d]Pipe[%d] EZoom CropRect(%d, %d, %d, %d)\n",
    //                   nSnsId, nPipeId,
    //                   g_tCropRectInfo.tCropRect.nStartX, g_tCropRectInfo.tCropRect.nStartY,
    //                   g_tCropRectInfo.tCropRect.nWidth, g_tCropRectInfo.tCropRect.nHeight);

    if (!Get3ARoi(nWidth, nHeight, &g_tCropRectInfo.tCropRect, &stIspAeStatParam, &stAwbStatParam)) {
        COMM_ISP_PRT("Get 3A Roi failed frome EZoom crop[%d, %d, %d, %d]!\n",
                                                g_tCropRectInfo.tCropRect.nStartX,
                                                g_tCropRectInfo.tCropRect.nStartY,
                                                g_tCropRectInfo.tCropRect.nWidth,
                                                g_tCropRectInfo.tCropRect.nHeight);
        nRet = -1;
        goto _set_ezoom_iq_out;
    }

    nRet = AX_ISP_IQ_SetAeStatParam(nPipeId, &stIspAeStatParam);
    if (AX_SUCCESS != nRet) {
        COMM_ISP_PRT("AX_ISP_IQ_SetAeStatParam failed ret=0x%x\n", nRet);
        goto _set_ezoom_iq_out;
    }
    nRet = AX_ISP_IQ_SetAwbStatParam(nPipeId, &stAwbStatParam);
    if (AX_SUCCESS != nRet) {
        COMM_ISP_PRT("AX_ISP_IQ_SetAwbStatParam failed ret=0x%x\n", nRet);
    }

_set_ezoom_iq_out:
    pthread_mutex_unlock(&g_mtxCropRect);

    return nRet;
}

static AX_S32 RawFrameDispatch(AX_U8 nPipeId, AX_CAMERA_T *pCam, AX_SNS_HDR_MODE_E eHdrMode)
{
    AX_S32 axRet = 0;
    AX_S32 timeOutMs = 500;
    AX_IMG_INFO_T tImg[AX_SNS_HDR_FRAME_MAX] = {0};
    AX_ISP_RUNONCE_PARAM_T nIspParm;

    if (eHdrMode != AX_SNS_HDR_2X_MODE) {
        eHdrMode = AX_SNS_LINEAR_MODE;
    }

    for (AX_S32 i = 0; i < eHdrMode; i++) {
        axRet = AX_VIN_GetRawFrame(nPipeId, AX_VIN_PIPE_DUMP_NODE_IFE, (AX_SNS_HDR_FRAME_E)i, tImg + i, timeOutMs);
        if (axRet != 0) {
            if (AX_ERR_VIN_RES_EMPTY == axRet) {
                COMM_ISP_PRT("get raw frame failed, ret=0x%x\n", axRet);
                return axRet;
            }
            usleep(10 * 1000);
            return axRet;
        }
    }

    if (tImg[0].tIspInfo.tSnsSWitchInfo.bFirstFrmFlag) {
        // load bin
        if (tImg[0].tIspInfo.tSnsSWitchInfo.nSnsId == 1) {
            AX_ISP_LoadBinParams(nPipeId, pCam->tPipeInfo[0].szBinPath);
        } else if (tImg[0].tIspInfo.tSnsSWitchInfo.nSnsId == 2) {
            AX_ISP_LoadBinParams(nPipeId, pCam->tPipeInfo[1].szBinPath);
        }
    }

    SetEZoomAeAwbIQ(tImg[0].tIspInfo.tSnsSWitchInfo.nSnsId, nPipeId, pCam->tSnsAttr.nWidth, pCam->tSnsAttr.nHeight);

    if (tImg[0].tIspInfo.tSnsSWitchInfo.bFirstFrmFlag) {
        nIspParm.eCmdType = AX_ISP_EXT_CMD_SNS_SWITCH;
        nIspParm.bFirstFrmFlag = AX_TRUE;
        nIspParm.nSnsId = tImg[0].tIspInfo.tSnsSWitchInfo.nSnsId;
        nIspParm.eLensType = tImg[0].tIspInfo.tSnsSWitchInfo.eLensType;
        axRet = AX_ISP_RunOnceExt(nPipeId, &nIspParm);
        if (axRet != 0) {
            COMM_ISP_PRT("RunOnceExt failed, ret=0x%x\n", axRet);
        }
    }

    axRet = AX_VIN_SendRawFrame(nPipeId, AX_VIN_FRAME_SOURCE_ID_ITP, (AX_S8)eHdrMode,  (const AX_IMG_INFO_T **)&tImg, timeOutMs);
    if (axRet != 0) {
        COMM_ISP_PRT("Send Pipe raw frame failed, ret=0x%x\n", axRet);
    }

    for (AX_S32 i = 0; i < eHdrMode; i++) {
        axRet = AX_VIN_ReleaseRawFrame(nPipeId, AX_VIN_PIPE_DUMP_NODE_IFE, (AX_SNS_HDR_FRAME_E)i, tImg + i);
        if (axRet != 0) {
            COMM_ISP_PRT("realse raw frame failed, ret=0x%x\n", axRet);
        }
    }

    return 0;
}

static void *DispatchThread(void *args)
{
    AX_CAMERA_T *pCam = (AX_CAMERA_T *)args;
    AX_CHAR token[32] = {0};

    AX_U8 nPipeId = pCam->nPipeId;
    AX_SNS_HDR_MODE_E eHdrMode = pCam->eHdrMode;

    snprintf(token, 32, "RAW_DISP_%u", nPipeId);
    prctl(PR_SET_NAME, token);

    while (!g_DispatcherLoopExit[pCam->nNumber]) {
        RawFrameDispatch(nPipeId, pCam, eHdrMode);
    }

    return NULL;
}

AX_S32 Save_Raw_File(AX_VOID *frame_addr, int frame_size, int chn)
{
    AX_S32 axRet = 0;
    FILE *pstFile = NULL;
    AX_U8 file_name[128] = "";

    sprintf((char *)file_name, "out_chn_%d.raw", chn);
    pstFile = fopen((char *)file_name, "wb");
    if (pstFile == NULL) {
        COMM_ISP_PRT("fail to open video file !\n");
        return -1;
    }
    fwrite((char *)frame_addr, frame_size, 1, pstFile);
    if (pstFile) {
        fclose(pstFile);
    }
    return axRet;
}

AX_S32 Save_YUV_File(AX_VOID *frame_addr, int frame_size, int chn)
{
    AX_S32 axRet = 0;
    FILE *pstFile = NULL;
    AX_U8 file_name[128] = "";

    sprintf((char *)file_name, "out_chn_%d.yuv", chn);
    pstFile = fopen((char *)file_name, "wb");
    if (pstFile == NULL) {
        COMM_ISP_PRT("fail to open video file !\n");
        return -1;
    }

    fwrite((char *)frame_addr, frame_size, 1, pstFile);
    if (pstFile) {
        fclose(pstFile);
    }
    return axRet;
}

AX_S32 COMMON_CAM_CaptureFrameProc(AX_U32 nCapturePipeId, const AX_IMG_INFO_T *pImgInfo[])
{
    AX_S32 axRet = 0;
    AX_IMG_INFO_T capture_img_info = {0};
    AX_U32 nRefPipeId = 0;
    AX_ISP_IQ_AE_PARAM_T tUserCaptureFrameAeParam;
    AX_ISP_IQ_AWB_PARAM_T tUserCaptureFrameAwbParam;
    //AX_ISP_IQ_AINR_PARAM_T  tUserCaptureFrameAinrParam;

    /* use your capture raw frame's ae in manual mode, this is just a sample*/
    AX_ISP_IQ_GetAeParam(nRefPipeId, &tUserCaptureFrameAeParam);
    tUserCaptureFrameAeParam.nEnable = AX_FALSE;
    AX_ISP_IQ_SetAeParam(nCapturePipeId, &tUserCaptureFrameAeParam);

    /* use your capture raw frame's awb in manual mode, this is just a sample*/
    AX_ISP_IQ_GetAwbParam(nRefPipeId, &tUserCaptureFrameAwbParam);
    tUserCaptureFrameAwbParam.nEnable = AX_FALSE;
    AX_ISP_IQ_SetAwbParam(nCapturePipeId, &tUserCaptureFrameAwbParam);

    /* 1. first send raw frame*/
    AX_ISP_RunOnce(nCapturePipeId);

    axRet = AX_VIN_SendRawFrame(nCapturePipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_SNS_LINEAR_MODE,
                                pImgInfo, 3000);
    if (axRet != 0) {
        COMM_ISP_PRT("Send Pipe raw frame failed");
        return axRet;
    }
    /* The first frame data is invalid for the user */
    axRet = AX_VIN_GetYuvFrame(nCapturePipeId, AX_VIN_CHN_ID_MAIN, &capture_img_info, 3000);
    if (axRet != 0) {
        COMM_ISP_PRT("func:%s, return error!.\n", __func__);
        return axRet;
    }
    AX_VIN_ReleaseYuvFrame(nCapturePipeId, AX_VIN_CHN_ID_MAIN, &capture_img_info);

    AX_ISP_RunOnce(nCapturePipeId);

    axRet = AX_VIN_SendRawFrame(nCapturePipeId, AX_VIN_FRAME_SOURCE_ID_IFE, AX_SNS_LINEAR_MODE,
                                pImgInfo, 3000);
    if (axRet != 0) {
        COMM_ISP_PRT("Send Pipe raw frame failed");
        return axRet;
    }

    /* The second frame data is the final result frame */
    axRet = AX_VIN_GetYuvFrame(nCapturePipeId, AX_VIN_CHN_ID_MAIN, &capture_img_info, 3000);
    if (axRet != 0) {
        COMM_ISP_PRT("func:%s, return error!.\n", __func__);
        return axRet;
    }

    /* Users can use second YUV frame for application development */

    /* User Code */
    /* ...... */

    AX_VIN_ReleaseYuvFrame(nCapturePipeId, AX_VIN_CHN_ID_MAIN, &capture_img_info);

    COMM_ISP_PRT("Capture Frame Proc success.\n");
    return AX_SUCCESS;
}

static void SetEZoomIQCropRect(AX_U8 nSnsId, AX_U32 nImgWidth, AX_U32 nImgHeight, AX_WIN_AREA_T* pCropRect, AX_VIN_ROTATION_E eRotation) {
    pthread_mutex_lock(&g_mtxCropRect);

    g_tCropRectInfo.nSnsId = nSnsId;
    g_tCropRectInfo.bValid = 1;

    switch (eRotation) {
        case AX_VIN_ROTATION_0:
            g_tCropRectInfo.tCropRect = *pCropRect;
            break;
        case AX_VIN_ROTATION_90:
            g_tCropRectInfo.tCropRect.nStartX = nImgWidth - pCropRect->nStartY - pCropRect->nHeight;
            g_tCropRectInfo.tCropRect.nStartY = pCropRect->nStartX;
            g_tCropRectInfo.tCropRect.nWidth  = pCropRect->nHeight;
            g_tCropRectInfo.tCropRect.nHeight = pCropRect->nWidth;
            break;
        case AX_VIN_ROTATION_180:
            g_tCropRectInfo.tCropRect.nStartX = nImgWidth - pCropRect->nStartX - pCropRect->nWidth;
            g_tCropRectInfo.tCropRect.nStartY = nImgHeight - pCropRect->nStartY - pCropRect->nHeight;
            g_tCropRectInfo.tCropRect.nWidth  = pCropRect->nWidth;
            g_tCropRectInfo.tCropRect.nHeight = pCropRect->nHeight;
            break;
        case AX_VIN_ROTATION_270:
            g_tCropRectInfo.tCropRect.nStartX = pCropRect->nStartY;
            g_tCropRectInfo.tCropRect.nStartY = nImgHeight - pCropRect->nStartX - pCropRect->nWidth;
            g_tCropRectInfo.tCropRect.nWidth  = pCropRect->nHeight;
            g_tCropRectInfo.tCropRect.nHeight = pCropRect->nWidth;
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&g_mtxCropRect);
}

AX_S32 COMMON_CAM_EZoomSwitch(AX_CAMERA_T * pCam, AX_F32 fZoomRatio) {
    AX_S32 nRet = 0;
    AX_F32 fSwitchEZoomOneCam = pCam->tEzoomSwitchInfo.fSwitchRatio;
    AX_VIN_CROP_INFO_T tCropInfo;
    AX_VIN_ROTATION_E eRotation = AX_VIN_ROTATION_0;
    AX_S32 nSnsId = 1;

    if (fZoomRatio < 1.0 || fZoomRatio > MAX_SWITCH_ZOOM_RATIO) {
        COMM_ISP_PRT("fZoomRatio = %f is invalid\n", fZoomRatio);
        return AX_FALSE;
    }

    AX_S16 nCenterOffsetX = 0;
    AX_S16 nCenterOffsetY = 0;

    if (fZoomRatio >= fSwitchEZoomOneCam) {
        fZoomRatio = fZoomRatio - fSwitchEZoomOneCam + 1.0;
        nSnsId = 2;
    } else {
        nCenterOffsetX = pCam->tEzoomSwitchInfo.nCx;
        nCenterOffsetY = pCam->tEzoomSwitchInfo.nCy;
    }

    nRet = AX_VIN_GetChnCropExt(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, nSnsId, &tCropInfo);
    if (AX_SUCCESS != nRet) {
        COMM_ISP_PRT("AX_VIN_GetChnCropExt(pipe: %d, sns: %d) failed ret=0x%x\n", pCam->nPipeId, nSnsId, nRet);
        return nRet;
    }

    nRet = AX_VIN_GetChnRotation(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, &eRotation);
    if (AX_SUCCESS != nRet) {
        COMM_ISP_PRT("AX_VIN_GetChnRotation(pipe: %d) failed ret=0x%x\n", pCam->nPipeId, nRet);
    }
    AX_U32 nWidth = pCam->tChnAttr[AX_VIN_CHN_ID_MAIN].nWidth;
    AX_U32 nHeight = pCam->tChnAttr[AX_VIN_CHN_ID_MAIN].nHeight;
    switch (eRotation) {
        case AX_VIN_ROTATION_90:
            COMM_SWAP_U32(nWidth, nHeight);
            COMM_SWAP_S16(nCenterOffsetX, nCenterOffsetY);
            nCenterOffsetX = -nCenterOffsetX;
            break;
        case AX_VIN_ROTATION_180:
            nCenterOffsetX = -nCenterOffsetX;
            nCenterOffsetY = -nCenterOffsetY;
            break;
        case AX_VIN_ROTATION_270:
            COMM_SWAP_U32(nWidth, nHeight);
            COMM_SWAP_S16(nCenterOffsetX, nCenterOffsetY);
            nCenterOffsetY = -nCenterOffsetY;
            break;
        default:
            break;
    }

    if (fZoomRatio == 1.0) {
        tCropInfo.bEnable = AX_FALSE;
        tCropInfo.tCropRect.nStartX = 0;
        tCropInfo.tCropRect.nStartY = 0;
        tCropInfo.tCropRect.nWidth  = nWidth;
        tCropInfo.tCropRect.nHeight = nHeight;
        nRet = AX_VIN_SetChnCropExt(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, nSnsId, &tCropInfo);
        if (AX_SUCCESS != nRet) {
            COMM_ISP_PRT("AX_VIN_SetChnCropExt(pipe: %d, sns: %d, bEnable: false) failed ret=0x%x\n", pCam->nPipeId, nSnsId, nRet);
            return nRet;
        }
    } else {
        tCropInfo.bEnable = AX_TRUE;
        // Cal Scaled width
        tCropInfo.tCropRect.nWidth  = (AX_U32)(nWidth  * 1.0f / fZoomRatio);
        tCropInfo.tCropRect.nHeight = (AX_U32)(nHeight * 1.0f / fZoomRatio);

        // Cal x y
        tCropInfo.tCropRect.nStartX = AX_MAX((AX_S32)((nWidth / 2) - (tCropInfo.tCropRect.nWidth / 2) + nCenterOffsetX * fZoomRatio / fSwitchEZoomOneCam), 0);
        tCropInfo.tCropRect.nStartY = AX_MAX((AX_S32)((nHeight / 2) - (tCropInfo.tCropRect.nHeight / 2) + nCenterOffsetY * fZoomRatio / fSwitchEZoomOneCam), 0);

        if (tCropInfo.tCropRect.nStartX + tCropInfo.tCropRect.nWidth > nWidth) {
            tCropInfo.tCropRect.nWidth = nWidth - tCropInfo.tCropRect.nStartX;
        }

        if (tCropInfo.tCropRect.nStartY + tCropInfo.tCropRect.nHeight > nHeight) {
            tCropInfo.tCropRect.nHeight = nHeight - tCropInfo.tCropRect.nStartY;
        }

        // COMM_ISP_PRT("sns[%d] crop info enable=%d, coord_mode=%d, rect=[%d, %d, %d, %d]\n", pCam->nPipeId, tCropInfo.bEnable, tCropInfo.eCoordMode,
        //     tCropInfo.tCropRect.nStartX, tCropInfo.tCropRect.nStartY, tCropInfo.tCropRect.nWidth, tCropInfo.tCropRect.nHeight);

        nRet = AX_VIN_SetChnCropExt(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, nSnsId, &tCropInfo);
        if (AX_SUCCESS != nRet) {
            COMM_ISP_PRT("AX_VIN_SetChnCropExt(pipe: %d, sns: %d) failed ret=0x%x\n", pCam->nPipeId, nSnsId, nRet);
            return nRet;
        }
    }

    SetEZoomIQCropRect(nSnsId, pCam->tChnAttr[AX_VIN_CHN_ID_MAIN].nWidth, pCam->tChnAttr[AX_VIN_CHN_ID_MAIN].nHeight, &tCropInfo.tCropRect, eRotation);

    if (nSnsId != pCam->nSwitchSnsId) {
        COMM_ISP_PRT("switch to sns %d...\n", nSnsId);
        nRet = ax_mipi_switch_change(nSnsId);
        if (AX_SUCCESS != nRet) {
            COMM_ISP_PRT("mipi_switch_change to sns(%d) failed with ret: 0x%x\n", nSnsId, nRet);
            return nRet;
        }
        pCam->nSwitchSnsId = nSnsId;
    }

    return nRet;
}

static void *CheckI2CReadyThread(void *args) {
    AX_CAMERA_T *pCam = (AX_CAMERA_T *)args;
    AX_CHAR token[32] = {0};
    AX_U8 nPipeId = pCam->nPipeId;
    long vaddr = 0;
    int fd = 0;

    snprintf(token, 32, "CHK_I2C_%u", nPipeId);
    prctl(PR_SET_NAME, token);

    AX_U8 nCheckBit = 6; // I2C-6
    if (pCam->nSwitchSnsId == 1) {
        nCheckBit = 0; // I2C-0
    }

    fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (-1 == fd) {
        return 0;
    }
    vaddr = (long)mmap(0, 0x1000, PROT_READ, MAP_SHARED, fd, 0x2340000);
    if (0 == vaddr) {
        close(fd);
        return 0;
    }
    AX_U8 nValue = *(volatile unsigned char *)(vaddr + 0x22C);

    while (((nValue >> nCheckBit) & 0x1) != 0) {
        usleep(10000);
        nValue = *(volatile unsigned char *)(vaddr + 0x22C);
    }

    usleep(10000);

    if (pCam->nSwitchSnsId == 1) {
        // init -> acitve -> stream on for sns 2
        if (pCam->ptSnsHdl1 && pCam->ptSnsHdl1->pfn_sensor_init) {
            pCam->ptSnsHdl1->pfn_sensor_init(2);
        }
        AX_ISP_SetSnsActive(nPipeId, 2, 1);
        if (pCam->ptSnsHdl1 && pCam->ptSnsHdl1->pfn_sensor_streaming_ctrl) {
            pCam->ptSnsHdl1->pfn_sensor_streaming_ctrl(2, 1);
        }
    } else {
        // init -> acitve -> stream on for sns 1
        if (pCam->ptSnsHdl && pCam->ptSnsHdl->pfn_sensor_init) {
            pCam->ptSnsHdl->pfn_sensor_init(1);
        }
        AX_ISP_SetSnsActive(nPipeId, 1, 1);

        if (pCam->ptSnsHdl && pCam->ptSnsHdl->pfn_sensor_streaming_ctrl) {
            pCam->ptSnsHdl->pfn_sensor_streaming_ctrl(1, 1);
        }
    }

    close(fd);

    return NULL;
}