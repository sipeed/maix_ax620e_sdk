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

#include "sc500ai_reg.h"
#include "sc500ai_ae_ctrl.h"

extern SNS_STATE_OBJ *gSc500aiCtx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = gSc500aiCtx[dev])
extern AX_U8 gSc500aiSlaveAddr[AX_VIN_MAX_PIPE_NUM];

static SNSSC500AI_OBJ gSc500aiParams[AX_VIN_MAX_PIPE_NUM];
static AX_F32 gAgainTable[SENSOR_MAX_GAIN_STEP];
static AX_F32 gDgainTable[SENSOR_MAX_GAIN_STEP];

/*user config*/
static AX_F32 gFpsGear[] = { 1.00, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00,
                            11.00, 12.00, 13.00, 14.00, 15.00, 16.00, 17.00, 18.00, 19.00, 20.00,
                            21.00, 22.00, 23.00, 24.00, 25.00, 26.00, 27.00, 28.00, 29.00, 30.00
                           };

static AX_SNS_DRV_DELAY_TABLE_T gSc500aiAeRegsTable[] = {
    /* index */                 /* nRegAddr */         /* regs value */   /* Delay Frame Number */
    {SC500AI_EXP_L_H_IDX,       SC500AI_EXP_L_H,               0,                  0},
    {SC500AI_EXP_L_M_IDX,       SC500AI_EXP_L_M,               0,                  0},
    {SC500AI_EXP_L_L_IDX,       SC500AI_EXP_L_L,               0,                  0},
    {SC500AI_EXP_S_H_IDX,       SC500AI_EXP_S_H,               0,                  0},
    {SC500AI_EXP_S_M_IDX,       SC500AI_EXP_S_M,               0,                  0},
    {SC500AI_EXP_S_L_IDX,       SC500AI_EXP_S_L,               0,                  0},
    {SC500AI_AGAIN_L_IDX,       SC500AI_AGAIN_L,               0,                  0},
    {SC500AI_AGAIN_L_FINE_IDX,  SC500AI_AGAIN_L_FINE,          0,                  0},
    {SC500AI_AGAIN_S_IDX,       SC500AI_AGAIN_S,               0,                  0},
    {SC500AI_AGAIN_S_FINE_IDX,  SC500AI_AGAIN_S_FINE,          0,                  0},
    {SC500AI_DGAIN_L_IDX,       SC500AI_DGAIN_L,               0,                  0},
    {SC500AI_DGAIN_L_FINE_IDX,  SC500AI_DGAIN_L_FINE,          0,                  0},
    {SC500AI_DGAIN_S_IDX,       SC500AI_DGAIN_S,               0,                  0},
    {SC500AI_DGAIN_S_FINE_IDX,  SC500AI_DGAIN_S_FINE,          0,                  0},
    {SC500AI_VTS_H_IDX,         SC500AI_VTS_H,                 0,                  0},
    {SC500AI_VTS_L_IDX,         SC500AI_VTS_L,                 0,                  0},
    {SC500AI_VTS_S_H_IDX,       SC500AI_VTS_S_H,               0,                  0},
    {SC500AI_VTS_S_L_IDX,       SC500AI_VTS_S_L,               0,                  0},
};


