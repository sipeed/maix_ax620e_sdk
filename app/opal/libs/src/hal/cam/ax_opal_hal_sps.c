/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_opal_hal_sps.h"
#include "ax_opal_log.h"
#include "ax_vin_api.h"
#include "ax_isp_iq_api.h"
#include "ax_opal_utils.h"
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#define LOG_TAG "SPS"

#define LOG_TAG_INTERVAL                       (5000) // 5s
#define SPS_STATUS_HOLD_TIME                   (10000) // 10s
#define SPS_WARM_DEFAULT_THRESHOLD             (50)

#define SPS_DFEAULT_ONLIGHT_THRESHOLD          (12819426304)
#define SPS_DFEAULT_OFFLIGHT_THRESHOLD         (456960000)

typedef struct axSPS_STATUS_T {
    AX_BOOL bMonitorStart;
    AX_BOOL bUpdateTrigger;
    AX_U64 onTicks;
    AX_U64 offTicks;
    AX_U64 onLightThreshold;
    AX_U64 offLightThreshold;
    AX_U32 curDayNightStatus;
} AX_OPAL_HAL_SPS_STATUS_T;

typedef struct axSPS_Info {
    AX_S32 nSnsId;
    AX_S32 nPipeId;
    AX_S32 nChnId;
    AX_OPAL_HAL_SPS_STATUS_T stStatus;
    AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T stSpsAttr;
    AX_OPAL_VIDEO_SNS_SOFTPHOTOSENSITIVITY_CALLBACK callback;
    AX_VOID*  pUserData;
    AX_BOOL bThrRuning;
    pthread_t  nThrId;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
}AX_OPAL_HAL_SPS_INFO_T;

static AX_OPAL_HAL_SPS_INFO_T g_stSpsInfo[AX_OPAL_SNS_ID_BUTT] = {
    [0] = {
        .nSnsId = 0,
        .nPipeId = 0,
        .nChnId = 0,
        .stStatus = {
            .bMonitorStart = AX_FALSE,
            .bUpdateTrigger = AX_FALSE,
            .onTicks = 0,
            .offTicks = 0,
            .onLightThreshold = SPS_DFEAULT_ONLIGHT_THRESHOLD,
            .offLightThreshold = SPS_DFEAULT_OFFLIGHT_THRESHOLD,
            .curDayNightStatus = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_STATUS_UNKOWN,
        },
        .stSpsAttr = {
            .bAutoCtrl = AX_TRUE,
            .eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_NONE,
            .stWarmAttr = {
                .nOnLightSensitivity = 0,
                .nOnLightExpValMax = 0,
                .nOnLightExpValMid = 0,
                .nOnLightExpValMin = 0,
                .nOffLightSensitivity = 0,
                .nOffLightExpValMax = 0,
                .nOffLightExpValMid = 0,
                .nOffLightExpValMin = 0,
            },
        },
        .callback = NULL,
        .pUserData = NULL,
        .nThrId = 0,
        .bThrRuning = AX_FALSE,
    },
    [1] = {
        .nSnsId = 1,
        .nPipeId = 1,
        .nChnId = 0,
        .stStatus = {
            .bMonitorStart = AX_FALSE,
            .bUpdateTrigger = AX_FALSE,
            .onTicks = 0,
            .offTicks = 0,
            .onLightThreshold = SPS_DFEAULT_ONLIGHT_THRESHOLD,
            .offLightThreshold = SPS_DFEAULT_OFFLIGHT_THRESHOLD,
            .curDayNightStatus = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_STATUS_UNKOWN,
        },
        .stSpsAttr = {
            .bAutoCtrl = AX_TRUE,
            .eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_NONE,
            .stWarmAttr = {
                .nOnLightSensitivity = 0,
                .nOnLightExpValMax = 0,
                .nOnLightExpValMid = 0,
                .nOnLightExpValMin = 0,
                .nOffLightSensitivity = 0,
                .nOffLightExpValMax = 0,
                .nOffLightExpValMid = 0,
                .nOffLightExpValMin = 0,
            },
        },
        .callback = NULL,
        .pUserData = NULL,
        .nThrId = 0,
        .bThrRuning = AX_FALSE,
    }
};

