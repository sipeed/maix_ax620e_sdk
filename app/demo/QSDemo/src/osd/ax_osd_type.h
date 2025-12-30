/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OSD_TYPE_H_
#define _AX_OSD_TYPE_H_

#include <pthread.h>
#include "ax_base_type.h"
#include "ax_ivps_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef AX_S32 AX_OSD_IVPS_FILTER;

#define MAX_REGION_COUNT (16) // VENC0/VENC1: rect+time+osd; JENC(DETECT): time+osd
#define AX_OSD_CHANNEL_FILTER_0 (0)
#define AX_OSD_CHANNEL_FILTER_1 (1)
#define AX_OSD_GROUP_FILTER_0 (0)
#define AX_OSD_GROUP_FILTER_1 (1)

#define AX_OSD_ALIGN_X_OFFSET (2)
#define AX_OSD_ALIGN_Y_OFFSET (2)
#define AX_OSD_ALIGN_WIDTH (2)
#define AX_OSD_ALIGN_HEIGHT (2)

#define BASE_FONT_SIZE (16)

#define MAX_OSD_TIME_CHAR_LEN (64)
#define MAX_OSD_STR_CHAR_LEN (128)
#define MAX_OSD_WSTR_CHAR_LEN (256)

/* OSD index*/
typedef enum {
    AX_OSD_TYPE_RECT,
    AX_OSD_TYPE_TIME,
    AX_OSD_TYPE_LOGO,
    AX_OSD_TYPE_MAX
} AX_OSD_TYPE_E;

/* OSD Align Type */
typedef enum {
    AX_OSD_ALIGN_TYPE_LEFT_TOP,
    AX_OSD_ALIGN_TYPE_RIGHT_TOP,
    AX_OSD_ALIGN_TYPE_LEFT_BOTTOM,
    AX_OSD_ALIGN_TYPE_RIGHT_BOTTOM,
    AX_OSD_ALIGN_TYPE_MAX
} AX_OSD_ALIGN_TYPE_E;

/* 时间类型时间显示格式 */
typedef enum {
    AX_OSD_DATE_FORMAT_YYMMDD1,        /* XXXX-XX-XX (年月日) */
    AX_OSD_DATE_FORMAT_MMDDYY1,        /* XX-XX-XXXX (月日年) */
    AX_OSD_DATE_FORMAT_DDMMYY1,        /* XX-XX-XXXX (日月年) */
    AX_OSD_DATE_FORMAT_YYMMDD2,        /* XXXX年XX月XX日 */
    AX_OSD_DATE_FORMAT_MMDDYY2,        /* XX月XX日XXXX年 */
    AX_OSD_DATE_FORMAT_DDMMYY2,        /* XX日XX月XXXX年 */
    AX_OSD_DATE_FORMAT_YYMMDD3,        /* XXXX/XX/XX (年月日) */
    AX_OSD_DATE_FORMAT_MMDDYY3,        /* XXXX/XX/XX (年月日) */
    AX_OSD_DATE_FORMAT_DDMMYY3,        /* XXXX/XX/XX (年月日) */
    AX_OSD_DATE_FORMAT_YYMMDDWW1,      /* XXXX-XX-XX XXX (年月日 星期几) */
    AX_OSD_DATE_FORMAT_HHmmSS,         /* XX:XX:XX (时分秒) */
    AX_OSD_DATE_FORMAT_YYMMDDHHmmSS,   /* XXXX-XX-XX XX:XX:XX (年月日 时分秒) */
    AX_OSD_DATE_FORMAT_YYMMDDHHmmSSWW, /* XXXX-XX-XX XX:XX:XX XXX (年月日 时分秒 星期几) */
    AX_OSD_OSD_DATE_FORMAT_MAX
} AX_OSD_DATE_FORMAT_E;

typedef struct _AX_OSD_PIC_ATTR_T {
    AX_S32 nWidth;
    AX_S32 nHeight;
    AX_CHAR szFileName[128];
} AX_OSD_PIC_ATTR_T;

typedef struct _AX_OSD_TIME_ATTR_T {
    AX_BOOL bExit;
    pthread_t nTid;
    pthread_mutex_t mtxUpdate;
    pthread_cond_t cvUpdate;
    AX_BOOL bForceUpdate;
    AX_OSD_DATE_FORMAT_E eFormat;
    AX_U32 nFontSize;
    AX_U32 nColor;
} AX_OSD_TIME_ATTR_T;

typedef struct {
    AX_U32 nColor;
    AX_U32 nLineWidth;
} AX_OSD_RECT_ATTR_T;

typedef struct _AX_OSD_REGION_PARAM {
    AX_BOOL bEnable;
    IVPS_GRP nIvpsGrp;
    AX_S32 nIvpsChn;
    AX_S32 nVencChn;
    AX_OSD_TYPE_E eOsdType;
    IVPS_RGN_HANDLE hRgn;
    AX_OSD_IVPS_FILTER nFilter;
    AX_S16 nZindex;
    AX_OSD_ALIGN_TYPE_E eAlign;
    AX_S32 nBoundaryX;
    AX_S32 nBoundaryY;
    union {
        AX_OSD_PIC_ATTR_T tPicAttr;
        AX_OSD_TIME_ATTR_T tTimeAttr;
        AX_OSD_RECT_ATTR_T tRectAttr;
    };
} AX_OSD_REGION_PARAM_T;

#ifdef __cplusplus
}
#endif

#endif
