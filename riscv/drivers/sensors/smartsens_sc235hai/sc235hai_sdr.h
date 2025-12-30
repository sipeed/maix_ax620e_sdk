#ifndef __SC235HAI_SDR_H__
#define __SC235HAI_SDR_H__

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
        25600,
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
        970755,
        /* nRealMaxShutter */
        39880,
    },
    /* tAeAlgAuto */
    {
        /* nSetPoint */
        25600,
        /* nFaceTarget */
        15360,
        /* nTolerance */
        10485760,
        /* nAgainLcg2HcgTh */
        15872,
        /* nAgainHcg2LcgTh */
        1228,
        /* nAgainLcg2HcgRatio */
        1024,
        /* nAgainHcg2LcgRatio */
        1024,
        /* nLuxk */
        104266,
        /* nCompensationMode */
        1,
        /* nMaxIspGain */
        16384,
        /* nMinIspGain */
        1024,
        /* nMaxUserDgain */
        1024,
        /* nMinUserDgain */
        1024,
        /* nMaxUserTotalAgain */
        55271,
        /* nMinUserTotalAgain */
        1024,
        /* nMaxUserSysGain */
        884336,
        /* nMinUserSysGain */
        1024,
        /* nMaxShutter */
        166000,
        /* nMinShutter */
        30,
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
                    6,
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
                            20000,
                            /* nGain */
                            2048,
                            /* nIncrementPriority */
                            0,
                        },
                        /* 2 */
                        {
                            /* nIntergrationTime */
                            30000,
                            /* nGain */
                            8192,
                            /* nIncrementPriority */
                            0,
                        },
                        /* 3 */
                        {
                            /* nIntergrationTime */
                            39880,
                            /* nGain */
                            16384,
                            /* nIncrementPriority */
                            0,
                        },
                        /* 4 */
                        {
                            /* nIntergrationTime */
                            49850,
                            /* nGain */
                            32768,
                            /* nIncrementPriority */
                            0,
                        },
                        /* 5 */
                        {
                            /* nIntergrationTime */
                            66000,
                            /* nGain */
                            884336,
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
            6,
            /* nRefList[10] */
            {256, 1536, 10240, 81920, 409600, 2048000, /*0 - 5*/},
            /* nSetPointList[10] */
            {18432, 20480, 22528, 26624, 28672, 30720, /*0 - 5*/},
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
                {61, 82, 102, 123, 154, 154, 205, 205, 256, 307, 358, 410, 512, 614, /*0 - 13*/},
                /* nFastOverSpeedDownFactorList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 102, 154, 307, /*0 - 13*/},
                /* nFastOverSkipList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 13*/},
                /* nFastUnderKneeCnt */
                11,
                /* nFastUnderLumaDiffList[16] */
                {5120, 10240, 15360, 20480, 25600, 30720, 35840, 40960, 51200, 153600, 262144, /*0 - 10*/},
                /* nFastUnderStepFactorList[16] */
                {51, 72, 92, 102, 123, 154, 154, 154, 184, 205, 256, /*0 - 10*/},
                /* nFastUnderSpeedDownFactorList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 20, 51, 102, /*0 - 10*/},
                /* nFastUnderSkipList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 10*/},
                /* nSlowOverKneeCnt */
                14,
                /* nSlowOverLumaDiffList[16] */
                {5120, 10240, 15360, 20480, 25600, 30720, 40960, 51200, 81920, 92160, 112640, 153600, 209920, 262144, /*0 - 13*/},
                /* nSlowOverStepFactorList[16] */
                {41, 61, 82, 102, 102, 123, 123, 154, 184, 184, 184, 184, 205, 256, /*0 - 13*/},
                /* nSlowOverSpeedDownFactorList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 41, 82, /*0 - 13*/},
                /* nSlowOverSkipList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*0 - 13*/},
                /* nSlowUnderKneeCnt */
                11,
                /* nSlowUnderLumaDiffList[16] */
                {5120, 10240, 15360, 20480, 25600, 30720, 35840, 40960, 51200, 153600, 262144, /*0 - 10*/},
                /* nSlowUnderStepFactorList[16] */
                {31, 51, 72, 92, 102, 123, 154, 154, 184, 205, 256, /*0 - 10*/},
                /* nSlowUnderSpeedDownFactorList[16] */
                {0, 0, 0, 0, 0, 0, 0, 0, 10, 31, 51, /*0 - 10*/},
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
            531,
            /* nGainGr */
            256,
            /* nGainGb */
            256,
            /* nGainB */
            469,
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
            1405966,
            /* nBg */
            1496629,
        },
        /* nCenterPntRadius */
        1255491,
        /* nLowCut */
        0,
        /* nHighCut */
        0,
        /* nCctMax */
        12000,
        /* nCctMin */
        1100,
        /* nPartCtrlPntNum */
        8,
        /* nCtrlPntNum */
        57,
        /* nCtrlSegKbNum */
        56,
        /* nCctList[512] */
        {
            1100, 1250, 1400, 1550, 1700, 1850, 2000, 2150, 2300, 2362, 2425, 2487, 2550, 2612, 2675, 2737, 2800, 2925, 3050, 3175, 3300, 3425, 3550, 3675, 3800, 3950, 4100, 4250, 4400, 4550, 4700, 4850,  /* 0 - 31*/
            5000, 5187, 5375, 5562, 5750, 5937, 6125, 6312, 6500, 6625, 6750, 6875, 7000, 7125, 7250, 7375, 7500, 8062, 8625, 9187, 9750, 10312, 10875, 11437, 12000, /*32 - 56*/
        },
        /* tChordKB */
        {
            /* nK */
            -562,
            /* nB */
            1228,
        },
        /* tChordPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1756742,
                /* nBg */
                291137,
            },
            /* 1 */
            {
                /* nRg */
                1666292,
                /* nBg */
                340913,
            },
            /* 2 */
            {
                /* nRg */
                1585216,
                /* nBg */
                385530,
            },
            /* 3 */
            {
                /* nRg */
                1511585,
                /* nBg */
                426047,
            },
            /* 4 */
            {
                /* nRg */
                1443931,
                /* nBg */
                463271,
            },
            /* 5 */
            {
                /* nRg */
                1381111,
                /* nBg */
                497843,
            },
            /* 6 */
            {
                /* nRg */
                1322170,
                /* nBg */
                530275,
            },
            /* 7 */
            {
                /* nRg */
                1266365,
                /* nBg */
                560978,
            },
            /* 8 */
            {
                /* nRg */
                1213066,
                /* nBg */
                590306,
            },
            /* 9 */
            {
                /* nRg */
                1191455,
                /* nBg */
                602197,
            },
            /* 10 */
            {
                /* nRg */
                1170148,
                /* nBg */
                613920,
            },
            /* 11 */
            {
                /* nRg */
                1149113,
                /* nBg */
                625497,
            },
            /* 12 */
            {
                /* nRg */
                1128310,
                /* nBg */
                636947,
            },
            /* 13 */
            {
                /* nRg */
                1107716,
                /* nBg */
                648282,
            },
            /* 14 */
            {
                /* nRg */
                1087300,
                /* nBg */
                659512,
            },
            /* 15 */
            {
                /* nRg */
                1067020,
                /* nBg */
                670669,
            },
            /* 16 */
            {
                /* nRg */
                1046867,
                /* nBg */
                681763,
            },
            /* 17 */
            {
                /* nRg */
                1024805,
                /* nBg */
                693906,
            },
            /* 18 */
            {
                /* nRg */
                1002827,
                /* nBg */
                705996,
            },
            /* 19 */
            {
                /* nRg */
                980880,
                /* nBg */
                718075,
            },
            /* 20 */
            {
                /* nRg */
                958933,
                /* nBg */
                730144,
            },
            /* 21 */
            {
                /* nRg */
                936966,
                /* nBg */
                742235,
            },
            /* 22 */
            {
                /* nRg */
                914925,
                /* nBg */
                754367,
            },
            /* 23 */
            {
                /* nRg */
                892789,
                /* nBg */
                766551,
            },
            /* 24 */
            {
                /* nRg */
                870507,
                /* nBg */
                778809,
            },
            /* 25 */
            {
                /* nRg */
                858637,
                /* nBg */
                785341,
            },
            /* 26 */
            {
                /* nRg */
                846715,
                /* nBg */
                791906,
            },
            /* 27 */
            {
                /* nRg */
                834729,
                /* nBg */
                798501,
            },
            /* 28 */
            {
                /* nRg */
                822671,
                /* nBg */
                805128,
            },
            /* 29 */
            {
                /* nRg */
                810549,
                /* nBg */
                811808,
            },
            /* 30 */
            {
                /* nRg */
                798344,
                /* nBg */
                818518,
            },
            /* 31 */
            {
                /* nRg */
                786055,
                /* nBg */
                825282,
            },
            /* 32 */
            {
                /* nRg */
                773671,
                /* nBg */
                832097,
            },
            /* 33 */
            {
                /* nRg */
                763951,
                /* nBg */
                837445,
            },
            /* 34 */
            {
                /* nRg */
                754178,
                /* nBg */
                842824,
            },
            /* 35 */
            {
                /* nRg */
                744332,
                /* nBg */
                848235,
            },
            /* 36 */
            {
                /* nRg */
                734423,
                /* nBg */
                853698,
            },
            /* 37 */
            {
                /* nRg */
                724430,
                /* nBg */
                859193,
            },
            /* 38 */
            {
                /* nRg */
                714374,
                /* nBg */
                864729,
            },
            /* 39 */
            {
                /* nRg */
                704224,
                /* nBg */
                870308,
            },
            /* 40 */
            {
                /* nRg */
                694000,
                /* nBg */
                875938,
            },
            /* 41 */
            {
                /* nRg */
                685244,
                /* nBg */
                880751,
            },
            /* 42 */
            {
                /* nRg */
                676426,
                /* nBg */
                885606,
            },
            /* 43 */
            {
                /* nRg */
                667544,
                /* nBg */
                890493,
            },
            /* 44 */
            {
                /* nRg */
                658579,
                /* nBg */
                895421,
            },
            /* 45 */
            {
                /* nRg */
                649551,
                /* nBg */
                900391,
            },
            /* 46 */
            {
                /* nRg */
                640449,
                /* nBg */
                905403,
            },
            /* 47 */
            {
                /* nRg */
                631264,
                /* nBg */
                910458,
            },
            /* 48 */
            {
                /* nRg */
                621994,
                /* nBg */
                915564,
            },
            /* 49 */
            {
                /* nRg */
                579202,
                /* nBg */
                939105,
            },
            /* 50 */
            {
                /* nRg */
                534407,
                /* nBg */
                963757,
            },
            /* 51 */
            {
                /* nRg */
                487263,
                /* nBg */
                989698,
            },
            /* 52 */
            {
                /* nRg */
                437393,
                /* nBg */
                1017140,
            },
            /* 53 */
            {
                /* nRg */
                384335,
                /* nBg */
                1046332,
            },
            /* 54 */
            {
                /* nRg */
                327533,
                /* nBg */
                1077590,
            },
            /* 55 */
            {
                /* nRg */
                266317,
                /* nBg */
                1111281,
            },
            /* 56 */
            {
                /* nRg */
                199901,
                /* nBg */
                1147824,
            },
        },
        /* tArcPointList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1733307,
                /* nBg */
                371678,
            },
            /* 1 */
            {
                /* nRg */
                1664572,
                /* nBg */
                348547,
            },
            /* 2 */
            {
                /* nRg */
                1594234,
                /* nBg */
                329630,
            },
            /* 3 */
            {
                /* nRg */
                1522532,
                /* nBg */
                315034,
            },
            /* 4 */
            {
                /* nRg */
                1449751,
                /* nBg */
                304853,
            },
            /* 5 */
            {
                /* nRg */
                1376162,
                /* nBg */
                299180,
            },
            /* 6 */
            {
                /* nRg */
                1302038,
                /* nBg */
                298068,
            },
            /* 7 */
            {
                /* nRg */
                1227662,
                /* nBg */
                301549,
            },
            /* 8 */
            {
                /* nRg */
                1153329,
                /* nBg */
                309665,
            },
            /* 9 */
            {
                /* nRg */
                1121725,
                /* nBg */
                311448,
            },
            /* 10 */
            {
                /* nRg */
                1090037,
                /* nBg */
                314069,
            },
            /* 11 */
            {
                /* nRg */
                1058307,
                /* nBg */
                317540,
            },
            /* 12 */
            {
                /* nRg */
                1026545,
                /* nBg */
                321860,
            },
            /* 13 */
            {
                /* nRg */
                994784,
                /* nBg */
                327040,
            },
            /* 14 */
            {
                /* nRg */
                963033,
                /* nBg */
                333080,
            },
            /* 15 */
            {
                /* nRg */
                931314,
                /* nBg */
                339980,
            },
            /* 16 */
            {
                /* nRg */
                899668,
                /* nBg */
                347750,
            },
            /* 17 */
            {
                /* nRg */
                867445,
                /* nBg */
                362493,
            },
            /* 18 */
            {
                /* nRg */
                835652,
                /* nBg */
                378148,
            },
            /* 19 */
            {
                /* nRg */
                804310,
                /* nBg */
                394694,
            },
            /* 20 */
            {
                /* nRg */
                773451,
                /* nBg */
                412111,
            },
            /* 21 */
            {
                /* nRg */
                743094,
                /* nBg */
                430399,
            },
            /* 22 */
            {
                /* nRg */
                713262,
                /* nBg */
                449525,
            },
            /* 23 */
            {
                /* nRg */
                683986,
                /* nBg */
                469500,
            },
            /* 24 */
            {
                /* nRg */
                655287,
                /* nBg */
                490283,
            },
            /* 25 */
            {
                /* nRg */
                640323,
                /* nBg */
                501618,
            },
            /* 26 */
            {
                /* nRg */
                625528,
                /* nBg */
                513184,
            },
            /* 27 */
            {
                /* nRg */
                610900,
                /* nBg */
                524959,
            },
            /* 28 */
            {
                /* nRg */
                596461,
                /* nBg */
                536965,
            },
            /* 29 */
            {
                /* nRg */
                582201,
                /* nBg */
                549171,
            },
            /* 30 */
            {
                /* nRg */
                568129,
                /* nBg */
                561596,
            },
            /* 31 */
            {
                /* nRg */
                554235,
                /* nBg */
                574232,
            },
            /* 32 */
            {
                /* nRg */
                540541,
                /* nBg */
                587077,
            },
            /* 33 */
            {
                /* nRg */
                529992,
                /* nBg */
                597227,
            },
            /* 34 */
            {
                /* nRg */
                519569,
                /* nBg */
                607493,
            },
            /* 35 */
            {
                /* nRg */
                509272,
                /* nBg */
                617884,
            },
            /* 36 */
            {
                /* nRg */
                499091,
                /* nBg */
                628391,
            },
            /* 37 */
            {
                /* nRg */
                489035,
                /* nBg */
                639023,
            },
            /* 38 */
            {
                /* nRg */
                479105,
                /* nBg */
                649761,
            },
            /* 39 */
            {
                /* nRg */
                469290,
                /* nBg */
                660624,
            },
            /* 40 */
            {
                /* nRg */
                459612,
                /* nBg */
                671602,
            },
            /* 41 */
            {
                /* nRg */
                451496,
                /* nBg */
                681008,
            },
            /* 42 */
            {
                /* nRg */
                443464,
                /* nBg */
                690487,
            },
            /* 43 */
            {
                /* nRg */
                435537,
                /* nBg */
                700061,
            },
            /* 44 */
            {
                /* nRg */
                427704,
                /* nBg */
                709697,
            },
            /* 45 */
            {
                /* nRg */
                419965,
                /* nBg */
                719418,
            },
            /* 46 */
            {
                /* nRg */
                412321,
                /* nBg */
                729222,
            },
            /* 47 */
            {
                /* nRg */
                404771,
                /* nBg */
                739089,
            },
            /* 48 */
            {
                /* nRg */
                397326,
                /* nBg */
                749040,
            },
            /* 49 */
            {
                /* nRg */
                365041,
                /* nBg */
                794684,
            },
            /* 50 */
            {
                /* nRg */
                334810,
                /* nBg */
                841734,
            },
            /* 51 */
            {
                /* nRg */
                306719,
                /* nBg */
                890073,
            },
            /* 52 */
            {
                /* nRg */
                280798,
                /* nBg */
                939618,
            },
            /* 53 */
            {
                /* nRg */
                257121,
                /* nBg */
                990265,
            },
            /* 54 */
            {
                /* nRg */
                235709,
                /* nBg */
                1041918,
            },
            /* 55 */
            {
                /* nRg */
                216625,
                /* nBg */
                1094472,
            },
            /* 56 */
            {
                /* nRg */
                199901,
                /* nBg */
                1147824,
            },
        },
        /* tRadiusLineList[512] */
        {
            /* 0 */
            {
                /* nK */
                -3518,
                /* nB */
                6180,
            },
            /* 1 */
            {
                /* nK */
                -4545,
                /* nB */
                7557,
            },
            /* 2 */
            {
                /* nK */
                -6346,
                /* nB */
                9972,
            },
            /* 3 */
            {
                /* nK */
                -10379,
                /* nB */
                15379,
            },
            /* 4 */
            {
                /* nK */
                -27869,
                /* nB */
                38830,
            },
            /* 5 */
            {
                /* nK */
                41142,
                /* nB */
                -53702,
            },
            /* 6 */
            {
                /* nK */
                11809,
                /* nB */
                -14372,
            },
            /* 7 */
            {
                /* nK */
                6863,
                /* nB */
                -7740,
            },
            /* 8 */
            {
                /* nK */
                4811,
                /* nB */
                -4988,
            },
            /* 9 */
            {
                /* nK */
                4270,
                /* nB */
                -4262,
            },
            /* 10 */
            {
                /* nK */
                3833,
                /* nB */
                -3677,
            },
            /* 11 */
            {
                /* nK */
                3473,
                /* nB */
                -3194,
            },
            /* 12 */
            {
                /* nK */
                3171,
                /* nB */
                -2789,
            },
            /* 13 */
            {
                /* nK */
                2913,
                /* nB */
                -2443,
            },
            /* 14 */
            {
                /* nK */
                2690,
                /* nB */
                -2144,
            },
            /* 15 */
            {
                /* nK */
                2495,
                /* nB */
                -1883,
            },
            /* 16 */
            {
                /* nK */
                2324,
                /* nB */
                -1653,
            },
            /* 17 */
            {
                /* nK */
                2157,
                /* nB */
                -1429,
            },
            /* 18 */
            {
                /* nK */
                2008,
                /* nB */
                -1230,
            },
            /* 19 */
            {
                /* nK */
                1875,
                /* nB */
                -1052,
            },
            /* 20 */
            {
                /* nK */
                1756,
                /* nB */
                -892,
            },
            /* 21 */
            {
                /* nK */
                1647,
                /* nB */
                -746,
            },
            /* 22 */
            {
                /* nK */
                1548,
                /* nB */
                -613,
            },
            /* 23 */
            {
                /* nK */
                1457,
                /* nB */
                -491,
            },
            /* 24 */
            {
                /* nK */
                1373,
                /* nB */
                -378,
            },
            /* 25 */
            {
                /* nK */
                1331,
                /* nB */
                -322,
            },
            /* 26 */
            {
                /* nK */
                1290,
                /* nB */
                -268,
            },
            /* 27 */
            {
                /* nK */
                1251,
                /* nB */
                -215,
            },
            /* 28 */
            {
                /* nK */
                1214,
                /* nB */
                -165,
            },
            /* 29 */
            {
                /* nK */
                1178,
                /* nB */
                -117,
            },
            /* 30 */
            {
                /* nK */
                1143,
                /* nB */
                -70,
            },
            /* 31 */
            {
                /* nK */
                1109,
                /* nB */
                -24,
            },
            /* 32 */
            {
                /* nK */
                1076,
                /* nB */
                19,
            },
            /* 33 */
            {
                /* nK */
                1051,
                /* nB */
                52,
            },
            /* 34 */
            {
                /* nK */
                1027,
                /* nB */
                84,
            },
            /* 35 */
            {
                /* nK */
                1003,
                /* nB */
                116,
            },
            /* 36 */
            {
                /* nK */
                980,
                /* nB */
                147,
            },
            /* 37 */
            {
                /* nK */
                958,
                /* nB */
                177,
            },
            /* 38 */
            {
                /* nK */
                936,
                /* nB */
                207,
            },
            /* 39 */
            {
                /* nK */
                914,
                /* nB */
                236,
            },
            /* 40 */
            {
                /* nK */
                893,
                /* nB */
                265,
            },
            /* 41 */
            {
                /* nK */
                875,
                /* nB */
                288,
            },
            /* 42 */
            {
                /* nK */
                858,
                /* nB */
                312,
            },
            /* 43 */
            {
                /* nK */
                841,
                /* nB */
                335,
            },
            /* 44 */
            {
                /* nK */
                824,
                /* nB */
                357,
            },
            /* 45 */
            {
                /* nK */
                807,
                /* nB */
                379,
            },
            /* 46 */
            {
                /* nK */
                791,
                /* nB */
                401,
            },
            /* 47 */
            {
                /* nK */
                775,
                /* nB */
                423,
            },
            /* 48 */
            {
                /* nK */
                759,
                /* nB */
                444,
            },
            /* 49 */
            {
                /* nK */
                691,
                /* nB */
                536,
            },
            /* 50 */
            {
                /* nK */
                626,
                /* nB */
                622,
            },
            /* 51 */
            {
                /* nK */
                565,
                /* nB */
                704,
            },
            /* 52 */
            {
                /* nK */
                507,
                /* nB */
                782,
            },
            /* 53 */
            {
                /* nK */
                451,
                /* nB */
                856,
            },
            /* 54 */
            {
                /* nK */
                398,
                /* nB */
                928,
            },
            /* 55 */
            {
                /* nK */
                346,
                /* nB */
                997,
            },
            /* 56 */
            {
                /* nK */
                296,
                /* nB */
                1064,
            },
        },
        /* tInLeftBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1759678,
                /* nBg */
                281071,
            },
            /* 1 */
            {
                /* nRg */
                1685313,
                /* nBg */
                256482,
            },
            /* 2 */
            {
                /* nRg */
                1609260,
                /* nBg */
                236454,
            },
            /* 3 */
            {
                /* nRg */
                1531802,
                /* nBg */
                221113,
            },
            /* 4 */
            {
                /* nRg */
                1453221,
                /* nBg */
                210544,
            },
            /* 5 */
            {
                /* nRg */
                1373813,
                /* nBg */
                204839,
            },
            /* 6 */
            {
                /* nRg */
                1293880,
                /* nBg */
                204042,
            },
            /* 7 */
            {
                /* nRg */
                1213737,
                /* nBg */
                208216,
            },
            /* 8 */
            {
                /* nRg */
                1133678,
                /* nBg */
                217370,
            },
            /* 9 */
            {
                /* nRg */
                1099715,
                /* nBg */
                219677,
            },
            /* 10 */
            {
                /* nRg */
                1065689,
                /* nBg */
                222896,
            },
            /* 11 */
            {
                /* nRg */
                1031621,
                /* nBg */
                227017,
            },
            /* 12 */
            {
                /* nRg */
                997542,
                /* nBg */
                232060,
            },
            /* 13 */
            {
                /* nRg */
                963484,
                /* nBg */
                238006,
            },
            /* 14 */
            {
                /* nRg */
                929458,
                /* nBg */
                244884,
            },
            /* 15 */
            {
                /* nRg */
                895494,
                /* nBg */
                252675,
            },
            /* 16 */
            {
                /* nRg */
                861615,
                /* nBg */
                261389,
            },
            /* 17 */
            {
                /* nRg */
                826970,
                /* nBg */
                277243,
            },
            /* 18 */
            {
                /* nRg */
                792786,
                /* nBg */
                294073,
            },
            /* 19 */
            {
                /* nRg */
                759085,
                /* nBg */
                311857,
            },
            /* 20 */
            {
                /* nRg */
                725908,
                /* nBg */
                330585,
            },
            /* 21 */
            {
                /* nRg */
                693266,
                /* nBg */
                350245,
            },
            /* 22 */
            {
                /* nRg */
                661190,
                /* nBg */
                370818,
            },
            /* 23 */
            {
                /* nRg */
                629712,
                /* nBg */
                392293,
            },
            /* 24 */
            {
                /* nRg */
                598852,
                /* nBg */
                414638,
            },
            /* 25 */
            {
                /* nRg */
                582767,
                /* nBg */
                426833,
            },
            /* 26 */
            {
                /* nRg */
                566860,
                /* nBg */
                439259,
            },
            /* 27 */
            {
                /* nRg */
                551142,
                /* nBg */
                451926,
            },
            /* 28 */
            {
                /* nRg */
                535613,
                /* nBg */
                464823,
            },
            /* 29 */
            {
                /* nRg */
                520282,
                /* nBg */
                477962,
            },
            /* 30 */
            {
                /* nRg */
                505141,
                /* nBg */
                491321,
            },
            /* 31 */
            {
                /* nRg */
                490209,
                /* nBg */
                504900,
            },
            /* 32 */
            {
                /* nRg */
                475487,
                /* nBg */
                518710,
            },
            /* 33 */
            {
                /* nRg */
                464152,
                /* nBg */
                529615,
            },
            /* 34 */
            {
                /* nRg */
                452943,
                /* nBg */
                540656,
            },
            /* 35 */
            {
                /* nRg */
                441870,
                /* nBg */
                551834,
            },
            /* 36 */
            {
                /* nRg */
                430923,
                /* nBg */
                563127,
            },
            /* 37 */
            {
                /* nRg */
                420112,
                /* nBg */
                574557,
            },
            /* 38 */
            {
                /* nRg */
                409427,
                /* nBg */
                586112,
            },
            /* 39 */
            {
                /* nRg */
                398889,
                /* nBg */
                597783,
            },
            /* 40 */
            {
                /* nRg */
                388476,
                /* nBg */
                609579,
            },
            /* 41 */
            {
                /* nRg */
                379752,
                /* nBg */
                619698,
            },
            /* 42 */
            {
                /* nRg */
                371123,
                /* nBg */
                629901,
            },
            /* 43 */
            {
                /* nRg */
                362598,
                /* nBg */
                640177,
            },
            /* 44 */
            {
                /* nRg */
                354167,
                /* nBg */
                650547,
            },
            /* 45 */
            {
                /* nRg */
                345841,
                /* nBg */
                661001,
            },
            /* 46 */
            {
                /* nRg */
                337631,
                /* nBg */
                671529,
            },
            /* 47 */
            {
                /* nRg */
                329515,
                /* nBg */
                682151,
            },
            /* 48 */
            {
                /* nRg */
                321504,
                /* nBg */
                692847,
            },
            /* 49 */
            {
                /* nRg */
                286796,
                /* nBg */
                741920,
            },
            /* 50 */
            {
                /* nRg */
                254301,
                /* nBg */
                792503,
            },
            /* 51 */
            {
                /* nRg */
                224091,
                /* nBg */
                844481,
            },
            /* 52 */
            {
                /* nRg */
                196231,
                /* nBg */
                897749,
            },
            /* 53 */
            {
                /* nRg */
                170761,
                /* nBg */
                952201,
            },
            /* 54 */
            {
                /* nRg */
                147744,
                /* nBg */
                1007734,
            },
            /* 55 */
            {
                /* nRg */
                127224,
                /* nBg */
                1064242,
            },
            /* 56 */
            {
                /* nRg */
                109241,
                /* nBg */
                1121609,
            },
        },
        /* tInRightBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1709871,
                /* nBg */
                452230,
            },
            /* 1 */
            {
                /* nRg */
                1646138,
                /* nBg */
                430378,
            },
            /* 2 */
            {
                /* nRg */
                1580875,
                /* nBg */
                412447,
            },
            /* 3 */
            {
                /* nRg */
                1514301,
                /* nBg */
                398511,
            },
            /* 4 */
            {
                /* nRg */
                1446678,
                /* nBg */
                388686,
            },
            /* 5 */
            {
                /* nRg */
                1378248,
                /* nBg */
                383045,
            },
            /* 6 */
            {
                /* nRg */
                1309283,
                /* nBg */
                381640,
            },
            /* 7 */
            {
                /* nRg */
                1240046,
                /* nBg */
                384523,
            },
            /* 8 */
            {
                /* nRg */
                1170788,
                /* nBg */
                391717,
            },
            /* 9 */
            {
                /* nRg */
                1141291,
                /* nBg */
                393017,
            },
            /* 10 */
            {
                /* nRg */
                1111690,
                /* nBg */
                395114,
            },
            /* 11 */
            {
                /* nRg */
                1082036,
                /* nBg */
                397998,
            },
            /* 12 */
            {
                /* nRg */
                1052330,
                /* nBg */
                401688,
            },
            /* 13 */
            {
                /* nRg */
                1022603,
                /* nBg */
                406176,
            },
            /* 14 */
            {
                /* nRg */
                992876,
                /* nBg */
                411482,
            },
            /* 15 */
            {
                /* nRg */
                963169,
                /* nBg */
                417585,
            },
            /* 16 */
            {
                /* nRg */
                933495,
                /* nBg */
                424516,
            },
            /* 17 */
            {
                /* nRg */
                903432,
                /* nBg */
                438273,
            },
            /* 18 */
            {
                /* nRg */
                873757,
                /* nBg */
                452880,
            },
            /* 19 */
            {
                /* nRg */
                844513,
                /* nBg */
                468315,
            },
            /* 20 */
            {
                /* nRg */
                815708,
                /* nBg */
                484568,
            },
            /* 21 */
            {
                /* nRg */
                787386,
                /* nBg */
                501639,
            },
            /* 22 */
            {
                /* nRg */
                759547,
                /* nBg */
                519486,
            },
            /* 23 */
            {
                /* nRg */
                732221,
                /* nBg */
                538119,
            },
            /* 24 */
            {
                /* nRg */
                705440,
                /* nBg */
                557517,
            },
            /* 25 */
            {
                /* nRg */
                691473,
                /* nBg */
                568098,
            },
            /* 26 */
            {
                /* nRg */
                677674,
                /* nBg */
                578887,
            },
            /* 27 */
            {
                /* nRg */
                664021,
                /* nBg */
                589887,
            },
            /* 28 */
            {
                /* nRg */
                650547,
                /* nBg */
                601086,
            },
            /* 29 */
            {
                /* nRg */
                637241,
                /* nBg */
                612484,
            },
            /* 30 */
            {
                /* nRg */
                624102,
                /* nBg */
                624070,
            },
            /* 31 */
            {
                /* nRg */
                611142,
                /* nBg */
                635867,
            },
            /* 32 */
            {
                /* nRg */
                598359,
                /* nBg */
                647842,
            },
            /* 33 */
            {
                /* nRg */
                588524,
                /* nBg */
                657321,
            },
            /* 34 */
            {
                /* nRg */
                578793,
                /* nBg */
                666905,
            },
            /* 35 */
            {
                /* nRg */
                569188,
                /* nBg */
                676594,
            },
            /* 36 */
            {
                /* nRg */
                559688,
                /* nBg */
                686408,
            },
            /* 37 */
            {
                /* nRg */
                550293,
                /* nBg */
                696317,
            },
            /* 38 */
            {
                /* nRg */
                541034,
                /* nBg */
                706352,
            },
            /* 39 */
            {
                /* nRg */
                531880,
                /* nBg */
                716481,
            },
            /* 40 */
            {
                /* nRg */
                522841,
                /* nBg */
                726726,
            },
            /* 41 */
            {
                /* nRg */
                515270,
                /* nBg */
                735503,
            },
            /* 42 */
            {
                /* nRg */
                507783,
                /* nBg */
                744353,
            },
            /* 43 */
            {
                /* nRg */
                500380,
                /* nBg */
                753276,
            },
            /* 44 */
            {
                /* nRg */
                493061,
                /* nBg */
                762283,
            },
            /* 45 */
            {
                /* nRg */
                485837,
                /* nBg */
                771353,
            },
            /* 46 */
            {
                /* nRg */
                478706,
                /* nBg */
                780497,
            },
            /* 47 */
            {
                /* nRg */
                471660,
                /* nBg */
                789704,
            },
            /* 48 */
            {
                /* nRg */
                464718,
                /* nBg */
                798983,
            },
            /* 49 */
            {
                /* nRg */
                434582,
                /* nBg */
                841587,
            },
            /* 50 */
            {
                /* nRg */
                406386,
                /* nBg */
                885491,
            },
            /* 51 */
            {
                /* nRg */
                380161,
                /* nBg */
                930601,
            },
            /* 52 */
            {
                /* nRg */
                355981,
                /* nBg */
                976832,
            },
            /* 53 */
            {
                /* nRg */
                333877,
                /* nBg */
                1024102,
            },
            /* 54 */
            {
                /* nRg */
                313902,
                /* nBg */
                1072295,
            },
            /* 55 */
            {
                /* nRg */
                296097,
                /* nBg */
                1121347,
            },
            /* 56 */
            {
                /* nRg */
                280484,
                /* nBg */
                1171134,
            },
        },
        /* tOutLeftBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1777252,
                /* nBg */
                220662,
            },
            /* 1 */
            {
                /* nRg */
                1699134,
                /* nBg */
                195109,
            },
            /* 2 */
            {
                /* nRg */
                1619284,
                /* nBg */
                174347,
            },
            /* 3 */
            {
                /* nRg */
                1537978,
                /* nBg */
                158503,
            },
            /* 4 */
            {
                /* nRg */
                1455528,
                /* nBg */
                147671,
            },
            /* 5 */
            {
                /* nRg */
                1372250,
                /* nBg */
                141946,
            },
            /* 6 */
            {
                /* nRg */
                1288448,
                /* nBg */
                141369,
            },
            /* 7 */
            {
                /* nRg */
                1204457,
                /* nBg */
                145983,
            },
            /* 8 */
            {
                /* nRg */
                1120582,
                /* nBg */
                155829,
            },
            /* 9 */
            {
                /* nRg */
                1085045,
                /* nBg */
                158492,
            },
            /* 10 */
            {
                /* nRg */
                1049446,
                /* nBg */
                162110,
            },
            /* 11 */
            {
                /* nRg */
                1013826,
                /* nBg */
                166671,
            },
            /* 12 */
            {
                /* nRg */
                978206,
                /* nBg */
                172187,
            },
            /* 13 */
            {
                /* nRg */
                942617,
                /* nBg */
                178656,
            },
            /* 14 */
            {
                /* nRg */
                907071,
                /* nBg */
                186080,
            },
            /* 15 */
            {
                /* nRg */
                871608,
                /* nBg */
                194469,
            },
            /* 16 */
            {
                /* nRg */
                836239,
                /* nBg */
                203822,
            },
            /* 17 */
            {
                /* nRg */
                799980,
                /* nBg */
                220411,
            },
            /* 18 */
            {
                /* nRg */
                764202,
                /* nBg */
                238027,
            },
            /* 19 */
            {
                /* nRg */
                728939,
                /* nBg */
                256639,
            },
            /* 20 */
            {
                /* nRg */
                694210,
                /* nBg */
                276247,
            },
            /* 21 */
            {
                /* nRg */
                660047,
                /* nBg */
                296820,
            },
            /* 22 */
            {
                /* nRg */
                626482,
                /* nBg */
                318348,
            },
            /* 23 */
            {
                /* nRg */
                593536,
                /* nBg */
                340819,
            },
            /* 24 */
            {
                /* nRg */
                561240,
                /* nBg */
                364212,
            },
            /* 25 */
            {
                /* nRg */
                544400,
                /* nBg */
                376963,
            },
            /* 26 */
            {
                /* nRg */
                527748,
                /* nBg */
                389976,
            },
            /* 27 */
            {
                /* nRg */
                511296,
                /* nBg */
                403230,
            },
            /* 28 */
            {
                /* nRg */
                495043,
                /* nBg */
                416736,
            },
            /* 29 */
            {
                /* nRg */
                479000,
                /* nBg */
                430482,
            },
            /* 30 */
            {
                /* nRg */
                463156,
                /* nBg */
                444460,
            },
            /* 31 */
            {
                /* nRg */
                447532,
                /* nBg */
                458679,
            },
            /* 32 */
            {
                /* nRg */
                432118,
                /* nBg */
                473128,
            },
            /* 33 */
            {
                /* nRg */
                420259,
                /* nBg */
                484547,
            },
            /* 34 */
            {
                /* nRg */
                408525,
                /* nBg */
                496102,
            },
            /* 35 */
            {
                /* nRg */
                396928,
                /* nBg */
                507794,
            },
            /* 36 */
            {
                /* nRg */
                385478,
                /* nBg */
                519622,
            },
            /* 37 */
            {
                /* nRg */
                374163,
                /* nBg */
                531576,
            },
            /* 38 */
            {
                /* nRg */
                362986,
                /* nBg */
                543676,
            },
            /* 39 */
            {
                /* nRg */
                351944,
                /* nBg */
                555892,
            },
            /* 40 */
            {
                /* nRg */
                341060,
                /* nBg */
                568234,
            },
            /* 41 */
            {
                /* nRg */
                331916,
                /* nBg */
                578824,
            },
            /* 42 */
            {
                /* nRg */
                322888,
                /* nBg */
                589499,
            },
            /* 43 */
            {
                /* nRg */
                313965,
                /* nBg */
                600268,
            },
            /* 44 */
            {
                /* nRg */
                305146,
                /* nBg */
                611110,
            },
            /* 45 */
            {
                /* nRg */
                296432,
                /* nBg */
                622057,
            },
            /* 46 */
            {
                /* nRg */
                287834,
                /* nBg */
                633078,
            },
            /* 47 */
            {
                /* nRg */
                279341,
                /* nBg */
                644182,
            },
            /* 48 */
            {
                /* nRg */
                270963,
                /* nBg */
                655381,
            },
            /* 49 */
            {
                /* nRg */
                234629,
                /* nBg */
                706751,
            },
            /* 50 */
            {
                /* nRg */
                200624,
                /* nBg */
                759683,
            },
            /* 51 */
            {
                /* nRg */
                169009,
                /* nBg */
                814083,
            },
            /* 52 */
            {
                /* nRg */
                139838,
                /* nBg */
                869836,
            },
            /* 53 */
            {
                /* nRg */
                113194,
                /* nBg */
                926826,
            },
            /* 54 */
            {
                /* nRg */
                89108,
                /* nBg */
                984948,
            },
            /* 55 */
            {
                /* nRg */
                67623,
                /* nBg */
                1044088,
            },
            /* 56 */
            {
                /* nRg */
                48801,
                /* nBg */
                1104130,
            },
        },
        /* tOutRightBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1692297,
                /* nBg */
                512638,
            },
            /* 1 */
            {
                /* nRg */
                1632318,
                /* nBg */
                491761,
            },
            /* 2 */
            {
                /* nRg */
                1570851,
                /* nBg */
                474554,
            },
            /* 3 */
            {
                /* nRg */
                1508125,
                /* nBg */
                461122,
            },
            /* 4 */
            {
                /* nRg */
                1444361,
                /* nBg */
                451559,
            },
            /* 5 */
            {
                /* nRg */
                1379811,
                /* nBg */
                445938,
            },
            /* 6 */
            {
                /* nRg */
                1314715,
                /* nBg */
                444313,
            },
            /* 7 */
            {
                /* nRg */
                1249326,
                /* nBg */
                446746,
            },
            /* 8 */
            {
                /* nRg */
                1183895,
                /* nBg */
                453257,
            },
            /* 9 */
            {
                /* nRg */
                1155961,
                /* nBg */
                454201,
            },
            /* 10 */
            {
                /* nRg */
                1127932,
                /* nBg */
                455889,
            },
            /* 11 */
            {
                /* nRg */
                1099830,
                /* nBg */
                458343,
            },
            /* 12 */
            {
                /* nRg */
                1071666,
                /* nBg */
                461552,
            },
            /* 13 */
            {
                /* nRg */
                1043469,
                /* nBg */
                465536,
            },
            /* 14 */
            {
                /* nRg */
                1015252,
                /* nBg */
                470276,
            },
            /* 15 */
            {
                /* nRg */
                987046,
                /* nBg */
                475791,
            },
            /* 16 */
            {
                /* nRg */
                958870,
                /* nBg */
                482083,
            },
            /* 17 */
            {
                /* nRg */
                930412,
                /* nBg */
                495106,
            },
            /* 18 */
            {
                /* nRg */
                902342,
                /* nBg */
                508926,
            },
            /* 19 */
            {
                /* nRg */
                874659,
                /* nBg */
                523533,
            },
            /* 20 */
            {
                /* nRg */
                847407,
                /* nBg */
                538916,
            },
            /* 21 */
            {
                /* nRg */
                820605,
                /* nBg */
                555064,
            },
            /* 22 */
            {
                /* nRg */
                794254,
                /* nBg */
                571967,
            },
            /* 23 */
            {
                /* nRg */
                768407,
                /* nBg */
                589593,
            },
            /* 24 */
            {
                /* nRg */
                743063,
                /* nBg */
                607954,
            },
            /* 25 */
            {
                /* nRg */
                729840,
                /* nBg */
                617968,
            },
            /* 26 */
            {
                /* nRg */
                716775,
                /* nBg */
                628170,
            },
            /* 27 */
            {
                /* nRg */
                703867,
                /* nBg */
                638572,
            },
            /* 28 */
            {
                /* nRg */
                691116,
                /* nBg */
                649173,
            },
            /* 29 */
            {
                /* nRg */
                678523,
                /* nBg */
                659953,
            },
            /* 30 */
            {
                /* nRg */
                666087,
                /* nBg */
                670931,
            },
            /* 31 */
            {
                /* nRg */
                653829,
                /* nBg */
                682088,
            },
            /* 32 */
            {
                /* nRg */
                641729,
                /* nBg */
                693423,
            },
            /* 33 */
            {
                /* nRg */
                632417,
                /* nBg */
                702389,
            },
            /* 34 */
            {
                /* nRg */
                623211,
                /* nBg */
                711459,
            },
            /* 35 */
            {
                /* nRg */
                614120,
                /* nBg */
                720634,
            },
            /* 36 */
            {
                /* nRg */
                605133,
                /* nBg */
                729914,
            },
            /* 37 */
            {
                /* nRg */
                596252,
                /* nBg */
                739299,
            },
            /* 38 */
            {
                /* nRg */
                587475,
                /* nBg */
                748788,
            },
            /* 39 */
            {
                /* nRg */
                578814,
                /* nBg */
                758372,
            },
            /* 40 */
            {
                /* nRg */
                570268,
                /* nBg */
                768061,
            },
            /* 41 */
            {
                /* nRg */
                563096,
                /* nBg */
                776376,
            },
            /* 42 */
            {
                /* nRg */
                556007,
                /* nBg */
                784754,
            },
            /* 43 */
            {
                /* nRg */
                549003,
                /* nBg */
                793195,
            },
            /* 44 */
            {
                /* nRg */
                542082,
                /* nBg */
                801710,
            },
            /* 45 */
            {
                /* nRg */
                535256,
                /* nBg */
                810298,
            },
            /* 46 */
            {
                /* nRg */
                528503,
                /* nBg */
                818948,
            },
            /* 47 */
            {
                /* nRg */
                521834,
                /* nBg */
                827662,
            },
            /* 48 */
            {
                /* nRg */
                515260,
                /* nBg */
                836449,
            },
            /* 49 */
            {
                /* nRg */
                486749,
                /* nBg */
                876767,
            },
            /* 50 */
            {
                /* nRg */
                460063,
                /* nBg */
                918301,
            },
            /* 51 */
            {
                /* nRg */
                435253,
                /* nBg */
                960988,
            },
            /* 52 */
            {
                /* nRg */
                412363,
                /* nBg */
                1004746,
            },
            /* 53 */
            {
                /* nRg */
                391444,
                /* nBg */
                1049467,
            },
            /* 54 */
            {
                /* nRg */
                372549,
                /* nBg */
                1095080,
            },
            /* 55 */
            {
                /* nRg */
                355687,
                /* nBg */
                1141501,
            },
            /* 56 */
            {
                /* nRg */
                340924,
                /* nBg */
                1188613,
            },
        },
        /* nIllumNum */
        6,
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
                    1149239,
                    /* nBg */
                    290456,
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
                    904921,
                    /* nBg */
                    359662,
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
                    607126,
                    /* nBg */
                    425722,
                },
            },
            /* 3 */
            {
                /* szName[32] */
                "D50",
                /* nCct */
                5000,
                /* nRadius */
                0,
                /* tCoord */
                {
                    /* nRg */
                    532677,
                    /* nBg */
                    578814,
                },
            },
            /* 4 */
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
                    485491,
                    /* nBg */
                    694157,
                },
            },
            /* 5 */
            {
                /* szName[32] */
                "D75",
                /* nCct */
                7500,
                /* nRadius */
                0,
                /* tCoord */
                {
                    /* nRg */
                    415236,
                    /* nBg */
                    762315,
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
                    577765,
                    /* nBg */
                    460325,
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
                1494,
                /* nGainGr */
                1024,
                /* nGainGb */
                1024,
                /* nGainB */
                2925,
            },
            /* nDampRatio */
            100000,
        },
        /* nMode */
        0,
        /* nIndex */
        0,
        /* nDampRatio */
        943718,
        /* nToleranceRg */
        3145,
        /* nToleranceBg */
        3145,
        /* nLuxVeryDarkStart */
        0,
        /* nLuxVeryDarkEnd */
        205,
        /* nLuxDarkStart */
        307,
        /* nLuxDarkEnd */
        5120,
        /* nLuxIndoorStart */
        20480,
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
        1800,
        /* nCctMaxInner */
        8000,
        /* nCctMinOuter */
        1500,
        /* nCctMaxOuter */
        12000,
        /* nCctSplitHtoA */
        2700,
        /* nCctSplitAtoF */
        3300,
        /* nCctSplitFtoD5 */
        4600,
        /* nCctSplitD5toD6 */
        5400,
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
            {200, 200, 100, 50, 20, 10, 10, 10, /*0 - 7*/},
            {600, 500, 400, 400, 200, 100, 100, 100, /*0 - 7*/},
            {600, 500, 400, 400, 200, 100, 100, 100, /*0 - 7*/},
            {300, 300, 100, 50, 20, 10, 10, 10, /*0 - 7*/},
            {500, 300, 150, 150, 80, 30, 30, 30, /*0 - 7*/},
            {700, 700, 500, 400, 200, 100, 100, 100, /*0 - 7*/},
            {700, 700, 500, 400, 200, 100, 100, 100, /*0 - 7*/},
            {500, 300, 150, 150, 80, 30, 30, 30, /*0 - 7*/},
            {200, 200, 200, 100, 100, 50, 50, 50, /*0 - 7*/},
            {700, 700, 700, 700, 600, 500, 500, 500, /*0 - 7*/},
            {800, 900, 900, 800, 700, 600, 600, 600, /*0 - 7*/},
            {200, 100, 100, 50, 50, 50, 50, 50, /*0 - 7*/},
            {300, 200, 200, 200, 100, 100, 100, 100, /*0 - 7*/},
            {800, 800, 800, 800, 800, 800, 800, 800, /*0 - 7*/},
            {900, 1000, 1000, 1000, 900, 900, 900, 900, /*0 - 7*/},
            {400, 200, 200, 100, 50, 50, 50, 50, /*0 - 7*/},
            {200, 300, 400, 400, 300, 200, 200, 200, /*0 - 7*/},
            {800, 800, 800, 900, 900, 900, 900, 900, /*0 - 7*/},
            {900, 1000, 1000, 1000, 900, 900, 900, 900, /*0 - 7*/},
            {300, 200, 100, 80, 50, 50, 50, 50, /*0 - 7*/},
            {200, 200, 200, 200, 200, 100, 100, 100, /*0 - 7*/},
            {400, 500, 500, 500, 500, 500, 500, 500, /*0 - 7*/},
            {400, 500, 500, 500, 500, 500, 500, 500, /*0 - 7*/},
            {200, 300, 300, 300, 100, 50, 50, 50, /*0 - 7*/},
        },
        /* nExtIlllumLuxWeight[32][8] */
        {
            {1000, 1000, 900, 500, 100, 0, 0, 0, /*0 - 7*/},
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
            {256, 512, 512, 1024, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 102, 205, 512, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 0, 0, 512, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 0, 0, 512, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 0, 0, 512, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 0, 0, 512, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 0, 0, 512, 1024, 1024, 1024, 820, /*0 - 7*/},
            {0, 0, 0, 512, 1024, 1024, 1024, 820, /*0 - 7*/},
        },
        /* bMixLightEn */
        1,
        /* nMixLightProba_0_CctStd[8] */
        {400, 400, 400, 450, 600, 9998, 9998, 9998, /*0 - 7*/},
        /* nMixLightProba_100_CctStd[8] */
        {600, 700, 800, 800, 1000, 9999, 9999, 9999, /*0 - 7*/},
        /* nMixLightProba_100_SatDiscnt[8] */
        {100, 100, 100, 100, 100, 100, 100, 100, /*0 - 7*/},
        /* nMixLightKneeNum */
        8,
        /* nMixLightKneeCctList[32] */
        {2300, 2800, 3500, 4600, 5500, 6500, 7500, 8500, /*0 - 7*/},
        /* nMixLightKneeWtList[8][32] */
        {
            {820, 820, 820, 1024, 1024, 820, 410, 358, /*0 - 7*/},
            {614, 614, 614, 1024, 1024, 820, 410, 358, /*0 - 7*/},
            {205, 205, 307, 820, 1024, 820, 410, 358, /*0 - 7*/},
            {205, 205, 410, 922, 1024, 820, 410, 358, /*0 - 7*/},
            {307, 307, 512, 1024, 1024, 820, 410, 358, /*0 - 7*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 7*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 7*/},
            {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, /*0 - 7*/},
        },
        /* tDomParamList[4] */
        {
            /* 0 */
            {
                /* nDominantEnable */
                0,
                /* nDomMinCctThresh */
                2000,
                /* nDomMaxCctThresh */
                2500,
                /* nDom2AllRatioThresh */
                1024,
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
                0,
                /* nDomMinCctThresh */
                2700,
                /* nDomMaxCctThresh */
                3300,
                /* nDom2AllRatioThresh */
                1024,
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
                0,
                /* nDomMinCctThresh */
                3600,
                /* nDomMaxCctThresh */
                4400,
                /* nDom2AllRatioThresh */
                1024,
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
                0,
                /* nDomMinCctThresh */
                4600,
                /* nDomMaxCctThresh */
                7500,
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
        {1800, 2300, 2800, 3800, 4100, 5000, 6500, 7500, 10000, 12000, /*0 - 9*/},
        /* nPreferDstCct[8][32] */
        {
            {1800, 2200, 2800, 3850, 4170, 5100, 6600, 7500, 10000, 12000, /*0 - 9*/},
            {1800, 2200, 2800, 3800, 4100, 5000, 6500, 7500, 10000, 12000, /*0 - 9*/},
            {1800, 2300, 2800, 3800, 4100, 5000, 6500, 7500, 10000, 12000, /*0 - 9*/},
            {1800, 2300, 2800, 3800, 4100, 5000, 6500, 7500, 10000, 12000, /*0 - 9*/},
            {1800, 2300, 2800, 3800, 4100, 5000, 6500, 7500, 10000, 12000, /*0 - 9*/},
            {1800, 2300, 2800, 3800, 4100, 5000, 6500, 7500, 10000, 12000, /*0 - 9*/},
            {1800, 2300, 2800, 3800, 4100, 5000, 6500, 7500, 10000, 12000, /*0 - 9*/},
            {1800, 2300, 2800, 3800, 4100, 5000, 6500, 7500, 10000, 12000, /*0 - 9*/},
        },
        /* nPreferGrShift[8][32] */
        {
            {0, 0, 0, 0, 0, -10485, -10485, -10485, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, -10485, -10485, -10485, 0, 0, /*0 - 9*/},
            {0, 0, 0, 0, 0, -10485, -10485, -10485, 0, 0, /*0 - 9*/},
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
            4128,
            /* nSblGrValue */
            4128,
            /* nSblGbValue */
            4128,
            /* nSblBValue */
            4128,
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
            12,
            /* nExposeTimeGrpNum */
            2,
            /* nGain[16] */
            {1024, 2048, 4096, 8192, 16384, 24576, 32768, 55296, 131072, 262144, 524288, 1048576, /*0 - 11*/},
            /* nExposeTime[10] */
            {1000, 5000, /*0 - 1*/},
            /* nAutoSblRValue[16][10] */
            {
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblGrValue[16][10] */
            {
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblGbValue[16][10] */
            {
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
            /* nAutoSblBValue[16][10] */
            {
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
            },
        },
        /* tLcgAutoTable */
        {
            /* nGainGrpNum */
            12,
            /* nExposeTimeGrpNum */
            2,
            /* nGain[16] */
            {1024, 2048, 4096, 8192, 16384, 24576, 32768, 55296, 131072, 262144, 524288, 1048576, /*0 - 11*/},
            /* nExposeTime[10] */
            {1000, 5000, /*0 - 1*/},
            /* nAutoSblRValue[16][10] */
            {
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4112, 4112, /*0 - 1*/},
                {4105, 4105, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4115, 4115, /*0 - 1*/},
            },
            /* nAutoSblGrValue[16][10] */
            {
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4112, 4112, /*0 - 1*/},
                {4105, 4105, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4115, 4115, /*0 - 1*/},
            },
            /* nAutoSblGbValue[16][10] */
            {
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4112, 4112, /*0 - 1*/},
                {4105, 4105, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4115, 4115, /*0 - 1*/},
            },
            /* nAutoSblBValue[16][10] */
            {
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4112, 4112, /*0 - 1*/},
                {4105, 4105, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4128, 4128, /*0 - 1*/},
                {4115, 4115, /*0 - 1*/},
            },
        },
    },
};

#endif
