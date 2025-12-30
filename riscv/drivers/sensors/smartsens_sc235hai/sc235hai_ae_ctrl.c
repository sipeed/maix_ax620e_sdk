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

#include "sc235hai_reg.h"
#include "sc235hai_ae_ctrl.h"


#define SC235HAI_MAX_VTS         (0x7FFF)
#define SC235HAI_MAX_RATIO       (16.0f)
#define SC235HAI_MIN_RATIO       (1.0f)

#define SC235HAI_MAX_AGAIN       (116.550f)
#define SC235HAI_MAX_DGAIN       (15.875f)

#define SC235HAI_SDR_EXP_LINE_MIN                                      (1.0f)
#define SC235HAI_SDR_EXP_LINE_MAX(vts)                                 (2 * vts - 11)
#define SC235HAI_SDR_INTEGRATION_TIME_STEP                             (0.5f)

#define SC235HAI_HDR_2X_LONG_EXP_LINE_MIN                              (1.0f)
#define SC235HAI_HDR_2X_LONG_EXP_LINE_MAX(vts, vts_s)                  (2 * (vts - vts_s) - 21)
#define SC235HAI_HDR_2X_LONG_INTEGRATION_TIME_STEP                     (2.0f)

#define SC235HAI_HDR_2X_SHORT_EXP_LINE_MIN                             (1.0f)
#define SC235HAI_HDR_2X_SHORT_EXP_LINE_MAX(vts_s)                      (2 * vts_s - 19)
#define SC235HAI_HDR_2X_SHORT_INTEGRATION_TIME_STEP                    (2.0f)

typedef struct _SNSSC235HAI_OBJ_T_ {
    AX_U32 hts;
    AX_U32 vts;
    AX_U32 vts_s;
    AX_U32 setting_vts;
    AX_F32 setting_fps;
} SNSSC235HAI_OBJ_T;

typedef struct _SC235HAI_GAIN_TABLE_T_ {
    float gain;
    AX_U8 gain_in;
    AX_U8 gain_de;
} SC235HAI_GAIN_TABLE_T;

extern SNS_STATE_OBJ *g_szsc235haiCtx[AX_VIN_MAX_PIPE_NUM];
extern AX_U8 gSc235haiSlaveAddr[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szsc235haiCtx[dev])

static SNSSC235HAI_OBJ_T sns_sc235haiparams[AX_VIN_MAX_PIPE_NUM];
static AX_F32 nAgainTable[SENSOR_MAX_GAIN_STEP];
static AX_F32 nDgainTable[SENSOR_MAX_GAIN_STEP];

/*user config*/
static AX_F32 gFpsGear[] = {1.00, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00,
                            11.00, 12.00, 13.00, 14.00, 15.00, 16.00, 17.00, 18.00, 19.00, 20.00,
                            21.00, 22.00, 23.00, 24.00, 25.00, 26.00, 27.00, 28.00, 29.00, 30.00
                           };

static AX_SNS_DRV_DELAY_TABLE_T gsc235haiAeRegsTable[] = {
    /* regs index */          /* regs addr */                /*regs value*/        /*Delay Frame Num*/
    {0, SC235HAI_LONG_EXP_LINE_H,        0,          0},
    {1, SC235HAI_LONG_EXP_LINE_M,        0,          0},
    {2, SC235HAI_LONG_EXP_LINE_L,        0,          0},
    {3, SC235HAI_SHORT_EXP_LINE_H,       0,          0},
    {4, SC235HAI_SHORT_EXP_LINE_M,       0,          0},
    {5, SC235HAI_SHORT_EXP_LINE_L,       0,          0},
    {6, SC235HAI_LONG_AGAIN_H,           0,          0},
    {7, SC235HAI_LONG_AGAIN_L,           0,          0},
    {8, SC235HAI_LONG_DGAIN_H,           0,          0},
    {9, SC235HAI_LONG_DGAIN_L,           0,          0},
    {10,SC235HAI_SHORT_AGAIN_H,          0,          0},
    {11,SC235HAI_SHORT_AGAIN_L,          0,          0},
    {12,SC235HAI_SHORT_DGAIN_H,          0,          0},
    {13,SC235HAI_SHORT_DGAIN_L,          0,          0},
};

