/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_ELEVENC_H_
#define _AX_OPAL_MAL_ELEVENC_H_

#include "ax_opal_mal_def.h"
#include "ax_opal_thread.h"

typedef struct _AX_OPAL_VIDEO_PKT_THREAD_T {
    AX_S32 nUniGrpId;
    AX_S32 nUniChnId;
    AX_OPAL_THREAD_T *pVideoPktThread;
    AX_OPAL_VIDEO_PKT_CALLBACK_T stVideoPktCb;
} AX_OPAL_VIDEO_PKT_THREAD_T;

typedef struct _AX_OPAL_MAL_ELEVENC_T {
    AX_OPAL_MAL_ELE_T stBase;
    AX_OPAL_VIDEO_CHN_ATTR_T *pstVideoChnAttr;
    AX_OPAL_VIDEO_PKT_THREAD_T stPktThreadAttr;
} AX_OPAL_MAL_ELEVENC_T;

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELEVENC_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr);
AX_S32 AX_OPAL_MAL_ELEVENC_Destroy(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEVENC_Start(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEVENC_Stop(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEVENC_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);

#endif // _AX_OPAL_MAL_ELEVENC_H_