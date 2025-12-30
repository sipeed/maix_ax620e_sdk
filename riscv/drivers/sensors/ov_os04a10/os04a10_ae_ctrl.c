/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "ax_base_type.h"
#include "ax_isp_common.h"

#include "isp_sensor_types.h"
#include "isp_sensor_internal.h"

#include "os04a10_reg.h"
#include "os04a10_ae_ctrl.h"

#include <rtdevice.h>

#define OS04A10_MAX_VTS         (0xFFFF)
#define OS04A10_MAX_RATIO       (32.0f)
#define OS04A10_MIN_RATIO       (1.0f)

#define OS04A10_MIN_AGAIN       (1.0f)
#define OS04A10_MAX_AGAIN       (16.0f)

#define OS04A10_MIN_DGAIN       (1.0f)
#define OS04A10_MAX_DGAIN       (15.99f)

#define OS04A10_SDR_EXP_LINE_MIN                                      (2.0f)
#define OS04A10_SDR_EXP_LINE_MAX(vts)                                 (vts - 8)
#define OS04A10_SDR_INTEGRATION_TIME_STEP                             (1.0f)

#define OS04A10_HDR_2X_LONG_EXP_LINE_MIN                              (2.0f)
#define OS04A10_HDR_2X_LONG_EXP_LINE_MAX(vts, ratio)                  (((vts - 10) * ratio)/ (ratio + 1))
#define OS04A10_HDR_2X_LONG_INTEGRATION_TIME_STEP                     (1.0f)

#define OS04A10_HDR_2X_SHORT_EXP_LINE_MIN                             (2.0f)
#define OS04A10_HDR_2X_SHORT_EXP_LINE_MAX(vts, ratio)                 (((vts - 10) * 1)/ (ratio + 1))
#define OS04A10_HDR_2X_SHORT_INTEGRATION_TIME_STEP                    (1.0f)

#define OS04A10_EXP_OFFSET_SDR           (0.3f) //unit:line
#define OS04A10_EXP_OFFSET_HDR_2STAGGER  (0.6f)
#define OS04A10_EXP_OFFSET_HDR_3STAGGER  (0.75f)

typedef struct SENSOR_OS04A10_S {
    AX_U32 hts;
    AX_U32 vs_hts;
    AX_U32 vts;
    AX_F32 sclk;
    AX_F32 line_period;
} SNSOS04A10_OBJ;

typedef struct _OS04A10_AGAIN_TABLE_T_ {
    float gain;
    AX_U8 again_in;
    AX_U8 again_de;
} OS04A10_AGAIN_TABLE_T;

typedef struct _OS04A10_DGAIN_TABLE_T_ {
    float gain;
    AX_U8 dgain_in;
    AX_U8 dgain_de;
    AX_U8 dgain_de2;
} OS04A10_DGAIN_TABLE_T;

extern SNS_STATE_OBJ *g_szOs04a10Ctx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs04a10Ctx[dev])

static SNSOS04A10_OBJ sns_os04a10params[AX_VIN_MAX_PIPE_NUM];
static AX_F32 nAgainTable[SENSOR_MAX_GAIN_STEP];
static AX_F32 nDgainTable[SENSOR_MAX_GAIN_STEP];

/*user config*/
static AX_F32 gFpsGear[] = {1.00, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00,
                            11.00, 12.00, 13.00, 14.00, 15.00, 16.00, 17.00, 18.00, 19.00, 20.00,
                            21.00, 22.00, 23.00, 24.00, 25.00, 26.00, 27.00, 28.00, 29.00, 30.00, 60.00
                           };

typedef enum _AX_SNS_AE_REG_IDX_E_ {
    OS04A10_LONG_EXP_LINE_H_IDX = 0,
    OS04A10_LONG_EXP_LINE_L_IDX,
    OS04A10_LONG_AGAIN_H_IDX,
    OS04A10_LONG_AGAIN_L_IDX,
    OS04A10_LONG_DGAIN_H_IDX,
    OS04A10_LONG_DGAIN_M_IDX,
    OS04A10_LONG_DGAIN_L_IDX,

    OS04A10_SHORT_EXP_LINE_H_IDX,
    OS04A10_SHORT_EXP_LINE_L_IDX,
    OS04A10_SHORT_AGAIN_H_IDX,
    OS04A10_SHORT_AGAIN_L_IDX,
    OS04A10_SHORT_DGAIN_H_IDX,
    OS04A10_SHORT_DGAIN_M_IDX,
    OS04A10_SHORT_DGAIN_L_IDX,

    OS04A10_VTS_H_IDX,
    OS04A10_VTS_L_IDX,

    OS04A10_GROUP1_IDX,
    OS04A10_GROUP2_IDX,
    OS04A10_HCG_LCG_IDX,
    OS04A10_GROUP3_IDX,
    OS04A10_GROUP4_IDX,
    OS04A10_GROUP5_IDX,
    OS04A10_GROUP6_IDX,
    OS04A10_REG_MAX_IDX,
} AX_SNS_AE_REG_IDX_E;

static AX_SNS_DRV_DELAY_TABLE_T gOs04a10AeRegsTable[] = {
    /* regs index */                  /* regs addr */                          /*regs value*/        /*Delay Frame Num*/
    {OS04A10_LONG_EXP_LINE_H_IDX,   OS04A10_LONG_EXP_LINE_H,       0,          0},
    {OS04A10_LONG_EXP_LINE_L_IDX,   OS04A10_LONG_EXP_LINE_L,       0,          0},
    {OS04A10_LONG_AGAIN_H_IDX,      OS04A10_LONG_AGAIN_H,          0,          0},
    {OS04A10_LONG_AGAIN_L_IDX,      OS04A10_LONG_AGAIN_L,          0,          0},
    {OS04A10_LONG_DGAIN_H_IDX,      OS04A10_LONG_DGAIN_H,          0,          0},
    {OS04A10_LONG_DGAIN_M_IDX,      OS04A10_LONG_DGAIN_M,          0,          0},
    {OS04A10_LONG_DGAIN_L_IDX,      OS04A10_LONG_DGAIN_L,          0,          0},

    {OS04A10_SHORT_EXP_LINE_H_IDX, OS04A10_SHORT_EXP_LINE_H,      0,          0},
    {OS04A10_SHORT_EXP_LINE_L_IDX, OS04A10_SHORT_EXP_LINE_L,      0,          0},
    {OS04A10_SHORT_AGAIN_H_IDX,    OS04A10_SHORT_AGAIN_H,         0,          0},
    {OS04A10_SHORT_AGAIN_L_IDX,    OS04A10_SHORT_AGAIN_L,         0,          0},
    {OS04A10_SHORT_DGAIN_H_IDX,    OS04A10_SHORT_DGAIN_H,         0,          0},
    {OS04A10_SHORT_DGAIN_M_IDX,    OS04A10_SHORT_DGAIN_M,         0,          0},
    {OS04A10_SHORT_DGAIN_L_IDX,    OS04A10_SHORT_DGAIN_L,         0,          0},

    {OS04A10_VTS_H_IDX,         OS04A10_VTS_H,                 0,          0},
    {OS04A10_VTS_L_IDX,         OS04A10_VTS_L,                 0,          0},

    /* group hold */
    {OS04A10_GROUP1_IDX,        OS04A10_GROUP_SWCTRL1,         0,          1},
    {OS04A10_GROUP2_IDX,        OS04A10_GROUP_ACCESS1,         0,          1},
    {OS04A10_HCG_LCG_IDX,       OS04A10_HCG_LCG,               0,          1},
    {OS04A10_GROUP3_IDX,        OS04A10_GROUP_ACCESS2,         0,          1},
    {OS04A10_GROUP4_IDX,        OS04A10_GROUP_SWCTRL2,         0,          1},
    {OS04A10_GROUP5_IDX,        OS04A10_GROUP10_STAY_NUM,      0,          1},
    {OS04A10_GROUP6_IDX,        OS04A10_SW_GROUP_ACCESS,       0,          1},
};

