/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __COMMON_CAM_H__
#define __COMMON_CAM_H__

#include "ax_base_type.h"
#include "common_vin.h"
#include "common_isp.h"
#include "common_sys.h"
#include "ax_engine_api.h"
#include "ax_sensor_struct.h"
#include <pthread.h>

#define MAX_FILE_NAME_CHAR_SIZE       (128)
#define MAX_CAMERAS                   (2)
#define MAX_SWITCH_ZOOM_RATIO         (8)

#ifndef COMM_CAM_PRT
#define COMM_CAM_PRT(fmt...)   \
do {\
    printf("[COMM_CAM][%s][%5d] ", __FUNCTION__, __LINE__);\
    printf(fmt);\
}while(0)
#endif


typedef enum {
    SAMPLE_PIPE_MODE_VIDEO,
    SAMPLE_PIPE_MODE_PICTURE,
    SAMPLE_PIPE_MODE_FLASH_SNAP,      /* Snap of flash lamp */
    SAMPLE_PIPE_MODE_MAX,
} SAMPLE_PIPE_MODE_E;

typedef enum {
    SAMPLE_PIPE_COMB_MODE_NONE = 0,         /* no combined  */
    SAMPLE_PIPE_COMB_MODE0,                 /* combined mode0 type  */
    SAMPLE_PIPE_COMB_MODE_MAX,
} SAMPLE_PIPE_COMB_MODE_E;

typedef struct {
    SAMPLE_PIPE_MODE_E  ePipeMode;
    AX_BOOL             bAiispEnable;
    AX_CHAR             szBinPath[128];
    SAMPLE_PIPE_COMB_MODE_E eCombMode;
} SAMPLE_PIPE_INFO_T;

typedef struct {
    AX_U32 nX;
    AX_U32 nY;
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_S16 nCx;
    AX_S16 nCy;
    AX_F32 fSwitchRatio;
} SAMPLE_EZOOM_SWITCH_INFO_T;

typedef struct {
    AX_U8          nSnsId;
    AX_U8          bValid;
    AX_WIN_AREA_T  tCropRect;
} SAMPLE_CROP_RECT_INFO_T;

typedef struct {
    /* common parameters */
    AX_U8                   nNumber;
    AX_BOOL                 bOpen;
    AX_SNS_HDR_MODE_E       eHdrMode;
    SAMPLE_SNS_TYPE_E       eSnsType;
    COMMON_VIN_MODE_E       eSysMode;
    AX_SNS_CONNECT_TYPE_E   eBusType;
    SAMPLE_LOAD_RAW_NODE_E  eLoadRawNode;
    AX_INPUT_MODE_E         eInputMode;

    AX_U32                  nRxDev;
    AX_U8                   nDevId;
    AX_U8                   nPipeId;
    AX_U8                   nI2cAddr;

    /* Resource Control Parameters */
    AX_BOOL                 bRegisterSns;
    AX_BOOL                 bEnableDev;     /* loadraw mode, it is not necessary to enable dev */

    /* Isp processing thread */
    pthread_t               tIspProcThread[AX_VIN_MAX_PIPE_NUM];
    pthread_t               tIspAFProcThread;

    /* Module Attribute Parameters */
    AX_MIPI_RX_ATTR_T       tMipiAttr;
    AX_SNS_ATTR_T           tSnsAttr;
    AX_SNS_CLK_ATTR_T       tSnsClkAttr;
    AX_VIN_DEV_ATTR_T       tDevAttr;
    AX_VIN_DEV_BIND_PIPE_T  tDevBindPipe;
    AX_VIN_PIPE_ATTR_T      tPipeAttr;
    SAMPLE_PIPE_INFO_T      tPipeInfo[AX_VIN_MAX_PIPE_NUM];
    AX_VIN_CHN_ATTR_T       tChnAttr[AX_VIN_CHN_ID_MAX];
    AX_BOOL                 bChnEn[AX_VIN_CHN_ID_MAX];
    AX_S32                  nChnFrmMode[AX_VIN_CHN_ID_MAX];

    /* 3A Parameters */
    AX_BOOL                 bUser3a;
    AX_ISP_AE_REGFUNCS_T    tAeFuncs;
    AX_ISP_AWB_REGFUNCS_T   tAwbFuncs;
    AX_ISP_AF_REGFUNCS_T    tAfFuncs;
    AX_ISP_LSC_REGFUNCS_T   tLscFuncs;

    AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl;

    /* mipi switch config */
    AX_BOOL                    bMipiSwitchEnable;
    AX_BOOL                    bRegisterSns1;
    AX_U8                      nSwitchSnsId;
    AX_U8                      nI2cAddr1;
    AX_SENSOR_REGISTER_FUNC_T *ptSnsHdl1;
    SAMPLE_EZOOM_SWITCH_INFO_T tEzoomSwitchInfo;
} AX_CAMERA_T;

AX_S32 COMMON_NPU_Init();
AX_S32 COMMON_CAM_Init(AX_VOID);
AX_S32 COMMON_CAM_Deinit(AX_VOID);
AX_S32 COMMON_CAM_PrivPoolInit(COMMON_SYS_ARGS_T *pPrivPoolArgs);

AX_S32 COMMON_CAM_Open(AX_CAMERA_T *pCamList, AX_U8 Num);
AX_S32 COMMON_CAM_Close(AX_CAMERA_T *pCamList, AX_U8 Num);

AX_S32 COMMON_CAM_EZoomSwitch(AX_CAMERA_T * pCam, AX_F32 fZoomRatio);
#endif //__COMMON_CAM_H__
