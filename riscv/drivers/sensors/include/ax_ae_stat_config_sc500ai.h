/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_AE_STAT_SC500AI_CONFIG_H_
#define _AX_AE_STAT_SC500AI_CONFIG_H_

#include "ax_vin_struct.h"


const static AX_AE_STAT_CONFIG_T sc500ai_ae_param = {
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
            .tGridRoi = { /* 720*404 */
                /* grid0 */
                {
                    .nRoiOffsetH = 0,
                    .nRoiOffsetV = 2,
                    .nRoiRegionNumH = 18,
                    .nRoiRegionNumV = 10,
                    .nRoiRegionW = 40,
                    .nRoiRegionH = 40,
                },
                /* grid1 */
                {
                    .nRoiOffsetH = 0,
                    .nRoiOffsetV = 2,
                    .nRoiRegionNumH = 1,
                    .nRoiRegionNumV = 10,
                    .nRoiRegionW = 720,
                    .nRoiRegionH = 40,
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
                .nRoiOffsetH = 8,
                .nRoiOffsetV = 10,
                .nRoiWidth = 44,
                .nRoiHeight = 24,
            },
        },
        {
            {0},
        },
    }
};

#endif // _AX_AE_STAT_SC200AI_CONFIG_H_
