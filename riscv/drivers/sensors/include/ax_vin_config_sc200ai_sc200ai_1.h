/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_SENSOR_SC200AI_SC200AI1_CONFIG_H_
#define _AX_SENSOR_SC200AI_SC200AI1_CONFIG_H_

#include "ax_base_type.h"
#include "ax_isp_common.h"
#include "ax_isp_iq_api.h"
#include "ax_isp_3a_struct.h"

#include "ax_vin_struct.h"

extern AX_SENSOR_REGISTER_FUNC_T gSnssc200aiObj;

const static AX_SNS_CONFIG_T sc200ai_sc200ai_sns_config1 = {
    2,
    ISP_RESERVED_DDR_BASE,
    ISP_RESERVED_DDR_END,
    /* attr */
    {
        {
                /* attr */
                {
                    /* sensor obj */
                    &gSnssc200aiObj,
                    /* mSnsId */
                    1,
                    /* eSnsMode */
                    AX_SNS_LINEAR_MODE,
                    /*AX_SNS_MASTER_SLAVE_E*/
                    AX_SNS_MASTER,
                    /* mClkId */
                    0,
                    /* eClkRate */
                    AX_SNS_CLK_24M,
                    /* nI2cDev */
                    0,
                    /* nI2cAddr */
                    0x30,
                    /* nResetGpio */
                    10,
                    /* tInitAttr */
                    {
                        /* nFps */
                        60,
                        /* nWidth */
                        1920,
                        /* nHeight */
                        1080,
                        /* nSettingIndex */
                        0,
                    },
                    /* tConvergeAttr */
                    {
                        /* nFps */
                        15,
                        /* nWidth */
                        1920,
                        /* nHeight */
                        1080,
                        /* nSettingIndex */
                        0,
                    },
                },
                /* nDevId */
                0,
                /* mipi attr */
                {
                    /* nMipiId */
                    0,
                    /* eLaneMode */
                    AX_LANE_COMBO_MODE_1,
                    /* tRxAttr */
                    {
                        /* ePhyMode */
                        AX_MIPI_PHY_TYPE_DPHY,
                        /* eLaneNum */
                        AX_MIPI_DATA_LANE_2,
                        /* nDataRate */
                        1440,
                        /* nDataLaneMap[4] */
                        {0, 1, 3, 4},
                        /* nClkLane[2] */
                        {2, 5},
                    },
                },
                /* pipe attr*/
                {
                    /* nPipeId */
                    0,
                    /* nWidth */
                    1920,
                    /* nHeight */
                    1080,
                    /* eRawType */
                    AX_RT_RAW10,
                    /* ePixelFmt */
                    AX_FORMAT_BAYER_RAW_10BPP_PACKED,
                    {
                        /*fSrcFrameRate*/
                        15.0,
                        /*fDstFrameRate*/
                        15.0
                    }
                },
        },
        {
                /* attr */
                {
                    /* sensor obj */
                    &gSnssc200aiObj,
                    /* mSnsId */
                    1,
                    /* eSnsMode */
                    AX_SNS_LINEAR_MODE,
                    /*AX_SNS_MASTER_SLAVE_E*/
                    AX_SNS_SYNC_SLAVE,
                    /* mClkId */
                    0,
                    /* eClkRate */
                    AX_SNS_CLK_24M,
                    /* nI2cDev */
                    0,
                    /* nI2cAddr */
                    0x32,
                    /* nResetGpio */
                    97,
                    /* tInitAttr */
                    {
                        /* nFps */
                        60,
                        /* nWidth */
                        1920,
                        /* nHeight */
                        1080,
                        /* nSettingIndex */
                        0,
                    },
                    /* tConvergeAttr */
                    {
                        /* nFps */
                        15,
                        /* nWidth */
                        1920,
                        /* nHeight */
                        1080,
                        /* nSettingIndex */
                        0,
                    },
                },
                /* nDevId */
                1,
                /* mipi attr */
                {
                    /* nMipiId */
                    1,
                    /* eLaneMode */
                    AX_LANE_COMBO_MODE_1,
                    /* tRxAttr */
                    {
                        /* ePhyMode */
                        AX_MIPI_PHY_TYPE_DPHY,
                        /* eLaneNum */
                        AX_MIPI_DATA_LANE_2,
                        /* nDataRate */
                        1440,
                        /* nDataLaneMap[4] */
                        {0, 1, 3, 4},
                        /* nClkLane[2] */
                        {2, 5},
                    },
                },
                /* pipe attr*/
                {
                    /* nPipeId */
                    1,
                    /* nWidth */
                    1920,
                    /* nHeight */
                    1080,
                    /* eRawType */
                    AX_RT_RAW10,
                    /* ePixelFmt */
                    AX_FORMAT_BAYER_RAW_10BPP_PACKED,
                    {
                        /*fSrcFrameRate*/
                        15.0,
                        /*fDstFrameRate*/
                        15.0
                    }
                },
        },
    },
    {
        /*vsync0 gpio*/
        15,
        /*vsync2 gpio*/
        34,
        /*switch gpio*/
        32,
        /*switch pin mode*/
        MIPI_SWITCH_STAY_HIGH,
        /*vsync pin type*/
        MIPI_SWITCH_FSYNC_VSYNC,
    },
};

#endif //_AX_SENSOR_SC200AI_CONFIG_H_