const SC235HAI_GAIN_TABLE_T sc235hai_again_table[] = {
    {1.000,  0x00, 0x20},
    {1.031,  0x00, 0x21},
    {1.063,  0x00, 0x22},
    {1.094,  0x00, 0x23},
    {1.125,  0x00, 0x24},
    {1.156,  0x00, 0x25},
    {1.188,  0x00, 0x26},
    {1.219,  0x00, 0x27},
    {1.250,  0x00, 0x28},
    {1.281,  0x00, 0x29},
    {1.313,  0x00, 0x2A},
    {1.344,  0x00, 0x2B},
    {1.375,  0x00, 0x2C},
    {1.406,  0x00, 0x2D},
    {1.438,  0x00, 0x2E},
    {1.469,  0x00, 0x2F},
    {1.500,  0x00, 0x30},
    {1.531,  0x00, 0x31},
    {1.563,  0x00, 0x32},
    {1.594,  0x00, 0x33},
    {1.625,  0x00, 0x34},
    {1.656,  0x00, 0x35},
    {1.688,  0x00, 0x36},
    {1.719,  0x00, 0x37},
    {1.750,  0x00, 0x38},
    {1.781,  0x00, 0x39},
    {1.813,  0x00, 0x3A},
    {1.844,  0x00, 0x3B},
    {1.875,  0x00, 0x3C},
    {1.906,  0x00, 0x3D},
    {1.938,  0x00, 0x3E},
    {1.969,  0x00, 0x3F},
    {2.000,  0x01, 0x20},
    {2.063,  0x01, 0x21},
    {2.125,  0x01, 0x22},
    {2.188,  0x01, 0x23},
    {2.250,  0x01, 0x24},
    {2.313,  0x01, 0x25},
    {2.375,  0x01, 0x26},
    {2.438,  0x01, 0x27},
    {2.500,  0x01, 0x28},
    {2.563,  0x01, 0x29},
    {2.625,  0x01, 0x2A},
    {2.688,  0x01, 0x2B},
    {2.750,  0x01, 0x2C},
    {2.813,  0x01, 0x2D},
    {2.875,  0x01, 0x2E},
    {2.938,  0x01, 0x2F},
    {3.000,  0x01, 0x30},
    {3.063,  0x01, 0x31},
    {3.125,  0x01, 0x32},
    {3.188,  0x01, 0x33},
    {3.250,  0x01, 0x34},
    {3.313,  0x01, 0x35},
    {3.375,  0x01, 0x36},
    {3.438,  0x01, 0x37},
    {3.500,  0x01, 0x38},
    {3.563,  0x01, 0x39},
    {3.625,  0x01, 0x3A},
    {3.688,  0x01, 0x3B},
    //{3.700,  0x80, 0x20},
    {3.816,  0x80, 0x21},
    {3.931,  0x80, 0x22},
    {4.047,  0x80, 0x23},
    {4.163,  0x80, 0x24},
    {4.278,  0x80, 0x25},
    {4.394,  0x80, 0x26},
    {4.509,  0x80, 0x27},
    {4.625,  0x80, 0x28},
    {4.741,  0x80, 0x29},
    {4.856,  0x80, 0x2A},
    {4.972,  0x80, 0x2B},
    {5.088,  0x80, 0x2C},
    {5.203,  0x80, 0x2D},
    {5.319,  0x80, 0x2E},
    {5.434,  0x80, 0x2F},
    {5.550,  0x80, 0x30},
    {5.666,  0x80, 0x31},
    {5.781,  0x80, 0x32},
    {5.897,  0x80, 0x33},
    {6.013,  0x80, 0x34},
    {6.128,  0x80, 0x35},
    {6.244,  0x80, 0x36},
    {6.359,  0x80, 0x37},
    {6.475,  0x80, 0x38},
    {6.591,  0x80, 0x39},
    {6.706,  0x80, 0x3A},
    {6.822,  0x80, 0x3B},
    {6.938,  0x80, 0x3C},
    {7.053,  0x80, 0x3D},
    {7.169,  0x80, 0x3E},
    {7.284,  0x80, 0x3F},
    //{7.400,  0x81, 0x20},
    {7.631,  0x81, 0x21},
    {7.863,  0x81, 0x22},
    {8.094,  0x81, 0x23},
    {8.325,  0x81, 0x24},
    {8.556,  0x81, 0x25},
    {8.788,  0x81, 0x26},
    {9.019,  0x81, 0x27},
    {9.250,  0x81, 0x28},
    {9.481,  0x81, 0x29},
    {9.713,  0x81, 0x2A},
    {9.944,  0x81, 0x2B},
    {10.175, 0x81, 0x2C},
    {10.406, 0x81, 0x2D},
    {10.638, 0x81, 0x2E},
    {10.869, 0x81, 0x2F},
    {11.100, 0x81, 0x30},
    {11.331, 0x81, 0x31},
    {11.563, 0x81, 0x32},
    {11.794, 0x81, 0x33},
    {12.025, 0x81, 0x34},
    {12.256, 0x81, 0x35},
    {12.488, 0x81, 0x36},
    {12.719, 0x81, 0x37},
    {12.950, 0x81, 0x38},
    {13.181, 0x81, 0x39},
    {13.413, 0x81, 0x3A},
    {13.644, 0x81, 0x3B},
    {13.875, 0x81, 0x3C},
    {14.106, 0x81, 0x3D},
    {14.338, 0x81, 0x3E},
    {14.569, 0x81, 0x3F},
    {14.800, 0x83, 0x20},
    {15.263, 0x83, 0x21},
    {15.725, 0x83, 0x22},
    {16.188, 0x83, 0x23},
    {16.650, 0x83, 0x24},
    {17.113, 0x83, 0x25},
    {17.575, 0x83, 0x26},
    {18.038, 0x83, 0x27},
    {18.500, 0x83, 0x28},
    {18.963, 0x83, 0x29},
    {19.425, 0x83, 0x2A},
    {19.888, 0x83, 0x2B},
    {20.350, 0x83, 0x2C},
    {20.813, 0x83, 0x2D},
    {21.275, 0x83, 0x2E},
    {21.738, 0x83, 0x2F},
    {22.200, 0x83, 0x30},
    {22.663, 0x83, 0x31},
    {23.125, 0x83, 0x32},
    {23.588, 0x83, 0x33},
    {24.050, 0x83, 0x34},
    {24.513, 0x83, 0x35},
    {24.975, 0x83, 0x36},
    {25.438, 0x83, 0x37},
    {25.900, 0x83, 0x38},
    {26.363, 0x83, 0x39},
    {26.825, 0x83, 0x3A},
    {27.288, 0x83, 0x3B},
    {27.750, 0x83, 0x3C},
    {28.213, 0x83, 0x3D},
    {28.675, 0x83, 0x3E},
    {29.138, 0x83, 0x3F},
    {29.600, 0x87, 0x20},
    {30.525, 0x87, 0x21},
    {31.450, 0x87, 0x22},
    {32.375, 0x87, 0x23},
    {33.300, 0x87, 0x24},
    {34.225, 0x87, 0x25},
    {35.150, 0x87, 0x26},
    {36.075, 0x87, 0x27},
    {37.000, 0x87, 0x28},
    {37.925, 0x87, 0x29},
    {38.850, 0x87, 0x2A},
    {39.775, 0x87, 0x2B},
    {40.700, 0x87, 0x2C},
    {41.625, 0x87, 0x2D},
    {42.550, 0x87, 0x2E},
    {43.475, 0x87, 0x2F},
    {44.400, 0x87, 0x30},
    {45.325, 0x87, 0x31},
    {46.250, 0x87, 0x32},
    {47.175, 0x87, 0x33},
    {48.100, 0x87, 0x34},
    {49.025, 0x87, 0x35},
    {49.950, 0x87, 0x36},
    {50.875, 0x87, 0x37},
    {51.800, 0x87, 0x38},
    {52.725, 0x87, 0x39},
    {53.650, 0x87, 0x3A},
    {54.575, 0x87, 0x3B},
    {55.500, 0x87, 0x3C},
    {56.425, 0x87, 0x3D},
    {57.350, 0x87, 0x3E},
    {58.275, 0x87, 0x3F},
    {59.200, 0x8F, 0x20},
    {61.050, 0x8F, 0x21},
    {62.900, 0x8F, 0x22},
    {64.750, 0x8F, 0x23},
    {66.600, 0x8F, 0x24},
    {68.450, 0x8F, 0x25},
    {70.300, 0x8F, 0x26},
    {72.150, 0x8F, 0x27},
    {74.000, 0x8F, 0x28},
    {75.850, 0x8F, 0x29},
    {77.700, 0x8F, 0x2A},
    {79.550, 0x8F, 0x2B},
    {81.400, 0x8F, 0x2C},
    {83.250, 0x8F, 0x2D},
    {85.100, 0x8F, 0x2E},
    {86.950, 0x8F, 0x2F},
    {88.800, 0x8F, 0x30},
    {90.650, 0x8F, 0x31},
    {92.500, 0x8F, 0x32},
    {94.350, 0x8F, 0x33},
    {96.200, 0x8F, 0x34},
    {98.050, 0x8F, 0x35},
    {99.900, 0x8F, 0x36},
    {101.750,0x8F, 0x37},
    {103.600,0x8F, 0x38},
    {105.450,0x8F, 0x39},
    {107.300,0x8F, 0x3A},
    {109.150,0x8F, 0x3B},
    {111.000,0x8F, 0x3C},
    {112.850,0x8F, 0x3D},
    {114.700,0x8F, 0x3E},
    {116.550,0x8F, 0x3F},
};