const OS04A10_AGAIN_TABLE_T os04a10_again_table[] = {
    {1,       0x01, 0x00},
    {1.0625,  0x01, 0x10},
    {1.125,   0x01, 0x20},
    {1.1875,  0x01, 0x30},
    {1.25,    0x01, 0x40},
    {1.3125,  0x01, 0x50},
    {1.375,   0x01, 0x60},
    {1.4375,  0x01, 0x70},
    {1.5,     0x01, 0x80},
    {1.5625,  0x01, 0x90},
    {1.625,   0x01, 0xA0},
    {1.6875,  0x01, 0xB0},
    {1.75,    0x01, 0xC0},
    {1.8125,  0x01, 0xD0},
    {1.875,   0x01, 0xE0},
    {1.9375,  0x01, 0xF0},
    {2,       0x02, 0x00},
    {2.125,   0x02, 0x20},
    {2.25,    0x02, 0x40},
    {2.375,   0x02, 0x60},
    {2.5,     0x02, 0x80},
    {2.625,   0x02, 0xA0},
    {2.75,    0x02, 0xC0},
    {2.875,   0x02, 0xE0},
    {3,       0x03, 0x00},
    {3.125,   0x03, 0x20},
    {3.25,    0x03, 0x40},
    {3.375,   0x03, 0x60},
    {3.5,     0x03, 0x80},
    {3.625,   0x03, 0xA0},
    {3.75,    0x03, 0xC0},
    {3.875,   0x03, 0xE0},
    {4,       0x04, 0x00},
    {4.25,    0x04, 0x40},
    {4.5,     0x04, 0x80},
    {4.75,    0x04, 0xC0},
    {5,       0x05, 0x00},
    {5.25,    0x05, 0x40},
    {5.5,     0x05, 0x80},
    {5.75,    0x05, 0xC0},
    {6,       0x06, 0x00},
    {6.25,    0x06, 0x40},
    {6.5,     0x06, 0x80},
    {6.75,    0x06, 0xC0},
    {7,       0x07, 0x00},
    {7.25,    0x07, 0x40},
    {7.5,     0x07, 0x80},
    {7.75,    0x07, 0xC0},
    {8,       0x08, 0x00},
    {8.5,     0x08, 0x80},
    {9,       0x09, 0x00},
    {9.5,     0x09, 0x80},
    {10,      0x0A, 0x00},
    {10.5,    0x0A, 0x80},
    {11,      0x0B, 0x00},
    {11.5,    0x0B, 0x80},
    {12,      0x0C, 0x00},
    {12.5,    0x0C, 0x80},
    {13,      0x0D, 0x00},
    {13.5,    0x0D, 0x80},
    {14,      0x0E, 0x00},
    {14.5,    0x0E, 0x80},
    {15,      0x0F, 0x00},
    {15.5,    0x0F, 0x80},
    {16.0,    0x0F, 0xF0},
};