static AX_U32 AeFloat2Int(AX_F32 value, AX_U32 int_bit, AX_U32 frac_bit, AX_BOOL signed_value) {
    AX_U32 result = 0;
    if ((int_bit + frac_bit + signed_value) > 32) {
        return -1;
    }

    result = AX_ABS(value) * (AX_U64)(1 << frac_bit);

    if (signed_value) {
        if (value < 0) {
            result = -result;
        }
    }

    return result;
}

static AX_U64 GetTickCount(AX_VOID) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    AX_U64 result = ts.tv_sec;
    result *= 1000;
    result += (ts.tv_nsec / 1000000);
    return result;
}

static AX_VOID SetWarmLightAttr(AX_S32 nSnsId, const AX_OPAL_SNS_WARMLIGHT_ATTR_T* pstWarmAttr) {
    if (pstWarmAttr->nOnLightSensitivity == SPS_WARM_DEFAULT_THRESHOLD) {
        g_stSpsInfo[nSnsId].stStatus.onLightThreshold = pstWarmAttr->nOnLightExpValMid;
    } else if (pstWarmAttr->nOnLightSensitivity > SPS_WARM_DEFAULT_THRESHOLD) {
        g_stSpsInfo[nSnsId].stStatus.onLightThreshold = pstWarmAttr->nOnLightExpValMid - (((pstWarmAttr->nOnLightSensitivity - SPS_WARM_DEFAULT_THRESHOLD) * 2) * (pstWarmAttr->nOnLightExpValMid - pstWarmAttr->nOnLightExpValMin) / 100);
    } else if (pstWarmAttr->nOnLightSensitivity < SPS_WARM_DEFAULT_THRESHOLD) {
        g_stSpsInfo[nSnsId].stStatus.onLightThreshold = pstWarmAttr->nOnLightExpValMid + (((SPS_WARM_DEFAULT_THRESHOLD - pstWarmAttr->nOnLightSensitivity) * 2) * (pstWarmAttr->nOnLightExpValMax - pstWarmAttr->nOnLightExpValMid) / 100);
    } else {
        g_stSpsInfo[nSnsId].stStatus.onLightThreshold = pstWarmAttr->nOnLightExpValMid;
    }

    if (pstWarmAttr->nOffLightSensitivity == SPS_WARM_DEFAULT_THRESHOLD) {
        g_stSpsInfo[nSnsId].stStatus.offLightThreshold = pstWarmAttr->nOffLightExpValMid;
    } else if (pstWarmAttr->nOffLightSensitivity > SPS_WARM_DEFAULT_THRESHOLD) {
        g_stSpsInfo[nSnsId].stStatus.offLightThreshold = pstWarmAttr->nOffLightExpValMid + (((pstWarmAttr->nOffLightSensitivity - SPS_WARM_DEFAULT_THRESHOLD) * 2) * (pstWarmAttr->nOffLightExpValMax - pstWarmAttr->nOffLightExpValMid) / 100);
    } else if (pstWarmAttr->nOffLightSensitivity < SPS_WARM_DEFAULT_THRESHOLD) {
        g_stSpsInfo[nSnsId].stStatus.offLightThreshold = pstWarmAttr->nOffLightExpValMid - (((SPS_WARM_DEFAULT_THRESHOLD - pstWarmAttr->nOffLightSensitivity) * 2) * (pstWarmAttr->nOffLightExpValMid - pstWarmAttr->nOffLightExpValMin) / 100);
    } else {
        g_stSpsInfo[nSnsId].stStatus.offLightThreshold = pstWarmAttr->nOffLightExpValMid;
    }
    g_stSpsInfo[nSnsId].stStatus.onTicks = 0;
    g_stSpsInfo[nSnsId].stStatus.offTicks = 0;
    g_stSpsInfo[nSnsId].stStatus.curDayNightStatus = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_STATUS_UNKOWN;
}

