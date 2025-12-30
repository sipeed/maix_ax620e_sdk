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

#include "sc850sl_reg.h"
#include "sc850sl_ae_ctrl.h"

extern SNS_STATE_OBJ *g_szsc850slCtx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szsc850slCtx[dev])

static SNSSC850SL_OBJ_T sns_sc850slparams[AX_VIN_MAX_PIPE_NUM];
static AX_F32 nAgainTable[SENSOR_MAX_GAIN_STEP];
static AX_F32 nDgainTable[SENSOR_MAX_GAIN_STEP];
extern AX_U8 gSc850slSlaveAddr[AX_VIN_MAX_PIPE_NUM];

static AX_S32 sc850sl_set_exp_limit(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio, AX_U32 nVts);

/*user config*/
static AX_F32 gFpsGear[] = {1.00, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00,
                            11.00, 12.00, 13.00, 14.00, 15.00, 16.00, 17.00, 18.00, 19.00, 20.00,
                            21.00, 22.00, 23.00, 24.00, 25.00, 26.00, 27.00, 28.00, 29.00, 30.00, 60.00
                           };


static AX_SNS_DRV_DELAY_TABLE_T gsc850slAeRegsTable[SC850SL_AE_REG_IDX_MAX] = {
    /* regs index */                    /* regs addr */           /*regs value*/  /*Delay Frame Num*/
    {SC850SL_LONG_EXP_LINE_H_IDX,       SC850SL_LONG_EXP_LINE_H,        0,          0},
    {SC850SL_LONG_EXP_LINE_M_IDX,       SC850SL_LONG_EXP_LINE_M,        0,          0},
    {SC850SL_LONG_EXP_LINE_L_IDX,       SC850SL_LONG_EXP_LINE_L,        0,          0},
    {SC850SL_SHORT_EXP_LINE_H_IDX,      SC850SL_SHORT_EXP_LINE_H,       0,          0},
    {SC850SL_SHORT_EXP_LINE_M_IDX,      SC850SL_SHORT_EXP_LINE_M,       0,          0},
    {SC850SL_SHORT_EXP_LINE_L_IDX,      SC850SL_SHORT_EXP_LINE_L,       0,          0},
    {SC850SL_LONG_AGAIN_H_IDX,          SC850SL_LONG_AGAIN_H,           0,          0},
    {SC850SL_LONG_AGAIN_L_IDX,          SC850SL_LONG_AGAIN_L,           0,          0},
    {SC850SL_SHORT_AGAIN_H_IDX,         SC850SL_SHORT_AGAIN_H,          0,          0},
    {SC850SL_SHORT_AGAIN_L_IDX,         SC850SL_SHORT_AGAIN_L,          0,          0},
    {SC850SL_AGAIN_ADJUST_IDX,          SC850SL_AGAIN_ADJUST,          0,          0},
    {SC850SL_LONG_DGAIN_IDX,            SC850SL_LONG_DGAIN,            0,          0},
    {SC850SL_SHORT_DGAIN_IDX,           SC850SL_SHORT_DGAIN,           0,          0},
    {SC850SL_VTS_L_H_IDX,               SC850SL_VTS_L_H,               0,          0},
    {SC850SL_VTS_L_L_IDX,               SC850SL_VTS_L_L,               0,          0},
    {SC850SL_VTS_S_H_IDX,               SC850SL_VTS_S_H,               0,          0},
    {SC850SL_VTS_S_L_IDX,               SC850SL_VTS_S_L,               0,          0},
    {SC850SL_RB_ROWS_H_IDX,             SC850SL_RB_ROWS_H,             0,          0},
    {SC850SL_RB_ROWS_L_IDX,             SC850SL_RB_ROWS_L,             0,          0},
};


