#ifndef __SC850SL_SDR_H__
#define __SC850SL_SDR_H__

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
        1,
        /* nSnsTotalAGain */
        1024,
        /* nSysTotalGain */
        1024,
        /* nShutter */
        9999,
        /* nIrisPwmDuty */
        0,
        /* nHdrRealRatioLtoS */
        1024,
        /* nHdrRealRatioStoVS */
        1024,
        /* nSetPoint */
        12288,
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
        39888,
        /* nRealMaxShutter */
        39888,
    },
    /* tAeAlgAuto */
    {
        /* nSetPoint */
        12288,
        /* nFaceTarget */
        35840,
        /* nTolerance */
        10485760,
        /* nAgainLcg2HcgTh */
        6451,
        /* nAgainHcg2LcgTh */
        1229,
        /* nAgainLcg2HcgRatio */
        4301,
        /* nAgainHcg2LcgRatio */
        4301,
        /* nLuxk */
        61052,
        /* nCompensationMode */
        1,
        /* nMaxIspGain */
        71680,
        /* nMinIspGain */
        1024,
        /* nMaxUserDgain */
        15872,
        /* nMinUserDgain */
        1024,
        /* nMaxUserTotalAgain */
        69632,
        /* nMinUserTotalAgain */
        1024,
        /* nMaxUserSysGain */
        4874240,
        /* nMinUserSysGain */
        1024,
        /* nMaxShutter */
        39888,
        /* nMinShutter */
        27,
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
            2,
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
                            4874240,
                            /* nIncrementPriority */
                            0,
                        },
                    },
                },
                /* 1 */
                {
                    /* sTableName[32] */
                    "",
                    /* nRouteCurveNum */
                    2,
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
                            39888,
                            /* nGain */
                            1024000,
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
            8,
            /* nRefList[10] */
            {102, 512, 1024, 10240, 51200, 102400, 409600, 2048000, /*0 - 7*/},
            /* nSetPointList[10] */
            {12288, 14336, 17408, 20480, 22528, 24576, 27648, 30720, /*0 - 7*/},
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
                0,
                /* nShortSatLuma */
                153600,
                /* nTolerance */
                10485760,
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
                3,
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
                {51, 51, 82, 102, 123, 123, 123, 123, 154, 154, 154, /*0 - 10*/},
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
                    240000,
                    /* nLuxEnd */
                    4000000,
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
            449,
            /* nGainGr */
            256,
            /* nGainGb */
            256,
            /* nGainB */
            524,
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
            1367152,
            /* nBg */
            1465871,
        },
        /* nCenterPntRadius */
        1238559,
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
            -489,
            /* nB */
            1084,
        },
        /* tChordPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1731209,
                /* nBg */
                282025,
            },
            /* 1 */
            {
                /* nRg */
                1650102,
                /* nBg */
                320833,
            },
            /* 2 */
            {
                /* nRg */
                1576157,
                /* nBg */
                356212,
            },
            /* 3 */
            {
                /* nRg */
                1508062,
                /* nBg */
                388791,
            },
            /* 4 */
            {
                /* nRg */
                1444749,
                /* nBg */
                419074,
            },
            /* 5 */
            {
                /* nRg */
                1385389,
                /* nBg */
                447480,
            },
            /* 6 */
            {
                /* nRg */
                1329280,
                /* nBg */
                474323,
            },
            /* 7 */
            {
                /* nRg */
                1275823,
                /* nBg */
                499898,
            },
            /* 8 */
            {
                /* nRg */
                1224527,
                /* nBg */
                524445,
            },
            /* 9 */
            {
                /* nRg */
                1203692,
                /* nBg */
                534407,
            },
            /* 10 */
            {
                /* nRg */
                1183119,
                /* nBg */
                544253,
            },
            /* 11 */
            {
                /* nRg */
                1162797,
                /* nBg */
                553973,
            },
            /* 12 */
            {
                /* nRg */
                1142686,
                /* nBg */
                563599,
            },
            /* 13 */
            {
                /* nRg */
                1122773,
                /* nBg */
                573131,
            },
            /* 14 */
            {
                /* nRg */
                1103018,
                /* nBg */
                582578,
            },
            /* 15 */
            {
                /* nRg */
                1083420,
                /* nBg */
                591953,
            },
            /* 16 */
            {
                /* nRg */
                1063938,
                /* nBg */
                601274,
            },
            /* 17 */
            {
                /* nRg */
                1039915,
                /* nBg */
                612767,
            },
            /* 18 */
            {
                /* nRg */
                1016007,
                /* nBg */
                624207,
            },
            /* 19 */
            {
                /* nRg */
                992163,
                /* nBg */
                635615,
            },
            /* 20 */
            {
                /* nRg */
                968360,
                /* nBg */
                647003,
            },
            /* 21 */
            {
                /* nRg */
                944536,
                /* nBg */
                658401,
            },
            /* 22 */
            {
                /* nRg */
                920671,
                /* nBg */
                669820,
            },
            /* 23 */
            {
                /* nRg */
                896711,
                /* nBg */
                681281,
            },
            /* 24 */
            {
                /* nRg */
                872625,
                /* nBg */
                692805,
            },
            /* 25 */
            {
                /* nRg */
                862234,
                /* nBg */
                697775,
            },
            /* 26 */
            {
                /* nRg */
                851811,
                /* nBg */
                702766,
            },
            /* 27 */
            {
                /* nRg */
                841346,
                /* nBg */
                707778,
            },
            /* 28 */
            {
                /* nRg */
                830839,
                /* nBg */
                712801,
            },
            /* 29 */
            {
                /* nRg */
                820280,
                /* nBg */
                717855,
            },
            /* 30 */
            {
                /* nRg */
                809679,
                /* nBg */
                722920,
            },
            /* 31 */
            {
                /* nRg */
                799025,
                /* nBg */
                728026,
            },
            /* 32 */
            {
                /* nRg */
                788309,
                /* nBg */
                733143,
            },
            /* 33 */
            {
                /* nRg */
                779260,
                /* nBg */
                737474,
            },
            /* 34 */
            {
                /* nRg */
                770169,
                /* nBg */
                741826,
            },
            /* 35 */
            {
                /* nRg */
                761025,
                /* nBg */
                746198,
            },
            /* 36 */
            {
                /* nRg */
                751839,
                /* nBg */
                750602,
            },
            /* 37 */
            {
                /* nRg */
                742591,
                /* nBg */
                755017,
            },
            /* 38 */
            {
                /* nRg */
                733301,
                /* nBg */
                759473,
            },
            /* 39 */
            {
                /* nRg */
                723937,
                /* nBg */
                763940,
            },
            /* 40 */
            {
                /* nRg */
                714531,
                /* nBg */
                768449,
            },
            /* 41 */
            {
                /* nRg */
                705492,
                /* nBg */
                772769,
            },
            /* 42 */
            {
                /* nRg */
                696391,
                /* nBg */
                777121,
            },
            /* 43 */
            {
                /* nRg */
                687237,
                /* nBg */
                781504,
            },
            /* 44 */
            {
                /* nRg */
                678020,
                /* nBg */
                785918,
            },
            /* 45 */
            {
                /* nRg */
                668729,
                /* nBg */
                790364,
            },
            /* 46 */
            {
                /* nRg */
                659376,
                /* nBg */
                794831,
            },
            /* 47 */
            {
                /* nRg */
                649949,
                /* nBg */
                799350,
            },
            /* 48 */
            {
                /* nRg */
                640439,
                /* nBg */
                803891,
            },
            /* 49 */
            {
                /* nRg */
                596682,
                /* nBg */
                824831,
            },
            /* 50 */
            {
                /* nRg */
                551079,
                /* nBg */
                846652,
            },
            /* 51 */
            {
                /* nRg */
                503327,
                /* nBg */
                869500,
            },
            /* 52 */
            {
                /* nRg */
                453037,
                /* nBg */
                893555,
            },
            /* 53 */
            {
                /* nRg */
                399812,
                /* nBg */
                919024,
            },
            /* 54 */
            {
                /* nRg */
                343136,
                /* nBg */
                946141,
            },
            /* 55 */
            {
                /* nRg */
                282413,
                /* nBg */
                975186,
            },
            /* 56 */
            {
                /* nRg */
                216919,
                /* nBg */
                1006518,
            },
        },
        /* tArcPointList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1706547,
                /* nBg */
                362210,
            },
            /* 1 */
            {
                /* nRg */
                1645404,
                /* nBg */
                339823,
            },
            /* 2 */
            {
                /* nRg */
                1582815,
                /* nBg */
                320843,
            },
            /* 3 */
            {
                /* nRg */
                1518967,
                /* nBg */
                305356,
            },
            /* 4 */
            {
                /* nRg */
                1454071,
                /* nBg */
                293444,
            },
            /* 5 */
            {
                /* nRg */
                1388304,
                /* nBg */
                285171,
            },
            /* 6 */
            {
                /* nRg */
                1321877,
                /* nBg */
                280609,
            },
            /* 7 */
            {
                /* nRg */
                1255020,
                /* nBg */
                279792,
            },
            /* 8 */
            {
                /* nRg */
                1187921,
                /* nBg */
                282759,
            },
            /* 9 */
            {
                /* nRg */
                1159410,
                /* nBg */
                282098,
            },
            /* 10 */
            {
                /* nRg */
                1130784,
                /* nBg */
                282140,
            },
            /* 11 */
            {
                /* nRg */
                1102043,
                /* nBg */
                282864,
            },
            /* 12 */
            {
                /* nRg */
                1073207,
                /* nBg */
                284300,
            },
            /* 13 */
            {
                /* nRg */
                1044287,
                /* nBg */
                286429,
            },
            /* 14 */
            {
                /* nRg */
                1015315,
                /* nBg */
                289281,
            },
            /* 15 */
            {
                /* nRg */
                986301,
                /* nBg */
                292836,
            },
            /* 16 */
            {
                /* nRg */
                957256,
                /* nBg */
                297104,
            },
            /* 17 */
            {
                /* nRg */
                923565,
                /* nBg */
                309466,
            },
            /* 18 */
            {
                /* nRg */
                890252,
                /* nBg */
                322804,
            },
            /* 19 */
            {
                /* nRg */
                857337,
                /* nBg */
                337096,
            },
            /* 20 */
            {
                /* nRg */
                824852,
                /* nBg */
                352343,
            },
            /* 21 */
            {
                /* nRg */
                792828,
                /* nBg */
                368522,
            },
            /* 22 */
            {
                /* nRg */
                761277,
                /* nBg */
                385624,
            },
            /* 23 */
            {
                /* nRg */
                730239,
                /* nBg */
                403628,
            },
            /* 24 */
            {
                /* nRg */
                699725,
                /* nBg */
                422524,
            },
            /* 25 */
            {
                /* nRg */
                686807,
                /* nBg */
                430902,
            },
            /* 26 */
            {
                /* nRg */
                673983,
                /* nBg */
                439448,
            },
            /* 27 */
            {
                /* nRg */
                661263,
                /* nBg */
                448151,
            },
            /* 28 */
            {
                /* nRg */
                648660,
                /* nBg */
                457011,
            },
            /* 29 */
            {
                /* nRg */
                636171,
                /* nBg */
                466019,
            },
            /* 30 */
            {
                /* nRg */
                623787,
                /* nBg */
                475194,
            },
            /* 31 */
            {
                /* nRg */
                611530,
                /* nBg */
                484516,
            },
            /* 32 */
            {
                /* nRg */
                599377,
                /* nBg */
                493984,
            },
            /* 33 */
            {
                /* nRg */
                589258,
                /* nBg */
                502069,
            },
            /* 34 */
            {
                /* nRg */
                579233,
                /* nBg */
                510248,
            },
            /* 35 */
            {
                /* nRg */
                569282,
                /* nBg */
                518542,
            },
            /* 36 */
            {
                /* nRg */
                559426,
                /* nBg */
                526930,
            },
            /* 37 */
            {
                /* nRg */
                549653,
                /* nBg */
                535424,
            },
            /* 38 */
            {
                /* nRg */
                539975,
                /* nBg */
                544022,
            },
            /* 39 */
            {
                /* nRg */
                530380,
                /* nBg */
                552715,
            },
            /* 40 */
            {
                /* nRg */
                520880,
                /* nBg */
                561512,
            },
            /* 41 */
            {
                /* nRg */
                511915,
                /* nBg */
                569995,
            },
            /* 42 */
            {
                /* nRg */
                503023,
                /* nBg */
                578562,
            },
            /* 43 */
            {
                /* nRg */
                494225,
                /* nBg */
                587224,
            },
            /* 44 */
            {
                /* nRg */
                485501,
                /* nBg */
                595969,
            },
            /* 45 */
            {
                /* nRg */
                476882,
                /* nBg */
                604798,
            },
            /* 46 */
            {
                /* nRg */
                468336,
                /* nBg */
                613721,
            },
            /* 47 */
            {
                /* nRg */
                459884,
                /* nBg */
                622718,
            },
            /* 48 */
            {
                /* nRg */
                451527,
                /* nBg */
                631809,
            },
            /* 49 */
            {
                /* nRg */
                415047,
                /* nBg */
                673710,
            },
            /* 50 */
            {
                /* nRg */
                380486,
                /* nBg */
                717195,
            },
            /* 51 */
            {
                /* nRg */
                347897,
                /* nBg */
                762199,
            },
            /* 52 */
            {
                /* nRg */
                317372,
                /* nBg */
                808609,
            },
            /* 53 */
            {
                /* nRg */
                288956,
                /* nBg */
                856351,
            },
            /* 54 */
            {
                /* nRg */
                262700,
                /* nBg */
                905309,
            },
            /* 55 */
            {
                /* nRg */
                238677,
                /* nBg */
                955400,
            },
            /* 56 */
            {
                /* nRg */
                216919,
                /* nBg */
                1006518,
            },
        },
        /* tRadiusLineList[512] */
        {
            /* 0 */
            {
                /* nK */
                -3329,
                /* nB */
                5773,
            },
            /* 1 */
            {
                /* nK */
                -4143,
                /* nB */
                6834,
            },
            /* 2 */
            {
                /* nK */
                -5436,
                /* nB */
                8520,
            },
            /* 3 */
            {
                /* nK */
                -7826,
                /* nB */
                11637,
            },
            /* 4 */
            {
                /* nK */
                -13812,
                /* nB */
                19441,
            },
            /* 5 */
            {
                /* nK */
                -57168,
                /* nB */
                75970,
            },
            /* 6 */
            {
                /* nK */
                26810,
                /* nB */
                -33522,
            },
            /* 7 */
            {
                /* nK */
                10831,
                /* nB */
                -12689,
            },
            /* 8 */
            {
                /* nK */
                6759,
                /* nB */
                -7380,
            },
            /* 9 */
            {
                /* nK */
                5835,
                /* nB */
                -6175,
            },
            /* 10 */
            {
                /* nK */
                5128,
                /* nB */
                -5254,
            },
            /* 11 */
            {
                /* nK */
                4569,
                /* nB */
                -4525,
            },
            /* 12 */
            {
                /* nK */
                4116,
                /* nB */
                -3934,
            },
            /* 13 */
            {
                /* nK */
                3741,
                /* nB */
                -3445,
            },
            /* 14 */
            {
                /* nK */
                3424,
                /* nB */
                -3032,
            },
            /* 15 */
            {
                /* nK */
                3154,
                /* nB */
                -2680,
            },
            /* 16 */
            {
                /* nK */
                2920,
                /* nB */
                -2374,
            },
            /* 17 */
            {
                /* nK */
                2670,
                /* nB */
                -2048,
            },
            /* 18 */
            {
                /* nK */
                2454,
                /* nB */
                -1768,
            },
            /* 19 */
            {
                /* nK */
                2267,
                /* nB */
                -1524,
            },
            /* 20 */
            {
                /* nK */
                2103,
                /* nB */
                -1309,
            },
            /* 21 */
            {
                /* nK */
                1957,
                /* nB */
                -1118,
            },
            /* 22 */
            {
                /* nK */
                1826,
                /* nB */
                -948,
            },
            /* 23 */
            {
                /* nK */
                1708,
                /* nB */
                -794,
            },
            /* 24 */
            {
                /* nK */
                1601,
                /* nB */
                -655,
            },
            /* 25 */
            {
                /* nK */
                1558,
                /* nB */
                -598,
            },
            /* 26 */
            {
                /* nK */
                1516,
                /* nB */
                -544,
            },
            /* 27 */
            {
                /* nK */
                1476,
                /* nB */
                -492,
            },
            /* 28 */
            {
                /* nK */
                1438,
                /* nB */
                -442,
            },
            /* 29 */
            {
                /* nK */
                1401,
                /* nB */
                -394,
            },
            /* 30 */
            {
                /* nK */
                1365,
                /* nB */
                -347,
            },
            /* 31 */
            {
                /* nK */
                1330,
                /* nB */
                -301,
            },
            /* 32 */
            {
                /* nK */
                1296,
                /* nB */
                -258,
            },
            /* 33 */
            {
                /* nK */
                1269,
                /* nB */
                -222,
            },
            /* 34 */
            {
                /* nK */
                1242,
                /* nB */
                -187,
            },
            /* 35 */
            {
                /* nK */
                1216,
                /* nB */
                -153,
            },
            /* 36 */
            {
                /* nK */
                1190,
                /* nB */
                -119,
            },
            /* 37 */
            {
                /* nK */
                1165,
                /* nB */
                -87,
            },
            /* 38 */
            {
                /* nK */
                1141,
                /* nB */
                -55,
            },
            /* 39 */
            {
                /* nK */
                1117,
                /* nB */
                -24,
            },
            /* 40 */
            {
                /* nK */
                1094,
                /* nB */
                5,
            },
            /* 41 */
            {
                /* nK */
                1073,
                /* nB */
                33,
            },
            /* 42 */
            {
                /* nK */
                1051,
                /* nB */
                61,
            },
            /* 43 */
            {
                /* nK */
                1031,
                /* nB */
                88,
            },
            /* 44 */
            {
                /* nK */
                1010,
                /* nB */
                114,
            },
            /* 45 */
            {
                /* nK */
                990,
                /* nB */
                140,
            },
            /* 46 */
            {
                /* nK */
                971,
                /* nB */
                166,
            },
            /* 47 */
            {
                /* nK */
                952,
                /* nB */
                191,
            },
            /* 48 */
            {
                /* nK */
                933,
                /* nB */
                215,
            },
            /* 49 */
            {
                /* nK */
                852,
                /* nB */
                321,
            },
            /* 50 */
            {
                /* nK */
                777,
                /* nB */
                418,
            },
            /* 51 */
            {
                /* nK */
                707,
                /* nB */
                510,
            },
            /* 52 */
            {
                /* nK */
                641,
                /* nB */
                596,
            },
            /* 53 */
            {
                /* nK */
                579,
                /* nB */
                677,
            },
            /* 54 */
            {
                /* nK */
                520,
                /* nB */
                754,
            },
            /* 55 */
            {
                /* nK */
                463,
                /* nB */
                828,
            },
            /* 56 */
            {
                /* nK */
                409,
                /* nB */
                898,
            },
        },
        /* tInLeftBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1734292,
                /* nBg */
                272001,
            },
            /* 1 */
            {
                /* nRg */
                1668043,
                /* nBg */
                248208,
            },
            /* 2 */
            {
                /* nRg */
                1600284,
                /* nBg */
                228107,
            },
            /* 3 */
            {
                /* nRg */
                1531215,
                /* nBg */
                211781,
            },
            /* 4 */
            {
                /* nRg */
                1461044,
                /* nBg */
                199334,
            },
            /* 5 */
            {
                /* nRg */
                1389992,
                /* nBg */
                190820,
            },
            /* 6 */
            {
                /* nRg */
                1318280,
                /* nBg */
                186300,
            },
            /* 7 */
            {
                /* nRg */
                1246128,
                /* nBg */
                185829,
            },
            /* 8 */
            {
                /* nRg */
                1173786,
                /* nBg */
                189446,
            },
            /* 9 */
            {
                /* nRg */
                1143105,
                /* nBg */
                189153,
            },
            /* 10 */
            {
                /* nRg */
                1112308,
                /* nBg */
                189593,
            },
            /* 11 */
            {
                /* nRg */
                1081407,
                /* nBg */
                190778,
            },
            /* 12 */
            {
                /* nRg */
                1050421,
                /* nBg */
                192718,
            },
            /* 13 */
            {
                /* nRg */
                1019373,
                /* nBg */
                195413,
            },
            /* 14 */
            {
                /* nRg */
                988283,
                /* nBg */
                198862,
            },
            /* 15 */
            {
                /* nRg */
                957161,
                /* nBg */
                203078,
            },
            /* 16 */
            {
                /* nRg */
                926029,
                /* nBg */
                208048,
            },
            /* 17 */
            {
                /* nRg */
                889769,
                /* nBg */
                221354,
            },
            /* 18 */
            {
                /* nRg */
                853918,
                /* nBg */
                235709,
            },
            /* 19 */
            {
                /* nRg */
                818497,
                /* nBg */
                251092,
            },
            /* 20 */
            {
                /* nRg */
                783538,
                /* nBg */
                267502,
            },
            /* 21 */
            {
                /* nRg */
                749061,
                /* nBg */
                284909,
            },
            /* 22 */
            {
                /* nRg */
                715108,
                /* nBg */
                303311,
            },
            /* 23 */
            {
                /* nRg */
                681700,
                /* nBg */
                322689,
            },
            /* 24 */
            {
                /* nRg */
                648869,
                /* nBg */
                343021,
            },
            /* 25 */
            {
                /* nRg */
                634965,
                /* nBg */
                352049,
            },
            /* 26 */
            {
                /* nRg */
                621166,
                /* nBg */
                361234,
            },
            /* 27 */
            {
                /* nRg */
                607482,
                /* nBg */
                370609,
            },
            /* 28 */
            {
                /* nRg */
                593924,
                /* nBg */
                380140,
            },
            /* 29 */
            {
                /* nRg */
                580471,
                /* nBg */
                389840,
            },
            /* 30 */
            {
                /* nRg */
                567154,
                /* nBg */
                399707,
            },
            /* 31 */
            {
                /* nRg */
                553952,
                /* nBg */
                409742,
            },
            /* 32 */
            {
                /* nRg */
                540876,
                /* nBg */
                419934,
            },
            /* 33 */
            {
                /* nRg */
                529992,
                /* nBg */
                428626,
            },
            /* 34 */
            {
                /* nRg */
                519192,
                /* nBg */
                437434,
            },
            /* 35 */
            {
                /* nRg */
                508496,
                /* nBg */
                446358,
            },
            /* 36 */
            {
                /* nRg */
                497885,
                /* nBg */
                455386,
            },
            /* 37 */
            {
                /* nRg */
                487368,
                /* nBg */
                464530,
            },
            /* 38 */
            {
                /* nRg */
                476945,
                /* nBg */
                473778,
            },
            /* 39 */
            {
                /* nRg */
                466627,
                /* nBg */
                483142,
            },
            /* 40 */
            {
                /* nRg */
                456403,
                /* nBg */
                492611,
            },
            /* 41 */
            {
                /* nRg */
                446746,
                /* nBg */
                501733,
            },
            /* 42 */
            {
                /* nRg */
                437183,
                /* nBg */
                510961,
            },
            /* 43 */
            {
                /* nRg */
                427704,
                /* nBg */
                520272,
            },
            /* 44 */
            {
                /* nRg */
                418329,
                /* nBg */
                529688,
            },
            /* 45 */
            {
                /* nRg */
                409039,
                /* nBg */
                539188,
            },
            /* 46 */
            {
                /* nRg */
                399853,
                /* nBg */
                548793,
            },
            /* 47 */
            {
                /* nRg */
                390762,
                /* nBg */
                558472,
            },
            /* 48 */
            {
                /* nRg */
                381766,
                /* nBg */
                568255,
            },
            /* 49 */
            {
                /* nRg */
                342507,
                /* nBg */
                613344,
            },
            /* 50 */
            {
                /* nRg */
                305303,
                /* nBg */
                660152,
            },
            /* 51 */
            {
                /* nRg */
                270239,
                /* nBg */
                708586,
            },
            /* 52 */
            {
                /* nRg */
                237387,
                /* nBg */
                758529,
            },
            /* 53 */
            {
                /* nRg */
                206800,
                /* nBg */
                809910,
            },
            /* 54 */
            {
                /* nRg */
                178552,
                /* nBg */
                862601,
            },
            /* 55 */
            {
                /* nRg */
                152694,
                /* nBg */
                916508,
            },
            /* 56 */
            {
                /* nRg */
                129279,
                /* nBg */
                971527,
            },
        },
        /* tInRightBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1681895,
                /* nBg */
                442384,
            },
            /* 1 */
            {
                /* nRg */
                1625282,
                /* nBg */
                421265,
            },
            /* 2 */
            {
                /* nRg */
                1567286,
                /* nBg */
                403282,
            },
            /* 3 */
            {
                /* nRg */
                1508093,
                /* nBg */
                388539,
            },
            /* 4 */
            {
                /* nRg */
                1447863,
                /* nBg */
                377099,
            },
            /* 5 */
            {
                /* nRg */
                1386794,
                /* nBg */
                369046,
            },
            /* 6 */
            {
                /* nRg */
                1325085,
                /* nBg */
                364433,
            },
            /* 7 */
            {
                /* nRg */
                1262915,
                /* nBg */
                363300,
            },
            /* 8 */
            {
                /* nRg */
                1200483,
                /* nBg */
                365691,
            },
            /* 9 */
            {
                /* nRg */
                1173912,
                /* nBg */
                364726,
            },
            /* 10 */
            {
                /* nRg */
                1147205,
                /* nBg */
                364401,
            },
            /* 11 */
            {
                /* nRg */
                1120382,
                /* nBg */
                364726,
            },
            /* 12 */
            {
                /* nRg */
                1093455,
                /* nBg */
                365701,
            },
            /* 13 */
            {
                /* nRg */
                1066433,
                /* nBg */
                367337,
            },
            /* 14 */
            {
                /* nRg */
                1039349,
                /* nBg */
                369644,
            },
            /* 15 */
            {
                /* nRg */
                1012201,
                /* nBg */
                372622,
            },
            /* 16 */
            {
                /* nRg */
                985022,
                /* nBg */
                376261,
            },
            /* 17 */
            {
                /* nRg */
                953617,
                /* nBg */
                387795,
            },
            /* 18 */
            {
                /* nRg */
                922558,
                /* nBg */
                400220,
            },
            /* 19 */
            {
                /* nRg */
                891866,
                /* nBg */
                413548,
            },
            /* 20 */
            {
                /* nRg */
                861583,
                /* nBg */
                427767,
            },
            /* 21 */
            {
                /* nRg */
                831720,
                /* nBg */
                442845,
            },
            /* 22 */
            {
                /* nRg */
                802307,
                /* nBg */
                458783,
            },
            /* 23 */
            {
                /* nRg */
                773367,
                /* nBg */
                475571,
            },
            /* 24 */
            {
                /* nRg */
                744929,
                /* nBg */
                493187,
            },
            /* 25 */
            {
                /* nRg */
                732881,
                /* nBg */
                500999,
            },
            /* 26 */
            {
                /* nRg */
                720927,
                /* nBg */
                508968,
            },
            /* 27 */
            {
                /* nRg */
                709079,
                /* nBg */
                517074,
            },
            /* 28 */
            {
                /* nRg */
                697324,
                /* nBg */
                525337,
            },
            /* 29 */
            {
                /* nRg */
                685685,
                /* nBg */
                533736,
            },
            /* 30 */
            {
                /* nRg */
                674140,
                /* nBg */
                542292,
            },
            /* 31 */
            {
                /* nRg */
                662700,
                /* nBg */
                550985,
            },
            /* 32 */
            {
                /* nRg */
                651375,
                /* nBg */
                559814,
            },
            /* 33 */
            {
                /* nRg */
                641949,
                /* nBg */
                567343,
            },
            /* 34 */
            {
                /* nRg */
                632595,
                /* nBg */
                574976,
            },
            /* 35 */
            {
                /* nRg */
                623326,
                /* nBg */
                582704,
            },
            /* 36 */
            {
                /* nRg */
                614130,
                /* nBg */
                590527,
            },
            /* 37 */
            {
                /* nRg */
                605028,
                /* nBg */
                598443,
            },
            /* 38 */
            {
                /* nRg */
                596000,
                /* nBg */
                606454,
            },
            /* 39 */
            {
                /* nRg */
                587056,
                /* nBg */
                614560,
            },
            /* 40 */
            {
                /* nRg */
                578206,
                /* nBg */
                622770,
            },
            /* 41 */
            {
                /* nRg */
                569838,
                /* nBg */
                630677,
            },
            /* 42 */
            {
                /* nRg */
                561554,
                /* nBg */
                638656,
            },
            /* 43 */
            {
                /* nRg */
                553344,
                /* nBg */
                646730,
            },
            /* 44 */
            {
                /* nRg */
                545218,
                /* nBg */
                654888,
            },
            /* 45 */
            {
                /* nRg */
                537175,
                /* nBg */
                663119,
            },
            /* 46 */
            {
                /* nRg */
                529216,
                /* nBg */
                671435,
            },
            /* 47 */
            {
                /* nRg */
                521342,
                /* nBg */
                679823,
            },
            /* 48 */
            {
                /* nRg */
                513540,
                /* nBg */
                688296,
            },
            /* 49 */
            {
                /* nRg */
                479535,
                /* nBg */
                727355,
            },
            /* 50 */
            {
                /* nRg */
                447312,
                /* nBg */
                767904,
            },
            /* 51 */
            {
                /* nRg */
                416935,
                /* nBg */
                809857,
            },
            /* 52 */
            {
                /* nRg */
                388476,
                /* nBg */
                853132,
            },
            /* 53 */
            {
                /* nRg */
                361979,
                /* nBg */
                897633,
            },
            /* 54 */
            {
                /* nRg */
                337505,
                /* nBg */
                943278,
            },
            /* 55 */
            {
                /* nRg */
                315108,
                /* nBg */
                989982,
            },
            /* 56 */
            {
                /* nRg */
                294828,
                /* nBg */
                1037629,
            },
        },
        /* tOutLeftBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1752789,
                /* nBg */
                211865,
            },
            /* 1 */
            {
                /* nRg */
                1683143,
                /* nBg */
                187129,
            },
            /* 2 */
            {
                /* nRg */
                1611934,
                /* nBg */
                166273,
            },
            /* 3 */
            {
                /* nRg */
                1539372,
                /* nBg */
                149401,
            },
            /* 4 */
            {
                /* nRg */
                1465689,
                /* nBg */
                136588,
            },
            /* 5 */
            {
                /* nRg */
                1391114,
                /* nBg */
                127916,
            },
            /* 6 */
            {
                /* nRg */
                1315879,
                /* nBg */
                123428,
            },
            /* 7 */
            {
                /* nRg */
                1240214,
                /* nBg */
                123197,
            },
            /* 8 */
            {
                /* nRg */
                1164360,
                /* nBg */
                127245,
            },
            /* 9 */
            {
                /* nRg */
                1132231,
                /* nBg */
                127182,
            },
            /* 10 */
            {
                /* nRg */
                1099988,
                /* nBg */
                127895,
            },
            /* 11 */
            {
                /* nRg */
                1067650,
                /* nBg */
                129384,
            },
            /* 12 */
            {
                /* nRg */
                1035228,
                /* nBg */
                131659,
            },
            /* 13 */
            {
                /* nRg */
                1002764,
                /* nBg */
                134732,
            },
            /* 14 */
            {
                /* nRg */
                970258,
                /* nBg */
                138580,
            },
            /* 15 */
            {
                /* nRg */
                937731,
                /* nBg */
                143235,
            },
            /* 16 */
            {
                /* nRg */
                905204,
                /* nBg */
                148678,
            },
            /* 17 */
            {
                /* nRg */
                867235,
                /* nBg */
                162613,
            },
            /* 18 */
            {
                /* nRg */
                829696,
                /* nBg */
                177650,
            },
            /* 19 */
            {
                /* nRg */
                792598,
                /* nBg */
                193756,
            },
            /* 20 */
            {
                /* nRg */
                755992,
                /* nBg */
                210932,
            },
            /* 21 */
            {
                /* nRg */
                719889,
                /* nBg */
                229166,
            },
            /* 22 */
            {
                /* nRg */
                684332,
                /* nBg */
                248439,
            },
            /* 23 */
            {
                /* nRg */
                649352,
                /* nBg */
                268729,
            },
            /* 24 */
            {
                /* nRg */
                614969,
                /* nBg */
                290026,
            },
            /* 25 */
            {
                /* nRg */
                600404,
                /* nBg */
                299473,
            },
            /* 26 */
            {
                /* nRg */
                585955,
                /* nBg */
                309099,
            },
            /* 27 */
            {
                /* nRg */
                571631,
                /* nBg */
                318903,
            },
            /* 28 */
            {
                /* nRg */
                557423,
                /* nBg */
                328886,
            },
            /* 29 */
            {
                /* nRg */
                543341,
                /* nBg */
                339047,
            },
            /* 30 */
            {
                /* nRg */
                529395,
                /* nBg */
                349386,
            },
            /* 31 */
            {
                /* nRg */
                515574,
                /* nBg */
                359892,
            },
            /* 32 */
            {
                /* nRg */
                501880,
                /* nBg */
                370567,
            },
            /* 33 */
            {
                /* nRg */
                490482,
                /* nBg */
                379668,
            },
            /* 34 */
            {
                /* nRg */
                479168,
                /* nBg */
                388896,
            },
            /* 35 */
            {
                /* nRg */
                467958,
                /* nBg */
                398239,
            },
            /* 36 */
            {
                /* nRg */
                456854,
                /* nBg */
                407697,
            },
            /* 37 */
            {
                /* nRg */
                445844,
                /* nBg */
                417270,
            },
            /* 38 */
            {
                /* nRg */
                434928,
                /* nBg */
                426959,
            },
            /* 39 */
            {
                /* nRg */
                424118,
                /* nBg */
                436753,
            },
            /* 40 */
            {
                /* nRg */
                413412,
                /* nBg */
                446672,
            },
            /* 41 */
            {
                /* nRg */
                403303,
                /* nBg */
                456225,
            },
            /* 42 */
            {
                /* nRg */
                393289,
                /* nBg */
                465882,
            },
            /* 43 */
            {
                /* nRg */
                383370,
                /* nBg */
                475645,
            },
            /* 44 */
            {
                /* nRg */
                373545,
                /* nBg */
                485501,
            },
            /* 45 */
            {
                /* nRg */
                363824,
                /* nBg */
                495452,
            },
            /* 46 */
            {
                /* nRg */
                354198,
                /* nBg */
                505498,
            },
            /* 47 */
            {
                /* nRg */
                344677,
                /* nBg */
                515648,
            },
            /* 48 */
            {
                /* nRg */
                335251,
                /* nBg */
                525882,
            },
            /* 49 */
            {
                /* nRg */
                294136,
                /* nBg */
                573110,
            },
            /* 50 */
            {
                /* nRg */
                255181,
                /* nBg */
                622120,
            },
            /* 51 */
            {
                /* nRg */
                218460,
                /* nBg */
                672840,
            },
            /* 52 */
            {
                /* nRg */
                184057,
                /* nBg */
                725143,
            },
            /* 53 */
            {
                /* nRg */
                152033,
                /* nBg */
                778945,
            },
            /* 54 */
            {
                /* nRg */
                122453,
                /* nBg */
                834121,
            },
            /* 55 */
            {
                /* nRg */
                95368,
                /* nBg */
                890577,
            },
            /* 56 */
            {
                /* nRg */
                70852,
                /* nBg */
                948185,
            },
        },
        /* tOutRightBorderPntList[512] */
        {
            /* 0 */
            {
                /* nRg */
                1663398,
                /* nBg */
                502520,
            },
            /* 1 */
            {
                /* nRg */
                1610193,
                /* nBg */
                482334,
            },
            /* 2 */
            {
                /* nRg */
                1555646,
                /* nBg */
                465106,
            },
            /* 3 */
            {
                /* nRg */
                1499925,
                /* nBg */
                450919,
            },
            /* 4 */
            {
                /* nRg */
                1443208,
                /* nBg */
                439846,
            },
            /* 5 */
            {
                /* nRg */
                1385672,
                /* nBg */
                431950,
            },
            /* 6 */
            {
                /* nRg */
                1327487,
                /* nBg */
                427295,
            },
            /* 7 */
            {
                /* nRg */
                1268829,
                /* nBg */
                425932,
            },
            /* 8 */
            {
                /* nRg */
                1209910,
                /* nBg */
                427903,
            },
            /* 9 */
            {
                /* nRg */
                1184786,
                /* nBg */
                426697,
            },
            /* 10 */
            {
                /* nRg */
                1159526,
                /* nBg */
                426099,
            },
            /* 11 */
            {
                /* nRg */
                1134140,
                /* nBg */
                426120,
            },
            /* 12 */
            {
                /* nRg */
                1108649,
                /* nBg */
                426760,
            },
            /* 13 */
            {
                /* nRg */
                1083053,
                /* nBg */
                428029,
            },
            /* 14 */
            {
                /* nRg */
                1057374,
                /* nBg */
                429927,
            },
            /* 15 */
            {
                /* nRg */
                1031631,
                /* nBg */
                432454,
            },
            /* 16 */
            {
                /* nRg */
                1005836,
                /* nBg */
                435631,
            },
            /* 17 */
            {
                /* nRg */
                976140,
                /* nBg */
                446536,
            },
            /* 18 */
            {
                /* nRg */
                946780,
                /* nBg */
                458291,
            },
            /* 19 */
            {
                /* nRg */
                917766,
                /* nBg */
                470884,
            },
            /* 20 */
            {
                /* nRg */
                889130,
                /* nBg */
                484327,
            },
            /* 21 */
            {
                /* nRg */
                860891,
                /* nBg */
                498587,
            },
            /* 22 */
            {
                /* nRg */
                833083,
                /* nBg */
                513655,
            },
            /* 23 */
            {
                /* nRg */
                805726,
                /* nBg */
                529531,
            },
            /* 24 */
            {
                /* nRg */
                778830,
                /* nBg */
                546182,
            },
            /* 25 */
            {
                /* nRg */
                767442,
                /* nBg */
                553575,
            },
            /* 26 */
            {
                /* nRg */
                756139,
                /* nBg */
                561104,
            },
            /* 27 */
            {
                /* nRg */
                744929,
                /* nBg */
                568779,
            },
            /* 28 */
            {
                /* nRg */
                733825,
                /* nBg */
                576580,
            },
            /* 29 */
            {
                /* nRg */
                722815,
                /* nBg */
                584529,
            },
            /* 30 */
            {
                /* nRg */
                711899,
                /* nBg */
                592613,
            },
            /* 31 */
            {
                /* nRg */
                701088,
                /* nBg */
                600834,
            },
            /* 32 */
            {
                /* nRg */
                690382,
                /* nBg */
                609181,
            },
            /* 33 */
            {
                /* nRg */
                681459,
                /* nBg */
                616301,
            },
            /* 34 */
            {
                /* nRg */
                672620,
                /* nBg */
                623515,
            },
            /* 35 */
            {
                /* nRg */
                663853,
                /* nBg */
                630823,
            },
            /* 36 */
            {
                /* nRg */
                655161,
                /* nBg */
                638216,
            },
            /* 37 */
            {
                /* nRg */
                646552,
                /* nBg */
                645703,
            },
            /* 38 */
            {
                /* nRg */
                638017,
                /* nBg */
                653284,
            },
            /* 39 */
            {
                /* nRg */
                629565,
                /* nBg */
                660949,
            },
            /* 40 */
            {
                /* nRg */
                621187,
                /* nBg */
                668708,
            },
            /* 41 */
            {
                /* nRg */
                613281,
                /* nBg */
                676174,
            },
            /* 42 */
            {
                /* nRg */
                605448,
                /* nBg */
                683734,
            },
            /* 43 */
            {
                /* nRg */
                597688,
                /* nBg */
                691368,
            },
            /* 44 */
            {
                /* nRg */
                590002,
                /* nBg */
                699075,
            },
            /* 45 */
            {
                /* nRg */
                582400,
                /* nBg */
                706856,
            },
            /* 46 */
            {
                /* nRg */
                574871,
                /* nBg */
                714720,
            },
            /* 47 */
            {
                /* nRg */
                567426,
                /* nBg */
                722658,
            },
            /* 48 */
            {
                /* nRg */
                560055,
                /* nBg */
                730669,
            },
            /* 49 */
            {
                /* nRg */
                527895,
                /* nBg */
                767600,
            },
            /* 50 */
            {
                /* nRg */
                497423,
                /* nBg */
                805936,
            },
            /* 51 */
            {
                /* nRg */
                468713,
                /* nBg */
                845603,
            },
            /* 52 */
            {
                /* nRg */
                441797,
                /* nBg */
                886519,
            },
            /* 53 */
            {
                /* nRg */
                416746,
                /* nBg */
                928598,
            },
            /* 54 */
            {
                /* nRg */
                393614,
                /* nBg */
                971757,
            },
            /* 55 */
            {
                /* nRg */
                372433,
                /* nBg */
                1015902,
            },
            /* 56 */
            {
                /* nRg */
                353255,
                /* nBg */
                1060970,
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
                    1183842,
                    /* nBg */
                    255853,
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
                    961544,
                    /* nBg */
                    309330,
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
                    666894,
                    /* nBg */
                    371196,
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
                    593494,
                    /* nBg */
                    486539,
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
                    540017,
                    /* nBg */
                    581960,
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
                    462422,
                    /* nBg */
                    641729,
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
                    590348,
                    /* nBg */
                    370147,
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
                486,
                /* nGainGr */
                256,
                /* nGainGb */
                256,
                /* nGainB */
                543,
            },
            /* nDampRatio */
            100000,
        },
        /* nMode */
        0,
        /* nIndex */
        0,
        /* nDampRatio */
        838861,
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
            {800, 600, 200, 200, 0, 0, 0, 0, /*0 - 7*/},
            {600, 150, 50, 50, 0, 0, 0, 0, /*0 - 7*/},
            {100, 100, 100, 0, 0, 0, 50, 50, /*0 - 7*/},
            {700, 700, 600, 500, 400, 200, 100, 100, /*0 - 7*/},
            {700, 700, 600, 500, 400, 200, 200, 200, /*0 - 7*/},
            {100, 10, 10, 0, 0, 0, 0, 0, /*0 - 7*/},
            {100, 100, 400, 400, 450, 450, 250, 200, /*0 - 7*/},
            {500, 500, 1000, 1000, 900, 900, 500, 400, /*0 - 7*/},
            {500, 500, 1000, 1000, 900, 900, 900, 900, /*0 - 7*/},
            {400, 10, 10, 0, 0, 0, 0, 0, /*0 - 7*/},
            {100, 300, 400, 500, 500, 400, 300, 300, /*0 - 7*/},
            {400, 500, 1000, 1000, 1000, 700, 500, 450, /*0 - 7*/},
            {400, 500, 1000, 1000, 1000, 700, 700, 700, /*0 - 7*/},
            {100, 10, 10, 0, 0, 0, 0, 0, /*0 - 7*/},
            {100, 200, 200, 200, 200, 200, 200, 200, /*0 - 7*/},
            {500, 500, 500, 500, 500, 500, 400, 400, /*0 - 7*/},
            {500, 500, 500, 500, 500, 500, 500, 600, /*0 - 7*/},
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
            {1800, 2300, 2500, 2850, 3800, 4400, 5000, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2950, 3800, 4450, 5050, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4550, 5100, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4500, 5100, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4450, 5100, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4500, 5100, 6500, 7500, 12000, /*0 - 9*/},
            {1800, 2300, 2500, 2850, 3800, 4500, 5100, 6600, 7600, 12000, /*0 - 9*/},
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
        0,
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
            4200,
            /* nSblGrValue */
            4200,
            /* nSblGbValue */
            4200,
            /* nSblBValue */
            4200,
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
            {1024, 2048, 4096, 8192, 16384, 32382, 64610, 128913, 257218, 513216, 1026432, 2052684, /*0 - 11*/},
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
                {4100, 4096, /*0 - 1*/},
                {4100, 4100, /*0 - 1*/},
                {4100, 4100, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
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
                {4100, 4096, /*0 - 1*/},
                {4100, 4100, /*0 - 1*/},
                {4100, 4100, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
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
                {4100, 4096, /*0 - 1*/},
                {4100, 4100, /*0 - 1*/},
                {4100, 4100, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
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
                {4100, 4096, /*0 - 1*/},
                {4100, 4100, /*0 - 1*/},
                {4100, 4100, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
            },
        },
        /* tLcgAutoTable */
        {
            /* nGainGrpNum */
            12,
            /* nExposeTimeGrpNum */
            2,
            /* nGain[16] */
            {1024, 2048, 4096, 8192, 16384, 32382, 64610, 128913, 257218, 513216, 1026432, 2052684, /*0 - 11*/},
            /* nExposeTime[10] */
            {1000, 40000, /*0 - 1*/},
            /* nAutoSblRValue[16][10] */
            {
                {4200, 4200, /*0 - 1*/},
                {4200, 4200, /*0 - 1*/},
                {4200, 4200, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
                {4262, 4262, /*0 - 1*/},
                {4262, 4262, /*0 - 1*/},
            },
            /* nAutoSblGrValue[16][10] */
            {
                {4200, 4200, /*0 - 1*/},
                {4200, 4200, /*0 - 1*/},
                {4200, 4200, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
                {4262, 4262, /*0 - 1*/},
                {4262, 4262, /*0 - 1*/},
            },
            /* nAutoSblGbValue[16][10] */
            {
                {4200, 4200, /*0 - 1*/},
                {4200, 4200, /*0 - 1*/},
                {4200, 4200, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
                {4262, 4262, /*0 - 1*/},
                {4262, 4262, /*0 - 1*/},
            },
            /* nAutoSblBValue[16][10] */
            {
                {4200, 4200, /*0 - 1*/},
                {4200, 4200, /*0 - 1*/},
                {4200, 4200, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4096, 4096, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4228, 4228, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
                {4250, 4250, /*0 - 1*/},
                {4262, 4262, /*0 - 1*/},
                {4262, 4262, /*0 - 1*/},
            },
        },
    },
};

#endif