const SC235HAI_GAIN_TABLE_T sc235hai_dgain_table[] = {
    {1.000,  0x00, 0x80},
    {1.016,  0x00, 0x82},
    {1.031,  0x00, 0x84},
    {1.047,  0x00, 0x86},
    {1.063,  0x00, 0x88},
    {1.078,  0x00, 0x8a},
    {1.094,  0x00, 0x8c},
    {1.109,  0x00, 0x8e},
    {1.125,  0x00, 0x90},
    {1.141,  0x00, 0x92},
    {1.156,  0x00, 0x94},
    {1.172,  0x00, 0x96},
    {1.188,  0x00, 0x98},
    {1.203,  0x00, 0x9a},
    {1.219,  0x00, 0x9c},
    {1.234,  0x00, 0x9e},
    {1.250,  0x00, 0xa0},
    {1.266,  0x00, 0xa2},
    {1.281,  0x00, 0xa4},
    {1.297,  0x00, 0xa6},
    {1.313,  0x00, 0xa8},
    {1.328,  0x00, 0xaa},
    {1.344,  0x00, 0xac},
    {1.359,  0x00, 0xae},
    {1.375,  0x00, 0xb0},
    {1.391,  0x00, 0xb2},
    {1.406,  0x00, 0xb4},
    {1.422,  0x00, 0xb6},
    {1.438,  0x00, 0xb8},
    {1.453,  0x00, 0xba},
    {1.469,  0x00, 0xbc},
    {1.484,  0x00, 0xbe},
    {1.500,  0x00, 0xc0},
    {1.516,  0x00, 0xc2},
    {1.531,  0x00, 0xc4},
    {1.547,  0x00, 0xc6},
    {1.563,  0x00, 0xc8},
    {1.578,  0x00, 0xca},
    {1.594,  0x00, 0xcc},
    {1.609,  0x00, 0xce},
    {1.625,  0x00, 0xd0},
    {1.641,  0x00, 0xd2},
    {1.656,  0x00, 0xd4},
    {1.672,  0x00, 0xd6},
    {1.688,  0x00, 0xd8},
    {1.703,  0x00, 0xda},
    {1.719,  0x00, 0xdc},
    {1.734,  0x00, 0xde},
    {1.750,  0x00, 0xe0},
    {1.766,  0x00, 0xe2},
    {1.781,  0x00, 0xe4},
    {1.797,  0x00, 0xe6},
    {1.813,  0x00, 0xe8},
    {1.828,  0x00, 0xea},
    {1.844,  0x00, 0xec},
    {1.859,  0x00, 0xee},
    {1.875,  0x00, 0xf0},
    {1.891,  0x00, 0xf2},
    {1.906,  0x00, 0xf4},
    {1.922,  0x00, 0xf6},
    {1.938,  0x00, 0xf8},
    {1.953,  0x00, 0xfa},
    {1.969,  0x00, 0xfc},
    {1.984,  0x00, 0xfe},
    {2.000,  0x01, 0x80},
    {2.031,  0x01, 0x82},
    {2.063,  0x01, 0x84},
    {2.094,  0x01, 0x86},
    {2.125,  0x01, 0x88},
    {2.156,  0x01, 0x8a},
    {2.188,  0x01, 0x8c},
    {2.219,  0x01, 0x8e},
    {2.250,  0x01, 0x90},
    {2.281,  0x01, 0x92},
    {2.313,  0x01, 0x94},
    {2.344,  0x01, 0x96},
    {2.375,  0x01, 0x98},
    {2.406,  0x01, 0x9a},
    {2.438,  0x01, 0x9c},
    {2.469,  0x01, 0x9e},
    {2.500,  0x01, 0xa0},
    {2.531,  0x01, 0xa2},
    {2.563,  0x01, 0xa4},
    {2.594,  0x01, 0xa6},
    {2.625,  0x01, 0xa8},
    {2.656,  0x01, 0xaa},
    {2.688,  0x01, 0xac},
    {2.719,  0x01, 0xae},
    {2.750,  0x01, 0xb0},
    {2.781,  0x01, 0xb2},
    {2.813,  0x01, 0xb4},
    {2.844,  0x01, 0xb6},
    {2.875,  0x01, 0xb8},
    {2.906,  0x01, 0xba},
    {2.938,  0x01, 0xbc},
    {2.969,  0x01, 0xbe},
    {3.000,  0x01, 0xc0},
    {3.031,  0x01, 0xc2},
    {3.063,  0x01, 0xc4},
    {3.094,  0x01, 0xc6},
    {3.125,  0x01, 0xc8},
    {3.156,  0x01, 0xca},
    {3.188,  0x01, 0xcc},
    {3.219,  0x01, 0xce},
    {3.250,  0x01, 0xd0},
    {3.281,  0x01, 0xd2},
    {3.313,  0x01, 0xd4},
    {3.344,  0x01, 0xd6},
    {3.375,  0x01, 0xd8},
    {3.406,  0x01, 0xda},
    {3.438,  0x01, 0xdc},
    {3.469,  0x01, 0xde},
    {3.500,  0x01, 0xe0},
    {3.531,  0x01, 0xe2},
    {3.563,  0x01, 0xe4},
    {3.594,  0x01, 0xe6},
    {3.625,  0x01, 0xe8},
    {3.656,  0x01, 0xea},
    {3.688,  0x01, 0xec},
    {3.719,  0x01, 0xee},
    {3.750,  0x01, 0xf0},
    {3.781,  0x01, 0xf2},
    {3.813,  0x01, 0xf4},
    {3.844,  0x01, 0xf6},
    {3.875,  0x01, 0xf8},
    {3.906,  0x01, 0xfa},
    {3.938,  0x01, 0xfc},
    {3.969,  0x01, 0xfe},
    {4.000,  0x03, 0x80},
    {4.063,  0x03, 0x82},
    {4.125,  0x03, 0x84},
    {4.188,  0x03, 0x86},
    {4.250,  0x03, 0x88},
    {4.313,  0x03, 0x8a},
    {4.375,  0x03, 0x8c},
    {4.438,  0x03, 0x8e},
    {4.500,  0x03, 0x90},
    {4.563,  0x03, 0x92},
    {4.625,  0x03, 0x94},
    {4.688,  0x03, 0x96},
    {4.750,  0x03, 0x98},
    {4.813,  0x03, 0x9a},
    {4.875,  0x03, 0x9c},
    {4.938,  0x03, 0x9e},
    {5.000,  0x03, 0xa0},
    {5.063,  0x03, 0xa2},
    {5.125,  0x03, 0xa4},
    {5.188,  0x03, 0xa6},
    {5.250,  0x03, 0xa8},
    {5.313,  0x03, 0xaa},
    {5.375,  0x03, 0xac},
    {5.438,  0x03, 0xae},
    {5.500,  0x03, 0xb0},
    {5.563,  0x03, 0xb2},
    {5.625,  0x03, 0xb4},
    {5.688,  0x03, 0xb6},
    {5.750,  0x03, 0xb8},
    {5.813,  0x03, 0xba},
    {5.875,  0x03, 0xbc},
    {5.938,  0x03, 0xbe},
    {6.000,  0x03, 0xc0},
    {6.063,  0x03, 0xc2},
    {6.125,  0x03, 0xc4},
    {6.188,  0x03, 0xc6},
    {6.250,  0x03, 0xc8},
    {6.313,  0x03, 0xca},
    {6.375,  0x03, 0xcc},
    {6.438,  0x03, 0xce},
    {6.500,  0x03, 0xd0},
    {6.563,  0x03, 0xd2},
    {6.625,  0x03, 0xd4},
    {6.688,  0x03, 0xd6},
    {6.750,  0x03, 0xd8},
    {6.813,  0x03, 0xda},
    {6.875,  0x03, 0xdc},
    {6.938,  0x03, 0xde},
    {7.000,  0x03, 0xe0},
    {7.063,  0x03, 0xe2},
    {7.125,  0x03, 0xe4},
    {7.188,  0x03, 0xe6},
    {7.250,  0x03, 0xe8},
    {7.313,  0x03, 0xea},
    {7.375,  0x03, 0xec},
    {7.438,  0x03, 0xee},
    {7.500,  0x03, 0xf0},
    {7.563,  0x03, 0xf2},
    {7.625,  0x03, 0xf4},
    {7.688,  0x03, 0xf6},
    {7.750,  0x03, 0xf8},
    {7.813,  0x03, 0xfa},
    {7.875,  0x03, 0xfc},
    {7.938,  0x03, 0xfe},
    {8.000,  0x07, 0x80},
    {8.125,  0x07, 0x82},
    {8.250,  0x07, 0x84},
    {8.375,  0x07, 0x86},
    {8.500,  0x07, 0x88},
    {8.625,  0x07, 0x8a},
    {8.750,  0x07, 0x8c},
    {8.875,  0x07, 0x8e},
    {9.000,  0x07, 0x90},
    {9.125,  0x07, 0x92},
    {9.250,  0x07, 0x94},
    {9.375,  0x07, 0x96},
    {9.500,  0x07, 0x98},
    {9.625,  0x07, 0x9a},
    {9.750,  0x07, 0x9c},
    {9.875,  0x07, 0x9e},
    {10.000, 0x07, 0xa0},
    {10.125, 0x07, 0xa2},
    {10.250, 0x07, 0xa4},
    {10.375, 0x07, 0xa6},
    {10.500, 0x07, 0xa8},
    {10.625, 0x07, 0xaa},
    {10.750, 0x07, 0xac},
    {10.875, 0x07, 0xae},
    {11.000, 0x07, 0xb0},
    {11.125, 0x07, 0xb2},
    {11.250, 0x07, 0xb4},
    {11.375, 0x07, 0xb6},
    {11.500, 0x07, 0xb8},
    {11.625, 0x07, 0xba},
    {11.750, 0x07, 0xbc},
    {11.875, 0x07, 0xbe},
    {12.000, 0x07, 0xc0},
    {12.125, 0x07, 0xc2},
    {12.250, 0x07, 0xc4},
    {12.375, 0x07, 0xc6},
    {12.500, 0x07, 0xc8},
    {12.625, 0x07, 0xca},
    {12.750, 0x07, 0xcc},
    {12.875, 0x07, 0xce},
    {13.000, 0x07, 0xd0},
    {13.125, 0x07, 0xd2},
    {13.250, 0x07, 0xd4},
    {13.375, 0x07, 0xd6},
    {13.500, 0x07, 0xd8},
    {13.625, 0x07, 0xda},
    {13.750, 0x07, 0xdc},
    {13.875, 0x07, 0xde},
    {14.000, 0x07, 0xe0},
    {14.125, 0x07, 0xe2},
    {14.250, 0x07, 0xe4},
    {14.375, 0x07, 0xe6},
    {14.500, 0x07, 0xe8},
    {14.625, 0x07, 0xea},
    {14.750, 0x07, 0xec},
    {14.875, 0x07, 0xee},
    {15.000, 0x07, 0xf0},
    {15.125, 0x07, 0xf2},
    {15.250, 0x07, 0xf4},
    {15.375, 0x07, 0xf6},
    {15.500, 0x07, 0xf8},
    {15.625, 0x07, 0xfa},
    {15.750, 0x07, 0xfc},
    {15.875, 0x07, 0xfe},
};

