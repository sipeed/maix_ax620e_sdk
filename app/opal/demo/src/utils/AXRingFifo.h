/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_RTSP_WRAPPER_H__
#define __AX_RTSP_WRAPPER_H__

#include "ax_base_type.h"
#include "ax_global_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _axring_data {
    AX_VOID  *buf;
    AX_U32    len;
} AX_RINGFIFO_DATA_T;

typedef struct _axring_data_info {
    AX_RINGFIFO_DATA_T data[2];
    AX_BOOL bIFrame;
    AX_U64  nPts;
    AX_VOID* pRrvData;
} AX_RINGFIFO_ELEMENT_T;


typedef AX_VOID* AX_RINGFIFO_HANDLE;

AX_S32 AX_RingFifo_Init(AX_RINGFIFO_HANDLE *pHandle, AX_S32 nSize, const AX_CHAR* pszName);
AX_S32 AX_RingFifo_Deinit(AX_RINGFIFO_HANDLE pHandle);

AX_S32 AX_RingFifo_Put(AX_RINGFIFO_HANDLE pHandle, const AX_VOID *buf, AX_U32 len, AX_U64 nPts, AX_BOOL bIFrame, AX_U8* pHeadBuf, AX_U32 nHeadSize);
AX_S32 AX_RingFifo_Get(AX_RINGFIFO_HANDLE pHandle, AX_RINGFIFO_ELEMENT_T* pData);
AX_S32 AX_RingFifo_Pop(AX_RINGFIFO_HANDLE pHandle, AX_BOOL bForce);
AX_S32 AX_RingFifo_Free(AX_RINGFIFO_HANDLE pHandle, AX_RINGFIFO_ELEMENT_T* pData, AX_BOOL bForce);
AX_U32 AX_RingFifo_Size(AX_RINGFIFO_HANDLE pHandle);
AX_S32 AX_RingFifo_Clear(AX_RINGFIFO_HANDLE pHandle);
AX_U32 AX_RingFifo_Capacity(AX_RINGFIFO_HANDLE pHandle);
AX_RINGFIFO_HANDLE * AX_RingFifo_GetHandle(AX_RINGFIFO_ELEMENT_T* pData);

#ifdef __cplusplus
}
#endif

#endif  // __AX_RTSP_WRAPPER_H__