const OS04A10_DGAIN_TABLE_T os04a10_dgain_table[] = {
   /* gain       dg_h  dg_l  dg_ll*/
    {1,          0x01, 0x00, 0x00},
    {1.00390625, 0x01, 0x01, 0x00},
    {1.00781250, 0x01, 0x02, 0x00},
    {1.01171875, 0x01, 0x03, 0x00},
    {1.01562500, 0x01, 0x04, 0x00},
    {1.01953125, 0x01, 0x05, 0x00},
    {1.02343750, 0x01, 0x06, 0x00},
    {1.02734375, 0x01, 0x07, 0x00},
    {1.03125000, 0x01, 0x08, 0x00},
    {1.03515625, 0x01, 0x09, 0x00},
    {1.03906250, 0x01, 0x0a, 0x00},
    {1.04296875, 0x01, 0x0b, 0x00},
    {1.04687500, 0x01, 0x0c, 0x00},
    {1.05078125, 0x01, 0x0d, 0x00},
    {1.05468750, 0x01, 0x0e, 0x00},
    {1.05859375, 0x01, 0x0f, 0x00},
    {1.06250000, 0x01, 0x10, 0x00},
    {1.06640625, 0x01, 0x11, 0x00},
    {1.07031250, 0x01, 0x12, 0x00},
    {1.07421875, 0x01, 0x13, 0x00},
    {1.07812500, 0x01, 0x14, 0x00},
    {1.08203125, 0x01, 0x15, 0x00},
    {1.08593750, 0x01, 0x16, 0x00},
    {1.08984375, 0x01, 0x17, 0x00},
    {1.09375000, 0x01, 0x18, 0x00},
    {1.09765625, 0x01, 0x19, 0x00},
    {1.10156250, 0x01, 0x1a, 0x00},
    {1.10546875, 0x01, 0x1b, 0x00},
    {1.10937500, 0x01, 0x1c, 0x00},
    {1.11328125, 0x01, 0x1d, 0x00},
    {1.11718750, 0x01, 0x1e, 0x00},
    {1.12109375, 0x01, 0x1f, 0x00},
    {1.12500000, 0x01, 0x20, 0x00},
    {1.12890625, 0x01, 0x21, 0x00},
    {1.13281250, 0x01, 0x22, 0x00},
    {1.13671875, 0x01, 0x23, 0x00},
    {1.14062500, 0x01, 0x24, 0x00},
    {1.14453125, 0x01, 0x25, 0x00},
    {1.14843750, 0x01, 0x26, 0x00},
    {1.15234375, 0x01, 0x27, 0x00},
    {1.15625000, 0x01, 0x28, 0x00},
    {1.16015625, 0x01, 0x29, 0x00},
    {1.16406250, 0x01, 0x2a, 0x00},
    {1.16796875, 0x01, 0x2b, 0x00},
    {1.17187500, 0x01, 0x2c, 0x00},
    {1.17578125, 0x01, 0x2d, 0x00},
    {1.17968750, 0x01, 0x2e, 0x00},
    {1.18359375, 0x01, 0x2f, 0x00},
    {1.18750000, 0x01, 0x30, 0x00},
    {1.19140625, 0x01, 0x31, 0x00},
    {1.19531250, 0x01, 0x32, 0x00},
    {1.19921875, 0x01, 0x33, 0x00},
    {1.20312500, 0x01, 0x34, 0x00},
    {1.20703125, 0x01, 0x35, 0x00},
    {1.21093750, 0x01, 0x36, 0x00},
    {1.21484375, 0x01, 0x37, 0x00},
    {1.21875000, 0x01, 0x38, 0x00},
    {1.22265625, 0x01, 0x39, 0x00},
    {1.22656250, 0x01, 0x3a, 0x00},
    {1.23046875, 0x01, 0x3b, 0x00},
    {1.23437500, 0x01, 0x3c, 0x00},
    {1.23828125, 0x01, 0x3d, 0x00},
    {1.24218750, 0x01, 0x3e, 0x00},
    {1.24609375, 0x01, 0x3f, 0x00},
    {1.25000000, 0x01, 0x40, 0x00},
    {1.25390625, 0x01, 0x41, 0x00},
    {1.25781250, 0x01, 0x42, 0x00},
    {1.26171875, 0x01, 0x43, 0x00},
    {1.26562500, 0x01, 0x44, 0x00},
    {1.26953125, 0x01, 0x45, 0x00},
    {1.27343750, 0x01, 0x46, 0x00},
    {1.27734375, 0x01, 0x47, 0x00},
    {1.28125000, 0x01, 0x48, 0x00},
    {1.28515625, 0x01, 0x49, 0x00},
    {1.28906250, 0x01, 0x4a, 0x00},
    {1.29296875, 0x01, 0x4b, 0x00},
    {1.29687500, 0x01, 0x4c, 0x00},
    {1.30078125, 0x01, 0x4d, 0x00},
    {1.30468750, 0x01, 0x4e, 0x00},
    {1.30859375, 0x01, 0x4f, 0x00},
    {1.31250000, 0x01, 0x50, 0x00},
    {1.31640625, 0x01, 0x51, 0x00},
    {1.32031250, 0x01, 0x52, 0x00},
    {1.32421875, 0x01, 0x53, 0x00},
    {1.32812500, 0x01, 0x54, 0x00},
    {1.33203125, 0x01, 0x55, 0x00},
    {1.33593750, 0x01, 0x56, 0x00},
    {1.33984375, 0x01, 0x57, 0x00},
    {1.34375000, 0x01, 0x58, 0x00},
    {1.34765625, 0x01, 0x59, 0x00},
    {1.35156250, 0x01, 0x5a, 0x00},
    {1.35546875, 0x01, 0x5b, 0x00},
    {1.35937500, 0x01, 0x5c, 0x00},
    {1.36328125, 0x01, 0x5d, 0x00},
    {1.36718750, 0x01, 0x5e, 0x00},
    {1.37109375, 0x01, 0x5f, 0x00},
    {1.37500000, 0x01, 0x60, 0x00},
    {1.37890625, 0x01, 0x61, 0x00},
    {1.38281250, 0x01, 0x62, 0x00},
    {1.38671875, 0x01, 0x63, 0x00},
    {1.39062500, 0x01, 0x64, 0x00},
    {1.39453125, 0x01, 0x65, 0x00},
    {1.39843750, 0x01, 0x66, 0x00},
    {1.40234375, 0x01, 0x67, 0x00},
    {1.40625000, 0x01, 0x68, 0x00},
    {1.41015625, 0x01, 0x69, 0x00},
    {1.41406250, 0x01, 0x6a, 0x00},
    {1.41796875, 0x01, 0x6b, 0x00},
    {1.42187500, 0x01, 0x6c, 0x00},
    {1.42578125, 0x01, 0x6d, 0x00},
    {1.42968750, 0x01, 0x6e, 0x00},
    {1.43359375, 0x01, 0x6f, 0x00},
    {1.43750000, 0x01, 0x70, 0x00},
    {1.44140625, 0x01, 0x71, 0x00},
    {1.44531250, 0x01, 0x72, 0x00},
    {1.44921875, 0x01, 0x73, 0x00},
    {1.45312500, 0x01, 0x74, 0x00},
    {1.45703125, 0x01, 0x75, 0x00},
    {1.46093750, 0x01, 0x76, 0x00},
    {1.46484375, 0x01, 0x77, 0x00},
    {1.46875000, 0x01, 0x78, 0x00},
    {1.47265625, 0x01, 0x79, 0x00},
    {1.47656250, 0x01, 0x7a, 0x00},
    {1.48046875, 0x01, 0x7b, 0x00},
    {1.48437500, 0x01, 0x7c, 0x00},
    {1.48828125, 0x01, 0x7d, 0x00},
    {1.49218750, 0x01, 0x7e, 0x00},
    {1.49609375, 0x01, 0x7f, 0x00},
    {1.50000000, 0x01, 0x80, 0x00},
    {1.50390625, 0x01, 0x81, 0x00},
    {1.50781250, 0x01, 0x82, 0x00},
    {1.51171875, 0x01, 0x83, 0x00},
    {1.51562500, 0x01, 0x84, 0x00},
    {1.51953125, 0x01, 0x85, 0x00},
    {1.52343750, 0x01, 0x86, 0x00},
    {1.52734375, 0x01, 0x87, 0x00},
    {1.53125000, 0x01, 0x88, 0x00},
    {1.53515625, 0x01, 0x89, 0x00},
    {1.53906250, 0x01, 0x8a, 0x00},
    {1.54296875, 0x01, 0x8b, 0x00},
    {1.54687500, 0x01, 0x8c, 0x00},
    {1.55078125, 0x01, 0x8d, 0x00},
    {1.55468750, 0x01, 0x8e, 0x00},
    {1.55859375, 0x01, 0x8f, 0x00},
    {1.56250000, 0x01, 0x90, 0x00},
    {1.56640625, 0x01, 0x91, 0x00},
    {1.57031250, 0x01, 0x92, 0x00},
    {1.57421875, 0x01, 0x93, 0x00},
    {1.57812500, 0x01, 0x94, 0x00},
    {1.58203125, 0x01, 0x95, 0x00},
    {1.58593750, 0x01, 0x96, 0x00},
    {1.58984375, 0x01, 0x97, 0x00},
    {1.59375000, 0x01, 0x98, 0x00},
    {1.59765625, 0x01, 0x99, 0x00},
    {1.60156250, 0x01, 0x9a, 0x00},
    {1.60546875, 0x01, 0x9b, 0x00},
    {1.60937500, 0x01, 0x9c, 0x00},
    {1.61328125, 0x01, 0x9d, 0x00},
    {1.61718750, 0x01, 0x9e, 0x00},
    {1.62109375, 0x01, 0x9f, 0x00},
    {1.62500000, 0x01, 0xa0, 0x00},
    {1.62890625, 0x01, 0xa1, 0x00},
    {1.63281250, 0x01, 0xa2, 0x00},
    {1.63671875, 0x01, 0xa3, 0x00},
    {1.64062500, 0x01, 0xa4, 0x00},
    {1.64453125, 0x01, 0xa5, 0x00},
    {1.64843750, 0x01, 0xa6, 0x00},
    {1.65234375, 0x01, 0xa7, 0x00},
    {1.65625000, 0x01, 0xa8, 0x00},
    {1.66015625, 0x01, 0xa9, 0x00},
    {1.66406250, 0x01, 0xaa, 0x00},
    {1.66796875, 0x01, 0xab, 0x00},
    {1.67187500, 0x01, 0xac, 0x00},
    {1.67578125, 0x01, 0xad, 0x00},
    {1.67968750, 0x01, 0xae, 0x00},
    {1.68359375, 0x01, 0xaf, 0x00},
    {1.68750000, 0x01, 0xb0, 0x00},
    {1.69140625, 0x01, 0xb1, 0x00},
    {1.69531250, 0x01, 0xb2, 0x00},
    {1.69921875, 0x01, 0xb3, 0x00},
    {1.70312500, 0x01, 0xb4, 0x00},
    {1.70703125, 0x01, 0xb5, 0x00},
    {1.71093750, 0x01, 0xb6, 0x00},
    {1.71484375, 0x01, 0xb7, 0x00},
    {1.71875000, 0x01, 0xb8, 0x00},
    {1.72265625, 0x01, 0xb9, 0x00},
    {1.72656250, 0x01, 0xba, 0x00},
    {1.73046875, 0x01, 0xbb, 0x00},
    {1.73437500, 0x01, 0xbc, 0x00},
    {1.73828125, 0x01, 0xbd, 0x00},
    {1.74218750, 0x01, 0xbe, 0x00},
    {1.74609375, 0x01, 0xbf, 0x00},
    {1.75000000, 0x01, 0xc0, 0x00},
    {1.75390625, 0x01, 0xc1, 0x00},
    {1.75781250, 0x01, 0xc2, 0x00},
    {1.76171875, 0x01, 0xc3, 0x00},
    {1.76562500, 0x01, 0xc4, 0x00},
    {1.76953125, 0x01, 0xc5, 0x00},
    {1.77343750, 0x01, 0xc6, 0x00},
    {1.77734375, 0x01, 0xc7, 0x00},
    {1.78125000, 0x01, 0xc8, 0x00},
    {1.78515625, 0x01, 0xc9, 0x00},
    {1.78906250, 0x01, 0xca, 0x00},
    {1.79296875, 0x01, 0xcb, 0x00},
    {1.79687500, 0x01, 0xcc, 0x00},
    {1.80078125, 0x01, 0xcd, 0x00},
    {1.80468750, 0x01, 0xce, 0x00},
    {1.80859375, 0x01, 0xcf, 0x00},
    {1.81250000, 0x01, 0xd0, 0x00},
    {1.81640625, 0x01, 0xd1, 0x00},
    {1.82031250, 0x01, 0xd2, 0x00},
    {1.82421875, 0x01, 0xd3, 0x00},
    {1.82812500, 0x01, 0xd4, 0x00},
    {1.83203125, 0x01, 0xd5, 0x00},
    {1.83593750, 0x01, 0xd6, 0x00},
    {1.83984375, 0x01, 0xd7, 0x00},
    {1.84375000, 0x01, 0xd8, 0x00},
    {1.84765625, 0x01, 0xd9, 0x00},
    {1.85156250, 0x01, 0xda, 0x00},
    {1.85546875, 0x01, 0xdb, 0x00},
    {1.85937500, 0x01, 0xdc, 0x00},
    {1.86328125, 0x01, 0xdd, 0x00},
    {1.86718750, 0x01, 0xde, 0x00},
    {1.87109375, 0x01, 0xdf, 0x00},
    {1.87500000, 0x01, 0xe0, 0x00},
    {1.87890625, 0x01, 0xe1, 0x00},
    {1.88281250, 0x01, 0xe2, 0x00},
    {1.88671875, 0x01, 0xe3, 0x00},
    {1.89062500, 0x01, 0xe4, 0x00},
    {1.89453125, 0x01, 0xe5, 0x00},
    {1.89843750, 0x01, 0xe6, 0x00},
    {1.90234375, 0x01, 0xe7, 0x00},
    {1.90625000, 0x01, 0xe8, 0x00},
    {1.91015625, 0x01, 0xe9, 0x00},
    {1.91406250, 0x01, 0xea, 0x00},
    {1.91796875, 0x01, 0xeb, 0x00},
    {1.92187500, 0x01, 0xec, 0x00},
    {1.92578125, 0x01, 0xed, 0x00},
    {1.92968750, 0x01, 0xee, 0x00},
    {1.93359375, 0x01, 0xef, 0x00},
    {1.93750000, 0x01, 0xf0, 0x00},
    {1.94140625, 0x01, 0xf1, 0x00},
    {1.94531250, 0x01, 0xf2, 0x00},
    {1.94921875, 0x01, 0xf3, 0x00},
    {1.95312500, 0x01, 0xf4, 0x00},
    {1.95703125, 0x01, 0xf5, 0x00},
    {1.96093750, 0x01, 0xf6, 0x00},
    {1.96484375, 0x01, 0xf7, 0x00},
    {1.96875000, 0x01, 0xf8, 0x00},
    {1.97265625, 0x01, 0xf9, 0x00},
    {1.97656250, 0x01, 0xfa, 0x00},
    {1.98046875, 0x01, 0xfb, 0x00},
    {1.98437500, 0x01, 0xfc, 0x00},
    {1.98828125, 0x01, 0xfd, 0x00},
    {1.99218750, 0x01, 0xfe, 0x00},
    {1.99609375, 0x01, 0xff, 0x00},
    {2,        0x02, 0x00,  0x00},
    {2.125,    0x02, 0x20,  0x00},
    {2.25,     0x02, 0x40,  0x00},
    {2.375,    0x02, 0x60,  0x00},
    {2.5,      0x02, 0x80,  0x00},
    {2.625,    0x02, 0xa0,  0x00},
    {2.75,     0x02, 0xc0,  0x00},
    {2.875,    0x02, 0xe0,  0x00},
    {3,        0x03, 0x00,  0x00},
    {3.125,    0x03, 0x20,  0x00},
    {3.25,     0x03, 0x40,  0x00},
    {3.375,    0x03, 0x60,  0x00},
    {3.5,      0x03, 0x80,  0x00},
    {3.625,    0x03, 0xa0,  0x00},
    {3.75,     0x03, 0xc0,  0x00},
    {3.875,    0x03, 0xe0,  0x00},
    {4,        0x04, 0x00,  0x00},
    {4.25,     0x04, 0x40,  0x00},
    {4.5,      0x04, 0x80,  0x00},
    {4.75,     0x04, 0xc0,  0x00},
    {5,        0x05, 0x00,  0x00},
    {5.25,     0x05, 0x40,  0x00},
    {5.5,      0x05, 0x80,  0x00},
    {5.75,     0x05, 0xc0,  0x00},
    {6,        0x06, 0x00,  0x00},
    {6.25,     0x06, 0x40,  0x00},
    {6.5,      0x06, 0x80,  0x00},
    {6.75,     0x06, 0xc0,  0x00},
    {7,        0x07, 0x00,  0x00},
    {7.25,     0x07, 0x40,  0x00},
    {7.5,      0x07, 0x80,  0x00},
    {7.75,     0x07, 0xc0,  0x00},
    {8,        0x08, 0x00,  0x00},
    {8.5,      0x08, 0x80,  0x00},
    {9,        0x09, 0x00,  0x00},
    {9.5,      0x09, 0x80,  0x00},
    {10,       0x0A, 0x00,  0x00},
    {10.5,     0x0A, 0x80,  0x00},
    {11,       0x0B, 0x00,  0x00},
    {11.5,     0x0B, 0x80,  0x00},
    {12,       0x0C, 0x00,  0x00},
    {12.5,     0x0C, 0x80,  0x00},
    {13,       0x0D, 0x00,  0x00},
    {13.5,     0x0D, 0x80,  0x00},
    {14,       0x0E, 0x00,  0x00},
    {14.5,     0x0E, 0x80,  0x00},
    {15,       0x0F, 0x00,  0x00},
    {15.5,     0x0F, 0x80,  0x00},
    {15.99,    0X0F, 0xfd,  0x40},
};