static AX_VOID SetIrAttr(AX_S32 nSnsId, const AX_OPAL_SNS_IRCUT_ATTR_T* pstIrAttr) {
    AX_S32 nRet = 0;
    AX_S32 nPipeId = g_stSpsInfo[nSnsId].nPipeId;
    AX_VIN_CHN_ID_E eChnId = g_stSpsInfo[nSnsId].nChnId;
    AX_DAYNIGHT_MODE_E eDayNightStatus = AX_DAYNIGHT_MODE_DAY;

    nRet = AX_VIN_GetChnDayNightMode(nPipeId, eChnId, &eDayNightStatus);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnDayNightMode failed, pipe=%d chn=%d ret=0x%x",  nPipeId, eChnId, nRet);
        return;
    }

    g_stSpsInfo[nSnsId].stStatus.curDayNightStatus = eDayNightStatus;

    AX_ISP_IQ_IR_PARAM_T tIrParam;
    memset(&tIrParam, 0, sizeof(tIrParam));
    tIrParam.nIrCalibR                = AeFloat2Int(pstIrAttr->fIrCalibR, 8, 10, AX_FALSE);
    tIrParam.nIrCalibG                = AeFloat2Int(pstIrAttr->fIrCalibG, 8, 10, AX_FALSE);
    tIrParam.nIrCalibB                = AeFloat2Int(pstIrAttr->fIrCalibB, 8, 10, AX_FALSE);
    tIrParam.nNight2DayIrStrengthTh   = AeFloat2Int(pstIrAttr->fNight2DayIrStrengthTh, 8, 10, AX_FALSE);
    tIrParam.nNight2DayIrDetectTh     = AeFloat2Int(pstIrAttr->fNight2DayIrDetectTh, 8, 10, AX_FALSE);
    tIrParam.nDay2NightLuxTh          = AeFloat2Int(pstIrAttr->fDay2NightLuxTh, 22, 10, AX_FALSE);
    tIrParam.nNight2DayLuxTh          = AeFloat2Int(pstIrAttr->fNight2DayLuxTh, 22, 10, AX_FALSE);
    tIrParam.nNight2DayBrightTh       = AeFloat2Int(pstIrAttr->fNight2DayBrightTh, 8, 10, AX_FALSE);
    tIrParam.nNight2DayDarkTh         = AeFloat2Int(pstIrAttr->fNight2DayDarkTh, 8, 10, AX_FALSE);
    tIrParam.nNight2DayUsefullWpRatio = AeFloat2Int(pstIrAttr->fNight2DayUsefullWpRatio, 8, 10, AX_FALSE);
    tIrParam.nCacheTime               = pstIrAttr->nCacheTime;
    tIrParam.nInitDayNightMode        = eDayNightStatus;

    nRet = AX_ISP_IQ_SetIrParam(nPipeId, &tIrParam);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_SetIrParam failed, ret=0x%x", nRet);
    }
}

