/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_opal_hal_hnb.h"
#include "ax_opal_log.h"
#include "ax_opal_type.h"
#include "ax_vin_api.h"
#include "ax_isp_iq_api.h"
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>


#define LOG_TAG "HNB"

#define HOT_NOISE_INVALID_HANDLE        (-1)
#define HOT_NOISE_THERMAL_NODE_NAME     "/sys/class/thermal/thermal_zone0/temp"

#define LOG_TAG_INTERVAL                (5000) // 5s

#define IS_SNS_LINEAR_MODE(eSnsMode) (((eSnsMode == AX_SNS_LINEAR_MODE) || (eSnsMode == AX_SNS_LINEAR_ONLY_MODE)) ? AX_TRUE : AX_FALSE)
#define IS_SNS_HDR_MODE(eSnsMode) (((eSnsMode >= AX_SNS_HDR_2X_MODE) && (eSnsMode <= AX_SNS_HDR_4X_MODE)) ? AX_TRUE : AX_FALSE)

// sensor hot noise balance status
typedef enum {
    HNB_STATUS_NORMAL_E,
    HNB_STATUS_BALANCE_E,
    HNB_STATUS_BUTT
} AX_OPAL_HAL_HNB_STATUS_E;

typedef struct axHOT_NOISE_BALANCE_STATUS_T {
    AX_BOOL bMonitorStart;
    AX_BOOL bUpdateTrigger;
    AX_OPAL_HAL_HNB_STATUS_E eStatus;
} AX_OPAL_HAL_HNB_STATUS_T;

typedef struct axHOT_NOISE_BALANCE_INFO_T {
    AX_S32 nSnsId;
    AX_S32 nPipeId;
    AX_S32 nChnId;
    AX_SENSOR_REGISTER_FUNC_T* pstSnsHdl;
    AX_OPAL_HAL_HNB_STATUS_T stStatus;
    AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T stAttr;
    AX_BOOL bThrRuning;
    pthread_t nThrId;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} AX_OPAL_HAL_HNB_INFO_T;

static AX_OPAL_HAL_HNB_INFO_T g_stHnbInfo[AX_OPAL_SNS_ID_BUTT] = {
[0] = {
        .nSnsId = 0,
        .nPipeId = 0,
        .nChnId = 0,
        .stStatus = {
            .bMonitorStart = AX_FALSE,
            .bUpdateTrigger = AX_FALSE,
            .eStatus = HNB_STATUS_NORMAL_E,
        },
        .stAttr = {
            .bEnable = AX_FALSE,
            .fNormalThreshold = 75,
            .fBalanceThreshold = 90,
            .strSdrHotNoiseNormalModeBin = NULL,
            .strSdrHotNoiseBalanceModeBin = NULL,
            .strHdrHotNoiseNormalModeBin = NULL,
            .strHdrHotNoiseBalanceModeBin = NULL,
        },
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
            .eStatus = HNB_STATUS_NORMAL_E,
        },
        .stAttr = {
            .bEnable = AX_FALSE,
            .fNormalThreshold = 75,
            .fBalanceThreshold = 90,
            .strSdrHotNoiseNormalModeBin = NULL,
            .strSdrHotNoiseBalanceModeBin = NULL,
            .strHdrHotNoiseNormalModeBin = NULL,
            .strHdrHotNoiseBalanceModeBin = NULL,
        },
        .nThrId = 0,
        .bThrRuning = AX_FALSE,
    }
};

AX_BOOL GetSnsTemperature(AX_S32 nSnsId, AX_F32* fTemperature) {
    AX_S32 nPipeId = g_stHnbInfo[nSnsId].nPipeId;
    if (g_stHnbInfo[nSnsId].pstSnsHdl && g_stHnbInfo[nSnsId].pstSnsHdl->pfn_sensor_get_temperature_info) {
        AX_S32 nTemperature = 0;
        AX_S32 nRet = g_stHnbInfo[nSnsId].pstSnsHdl->pfn_sensor_get_temperature_info(nPipeId, &nTemperature);
        if (0 == nRet) {
            *fTemperature = (AX_F32)nTemperature/1000.0;
            return AX_TRUE;
        }
    }
    return AX_FALSE;
}