const SC850SL_GAIN_TABLE_T sc850sl_again_table[] = {
    {1.000,  0x03, 0x40},
    // {1.016,  0x03, 0x41},
    {1.031,  0x03, 0x42},
    // {1.047,  0x03, 0x43},
    {1.063,  0x03, 0x44},
    // {1.078,  0x03, 0x45},
    {1.094,  0x03, 0x46},
    // {1.109,  0x03, 0x47},
    {1.125,  0x03, 0x48},
    // {1.141,  0x03, 0x49},
    {1.156,  0x03, 0x4A},
    // {1.172,  0x03, 0x4B},
    {1.188,  0x03, 0x4C},
    // {1.203,  0x03, 0x4D},
    {1.219,  0x03, 0x4E},
    // {1.234,  0x03, 0x4F},
    {1.250,  0x03, 0x50},
    // {1.266,  0x03, 0x51},
    {1.281,  0x03, 0x52},
    // {1.297,  0x03, 0x53},
    {1.313,  0x03, 0x54},
    // {1.328,  0x03, 0x55},
    {1.344,  0x03, 0x56},
    // {1.359,  0x03, 0x57},
    {1.375,  0x03, 0x58},
    // {1.391,  0x03, 0x59},
    {1.406,  0x03, 0x5A},
    // {1.422,  0x03, 0x5B},
    {1.438,  0x03, 0x5C},
    // {1.453,  0x03, 0x5D},
    {1.469,  0x03, 0x5E},
    // {1.484,  0x03, 0x5F},
    {1.500,  0x03, 0x60},
    // {1.516,  0x03, 0x61},
    {1.531,  0x03, 0x62},
    // {1.547,  0x03, 0x63},
    {1.563,  0x03, 0x64},
    // {1.578,  0x03, 0x65},
    {1.594,  0x03, 0x66},
    // {1.609,  0x03, 0x67},
    {1.625,  0x03, 0x68},
    // {1.641,  0x03, 0x69},
    {1.656,  0x03, 0x6A},
    // {1.672,  0x03, 0x6B},
    {1.688,  0x03, 0x6C},
    // {1.703,  0x03, 0x6D},
    {1.719,  0x03, 0x6E},
    // {1.734,  0x03, 0x6F},
    {1.750,  0x03, 0x70},
    // {1.766,  0x03, 0x71},
    {1.781,  0x03, 0x72},
    // {1.797,  0x03, 0x73},
    {1.813,  0x03, 0x74},
    // {1.828,  0x03, 0x75},
    {1.844,  0x03, 0x76},
    // {1.859,  0x03, 0x77},
    {1.875,  0x03, 0x78},
    // {1.891,  0x03, 0x79},
    {1.906,  0x03, 0x7A},
    // {1.922,  0x03, 0x7B},
    {1.938,  0x03, 0x7C},
    // {1.953,  0x03, 0x7D},
    {1.969,  0x03, 0x7E},
    // {1.984,  0x03, 0x7F},
    {2.000,  0x07, 0x40},
    // {2.031,  0x07, 0x41},
    {2.063,  0x07, 0x42},
    // {2.094,  0x07, 0x43},
    {2.125,  0x07, 0x44},
    // {2.156,  0x07, 0x45},
    {2.188,  0x07, 0x46},
    // {2.219,  0x07, 0x47},
    {2.250,  0x07, 0x48},
    // {2.281,  0x07, 0x49},
    {2.313,  0x07, 0x4A},
    // {2.344,  0x07, 0x4B},
    {2.375,  0x07, 0x4C},
    // {2.406,  0x07, 0x4D},
    {2.438,  0x07, 0x4E},
    // {2.469,  0x07, 0x4F},
    {2.500,  0x07, 0x50},
    // {2.531,  0x07, 0x51},
    {2.563,  0x07, 0x52},
    // {2.594,  0x07, 0x53},
    {2.625,  0x07, 0x54},
    // {2.656,  0x07, 0x55},
    {2.688,  0x07, 0x56},
    // {2.719,  0x07, 0x57},
    {2.750,  0x07, 0x58},
    // {2.781,  0x07, 0x59},
    {2.813,  0x07, 0x5A},
    // {2.844,  0x07, 0x5B},
    {2.875,  0x07, 0x5C},
    // {2.906,  0x07, 0x5D},
    {2.938,  0x07, 0x5E},
    // {2.969,  0x07, 0x5F},
    {3.000,  0x07, 0x60},
    // {3.031,  0x07, 0x61},
    {3.063,  0x07, 0x62},
    // {3.094,  0x07, 0x63},
    // {3.125,  0x23, 0x40},//bad
    // {3.174,  0x23, 0x41},//bad
    {3.223,  0x23, 0x42},
    // {3.271,  0x23, 0x43},
    {3.320,  0x23, 0x44},
    // {3.369,  0x23, 0x45},
    {3.418,  0x23, 0x46},
    // {3.467,  0x23, 0x47},
    {3.516,  0x23, 0x48},
    // {3.564,  0x23, 0x49},
    {3.613,  0x23, 0x4A},
    // {3.662,  0x23, 0x4B},
    {3.711,  0x23, 0x4C},
    // {3.760,  0x23, 0x4D},
    {3.809,  0x23, 0x4E},
    // {3.857,  0x23, 0x4F},
    {3.906,  0x23, 0x50},
    // {3.955,  0x23, 0x51},
    {4.004,  0x23, 0x52},
    // {4.053,  0x23, 0x53},
    {4.102,  0x23, 0x54},
    // {4.150,  0x23, 0x55},
    {4.199,  0x23, 0x56},
    // {4.248,  0x23, 0x57},
    {4.297,  0x23, 0x58},
    // {4.346,  0x23, 0x59},
    {4.395,  0x23, 0x5A},
    // {4.443,  0x23, 0x5B},
    {4.492,  0x23, 0x5C},
    // {4.541,  0x23, 0x5D},
    {4.590,  0x23, 0x5E},
    // {4.639,  0x23, 0x5F},
    {4.688,  0x23, 0x60},
    // {4.736,  0x23, 0x61},
    {4.785,  0x23, 0x62},
    // {4.834,  0x23, 0x63},
    {4.883,  0x23, 0x64},
    // {4.932,  0x23, 0x65},
    {4.980,  0x23, 0x66},
    // {5.029,  0x23, 0x67},
    {5.078,  0x23, 0x68},
    // {5.127,  0x23, 0x69},
    {5.176,  0x23, 0x6A},
    // {5.225,  0x23, 0x6B},
    {5.273,  0x23, 0x6C},
    // {5.322,  0x23, 0x6D},
    {5.371,  0x23, 0x6E},
    // {5.420,  0x23, 0x6F},
    {5.469,  0x23, 0x70},
    // {5.518,  0x23, 0x71},
    {5.566,  0x23, 0x72},
    // {5.615,  0x23, 0x73},
    {5.664,  0x23, 0x74},
    // {5.713,  0x23, 0x75},
    {5.762,  0x23, 0x76},
    // {5.811,  0x23, 0x77},
    {5.859,  0x23, 0x78},
    // {5.908,  0x23, 0x79},
    {5.957,  0x23, 0x7A},
    // {6.006,  0x23, 0x7B},
    {6.055,  0x23, 0x7C},
    // {6.104,  0x23, 0x7D},
    {6.152,  0x23, 0x7E},
    // {6.201,  0x23, 0x7F},
    {6.250,  0x27, 0x40},
    // {6.348,  0x27, 0x41},
    {6.445,  0x27, 0x42},
    // {6.543,  0x27, 0x43},
    {6.641,  0x27, 0x44},
    // {6.738,  0x27, 0x45},
    {6.836,  0x27, 0x46},
    // {6.934,  0x27, 0x47},
    {7.031,  0x27, 0x48},
    // {7.129,  0x27, 0x49},
    {7.227,  0x27, 0x4A},
    // {7.324,  0x27, 0x4B},
    {7.422,  0x27, 0x4C},
    // {7.520,  0x27, 0x4D},
    {7.617,  0x27, 0x4E},
    // {7.715,  0x27, 0x4F},
    {7.813,  0x27, 0x50},
    // {7.910,  0x27, 0x51},
    {8.008,  0x27, 0x52},
    // {8.105,  0x27, 0x53},
    {8.203,  0x27, 0x54},
    // {8.301,  0x27, 0x55},
    {8.398,  0x27, 0x56},
    // {8.496,  0x27, 0x57},
    {8.594,  0x27, 0x58},
    // {8.691,  0x27, 0x59},
    {8.789,  0x27, 0x5A},
    // {8.887,  0x27, 0x5B},
    {8.984,  0x27, 0x5C},
    // {9.082,  0x27, 0x5D},
    {9.180,  0x27, 0x5E},
    // {9.277,  0x27, 0x5F},
    {9.375,  0x27, 0x60},
    // {9.473,  0x27, 0x61},
    {9.570,  0x27, 0x62},
    // {9.668,  0x27, 0x63},
    {9.766,  0x27, 0x64},
    // {9.863,  0x27, 0x65},
    {9.961,  0x27, 0x66},
    // {10.059, 0x27, 0x67},
    {10.156, 0x27, 0x68},
    // {10.254, 0x27, 0x69},
    {10.352, 0x27, 0x6A},
    // {10.449, 0x27, 0x6B},
    {10.547, 0x27, 0x6C},
    // {10.645, 0x27, 0x6D},
    {10.742, 0x27, 0x6E},
    // {10.840, 0x27, 0x6F},
    {10.938, 0x27, 0x70},
    // {11.035, 0x27, 0x71},
    {11.133, 0x27, 0x72},
    // {11.230, 0x27, 0x73},
    {11.328, 0x27, 0x74},
    // {11.426, 0x27, 0x75},
    {11.523, 0x27, 0x76},
    // {11.621, 0x27, 0x77},
    {11.719, 0x27, 0x78},
    // {11.816, 0x27, 0x79},
    {11.914, 0x27, 0x7A},
    // {12.012, 0x27, 0x7B},
    {12.109, 0x27, 0x7C},
    // {12.207, 0x27, 0x7D},
    {12.305, 0x27, 0x7E},
    // {12.402, 0x27, 0x7F},
    {12.500, 0x2F, 0x40},
    // {12.695, 0x2F, 0x41},
    {12.891, 0x2F, 0x42},
    // {13.086, 0x2F, 0x43},
    {13.281, 0x2F, 0x44},
    // {13.477, 0x2F, 0x45},
    {13.672, 0x2F, 0x46},
    // {13.867, 0x2F, 0x47},
    {14.063, 0x2F, 0x48},
    // {14.258, 0x2F, 0x49},
    {14.453, 0x2F, 0x4A},
    // {14.648, 0x2F, 0x4B},
    {14.844, 0x2F, 0x4C},
    // {15.039, 0x2F, 0x4D},
    {15.234, 0x2F, 0x4E},
    // {15.430, 0x2F, 0x4F},
    {15.625, 0x2F, 0x50},
    // {15.820, 0x2F, 0x51},
    {16.016, 0x2F, 0x52},
    // {16.211, 0x2F, 0x53},
    {16.406, 0x2F, 0x54},
    // {16.602, 0x2F, 0x55},
    {16.797, 0x2F, 0x56},
    // {16.992, 0x2F, 0x57},
    {17.188, 0x2F, 0x58},
    // {17.383, 0x2F, 0x59},
    {17.578, 0x2F, 0x5A},
    // {17.773, 0x2F, 0x5B},
    {17.969, 0x2F, 0x5C},
    // {18.164, 0x2F, 0x5D},
    // {18.555, 0x2F, 0x5F},//spec redundant bad
    {18.359, 0x2F, 0x5E},
    // {18.555, 0x2F, 0x5F},
    {18.750, 0x2F, 0x60},
    // {18.945, 0x2F, 0x61},
    {19.141, 0x2F, 0x62},
    // {19.336, 0x2F, 0x63},
    {19.531, 0x2F, 0x64},
    // {19.727, 0x2F, 0x65},
    {19.922, 0x2F, 0x66},
    // {20.117, 0x2F, 0x67},
    {20.313, 0x2F, 0x68},
    // {20.508, 0x2F, 0x69},
    {20.703, 0x2F, 0x6A},
    // {20.898, 0x2F, 0x6B},
    {21.094, 0x2F, 0x6C},
    // {21.289, 0x2F, 0x6D},
    {21.484, 0x2F, 0x6E},
    // {21.680, 0x2F, 0x6F},
    {21.875, 0x2F, 0x70},
    // {22.070, 0x2F, 0x71},
    {22.266, 0x2F, 0x72},
    // {22.461, 0x2F, 0x73},
    {22.656, 0x2F, 0x74},
    // {22.852, 0x2F, 0x75},
    {23.047, 0x2F, 0x76},
    // {23.242, 0x2F, 0x77},
    {23.438, 0x2F, 0x78},
    // {23.633, 0x2F, 0x79},
    {23.828, 0x2F, 0x7A},
    // {24.023, 0x2F, 0x7B},
    {24.219, 0x2F, 0x7C},
    // {24.414, 0x2F, 0x7D},
    {24.609, 0x2F, 0x7E},
    // {24.805, 0x2F, 0x7F},
    {25.000, 0x3F, 0x40},
    // {25.391, 0x3F, 0x41},
    {25.781, 0x3F, 0x42},
    // {26.172, 0x3F, 0x43},
    {26.563, 0x3F, 0x44},
    // {26.953, 0x3F, 0x45},
    {27.344, 0x3F, 0x46},
    // {27.734, 0x3F, 0x47},
    {28.125, 0x3F, 0x48},
    // {28.516, 0x3F, 0x49},
    {28.906, 0x3F, 0x4A},
    // {29.297, 0x3F, 0x4B},
    {29.688, 0x3F, 0x4C},
    // {30.078, 0x3F, 0x4D},
    {30.469, 0x3F, 0x4E},
    // {30.859, 0x3F, 0x4F},
    {31.250, 0x3F, 0x50},
    // {31.641, 0x3F, 0x51},
    {32.031, 0x3F, 0x52},
    // {32.422, 0x3F, 0x53},
    {32.813, 0x3F, 0x54},
    // {33.203, 0x3F, 0x55},
    {33.594, 0x3F, 0x56},
    // {33.984, 0x3F, 0x57},
    {34.375, 0x3F, 0x58},
    // {34.766, 0x3F, 0x59},
    {35.156, 0x3F, 0x5A},
    // {35.547, 0x3F, 0x5B},
    {35.938, 0x3F, 0x5C},
    // {36.328, 0x3F, 0x5D},
    {36.719, 0x3F, 0x5E},
    // {37.109, 0x3F, 0x5F},
    {37.500, 0x3F, 0x60},
    // {37.891, 0x3F, 0x61},
    {38.281, 0x3F, 0x62},
    // {38.672, 0x3F, 0x63},
    {39.063, 0x3F, 0x64},
    // {39.453, 0x3F, 0x65},
    {39.844, 0x3F, 0x66},
    // {40.234, 0x3F, 0x67},
    {40.625, 0x3F, 0x68},
    // {41.016, 0x3F, 0x69},
    {41.406, 0x3F, 0x6A},
    // {41.797, 0x3F, 0x6B},
    {42.188, 0x3F, 0x6C},
    // {42.578, 0x3F, 0x6D},
    {42.969, 0x3F, 0x6E},
    // {43.359, 0x3F, 0x6F},
    {43.750, 0x3F, 0x70},
    // {44.141, 0x3F, 0x71},
    {44.531, 0x3F, 0x72},
    // {44.922, 0x3F, 0x73},
    {45.313, 0x3F, 0x74},
    // {45.703, 0x3F, 0x75},
    {46.094, 0x3F, 0x76},
    // {46.484, 0x3F, 0x77},
    {46.875, 0x3F, 0x78},
    // {47.266, 0x3F, 0x79},
    {47.656, 0x3F, 0x7A},
    // {48.047, 0x3F, 0x7B},
    {48.438, 0x3F, 0x7C},
    // {48.828, 0x3F, 0x7D},
    {49.219, 0x3F, 0x7E},
    {49.609, 0x3F, 0x7F},
};