static AX_VOID WarmLightMonitor(AX_S32 nSnsId, const AX_OPAL_SNS_WARMLIGHT_ATTR_T* pstWarmAttr) {
    AX_S32 nRet = 0;
    AX_S32 nPipeId = g_stSpsInfo[nSnsId].nPipeId;
    AX_VIN_CHN_ID_E eChnId = g_stSpsInfo[nSnsId].nChnId;
    AX_ISP_IQ_AE_STATUS_T tAeStatus;
    nRet = AX_ISP_IQ_GetAeStatus(nPipeId, &tAeStatus);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ISP_IQ_GetAeStatus failed, ret=0x%x.", nRet);
        return;
    }

    AX_U64 ticks = GetTickCount();
    AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_RESULT_T tResult = {0};

    // night mode
    if (tAeStatus.tAlgStatus.nExpVal >= g_stSpsInfo[nSnsId].stStatus.onLightThreshold) {
        g_stSpsInfo[nSnsId].stStatus.offTicks = 0;

        if (g_stSpsInfo[nSnsId].stStatus.onTicks == 0) {
            g_stSpsInfo[nSnsId].stStatus.onTicks = ticks;
        }

        if (ticks - g_stSpsInfo[nSnsId].stStatus.onTicks > SPS_STATUS_HOLD_TIME) {
            LOG_M_N(LOG_TAG, "The current expval meets the night conditions.");

            if (AX_DAYNIGHT_MODE_NIGHT != g_stSpsInfo[nSnsId].stStatus.curDayNightStatus) {
                if (g_stSpsInfo[nSnsId].stSpsAttr.bAutoCtrl) {
                    AX_VIN_SetChnDayNightMode(nPipeId, eChnId, AX_DAYNIGHT_MODE_NIGHT);
                }

                tResult.eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_WARMLIGHT;
                tResult.eStatus = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_STATUS_NIGHT;
                tResult.pUserData = g_stSpsInfo[nSnsId].pUserData;
                if (g_stSpsInfo[nSnsId].callback) {
                    g_stSpsInfo[nSnsId].callback(nSnsId, &tResult);
                }
            }

            g_stSpsInfo[nSnsId].stStatus.onTicks = 0;
        }
    } else if (tAeStatus.tAlgStatus.nExpVal < g_stSpsInfo[nSnsId].stStatus.offLightThreshold) {
        g_stSpsInfo[nSnsId].stStatus.onTicks = 0;

        if (g_stSpsInfo[nSnsId].stStatus.offTicks == 0) {
            g_stSpsInfo[nSnsId].stStatus.offTicks = ticks;
        }

        if (ticks - g_stSpsInfo[nSnsId].stStatus.offTicks > SPS_STATUS_HOLD_TIME) {
            LOG_M_N(LOG_TAG, "The current expval meets the day conditions.");

            if (AX_DAYNIGHT_MODE_DAY != g_stSpsInfo[nSnsId].stStatus.curDayNightStatus) {
                if (g_stSpsInfo[nSnsId].stSpsAttr.bAutoCtrl) {
                    AX_VIN_SetChnDayNightMode(nPipeId, eChnId, AX_DAYNIGHT_MODE_DAY);
                }

                tResult.eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_WARMLIGHT;
                tResult.eStatus = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_STATUS_DAY;
                tResult.pUserData = g_stSpsInfo[nSnsId].pUserData;
                if (g_stSpsInfo[nSnsId].callback) {
                    g_stSpsInfo[nSnsId].callback(nSnsId, &tResult);
                }
            }

            g_stSpsInfo[nSnsId].stStatus.offTicks = 0;
        }
    }

    AX_DAYNIGHT_MODE_E eDayNightStatus = AX_DAYNIGHT_MODE_DAY;
    AX_VIN_GetChnDayNightMode(nPipeId, eChnId, &eDayNightStatus);
    g_stSpsInfo[nSnsId].stStatus.curDayNightStatus = eDayNightStatus;
}

