/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_ELE_H_
#define _AX_OPAL_MAL_ELE_H_

#include "ax_opal_mal_def.h"

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELE_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr);
AX_S32 AX_OPAL_MAL_ELE_Destroy(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELE_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);

AX_VOID AX_OPAL_MAL_ELE_MappingGrpChn(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELE_GetUniGrpId(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELE_GetUniChnId(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nGrpId, AX_S32 nChnId);
AX_S32 AX_OPAL_MAL_ELE_GetGrpId(AX_OPAL_MAL_ELE_HANDLE self);
AX_S32 AX_OPAL_MAL_ELE_GetChnId(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nUniGrpId, AX_S32 nUniChnId);
AX_BOOL AX_OPAL_MAL_ELE_CheckUniGrpChnId(AX_OPAL_MAL_ELE_HANDLE self, AX_S32 nUniGrpId, AX_S32 nUniChnId);

#endif // _AX_OPAL_MAL_ELE_H_