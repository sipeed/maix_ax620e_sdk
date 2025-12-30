/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_ELESYS_H_
#define _AX_OPAL_MAL_ELESYS_H_

#include "ax_opal_mal_def.h"

typedef struct _AX_OPAL_MAL_ELESYS_T {
    AX_OPAL_MAL_ELE_T stBase;
} AX_OPAL_MAL_ELESYS_T;

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELESYS_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr);
AX_S32 AX_OPAL_MAL_ELESYS_Destroy(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELESYS_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);

#endif // _AX_OPAL_MAL_ELESYS_H_