static AX_VOID IrMonitor(AX_S32 nSnsId, const AX_OPAL_SNS_IRCUT_ATTR_T* pstIrAttr) {
    AX_S32 nRet = 0;
    AX_S32 nPipeId = g_stSpsInfo[nSnsId].nPipeId;
    AX_VIN_CHN_ID_E eChnId = g_stSpsInfo[nSnsId].nChnId;
    AX_DAYNIGHT_MODE_E eDayNightStatus = AX_DAYNIGHT_MODE_DAY;
    nRet = AX_VIN_GetChnDayNightMode(nPipeId, eChnId, &eDayNightStatus);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_VIN_GetChnDayNightMode failed, ret=0x%x", nRet);
        return;
    }
    AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_RESULT_T tResult = {0};

    if (eDayNightStatus != g_stSpsInfo[nSnsId].stStatus.curDayNightStatus) {
        switch(eDayNightStatus) {
            case AX_DAYNIGHT_MODE_DAY:
                /* night to day */
                if (g_stSpsInfo[nSnsId].stSpsAttr.bAutoCtrl) {
                    AX_VIN_SetChnDayNightMode(nPipeId, eChnId, AX_DAYNIGHT_MODE_DAY);
                }

                tResult.eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_IR;
                tResult.eStatus = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_STATUS_DAY;
                tResult.pUserData = g_stSpsInfo[nSnsId].pUserData;
                if (g_stSpsInfo[nSnsId].callback) {
                    g_stSpsInfo[nSnsId].callback(nSnsId, &tResult);
                }

                break;

            case AX_DAYNIGHT_MODE_NIGHT:
                /* day to night */
                if (g_stSpsInfo[nSnsId].stSpsAttr.bAutoCtrl) {
                    AX_VIN_SetChnDayNightMode(nPipeId, eChnId, AX_DAYNIGHT_MODE_NIGHT);
                }

                tResult.eType = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_IR;
                tResult.eStatus = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_STATUS_NIGHT;
                tResult.pUserData = g_stSpsInfo[nSnsId].pUserData;
                if (g_stSpsInfo[nSnsId].callback) {
                    g_stSpsInfo[nSnsId].callback(nSnsId, &tResult);
                }

                break;

            default:
                LOG_M_N(LOG_TAG, "Unkonw dayNightMode [%d]", eDayNightStatus);
                break;
        }

        AX_VIN_GetChnDayNightMode(nPipeId, eChnId, &eDayNightStatus);
        g_stSpsInfo[nSnsId].stStatus.curDayNightStatus = eDayNightStatus;
    }
}