AX_F32 os04a10_again2value(float gain, AX_U8 *again_in, AX_U8 *again_de)
{
    AX_U32 i;
    AX_U32 count;
    //AX_U32 ret_value = 0;

    if (!again_in || !again_de)
        return -1;

    count = sizeof(os04a10_again_table) / sizeof(os04a10_again_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > os04a10_again_table[i].gain) {
            continue;
        } else {
            *again_in = os04a10_again_table[i].again_in;
            *again_de = os04a10_again_table[i].again_de;
            SNS_DBG("again=%f, again_in=0x%x, again_de=0x%x\n", gain, *again_in, *again_de);
            return os04a10_again_table[i].gain;
        }
    }

    return -1;
}

AX_F32 os04a10_dgain2value(float gain, AX_U8 *dgain_in, AX_U8 *dgain_de, AX_U8 *dgain_de2)
{
    AX_U32 i;
    AX_U32 count;
    //AX_U32 ret_value = 0;

    if (!dgain_in || !dgain_de || !dgain_de2)
        return -1;

    count = sizeof(os04a10_dgain_table) / sizeof(os04a10_dgain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > os04a10_dgain_table[i].gain) {
            continue;
        } else {
            *dgain_in = os04a10_dgain_table[i].dgain_in;
            *dgain_de = os04a10_dgain_table[i].dgain_de;
            *dgain_de2 = os04a10_dgain_table[i].dgain_de2;
            SNS_DBG("dgain=%f, dgain_in=0x%x, dgain_de=0x%x dgain_de2=0x%x\n", gain, *dgain_in, *dgain_de, *dgain_de2);

            return os04a10_dgain_table[i].gain;
        }
    }

    return -1;
}


