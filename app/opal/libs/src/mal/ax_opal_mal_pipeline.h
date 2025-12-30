/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_PIPELINE_H_
#define _AX_OPAL_MAL_PIPELINE_H_

#include "ax_opal_mal_def.h"

AX_OPAL_MAL_PPL_HANDLE AX_OPAL_MAL_PPL_Create(AX_OPAL_PPL_ATTR_T *pPplAttr, const AX_OPAL_ATTR_T *pstAttr);
AX_S32 AX_OPAL_MAL_PPL_Destroy(AX_OPAL_MAL_PPL_HANDLE self);

AX_S32 AX_OPAL_MAL_PPL_Process(AX_OPAL_MAL_PPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);

AX_OPAL_MAL_SUBPPL_HANDLE AX_OPAL_MAL_SUBPPL_Create(AX_OPAL_MAL_PPL_HANDLE parent, AX_OPAL_SUBPPL_ATTR_T *pSubPplAttr);
AX_S32 AX_OPAL_MAL_SUBPPL_Destroy(AX_OPAL_MAL_SUBPPL_HANDLE self);
AX_S32 AX_OPAL_MAL_SUBPPL_Process(AX_OPAL_MAL_SUBPPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData);
AX_S32 AX_OPAL_MAL_SUBPPL_GetGrpId(AX_OPAL_MAL_SUBPPL_HANDLE self, AX_S32 nEleId);
AX_S32 AX_OPAL_MAL_SUBPPL_GetChnId(AX_OPAL_MAL_SUBPPL_HANDLE self, AX_S32 nEleId);

#endif // _AX_OPAL_MAL_PIPELINE_H_