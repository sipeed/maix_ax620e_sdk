/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_AE_STAT_SC235HAI_CONFIG_H_
#define _AX_AE_STAT_SC235HAI_CONFIG_H_

#include "ax_vin_struct.h"


const static AX_AE_STAT_CONFIG_T sc235hai_ae_param = {
    {
        {
            .nEnable = {1, 0},
            .nIspGainEnable = 1,
            .nSkipNum = 0,
            .tAeStatPos = {
                .tPreHdrPos = {
                    .eAE0StatPos = AX_ISP_AE0_STAT_IFE_PREHDR,
                },
            },
            .nGridMode = {1, 1},
            .nGridYcoeff = {1024, 1024, 1024, 1024},
            .nHistMode = 2,
            .nHistLinearBinNum = 0,
            .nHistYcoeff = {1024, 1024, 1024, 1024},
            .nHistWeight = {
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            },
            .tGridRoi = {
                /* grid0 */
                {
                    .nRoiOffsetH = 12,
                    .nRoiOffsetV = 0,
                    .nRoiRegionNumH = 18,
                    .nRoiRegionNumV = 10,
                    .nRoiRegionW = 52,
                    .nRoiRegionH = 54,
                },
                /* grid1 */
                {
                    .nRoiOffsetH = 0,
                    .nRoiOffsetV = 0,
                    .nRoiRegionNumH = 1,
                    .nRoiRegionNumV = 10,
                    .nRoiRegionW = 960,
                    .nRoiRegionH = 54,
                },
            },
            .tSatThr = {
                {
                    .nRThr = 1048575,
                    .nBThr = 1048575,
                    .nGrThr = 1048575,
                    .nGbThr = 1048575
                },
                {
                    .nRThr = 1048575,
                    .nBThr = 1048575,
                    .nGrThr = 1048575,
                    .nGbThr = 1048575
                },
            },
            .tHistRoi = {
                .nRoiOffsetH = 0,
                .nRoiOffsetV = 14,
                .nRoiWidth = 60,
                .nRoiHeight = 32,
            },
        },
        {
            {0},
        },
    }
};

#endif