AX_S32 os04a10_get_gain_table(AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_U32 i;
    AX_S32 ret = 0;
    if (!params)
        return -1;

    params->nAgainTableSize = sizeof(os04a10_again_table) / sizeof(os04a10_again_table[0]);
    params->nDgainTableSize = sizeof(os04a10_dgain_table) / sizeof(os04a10_dgain_table[0]);

    for (i = 0; i < params->nAgainTableSize ; i++) {
        nAgainTable[i] = os04a10_again_table[i].gain;
        params->pAgainTable = nAgainTable;
    }

    for (i = 0; i < params->nDgainTableSize ; i++) {
        nDgainTable[i] = os04a10_dgain_table[i].gain;
        params->pDgainTable = nDgainTable;
    }

    return ret;
}

static AX_SNS_HCGLCG_MODE_E os04a10_get_dcg_mode(ISP_PIPE_ID nPipeId)
{
    AX_S32 value = 0;
    AX_U8 data = 0;

    value = os04a10_reg_read(nPipeId, OS04A10_HCG_LCG);
    if (value == -1) {
        SNS_ERR("OS04A10_HCG_LCG reg value:0x%x\n", value);
        return AX_LCG_NOTSUPPORT_MODE;
    }

    data = value & 0xff;

    if ((data & 0x70) != 0) {
        return AX_LCG_MODE;
    } else {
        return AX_HCG_MODE;
    }

    return AX_LCG_NOTSUPPORT_MODE;
}