AX_BOOL LoadBinParams(AX_S32 nSnsId, AX_CHAR *pBinFile) {
    if (pBinFile) {
        LOG_M_I(LOG_TAG, "Try loading %s",pBinFile);
        if (0 != access(pBinFile, F_OK)) {
            LOG_M_E(LOG_TAG, "%s is not exist.",pBinFile);
            return AX_FALSE;
        }
        AX_S32 nRet = AX_ISP_LoadBinParams(g_stHnbInfo[nSnsId].nPipeId, pBinFile);
        if (AX_SUCCESS == nRet) {
            return AX_FALSE;
        }
    }
    return AX_FALSE;
}

AX_BOOL Process(AX_S32 nSnsId, AX_F32 fThermal) {

    AX_CHAR * pBinFile = NULL;

    if (g_stHnbInfo[nSnsId].stAttr.bEnable) {
        if (fThermal >= g_stHnbInfo[nSnsId].stAttr.fBalanceThreshold) {
            // Escape
            if (g_stHnbInfo[nSnsId].stStatus.eStatus == HNB_STATUS_NORMAL_E) {
                LOG_M_C(LOG_TAG, "Thermal: %.2f", fThermal);
                AX_VIN_PIPE_ATTR_T tPipeAttr = {0};
                AX_VIN_GetPipeAttr(g_stHnbInfo[nSnsId].nPipeId, &tPipeAttr);

                if (IS_SNS_HDR_MODE(tPipeAttr.eSnsMode)) {
                    pBinFile = g_stHnbInfo[nSnsId].stAttr.strHdrHotNoiseBalanceModeBin;
                } else {
                    pBinFile = g_stHnbInfo[nSnsId].stAttr.strSdrHotNoiseBalanceModeBin;
                }

                if (LoadBinParams(nSnsId, pBinFile)) {
                    pthread_mutex_lock(&g_stHnbInfo[nSnsId].mutex);
                    g_stHnbInfo[nSnsId].stStatus.eStatus = HNB_STATUS_BALANCE_E;
                    pthread_mutex_unlock(&g_stHnbInfo[nSnsId].mutex);
                }
            }
        } else if (fThermal <= g_stHnbInfo[nSnsId].stAttr.fNormalThreshold) {
            // Recovery
            if (g_stHnbInfo[nSnsId].stStatus.eStatus == HNB_STATUS_BALANCE_E) {
                LOG_M_C(LOG_TAG, "Thermal: %.2f", fThermal);

                AX_VIN_PIPE_ATTR_T tPipeAttr = {0};
                AX_VIN_GetPipeAttr(g_stHnbInfo[nSnsId].nPipeId, &tPipeAttr);

                if (IS_SNS_HDR_MODE(tPipeAttr.eSnsMode)) {
                    pBinFile = g_stHnbInfo[nSnsId].stAttr.strHdrHotNoiseNormalModeBin;
                } else {
                    pBinFile = g_stHnbInfo[nSnsId].stAttr.strSdrHotNoiseNormalModeBin;
                }

                if (LoadBinParams(nSnsId, pBinFile)) {
                    pthread_mutex_lock(&g_stHnbInfo[nSnsId].mutex);
                    g_stHnbInfo[nSnsId].stStatus.eStatus = HNB_STATUS_NORMAL_E;
                    pthread_mutex_unlock(&g_stHnbInfo[nSnsId].mutex);
                }
            }
        }
    }
    return AX_TRUE;
}