const SC850SL_GAIN_TABLE_T sc850sl_dgain_table[] = {
    {1.000, 0x00},
    {2.000, 0x01},
    {4.000, 0x03},
    {8.000, 0x07},
};

static AX_F32 sc850sl_again2value(float gain, AX_U8 *again_in, AX_U8 *again_de)
{
    AX_U32 i;
    AX_U32 count;

    if (!again_in || !again_de)
        return -1;

    count = sizeof(sc850sl_again_table) / sizeof(sc850sl_again_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > sc850sl_again_table[i].gain) {
            continue;
        } else {
            if (again_in)
                *again_in = sc850sl_again_table[i].gain_in;
            if (again_de)
                *again_de = sc850sl_again_table[i].gain_de;
            SNS_DBG("again=%f, again_in=0x%x, again_de=0x%x\n", gain, *again_in, *again_de);
            return sc850sl_again_table[i].gain;
        }
    }

    return -1;
}

AX_F32 sc850sl_dgain2value(float gain, AX_U8 *dgain_in)
{
    AX_U32 i;
    AX_U32 count;

    if (!dgain_in)
        return -1;

    count = sizeof(sc850sl_dgain_table) / sizeof(sc850sl_dgain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > sc850sl_dgain_table[i].gain) {
            continue;
        } else {
            *dgain_in = sc850sl_dgain_table[i].gain_in;
            SNS_DBG("dgain=%f, dgain_in=0x%x\n", gain, *dgain_in);

            return sc850sl_dgain_table[i].gain;
        }
    }
    return -1;
}

