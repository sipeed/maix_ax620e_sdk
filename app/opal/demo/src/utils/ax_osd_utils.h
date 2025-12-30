/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_OSD_UTILS_H__
#define __AX_OSD_UTILS_H__

#include "ax_timer.h"
#include "GlobalDef.h"
#include "ax_base_type.h"
#include "string.h"
#include "ax_opal_type.h"

#define OSD_ALIGN_X_OFFSET (2)
#define OSD_ALIGN_Y_OFFSET (2)
#define OSD_ALIGN_WIDTH (2)
#define OSD_ALIGN_HEIGHT (2)

#define BASE_FONT_SIZE (16)


AX_U32 GetTimeFontSize(AX_U32 nWidth, AX_U32 nHeight);
AX_U32 GetRectLineWidth(AX_U32 nWidth, AX_U32 nHeight);
AX_U32 GetBoundaryX(AX_U32 nWidth, AX_U32 nHeight);


AX_S32 CalcOsdOffsetX(AX_S32 nImgWidth, AX_S32 nOsdWidth, AX_S32 nXMargin, AX_OPAL_OSD_ALIGN_E eAlign);
AX_S32 CalcOsdOffsetY(AX_S32 nImgHeight, AX_S32 nOsdHeight, AX_S32 nYMargin, AX_OPAL_OSD_ALIGN_E eAlign);
AX_S32 CalcOsdBoudingX(AX_S32 nImgWidth, AX_S32 nOsdWidth, AX_S32 nBoudingX, AX_OPAL_OSD_ALIGN_E eAlign);
AX_S32 CalcOsdBoudingY(AX_S32 nImgHeight, AX_S32 nOsdHeight, AX_S32 nBoudingY, AX_OPAL_OSD_ALIGN_E eAlign);

#endif  // __AX_OSD_UTILS_H__
