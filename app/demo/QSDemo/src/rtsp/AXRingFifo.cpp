/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "AXRingFifo.h"
#include "AXRingBufferEx.h"

AX_S32 AX_RingFifo_Init(AX_RINGFIFO_HANDLE *pHandle, AX_S32 nSize, const AX_CHAR* pszName) {
    CAXRingBufferEx *rb = new CAXRingBufferEx(nSize, 1, pszName);
    if (rb) {
        if (pHandle) {
            *pHandle = (AX_RINGFIFO_HANDLE)rb;
        }
    }

    return 0;
}

AX_S32 AX_RingFifo_Deinit(AX_RINGFIFO_HANDLE pHandle) {
    if (pHandle) {
        delete (CAXRingBufferEx *)pHandle;
    }
    return 0;
}

AX_S32 AX_RingFifo_Put(AX_RINGFIFO_HANDLE pHandle, const AX_VOID *buf, AX_U32 len, AX_U64 nPts, AX_BOOL bIFrame) {
    if (!pHandle || !buf || len == 0) {
        return 1;
    }

    CAXRingBufferEx *rb = (CAXRingBufferEx *)pHandle;
    CAXRingElementEx ele((AX_U8*)buf, len, nPts, bIFrame);
    if (rb->Put(ele)) {
        return 0;
    }

    return 1;
}

AX_S32 AX_RingFifo_Get(AX_RINGFIFO_HANDLE pHandle, AX_RINGFIFO_ELEMENT_PTR pData) {
    if (!pHandle || !pData) {
        return 1;
    }

    CAXRingBufferEx *rb = (CAXRingBufferEx *)pHandle;
    CAXRingElementEx *pEle = rb->Get();
    if (pEle) {
        pData->pRrvData = (AX_VOID*)pEle;
        pData->nPts = pEle->nPts;
        pData->bIFrame = pEle->bIFrame;
        pData->data[0].buf = pEle->pBuf;
        pData->data[0].len = pEle->nSize;
        pData->data[1].buf = pEle->pBuf2;
        pData->data[1].len = pEle->nSize2;
        return 0;
    }

    return 1;
}

AX_S32 AX_RingFifo_Pop(AX_RINGFIFO_HANDLE pHandle) {
    if (pHandle) {
        CAXRingBufferEx *rb = (CAXRingBufferEx *)pHandle;
        if(rb->Pop()) {
            return 0;
        }
    }
    return 1;
}

AX_U32 AX_RingFifo_Size(AX_RINGFIFO_HANDLE pHandle) {
    if (pHandle) {
        CAXRingBufferEx *rb = (CAXRingBufferEx *)pHandle;
        return rb->Size();
    }
    return 0;
}

AX_S32 AX_RingFifo_Clear(AX_RINGFIFO_HANDLE pHandle) {
    if (pHandle) {
        CAXRingBufferEx *rb = (CAXRingBufferEx *)pHandle;
        rb->Clear();
        return 0;
    }
    return 1;
}

AX_U32 AX_RingFifo_Capacity(AX_RINGFIFO_HANDLE pHandle) {
    if (pHandle) {
        CAXRingBufferEx *rb = (CAXRingBufferEx *)pHandle;
        return rb->Capacity();
    }
    return 0;
}