const sc500ai_GAIN_TABLE_T sc500ai_again_table[] = {
    {1.000,  0x03, 0x40},
    {1.016,  0x03, 0x41},
    {1.031,  0x03, 0x42},
    {1.047,  0x03, 0x43},
    {1.063,  0x03, 0x44},
    {1.078,  0x03, 0x45},
    {1.094,  0x03, 0x46},
    {1.109,  0x03, 0x47},
    {1.125,  0x03, 0x48},
    {1.141,  0x03, 0x49},
    {1.156,  0x03, 0x4A},
    {1.172,  0x03, 0x4B},
    {1.188,  0x03, 0x4C},
    {1.203,  0x03, 0x4D},
    {1.219,  0x03, 0x4E},
    {1.234,  0x03, 0x4F},
    {1.250,  0x03, 0x50},
    {1.266,  0x03, 0x51},
    {1.281,  0x03, 0x52},
    {1.297,  0x03, 0x53},
    {1.313,  0x03, 0x54},
    {1.328,  0x03, 0x55},
    {1.344,  0x03, 0x56},
    {1.359,  0x03, 0x57},
    {1.375,  0x03, 0x58},
    {1.391,  0x03, 0x59},
    {1.406,  0x03, 0x5A},
    {1.422,  0x03, 0x5B},
    {1.438,  0x03, 0x5C},
    {1.453,  0x03, 0x5D},
    {1.469,  0x03, 0x5E},
    {1.484,  0x03, 0x5F},
    {1.500,  0x03, 0x60},
    {1.516,  0x23, 0x40},
    {1.540,  0x23, 0x41},
    {1.563,  0x23, 0x42},
    {1.587,  0x23, 0x43},
    {1.611,  0x23, 0x44},
    {1.634,  0x23, 0x45},
    {1.658,  0x23, 0x46},
    {1.682,  0x23, 0x47},
    {1.706,  0x23, 0x48},
    {1.729,  0x23, 0x49},
    {1.753,  0x23, 0x4A},
    {1.777,  0x23, 0x4B},
    {1.800,  0x23, 0x4C},
    {1.824,  0x23, 0x4D},
    {1.848,  0x23, 0x4E},
    {1.871,  0x23, 0x4F},
    {1.895,  0x23, 0x50},
    {1.919,  0x23, 0x51},
    {1.942,  0x23, 0x52},
    {1.966,  0x23, 0x53},
    {1.990,  0x23, 0x54},
    {2.013,  0x23, 0x55},
    {2.037,  0x23, 0x56},
    {2.061,  0x23, 0x57},
    {2.085,  0x23, 0x58},
    {2.108,  0x23, 0x59},
    {2.132,  0x23, 0x5A},
    {2.156,  0x23, 0x5B},
    {2.179,  0x23, 0x5C},
    {2.203,  0x23, 0x5D},
    {2.227,  0x23, 0x5E},
    {2.250,  0x23, 0x5F},
    {2.274,  0x23, 0x60},
    {2.298,  0x23, 0x61},
    {2.321,  0x23, 0x62},
    {2.345,  0x23, 0x63},
    {2.369,  0x23, 0x64},
    {2.392,  0x23, 0x65},
    {2.416,  0x23, 0x66},
    {2.440,  0x23, 0x67},
    {2.464,  0x23, 0x68},
    {2.487,  0x23, 0x69},
    {2.511,  0x23, 0x6A},
    {2.535,  0x23, 0x6B},
    {2.558,  0x23, 0x6C},
    {2.582,  0x23, 0x6D},
    {2.606,  0x23, 0x6E},
    {2.629,  0x23, 0x6F},
    {2.653,  0x23, 0x70},
    {2.677,  0x23, 0x71},
    {2.700,  0x23, 0x72},
    {2.724,  0x23, 0x73},
    {2.748,  0x23, 0x74},
    {2.771,  0x23, 0x75},
    {2.795,  0x23, 0x76},
    {2.819,  0x23, 0x77},
    {2.843,  0x23, 0x78},
    {2.866,  0x23, 0x79},
    {2.890,  0x23, 0x7A},
    {2.914,  0x23, 0x7B},
    {2.937,  0x23, 0x7C},
    {2.961,  0x23, 0x7D},
    {2.985,  0x23, 0x7E},
    {3.008,  0x23, 0x7F},
    {3.032,  0x27, 0x40},
    {3.079,  0x27, 0x41},
    {3.127,  0x27, 0x42},
    {3.174,  0x27, 0x43},
    {3.222,  0x27, 0x44},
    {3.269,  0x27, 0x45},
    {3.316,  0x27, 0x46},
    {3.364,  0x27, 0x47},
    {3.411,  0x27, 0x48},
    {3.458,  0x27, 0x49},
    {3.506,  0x27, 0x4A},
    {3.553,  0x27, 0x4B},
    {3.601,  0x27, 0x4C},
    {3.648,  0x27, 0x4D},
    {3.695,  0x27, 0x4E},
    {3.743,  0x27, 0x4F},
    {3.790,  0x27, 0x50},
    {3.837,  0x27, 0x51},
    {3.885,  0x27, 0x52},
    {3.932,  0x27, 0x53},
    {3.980,  0x27, 0x54},
    {4.027,  0x27, 0x55},
    {4.074,  0x27, 0x56},
    {4.122,  0x27, 0x57},
    {4.169,  0x27, 0x58},
    {4.216,  0x27, 0x59},
    {4.264,  0x27, 0x5A},
    {4.311,  0x27, 0x5B},
    {4.359,  0x27, 0x5C},
    {4.406,  0x27, 0x5D},
    {4.453,  0x27, 0x5E},
    {4.501,  0x27, 0x5F},
    {4.548,  0x27, 0x60},
    {4.595,  0x27, 0x61},
    {4.643,  0x27, 0x62},
    {4.690,  0x27, 0x63},
    {4.738,  0x27, 0x64},
    {4.785,  0x27, 0x65},
    {4.832,  0x27, 0x66},
    {4.880,  0x27, 0x67},
    {4.927,  0x27, 0x68},
    {4.974,  0x27, 0x69},
    {5.022,  0x27, 0x6A},
    {5.069,  0x27, 0x6B},
    {5.117,  0x27, 0x6C},
    {5.164,  0x27, 0x6D},
    {5.211,  0x27, 0x6E},
    {5.259,  0x27, 0x6F},
    {5.306,  0x27, 0x70},
    {5.353,  0x27, 0x71},
    {5.401,  0x27, 0x72},
    {5.448,  0x27, 0x73},
    {5.496,  0x27, 0x74},
    {5.543,  0x27, 0x75},
    {5.590,  0x27, 0x76},
    {5.638,  0x27, 0x77},
    {5.685,  0x27, 0x78},
    {5.732,  0x27, 0x79},
    {5.780,  0x27, 0x7A},
    {5.827,  0x27, 0x7B},
    {5.875,  0x27, 0x7C},
    {5.922,  0x27, 0x7D},
    {5.969,  0x27, 0x7E},
    {6.017,  0x27, 0x7F},
    {6.064,  0x2F, 0x40},
    {6.159,  0x2F, 0x41},
    {6.254,  0x2F, 0x42},
    {6.348,  0x2F, 0x43},
    {6.443,  0x2F, 0x44},
    {6.538,  0x2F, 0x45},
    {6.633,  0x2F, 0x46},
    {6.727,  0x2F, 0x47},
    {6.822,  0x2F, 0x48},
    {6.917,  0x2F, 0x49},
    {7.012,  0x2F, 0x4A},
    {7.106,  0x2F, 0x4B},
    {7.201,  0x2F, 0x4C},
    {7.296,  0x2F, 0x4D},
    {7.391,  0x2F, 0x4E},
    {7.485,  0x2F, 0x4F},
    {7.580,  0x2F, 0x50},
    {7.675,  0x2F, 0x51},
    {7.770,  0x2F, 0x52},
    {7.864,  0x2F, 0x53},
    {7.959,  0x2F, 0x54},
    {8.054,  0x2F, 0x55},
    {8.149,  0x2F, 0x56},
    {8.243,  0x2F, 0x57},
    {8.338,  0x2F, 0x58},
    {8.433,  0x2F, 0x59},
    {8.528,  0x2F, 0x5A},
    {8.622,  0x2F, 0x5B},
    {8.717,  0x2F, 0x5C},
    {8.812,  0x2F, 0x5D},
    {8.907,  0x2F, 0x5E},
    {9.001,  0x2F, 0x5F},
    {9.096,  0x2F, 0x60},
    {9.191,  0x2F, 0x61},
    {9.286,  0x2F, 0x62},
    {9.380,  0x2F, 0x63},
    {9.475,  0x2F, 0x64},
    {9.570,  0x2F, 0x65},
    {9.665,  0x2F, 0x66},
    {9.759,  0x2F, 0x67},
    {9.854,  0x2F, 0x68},
    {9.949,  0x2F, 0x69},
    {10.044, 0x2F, 0x6A},
    {10.138, 0x2F, 0x6B},
    {10.233, 0x2F, 0x6C},
    {10.328, 0x2F, 0x6D},
    {10.423, 0x2F, 0x6E},
    {10.517, 0x2F, 0x6F},
    {10.612, 0x2F, 0x70},
    {10.707, 0x2F, 0x71},
    {10.802, 0x2F, 0x72},
    {10.896, 0x2F, 0x73},
    {10.991, 0x2F, 0x74},
    {11.086, 0x2F, 0x75},
    {11.181, 0x2F, 0x76},
    {11.275, 0x2F, 0x77},
    {11.370, 0x2F, 0x78},
    {11.465, 0x2F, 0x79},
    {11.560, 0x2F, 0x7A},
    {11.654, 0x2F, 0x7B},
    {11.749, 0x2F, 0x7C},
    {11.844, 0x2F, 0x7D},
    {11.939, 0x2F, 0x7E},
    {12.033, 0x2F, 0x7F},
    // {12.128, 0x3F, 0x40}, bad
    {12.318, 0x3F, 0x41},
    {12.507, 0x3F, 0x42},
    {12.697, 0x3F, 0x43},
    {12.886, 0x3F, 0x44},
    {13.076, 0x3F, 0x45},
    {13.265, 0x3F, 0x46},
    {13.455, 0x3F, 0x47},
    {13.644, 0x3F, 0x48},
    {13.834, 0x3F, 0x49},
    {14.023, 0x3F, 0x4A},
    {14.213, 0x3F, 0x4B},
    {14.402, 0x3F, 0x4C},
    {14.592, 0x3F, 0x4D},
    {14.781, 0x3F, 0x4E},
    {14.971, 0x3F, 0x4F},
    {15.160, 0x3F, 0x50},
    {15.350, 0x3F, 0x51},
    {15.539, 0x3F, 0x52},
    {15.729, 0x3F, 0x53},
    {15.918, 0x3F, 0x54},
    {16.108, 0x3F, 0x55},
    {16.297, 0x3F, 0x56},
    {16.487, 0x3F, 0x57},
    {16.676, 0x3F, 0x58},
    {16.866, 0x3F, 0x59},
    {17.055, 0x3F, 0x5A},
    {17.245, 0x3F, 0x5B},
    {17.434, 0x3F, 0x5C},
    {17.624, 0x3F, 0x5D},
    {17.813, 0x3F, 0x5E},
    {18.003, 0x3F, 0x5F},
    {18.192, 0x3F, 0x60},
    {18.382, 0x3F, 0x61},
    {18.571, 0x3F, 0x62},
    {18.761, 0x3F, 0x63},
    {18.950, 0x3F, 0x64},
    {19.140, 0x3F, 0x65},
    {19.329, 0x3F, 0x66},
    {19.519, 0x3F, 0x67},
    {19.708, 0x3F, 0x68},
    {19.898, 0x3F, 0x69},
    {20.087, 0x3F, 0x6A},
    {20.277, 0x3F, 0x6B},
    {20.466, 0x3F, 0x6C},
    {20.656, 0x3F, 0x6D},
    {20.845, 0x3F, 0x6E},
    {21.035, 0x3F, 0x6F},
    {21.224, 0x3F, 0x70},
    {21.414, 0x3F, 0x71},
    {21.603, 0x3F, 0x72},
    {21.793, 0x3F, 0x73},
    {21.982, 0x3F, 0x74},
    {22.172, 0x3F, 0x75},
    {22.361, 0x3F, 0x76},
    {22.551, 0x3F, 0x77},
    {22.740, 0x3F, 0x78},
    {22.930, 0x3F, 0x79},
    {23.119, 0x3F, 0x7A},
    {23.309, 0x3F, 0x7B},
    {23.498, 0x3F, 0x7C},
    {23.688, 0x3F, 0x7D},
    {23.877, 0x3F, 0x7E},
    {24.067, 0x3F, 0x7F},
};