static AX_S32 sc850sl_get_gain_table(AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_U32 i;
    AX_S32 ret = 0;
    if (!params)
        return -1;

    params->nAgainTableSize = sizeof(sc850sl_again_table) / sizeof(sc850sl_again_table[0]);
    params->nDgainTableSize = sizeof(sc850sl_dgain_table) / sizeof(sc850sl_dgain_table[0]);

    for (i = 0; i < params->nAgainTableSize ; i++) {
        nAgainTable[i] = sc850sl_again_table[i].gain;
        params->pAgainTable = nAgainTable;
    }

    for (i = 0; i < params->nDgainTableSize ; i++) {
        nDgainTable[i] = sc850sl_dgain_table[i].gain;
        params->pDgainTable = nDgainTable;
    }

    return ret;
}


/****************************************************************************
 * exposure control external function
 ****************************************************************************/

static AX_S32 sc850sl_set_exp_limit(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio, AX_U32 nVts)
{
    AX_U32 vts_s = 0;
    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_LINEAR_MODE) {
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = SC850SL_SDR_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = SC850SL_SDR_EXP_LINE_MAX(sns_sc850slparams[nPipeId].vts);

        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = SC850SL_SDR_INTEGRATION_TIME_STEP;

    } else if (sns_obj->sns_mode_obj.eHDRMode == AX_SNS_HDR_2X_MODE) {
        vts_s = (sns_sc850slparams[nPipeId].vts + 2 * fHdrRatio - 8) / (fHdrRatio + 1);
        SNS_DBG("hdr vts_s:%d, vts:%d, ratio:%f\n", vts_s, sns_sc850slparams[nPipeId].vts ,fHdrRatio);

        sns_sc850slparams[nPipeId].vts_s = vts_s;
        sc850sl_set_vts_s(nPipeId, sns_sc850slparams[nPipeId].vts_s);

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = SC850SL_HDR_2X_LONG_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] = SC850SL_HDR_2X_SHORT_EXP_LINE_MIN;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = SC850SL_HDR_2X_LONG_EXP_LINE_MAX(sns_sc850slparams[nPipeId].vts, vts_s);
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = SC850SL_HDR_2X_SHORT_EXP_LINE_MAX(vts_s);

        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = SC850SL_HDR_2X_LONG_INTEGRATION_TIME_STEP;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_MEDIUM_FRAME_IDX] = SC850SL_HDR_2X_SHORT_INTEGRATION_TIME_STEP;
    }

    SNS_DBG("pipe:%d, ratio:%f, expline_limit:%.2f-%.2f-%.2f-%.2f(%.2f-%.2f-%.2f-%.2f), line_period:%f, vts:0x%x, vts_s:0x%x\n",
        nPipeId, fHdrRatio,
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX],
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX],
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX],
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
        sns_obj->ae_ctrl_param.fTimePerLine, nVts, vts_s);

    return 0;
}

