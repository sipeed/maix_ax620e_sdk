/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_UTILS_H_
#define _AX_OPAL_MAL_UTILS_H_

#include "ax_opal_mal_def.h"
#include "ax_opal_mal_pipeline.h"

AX_OPAL_VIDEO_SNS_ATTR_T *AX_OPAL_MAL_GetSnsAttr(AX_OPAL_MAL_ELE_HANDLE hdlEle);
AX_OPAL_VIDEO_CHN_ATTR_T *AX_OPAL_MAL_GetChnAttr(AX_OPAL_MAL_ELE_HANDLE hdlEle, AX_S32 nEleGrpId, AX_S32 nEleChnId);

#endif // _AX_OPAL_MAL_UTILS_H_