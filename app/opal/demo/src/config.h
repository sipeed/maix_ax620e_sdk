/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_DEMO_CONFIG_H_
#define _AX_OPAL_DEMO_CONFIG_H_

#include "ax_opal_type.h"

#define AUDIO_CHN_ID (0)
#define VIDEO_SNS_ID_INVALID (-1)
#define VIDEO_SNS_ID_0 (0)
#define VIDEO_SNS_ID_1 (1)
#define VIDEO_CHN_VENC_MAIN_ID (0)
#define VIDEO_CHN_VENC_SUB1_ID (2)
#define AX_OPAL_DEMO_HVCFP_RECT_NUM (32)
#define AX_OPAL_DEMO_VIDEO_CHN_NUM  (2)
#define MAX_OSD_STR_CHAR_LEN        (128)

#define RED 0xFF0000
#define PINK 0xFFC0CB
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define PURPLE 0xA020F0
#define ORANGE 0xFFA500
#define YELLOW 0xFFFF00
#define WHITE 0xFFFFFF

typedef enum {
    OSD_IND_TIME = 0,        /* 时间 */
    OSD_IND_PICTURE,         /* 图片 */
    OSD_IND_STRING_CHANNEL,  /* 字符串 */
    OSD_IND_STRING_LOCATION, /* 字符串 */
    OSD_IND_PRIVACY,         /* 矩形遮挡 */
    OSD_TYPE_MAX
} OSD_TYPE_E;

#define AX_OPAL_DEMO_OSD_CNT     OSD_TYPE_MAX

typedef struct _DEMO_OSD_ITEM_T {
    AX_OPAL_OSD_ATTR_T stOsdAttr;
    AX_U8              nSrcChn;
    AX_OPAL_HANDLE     pHandle;
    AX_CHAR            szStr[MAX_OSD_STR_CHAR_LEN + 1];
} DEMO_OSD_ITEM_T;

typedef struct _DEMO_SNS_OSD_CONFIG_T {
    DEMO_OSD_ITEM_T stOsd[AX_OPAL_DEMO_VIDEO_CHN_NUM][AX_OPAL_DEMO_OSD_CNT];
    DEMO_OSD_ITEM_T stPriv;
} DEMO_SNS_OSD_CONFIG_T;

typedef enum {
    DEMO_OS04A10            = 0,
    DEMO_SC200AI,
    DEMO_SC450AI,
    DEMO_SC500AI,
    DEMO_SC850SL,
    DEMO_OS04A10_SC200AI    = 101,
    DEMO_SC450AI_SC200AI,
    DEMO_SNS_BUTT,
} DEMO_SNS_TYPE_E;

typedef enum {
    DEMO_DUAL_SNS       = 0,
    DEMO_SINGLE_SNS     = 1,
    DEMO_SNSCOMB_BUTT,
} DEMO_SNS_COMB_TYPE_E;

#endif // _AX_OPAL_DEMO_CONFIG_H_