static AX_S32 sc235hai_again2value(AX_F32 gain, AX_U8 *again_in, AX_U8 *again_de, AX_F32 *gain_value)
{
    AX_U32 i;
    AX_U32 count;

    if (!again_in || !again_de)
        return AX_SNS_ERR_NULL_PTR;

    count = sizeof(sc235hai_again_table) / sizeof(sc235hai_again_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > sc235hai_again_table[i].gain) {
            continue;
        } else {
            *again_in = sc235hai_again_table[i].gain_in;
            *again_de = sc235hai_again_table[i].gain_de;
            *gain_value = sc235hai_again_table[i].gain;
            SNS_DBG("again=%f, again_in=0x%x, again_de=0x%x\n", gain, *again_in, *again_de);
            return AX_SNS_SUCCESS;
        }
    }

    return AX_SNS_ERR_NOT_MATCH;
}

static AX_S32 sc235hai_dgain2value(AX_F32 gain, AX_U8 *dgain_in, AX_U8 *dgain_de, AX_U8 *dgain_de2, AX_F32 *gain_value)
{
    AX_U32 i;
    AX_U32 count;

    if (!dgain_in || !dgain_de)
        return AX_SNS_ERR_NULL_PTR;

    count = sizeof(sc235hai_dgain_table) / sizeof(sc235hai_dgain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > sc235hai_dgain_table[i].gain) {
            continue;
        } else {
            *dgain_in = sc235hai_dgain_table[i].gain_in;
            *dgain_de = sc235hai_dgain_table[i].gain_de;
            *gain_value = sc235hai_dgain_table[i].gain;
            SNS_DBG("dgain=%f, dgain_in=0x%x, dgain_de=0x%x\n", gain, *dgain_in, *dgain_de);
            return AX_SNS_SUCCESS;
        }
    }

    return AX_SNS_ERR_NOT_MATCH;
}