AX_S32 os04a10_cfg_aec_param(ISP_PIPE_ID nPipeId)
{
    //AX_F32 vs_line_period = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_os04a10params[nPipeId].hts = os04a10_get_hts(nPipeId);
    sns_os04a10params[nPipeId].vs_hts = os04a10_get_vs_hts(nPipeId);
    sns_os04a10params[nPipeId].vts = os04a10_get_vts(nPipeId);
    sns_os04a10params[nPipeId].sclk = os04a10_get_sclk(nPipeId);

    if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_os04a10params[nPipeId].line_period = (float)sns_os04a10params[nPipeId].hts / AXSNS_DIV_0_TO_1(sns_os04a10params[nPipeId].sclk)
                * SNS_1_SECOND_UNIT_US;
    } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_os04a10params[nPipeId].line_period = (float)(2 * sns_os04a10params[nPipeId].hts) / AXSNS_DIV_0_TO_1(sns_os04a10params[nPipeId].sclk)
                * SNS_1_SECOND_UNIT_US;
    } else {
        // wrong hdr mode
    }

    sns_obj->ae_ctrl_param.fTimePerLine = sns_os04a10params[nPipeId].line_period;

    /* sensor again  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX] = OS04A10_MIN_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX] = OS04A10_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 16;

    /* sensor dgain  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX] = OS04A10_MIN_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX] = OS04A10_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 1024;

    /* sensor medium again limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX] = OS04A10_MIN_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX] = OS04A10_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 16;

    /* sensor medium dgain limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX] = OS04A10_MIN_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX] = OS04A10_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 1024;

    sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio = OS04A10_MIN_RATIO;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio = OS04A10_MAX_RATIO;

    if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = OS04A10_SDR_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = OS04A10_SDR_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts);
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = OS04A10_SDR_INTEGRATION_TIME_STEP;
    } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = OS04A10_HDR_2X_LONG_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = OS04A10_HDR_2X_LONG_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts, OS04A10_MAX_RATIO);
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = OS04A10_HDR_2X_LONG_INTEGRATION_TIME_STEP;

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] = OS04A10_HDR_2X_SHORT_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = OS04A10_HDR_2X_SHORT_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts, OS04A10_MAX_RATIO);
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_MEDIUM_FRAME_IDX] = OS04A10_HDR_2X_SHORT_INTEGRATION_TIME_STEP;
    } else {
        // wrong hdr mode
    }

    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = sns_obj->sns_mode_obj.fFrameRate;

    sns_obj->ae_ctrl_param.fSnsHcgLcgRatio = 4.1953125f;
    sns_obj->ae_ctrl_param.eSnsHcgLcgMode = os04a10_get_dcg_mode(nPipeId);

    if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_LINEAR_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = OS04A10_EXP_OFFSET_SDR;
    } else if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_HDR_2X_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = OS04A10_EXP_OFFSET_HDR_2STAGGER;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_MEDIUM_FRAME_IDX] = OS04A10_EXP_OFFSET_HDR_2STAGGER;
    } else if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_HDR_3X_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = OS04A10_EXP_OFFSET_HDR_3STAGGER;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_MEDIUM_FRAME_IDX] = OS04A10_EXP_OFFSET_HDR_3STAGGER;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_SHORT_FRAME_IDX] = OS04A10_EXP_OFFSET_HDR_3STAGGER;
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = OS04A10_EXP_OFFSET_SDR;
    }

    SNS_DBG("inttime min = %.2f, max = %.2f, line_period=%f, vts = %d \n",
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_os04a10params[nPipeId].line_period, sns_os04a10params[nPipeId].vts);

    return SNS_SUCCESS;
}

AX_S32 os04a10_sns_update_regidx_table(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sztRegsInfo[0].sztI2cData[nRegIdx].nData = nRegValue;

    SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
        nRegIdx, sns_obj->sztRegsInfo[0].sztI2cData[nRegIdx].nRegAddr, nRegValue);

    return SNS_SUCCESS;
}

AX_S32 os04a10_get_sensor_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_S32 result = 0;
    SNS_CHECK_PTR_VALID(params);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    result = os04a10_get_gain_table(params);
    return result;

    return SNS_SUCCESS;
}


AX_S32 os04a10_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain)
{
    AX_U8 Gain_in;
    AX_U8 Gain_de;
    AX_S32 result = 0;
    AX_F32 gain_value = 0;
    AX_F32 nGainFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptAGain);

    /* long gain seting */
    nGainFromUser = ptAGain->fGain[HDR_LONG_FRAME_IDX];
    nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX],
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX]);

    gain_value = os04a10_again2value(nGainFromUser, &Gain_in, &Gain_de);
    if (gain_value == -1) {
        SNS_ERR("new gain match failed \n");
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX] = gain_value;
        result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_LONG_AGAIN_H_IDX, (Gain_in & 0x1F));
        result |= os04a10_sns_update_regidx_table(nPipeId, OS04A10_LONG_AGAIN_L_IDX, (Gain_de & 0xF0));
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    /* medium gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {

        nGainFromUser = ptAGain->fGain[HDR_MEDIUM_FRAME_IDX];
        nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX]);

        gain_value = os04a10_again2value(nGainFromUser, &Gain_in, &Gain_de);
        if (gain_value == -1) {
            SNS_ERR(" new gain match failed \n");
        } else {
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_MEDIUM_FRAME_IDX] = gain_value;
            result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_SHORT_AGAIN_H_IDX, (Gain_in & 0x1F));
            result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_SHORT_AGAIN_L_IDX, (Gain_de & 0xF0));
            if (result != 0) {
                SNS_ERR("%s: write hw failed %d \n", __func__, result);
                return result;
            }
        }
    }

    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->sztRegsInfo[0].tSnsExpInfo.szCurAGain[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX];
        sns_obj->sztRegsInfo[0].tSnsExpInfo.szCurAGain[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_MEDIUM_FRAME_IDX];

        SNS_DBG("again: long frame %f, short frame %f\n",
                sns_obj->sztRegsInfo[0].tSnsExpInfo.szCurAGain[HDR_LONG_FRAME_IDX],
                sns_obj->sztRegsInfo[0].tSnsExpInfo.szCurAGain[HDR_MEDIUM_FRAME_IDX]);
    }

    return SNS_SUCCESS;
}

AX_S32 os04a10_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain)
{
    AX_U8 Gain_in;
    AX_U8 Gain_de;
    AX_U8 Gain_de2;
    AX_S32 result = 0;
    AX_F32 gain_val = 0;
    AX_F32 nGainFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptDGain);

    /* long frame digital gain seting */
    nGainFromUser = ptDGain->fGain[HDR_LONG_FRAME_IDX];
    nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX],
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX]);

    gain_val = os04a10_dgain2value(nGainFromUser, &Gain_in, &Gain_de, &Gain_de2);
    if (gain_val == -1) {
        SNS_ERR("new gain match failed \n");
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX] = gain_val;
        result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_LONG_DGAIN_H_IDX, (Gain_in & 0x0F));
        result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_LONG_DGAIN_M_IDX, (Gain_de & 0xFF));
        result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_LONG_DGAIN_L_IDX, (Gain_de & 0xC0));
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    /* medium frame digital gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        nGainFromUser = ptDGain->fGain[HDR_MEDIUM_FRAME_IDX];
        nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX]);

        gain_val = os04a10_dgain2value(nGainFromUser, &Gain_in, &Gain_de, &Gain_de2);
        if (gain_val == -1) {
            SNS_ERR("new gain match failed \n");
        } else {
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_MEDIUM_FRAME_IDX] = gain_val;
            result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_SHORT_DGAIN_H_IDX, (Gain_in & 0x0F));
            result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_SHORT_DGAIN_M_IDX, (Gain_de & 0xFF));
            result = os04a10_sns_update_regidx_table(nPipeId, OS04A10_SHORT_DGAIN_L_IDX, (Gain_de & 0xC0));
            if (result != 0) {
                SNS_ERR("write hw failed %d \n", result);
                return result;
            }
        }
    }

    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->sztRegsInfo[0].tSnsExpInfo.szCurDGain[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX];
        sns_obj->sztRegsInfo[0].tSnsExpInfo.szCurDGain[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_MEDIUM_FRAME_IDX];

        SNS_DBG("dgain: long frame %f, short frame %f\n",
                sns_obj->sztRegsInfo[0].tSnsExpInfo.szCurDGain[HDR_LONG_FRAME_IDX],
                sns_obj->sztRegsInfo[0].tSnsExpInfo.szCurDGain[HDR_MEDIUM_FRAME_IDX]);
    }

    return SNS_SUCCESS;
}

AX_S32 os04a10_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio,
        AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange)
{
    AX_F32 ratio = 0.0f;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTimeRange);

    if (fabs(fHdrRatio) < EPS) {
        SNS_ERR("hdr ratio is error \n");
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    if (AX_SNS_HDR_2X_MODE == sns_obj->sns_mode_obj.eHDRMode) {

        ratio = fHdrRatio;
        ratio = AXSNS_CLIP3(ratio, sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio,
                            sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio);
        if (fabs(ratio) <= EPS) {
            SNS_ERR("hdr ratio is error \n");
        }

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = OS04A10_HDR_2X_LONG_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts, ratio);
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = OS04A10_HDR_2X_SHORT_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts, ratio);

        ptIntTimeRange->fMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->fMinIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX];
        ptIntTimeRange->fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX];

    } else if (AX_SNS_LINEAR_MODE == sns_obj->sns_mode_obj.eHDRMode) {

        ptIntTimeRange->fMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->fMinIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX];

    } else {
        // do nothing
    }

    return SNS_SUCCESS;
}

AX_S32 os04a10_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime)
{
    AX_U8 ex_h;
    AX_U8 ex_l;
    AX_U32 ex_ival = 0;
    //AX_S32 result = 0;
    AX_F32 fExpLineFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTime);

    SNS_DBG("Exptime:%.2f-%.2f-%.2f-%.2f, Hdrratio:%.2f-%.2f-%.2f-%.2f\n",
            ptIntTime->fIntTime[0], ptIntTime->fIntTime[1], ptIntTime->fIntTime[2], ptIntTime->fIntTime[3],
            ptIntTime->fHdrRatio[0], ptIntTime->fHdrRatio[1], ptIntTime->fHdrRatio[2], ptIntTime->fHdrRatio[3]);

    fExpLineFromUser = ptIntTime->fIntTime[HDR_LONG_FRAME_IDX];

    fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX]);

    ex_ival = fExpLineFromUser;
    ex_l = REG_LOW_8BITS(ex_ival);
    ex_h = REG_HIGH_8BITS(ex_ival);

    os04a10_sns_update_regidx_table(nPipeId, OS04A10_LONG_EXP_LINE_H_IDX, ex_h);
    os04a10_sns_update_regidx_table(nPipeId, OS04A10_LONG_EXP_LINE_L_IDX, ex_l);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX] = ex_ival;

    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        fExpLineFromUser = ptIntTime->fIntTime[HDR_MEDIUM_FRAME_IDX];

        fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX],
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX]);

        ex_ival = fExpLineFromUser;
        ex_l = REG_LOW_8BITS(ex_ival);
        ex_h = REG_HIGH_8BITS(ex_ival);

        os04a10_sns_update_regidx_table(nPipeId, OS04A10_SHORT_EXP_LINE_H_IDX, ex_h);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_SHORT_EXP_LINE_L_IDX, ex_l);

        sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_MEDIUM_FRAME_IDX] = ex_ival;
    }

    sns_obj->sztRegsInfo[0].tSnsExpInfo.szExpTime[HDR_LONG_FRAME_IDX] =
        sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX];

    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->sztRegsInfo[0].tSnsExpInfo.szExpTime[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_MEDIUM_FRAME_IDX];
    }

    SNS_DBG("Exp line: long frame %d, short frame %d\n",
            sns_obj->sztRegsInfo[0].tSnsExpInfo.szExpTime[HDR_LONG_FRAME_IDX],
            sns_obj->sztRegsInfo[0].tSnsExpInfo.szExpTime[HDR_MEDIUM_FRAME_IDX]);

    return SNS_SUCCESS;
}

AX_S32 os04a10_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(ptAeCtrlParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    memcpy(ptAeCtrlParam, &sns_obj->ae_ctrl_param, sizeof(AX_SNS_EXP_CTRL_PARAM_T));
    memcpy(&ptAeCtrlParam->sns_dev_attr, &sns_obj->sns_attr_param, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

/****************************************************************************
 * exposure control function
 ****************************************************************************/