static AX_VOID* Monitor(AX_VOID* pArg) {
    AX_S32 nSnsId = ((AX_OPAL_HAL_HNB_INFO_T*)pArg)->nSnsId;

    AX_S32 nThermalHandle = HOT_NOISE_INVALID_HANDLE;

    AX_CHAR threadName[32] = {0};
    sprintf(threadName, "APP_HnbMoni_%d", nSnsId);
    prctl(PR_SET_NAME, threadName);

    g_stHnbInfo[nSnsId].bThrRuning = AX_TRUE;

    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    pthread_cond_init(&g_stHnbInfo[nSnsId].cond, &cattr);

    while (g_stHnbInfo[nSnsId].bThrRuning) {
        pthread_mutex_lock(&g_stHnbInfo[nSnsId].mutex);

        struct timespec tv;
        clock_gettime(CLOCK_MONOTONIC, &tv);
        tv.tv_sec += LOG_TAG_INTERVAL / 1000;
        tv.tv_nsec += (LOG_TAG_INTERVAL  % 1000) * 1000000;
        if (tv.tv_nsec >= 1000000000) {
            tv.tv_nsec -= 1000000000;
            tv.tv_sec += 1;
        }
        pthread_cond_timedwait(&g_stHnbInfo[nSnsId].cond, &g_stHnbInfo[nSnsId].mutex, &tv);

        if (!g_stHnbInfo[nSnsId].bThrRuning) {
            pthread_mutex_unlock(&g_stHnbInfo[nSnsId].mutex);
            break;
        }

        if (g_stHnbInfo[nSnsId].stStatus.bUpdateTrigger) {
            g_stHnbInfo[nSnsId].stStatus.bUpdateTrigger = AX_FALSE;

            if (!g_stHnbInfo[nSnsId].stAttr.bEnable) {
                g_stHnbInfo[nSnsId].stStatus.bMonitorStart = AX_FALSE;
                pthread_mutex_unlock(&g_stHnbInfo[nSnsId].mutex);
                break;
            }
        }

        pthread_mutex_unlock(&g_stHnbInfo[nSnsId].mutex);

        if (!g_stHnbInfo[nSnsId].pstSnsHdl) {
            continue;
        }

        AX_F32 fTemperature = 0;

        // get sensor temperature
        if (!GetSnsTemperature(nSnsId, &fTemperature)) {
            // get temperature from chip
            if (nThermalHandle == HOT_NOISE_INVALID_HANDLE) {
                nThermalHandle = open(HOT_NOISE_THERMAL_NODE_NAME, O_RDONLY);
            }

            if (nThermalHandle != HOT_NOISE_INVALID_HANDLE) {
                AX_CHAR strThermal[50] = {0};

                lseek(nThermalHandle, 0, SEEK_SET);

                if (read(nThermalHandle, strThermal, 50) > 0) {
                    AX_S32 nThermal = atoi(strThermal);
                    fTemperature = (AX_F32)nThermal/1000.0;
                } else {
                    LOG_M_C(LOG_TAG, "read %s fail", HOT_NOISE_THERMAL_NODE_NAME);

                    close(nThermalHandle);

                    nThermalHandle = HOT_NOISE_INVALID_HANDLE;
                }
            } else {
                LOG_M_C(LOG_TAG, "open %s fail", HOT_NOISE_THERMAL_NODE_NAME);
            }

            LOG_M_N(LOG_TAG, "real time sensor temperature(C): %.2f", fTemperature);

        } else {
            LOG_M_N(LOG_TAG, "real time sensor temperature(S): %.2f", fTemperature);
        }

        // process
        Process(nSnsId, fTemperature);
    }
    return 0;
}

AX_S32 AX_OPAL_HAL_HNB_Init() {
    for (AX_S32 i = 0; i < AX_OPAL_SNS_ID_BUTT; i++) {
        pthread_mutex_init(&g_stHnbInfo[i].mutex, NULL);
        pthread_cond_init(&g_stHnbInfo[i].cond, NULL);
    }
    return 0;
}

AX_S32 AX_OPAL_HAL_HNB_Deinit() {
    for (AX_S32 i = 0; i < AX_OPAL_SNS_ID_BUTT; i++) {
        pthread_mutex_destroy(&g_stHnbInfo[i].mutex);
        pthread_cond_destroy(&g_stHnbInfo[i].cond);
    }
    return 0;
}