const sc500ai_GAIN_TABLE_T sc500ai_dgain_table[] = {

    {1.000,  0x00, 0x80},
    // {1.016,  0x00, 0x82},bad
    {1.031,  0x00, 0x84},
    // {1.047,  0x00, 0x86},bad
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
    // {1.516,  0x00, 0xc2},bad
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
    // {1.984,  0x00, 0xfe},bad
    {2.000,  0x01, 0x80},
    // {2.031,  0x01, 0x82},bad
    // {2.063,  0x01, 0x84},bad
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
    // {3.031,  0x01, 0xc2},bad
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
    {16.000, 0x0f, 0x80},
    {16.250, 0x0f, 0x82},
    {16.500, 0x0f, 0x84},
    {16.750, 0x0f, 0x86},
    {17.000, 0x0f, 0x88},
    {17.250, 0x0f, 0x8a},
    {17.500, 0x0f, 0x8c},
    {17.750, 0x0f, 0x8e},
    {18.000, 0x0f, 0x90},
    {18.250, 0x0f, 0x92},
    {18.500, 0x0f, 0x94},
    {18.750, 0x0f, 0x96},
    {19.000, 0x0f, 0x98},
    {19.250, 0x0f, 0x9a},
    {19.500, 0x0f, 0x9c},
    {19.750, 0x0f, 0x9e},
    {20.000, 0x0f, 0xa0},
    {20.250, 0x0f, 0xa2},
    {20.500, 0x0f, 0xa4},
    {20.750, 0x0f, 0xa6},
    {21.000, 0x0f, 0xa8},
    {21.250, 0x0f, 0xaa},
    {21.500, 0x0f, 0xac},
    {21.750, 0x0f, 0xae},
    {22.000, 0x0f, 0xb0},
    {22.250, 0x0f, 0xb2},
    {22.500, 0x0f, 0xb4},
    {22.750, 0x0f, 0xb6},
    {23.000, 0x0f, 0xb8},
    {23.250, 0x0f, 0xba},
    {23.500, 0x0f, 0xbc},
    {23.750, 0x0f, 0xbe},
    {24.000, 0x0f, 0xc0},
    {24.250, 0x0f, 0xc2},
    {24.500, 0x0f, 0xc4},
    {24.750, 0x0f, 0xc6},
    {25.000, 0x0f, 0xc8},
    {25.250, 0x0f, 0xca},
    {25.500, 0x0f, 0xcc},
    {25.750, 0x0f, 0xce},
    {26.000, 0x0f, 0xd0},
    {26.250, 0x0f, 0xd2},
    {26.500, 0x0f, 0xd4},
    {26.750, 0x0f, 0xd6},
    {27.000, 0x0f, 0xd8},
    {27.250, 0x0f, 0xda},
    {27.500, 0x0f, 0xdc},
    {27.750, 0x0f, 0xde},
    {28.000, 0x0f, 0xe0},
    {28.250, 0x0f, 0xe2},
    {28.500, 0x0f, 0xe4},
    {28.750, 0x0f, 0xe6},
    {29.000, 0x0f, 0xe8},
    {29.250, 0x0f, 0xea},
    {29.500, 0x0f, 0xec},
    {29.750, 0x0f, 0xee},
    {30.000, 0x0f, 0xf0},
    {30.250, 0x0f, 0xf2},
    {30.500, 0x0f, 0xf4},
    {30.750, 0x0f, 0xf6},
    {31.000, 0x0f, 0xf8},
    {31.250, 0x0f, 0xfa},
    {31.500, 0x0f, 0xfc},
    {31.750, 0x0f, 0xfe},
};