AX_S32 os04a10_hcglcg_ctrl(ISP_PIPE_ID nPipeId, AX_U32 nSnsHcgLcg)
{
    AX_U8 hcglcg_value = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (AX_LCG_MODE == nSnsHcgLcg) {
        SNS_DBG("switch to LCG mode\n");
        sns_obj->ae_ctrl_param.eSnsHcgLcgMode = AX_LCG_MODE;
        hcglcg_value = os04a10_reg_read(nPipeId, OS04A10_HCG_LCG);
        hcglcg_value = hcglcg_value | 0x70; // l/s/vs hcg/lcg switch

        /* group hold */
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP1_IDX, 0x00);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP2_IDX, 0x01);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_HCG_LCG_IDX, hcglcg_value);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP3_IDX, 0x11);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP4_IDX, 0x05);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP5_IDX, 0x01);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP6_IDX, 0xA0);
    } else if (AX_HCG_MODE == nSnsHcgLcg) {
        SNS_DBG("switch to HCG mode \n");
        sns_obj->ae_ctrl_param.eSnsHcgLcgMode = AX_HCG_MODE;
        hcglcg_value = os04a10_reg_read(nPipeId, OS04A10_HCG_LCG);
        hcglcg_value = hcglcg_value & 0x8F; // l/s/vs hcg/lcg switch

        /* group hold */
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP1_IDX, 0x00);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP2_IDX, 0x01);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_HCG_LCG_IDX, hcglcg_value);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP3_IDX, 0x11);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP4_IDX, 0x05);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP5_IDX, 0x01);
        os04a10_sns_update_regidx_table(nPipeId, OS04A10_GROUP6_IDX, 0xA0);
    }

    return SNS_SUCCESS;
}

AX_S32 os04a10_sns_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    AX_U32  nRegValue;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    nNum = sizeof(gOs04a10AeRegsTable) / sizeof(gOs04a10AeRegsTable[0]);

    for (i = 0; i < nNum; i++) {
        nRegValue = os04a10_reg_read(nPipeId, gOs04a10AeRegsTable[i].nRegAddr);
        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr = gOs04a10AeRegsTable[i].nRegAddr;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nData = nRegValue;

        SNS_DBG(" nRegAddr 0x%x, nRegValue 0x%x\n", gOs04a10AeRegsTable[i].nRegAddr, gOs04a10AeRegsTable[i].nRegValue);
    }

    return SNS_SUCCESS;
}

AX_S32 os04a10_sns_update_init_exposure_reg(ISP_PIPE_ID nPipeId)
{
    int i = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        os04a10_write_register(nPipeId, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
        SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
            i, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
    }

    return SNS_SUCCESS;
}

static AX_VOID _sns_reg_info_init(ISP_PIPE_ID nPipeId, SNS_STATE_OBJ *sns_obj)
{
    AX_S32 i = 0;

    sns_obj->sztRegsInfo[0].eSnsType = ISP_SNS_CONNECT_I2C_TYPE;
    sns_obj->sztRegsInfo[0].tComBus.I2cDev = os04a10_get_bus_num(nPipeId);
    sns_obj->sztRegsInfo[0].nRegNum = sizeof(gOs04a10AeRegsTable) / sizeof(gOs04a10AeRegsTable[0]);
    sns_obj->sztRegsInfo[0].nCfg2ValidDelayMax = 2;

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDevAddr = OS04A10_SLAVE_ADDR;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nAddrByteNum = OS04A10_ADDR_BYTE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDataByteNum = OS04A10_DATA_BYTE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDelayFrmNum = gOs04a10AeRegsTable[i].nDelayFrmNum;
        if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].nIntPos = AX_SNS_L_FSOF;
        } else {
            sns_obj->sztRegsInfo[0].sztI2cData[i].nIntPos = AX_SNS_S_FSOF;
        }
        SNS_DBG("[%2d] nRegAddr 0x%x, nRegValue 0x%x\n", i,
                gOs04a10AeRegsTable[i].nRegAddr, gOs04a10AeRegsTable[i].nRegValue);
    }

    sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP1_IDX].nData = 0x00;
    sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP2_IDX].nData = 0x01;
    sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP3_IDX].nData = 0x11;
    sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP4_IDX].nData = 0x05;
    sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP5_IDX].nData = 0x01;
    sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP6_IDX].nData = 0xa0;

    return;
}

static AX_VOID _sns_reg_info_update(ISP_PIPE_ID nPipeId, SNS_STATE_OBJ *sns_obj, AX_BOOL *bUpdateReg)
{
    AX_S32 i = 0;
    // SNS_CHECK_PTR_VALID(bUpdateReg);

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        if (sns_obj->sztRegsInfo[0].sztI2cData[i].nData == sns_obj->sztRegsInfo[1].sztI2cData[i].nData) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_FALSE;
        } else {
            SNS_DBG("[%2d] nRegAddr 0x%x, nRegValue 0x%x\n", i,
                    sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);

            if (OS04A10_HCG_LCG_IDX != i) {
                sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
                *bUpdateReg = AX_TRUE;
            } else {
                sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP1_IDX].bUpdate = AX_TRUE;
                sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP2_IDX].bUpdate = AX_TRUE;
                sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_HCG_LCG_IDX].bUpdate = AX_TRUE;
                sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP3_IDX].bUpdate = AX_TRUE;
                sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP4_IDX].bUpdate = AX_TRUE;
                sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP5_IDX].bUpdate = AX_TRUE;
                sns_obj->sztRegsInfo[0].sztI2cData[OS04A10_GROUP6_IDX].bUpdate = AX_TRUE;

                *bUpdateReg = AX_TRUE;
                break;
            }
        }
    }

    return;
}

AX_S32 os04a10_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_BOOL bUpdateReg = AX_FALSE;

    SNS_CHECK_PTR_VALID(ptSnsRegsInfo);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if ((AX_FALSE == sns_obj->bSyncInit) || (AX_FALSE == ptSnsRegsInfo->bConfig)) {
        /* sync config */
        SNS_DBG(" bSyncInit %d, bConfig %d\n", sns_obj->bSyncInit, ptSnsRegsInfo->bConfig);

        _sns_reg_info_init(nPipeId, sns_obj);

        bUpdateReg = AX_TRUE;
        sns_obj->bSyncInit = AX_TRUE;
        // os04a10_sns_update_init_exposure_reg(nPipeId);
    } else {
        _sns_reg_info_update(nPipeId, sns_obj, &bUpdateReg);
    }

    if (AX_TRUE == bUpdateReg) {
        sns_obj->sztRegsInfo[0].bConfig = AX_FALSE;
    } else {
        sns_obj->sztRegsInfo[0].bConfig = AX_TRUE;
    }

    memcpy(ptSnsRegsInfo, &sns_obj->sztRegsInfo[0], sizeof(AX_SNS_REGS_CFG_TABLE_T));
    /* Save the current register table */
    memcpy(&sns_obj->sztRegsInfo[1], &sns_obj->sztRegsInfo[0], sizeof(AX_SNS_REGS_CFG_TABLE_T));

    return nRet;
}

static AX_U32 os04a10_set_vts(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vts_h;
    AX_U8 vts_l;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_l = vts & 0xFF;
    vts_h = (vts & 0xFF00) >> 8;

    result |= os04a10_sns_update_regidx_table(nPipeId, OS04A10_VTS_H_IDX, vts_h);
    result |= os04a10_sns_update_regidx_table(nPipeId, OS04A10_VTS_L_IDX, vts_l);

    return result;
}