AX_S32 AX_OPAL_HAL_HNB_SetSnsInfo(AX_S32 nSnsId, AX_S32 nPipeId, AX_S32 nChnId, AX_SENSOR_REGISTER_FUNC_T* pstSnsHdl) {
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT) {
        return -1;
    }
    g_stHnbInfo[nSnsId].nPipeId = nPipeId;
    g_stHnbInfo[nSnsId].nChnId = nChnId;
    g_stHnbInfo[nSnsId].pstSnsHdl = pstSnsHdl;
    return 0;
}

AX_S32 AX_OPAL_HAL_HNB_Start(AX_S32 nSnsId) {
    if (!g_stHnbInfo[nSnsId].stStatus.bMonitorStart) {
        if (g_stHnbInfo[nSnsId].stAttr.bEnable) {
            AX_S32 nRet = pthread_create(&g_stHnbInfo[nSnsId].nThrId, NULL, Monitor, &g_stHnbInfo[nSnsId]);
            if (nRet != 0) {
                return -1;
            }
            g_stHnbInfo[nSnsId].stStatus.bMonitorStart = AX_TRUE;
        }
    }
    return 0;
}

AX_S32 AX_OPAL_HAL_HNB_Stop(AX_S32 nSnsId) {
    LOG_M_C(LOG_TAG, "[%d] +++", nSnsId);
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT) {
        return -1;
    }

    if (g_stHnbInfo[nSnsId].stStatus.bMonitorStart) {
        pthread_mutex_lock(&g_stHnbInfo[nSnsId].mutex);
        g_stHnbInfo[nSnsId].bThrRuning = AX_FALSE;
        pthread_cond_broadcast(&g_stHnbInfo[nSnsId].cond);
        pthread_mutex_unlock(&g_stHnbInfo[nSnsId].mutex);

        pthread_join(g_stHnbInfo[nSnsId].nThrId, NULL);
    }
    g_stHnbInfo[nSnsId].stStatus.bMonitorStart = AX_FALSE;
    g_stHnbInfo[nSnsId].stStatus.bUpdateTrigger = AX_FALSE;
    g_stHnbInfo[nSnsId].stStatus.eStatus = HNB_STATUS_NORMAL_E;

    LOG_M_C(LOG_TAG, "[%d] ---", nSnsId);
    return 0;
}

AX_S32 AX_OPAL_HAL_HNB_SetAttr(AX_S32 nSnsId, const AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T* pstAttr) {
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT || !pstAttr) {
        return -1;
    }

    if (g_stHnbInfo[nSnsId].stStatus.bMonitorStart) {
        if (pstAttr->bEnable) {
            pthread_mutex_lock(&g_stHnbInfo[nSnsId].mutex);
            g_stHnbInfo[nSnsId].stAttr = *pstAttr;
            g_stHnbInfo[nSnsId].stStatus.bUpdateTrigger = AX_TRUE;
            pthread_cond_broadcast(&g_stHnbInfo[nSnsId].cond);
            pthread_mutex_unlock(&g_stHnbInfo[nSnsId].mutex);
        } else {
            AX_OPAL_HAL_HNB_Stop(nSnsId);
            pthread_mutex_lock(&g_stHnbInfo[nSnsId].mutex);
            g_stHnbInfo[nSnsId].stAttr = *pstAttr;
            pthread_cond_broadcast(&g_stHnbInfo[nSnsId].cond);
        }
    } else {
        pthread_mutex_lock(&g_stHnbInfo[nSnsId].mutex);
        g_stHnbInfo[nSnsId].stAttr = *pstAttr;
        pthread_mutex_unlock(&g_stHnbInfo[nSnsId].mutex);
        if (g_stHnbInfo[nSnsId].stAttr.bEnable) {
            AX_OPAL_HAL_HNB_Start(nSnsId);
        }
    }
    return AX_TRUE;
}

AX_S32 AX_OPAL_HAL_HNB_GetAttr(AX_S32 nSnsId, AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T* pstAttr) {
    if (nSnsId < 0 || nSnsId >= AX_OPAL_SNS_ID_BUTT || !pstAttr) {
        return -1;
    }
    *pstAttr = g_stHnbInfo[nSnsId].stAttr;
    return 0;
}