AX_F32 sc500ai_again2value(float gain, AX_U8 *again_in, AX_U8 *again_de)
{
    AX_U32 i;
    AX_U32 count;

    if (!again_in || !again_de)
        return -1;

    count = sizeof(sc500ai_again_table) / sizeof(sc500ai_again_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > sc500ai_again_table[i].gain) {
            continue;
        } else {
            *again_in = sc500ai_again_table[i].gain_in;
            *again_de = sc500ai_again_table[i].gain_de;
            SNS_DBG("again=%f, again_in=0x%x, again_de=0x%x\n", gain, *again_in, *again_de);
            return sc500ai_again_table[i].gain;
        }
    }

    return -1;
}

AX_F32 sc500ai_dgain2value(float gain, AX_U8 *dgain_in, AX_U8 *dgain_de)
{
    AX_U32 i;
    AX_U32 count;

    if (!dgain_in || !dgain_de)
        return -1;

    count = sizeof(sc500ai_dgain_table) / sizeof(sc500ai_dgain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > sc500ai_dgain_table[i].gain) {
            continue;
        } else {
            *dgain_in = sc500ai_dgain_table[i].gain_in;
            *dgain_de = sc500ai_dgain_table[i].gain_de;
            SNS_DBG("dgain=%f, dgain_in=0x%x, dgain_de=0x%x\n", gain, *dgain_in, *dgain_de);

            return sc500ai_dgain_table[i].gain;
        }
    }
    return -1;
}