AX_S32 sc850sl_cfg_aec_param(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sns_mode_obj.nVts = AXSNS_DIV_0_TO_1(sc850sl_get_vts(nPipeId));
    sns_sc850slparams[nPipeId].vts = sns_obj->sns_mode_obj.nVts;
    sns_sc850slparams[nPipeId].vts_s = AXSNS_DIV_0_TO_1(sc850sl_get_vts_s(nPipeId));
    sns_sc850slparams[nPipeId].setting_vts = sns_obj->sns_mode_obj.nVts;
    sns_sc850slparams[nPipeId].setting_fps = sns_obj->sns_mode_obj.fFrameRate;

    sns_obj->ae_ctrl_param.fTimePerLine =
        (float)1 * SNS_1_SECOND_UNIT_US / (sns_obj->sns_mode_obj.nVts * sns_obj->sns_mode_obj.fFrameRate);

    /* sensor again  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX] = SC850SL_MIN_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX] = SC850SL_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 64;

    /* sensor dgain  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX] = SC850SL_MIN_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX] = SC850SL_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1;

    /* sensor medium again  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX] = SC850SL_MIN_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX] = SC850SL_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 64;

    /* sensor medium dgain  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX] = SC850SL_MIN_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX] = SC850SL_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1;

    sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio = SC850SL_MIN_RATIO;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio = SC850SL_MAX_RATIO;
    sc850sl_set_exp_limit(nPipeId, SC850SL_MAX_RATIO, sns_sc850slparams[nPipeId].vts);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurRatio = SC850SL_MAX_RATIO;
    sns_obj->ae_ctrl_param.eSnsHcgLcgMode = AX_LCG_NOTSUPPORT_MODE;

    return SNS_SUCCESS;
}


AX_S32 sc850sl_get_sensor_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_S32 result = 0;
    SNS_CHECK_PTR_VALID(params);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    result = sc850sl_get_gain_table(params);

    return result;
}


AX_S32 sc850sl_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain)
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

    gain_value = sc850sl_again2value(nGainFromUser, &Gain_in, &Gain_de);
    if (gain_value == -1) {
        SNS_ERR("new gain match failed \n");
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX] = gain_value;
        result = sc850sl_sns_update_regidx_table(nPipeId, SC850SL_LONG_AGAIN_H_IDX, (Gain_in & 0xFF));
        result |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_LONG_AGAIN_L_IDX, (Gain_de & 0xFF));
        if(IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            if(gain_value < 2) {
                result |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_AGAIN_ADJUST_IDX, 0x05);
            } else {
                result |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_AGAIN_ADJUST_IDX, 0x07);
            }
        }
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

        gain_value = sc850sl_again2value(nGainFromUser, &Gain_in, &Gain_de);
        if (gain_value == -1) {
            SNS_ERR("new gain match failed \n");
        } else {
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_MEDIUM_FRAME_IDX] = gain_value;
            result = sc850sl_sns_update_regidx_table(nPipeId, SC850SL_SHORT_AGAIN_H_IDX, (Gain_in & 0xFF));
            result |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_SHORT_AGAIN_L_IDX, (Gain_de & 0xFF));
            if (result != 0) {
                SNS_ERR("write hw failed %d \n", result);
                return result;
            }
        }
    }

    SNS_DBG("again: long frame %f, short frame %f\n",
            ptAGain->fGain[HDR_LONG_FRAME_IDX],
            ptAGain->fGain[HDR_MEDIUM_FRAME_IDX]);

    return SNS_SUCCESS;
}

AX_S32 sc850sl_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain)
{
    AX_U8 Gain_in;
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

    gain_val = sc850sl_dgain2value(nGainFromUser, &Gain_in);
    if (gain_val == -1) {
        SNS_ERR("new gain match failed \n");
    } else {
        result = sc850sl_sns_update_regidx_table(nPipeId, SC850SL_LONG_DGAIN_IDX, Gain_in);
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
        sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX] = gain_val;
    }


    /* short frame digital gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        nGainFromUser = ptDGain->fGain[HDR_MEDIUM_FRAME_IDX];
        nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX]);

        gain_val = sc850sl_dgain2value(nGainFromUser, &Gain_in);
        if (gain_val == -1) {
            SNS_ERR("new gain match failed \n");
        } else {
            result |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_SHORT_DGAIN_IDX, Gain_in);
            if (result != 0) {
                SNS_ERR("write hw failed %d \n", result);
                return result;
            }
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_MEDIUM_FRAME_IDX] = gain_val;
        }

    }

    SNS_DBG("dgain: long frame %f, short frame %f\n",
            ptDGain->fGain[HDR_LONG_FRAME_IDX],
            ptDGain->fGain[HDR_MEDIUM_FRAME_IDX]);

    return SNS_SUCCESS;
}

AX_S32 sc850sl_hcglcg_ctrl(ISP_PIPE_ID nPipeId, AX_U32 nSnsHcgLcg)
{
    //not supoort
    return AX_LCG_NOTSUPPORT_MODE;
}

/* Calculate the max int time according to the exposure ratio */
AX_S32 sc850sl_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio,
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
        sns_obj->ae_ctrl_param.sns_ae_param.fCurRatio = ratio;
        sc850sl_set_exp_limit(nPipeId, ratio, sns_sc850slparams[nPipeId].vts);

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

AX_U32 sc850sl_set_int_t_l(ISP_PIPE_ID nPipeId, AX_U32 int_t)
{
    AX_U8 int_t_h = 0;
    AX_U8 int_t_m = 0;
    AX_U8 int_t_l = 0;
    AX_S32 ret = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    int_t_h = (int_t & 0xF000) >> 12;
    int_t_m = (int_t & 0xFF0) >> 4;
    int_t_l = (int_t & 0xF) << 4;

    ret |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_LONG_EXP_LINE_H_IDX, int_t_h);
    ret |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_LONG_EXP_LINE_M_IDX, int_t_m);
    ret |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_LONG_EXP_LINE_L_IDX, int_t_l);

    return ret;
}

