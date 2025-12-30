#ifndef __SC500AI_SDR_H__
#define __SC500AI_SDR_H__

/********************************************************************
* Store the default parameters of the sdr mode
* warn: user need to add 'static' when defining global variables.
        Limit the scope of the variable to this sensor
*********************************************************************/

#include "ax_isp_iq_api.h"
#include "ax_isp_api.h"

const static AX_ISP_IQ_AE_PARAM_T ae_param_sdr = {
    /* nEnable */
    1,
    /* tExpManual */
    {
        /* nIspGain */
        1024,
        /* nAGain */
        1024,
        /* nDgain */
        1024,
        /* nHcgLcg */
        2,
        /* nSnsTotalAGain */
        1024,
        /* nSysTotalGain */
        1024,
        /* nShutter */
        10000,
        /* nIrisPwmDuty */
        0,
        /* nHdrRealRatioLtoS */
        1024,
        /* nHdrRealRatioStoVS */
        1024,
        /* nSetPoint */
        40960,
        /* nShortAgain */
        0,
        /* nShortDgain */
        0,
        /* nShortShutter */
        0,
        /* nVsAgain */
        0,
        /* nVsDgain */
        0,
        /* nVsShutter */
        0,
        /* nHdrRatio */
        1024,
        /* nHdrMaxShutterHwLimit */
        39898,
        /* nRealMaxShutter */
        39888,
    },
    /* tAeAlgAuto */
    {
        /* nSetPoint */
        40960,
        /* nFaceTarget */
        35840,
        /* nTolerance */
        10485760,
        /* nAgainLcg2HcgTh */
        6451,
        /* nAgainHcg2LcgTh */
        1229,
        /* nAgainLcg2HcgRatio */
        1024,
        /* nAgainHcg2LcgRatio */
        1024,
        /* nLuxk */
        72091,
        /* nCompensationMode */
        1,
        /* nMaxIspGain */
        43008,
        /* nMinIspGain */
        1024,
        /* nMaxUserDgain */
        1024,
        /* nMinUserDgain */
        1024,
        /* nMaxUserTotalAgain */
        24645,
        /* nMinUserTotalAgain */
        1024,
        /* nMaxUserSysGain */
        1048576,
        /* nMinUserSysGain */
        1024,
        /* nMaxShutter */
        39888,
        /* nMinShutter */
        36,
        /* nPositionWeightMode */
        1,
        /* nRoiStartX */
        0,
        /* nRoiStartY */
        0,
        /* nRoiWidth */
        0,
        /* nRoiHeight */
        0,
        /* nWeightRoi */
        1024,
        /* nWeightBackgnd */
        1024,
        /* nGridWeightRow */
        15,
        /* nGridWeightColumn */
        17,
        /* nGridWeightTable[54][72] */
        {
            {139, 175, 215, 255, 293, 327, 354, 371, 377, 371, 354, 327, 293, 255, 215, 175, 139, /*0 - 16*/},
            {181, 228, 280, 332, 383, 427, 461, 484, 491, 484, 461, 427, 383, 332, 280, 228, 181, /*0 - 16*/},
            {226, 286, 350, 416, 479, 534, 578, 605, 615, 605, 578, 534, 479, 416, 350, 286, 226, /*0 - 16*/},
            {272, 344, 421, 500, 575, 642, 694, 727, 739, 727, 694, 642, 575, 500, 421, 344, 272, /*0 - 16*/},
            {313, 396, 486, 577, 664, 740, 801, 839, 852, 839, 801, 740, 664, 577, 486, 396, 313, /*0 - 16*/},
            {347, 439, 538, 639, 735, 820, 887, 929, 944, 929, 887, 820, 735, 639, 538, 439, 347, /*0 - 16*/},
            {369, 467, 572, 679, 781, 872, 943, 988, 1003, 988, 943, 872, 781, 679, 572, 467, 369, /*0 - 16*/},
            {377, 476, 583, 693, 797, 890, 962, 1008, 1024, 1008, 962, 890, 797, 693, 583, 476, 377, /*0 - 16*/},
            {369, 467, 572, 679, 781, 872, 943, 988, 1003, 988, 943, 872, 781, 679, 572, 467, 369, /*0 - 16*/},
            {347, 439, 538, 639, 735, 820, 887, 929, 944, 929, 887, 820, 735, 639, 538, 439, 347, /*0 - 16*/},
            {313, 396, 486, 577, 664, 740, 801, 839, 852, 839, 801, 740, 664, 577, 486, 396, 313, /*0 - 16*/},
            {272, 344, 421, 500, 575, 642, 694, 727, 739, 727, 694, 642, 575, 500, 421, 344, 272, /*0 - 16*/},
            {226, 286, 350, 416, 479, 534, 578, 605, 615, 605, 578, 534, 479, 416, 350, 286, 226, /*0 - 16*/},
            {181, 228, 280, 332, 383, 427, 461, 484, 491, 484, 461, 427, 383, 332, 280, 228, 181, /*0 - 16*/},
            {139, 175, 215, 255, 293, 327, 354, 371, 377, 371, 354, 327, 293, 255, 215, 175, 139, /*0 - 16*/},
        },
        /* tAntiFlickerParam */
        {
            /* nEnable */
            1,
            /* nFlickerPeriod */
            0,
            /* nAntiFlickerTolerance */
            150,
            /* nOverExpMode */
            1,
            /* nUnderExpMode */
            1,
        },
        /* nSetPointMode */
        2,
        /* nFaceTargetMode */
        0,
        /* nStrategyMode */
        2,
        /* tAeRouteParam */
        {
            /* nTableNum */
            1,
            /* nUsedTableId */
            0,
            /* tRouteTable[8] */
            {
                /* 0 */
                {
                    /* sTableName[32] */
                    "DefaultAeRoute",
                    /* nRouteCurveNum */
                    3,
                    /* tRouteCurveList[16] */
                    {
                        /* 0 */
                        {
                            /* nIntergrationTime */
                            40,
                            /* nGain */
                            1024,
                            /* nIncrementPriority */
                            0,
                        },
                        /* 1 */
                        {
                            /* nIntergrationTime */
                            30000,
                            /* nGain */
                            2048,
                            /* nIncrementPriority */
                            0,
                        },
                        /* 2 */
                        {
                            /* nIntergrationTime */
                            39888,
                            /* nGain */
                            517120,
                            /* nIncrementPriority */
                            0,
                        },
                    },
                },
            },
        },
        /* tAeSetPointCurve */
        {
            /* nSize */
            5,
            /* nRefList[10] */
            {512, 5120, 102400, 512000, 2048000, /*0 - 4*/},
            /* nSetPointList[10] */
            {34816, 37888, 40960, 44032, 47104, /*0 - 4*/},
        },
        /* tFaceUIParam */
        {
            /* nEnable */
            0,
            /* tFaceTargetCurve */
            {
                /* nSize */
                6,
                /* nRefList[10] */
                {102, 1536, 10240, 51200, 307200, 2048000, /*0 - 5*/},
                /* nFaceTargetList[10] */
                {18432, 18432, 18432, 22528, 25600, 25600, /*0 - 5*/},
            },
            /* nFaceScoreList[6] */
            {0, 205, 410, 614, 819, 1024, /*0 - 5*/},
            /* nFaceScoreWeightList[6] */
            {0, 205, 410, 614, 819, 1024, /*0 - 5*/},
            /* nFaceDistanceList[6] */
            {0, 205, 410, 614, 819, 1024, /*0 - 5*/},
            /* nFaceDistanceWeightList[6] */
            {1024, 922, 768, 563, 307, 0, /*0 - 5*/},
            /* nFaceWeight */
            512,
            /* nFaceTargetWeight */
            512,
            /* nNoFaceFrameTh */
            20,
            /* nToNormalFrameTh */
            8,
            /* nWithFaceFrameTh */
            3,
            /* nToFaceAeFrameTh */
            8,
        },
        /* tAeHdrRatio */
        {
            /* nHdrMode */
            0,
            /* tRatioStrategyParam */
            {
                /* nMinRatio */
                5120,
                /* nMaxRatio */
                16384,
                /* nShortNonSatAreaPercent */
                103284736,
                /* nShortSatLuma */
                153600,
                /* nTolerance */
                1048576,
                /* nConvergeCntFrameNum */
                3,
                /* nDampRatio */
                870,
            },
            /* nFixedHdrRatio */
            1024,
        },
        /* nMultiCamSyncMode */
        0,
        /* nMultiCamSyncRatio */
        1048576,
        /* tSlowShutterParam */
        {
            /* nFrameRateMode */
            0,
            /* nFpsIncreaseDelayFrame */
            0,
        },
        /* tIrisParam */
        {
            /* nIrisType */
            0,
            /* tDcIrisParam */
            {
                /* nBigStepFactor */
                104858,
                /* nSmallStepFactor */
                10486,
                /* nLumaDiffOverThresh */
                35840,
                /* nLumaDiffUnderThresh */
                35840,
                /* nLumaSpeedThresh */
                205,
                /* nSpeedDownFactor */
                209715,
                /* nMinUserPwmDuty */
                30720,
                /* nMaxUserPwmDuty */
                66560,
                /* nOpenPwmDuty */
                61440,
                /* nConvergeLumaDiffTolerance */
                52429,
                /* nConvergeFrameCntThresh */
                10,
            },
        },
        /* tLumaWeightParam */
        {
            /* nEnable */
            0,
            /* nLumaWeightNum */
            16,
            /* nLumaSplitList[64] */
            {0, 16384, 32768, 49152, 65535, 81920, 98304, 114688, 131072, 147456, 163840, 180224, 196608, 212992, 229376, 245760, /*0 - 15*/},
            /* nWeightList[64] */
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 15*/},
        },
        /* tTimeSmoothParam */
        {
            /* tStateMachineParam */
            {
                /* nToFastLumaThOver */
                1536,
                /* nToFastLumaThUnder */
                819,
                /* nToSlowFrameTh */
                8,
                /* nToConvergedFrameTh */
                2,
            },
            /* tConvergeSpeedParam */
            {
                /* nFastOverKneeCnt */
                14,
                /* nFastOverLumaDiffList[16] */
                {5120, 10240, 15360, 20480, 25600, 30720, 40960, 51200, 71680, 92160, 112640, 153600, 209920, 262144, /*0 - 13*/},
                /* nFastOverStepFactorList[16] */
                {102, 154, 184, 184, 205, 205, 205, 205, 256, 307, 358, 410, 512, 614, /*0 - 13*/},
                /* nFastOverSpeedDownFactorList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 102, 102, 307, /*0 - 13*/},
                /* nFastOverSkipList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 13*/},
                /* nFastUnderKneeCnt */
                11,
                /* nFastUnderLumaDiffList[16] */
                {5120, 10240, 15360, 20480, 25600, 30720, 35840, 40960, 51200, 153600, 262144, /*0 - 10*/},
                /* nFastUnderStepFactorList[16] */
                {102, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, /*0 - 10*/},
                /* nFastUnderSpeedDownFactorList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 10*/},
                /* nFastUnderSkipList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 10*/},
                /* nSlowOverKneeCnt */
                14,
                /* nSlowOverLumaDiffList[16] */
                {5120, 10240, 15360, 20480, 25600, 30720, 40960, 51200, 81920, 92160, 112640, 153600, 209920, 262144, /*0 - 13*/},
                /* nSlowOverStepFactorList[16] */
                {82, 102, 123, 154, 154, 174, 184, 184, 184, 184, 184, 184, 184, 184, /*0 - 13*/},
                /* nSlowOverSpeedDownFactorList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 13*/},
                /* nSlowOverSkipList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 13*/},
                /* nSlowUnderKneeCnt */
                11,
                /* nSlowUnderLumaDiffList[16] */
                {5120, 10240, 15360, 20480, 25600, 30720, 35840, 40960, 51200, 153600, 262144, /*0 - 10*/},
                /* nSlowUnderStepFactorList[16] */
                {123, 154, 154, 154, 154, 154, 154, 154, 154, 154, 154, /*0 - 10*/},
                /* nSlowUnderSpeedDownFactorList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 10*/},
                /* nSlowUnderSkipList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 10*/},
            },
        },
        /* tSleepWakeUpParam */
        {
            /* nNoiseLevel */
            41,
            /* nLinearLumaTh */
            51,
            /* nAeStatsDelayFrame */
            3,
            /* tSleepSetting */
            {
                /* nAGain */
                1024,
                /* nDGain */
                1024,
                /* nIspGain */
                1024,
                /* nHcgLcg */
                1,
                /* nShutter */
                1000,
            },
            /* nOverExpCompLumaTh */
            102400,
            /* tOverExpCompLut */
            {
                /* nLutNum */
                5,
                /* nLumaSplitList[8] */
                {179200, 220160, 225280, 240640, 244736, /*0 - 4*/},
                /* nCompFactorList[8] */
                {1024, 1208, 1352, 2324, 2560, /*0 - 4*/},
            },
        },
        /* tHistPointCtrlParam */
        {
            /* nEnable */
            0,
            /* nHistPointLutNum */
            1,
            /* tHistPointCtrlLut[10] */
            {
                /* 0 */
                {
                    /* nLuxStart */
                    102400,
                    /* nLuxEnd */
                    204800,
                    /* tHistPointTh */
                    {
                        /* nLumaThList[2] */
                        {0, 0, /*0 - 1*/},
                        /* nPercentThList[2] */
                        {102400, 102400, /*0 - 1*/},
                    },
                },
            },
        },
    },
    /* nLogLevel */
    4,
    /* nLogTarget */
    2,
};