AX_S32 sc500ai_update_regidx_table(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue)
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

static AX_S32 sc500ai_set_exp_limit(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio, AX_U32 nVts)
{
    AX_U32 vts_s = 0;
    SNS_STATE_OBJ *sns_obj = NULL;

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = (AX_F32)(2 * nVts - 10) / 2;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = (AX_F32)3 / 2;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 2;
    } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        //2*vts - 2*vts_s - 18 = (2*vts_s - 14) * fHdrRatio
        vts_s = (nVts + 7 * fHdrRatio - 9) / (fHdrRatio + 1);
        sc500ai_set_vts_s(nPipeId, vts_s);

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = (AX_F32)(2 * nVts - 2 * vts_s - 18) / 2;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = (AX_F32)5 / 2;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)4 / 2;

        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] = (AX_F32)(2 * vts_s - 14) / 2;
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] = (AX_F32)5 / 2;
        sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)4 / 2;

    } else {
        // wrong hdr mode
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

AX_S32 sc500ai_cfg_aec_param(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sns_mode_obj.nVts = AXSNS_DIV_0_TO_1(sc500ai_get_vts(nPipeId));
    gSc500aiParams[nPipeId].vts = sns_obj->sns_mode_obj.nVts;

    sns_obj->ae_ctrl_param.fTimePerLine =
        (float)1 * SNS_1_SECOND_UNIT_US / (sns_obj->sns_mode_obj.nVts * sns_obj->sns_mode_obj.fFrameRate);

    /* exp limit */
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio = SC500AI_MIN_RATIO;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio = SC500AI_MAX_RATIO;
    sc500ai_set_exp_limit(nPipeId, SC500AI_MAX_RATIO, gSc500aiParams[nPipeId].vts);

    /* gain limit */
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX] = SC500AI_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 256;

    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX] = SC500AI_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 128;

    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX] = SC500AI_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 256;

    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX] = 1.0f;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX] = SC500AI_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 128;

    /* ispgain */
    sns_obj->ae_ctrl_param.sns_ae_param.fIspDGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1/256;  //U10.8

    /* hcglcg */
    sns_obj->ae_ctrl_param.eSnsHcgLcgMode = AX_LCG_NOTSUPPORT_MODE;
    sns_obj->ae_ctrl_param.fSnsHcgLcgRatio = 1;

    /* current fps */
    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = sns_obj->sns_mode_obj.fFrameRate;

    return SNS_SUCCESS;
}

AX_S32 sc500ai_get_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_U32 i = 0;

    SNS_CHECK_PTR_VALID(params);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    params->nAgainTableSize = sizeof(sc500ai_again_table) / sizeof(sc500ai_again_table[0]);
    params->nDgainTableSize = sizeof(sc500ai_dgain_table) / sizeof(sc500ai_dgain_table[0]);

    for (i = 0; i < params->nAgainTableSize; i++) {
        gAgainTable[i] = sc500ai_again_table[i].gain;
    }

    for (i = 0; i < params->nDgainTableSize; i++) {
        gDgainTable[i] = sc500ai_dgain_table[i].gain;
    }

    params->pAgainTable = gAgainTable;
    params->pDgainTable = gDgainTable;

    return SNS_SUCCESS;
}