static AX_S32 sc235hai_get_gain_table(AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_U32 i;
    AX_S32 ret = 0;
    if (!params)
        return AX_SNS_ERR_NULL_PTR;

    params->nAgainTableSize = sizeof(sc235hai_again_table) / sizeof(sc235hai_again_table[0]);
    params->nDgainTableSize = sizeof(sc235hai_dgain_table) / sizeof(sc235hai_dgain_table[0]);

    for (i = 0; i < params->nAgainTableSize ; i++) {
        nAgainTable[i] = sc235hai_again_table[i].gain;
        params->pAgainTable = nAgainTable;
    }

    for (i = 0; i < params->nDgainTableSize ; i++) {
        nDgainTable[i] = sc235hai_dgain_table[i].gain;
        params->pDgainTable = nDgainTable;
    }

    return ret;
}


/****************************************************************************
 * exposure control external function
 ****************************************************************************/
static AX_S32 sc235hai_set_exp_limit(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio)
{
    AX_U32 vts_s = 0;
    AX_F32 ratio = 1.0;
    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_LINEAR_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = SC235HAI_SDR_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            SC235HAI_SDR_EXP_LINE_MAX(sns_sc235haiparams[nPipeId].vts) / 2.0f;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = SC235HAI_SDR_INTEGRATION_TIME_STEP;
    } else if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_HDR_2X_MODE) {
        //2*vts - 2*vts_s - 21 = (2*vts_s - 19) * fHdrRatio
        vts_s = (2 * sns_sc235haiparams[nPipeId].vts + 19 * fHdrRatio - 21) / (2 * fHdrRatio + 2);
        SNS_DBG("hdr vts_s:%d, vts:%d, ratio:%f\n", vts_s, sns_sc235haiparams[nPipeId].vts, fHdrRatio);

        sns_sc235haiparams[nPipeId].vts_s = vts_s;
        sc235hai_set_vts_s(nPipeId, sns_sc235haiparams[nPipeId].vts_s);

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = SC235HAI_HDR_2X_LONG_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] = SC235HAI_HDR_2X_SHORT_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = SC235HAI_HDR_2X_LONG_EXP_LINE_MAX(sns_sc235haiparams[nPipeId].vts, vts_s) / 2.0f;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = SC235HAI_HDR_2X_SHORT_EXP_LINE_MAX(vts_s) / 2.0f;

        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = SC235HAI_HDR_2X_LONG_INTEGRATION_TIME_STEP;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_MEDIUM_FRAME_IDX] = SC235HAI_HDR_2X_SHORT_INTEGRATION_TIME_STEP;

        ratio = (float)sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] /
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX];

    }

    SNS_DBG("userRatio:%.2f, userVts:0x%x, userVts_s:0x%x, limitRatio:%.2f, limitExp:%.2f-%.2f-%.2f-%.2f\n",
            fHdrRatio, sns_sc235haiparams[nPipeId].vts, sns_sc235haiparams[nPipeId].vts_s, ratio,
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX]);

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_get_params_from_setting(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    AX_U32 reg_cnt = 0;
    camera_i2c_reg_array *setting = AX_NULL;
    AX_U32 vts = 0;

    ret = sc235hai_select_setting(nPipeId, &setting, &reg_cnt);
    if (ret) {
        return ret;
    }

    ret = sc235hai_get_vts_from_setting(nPipeId, setting, reg_cnt, &vts);
    if (ret) {
        return ret;
    }

    sns_sc235haiparams[nPipeId].vts = vts;

    SNS_DBG("pipe:%d, get setting params vts:0x%x\n", nPipeId, sns_sc235haiparams[nPipeId].vts);

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_cfg_aec_param(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sc235hai_get_params_from_setting(nPipeId);
    sns_obj->sns_mode_obj.nVts = sns_sc235haiparams[nPipeId].vts;
    sns_sc235haiparams[nPipeId].setting_vts = sns_sc235haiparams[nPipeId].vts;
    sns_sc235haiparams[nPipeId].setting_fps = sns_obj->sns_mode_obj.fFrameRate;

    sns_obj->ae_ctrl_param.fTimePerLine =
        (float)1000000 / sns_sc235haiparams[nPipeId].setting_fps /
        AXSNS_DIV_0_TO_1(sns_sc235haiparams[nPipeId].setting_vts);

    /* sensor again  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX] = SC235HAI_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 256;

    /* sensor dgain  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX] = SC235HAI_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 128;

    /* sensor medium again  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX] = SC235HAI_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 256;

    /* sensor medium dgain  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX] = SC235HAI_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 128;

    sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio = SC235HAI_MIN_RATIO;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio = SC235HAI_MAX_RATIO;
    sc235hai_set_exp_limit(nPipeId, SC235HAI_MAX_RATIO);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurRatio = SC235HAI_MAX_RATIO;
    sns_obj->ae_ctrl_param.eSnsHcgLcgMode = AX_LCG_NOTSUPPORT_MODE;

    if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_LINEAR_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = sc235hai_get_exp_offset(nPipeId);
    } else if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_HDR_2X_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = sc235hai_get_exp_offset(nPipeId);
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_MEDIUM_FRAME_IDX] = sc235hai_get_exp_offset(nPipeId);
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = sc235hai_get_exp_offset(nPipeId);
    }

    SNS_DBG("pipe:%d, line_period=%f, vts = 0x%x hts = 0x%x\n",
            nPipeId, sns_obj->ae_ctrl_param.fTimePerLine, sns_sc235haiparams[nPipeId].vts,
            sns_sc235haiparams[nPipeId].hts);

    return AX_SNS_SUCCESS;
}


AX_S32 sc235hai_get_sensor_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_S32 result = 0;
    SNS_CHECK_PTR_VALID(params);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    result = sc235hai_get_gain_table(params);
    return result;

    return AX_SNS_SUCCESS;
}


AX_S32 sc235hai_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain)
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

    result =  sc235hai_again2value(nGainFromUser, &Gain_in, &Gain_de, &gain_value);
    if (result) {
        SNS_ERR("new gain match failed \n");
        return result;
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX] = gain_value;
        result = sc235hai_sns_update_regs_table(nPipeId, SC235HAI_LONG_AGAIN_H, (Gain_in & 0xFF));
        result |= sc235hai_sns_update_regs_table(nPipeId, SC235HAI_LONG_AGAIN_L, (Gain_de & 0xFF));
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    /* short gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        nGainFromUser = ptAGain->fGain[HDR_MEDIUM_FRAME_IDX];
        nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX]);

        result =  sc235hai_again2value(nGainFromUser, &Gain_in, &Gain_de, &gain_value);
        if (result) {
            SNS_ERR("new gain match failed \n");
            return result;
        } else {
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_MEDIUM_FRAME_IDX] = gain_value;
            result = sc235hai_sns_update_regs_table(nPipeId, SC235HAI_SHORT_AGAIN_H, (Gain_in & 0xFF));
            result |= sc235hai_sns_update_regs_table(nPipeId, SC235HAI_SHORT_AGAIN_L, (Gain_de & 0xFF));
            if (result != 0) {
                SNS_ERR("write hw failed %d \n", result);
                return result;
            }
        }
    }

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain)
{
    AX_U8 Gain_in;
    AX_U8 Gain_de;
    AX_U8 Gain_de2;
    AX_S32 result = 0;
    AX_F32 gain_value = 0;
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

    result = sc235hai_dgain2value(nGainFromUser, &Gain_in, &Gain_de, &Gain_de2, &gain_value);
    if (result) {
        SNS_ERR("new gain match failed \n");
        return result;
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX] = gain_value;
        result = sc235hai_sns_update_regs_table(nPipeId, SC235HAI_LONG_DGAIN_H, Gain_in);
        result = sc235hai_sns_update_regs_table(nPipeId, SC235HAI_LONG_DGAIN_L, Gain_de);
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    /* short frame digital gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        nGainFromUser = ptDGain->fGain[HDR_MEDIUM_FRAME_IDX];
        nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX]);

        result = sc235hai_dgain2value(nGainFromUser, &Gain_in, &Gain_de, &Gain_de2, &gain_value);
        if (result) {
            SNS_ERR("new gain match failed \n");
            return result;
        } else {
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_MEDIUM_FRAME_IDX] = gain_value;
            result = sc235hai_sns_update_regs_table(nPipeId, SC235HAI_SHORT_DGAIN_H, Gain_in);
            result = sc235hai_sns_update_regs_table(nPipeId, SC235HAI_SHORT_DGAIN_L, Gain_de);
            if (result != 0) {
                SNS_ERR("write hw failed %d \n", result);
                return result;
            }
        }
    }

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_hcglcg_ctrl(ISP_PIPE_ID nPipeId, AX_U32 nSnsHcgLcg)
{
    //not supoort
    return AX_LCG_NOTSUPPORT_MODE;
}

/* Calculate the max int time according to the exposure ratio */
AX_S32 sc235hai_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio,
        AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_F32 ratio = 0.0f;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTimeRange);

    if (AX_SNS_LINEAR_MODE == sns_obj->sns_mode_obj.eHDRMode) {
        ptIntTimeRange->fMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->fMinIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX];
    } else if (AX_SNS_HDR_2X_MODE == sns_obj->sns_mode_obj.eHDRMode) {
        ratio = AXSNS_CLIP3(fHdrRatio, sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio, sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio);
        sns_obj->ae_ctrl_param.sns_ae_param.fCurRatio = ratio;
        if (fabs(ratio) <= EPS) {
            SNS_ERR("hdr ratio is error \n");
        }

        sc235hai_set_exp_limit(nPipeId, sns_obj->ae_ctrl_param.sns_ae_param.fCurRatio);

        ptIntTimeRange->fMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->fMinIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX];
        ptIntTimeRange->fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX];
    } else {
        // do nothing
    }

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime)
{
    AX_U8 ex_h;
    AX_U8 ex_l;
    AX_U32 ex_ival = 0;
    //AX_S32 result = 0;
   // AX_S32 hdr_ratio = 0;
   // AX_U32 vts;
    AX_F32 fExpLineFromUser = 0;
    AX_U32 LineGap = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTime);

    SNS_DBG("Exptime:%f-%f-%f-%f, Hdrratio:%f-%f-%f-%f\n",
            ptIntTime->fIntTime[0], ptIntTime->fIntTime[1], ptIntTime->fIntTime[2], ptIntTime->fIntTime[3],
            ptIntTime->fHdrRatio[0], ptIntTime->fHdrRatio[1], ptIntTime->fHdrRatio[2], ptIntTime->fHdrRatio[3]);

    fExpLineFromUser = ptIntTime->fIntTime[HDR_LONG_FRAME_IDX];
    fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                       sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
                                       sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX]);

    ex_ival = fExpLineFromUser * 2;
    ex_l = REG_LOW_8BITS(ex_ival);
    ex_h = REG_HIGH_8BITS(ex_ival);

    sc235hai_sns_update_regs_table(nPipeId, SC235HAI_LONG_EXP_LINE_H, (ex_h & 0xf0) >> 4);
    sc235hai_sns_update_regs_table(nPipeId, SC235HAI_LONG_EXP_LINE_M, ((ex_h & 0xf) << 4) | ((ex_l & 0xf0) >> 4));
    sc235hai_sns_update_regs_table(nPipeId, SC235HAI_LONG_EXP_LINE_L, ((ex_l & 0xf) << 4));

    sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX] = ex_ival / 2.0f;

    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        fExpLineFromUser = ptIntTime->fIntTime[HDR_MEDIUM_FRAME_IDX];
        fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX],
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX]);

        ex_ival = fExpLineFromUser * 2;

        //LineGap < Height
        LineGap = sc235hai_get_vs_vts(nPipeId) / 2;
        if(LineGap > sns_obj->sns_mode_obj.nHeight) {
            ex_ival = sns_obj->sns_mode_obj.nHeight - 1;
        }

        ex_l = REG_LOW_8BITS(ex_ival);
        ex_h = REG_HIGH_8BITS(ex_ival);

        sc235hai_sns_update_regs_table(nPipeId, SC235HAI_SHORT_EXP_LINE_H, (ex_h & 0xf0) >> 4);
        sc235hai_sns_update_regs_table(nPipeId, SC235HAI_SHORT_EXP_LINE_M, ((ex_h & 0xf) << 4) | ((ex_l & 0xf0) >> 4));
        sc235hai_sns_update_regs_table(nPipeId, SC235HAI_SHORT_EXP_LINE_L, ((ex_l & 0xf) << 4));

        sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_MEDIUM_FRAME_IDX] = ex_ival / 2.0f;
    }

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(ptAeCtrlParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    memcpy(ptAeCtrlParam, &sns_obj->ae_ctrl_param, sizeof(AX_SNS_EXP_CTRL_PARAM_T));
    memcpy(&ptAeCtrlParam->sns_dev_attr, &sns_obj->sns_attr_param, sizeof(AX_SNS_ATTR_T));

    return AX_SNS_SUCCESS;
}


