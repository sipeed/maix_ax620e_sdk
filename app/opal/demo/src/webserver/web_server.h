/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include "ax_opal_type.h"
#include "web_helper.h"

typedef enum _WEB_EVENTS_TYPE_E {
    E_WEB_EVENTS_TYPE_ReStartPreview = 0,
    E_WEB_EVENTS_TYPE_MD,
    E_WEB_EVENTS_TYPE_OD,
    E_WEB_EVENTS_TYPE_SCD,
    E_WEB_EVENTS_TYPE_LogOut,
    E_WEB_EVENTS_TYPE_MAX
} WEB_EVENTS_TYPE_E;

typedef struct _AI_EVENTS_MD_INFO_T {
    AX_CHAR szDisplay[64];
    AX_U32 nAreaID;
    AX_U64 nReserved;
} AI_EVENTS_MD_INFO_T;

typedef struct _AI_EVENTS_OD_INFO_T {
    AX_CHAR szDisplay[64];
    AX_U32 nAreaID;
    AX_U64 nReserved;
} AI_EVENTS_OD_INFO_T;

typedef struct _AI_EVENTS_SCD_INFO_T {
    AX_CHAR szDisplay[64];
    AX_U32 nAreaID;
    AX_U64 nReserved;
} AI_EVENTS_SCD_INFO_T;

typedef struct _WEB_EVENTS_DATA {
    WEB_EVENTS_TYPE_E eType;
    AX_U64 nTime;
    AX_U32 nReserved;
    union {
        AI_EVENTS_MD_INFO_T tMD;
        AI_EVENTS_OD_INFO_T tOD;
        AI_EVENTS_SCD_INFO_T tSCD;
    };
} WEB_EVENTS_DATA_T, *WEB_EVENTS_DATA_PTR;

#ifdef __cplusplus
extern "C" {
#endif

AX_BOOL WS_Init();
AX_BOOL WS_DeInit();
AX_BOOL WS_Start();
AX_BOOL WS_Stop();
AX_VOID WS_SendPreviewData(AX_U8 nSnsId, AX_U8 nSrcChn, AX_U8* pData, AX_U32 size, AX_U64 u64Pts, AX_BOOL bIFrame);
AX_VOID WS_SendPushImgData(AX_U8 nSnsId, AX_VOID* data, AX_U32 size, AX_U64 nPts, AX_BOOL bIFrame, JPEG_DATA_INFO_T* pJpegInfo);
AX_VOID WS_SendSnapshotData(AX_VOID* data, AX_U32 size, AX_VOID* conn);
AX_BOOL WS_SendEventsData(WEB_EVENTS_DATA_T* data);
AX_VOID WS_SendAudioData(AX_VOID* data, AX_U32 size, AX_U64 nPts);
AX_BOOL WS_SendWebResetEvent(AX_U8 nSnsId);
AX_VOID WS_EnableAudioPlay(AX_BOOL bEnable);
AX_VOID WS_EnableAudioCapture(AX_BOOL bEnable);
AX_BOOL WS_IsRunning();

#ifdef __cplusplus
}
#endif

#endif // _WEB_SERVER_H_