const static AX_ISP_IQ_AWB_PARAM_T awb_param_sdr = {
    /* nEnable */
    1,
    /* tManualParam */
    {
        /* nManualMode */
        0,
        /* tGain */
        {
            /* nGainR */
            530,
            /* nGainGr */
            256,
            /* nGainGb */
            256,
            /* nGainB */
            418,
        },
        /* tManualLightSource */
        {
            /* nLightSourceIndex */
            0,
            /* tLightSource[15] */
            {
                /* 0 */
                {
                    /* szName[32] */
                    "Shade",
                    /* nColorTemperature */
                    7500,
                    /* nGreenShift */
                    0,
                },
                /* 1 */
                {
                    /* szName[32] */
                    "Day",
                    /* nColorTemperature */
                    6500,
                    /* nGreenShift */
                    0,
                },
                /* 2 */
                {
                    /* szName[32] */
                    "Cloudy",
                    /* nColorTemperature */
                    5000,
                    /* nGreenShift */
                    0,
                },
                /* 3 */
                {
                    /* szName[32] */
                    "Flourescent",
                    /* nColorTemperature */
                    5000,
                    /* nGreenShift */
                    0,
                },
                /* 4 */
                {
                    /* szName[32] */
                    "Sunset",
                    /* nColorTemperature */
                    3500,
                    /* nGreenShift */
                    0,
                },
                /* 5 */
                {
                    /* szName[32] */
                    "Incandescent",
                    /* nColorTemperature */
                    2800,
                    /* nGreenShift */
                    0,
                },
                /* 6 */
                {
                    /* szName[32] */
                    "Candle",
                    /* nColorTemperature */
                    2000,
                    /* nGreenShift */
                    0,
                },
                /* 7 */
                {
                    /* szName[32] */
                    "Flash",
                    /* nColorTemperature */
                    3500,
                    /* nGreenShift */
                    0,
                },
                /* 8 */
                {
                    /* szName[32] */
                    "UserDefined-1",
                    /* nColorTemperature */
                    2800,
                    /* nGreenShift */
                    0,
                },
                /* 9 */
                {
                    /* szName[32] */
                    "UserDefined-2",
                    /* nColorTemperature */
                    2000,
                    /* nGreenShift */
                    0,
                },
                /* 10 */
                {
                    /* szName[32] */
                    "UserDefined-3",
                    /* nColorTemperature */
                    5000,
                    /* nGreenShift */
                    0,
                },
                /* 11 */
                {
                    /* szName[32] */
                    "UserDefined-4",
                    /* nColorTemperature */
                    5000,
                    /* nGreenShift */
                    0,
                },
                /* 12 */
                {
                    /* szName[32] */
                    "UserDefined-5",
                    /* nColorTemperature */
                    5000,
                    /* nGreenShift */
                    0,
                },
                /* 13 */
                {
                    /* szName[32] */
                    "UserDefined-6",
                    /* nColorTemperature */
                    5000,
                    /* nGreenShift */
                    0,
                },
                /* 14 */
                {
                    /* szName[32] */
                    "UserDefined-7",
                    /* nColorTemperature */
                    5000,
                    /* nGreenShift */
                    0,
                },
            },
        },
    },
    /* tAutoParam */
    {
        /* tCenterPnt */
        {
            /* nRg */
            1412364,
            /* nBg */
            1440118,
        },
        /* nCenterPntRadius */
        1232978,
        /* nLowCut */
        0,
        /* nHighCut */
        0,
        /* nCctMax */
        12000,
        /* nCctMin */
        1800,
        /* nPartCtrlPntNum */
        8,
        /* nCtrlPntNum */
        41,
        /* nCtrlSegKbNum */
        40,
        /* nCctList[512] */
        {
            1800, 1862, 1925, 1987, 2050, 2112, 2175, 2237, 2300, 2362, 2425, 2487, 2550, 2612, 2675, 2737, 2800, 2925, 3050, 3175, 3300, 3425, 3550, 3675, 3800, 4137, 4475, 4812, 5150, 5487, 5825, 6162,  /* 0 - 31*/
            6500, 7187, 7875, 8562, 9250, 9937, 10625, 11312, 12000, /*32 - 40*/
        },
        /* tChordKB */
        {
            /* nK */
            -715,
            /* nB */
            1339,
        },
        /* tChordPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1534696,
                /* nBg */
                297554,
            },
            /* 1 */
            {
                /* nRg */
                1470198,
                /* nBg */
                342664,
            },
            /* 2 */
            {
                /* nRg */
                1410922,
                /* nBg */
                384125,
            },
            /* 3 */
            {
                /* nRg */
                1355956,
                /* nBg */
                422566,
            },
            /* 4 */
            {
                /* nRg */
                1304565,
                /* nBg */
                458511,
            },
            /* 5 */
            {
                /* nRg */
                1256110,
                /* nBg */
                492401,
            },
            /* 6 */
            {
                /* nRg */
                1210120,
                /* nBg */
                524571,
            },
            /* 7 */
            {
                /* nRg */
                1166132,
                /* nBg */
                555326,
            },
            /* 8 */
            {
                /* nRg */
                1123801,
                /* nBg */
                584938,
            },
            /* 9 */
            {
                /* nRg */
                1106573,
                /* nBg */
                596996,
            },
            /* 10 */
            {
                /* nRg */
                1089544,
                /* nBg */
                608898,
            },
            /* 11 */
            {
                /* nRg */
                1072704,
                /* nBg */
                620684,
            },
            /* 12 */
            {
                /* nRg */
                1056031,
                /* nBg */
                632344,
            },
            /* 13 */
            {
                /* nRg */
                1039506,
                /* nBg */
                643899,
            },
            /* 14 */
            {
                /* nRg */
                1023106,
                /* nBg */
                655370,
            },
            /* 15 */
            {
                /* nRg */
                1006822,
                /* nBg */
                666758,
            },
            /* 16 */
            {
                /* nRg */
                990632,
                /* nBg */
                678083,
            },
            /* 17 */
            {
                /* nRg */
                974452,
                /* nBg */
                689397,
            },
            /* 18 */
            {
                /* nRg */
                958346,
                /* nBg */
                700658,
            },
            /* 19 */
            {
                /* nRg */
                942271,
                /* nBg */
                711899,
            },
            /* 20 */
            {
                /* nRg */
                926228,
                /* nBg */
                723119,
            },
            /* 21 */
            {
                /* nRg */
                910195,
                /* nBg */
                734339,
            },
            /* 22 */
            {
                /* nRg */
                894163,
                /* nBg */
                745559,
            },
            /* 23 */
            {
                /* nRg */
                878099,
                /* nBg */
                756789,
            },
            /* 24 */
            {
                /* nRg */
                862003,
                /* nBg */
                768050,
            },
            /* 25 */
            {
                /* nRg */
                840906,
                /* nBg */
                782804,
            },
            /* 26 */
            {
                /* nRg */
                819682,
                /* nBg */
                797641,
            },
            /* 27 */
            {
                /* nRg */
                798281,
                /* nBg */
                812615,
            },
            /* 28 */
            {
                /* nRg */
                776680,
                /* nBg */
                827725,
            },
            /* 29 */
            {
                /* nRg */
                754828,
                /* nBg */
                843003,
            },
            /* 30 */
            {
                /* nRg */
                732672,
                /* nBg */
                858501,
            },
            /* 31 */
            {
                /* nRg */
                710180,
                /* nBg */
                874229,
            },
            /* 32 */
            {
                /* nRg */
                687300,
                /* nBg */
                890231,
            },
            /* 33 */
            {
                /* nRg */
                639285,
                /* nBg */
                923816,
            },
            /* 34 */
            {
                /* nRg */
                588964,
                /* nBg */
                959017,
            },
            /* 35 */
            {
                /* nRg */
                535770,
                /* nBg */
                996221,
            },
            /* 36 */
            {
                /* nRg */
                479042,
                /* nBg */
                1035888,
            },
            /* 37 */
            {
                /* nRg */
                417952,
                /* nBg */
                1078618,
            },
            /* 38 */
            {
                /* nRg */
                351493,
                /* nBg */
                1125101,
            },
            /* 39 */
            {
                /* nRg */
                278355,
                /* nBg */
                1176261,
            },
            /* 40 */
            {
                /* nRg */
                196860,
                /* nBg */
                1233251,
            },
        },
        /* tArcPointList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1245876,
                /* nBg */
                303154,
            },
            /* 1 */
            {
                /* nRg */
                1219462,
                /* nBg */
                302011,
            },
            /* 2 */
            {
                /* nRg */
                1192912,
                /* nBg */
                301497,
            },
            /* 3 */
            {
                /* nRg */
                1166237,
                /* nBg */
                301602,
            },
            /* 4 */
            {
                /* nRg */
                1139456,
                /* nBg */
                302325,
            },
            /* 5 */
            {
                /* nRg */
                1112581,
                /* nBg */
                303689,
            },
            /* 6 */
            {
                /* nRg */
                1085633,
                /* nBg */
                305681,
            },
            /* 7 */
            {
                /* nRg */
                1058621,
                /* nBg */
                308323,
            },
            /* 8 */
            {
                /* nRg */
                1031568,
                /* nBg */
                311595,
            },
            /* 9 */
            {
                /* nRg */
                1004483,
                /* nBg */
                315527,
            },
            /* 10 */
            {
                /* nRg */
                977378,
                /* nBg */
                320099,
            },
            /* 11 */
            {
                /* nRg */
                950282,
                /* nBg */
                325331,
            },
            /* 12 */
            {
                /* nRg */
                923198,
                /* nBg */
                331214,
            },
            /* 13 */
            {
                /* nRg */
                896145,
                /* nBg */
                337757,
            },
            /* 14 */
            {
                /* nRg */
                869133,
                /* nBg */
                344950,
            },
            /* 15 */
            {
                /* nRg */
                842195,
                /* nBg */
                352814,
            },
            /* 16 */
            {
                /* nRg */
                815331,
                /* nBg */
                361329,
            },
            /* 17 */
            {
                /* nRg */
                791119,
                /* nBg */
                375086,
            },
            /* 18 */
            {
                /* nRg */
                767222,
                /* nBg */
                389389,
            },
            /* 19 */
            {
                /* nRg */
                743661,
                /* nBg */
                404226,
            },
            /* 20 */
            {
                /* nRg */
                720435,
                /* nBg */
                419598,
            },
            /* 21 */
            {
                /* nRg */
                697565,
                /* nBg */
                435484,
            },
            /* 22 */
            {
                /* nRg */
                675052,
                /* nBg */
                451884,
            },
            /* 23 */
            {
                /* nRg */
                652917,
                /* nBg */
                468787,
            },
            /* 24 */
            {
                /* nRg */
                631180,
                /* nBg */
                486183,
            },
            /* 25 */
            {
                /* nRg */
                603403,
                /* nBg */
                509618,
            },
            /* 26 */
            {
                /* nRg */
                576339,
                /* nBg */
                533861,
            },
            /* 27 */
            {
                /* nRg */
                549999,
                /* nBg */
                558891,
            },
            /* 28 */
            {
                /* nRg */
                524403,
                /* nBg */
                584686,
            },
            /* 29 */
            {
                /* nRg */
                499584,
                /* nBg */
                611225,
            },
            /* 30 */
            {
                /* nRg */
                475550,
                /* nBg */
                638478,
            },
            /* 31 */
            {
                /* nRg */
                452335,
                /* nBg */
                666433,
            },
            /* 32 */
            {
                /* nRg */
                429958,
                /* nBg */
                695059,
            },
            /* 33 */
            {
                /* nRg */
                387019,
                /* nBg */
                755342,
            },
            /* 34 */
            {
                /* nRg */
                347781,
                /* nBg */
                818099,
            },
            /* 35 */
            {
                /* nRg */
                312381,
                /* nBg */
                883090,
            },
            /* 36 */
            {
                /* nRg */
                280945,
                /* nBg */
                950094,
            },
            /* 37 */
            {
                /* nRg */
                253577,
                /* nBg */
                1018859,
            },
            /* 38 */
            {
                /* nRg */
                230393,
                /* nBg */
                1089145,
            },
            /* 39 */
            {
                /* nRg */
                211466,
                /* nBg */
                1160700,
            },
            /* 40 */
            {
                /* nRg */
                196860,
                /* nBg */
                1233251,
            },
        },
        /* tRadiusLineList[512] */
        {
            /* 0 */
            {
                /* nK */
                -9563,
                /* nB */
                14289,
            },
            /* 1 */
            {
                /* nK */
                -19431,
                /* nB */
                27579,
            },
            /* 2 */
            {
                /* nK */
                751402,
                /* nB */
                -1010682,
            },
            /* 3 */
            {
                /* nK */
                18473,
                /* nB */
                -23475,
            },
            /* 4 */
            {
                /* nK */
                9324,
                /* nB */
                -11151,
            },
            /* 5 */
            {
                /* nK */
                6211,
                /* nB */
                -6958,
            },
            /* 6 */
            {
                /* nK */
                4636,
                /* nB */
                -4836,
            },
            /* 7 */
            {
                /* nK */
                3680,
                /* nB */
                -3549,
            },
            /* 8 */
            {
                /* nK */
                3035,
                /* nB */
                -2680,
            },
            /* 9 */
            {
                /* nK */
                2823,
                /* nB */
                -2395,
            },
            /* 10 */
            {
                /* nK */
                2637,
                /* nB */
                -2144,
            },
            /* 11 */
            {
                /* nK */
                2470,
                /* nB */
                -1920,
            },
            /* 12 */
            {
                /* nK */
                2321,
                /* nB */
                -1719,
            },
            /* 13 */
            {
                /* nK */
                2187,
                /* nB */
                -1538,
            },
            /* 14 */
            {
                /* nK */
                2064,
                /* nB */
                -1373,
            },
            /* 15 */
            {
                /* nK */
                1953,
                /* nB */
                -1223,
            },
            /* 16 */
            {
                /* nK */
                1850,
                /* nB */
                -1085,
            },
            /* 17 */
            {
                /* nK */
                1755,
                /* nB */
                -957,
            },
            /* 18 */
            {
                /* nK */
                1668,
                /* nB */
                -839,
            },
            /* 19 */
            {
                /* nK */
                1586,
                /* nB */
                -729,
            },
            /* 20 */
            {
                /* nK */
                1510,
                /* nB */
                -627,
            },
            /* 21 */
            {
                /* nK */
                1439,
                /* nB */
                -531,
            },
            /* 22 */
            {
                /* nK */
                1372,
                /* nB */
                -441,
            },
            /* 23 */
            {
                /* nK */
                1310,
                /* nB */
                -357,
            },
            /* 24 */
            {
                /* nK */
                1250,
                /* nB */
                -277,
            },
            /* 25 */
            {
                /* nK */
                1178,
                /* nB */
                -179,
            },
            /* 26 */
            {
                /* nK */
                1110,
                /* nB */
                -88,
            },
            /* 27 */
            {
                /* nK */
                1046,
                /* nB */
                -2,
            },
            /* 28 */
            {
                /* nK */
                986,
                /* nB */
                78,
            },
            /* 29 */
            {
                /* nK */
                930,
                /* nB */
                154,
            },
            /* 30 */
            {
                /* nK */
                876,
                /* nB */
                226,
            },
            /* 31 */
            {
                /* nK */
                825,
                /* nB */
                295,
            },
            /* 32 */
            {
                /* nK */
                777,
                /* nB */
                360,
            },
            /* 33 */
            {
                /* nK */
                684,
                /* nB */
                485,
            },
            /* 34 */
            {
                /* nK */
                598,
                /* nB */
                600,
            },
            /* 35 */
            {
                /* nK */
                519,
                /* nB */
                708,
            },
            /* 36 */
            {
                /* nK */
                443,
                /* nB */
                809,
            },
            /* 37 */
            {
                /* nK */
                372,
                /* nB */
                905,
            },
            /* 38 */
            {
                /* nK */
                304,
                /* nB */
                997,
            },
            /* 39 */
            {
                /* nK */
                238,
                /* nB */
                1085,
            },
            /* 40 */
            {
                /* nK */
                174,
                /* nB */
                1172,
            },
        },
        /* tInLeftBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1232213,
                /* nBg */
                209778,
            },
            /* 1 */
            {
                /* nRg */
                1203692,
                /* nBg */
                208971,
            },
            /* 2 */
            {
                /* nRg */
                1175045,
                /* nBg */
                208834,
            },
            /* 3 */
            {
                /* nRg */
                1146293,
                /* nBg */
                209359,
            },
            /* 4 */
            {
                /* nRg */
                1117436,
                /* nBg */
                210554,
            },
            /* 5 */
            {
                /* nRg */
                1088506,
                /* nBg */
                212441,
            },
            /* 6 */
            {
                /* nRg */
                1059513,
                /* nBg */
                215000,
            },
            /* 7 */
            {
                /* nRg */
                1030467,
                /* nBg */
                218240,
            },
            /* 8 */
            {
                /* nRg */
                1001390,
                /* nBg */
                222183,
            },
            /* 9 */
            {
                /* nRg */
                972303,
                /* nBg */
                226807,
            },
            /* 10 */
            {
                /* nRg */
                943215,
                /* nBg */
                232134,
            },
            /* 11 */
            {
                /* nRg */
                914138,
                /* nBg */
                238153,
            },
            /* 12 */
            {
                /* nRg */
                885103,
                /* nBg */
                244863,
            },
            /* 13 */
            {
                /* nRg */
                856120,
                /* nBg */
                252287,
            },
            /* 14 */
            {
                /* nRg */
                827201,
                /* nBg */
                260414,
            },
            /* 15 */
            {
                /* nRg */
                798365,
                /* nBg */
                269232,
            },
            /* 16 */
            {
                /* nRg */
                769634,
                /* nBg */
                278764,
            },
            /* 17 */
            {
                /* nRg */
                743566,
                /* nBg */
                293570,
            },
            /* 18 */
            {
                /* nRg */
                717845,
                /* nBg */
                308973,
            },
            /* 19 */
            {
                /* nRg */
                692480,
                /* nBg */
                324943,
            },
            /* 20 */
            {
                /* nRg */
                667471,
                /* nBg */
                341490,
            },
            /* 21 */
            {
                /* nRg */
                642850,
                /* nBg */
                358592,
            },
            /* 22 */
            {
                /* nRg */
                618618,
                /* nBg */
                376240,
            },
            /* 23 */
            {
                /* nRg */
                594794,
                /* nBg */
                394443,
            },
            /* 24 */
            {
                /* nRg */
                571390,
                /* nBg */
                413170,
            },
            /* 25 */
            {
                /* nRg */
                541495,
                /* nBg */
                438399,
            },
            /* 26 */
            {
                /* nRg */
                512355,
                /* nBg */
                464498,
            },
            /* 27 */
            {
                /* nRg */
                483991,
                /* nBg */
                491447,
            },
            /* 28 */
            {
                /* nRg */
                456445,
                /* nBg */
                519213,
            },
            /* 29 */
            {
                /* nRg */
                429717,
                /* nBg */
                547776,
            },
            /* 30 */
            {
                /* nRg */
                403849,
                /* nBg */
                577126,
            },
            /* 31 */
            {
                /* nRg */
                378861,
                /* nBg */
                607220,
            },
            /* 32 */
            {
                /* nRg */
                354765,
                /* nBg */
                638038,
            },
            /* 33 */
            {
                /* nRg */
                308543,
                /* nBg */
                702934,
            },
            /* 34 */
            {
                /* nRg */
                266307,
                /* nBg */
                770494,
            },
            /* 35 */
            {
                /* nRg */
                228191,
                /* nBg */
                840455,
            },
            /* 36 */
            {
                /* nRg */
                194343,
                /* nBg */
                912586,
            },
            /* 37 */
            {
                /* nRg */
                164889,
                /* nBg */
                986616,
            },
            /* 38 */
            {
                /* nRg */
                139922,
                /* nBg */
                1062281,
            },
            /* 39 */
            {
                /* nRg */
                119548,
                /* nBg */
                1139309,
            },
            /* 40 */
            {
                /* nRg */
                103830,
                /* nBg */
                1217418,
            },
        },
        /* tInRightBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1258040,
                /* nBg */
                386149,
            },
            /* 1 */
            {
                /* nRg */
                1233482,
                /* nBg */
                384723,
            },
            /* 2 */
            {
                /* nRg */
                1208788,
                /* nBg */
                383863,
            },
            /* 3 */
            {
                /* nRg */
                1183958,
                /* nBg */
                383590,
            },
            /* 4 */
            {
                /* nRg */
                1159023,
                /* nBg */
                383905,
            },
            /* 5 */
            {
                /* nRg */
                1133972,
                /* nBg */
                384796,
            },
            /* 6 */
            {
                /* nRg */
                1108848,
                /* nBg */
                386295,
            },
            /* 7 */
            {
                /* nRg */
                1083651,
                /* nBg */
                388382,
            },
            /* 8 */
            {
                /* nRg */
                1058391,
                /* nBg */
                391077,
            },
            /* 9 */
            {
                /* nRg */
                1033078,
                /* nBg */
                394380,
            },
            /* 10 */
            {
                /* nRg */
                1007744,
                /* nBg */
                398291,
            },
            /* 11 */
            {
                /* nRg */
                982400,
                /* nBg */
                402821,
            },
            /* 12 */
            {
                /* nRg */
                957046,
                /* nBg */
                407959,
            },
            /* 13 */
            {
                /* nRg */
                931712,
                /* nBg */
                413726,
            },
            /* 14 */
            {
                /* nRg */
                906410,
                /* nBg */
                420101,
            },
            /* 15 */
            {
                /* nRg */
                881150,
                /* nBg */
                427106,
            },
            /* 16 */
            {
                /* nRg */
                855953,
                /* nBg */
                434729,
            },
            /* 17 */
            {
                /* nRg */
                833387,
                /* nBg */
                447543,
            },
            /* 18 */
            {
                /* nRg */
                811115,
                /* nBg */
                460881,
            },
            /* 19 */
            {
                /* nRg */
                789148,
                /* nBg */
                474711,
            },
            /* 20 */
            {
                /* nRg */
                767505,
                /* nBg */
                489024,
            },
            /* 21 */
            {
                /* nRg */
                746188,
                /* nBg */
                503830,
            },
            /* 22 */
            {
                /* nRg */
                725216,
                /* nBg */
                519119,
            },
            /* 23 */
            {
                /* nRg */
                704591,
                /* nBg */
                534868,
            },
            /* 24 */
            {
                /* nRg */
                684322,
                /* nBg */
                551090,
            },
            /* 25 */
            {
                /* nRg */
                658443,
                /* nBg */
                572931,
            },
            /* 26 */
            {
                /* nRg */
                633214,
                /* nBg */
                595518,
            },
            /* 27 */
            {
                /* nRg */
                608667,
                /* nBg */
                618849,
            },
            /* 28 */
            {
                /* nRg */
                584822,
                /* nBg */
                642882,
            },
            /* 29 */
            {
                /* nRg */
                561680,
                /* nBg */
                667618,
            },
            /* 30 */
            {
                /* nRg */
                539293,
                /* nBg */
                693025,
            },
            /* 31 */
            {
                /* nRg */
                517651,
                /* nBg */
                719071,
            },
            /* 32 */
            {
                /* nRg */
                496794,
                /* nBg */
                745747,
            },
            /* 33 */
            {
                /* nRg */
                456781,
                /* nBg */
                801930,
            },
            /* 34 */
            {
                /* nRg */
                420217,
                /* nBg */
                860420,
            },
            /* 35 */
            {
                /* nRg */
                387218,
                /* nBg */
                920996,
            },
            /* 36 */
            {
                /* nRg */
                357921,
                /* nBg */
                983438,
            },
            /* 37 */
            {
                /* nRg */
                332420,
                /* nBg */
                1047517,
            },
            /* 38 */
            {
                /* nRg */
                310808,
                /* nBg */
                1113021,
            },
            /* 39 */
            {
                /* nRg */
                293171,
                /* nBg */
                1179711,
            },
            /* 40 */
            {
                /* nRg */
                279561,
                /* nBg */
                1247323,
            },
        },
        /* tOutLeftBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1223091,
                /* nBg */
                147524,
            },
            /* 1 */
            {
                /* nRg */
                1193175,
                /* nBg */
                146937,
            },
            /* 2 */
            {
                /* nRg */
                1163143,
                /* nBg */
                147052,
            },
            /* 3 */
            {
                /* nRg */
                1132997,
                /* nBg */
                147860,
            },
            /* 4 */
            {
                /* nRg */
                1102766,
                /* nBg */
                149380,
            },
            /* 5 */
            {
                /* nRg */
                1072463,
                /* nBg */
                151603,
            },
            /* 6 */
            {
                /* nRg */
                1042106,
                /* nBg */
                154539,
            },
            /* 7 */
            {
                /* nRg */
                1011698,
                /* nBg */
                158199,
            },
            /* 8 */
            {
                /* nRg */
                981278,
                /* nBg */
                162571,
            },
            /* 9 */
            {
                /* nRg */
                950849,
                /* nBg */
                167667,
            },
            /* 10 */
            {
                /* nRg */
                920440,
                /* nBg */
                173487,
            },
            /* 11 */
            {
                /* nRg */
                890052,
                /* nBg */
                180030,
            },
            /* 12 */
            {
                /* nRg */
                859706,
                /* nBg */
                187307,
            },
            /* 13 */
            {
                /* nRg */
                829434,
                /* nBg */
                195308,
            },
            /* 14 */
            {
                /* nRg */
                799246,
                /* nBg */
                204042,
            },
            /* 15 */
            {
                /* nRg */
                769151,
                /* nBg */
                213511,
            },
            /* 16 */
            {
                /* nRg */
                739173,
                /* nBg */
                223714,
            },
            /* 17 */
            {
                /* nRg */
                711868,
                /* nBg */
                239222,
            },
            /* 18 */
            {
                /* nRg */
                684930,
                /* nBg */
                255349,
            },
            /* 19 */
            {
                /* nRg */
                658348,
                /* nBg */
                272085,
            },
            /* 20 */
            {
                /* nRg */
                632165,
                /* nBg */
                289407,
            },
            /* 21 */
            {
                /* nRg */
                606371,
                /* nBg */
                307327,
            },
            /* 22 */
            {
                /* nRg */
                580995,
                /* nBg */
                325814,
            },
            /* 23 */
            {
                /* nRg */
                556039,
                /* nBg */
                344877,
            },
            /* 24 */
            {
                /* nRg */
                531523,
                /* nBg */
                364496,
            },
            /* 25 */
            {
                /* nRg */
                500213,
                /* nBg */
                390920,
            },
            /* 26 */
            {
                /* nRg */
                469689,
                /* nBg */
                418256,
            },
            /* 27 */
            {
                /* nRg */
                439993,
                /* nBg */
                446473,
            },
            /* 28 */
            {
                /* nRg */
                411126,
                /* nBg */
                475561,
            },
            /* 29 */
            {
                /* nRg */
                383139,
                /* nBg */
                505487,
            },
            /* 30 */
            {
                /* nRg */
                356044,
                /* nBg */
                536221,
            },
            /* 31 */
            {
                /* nRg */
                329872,
                /* nBg */
                567741,
            },
            /* 32 */
            {
                /* nRg */
                304632,
                /* nBg */
                600016,
            },
            /* 33 */
            {
                /* nRg */
                256230,
                /* nBg */
                667995,
            },
            /* 34 */
            {
                /* nRg */
                211980,
                /* nBg */
                738753,
            },
            /* 35 */
            {
                /* nRg */
                172061,
                /* nBg */
                812038,
            },
            /* 36 */
            {
                /* nRg */
                136608,
                /* nBg */
                887588,
            },
            /* 37 */
            {
                /* nRg */
                105759,
                /* nBg */
                965120,
            },
            /* 38 */
            {
                /* nRg */
                79618,
                /* nBg */
                1044371,
            },
            /* 39 */
            {
                /* nRg */
                58269,
                /* nBg */
                1125049,
            },
            /* 40 */
            {
                /* nRg */
                41807,
                /* nBg */
                1206859,
            },
        },
        /* tOutRightBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1267152,
                /* nBg */
                448403,
            },
            /* 1 */
            {
                /* nRg */
                1243989,
                /* nBg */
                446756,
            },
            /* 2 */
            {
                /* nRg */
                1220689,
                /* nBg */
                445645,
            },
            /* 3 */
            {
                /* nRg */
                1197254,
                /* nBg */
                445089,
            },
            /* 4 */
            {
                /* nRg */
                1173692,
                /* nBg */
                445079,
            },
            /* 5 */
            {
                /* nRg */
                1150026,
                /* nBg */
                445634,
            },
            /* 6 */
            {
                /* nRg */
                1126265,
                /* nBg */
                446746,
            },
            /* 7 */
            {
                /* nRg */
                1102420,
                /* nBg */
                448434,
            },
            /* 8 */
            {
                /* nRg */
                1078502,
                /* nBg */
                450688,
            },
            /* 9 */
            {
                /* nRg */
                1054532,
                /* nBg */
                453530,
            },
            /* 10 */
            {
                /* nRg */
                1030530,
                /* nBg */
                456938,
            },
            /* 11 */
            {
                /* nRg */
                1006486,
                /* nBg */
                460944,
            },
            /* 12 */
            {
                /* nRg */
                982442,
                /* nBg */
                465526,
            },
            /* 13 */
            {
                /* nRg */
                958398,
                /* nBg */
                470695,
            },
            /* 14 */
            {
                /* nRg */
                934365,
                /* nBg */
                476462,
            },
            /* 15 */
            {
                /* nRg */
                910363,
                /* nBg */
                482817,
            },
            /* 16 */
            {
                /* nRg */
                886414,
                /* nBg */
                489769,
            },
            /* 17 */
            {
                /* nRg */
                865086,
                /* nBg */
                501890,
            },
            /* 18 */
            {
                /* nRg */
                844030,
                /* nBg */
                514494,
            },
            /* 19 */
            {
                /* nRg */
                823279,
                /* nBg */
                527560,
            },
            /* 20 */
            {
                /* nRg */
                802811,
                /* nBg */
                541097,
            },
            /* 21 */
            {
                /* nRg */
                782668,
                /* nBg */
                555095,
            },
            /* 22 */
            {
                /* nRg */
                762839,
                /* nBg */
                569545,
            },
            /* 23 */
            {
                /* nRg */
                743346,
                /* nBg */
                584434,
            },
            /* 24 */
            {
                /* nRg */
                724189,
                /* nBg */
                599765,
            },
            /* 25 */
            {
                /* nRg */
                699725,
                /* nBg */
                620411,
            },
            /* 26 */
            {
                /* nRg */
                675881,
                /* nBg */
                641760,
            },
            /* 27 */
            {
                /* nRg */
                652676,
                /* nBg */
                663812,
            },
            /* 28 */
            {
                /* nRg */
                630131,
                /* nBg */
                686534,
            },
            /* 29 */
            {
                /* nRg */
                608258,
                /* nBg */
                709917,
            },
            /* 30 */
            {
                /* nRg */
                587087,
                /* nBg */
                733930,
            },
            /* 31 */
            {
                /* nRg */
                566640,
                /* nBg */
                758550,
            },
            /* 32 */
            {
                /* nRg */
                546927,
                /* nBg */
                783769,
            },
            /* 33 */
            {
                /* nRg */
                509105,
                /* nBg */
                836879,
            },
            /* 34 */
            {
                /* nRg */
                474533,
                /* nBg */
                892160,
            },
            /* 35 */
            {
                /* nRg */
                443348,
                /* nBg */
                949412,
            },
            /* 36 */
            {
                /* nRg */
                415656,
                /* nBg */
                1008437,
            },
            /* 37 */
            {
                /* nRg */
                391549,
                /* nBg */
                1069013,
            },
            /* 38 */
            {
                /* nRg */
                371123,
                /* nBg */
                1130931,
            },
            /* 39 */
            {
                /* nRg */
                354450,
                /* nBg */
                1193961,
            },
            /* 40 */
            {
                /* nRg */
                341584,
                /* nBg */
                1257882,
            },
        },
        /* nIllumNum */
        4,
        /* tIllumList[16] */
        {
            /* 0 */
            {
                /* szName[32] */
                "H",
                /* nCct */
                2300,
                /* nRadius */
                0,
                /* tCoord */
                {
                    /* nRg */
                    1026556,
                    /* nBg */
                    296747,
                },
            },
            /* 1 */
            {
                /* szName[32] */
                "A",
                /* nCct */
                2800,
                /* nRadius */
                0,
                /* tCoord */
                {
                    /* nRg */
                    817889,
                    /* nBg */
                    365953,
                },
            },
            /* 2 */
            {
                /* szName[32] */
                "TL84",
                /* nCct */
                3800,
                /* nRadius */
                0,
                /* tCoord */
                {
                    /* nRg */
                    594543,
                    /* nBg */
                    441450,
                },
            },
            /* 3 */
            {
                /* szName[32] */
                "D65",
                /* nCct */
                6500,
                /* nRadius */
                0,
                /* tCoord */
                {
                    /* nRg */
                    450888,
                    /* nBg */
                    710935,
                },
            },
        },
        /* nExtIllumNum */
        1,
        /* tExtIllumList[32] */
        {
            /* 0 */
            {
                /* szName[32] */
                "CWF",
                /* nCct */
                4100,
                /* nRadius */
                31457,
                /* tCoord */
                {
                    /* nRg */
                    532677,
                    /* nBg */
                    421528,
                },
            },
        },
        /* nPolyNum  */
        0,
        /* tPolyList[32] */
        {
            /* 0 */
            {
                /* nMinX */
                0,
                /* nMaxX */
                0,
                /* nMinY */
                0,
                /* nMaxY */
                0,
                /* nPntCnt */
                0,
                /* tPntArray[64] */
                {
                    /* 0 */
                    {
                        /* nRg */
                        0,
                        /* nBg */
                        0,
                    },
                },
            },
        },
        /* tInitParam */
        {
            /* tGains */
            {
                /* nGainR */
                374,
                /* nGainGr */
                256,
                /* nGainGb */
                256,
                /* nGainB */
                731,
            },
            /* nDampRatio */
            10000,
        },
        /* nMode */
        0,
        /* nIndex */
        0,
        /* nDampRatio */
        524288,
        /* nToleranceRg */
        3145,
        /* nToleranceBg */
        3145,
        /* nLuxVeryDarkStart */
        0,
        /* nLuxVeryDarkEnd */
        8192,
        /* nLuxDarkStart */
        10240,
        /* nLuxDarkEnd */
        51200,
        /* nLuxIndoorStart */
        61440,
        /* nLuxIndoorEnd */
        409600,
        /* nLuxTransInStart */
        460800,
        /* nLuxTransInEnd */
        972800,
        /* nLuxTransOutStart */
        1024000,
        /* nLuxTransOutEnd */
        1894400,
        /* nLuxOutdoorStart */
        1945600,
        /* nLuxOutdoorEnd */
        4044800,
        /* nLuxBrightStart */
        4096000,
        /* nLuxBrightEnd */
        10137600,
        /* nLuxVeryBrightStart */
        10240000,
        /* nCctMinInner */
        2300,
        /* nCctMaxInner */
        7800,
        /* nCctMinOuter */
        1950,
        /* nCctMaxOuter */
        10000,
        /* nCctSplitHtoA */
        2450,
        /* nCctSplitAtoF */
        3300,
        /* nCctSplitFtoD5 */
        4400,
        /* nCctSplitD5toD6 */
        5350,
        /* nCctSplitD6toS */
        6800,
        /* nGridWeightEnable */
        0,
        /* nGridWeightRow */
        9,
        /* nGridWeightColumn */
        9,
        /* nGridWeightTable[54][72] */
        {
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 8*/},
        },
        /* nGrayZoneLuxWeight[24][8] */
        {
            {300, 300, 300, 200, 0, 0, 0, 0, /*0 - 7*/},
            {800, 800, 700, 400, 0, 0, 0, 0, /*0 - 7*/},
            {800, 800, 700, 400, 0, 0, 0, 0, /*0 - 7*/},
            {300, 100, 50, 50, 0, 0, 0, 0, /*0 - 7*/},
            {600, 150, 50, 50, 0, 0, 0, 0, /*0 - 7*/},
            {800, 600, 200, 200, 0, 0, 0, 0, /*0 - 7*/},
            {800, 600, 200, 200, 0, 0, 0, 1000, /*0 - 7*/},
            {600, 150, 50, 50, 0, 0, 0, 0, /*0 - 7*/},
            {10, 100, 100, 0, 0, 0, 100, 100, /*0 - 7*/},
            {700, 700, 600, 500, 400, 200, 100, 700, /*0 - 7*/},
            {700, 700, 600, 500, 400, 200, 200, 900, /*0 - 7*/},
            {100, 10, 10, 0, 0, 0, 0, 0, /*0 - 7*/},
            {100, 100, 300, 400, 450, 450, 250, 250, /*0 - 7*/},
            {500, 500, 500, 1000, 900, 900, 500, 900, /*0 - 7*/},
            {500, 500, 500, 1000, 900, 900, 900, 900, /*0 - 7*/},
            {400, 10, 10, 0, 0, 0, 0, 0, /*0 - 7*/},
            {100, 300, 300, 500, 500, 400, 300, 300, /*0 - 7*/},
            {400, 500, 500, 1000, 1000, 700, 500, 450, /*0 - 7*/},
            {500, 500, 500, 1000, 1000, 700, 700, 700, /*0 - 7*/},
            {200, 10, 10, 0, 0, 0, 0, 0, /*0 - 7*/},
            {100, 200, 200, 200, 200, 200, 200, 200, /*0 - 7*/},
            {500, 500, 300, 500, 500, 500, 400, 400, /*0 - 7*/},
            {500, 500, 300, 500, 500, 500, 500, 600, /*0 - 7*/},
            {100, 200, 300, 300, 100, 0, 0, 0, /*0 - 7*/},
        },
        /* nExtIlllumLuxWeight[32][8] */
        {
            {1000, 1000, 1000, 200, 50, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
            {0, 0, 0, 0, 0, 0, 0, 0, /*0 - 7*/},
        },
        /* nLumaWeightNum */
        8,
        /* nLumaSplitList[32] */
        {51, 819, 3072, 8192, 20480, 40960, 92160, 215040, /*0 - 7*/},
        /* nLumaWeightList[8][32] */
        {
            {51, 102, 614, 1024, 1024, 1024, 1024, 512, /*0 - 7*/},
            {102, 205, 819, 1024, 1024, 1024, 1024, 512, /*0 - 7*/},
            {100, 200, 820, 1024, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 200, 820, 1024, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 200, 820, 1024, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 200, 820, 1024, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 200, 820, 1024, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 200, 820, 1024, 1024, 1024, 1024, 820, /*0 - 7*/},
        },
        /* bMixLightEn */
        1,
        /* nMixLightProba_0_CctStd[8] */
        {400, 400, 400, 450, 600, 9998, 9998, 9998, /*0 - 7*/},
        /* nMixLightProba_100_CctStd[8] */
        {600, 600, 600, 800, 1000, 9999, 9999, 9999, /*0 - 7*/},
        /* nMixLightProba_100_SatDiscnt[8] */
        {100, 100, 100, 100, 100, 100, 100, 100, /*0 - 7*/},
        /* nMixLightKneeNum */
        8,
        /* nMixLightKneeCctList[32] */
        {2300, 2800, 3500, 4600, 5500, 6500, 7500, 8500, /*0 - 7*/},
        /* nMixLightKneeWtList[8][32] */
        {
            {100, 512, 614, 640, 820, 820, 820, 820, /*0 - 7*/},
            {100, 512, 614, 820, 820, 820, 820, 820, /*0 - 7*/},
            {205, 307, 310, 820, 820, 820, 820, 820, /*0 - 7*/},
            {205, 205, 310, 820, 820, 820, 820, 820, /*0 - 7*/},
            {407, 407, 512, 1024, 1024, 820, 820, 820, /*0 - 7*/},
            {512, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 7*/},
            {512, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 7*/},
            {512, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 7*/},
        },
        /* tDomParamList[4] */
        {
            /* 0 */
            {
                /* nDominantEnable */
                1,
                /* nDomMinCctThresh */
                2000,
                /* nDomMaxCctThresh */
                2500,
                /* nDom2AllRatioThresh */
                409,
                /* nDom2MinorRatioThresh */
                4096,
                /* nMinorWeight */
                0,
                /* nSmoothPercent */
                204,
            },
            /* 1 */
            {
                /* nDominantEnable */
                1,
                /* nDomMinCctThresh */
                2500,
                /* nDomMaxCctThresh */
                3300,
                /* nDom2AllRatioThresh */
                409,
                /* nDom2MinorRatioThresh */
                4096,
                /* nMinorWeight */
                0,
                /* nSmoothPercent */
                204,
            },
            /* 2 */
            {
                /* nDominantEnable */
                1,
                /* nDomMinCctThresh */
                3300,
                /* nDomMaxCctThresh */
                4600,
                /* nDom2AllRatioThresh */
                409,
                /* nDom2MinorRatioThresh */
                4096,
                /* nMinorWeight */
                0,
                /* nSmoothPercent */
                204,
            },
            /* 3 */
            {
                /* nDominantEnable */
                1,
                /* nDomMinCctThresh */
                4600,
                /* nDomMaxCctThresh */
                8500,
                /* nDom2AllRatioThresh */
                409,
                /* nDom2MinorRatioThresh */
                4096,
                /* nMinorWeight */
                0,
                /* nSmoothPercent */
                204,
            },
        },
        /* nTmpoStabTriggerAvgBlkWt */
        10,
        /* nPlanckianLocusProjEn */
        0,
        /* nPlanckianLocusNotProjLux */
        4096000,
        /* nPlanckianLocusFullProjLux */
        10240000,
        /* nSpatialSegmetNum */
        2,
        /* nSpatialStartLux[32] */
        {4096000, 11264000, /*0 - 1*/},
        /* nSpatialEndLux[32] */
        {10240000, 1024000000, /*0 - 1*/},
        /* nSpatialRg[32] */
        {488636, 622854, /*0 - 1*/},
        /* nSpatialBg[32] */
        {636485, 615514, /*0 - 1*/},
        /* nFusionGrayZoneConfid_0_AvgBlkWeight */
        20,
        /* nFusionGrayZoneConfid_100_AvgBlkWeight */
        500,
        /* nFusionSpatialConfid_0_Lux */
        5120000,
        /* nFusionSpatialConfid_100_Lux */
        15360000,
        /* nFusionWeightGrayZone */
        1024,
        /* nFusionWeightSpatial */
        0,
        /* nPreferCctNum */
        10,
        /* nPreferSrcCctList[32] */
        {1800, 2300, 2500, 2850, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
        /* nPreferDstCct[8][32] */
        {
            {1800, 2300, 2500, 2950, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2950, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4400, 5000, 6600, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
        },
        /* nPreferGrShift[8][32] */
        {
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 9*/},
        },
        /* nMultiCamSyncMode */
        0,
    },
    /* tLogParam */
    {
        /* nLogLevel */
        4,
        /* nLogTarget */
        2,
        /* nAlgoPrintInterval */
        0,
        /* nStatisticsPrintInterval */
        0,
    },
};