static AX_VOID* Monitor(AX_VOID* pArg) {
    AX_S32 nSnsId = ((AX_OPAL_HAL_SPS_INFO_T*)pArg)->nSnsId;

    if (AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_WARMLIGHT == g_stSpsInfo[nSnsId].stSpsAttr.eType) {
        SetWarmLightAttr(nSnsId, &g_stSpsInfo[nSnsId].stSpsAttr.stWarmAttr);
    } else if (AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_IR == g_stSpsInfo[nSnsId].stSpsAttr.eType) {
        SetIrAttr(nSnsId, &g_stSpsInfo[nSnsId].stSpsAttr.stIrAttr);
    } else {
        LOG_M_E(LOG_TAG, "soft photosensitivity[%d] invalid", g_stSpsInfo[nSnsId].stSpsAttr.eType);
        return NULL;
    }

    AX_CHAR threadName[32] = {0};
    sprintf(threadName, "APP_SpsMoni_%d", nSnsId);
    prctl(PR_SET_NAME, threadName);

    g_stSpsInfo[nSnsId].bThrRuning = AX_TRUE;

    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    pthread_cond_init(&g_stSpsInfo[nSnsId].cond, &cattr);

    while (g_stSpsInfo[nSnsId].bThrRuning) {
        pthread_mutex_lock(&g_stSpsInfo[nSnsId].mutex);

        struct timespec tv;
        clock_gettime(CLOCK_MONOTONIC, &tv);
        tv.tv_sec += LOG_TAG_INTERVAL / 1000;
        tv.tv_nsec += (LOG_TAG_INTERVAL  % 1000) * 1000000;
        if (tv.tv_nsec >= 1000000000) {
            tv.tv_nsec -= 1000000000;
            tv.tv_sec += 1;
        }
        pthread_cond_timedwait(&g_stSpsInfo[nSnsId].cond, &g_stSpsInfo[nSnsId].mutex, &tv);

        if (!g_stSpsInfo[nSnsId].bThrRuning) {
            pthread_mutex_unlock(&g_stSpsInfo[nSnsId].mutex);
            break;
        }

        if (g_stSpsInfo[nSnsId].stStatus.bUpdateTrigger) {
            g_stSpsInfo[nSnsId].stStatus.bUpdateTrigger = AX_FALSE;

            if (AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_IR == g_stSpsInfo[nSnsId].stSpsAttr.eType) {
                SetIrAttr(nSnsId, &g_stSpsInfo[nSnsId].stSpsAttr.stIrAttr);
            } else if (AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_WARMLIGHT == g_stSpsInfo[nSnsId].stSpsAttr.eType){
                SetWarmLightAttr(nSnsId, &g_stSpsInfo[nSnsId].stSpsAttr.stWarmAttr);
            } else {
                g_stSpsInfo[nSnsId].bThrRuning = AX_FALSE;
                g_stSpsInfo[nSnsId].stStatus.bMonitorStart = AX_FALSE;
                pthread_mutex_unlock(&g_stSpsInfo[nSnsId].mutex);
                break;
            }
        }
        pthread_mutex_unlock(&g_stSpsInfo[nSnsId].mutex);

        if (AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_WARMLIGHT == g_stSpsInfo[nSnsId].stSpsAttr.eType) {
            WarmLightMonitor(nSnsId, &g_stSpsInfo[nSnsId].stSpsAttr.stWarmAttr);
        } else if (AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_IR == g_stSpsInfo[nSnsId].stSpsAttr.eType) {
            IrMonitor(nSnsId, &g_stSpsInfo[nSnsId].stSpsAttr.stIrAttr);
        } else {
            LOG_M_E(LOG_TAG, "soft photosensitivity[%d] invalid", g_stSpsInfo[nSnsId].stSpsAttr.eType);
            break;
        }
    }

    return NULL;
}

