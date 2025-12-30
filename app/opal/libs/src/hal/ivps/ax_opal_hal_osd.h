
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_HAL_OSD_H_
#define _AX_OPAL_HAL_OSD_H_

#include "ax_base_type.h"
#include "ax_global_type.h"
#include "ax_ivps_api.h"
#include "ax_opal_type.h"
#include "ax_opal_utils.h"
#include "OSDHandler.h"

#define OSD_ALIGN_X_OFFSET (4)
#define OSD_ALIGN_Y_OFFSET (4)
#define OSD_BMP_ALIGN_X_OFFSET (16)
#define OSD_BMP_ALIGN_Y_OFFSET (2)

#define OSD_ALIGN_WIDTH (8)
#define OSD_ALIGN_HEIGHT (2)
#define IS_VALID_CHANNEL(chn) ((chn) >= 0 && (chn) < MAX_WEB_CHANNEL_NUM)
#define IS_VALID_OSD_NUM(num) ((num) >= 0 && (num) <= AX_MAX_VO_NUM)

#define BASE_FONT_SIZE (16)
#define MAX_OSD_TIME_CHAR_LEN (32)
#define MAX_OSD_STR_CHAR_LEN (128)
#define MAX_OSD_WSTR_CHAR_LEN (256)

#define MAX_REGION_COUNT (32)
#define APP_OSD_CHANNEL_FILTER_0 (0)
#define APP_OSD_CHANNEL_FILTER_1 (1)
#define APP_OSD_GROUP_FILTER_0 (0)
#define APP_OSD_GROUP_FILTER_1 (1)

#define OSD_ALIGN_X_OFFSET (4)
#define OSD_ALIGN_Y_OFFSET (4)
#define OSD_BMP_ALIGN_X_OFFSET (16)
#define OSD_BMP_ALIGN_Y_OFFSET (2)

typedef struct _AX_OPAL_HAL_OSD_FONT_STYLE {
    AX_U32 nMagic;
    AX_U32 nTimeFontSize;
    AX_U32 nRectLineWidth;
    AX_U32 nBoundaryX;
    AX_U32 nBoundaryY;
} AX_OPAL_HAL_OSD_FONT_STYLE;

/* must be ordered as nMagic */
const static AX_OPAL_HAL_OSD_FONT_STYLE g_arrOsdStyle[] = {
    {3840 * 2160, 128, 4, 56, 8},
    {3072 * 2048, 128, 4, 56, 8},
    {3072 * 1728, 128, 4, 48, 8},
    {2624 * 1944, 96, 4, 48, 8},
    {2688 * 1520, 96, 4, 48, 8},
    {2048 * 1536, 96, 4, 48, 8},
    {2304 * 1296, 96, 4, 48, 8},
    {1920 * 1080, 48, 2, 24, 8},
    {1280 * 720, 48, 2, 20, 8},
    {1024 * 768, 32, 2, 16, 8},
    {720 * 576, 32, 2, 12, 8},
    {704 * 576, 32, 2, 12, 8},
    {640 * 480, 16, 2, 8, 8},
    {384 * 288, 16, 2, 2, 8}
};

IVPS_RGN_HANDLE AX_OPAL_HAL_OSD_CreateRgn(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_OSD_TYPE_E eType);
AX_S32 AX_OPAL_HAL_OSD_DestoryRgn(AX_S32 nGrpId, AX_S32 nChnId, IVPS_RGN_HANDLE hRgn, AX_OPAL_OSD_TYPE_E eType);

AX_S32 AX_OPAL_HAL_OSD_UpdatePic(IVPS_RGN_HANDLE hRgn, const AX_OPAL_OSD_ATTR_T *pOsdCfg, AX_U32 nSrcWidth, AX_U32 nSrcHeight);
AX_S32 AX_OPAL_HAL_OSD_UpdateStr(IVPS_RGN_HANDLE hRgn, const AX_OPAL_OSD_ATTR_T *pOsdCfg, AX_U32 nSrcWidth, AX_U32 nSrcHeight);
AX_S32 AX_OPAL_HAL_OSD_UpdatePrivacy(IVPS_RGN_HANDLE hRgn, const AX_OPAL_OSD_ATTR_T *pOsdCfg, AX_U32 nSrcWidth, AX_U32 nSrcHeight);
AX_S32 AX_OPAL_HAL_OSD_UpdateTime(IVPS_RGN_HANDLE hRgn, const AX_OPAL_OSD_ATTR_T *pOsdCfg, AX_U32 nSrcWidth, AX_U32 nSrcHeight);

AX_S32 AX_OPAL_HAL_OSD_UpdatePolygon(IVPS_RGN_HANDLE hRgn, AX_U32 nRectCount, const AX_IVPS_RGN_POLYGON_T* pstRects,
                                            AX_U32 nPolygonCount, const AX_IVPS_RGN_POLYGON_T* pstPolygons, AX_BOOL bVoRect);
#endif //  _AX_OPAL_HAL_OSD_H_