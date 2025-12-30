/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_ISP_COMMON_H__
#define __AX_ISP_COMMON_H__

#include "ax_base_type.h"
#include "ax_global_type.h"


#define AX_VIN_MAX_DEV_NUM              (4)
#define AX_VIN_MAX_PIPE_NUM             (6)
#define AX_VIN_MAX_CHN_NUM              (1)

#define AX_HDR_CHN_NUM                  (4)
#define AX_ISP_BAYER_CHN_NUM            (4)
#define AX_VIN_SYNC_CODE_NUM            (4)
#define AX_VIN_LVDS_LANE_NUM            (4)

#define AX_BT_SYNC_CODE_CHN_NUM                             (4)
#define AX_BT_DATA_MUX_NUM                                  (20)
#define AX_DVP_DATA_MUX_NUM                                 (14)

#define AX_MIPI_CSI_DT_BLACKING_DATA                        (0x11)
#define AX_MIPI_CSI_DT_EMBEDDED_8BIT_NON_IMAGE_DATA         (0x12)

#define AX_MIPI_CSI_DT_YUV420_8BIT                          (0x18)
#define AX_MIPI_CSI_DT_YUV420_10BIT                         (0x19)
#define AX_MIPI_CSI_DT_YUV420_8BIT_CHROMA_SHIFTED_PIXEL     (0x1C)
#define AX_MIPI_CSI_DT_YUV420_10BIT_CHROMA_SHIFTED_PIXEL    (0x1D)
#define AX_MIPI_CSI_DT_YUV422_8BIT                          (0x1E)
#define AX_MIPI_CSI_DT_YUV422_10BIT                         (0x1F)

#define AX_MIPI_CSI_DT_RGB444                               (0x20)
#define AX_MIPI_CSI_DT_RGB555                               (0x21)
#define AX_MIPI_CSI_DT_RGB565                               (0x22)
#define AX_MIPI_CSI_DT_RGB666                               (0x23)
#define AX_MIPI_CSI_DT_RGB888                               (0x24)

#define AX_MIPI_CSI_DT_RAW6                                 (0x28)
#define AX_MIPI_CSI_DT_RAW7                                 (0x29)
#define AX_MIPI_CSI_DT_RAW8                                 (0x2A)
#define AX_MIPI_CSI_DT_RAW10                                (0x2B)
#define AX_MIPI_CSI_DT_RAW12                                (0x2C)
#define AX_MIPI_CSI_DT_RAW14                                (0x2D)

#define AX_MIPI_CSI_DT_USER_DEF_8BIT_TYPE1                  (0x30)
#define AX_MIPI_CSI_DT_USER_DEF_8BIT_TYPE2                  (0x31)
#define AX_MIPI_CSI_DT_USER_DEF_8BIT_TYPE3                  (0x32)
#define AX_MIPI_CSI_DT_USER_DEF_8BIT_TYPE4                  (0x33)
#define AX_MIPI_CSI_DT_USER_DEF_8BIT_TYPE5                  (0x34)
#define AX_MIPI_CSI_DT_USER_DEF_8BIT_TYPE6                  (0x35)
#define AX_MIPI_CSI_DT_USER_DEF_8BIT_TYPE7                  (0x36)
#define AX_MIPI_CSI_DT_USER_DEF_8BIT_TYPE8                  (0x37)

typedef enum {
    AX_SNS_MODE_NONE   = 0,
    AX_SNS_LINEAR_MODE = 1,
    AX_SNS_HDR_2X_MODE = 2,
    AX_SNS_HDR_3X_MODE = 3,
    AX_SNS_HDR_4X_MODE = 4,
    AX_SNS_HDR_MODE_MAX
} AX_SNS_HDR_MODE_E;

typedef enum {
    AX_SNS_HDR_OUTPUT_MODE_FRAME_BASED = 0,
    AX_SNS_HDR_OUTPUT_MODE_DOL = 1,
    AX_SNS_HDR_OUTPUT_MODE_MAX
} AX_SNS_HDR_OUTPUT_MODE_E;

typedef enum {
    AX_SNS_HDR_FRAME_L      = 0,
    AX_SNS_HDR_FRAME_M      = 1,
    AX_SNS_HDR_FRAME_S      = 2,
    AX_SNS_HDR_FRAME_VS     = 3,
    AX_SNS_HDR_FRAME_MAX
} AX_SNS_HDR_FRAME_E;

typedef enum {
    AX_RT_RAW8                          = 8,        // raw8, 8-bit per pixel
    AX_RT_RAW10                         = 10,       // raw10, 10-bit per pixel
    AX_RT_RAW12                         = 12,       // raw12, 12-bit per pixel
    AX_RT_RAW14                         = 14,       // raw14, 14-bit per pixel
    AX_RT_RAW16                         = 16,       // raw16, 16-bit per pixel
    AX_RT_YUV422                        = 20,       // yuv422
    AX_RT_YUV420                        = 21,       // yuv420
} AX_RAW_TYPE_E;

typedef enum {
    AX_BP_RGGB                          = 0,        // R Gr Gb B bayer pattern
    AX_BP_GRBG                          = 1,        // Gr R B Gb bayer pattern
    AX_BP_GBRG                          = 2,        // Gb B R Gr byaer pattern
    AX_BP_BGGR                          = 3,        // B Gb Gr R byaer pattern
    AX_BP_MONO                          = 4,        // MONO, Gray, IR, etc
    AX_BP_RGBIR4x4                      = 16,       //< RGBIR 4x4
    AX_BP_MAX
} AX_BAYER_PATTERN_E;

typedef enum {
    AX_SNS_NORMAL                       = 0,
    AX_SNS_DOL_HDR                      = 1,
    AX_SNS_BME_HDR                      = 2,
    AX_SNS_QUAD_BAYER_NO_HDR            = 3,
    AX_SNS_QUAD_BAYER_2_HDR_MODE0       = 4,
    AX_SNS_QUAD_BAYER_2_HDR_MODE1       = 5,
    AX_SNS_QUAD_BAYER_2_HDR_MODE2       = 6,
    AX_SNS_QUAD_BAYER_3_HDR_MODE3       = 7,
    AX_SNS_DCG_HDR                      = 8,
    AX_SNS_OUTPUT_MODE_MAX
} AX_SNS_OUTPUT_MODE_E;

typedef struct _AX_WIN_AREA_T_ {
    AX_U32                              nStartX;
    AX_U32                              nStartY;
    AX_U32                              nWidth;
    AX_U32                              nHeight;
} AX_WIN_AREA_T;

#endif // __AX_ISP_CONNON_H__