AX_S32 sc500ai_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain)
{
    AX_U8 gain_in = 0;
    AX_U8 gain_de = 0;
    AX_F32 gain_val = 0;
    AX_F32 fGainFromUser = 0;

    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptAGain);

    /* long gain seting */
    fGainFromUser = ptAGain->fGain[HDR_LONG_FRAME_IDX];
    fGainFromUser = AXSNS_CLIP3(fGainFromUser,
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX],
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX]);

    gain_val = sc500ai_again2value(fGainFromUser, &gain_in, &gain_de);
    if (gain_val == -1) {
        SNS_ERR("LEF userAgain:%f match failed\n", fGainFromUser);
        return -1;
    } else {
        sc500ai_update_regidx_table(nPipeId, SC500AI_AGAIN_L_IDX, gain_in);
        sc500ai_update_regidx_table(nPipeId, SC500AI_AGAIN_L_FINE_IDX, gain_de);

        sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX] = gain_val;
    }

    /* medium gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {

        fGainFromUser = ptAGain->fGain[HDR_MEDIUM_FRAME_IDX];
        fGainFromUser = AXSNS_CLIP3(fGainFromUser,
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX]);

        gain_val = sc500ai_again2value(fGainFromUser, &gain_in, &gain_de);
        if (gain_val == -1) {
            SNS_ERR("SEF1 userAgain:%f match failed\n", fGainFromUser);
            return -1;
        } else {
            sc500ai_update_regidx_table(nPipeId, SC500AI_AGAIN_S_IDX, gain_in);
            sc500ai_update_regidx_table(nPipeId, SC500AI_AGAIN_S_FINE_IDX, gain_de);

            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_MEDIUM_FRAME_IDX] = gain_val;
        }
    }

    SNS_DBG("pipe:%d, userAgain:%f-%f, curAgain:%f-%f\n",
            nPipeId,
            ptAGain->fGain[HDR_LONG_FRAME_IDX], ptAGain->fGain[HDR_MEDIUM_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_MEDIUM_FRAME_IDX]);

    return SNS_SUCCESS;
}

AX_S32 sc500ai_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain)
{
    AX_U8 gain_in = 0;
    AX_U8 gain_de = 0;
    AX_F32 gain_val = 0;
    AX_F32 fGainFromUser = 0;

    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptDGain);

    /* long frame digital gain seting */
    fGainFromUser = ptDGain->fGain[HDR_LONG_FRAME_IDX];
    fGainFromUser = AXSNS_CLIP3(fGainFromUser,
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX],
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX]);

    gain_val = sc500ai_dgain2value(fGainFromUser, &gain_in, &gain_de);
    if (gain_val == -1) {
        SNS_ERR("LEF userDgain:%f match failed\n", fGainFromUser);
        return -1;
    } else {
        sc500ai_update_regidx_table(nPipeId, SC500AI_DGAIN_L_IDX, gain_in);
        sc500ai_update_regidx_table(nPipeId, SC500AI_DGAIN_L_FINE_IDX, gain_de);
        sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX] = gain_val;
    }

    /* medium frame digital gain seting */
    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        fGainFromUser = ptDGain->fGain[HDR_MEDIUM_FRAME_IDX];
        fGainFromUser = AXSNS_CLIP3(fGainFromUser,
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX]);

        gain_val = sc500ai_dgain2value(fGainFromUser, &gain_in, &gain_de);
        if (gain_val == -1) {
            SNS_ERR("SEF1 userDgain:%f match failed\n", fGainFromUser);
            return -1;
        } else {
            sc500ai_update_regidx_table(nPipeId, SC500AI_DGAIN_S_IDX, gain_in);
            sc500ai_update_regidx_table(nPipeId, SC500AI_DGAIN_S_FINE_IDX, gain_de);
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_MEDIUM_FRAME_IDX] = gain_val;
        }
    }

    SNS_DBG("pipe:%d, userDgain:%f-%f, curDgain:%f-%f\n",
            nPipeId,
            ptDGain->fGain[HDR_LONG_FRAME_IDX], ptDGain->fGain[HDR_MEDIUM_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_MEDIUM_FRAME_IDX]);

    return SNS_SUCCESS;
}

AX_S32 sc500ai_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio,
        AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SNS_CHECK_PTR_VALID(ptIntTimeRange);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (fabs(fHdrRatio) < EPS) {
        SNS_ERR("hdr ratio is error \n");
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    sc500ai_set_exp_limit(nPipeId, fHdrRatio, gSc500aiParams[nPipeId].vts);

    if (AX_SNS_LINEAR_MODE == sns_obj->sns_mode_obj.eHDRMode) {

        ptIntTimeRange->fMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX];
        ptIntTimeRange->fMinIntegrationTime[HDR_LONG_FRAME_IDX] =
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX];

    } else if (AX_SNS_HDR_2X_MODE == sns_obj->sns_mode_obj.eHDRMode) {

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

    return SNS_SUCCESS;
}

AX_S32 sc500ai_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime)
{
    AX_U8 exp_h = 0, exp_m = 0, exp_l = 0;
    AX_U32 exp_reg = 0;
    AX_F32 fExpLineFromUser = 0;
    AX_F32 ratio = 1.0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTime);

    fExpLineFromUser = ptIntTime->fIntTime[HDR_LONG_FRAME_IDX];
    fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX]);

    exp_reg = 2 * fExpLineFromUser;
    exp_h = (exp_reg & 0xF000) >> 12;
    exp_m = (exp_reg & 0xFF0) >> 4;
    exp_l = (exp_reg & 0xF) << 4;

    sc500ai_update_regidx_table(nPipeId, SC500AI_EXP_L_H_IDX, exp_h);
    sc500ai_update_regidx_table(nPipeId, SC500AI_EXP_L_M_IDX, exp_m);
    sc500ai_update_regidx_table(nPipeId, SC500AI_EXP_L_L_IDX, exp_l);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX] = (AX_F32)exp_reg / 2;

    if (IS_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        fExpLineFromUser = ptIntTime->fIntTime[HDR_MEDIUM_FRAME_IDX];
        fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX],
                                        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX]);

        exp_reg = 2 * fExpLineFromUser;
        exp_h = (exp_reg & 0xF000) >> 12;
        exp_m = (exp_reg & 0xFF0) >> 4;
        exp_l = (exp_reg & 0xF) << 4;

        sc500ai_update_regidx_table(nPipeId, SC500AI_EXP_S_H_IDX, exp_h);
        sc500ai_update_regidx_table(nPipeId, SC500AI_EXP_S_M_IDX, exp_m);
        sc500ai_update_regidx_table(nPipeId, SC500AI_EXP_S_L_IDX, exp_l);

        sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_MEDIUM_FRAME_IDX] = (AX_F32)exp_reg / 2;

        ratio = (AX_F32)sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX] /
                        sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_MEDIUM_FRAME_IDX];
    }

    SNS_DBG("userExpline:%f-%f, userRatio:%.2f, curExpline:%f-%f(%f-%f), curRatio:%.2f, line_period:%f\n",
            ptIntTime->fIntTime[0], ptIntTime->fIntTime[1], ptIntTime->fHdrRatio[0],
            sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_MEDIUM_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
            sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_MEDIUM_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
            ratio, sns_obj->ae_ctrl_param.fTimePerLine);

    return SNS_SUCCESS;
}