AX_U32 sc235hai_sns_update_regs_table(ISP_PIPE_ID nPipeId, AX_U32 nRegsAddr, AX_U8 nRegsValue)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    nNum = sizeof(gsc235haiAeRegsTable) / sizeof(gsc235haiAeRegsTable[0]);
    for (i = 0; i < nNum; i++) {
        if (nRegsAddr == sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].nData = nRegsValue;
            gsc235haiAeRegsTable[i].nRegValue = nRegsValue;
            break;
        }
    }

    if (nNum <= i) {
        SNS_ERR(" reg addr 0x%x not find.\n", nRegsAddr);
        return AX_SNS_ERR_BAD_ADDR;
    }

    return AX_SNS_SUCCESS;
}

AX_U32 sc235hai_sns_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    AX_U32  nRegValue;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);

    nNum = sizeof(gsc235haiAeRegsTable) / sizeof(gsc235haiAeRegsTable[0]);

    for (i = 0; i < nNum; i++) {
        nRegValue = sc235hai_reg_read(nPipeId, gsc235haiAeRegsTable[i].nRegAddr);
        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr = gsc235haiAeRegsTable[i].nRegAddr;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nData = nRegValue;

        SNS_DBG(" nRegAddr 0x%x, nRegValue 0x%x\n", gsc235haiAeRegsTable[i].nRegAddr, gsc235haiAeRegsTable[i].nRegValue);
    }

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_sns_update_init_exposure_reg(ISP_PIPE_ID nPipeId)
{
    int i = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        sc235hai_write_register(nPipeId, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
        SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
            i, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
    }

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo)
{
    AX_S32 i = 0;
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
        sns_obj->sztRegsInfo[0].eSnsType = ISP_SNS_CONNECT_I2C_TYPE;
        sns_obj->sztRegsInfo[0].tComBus.I2cDev = sc235hai_get_bus_num(nPipeId);
        sns_obj->sztRegsInfo[0].nRegNum = sizeof(gsc235haiAeRegsTable) / sizeof(gsc235haiAeRegsTable[0]);
        sns_obj->sztRegsInfo[0].nCfg2ValidDelayMax = 2;

        for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDevAddr = gSc235haiSlaveAddr[nPipeId];
            sns_obj->sztRegsInfo[0].sztI2cData[i].nAddrByteNum = SC235HAI_ADDR_BYTE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDataByteNum = SC235HAI_DATA_BYTE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDelayFrmNum = gsc235haiAeRegsTable[i].nDelayFrmNum;
            SNS_DBG("pipe %d, [%2d] nRegAddr 0x%x, nRegValue 0x%x\n", nPipeId, i,
                    gsc235haiAeRegsTable[i].nRegAddr, gsc235haiAeRegsTable[i].nRegValue);
        }

        bUpdateReg = AX_TRUE;
        sns_obj->bSyncInit = AX_TRUE;
        sc235hai_sns_update_init_exposure_reg(nPipeId);
    } else {
        for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
            if (sns_obj->sztRegsInfo[0].sztI2cData[i].nData == sns_obj->sztRegsInfo[1].sztI2cData[i].nData) {
                sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_FALSE;
            } else {
                sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
                bUpdateReg = AX_TRUE;
                SNS_DBG("pipe %d, [%2d] nRegAddr 0x%x, nRegValue 0x%x\n", nPipeId, i,
                        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
            }
        }
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

AX_S32 sc235hai_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam)
{
    AX_S32 framerate = SNS_MAX_FRAME_RATE;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_U32 nVts_s = 0;
    AX_U32 nVts = 0;
    AX_U32 nfps = 0;

    SNS_CHECK_PTR_VALID(ptSlowShutterParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate = sns_obj->sns_mode_obj.fFrameRate;
    if (framerate > SNS_MAX_FRAME_RATE) {
        SNS_ERR(" framerate out of range %d \n", framerate);
        return AX_SNS_ERR_ILLEGAL_PARAM;
    }

    if (sns_obj->ae_ctrl_param.fTimePerLine == 0) {
        SNS_ERR("line_period is zero : %f\n", sns_obj->ae_ctrl_param.fTimePerLine);
        return AX_SNS_ERR_ILLEGAL_PARAM;
    }

    ptSlowShutterParam->nGroupNum = AXSNS_MIN((sizeof(gFpsGear) / sizeof(AX_F32)), framerate);
    ptSlowShutterParam->fMinFps = AXSNS_MAX(gFpsGear[0],
                                            (1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * SC235HAI_MAX_VTS)));

    for (nfps = 0 ; nfps < ptSlowShutterParam->nGroupNum; nfps++) {
        nVts = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * gFpsGear[nfps]);
        if ((AX_S32)gFpsGear[nfps] >= framerate) {
            nVts = sns_obj->sns_mode_obj.nVts;
        }
        if (nVts > SC235HAI_MAX_VTS) {
            nVts = SC235HAI_MAX_VTS;
            SNS_WRN("Beyond minmum fps  %f\n", ptSlowShutterParam->fMinFps);
        }

        if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime = (2 * nVts - 11) / 2;
        } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            nVts_s = (2 * nVts + 19 * SC235HAI_MAX_RATIO - 21) / (2 * SC235HAI_MAX_RATIO + 2);
            ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime = (2 * nVts - 2 * nVts_s - 21) / 2;
        }

        ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * nVts);
        ptSlowShutterParam->fMaxFps = ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps;

        SNS_DBG("nPipeId = %d, line_period = %.2f, fps = %.2f, fMaxIntTime = %.2f(%.2f), vts=0x%x\n",
                nPipeId, sns_obj->ae_ctrl_param.fTimePerLine,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime * sns_obj->ae_ctrl_param.fTimePerLine,
                nVts);
    }

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_get_fps(ISP_PIPE_ID nPipeId, AX_F32 *pFps)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(pFps);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    *pFps = sns_obj->ae_ctrl_param.sns_ae_param.fCurFps;

    return AX_SNS_SUCCESS;
}


