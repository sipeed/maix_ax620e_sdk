/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_MAL_ELEDETECT_H_
#define _AX_OPAL_MAL_ELEDETECT_H_

#include "ax_opal_mal_def.h"
#include "ax_opal_thread.h"

typedef struct _AX_OPAL_MAL_ELEALGO_T {
    AX_OPAL_MAL_ELE_T stBase;
    AX_U32 nSnsID;
    AX_U32 nChnID;
    AX_OPAL_VIDEO_CHN_ATTR_T stAttr;
} AX_OPAL_MAL_ELEALGO_T;

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELEALGO_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr);
AX_S32 AX_OPAL_MAL_ELEALGO_Destroy(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEALGO_Start(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEALGO_Stop(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELEALGO_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);
AX_S32 AX_OPAL_MAL_ELEALGO_ProcData(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nUniGrpId, AX_S32 nUniChnId, AX_OPAL_UNITLINK_TYPE_E eLinkType, AX_VOID* pData, AX_U32 nSize);

#endif // _AX_OPAL_MAL_ELEDETECT_H_