const static AX_ISP_IQ_BLC_PARAM_T blc_param_sdr = {
    /* nBlcEnable */
    1,
    /* nSblEnable */
    1,
    /* nAutoMode */
    1,
    /* tManualParam[4] */
    {
        /* 0 */
        {
            /* nSblRValue */
            4148,
            /* nSblGrValue */
            4149,
            /* nSblGbValue */
            4149,
            /* nSblBValue */
            4148,
        },
        /* 1 */
        {
            /* nSblRValue */
            0,
            /* nSblGrValue */
            0,
            /* nSblGbValue */
            0,
            /* nSblBValue */
            0,
        },
        /* 2 */
        {
            /* nSblRValue */
            0,
            /* nSblGrValue */
            0,
            /* nSblGbValue */
            0,
            /* nSblBValue */
            0,
        },
        /* 3 */
        {
            /* nSblRValue */
            0,
            /* nSblGrValue */
            0,
            /* nSblGbValue */
            0,
            /* nSblBValue */
            0,
        },
    },
    /* tAutoParam */
    {
        /* tHcgAutoTable */
        {
            /* nGainGrpNum */
            2,
            /* nExposeTimeGrpNum */
            2,
            /* nGain[16] */
            {1024, 2048, /*0 - 1*/},
            /* nExposeTime[10] */
            {1000, 5000, /*0 - 1*/},
            /* nAutoSblRValue[16][10] */
            {
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblGrValue[16][10] */
            {
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblGbValue[16][10] */
            {
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblBValue[16][10] */
            {
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
        },
        /* tLcgAutoTable */
        {
            /* nGainGrpNum */
            13,
            /* nExposeTimeGrpNum */
            2,
            /* nGain[16] */
            {1024, 2048, 4096, 8192, 12288, 16384, 20480, 24576, 49251, 98304, 196608, 393216, 517536, /*0 - 12*/},
            /* nExposeTime[10] */
            {10000, 30000, /*0 - 1*/},
            /* nAutoSblRValue[16][10] */
            {
                {4149, 4149, /*0 - 1*/},
                {4143, 4143, /*0 - 1*/},
                {4144, 4145, /*0 - 1*/},
                {4141, 4143, /*0 - 1*/},
                {4140, 4145, /*0 - 1*/},
                {4139, 4141, /*0 - 1*/},
                {4142, 4140, /*0 - 1*/},
                {4144, 4141, /*0 - 1*/},
                {4144, 4141, /*0 - 1*/},
                {4144, 4141, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblGrValue[16][10] */
            {
                {4150, 4150, /*0 - 1*/},
                {4143, 4144, /*0 - 1*/},
                {4145, 4146, /*0 - 1*/},
                {4143, 4144, /*0 - 1*/},
                {4142, 4147, /*0 - 1*/},
                {4142, 4144, /*0 - 1*/},
                {4145, 4143, /*0 - 1*/},
                {4147, 4143, /*0 - 1*/},
                {4147, 4143, /*0 - 1*/},
                {4147, 4143, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblGbValue[16][10] */
            {
                {4150, 4150, /*0 - 1*/},
                {4143, 4144, /*0 - 1*/},
                {4145, 4146, /*0 - 1*/},
                {4143, 4145, /*0 - 1*/},
                {4142, 4147, /*0 - 1*/},
                {4142, 4144, /*0 - 1*/},
                {4146, 4144, /*0 - 1*/},
                {4148, 4144, /*0 - 1*/},
                {4148, 4144, /*0 - 1*/},
                {4148, 4144, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblBValue[16][10] */
            {
                {4149, 4149, /*0 - 1*/},
                {4143, 4143, /*0 - 1*/},
                {4144, 4145, /*0 - 1*/},
                {4141, 4142, /*0 - 1*/},
                {4139, 4145, /*0 - 1*/},
                {4139, 4141, /*0 - 1*/},
                {4142, 4140, /*0 - 1*/},
                {4144, 4140, /*0 - 1*/},
                {4144, 4140, /*0 - 1*/},
                {4144, 4140, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
        },
    },
};

#endif
