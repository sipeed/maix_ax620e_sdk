/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_ELEIVPS_H_
#define _AX_OPAL_MAL_ELEIVPS_H_

#include "ax_opal_mal_def.h"
#include "ax_opal_thread.h"
#include "ax_ivps_api.h"

#define AX_OPAL_MAX_CHN_OSD_CNT (6)

typedef struct _AX_OPAL_IVPS_FRAMERATE_T {
    AX_F32 fSrc;
    AX_F32 fDst;
} AX_OPAL_IVPS_FRAMERATE_T;

typedef struct _AX_OPAL_IVPS_RESOLUTION_T {
    AX_S32 nWidth;
    AX_S32 nHeight;
} AX_OPAL_IVPS_RESOLUTION_T;

typedef struct _AX_OPAL_FRAME_COMPRESS_INFO_T {
    AX_U32  enCompressMode;
    AX_U32  u32CompressLevel;
} AX_OPAL_FRAME_COMPRESS_INFO_T;

typedef struct _AX_OPAL_IVPS_GRP_ATTR_T {
    AX_S32 eEngineType0;                            /* engine of group filter 0 */
    AX_S32 eEngineType1;                            /* engine of group filter 1 */
    AX_OPAL_IVPS_FRAMERATE_T tFramerate;
    AX_OPAL_IVPS_RESOLUTION_T tResolution;
    AX_S32 nFifoDepth;                              /* in fifo */
    AX_OPAL_FRAME_COMPRESS_INFO_T tCompress;
    AX_BOOL bInplace0;                              /* inplace of group filter 0 */
    AX_BOOL bInplace1;                              /* inplace of group filter 1 */
} AX_OPAL_IVPS_GRP_ATTR_T;

typedef struct _AX_OPAL_IVPS_CHN_ATTR_T {
    AX_BOOL bEnable;
    AX_S32 eEngineType0;                            /* engine of channel filter 0 */
    AX_S32 eEngineType1;                            /* engine of channel filter 1 */
    AX_OPAL_IVPS_FRAMERATE_T tFramerate;
    AX_OPAL_IVPS_RESOLUTION_T tResolution;
    AX_S32 nFifoDepth;                              /* out fifo */
    AX_OPAL_FRAME_COMPRESS_INFO_T tCompress;
    AX_BOOL bInplace0;                              /* inplace of channel filter 0 */
    AX_BOOL bInplace1;                              /* inplace of channel filter 1 */
    AX_S32  eSclType;
    AX_BOOL bVoOsd;
    AX_BOOL bVoRect;
} AX_OPAL_IVPS_CHN_ATTR_T;

typedef struct _AX_OPAL_IVPS_ATTR_T {
    AX_S32 nGrpChnCnt;
    AX_OPAL_IVPS_GRP_ATTR_T tGrpAttr;
    AX_OPAL_IVPS_CHN_ATTR_T arrChnAttr[AX_OPAL_MAX_CHN_CNT];
    AX_BOOL bMaskEnable;
} AX_OPAL_IVPS_ATTR_T;

typedef struct {
    AX_BOOL bInit;
    AX_S32 nOsdHandle;
    AX_OPAL_OSD_ATTR_T stOsdAttr;
} AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T;

typedef struct _AX_OPAL_MAL_RECT_THREADPARAM_T {
    AX_S32 nGrpId;
    AX_S32 nChnId;
    AX_U32 nRectNum;
    AX_IVPS_RGN_POLYGON_T stRects[AX_IVPS_REGION_MAX_DISP_NUM];
    AX_U32 nPolygonNum;
    AX_IVPS_RGN_POLYGON_T stPolygons[AX_IVPS_REGION_MAX_DISP_NUM];
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pChnOsd;
    AX_OPAL_THREAD_T *pRectThread;
    AX_OPAL_MAL_ELE_HANDLE elehdl;
} AX_OPAL_MAL_RECT_THREADPARAM_T;

typedef struct _AX_OPAL_MAL_TIMEOSD_THREADPARAM_T {
    AX_S32 nGrpId;
    AX_S32 nChnId;
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T *pChnOsd;
    AX_OPAL_THREAD_T *pTimeThread;
    AX_OPAL_MAL_ELE_HANDLE elehdl;
} AX_OPAL_MAL_TIMEOSD_THREADPARAM_T;

typedef struct _AX_OPAL_MAL_IVPS_FRAME_THREADPARAM_T {
    AX_S32 nUniGrpId;
    AX_S32 nUniChnId;
} AX_OPAL_MAL_IVPS_FRAME_THREADPARAM_T;

typedef struct {
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T arrChnOsdAttr[AX_OPAL_MAX_CHN_CNT][AX_OPAL_MAX_CHN_OSD_CNT];
    AX_OPAL_MAL_IVPS_CHN_OSD_ATTR_T arrChnOsdRect[AX_OPAL_MAX_CHN_CNT][2];
    AX_OPAL_MAL_RECT_THREADPARAM_T arrThreadRectParam[AX_OPAL_MAX_CHN_CNT][2];
    AX_OPAL_MAL_TIMEOSD_THREADPARAM_T arrThreadTimeParam[AX_OPAL_MAX_CHN_CNT];
    AX_OPAL_MAL_IVPS_FRAME_THREADPARAM_T arrThreadFrameParam[AX_OPAL_MAX_CHN_CNT];
    AX_OPAL_THREAD_T *pThreadGetStream;
} AX_OPAL_MAL_IVPS_ATTR_T;

typedef struct _AX_OPAL_MAL_IVPS_ID_T {
    AX_S32 nGrpId;
    AX_S32 arrChnId[AX_OPAL_MAX_CHN_CNT];
} AX_OPAL_MAL_IVPS_ID_T;

typedef struct _AX_OPAL_MAL_ELEIVPS_T {
    AX_OPAL_MAL_ELE_T stBase;
    AX_OPAL_IVPS_ATTR_T stOpalIvpsAttr;

    AX_OPAL_MAL_IVPS_ATTR_T stEleIvpsAttr;
} AX_OPAL_MAL_ELEIVPS_T;

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELEIVPS_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr);
AX_S32 AX_OPAL_MAL_ELEIVPS_Destroy(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEIVPS_Start(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEIVPS_Stop(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEIVPS_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);

#endif // _AX_OPAL_MAL_ELECAM_H_