AX_S32 os04a10_set_slow_fps(ISP_PIPE_ID nPipeId, AX_F32 nFps)
{
    AX_S32 result = 0;
    AX_S32 framerate = 30;
    AX_U32 vts = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate =  sns_obj->sns_mode_obj.fFrameRate;
    vts = sns_obj->sns_mode_obj.nVts;

    if (nFps >= framerate) {
        sns_os04a10params[nPipeId].vts = vts;
    } else {
        sns_os04a10params[nPipeId].vts = 1 * SNS_1_SECOND_UNIT_US / (sns_os04a10params[nPipeId].line_period * nFps);
    }

    if (sns_os04a10params[nPipeId].vts > OS04A10_MAX_VTS) {
        sns_os04a10params[nPipeId].vts = OS04A10_MAX_VTS;
        nFps = 1 * SNS_1_SECOND_UNIT_US / (sns_os04a10params[nPipeId].line_period * sns_os04a10params[nPipeId].vts);
        SNS_ERR("Beyond minmum fps  %f\n", nFps);
    }
    result = os04a10_set_vts(nPipeId, sns_os04a10params[nPipeId].vts);
    if (result != 0) {
        SNS_ERR("%s: write vts failed %d \n", __func__, result);
        return result;
    }

    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = OS04A10_HDR_2X_LONG_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts, OS04A10_MAX_RATIO);
    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = OS04A10_HDR_2X_SHORT_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts, OS04A10_MAX_RATIO);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = 1 * SNS_1_SECOND_UNIT_US / (sns_os04a10params[nPipeId].line_period *
            sns_os04a10params[nPipeId].vts);

    SNS_DBG("nPipeId = %d, fps(from alg) = %f, current vts = 0x%x\n", nPipeId, nFps, sns_os04a10params[nPipeId].vts);

    return SNS_SUCCESS;
}

AX_S32 os04a10_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam)
{
    AX_S32 framerate = 30;
    AX_U32 nfps = 0;
    AX_U32 nVts = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;


    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate = sns_obj->sns_mode_obj.fFrameRate;
    if (SNS_MAX_FRAME_RATE < framerate) {
        SNS_ERR("framerate out of range : %d\n", framerate);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }
    if (sns_os04a10params[nPipeId].line_period == 0) {
        SNS_ERR("line_period is zero : %f\n", sns_os04a10params[nPipeId].line_period);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    ptSlowShutterParam->nGroupNum = AXSNS_MIN((sizeof(gFpsGear) / sizeof(AX_F32)), framerate);
    //ax_sns_quick_sort_float(gFpsGear, ptSlowShutterParam->nGroupNum);
    ptSlowShutterParam->fMinFps = AXSNS_MAX(gFpsGear[0],
                                            (1 * SNS_1_SECOND_UNIT_US / (sns_os04a10params[nPipeId].line_period * OS04A10_MAX_VTS)));

    for (nfps = 0 ; nfps < ptSlowShutterParam->nGroupNum; nfps++) {
        nVts = 1 * SNS_1_SECOND_UNIT_US / (sns_os04a10params[nPipeId].line_period * gFpsGear[nfps]);
        if ((AX_S32)gFpsGear[nfps] >= framerate) {
            nVts = sns_obj->sns_mode_obj.nVts;
        }
        if (nVts > OS04A10_MAX_VTS) {
            nVts = OS04A10_MAX_VTS;
            SNS_WRN("Beyond minmum fps  %f\n", ptSlowShutterParam->fMinFps);
        }

        if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime = OS04A10_SDR_EXP_LINE_MAX(nVts);
        } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime = OS04A10_HDR_2X_LONG_EXP_LINE_MAX(nVts, OS04A10_MAX_RATIO);
        }

        ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps = 1 * SNS_1_SECOND_UNIT_US / (sns_os04a10params[nPipeId].line_period
                * nVts);
        ptSlowShutterParam->fMaxFps  =  ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps;

        SNS_DBG("nPipeId = %d, line_period = %.2f, fps = %.2f, fMaxIntTime = %.2f, vts=0x%x\n",
                nPipeId, sns_os04a10params[nPipeId].line_period,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime, nVts);
    }

    return SNS_SUCCESS;
}

AX_S32 os04a10_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps)
{
    AX_S32 result = 0;
    AX_U32 vts = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if(IS_SNS_FPS_EQUAL(fFps, 25.0) || IS_SNS_FPS_EQUAL(fFps, 50.0)) {  /* PAL */
        if(AX_SNS_LINEAR_MODE == sns_obj->sns_attr_param.eSnsMode) {
            if(AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) {
                vts = OS04A10_VTS_10BIT_4M25_SDR;  /* SDR 10BIT 4LINE */
                if (sns_obj->sns_attr_param.nSettingIndex > 4) {
                    vts = OS04A10_VTS_10BIT_4M25_SDR_2LINE;  /* SDR 10BIT 2LINE */
                }
            }
            if(AX_RT_RAW12 == sns_obj->sns_attr_param.eRawType) {
                vts = OS04A10_VTS_12BIT_4M25_SDR;  /* SDR 12BIT 4LINE */
            }
        }
        if(AX_SNS_HDR_2X_MODE == sns_obj->sns_attr_param.eSnsMode) {
            vts = OS04A10_VTS_10BIT_4M25_HDR_2X;  /* HDR 10BIT 4LINE */
            if (sns_obj->sns_attr_param.nSettingIndex > 4) {
                vts = OS04A10_VTS_10BIT_4M25_HDR_2X_2LINE;  /* HDR 10BIT 2LINE */
            }
        }
    }
    else if(IS_SNS_FPS_EQUAL(fFps, 30.0)) {  /* NTSC */
        if(AX_SNS_LINEAR_MODE == sns_obj->sns_attr_param.eSnsMode) {
            if(AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) {
                vts = OS04A10_VTS_10BIT_4M30_SDR;  /* SDR 10BIT 4LINE */
                if (sns_obj->sns_attr_param.nSettingIndex > 4) {
                    vts = OS04A10_VTS_10BIT_4M30_SDR_2LINE;  /* SDR 10BIT 2LINE */
                }
            }
            if(AX_RT_RAW12 == sns_obj->sns_attr_param.eRawType) {
                vts = OS04A10_VTS_12BIT_4M30_SDR;  /* SDR 12BIT 4LINE */
            }
        }
        if(AX_SNS_HDR_2X_MODE == sns_obj->sns_attr_param.eSnsMode) {
            vts = OS04A10_VTS_10BIT_4M30_HDR_2X;  /* HDR 10BIT 4LINE */
        }
    }
    else if (IS_SNS_FPS_EQUAL(fFps, 60.0)) {
        if(AX_SNS_LINEAR_MODE == sns_obj->sns_attr_param.eSnsMode) {
            if(AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) {
                vts = OS04A10_VTS_12BIT_4M60_SDR;  /* SDR 10BIT 4LINE */
            }
        }
    }
    else if (IS_SNS_FPS_EQUAL(fFps, 15.0)) {  /* NTSC */
        if (AX_SNS_LINEAR_MODE == sns_obj->sns_attr_param.eSnsMode) {
            if (AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) {
                vts = OS04A10_VTS_10BIT_4M15_SDR;  /* SDR 10BIT 4LINE */
            }
        }
    }

    sns_os04a10params[nPipeId].vts = vts;

    /* set sensor vts */
    result = os04a10_set_vts(nPipeId, sns_os04a10params[nPipeId].vts);
    if (result != 0) {
        SNS_ERR("%s: write vts failed %d \n", __func__, result);
        return result;
    }

    /* change fps */
    sns_obj->sns_mode_obj.nVts = sns_os04a10params[nPipeId].vts;
    sns_obj->sns_mode_obj.fFrameRate = fFps;

    /* update exposure limit parameters */
    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = OS04A10_HDR_2X_LONG_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts, OS04A10_MAX_RATIO);
    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = OS04A10_HDR_2X_SHORT_EXP_LINE_MAX(sns_os04a10params[nPipeId].vts, OS04A10_MAX_RATIO);
    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = fFps;

    SNS_DBG("nPipeId = %d, fps(from alg) = %f, current vts = 0x%x\n", nPipeId, fFps, sns_os04a10params[nPipeId].vts);

    return SNS_SUCCESS;
}