AX_S32 AX_OPAL_HAL_SPS_Init() {
    for (AX_S32 i = 0; i < AX_OPAL_SNS_ID_BUTT; i++) {
        pthread_mutex_init(&g_stSpsInfo[i].mutex, NULL);
        pthread_cond_init(&g_stSpsInfo[i].cond, NULL);
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_SPS_Deinit() {
    for (AX_S32 i = 0; i < AX_OPAL_SNS_ID_BUTT; i++) {
        pthread_mutex_destroy(&g_stSpsInfo[i].mutex);
        pthread_cond_destroy(&g_stSpsInfo[i].cond);
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_SPS_SetSnsInfo(AX_S32 nSnsId, AX_S32 nPipeId, AX_S32 nChnId) {
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT) {
        return -1;
    }
    g_stSpsInfo[nSnsId].nPipeId = nPipeId;
    g_stSpsInfo[nSnsId].nChnId = nChnId;
    return 0;
}

AX_S32 AX_OPAL_HAL_SPS_Start(AX_S32 nSnsId) {
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT) {
        return -1;
    }

    LOG_M_C(LOG_TAG, "[%d] +++", nSnsId);

    if (!g_stSpsInfo[nSnsId].stStatus.bMonitorStart) {
        if (AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_IR == g_stSpsInfo[nSnsId].stSpsAttr.eType
            || AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_WARMLIGHT == g_stSpsInfo[nSnsId].stSpsAttr.eType) {
            AX_S32 nRet = pthread_create(&g_stSpsInfo[nSnsId].nThrId, NULL, Monitor, &g_stSpsInfo[nSnsId]);
            if (nRet != 0) {
                return AX_FALSE;
            }
            g_stSpsInfo[nSnsId].stStatus.bMonitorStart = AX_TRUE;
        }
    }
    LOG_M_C(LOG_TAG, "[%d] ---", nSnsId);

    return 0;
}

AX_S32 AX_OPAL_HAL_SPS_Stop(AX_S32 nSnsId) {
    LOG_M_C(LOG_TAG, "[%d] +++", nSnsId);

    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT) {
        return -1;
    }

    if (g_stSpsInfo[nSnsId].stStatus.bMonitorStart) {
        pthread_mutex_lock(&g_stSpsInfo[nSnsId].mutex);
        g_stSpsInfo[nSnsId].bThrRuning = AX_FALSE;
        pthread_cond_broadcast(&g_stSpsInfo[nSnsId].cond);
        pthread_mutex_unlock(&g_stSpsInfo[nSnsId].mutex);

        pthread_join(g_stSpsInfo[nSnsId].nThrId, NULL);
    }

    g_stSpsInfo[nSnsId].stStatus.bMonitorStart = AX_FALSE;
    g_stSpsInfo[nSnsId].stStatus.bUpdateTrigger = AX_FALSE;
    g_stSpsInfo[nSnsId].stStatus.onTicks = 0;
    g_stSpsInfo[nSnsId].stStatus.offTicks = 0;
    g_stSpsInfo[nSnsId].stStatus.curDayNightStatus = AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_STATUS_UNKOWN;

    LOG_M_C(LOG_TAG, "[%d] ---", nSnsId);
    return 0;
}

AX_S32 AX_OPAL_HAL_SPS_SetAttr(AX_S32 nSnsId, const AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T* pstAttr){
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT || !pstAttr) {
        return -1;
    }

    if (g_stSpsInfo[nSnsId].stStatus.bMonitorStart) {
        if (pstAttr->eType != AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_NONE) {
            pthread_mutex_lock(&g_stSpsInfo[nSnsId].mutex);
            g_stSpsInfo[nSnsId].stSpsAttr = *pstAttr;
            g_stSpsInfo[nSnsId].stStatus.bUpdateTrigger = AX_TRUE;
            pthread_cond_broadcast(&g_stSpsInfo[nSnsId].cond);
            pthread_mutex_unlock(&g_stSpsInfo[nSnsId].mutex);
        } else {
            AX_OPAL_HAL_SPS_Stop(nSnsId);
            pthread_mutex_lock(&g_stSpsInfo[nSnsId].mutex);
            g_stSpsInfo[nSnsId].stSpsAttr = *pstAttr;
            pthread_mutex_unlock(&g_stSpsInfo[nSnsId].mutex);
        }

    } else {
        pthread_mutex_lock(&g_stSpsInfo[nSnsId].mutex);
        g_stSpsInfo[nSnsId].stSpsAttr = *pstAttr;
        pthread_mutex_unlock(&g_stSpsInfo[nSnsId].mutex);
        if (g_stSpsInfo[nSnsId].stSpsAttr.eType != AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_NONE) {
            AX_OPAL_HAL_SPS_Start(nSnsId);
        }
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_SPS_GetAttr(AX_S32 nSnsId, AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T* pstAttr) {
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT || !pstAttr) {
        return -1;
    }

    *pstAttr = g_stSpsInfo[nSnsId].stSpsAttr;

    return 0;
}

AX_S32 AX_OPAL_HAL_SPS_RegisterCallback(AX_S32 nSnsId,
                                        const AX_OPAL_VIDEO_SNS_SOFTPHOTOSENSITIVITY_CALLBACK callback,
                                        AX_VOID *pUserData) {
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT || !callback) {
        return -1;
    }
    g_stSpsInfo[nSnsId].callback = callback;
    g_stSpsInfo[nSnsId].pUserData = pUserData;
    return 0;
}

AX_S32 AX_OPAL_HAL_SPS_UnRegisterCallback(AX_S32 nSnsId) {
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT) {
        return -1;
    }
    g_stSpsInfo[nSnsId].callback = NULL;
    g_stSpsInfo[nSnsId].pUserData = NULL;
    return 0;
}