AX_U32 sc850sl_set_int_t_s(ISP_PIPE_ID nPipeId, AX_U32 int_t)
{
    AX_U8 int_t_h;
    AX_U8 int_t_m;
    AX_U8 int_t_l;
    AX_S32 ret = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    int_t_h = (int_t & 0xF000) >> 12;
    int_t_m = (int_t & 0xFF0) >> 4;
    int_t_l = (int_t & 0xF) << 4;

    ret |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_SHORT_EXP_LINE_H_IDX, int_t_h);
    ret |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_SHORT_EXP_LINE_M_IDX, int_t_m);
    ret |= sc850sl_sns_update_regidx_table(nPipeId, SC850SL_SHORT_EXP_LINE_L_IDX, int_t_l);

    return ret;
}

AX_S32 sc850sl_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime)
{
    AX_U32 ex_ival = 0;
    AX_F32 ratio = 1.0f;
    AX_F32 fExpLineFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTime);

    SNS_DBG("Exptime:%f-%f-%f-%f, Hdrratio:%f-%f-%f-%f\n",
            ptIntTime->fIntTime[0], ptIntTime->fIntTime[1], ptIntTime->fIntTime[2], ptIntTime->fIntTime[3],
            ptIntTime->fHdrRatio[0], ptIntTime->fHdrRatio[1], ptIntTime->fHdrRatio[2], ptIntTime->fHdrRatio[3]);

    if (fabs(ptIntTime->fHdrRatio[HDR_LONG_FRAME_IDX]) < EPS) {
        SNS_ERR("hdr ratio is error\n");
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    ratio = ptIntTime->fHdrRatio[HDR_LONG_FRAME_IDX];
    ratio = AXSNS_CLIP3(ratio, sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio, sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio);

    fExpLineFromUser = ptIntTime->fIntTime[HDR_LONG_FRAME_IDX];

    fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                       sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
                                       sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX]);

    ex_ival = fExpLineFromUser;
    sc850sl_set_int_t_l(nPipeId, ex_ival);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX] = ex_ival;

    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        fExpLineFromUser = ptIntTime->fIntTime[HDR_MEDIUM_FRAME_IDX];
        fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX],
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX]);

        ex_ival = fExpLineFromUser;
        sc850sl_set_int_t_s(nPipeId, ex_ival);

        sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_MEDIUM_FRAME_IDX] = ex_ival;
    }

    return SNS_SUCCESS;
}