AX_S32 sc235hai_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 result = 0;
    AX_U32 vts = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if(AXSNS_CAMPARE_FLOAT(AX_SNS_FPS_MIN, fFps) || AXSNS_CAMPARE_FLOAT(fFps, AX_SNS_FPS_MAX)) {
        SNS_ERR("pipe:%d, fps:%f is not supported, range:[%f-%f]\n", nPipeId, fFps, AX_SNS_FPS_MIN, AX_SNS_FPS_MAX);
        return AX_SNS_ERR_NOT_SUPPORT;
    }

    if (IS_SNS_FPS_EQUAL(fFps, sns_sc235haiparams[nPipeId].setting_fps)) {
        vts = sns_sc235haiparams[nPipeId].setting_vts;
    } else {
        vts = round(1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * fFps));
        if (vts > SC235HAI_MAX_VTS){
            vts = SC235HAI_MAX_VTS;
            SNS_WRN("Beyond max vts:0x%x\n", vts);
        }
    }

    result = sc235hai_set_vts(nPipeId, vts);
    if (result != 0) {
        SNS_ERR("%s: write vts failed %d \n", __func__, result);
        return result;
    }
    sns_sc235haiparams[nPipeId].vts = vts;

    if (IS_SNS_FPS_EQUAL(fFps, sns_obj->sns_attr_param.fFrameRate)) {
        sns_obj->sns_mode_obj.nVts = vts;
        sns_obj->sns_mode_obj.fFrameRate = sns_obj->sns_attr_param.fFrameRate;
    }

    sc235hai_set_exp_limit(nPipeId, SC235HAI_MAX_RATIO);
    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * vts);

    SNS_DBG("pipe:%d, userFps:%f, curFps:%f, vts:0x%x\n",
        nPipeId, fFps, sns_obj->ae_ctrl_param.sns_ae_param.fCurFps, vts);

    return AX_SNS_SUCCESS;
}