AX_S32 sc500ai_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam)
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

AX_S32 sc500ai_hcglcg_ctrl(ISP_PIPE_ID nPipeId, AX_U32 nSnsHcgLcg)
{
    return SNS_SUCCESS;
}

AX_S32 sc500ai_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    AX_U32  nRegValue;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    nNum = sizeof(gSc500aiAeRegsTable) / sizeof(gSc500aiAeRegsTable[0]);

    for (i = 0; i < nNum; i++) {
        nRegValue = sc500ai_reg_read(nPipeId, gSc500aiAeRegsTable[i].nRegAddr);
        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr = gSc500aiAeRegsTable[i].nRegAddr;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nData = nRegValue;

        SNS_DBG(" nRegAddr 0x%x, nRegValue 0x%x\n", gSc500aiAeRegsTable[i].nRegAddr, gSc500aiAeRegsTable[i].nRegValue);
    }

    return SNS_SUCCESS;
}

AX_S32 sc500ai_init_exposure_reg(ISP_PIPE_ID nPipeId)
{
    int i = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        sc500ai_write_register(nPipeId, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
        SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
            i, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
    }

    return SNS_SUCCESS;
}

static AX_VOID _sns_reg_info_init(ISP_PIPE_ID nPipeId, SNS_STATE_OBJ *sns_obj)
{
    AX_S32 i = 0;

    sns_obj->sztRegsInfo[0].eSnsType = ISP_SNS_CONNECT_I2C_TYPE;
    sns_obj->sztRegsInfo[0].tComBus.I2cDev = sc500ai_get_bus_num(nPipeId);
    sns_obj->sztRegsInfo[0].nRegNum = sizeof(gSc500aiAeRegsTable) / sizeof(gSc500aiAeRegsTable[0]);
    sns_obj->sztRegsInfo[0].nCfg2ValidDelayMax = 2;

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDevAddr = gSc500aiSlaveAddr[nPipeId];
        sns_obj->sztRegsInfo[0].sztI2cData[i].nAddrByteNum = SC500AI_ADDR_BYTE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDataByteNum = SC500AI_DATA_BYTE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDelayFrmNum = gSc500aiAeRegsTable[i].nDelayFrmNum;
        if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].nIntPos = AX_SNS_L_FSOF;
        } else {
            sns_obj->sztRegsInfo[0].sztI2cData[i].nIntPos = AX_SNS_S_FSOF;
        }
        SNS_DBG("[%2d] nRegAddr 0x%x, nRegValue 0x%x\n", i,
                gSc500aiAeRegsTable[i].nRegAddr, gSc500aiAeRegsTable[i].nRegValue);
    }

    return;
}

static AX_VOID _sns_reg_info_update(ISP_PIPE_ID nPipeId, SNS_STATE_OBJ *sns_obj, AX_BOOL *bUpdateReg)
{
    AX_S32 i = 0;

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        if (sns_obj->sztRegsInfo[0].sztI2cData[i].nData == sns_obj->sztRegsInfo[1].sztI2cData[i].nData) {
            sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_FALSE;
        } else {
            SNS_DBG("[%2d] nRegAddr 0x%x, nRegValue 0x%x\n", i,
                    sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);

            sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
            *bUpdateReg = AX_TRUE;
        }
    }

    return;
}

AX_S32 sc500ai_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo)
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
        // sc500ai_init_exposure_reg(nPipeId);
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

AX_S32 sc500ai_set_slow_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps)
{
    AX_S32 framerate = 30;
    AX_U32 vts = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate =  sns_obj->sns_mode_obj.fFrameRate;
    vts = sns_obj->sns_mode_obj.nVts;

    if (fFps >= framerate) {
        gSc500aiParams[nPipeId].vts = vts;
    } else {
        gSc500aiParams[nPipeId].vts = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * fFps);
    }

    if (gSc500aiParams[nPipeId].vts > SC500AI_MAX_VTS) {
        gSc500aiParams[nPipeId].vts = SC500AI_MAX_VTS;
        fFps = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * gSc500aiParams[nPipeId].vts);
        SNS_ERR("Beyond minmum fps  %f\n", fFps);
    }

    if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
        gSc500aiParams[nPipeId].vts = AXSNS_ALIGN_DOWN(gSc500aiParams[nPipeId].vts, 2);
    }

    sc500ai_set_vts(nPipeId, gSc500aiParams[nPipeId].vts);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps =
        1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * gSc500aiParams[nPipeId].vts);

    SNS_DBG("pipe:%d, userFps:%f, curFps:%f, line_period:%f, vts:0x%x\n",
            nPipeId, fFps, sns_obj->ae_ctrl_param.sns_ae_param.fCurFps,
            sns_obj->ae_ctrl_param.fTimePerLine, gSc500aiParams[nPipeId].vts);

    return SNS_SUCCESS;
}