AX_S32 sc850sl_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam)
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

AX_U32 sc850sl_sns_update_regidx_table(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue)
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

AX_U32 sc850sl_sns_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    AX_U32  nRegValue;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);

    nNum = sizeof(gsc850slAeRegsTable) / sizeof(gsc850slAeRegsTable[0]);

    for (i = 0; i < nNum; i++) {
        nRegValue = sc850sl_reg_read(nPipeId, gsc850slAeRegsTable[i].nRegAddr);
        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr = gsc850slAeRegsTable[i].nRegAddr;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nData = nRegValue;

        SNS_DBG(" nRegAddr 0x%x, nRegValue 0x%x\n", sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
    }

    return SNS_SUCCESS;
}

AX_S32 sc850sl_sns_update_init_exposure_reg(ISP_PIPE_ID nPipeId)
{
    int i = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        sc850sl_write_register(nPipeId, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
        SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
            i, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
    }

    return SNS_SUCCESS;
}

AX_S32 sc850sl_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo)
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
        sns_obj->sztRegsInfo[0].tComBus.I2cDev = sc850sl_get_bus_num(nPipeId);
        sns_obj->sztRegsInfo[0].nRegNum = sizeof(gsc850slAeRegsTable) / sizeof(gsc850slAeRegsTable[0]);
        sns_obj->sztRegsInfo[0].nCfg2ValidDelayMax = 2;

        for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDevAddr = gSc850slSlaveAddr[nPipeId];
            sns_obj->sztRegsInfo[0].sztI2cData[i].nAddrByteNum = SC850SL_ADDR_BYTE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDataByteNum = SC850SL_DATA_BYTE;
            sns_obj->sztRegsInfo[0].sztI2cData[i].nDelayFrmNum = gsc850slAeRegsTable[i].nDelayFrmNum;
            if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
                sns_obj->sztRegsInfo[0].sztI2cData[i].nIntPos = AX_SNS_L_FSOF;
            } else {
                sns_obj->sztRegsInfo[0].sztI2cData[i].nIntPos = AX_SNS_S_FSOF;
            }
            SNS_DBG("pipe %d, [%2d] nRegAddr 0x%x, nRegValue 0x%x\n", nPipeId, i,
                    gsc850slAeRegsTable[i].nRegAddr, gsc850slAeRegsTable[i].nRegValue);
        }

        bUpdateReg = AX_TRUE;
        sns_obj->bSyncInit = AX_TRUE;
        // sc850sl_sns_update_init_exposure_reg(nPipeId);
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

AX_S32 sc850sl_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 framerate = SNS_MAX_FRAME_RATE;
    AX_U32 nfps = 0;
    AX_U32 nVts = 0;

    SNS_CHECK_PTR_VALID(ptSlowShutterParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate = sns_obj->sns_mode_obj.fFrameRate;
    if (framerate > SNS_MAX_FRAME_RATE) {
        SNS_ERR(" framerate out of range %d \n", framerate);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    if (sns_obj->ae_ctrl_param.fTimePerLine == 0) {
        SNS_ERR("line_period is zero : %f\n", sns_obj->ae_ctrl_param.fTimePerLine);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    ptSlowShutterParam->nGroupNum = AXSNS_MIN((sizeof(gFpsGear) / sizeof(AX_F32)), framerate);
    ptSlowShutterParam->fMinFps = AXSNS_MAX(gFpsGear[0],
                                            (1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * SC850SL_MAX_VTS)));

    for (nfps = 0 ; nfps < ptSlowShutterParam->nGroupNum; nfps++) {
        nVts = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * gFpsGear[nfps]);
        if ((AX_S32)gFpsGear[nfps] >= framerate) {
            nVts = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * framerate);
        }
        if (nVts > SC850SL_MAX_VTS) {
            nVts = SC850SL_MAX_VTS;
            SNS_WRN("Beyond minmum fps  %f\n", ptSlowShutterParam->fMinFps);
        }

        if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime =  SC850SL_SDR_EXP_LINE_MAX(nVts);
        } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime = SC850SL_HDR_2X_LONG_EXP_LINE_MAX(nVts, (nVts + 2 * SC850SL_MAX_RATIO - 8) / (SC850SL_MAX_RATIO + 1));
        }

        ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * nVts);
        ptSlowShutterParam->fMaxFps  =  ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps;

        SNS_DBG("nPipeId = %d, line_period = %.2f, fps = %.2f, fMaxIntTime = %.2f, vts=0x%x\n",
                nPipeId, ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime * sns_obj->ae_ctrl_param.fTimePerLine,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime, nVts);
    }

    return SNS_SUCCESS;
}

AX_S32 sc850sl_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps)
{
    AX_U32 vts = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (IS_FLOAT_ZERO(fFps)) {
        SNS_ERR("userFps is zero !\n");
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    if (IS_SNS_FPS_EQUAL(fFps, sns_sc850slparams[nPipeId].setting_fps)) {
        vts = sns_sc850slparams[nPipeId].setting_vts;
    } else {
        vts = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * fFps);
        if (vts > SC850SL_MAX_VTS){
            vts = SC850SL_MAX_VTS;
            SNS_WRN("Beyond max vts:0x%x\n", vts);
        }
    }

    sc850sl_set_vts(nPipeId, vts);
    sns_sc850slparams[nPipeId].vts = vts;
    if (IS_SNS_FPS_EQUAL(fFps, sns_obj->sns_attr_param.fFrameRate)) {
        sns_obj->sns_mode_obj.nVts = vts;
        sns_obj->sns_mode_obj.fFrameRate = sns_obj->sns_attr_param.fFrameRate;
    }

    sc850sl_set_exp_limit(nPipeId, sns_obj->ae_ctrl_param.sns_ae_param.fCurRatio, sns_sc850slparams[nPipeId].vts);
    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * vts);

    SNS_DBG("pipe:%d, userFps:%f, curFps:%f, vts:0x%x\n",
        nPipeId, fFps, sns_obj->ae_ctrl_param.sns_ae_param.fCurFps, vts);

    return SNS_SUCCESS;
}