AX_S32 sc500ai_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam)
{
    AX_U32 i = 0;
    AX_U32 vts = 0;
    AX_U32 vts_s = 0;
    AX_S32 framerate = 30;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate = sns_obj->sns_mode_obj.fFrameRate;
    if (framerate > SNS_MAX_FRAME_RATE) {
        SNS_ERR("framerate out of range : %d\n", framerate);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    ptSlowShutterParam->nGroupNum = AXSNS_MIN((sizeof(gFpsGear) / sizeof(AX_F32)), framerate);
    //ax_sns_quick_sort_float(gFpsGear, ptSlowShutterParam->nGroupNum);
    ptSlowShutterParam->fMinFps =
        AXSNS_MAX(gFpsGear[0], (1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * SC500AI_MAX_VTS)));

    for (i = 0; i < ptSlowShutterParam->nGroupNum; i++) {
        vts = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * gFpsGear[i]);
        if ((AX_S32)gFpsGear[i] >= framerate) {
            vts = sns_obj->sns_mode_obj.nVts;
        }
        if (vts > SC500AI_MAX_VTS) {
            vts = SC500AI_MAX_VTS;
            SNS_WRN("Beyond minmum fps  %f\n", ptSlowShutterParam->fMinFps);
        }

        if (IS_LINEAR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            ptSlowShutterParam->tSlowShutterTbl[i].fMaxIntTime = (2 * vts - 10) / 2;
        } else if (IS_2DOL_HDR_MODE(sns_obj->sns_mode_obj.eHDRMode)) {
            vts = AXSNS_ALIGN_DOWN(vts, 2);
            vts_s = (vts + 7 * SC500AI_MAX_RATIO - 9) / (SC500AI_MAX_RATIO + 1);
            ptSlowShutterParam->tSlowShutterTbl[i].fMaxIntTime = (2 * vts - 2 * vts_s - 18) / 2;
        }

        ptSlowShutterParam->tSlowShutterTbl[i].fRealFps =
            1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * vts);
        ptSlowShutterParam->fMaxFps = ptSlowShutterParam->tSlowShutterTbl[i].fRealFps;

        SNS_DBG("pipe=%d, ratio=%.2f, fps=%f, max_expline=%f(%f), vts=0x%x, vts_s=0x%x, line_period=%f\n",
                nPipeId, SC500AI_MAX_RATIO, ptSlowShutterParam->tSlowShutterTbl[i].fRealFps,
                ptSlowShutterParam->tSlowShutterTbl[i].fMaxIntTime,
                ptSlowShutterParam->tSlowShutterTbl[i].fMaxIntTime * sns_obj->ae_ctrl_param.fTimePerLine,
                vts, vts_s, sns_obj->ae_ctrl_param.fTimePerLine);
    }

    return SNS_SUCCESS;
}

AX_S32 sc500ai_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps)
{
    AX_U32 vts = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if(IS_SNS_FPS_EQUAL(fFps, 25.0)) {  /* PAL */
        if(AX_SNS_LINEAR_MODE == sns_obj->sns_attr_param.eSnsMode) {
            if(AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) {
                vts = SC500AI_VTS_10BIT_SDR_2880_1620_25FPS;  /* SDR 10BIT 4LINE */
            }
        } else if(AX_SNS_HDR_2X_MODE == sns_obj->sns_attr_param.eSnsMode) {
            vts = SC500AI_VTS_10BIT_HDR_2X_2880_1620_25FPS;  /* HDR 10BIT 4LINE */
        }
    }

    if(IS_SNS_FPS_EQUAL(fFps, 30.0)) {  /* NTSC */
        if(AX_SNS_LINEAR_MODE == sns_obj->sns_attr_param.eSnsMode) {
            if(AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) {
                vts = SC500AI_VTS_10BIT_SDR_2880_1620_30FPS;  /* SDR 10BIT 4LINE */
            }
        } else if(AX_SNS_HDR_2X_MODE == sns_obj->sns_attr_param.eSnsMode) {
            vts = SC500AI_VTS_10BIT_HDR_2X_2880_1620_30FPS;  /* HDR 10BIT 4LINE */
        }
    }

    if(IS_SNS_FPS_EQUAL(fFps, 15.0)) {  /* NTSC */
        if(AX_SNS_LINEAR_MODE == sns_obj->sns_attr_param.eSnsMode) {
            if(AX_RT_RAW10 == sns_obj->sns_attr_param.eRawType) {
                vts = SC500AI_VTS_10BIT_SDR_2880_1620_15FPS;  /* SDR 10BIT 4LINE */
            }
        }
    }

    if(vts == 0) {
        SNS_ERR("vts:%d is not set!\n", vts);
        return -1;
    }

    /* set sensor vts */
    sc500ai_set_vts(nPipeId, vts);
    gSc500aiParams[nPipeId].vts = vts;

    /* change fps */
    sns_obj->sns_mode_obj.nVts = vts;
    sns_obj->sns_mode_obj.fFrameRate = fFps;

    /* update exposure limit parameters */
    sc500ai_set_exp_limit(nPipeId, SC500AI_MAX_RATIO, gSc500aiParams[nPipeId].vts);

    /* cur fps */
    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = fFps;

    SNS_DBG("nPipeId = %d, fps(from alg) = %f, current vts = 0x%x\n", nPipeId, fFps, gSc500aiParams[nPipeId].vts);

    return SNS_SUCCESS;
}
