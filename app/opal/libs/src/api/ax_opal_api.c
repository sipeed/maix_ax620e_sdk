/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_api.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ax_opal_mal_pipeline.h"
#include "ax_opal_mal_ppl_parser.h"
#include "ax_opal_api_def.h"
#include "ax_opal_log.h"

AX_OPAL_LOG_LEVEL_E g_opallib_log_level = AX_OPAL_LOG_ERROR;

static AX_OPAL_MAL_PPL_HANDLE g_pipeline = AX_NULL;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
#define API_LOCK do { \
    if (pthread_mutex_lock(&g_mutex) != 0) { \
        fprintf(stderr, "Failed to lock mutex\n"); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define API_UNLOCK do { \
    if (pthread_mutex_unlock(&g_mutex) != 0) { \
        fprintf(stderr, "Failed to unlock mutex\n"); \
        exit(EXIT_FAILURE); \
    } \
} while (0)


#define NULL_PTR_CHECK(ptr) do { \
    if ((ptr) == AX_NULL) { \
        fprintf(stderr, "invalid input param, null ptr\n"); \
        return AX_ERR_OPAL_NULL_PTR; \
    } \
} while (0)

#define TAG "OPAL_API"

#define OPAL_API  __attribute__((visibility("default")))

OPAL_API
const AX_CHAR *AX_OPAL_GetVersion(AX_VOID) {
    return OPAL_BUILD_VERSION;
}

OPAL_API
AX_OPAL_CHIP_TYPE_E AX_OPAL_GetChipType(AX_VOID) {
    AX_CHIP_TYPE_E eChipType = AX_SYS_GetChipType();
    switch (eChipType) {
        case NONE_CHIP_TYPE:
            return AX_OPAL_CHIP_TYPE_NONE;
        case AX620Q_CHIP:
            return AX_OPAL_CHIP_TYPE_AX620Q;
        case AX620QX_CHIP:
            return AX_OPAL_CHIP_TYPE_AX620QX;
        case AX630C_CHIP:
            return AX_OPAL_CHIP_TYPE_AX630C;
        case AX620QZ_CHIP:
            return AX_OPAL_CHIP_TYPE_AX620QZ;
        case AX620QP_CHIP:
            return AX_OPAL_CHIP_TYPE_AX620QP;
        default:
            return AX_OPAL_CHIP_TYPE_BUTT;
    }
}

OPAL_API
AX_S32 AX_OPAL_Init(const AX_OPAL_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);

    // log level
    {
        AX_CHAR* pEnv = NULL;

        pEnv = getenv("OPAL_LOG_level");
        if (pEnv) {
            g_opallib_log_level = atoi(pEnv);
        }
    }

    // AX_OPAL_ATTR_T* pstAttr = (AX_OPAL_ATTR_T*)pstAttrArg;
    // pstAttr->szPipelineConfigPath = "./config/pipeline.ini";
    if (strlen(pstAttr->szPipelineConfigPath) != 0) {
        AX_OPAL_PPL_ATTR_T stPplAttr;
        memset(&stPplAttr, 0x0, sizeof(AX_OPAL_PPL_ATTR_T));
        if (0 == AX_OPAL_MAL_PPL_Parse(pstAttr->szPipelineConfigPath, &stPplAttr)) {
            g_pipeline = AX_OPAL_MAL_PPL_Create(&stPplAttr, pstAttr);
        }
    } else {
        g_pipeline = AX_OPAL_MAL_PPL_Create(&g_stPplAttr, pstAttr);
    }
    if (g_pipeline == AX_NULL) {
        return AX_ERR_OPAL_INVALID_HANDLE;
    }

    /* init */
    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_INIT;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        return AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Deinit(AX_VOID) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    /* deinit */
    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_DEINIT;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    nRet = AX_OPAL_MAL_PPL_Destroy(g_pipeline);
    g_pipeline = NULL;

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Start(AX_VOID) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_START;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Stop(AX_VOID) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_STOP;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_GetSnsAttr(AX_S32 nSnsId, AX_OPAL_VIDEO_SNS_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);

    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_GETSNSATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_SNS_ATTR_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_SetSnsAttr(AX_S32 nSnsId, const AX_OPAL_VIDEO_SNS_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_SETSNSATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_VIDEO_SNS_ATTR_T*)pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_SNS_ATTR_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_GetSnsSoftPhotoSensitivityAttr(AX_S32 nSnsId, AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_GETSNSSOFTPHOTOSENSITIVITYATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_SetSnsSoftPhotoSensitivityAttr(AX_S32 nSnsId,
                                                           const AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_SETSNSSOFTPHOTOSENSITIVITYATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T*)pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_SNS_SOFT_PHOTOSENSITIVITY_ATTR_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_RegisterSnsSoftPhotoSensitivityCallback(AX_S32 nSnsId,
                                                                    const AX_OPAL_VIDEO_SNS_SOFTPHOTOSENSITIVITY_CALLBACK callback,
                                                                    AX_VOID *pUserData) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_REGISTERSNSSOFTPHOTOSENSITIVITYCALLBACK;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_SOFTPHOTOSENSITIVITY_CALLBACK_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_SOFTPHOTOSENSITIVITY_CALLBACK_T));
    stData.callback = callback;
    stData.pUserData = pUserData;
    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_SOFTPHOTOSENSITIVITY_CALLBACK_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_UnRegisterSnsSoftPhotoSensitivityCallback(AX_S32 nSnsId) {
    AX_S32 nRet = AX_OPAL_SUCC;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_UNREGISTERSNSSOFTPHOTOSENSITIVITYCALLBACK;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }
    return nRet;
}


OPAL_API
AX_S32 AX_OPAL_Video_GetSnsHotNoiseBalanceAttr(AX_S32 nSnsId, AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_GETSNSHOTNOISEBALANCEATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_SetSnsHotNoiseBalanceAttr(AX_S32 nSnsId,
                                        const AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_SETSNSHOTNOISEBALANCEATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T*)pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_SNS_HOTNOISEBALANCE_ATTR_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_GetChnAttr(AX_S32 nSnsId, AX_S32 nChnId, AX_OPAL_VIDEO_CHN_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_GETCHNATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_CHN_ATTR_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_SetChnAttr(AX_S32 nSnsId, AX_S32 nChnId, const AX_OPAL_VIDEO_CHN_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_SETCHNATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_VIDEO_CHN_ATTR_T*)pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_CHN_ATTR_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_RegisterPacketCallback(AX_S32 nSnsId, AX_S32 nChnId,
                                                     const AX_OPAL_VIDEO_PKT_CALLBACK callback, AX_VOID *pUserData) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_REGISTERPACKETCALLBACK;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_PKT_CALLBACK_T stVideoRegPktCb;
    memset(&stVideoRegPktCb, 0x0, sizeof(AX_OPAL_VIDEO_PKT_CALLBACK_T));
    stVideoRegPktCb.callback = callback;
    stVideoRegPktCb.pUserData = pUserData;
    stProcData.pData = &stVideoRegPktCb;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_PKT_CALLBACK_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_UnRegisterPacketCallback(AX_S32 nSnsId, AX_S32 nChnId) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_UNREGISTERPACKETCALLBACK;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_RequestIDR(AX_S32 nSnsId, AX_S32 nChnId) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_REQUESTIDR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_Snapshot(AX_S32 nSnsId, AX_S32 nChnId, AX_VOID *pImageBuf, AX_U32 nImageBufSize,
                                       AX_U32 *pActSize, AX_U32 nQpLevel) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pImageBuf);
    NULL_PTR_CHECK(pActSize);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_SNAPSHOT;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_SNAPSHOT_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_SNAPSHOT_T));
    stData.pImageBuf = pImageBuf;
    stData.nImageBufSize = nImageBufSize;
    stData.pActSize = pActSize;
    stData.nQpLevel = nQpLevel;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_SNAPSHOT_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_CaptureFrame(AX_S32 nSnsId, AX_S32 nChnId, AX_VOID *pFrameBuf,
                AX_U32 nFrameBufSize, AX_U32 *pActSize, AX_U32 nWidth, AX_U32 nHeight) {

    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pFrameBuf);
    NULL_PTR_CHECK(pActSize);

    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_CAPTUREFRAME;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_CAPTUREFRAME_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_CAPTUREFRAME_T));
    stData.pFrameBuf = pFrameBuf;
    stData.nFrameBufSize = nFrameBufSize;
    stData.pActSize = pActSize;
    stData.nWidth = nWidth;
    stData.nHeight = nHeight;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_CAPTUREFRAME_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_GetSvcParam(AX_S32 nSnsId, AX_S32 nChnId, AX_OPAL_VIDEO_SVC_PARAM_T* pstParam) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstParam);

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_GETSVCPARAM;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstParam;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_SVC_PARAM_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_SetSvcParam(AX_S32 nSnsId, AX_S32 nChnId, const AX_OPAL_VIDEO_SVC_PARAM_T* pstParam) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstParam);

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_SETSVCPARAM;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_VIDEO_SVC_PARAM_T*)pstParam;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_SVC_PARAM_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_SetSvcRegion(AX_S32 nSnsId, AX_S32 nChnId, const AX_OPAL_VIDEO_SVC_REGION_T* pstRegion) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstRegion);

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_SETSVCREGION;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_VIDEO_SVC_REGION_T*)pstRegion;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_SVC_REGION_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_AlgoGetParam(AX_S32 nSnsId, AX_OPAL_ALGO_PARAM_T* pstParam) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstParam);

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_ALGOGETPARAM;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstParam;
    stProcData.nDataSize = sizeof(AX_OPAL_ALGO_PARAM_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_AlgoSetParam(AX_S32 nSnsId, const AX_OPAL_ALGO_PARAM_T* pstParam) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstParam);

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_ALGOSETPARAM;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_ALGO_PARAM_T*)pstParam;
    stProcData.nDataSize = sizeof(AX_OPAL_ALGO_PARAM_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_RegisterAlgoCallback(AX_S32 nSnsId, const AX_OPAL_VIDEO_ALGO_CALLBACK callback,
                                                   AX_VOID *pUserData) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_REGISTERALGOCALLBACK;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_ALGO_CALLBACK_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_ALGO_CALLBACK_T));
    stData.callback = callback;
    stData.pUserData = pUserData;
    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_ALGO_CALLBACK_T);
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_UnRegisterAlgoCallback(AX_S32 nSnsId) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_UNREGISTERALGOCALLBACK;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_OsdCreate(AX_S32 nSnsId, AX_S32 nChnId, const AX_OPAL_OSD_ATTR_T* pstAttr,
                                        AX_OPAL_HANDLE *pOsdHandle) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    NULL_PTR_CHECK(pOsdHandle);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_OSDCREATE;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_OSD_CREATE_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_OSD_CREATE_T));
    stData.pstAttr = (AX_OPAL_OSD_ATTR_T*)pstAttr;
    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_OSD_CREATE_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    *pOsdHandle = stData.OsdHandle;

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_OsdUpdate(AX_S32 nSnsId, AX_S32 nChnId, AX_OPAL_HANDLE OsdHandle, const AX_OPAL_OSD_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_OSDUPDATE;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_OSD_UPDATE_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_OSD_UPDATE_T));
    stData.OsdHandle = OsdHandle;
    stData.pstAttr = (AX_OPAL_OSD_ATTR_T*)pstAttr;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_OSD_UPDATE_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_OsdDestroy(AX_S32 nSnsId, AX_S32 nChnId, AX_OPAL_HANDLE OsdHandle) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_OSDDESTROY;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_OSD_DESTROY_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_OSD_DESTROY_T));
    stData.OsdHandle = OsdHandle;
    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_OSD_DESTROY_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_OsdDrawRect(AX_S32 nSnsId, AX_S32 nChnId, AX_U32 nRectSize,
                                          const AX_OPAL_RECT_T* pstRects, AX_U32 nLineWidth, AX_U32 nARGB) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstRects);

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_OSDDRAWRECT;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_OSD_DRAWRECT_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_OSD_DRAWRECT_T));
    stData.nRectSize = nRectSize;
    stData.pstRects = (AX_OPAL_RECT_T*)pstRects;
    stData.nLineWidth = nLineWidth;
    stData.nARGB = nARGB;
    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_OSD_DRAWRECT_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_OsdClearRect(AX_S32 nSnsId, AX_S32 nChnId) {
    AX_S32 nRet = AX_OPAL_SUCC;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_OSDCLEARRECT;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_OsdDrawPolygon(AX_S32 nSnsId, AX_S32 nChnId, AX_U32 nPolygonSize,
                                             const AX_OPAL_POLYGON_T* pstPolygons, AX_U32 nLineWidth, AX_U32 nARGB) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstPolygons);

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_OSDDRAWPOLYGON;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_VIDEO_OSD_DRAWPOLYGON_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_VIDEO_OSD_DRAWPOLYGON_T));
    stData.nPolygonSize = nPolygonSize;
    stData.pstPolygons = (AX_OPAL_POLYGON_T*)pstPolygons;
    stData.nLineWidth = nLineWidth;
    stData.nARGB = nARGB;
    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_OSD_DRAWPOLYGON_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Video_OsdClearPolygon(AX_S32 nSnsId, AX_S32 nChnId) {
    AX_S32 nRet = AX_OPAL_SUCC;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = nSnsId;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_OSDCLEARPOLYGON;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_GetAttr(AX_OPAL_AUDIO_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_GETATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_ATTR_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_SetAttr(const AX_OPAL_AUDIO_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_SETATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_AUDIO_ATTR_T*)pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_ATTR_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_Play(AX_S32 nChnId, AX_PAYLOAD_TYPE_E eType, const AX_U8 *pData, AX_U32 nDataSize) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pData);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_PLAY;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_AUDIO_PLAY_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_AUDIO_PLAY_T));
    stData.eType = eType;
    stData.pData = (AX_U8 *)pData;
    stData.nDataSize = nDataSize;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_PLAY_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_PlayFile(AX_S32 nChnId, AX_PAYLOAD_TYPE_E eType, const AX_CHAR *pstrFileName, AX_S32 nLoop,
                                       AX_OPAL_AUDIO_PLAYFILERESULT_CALLBACK callback, AX_VOID *pUserData) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstrFileName);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_PLAYFILE;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_AUDIO_PLAYFILE_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_AUDIO_PLAYFILE_T));
    stData.eType = eType;
    stData.pstrFileName = (AX_CHAR *)pstrFileName;
    stData.nLoop = nLoop;
    stData.callback = callback;
    stData.pUserData = pUserData;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_PLAYFILE_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_StopPlay(AX_S32 nChnId) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_STOPPLAY;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_GetCapVolume(AX_F32 *pfVol) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pfVol);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_GETCAPVOLUME;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_AUDIO_GETVOLUME_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_AUDIO_GETVOLUME_T));
    stData.pfVol = pfVol;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_GETVOLUME_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_SetCapVolume(AX_F32 fVol) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_SETCAPVOLUME;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_AUDIO_SETVOLUME_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_AUDIO_SETVOLUME_T));
    stData.fVol = fVol;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_SETVOLUME_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_GetPlayVolume(AX_F32 *pfVol) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pfVol);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_GETPLAYVOLUME;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_AUDIO_GETVOLUME_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_AUDIO_GETVOLUME_T));
    stData.pfVol = pfVol;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_GETVOLUME_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_SetPlayVolume(AX_F32 fVol) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_SETPLAYVOLUME;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_AUDIO_SETVOLUME_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_AUDIO_SETVOLUME_T));
    stData.fVol = fVol;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_SETVOLUME_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_GetEncoderAttr(AX_S32 nChnId, AX_OPAL_AUDIO_ENCODER_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_GETENCODERATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_ENCODER_ATTR_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_GetPlayPipeAttr(AX_S32 nChnId, AX_OPAL_AUDIO_PLAY_CHN_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_GETPLAYPIPEATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_PLAY_CHN_ATTR_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_SetPlayPipeAttr(AX_S32 nChnId, const AX_OPAL_AUDIO_PLAY_CHN_ATTR_T* pstAttr) {
    AX_S32 nRet = AX_OPAL_SUCC;
    NULL_PTR_CHECK(pstAttr);
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = -1;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_SETPLAYPIPEATTR;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
    stProcData.pData = (AX_OPAL_AUDIO_PLAY_CHN_ATTR_T*)pstAttr;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_PLAY_CHN_ATTR_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_RegisterPacketCallback(AX_S32 nChnId, const AX_OPAL_AUDIO_PKT_CALLBACK callback,
                                                     AX_VOID *pUserData) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_REGISTERPACKETCALLBACK;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    AX_OPAL_AUDIO_PKT_CALLBACK_T stData;
    memset(&stData, 0x0, sizeof(AX_OPAL_AUDIO_PKT_CALLBACK_T));
    stData.callback = callback;
    stData.pUserData = pUserData;

    stProcData.pData = &stData;
    stProcData.nDataSize = sizeof(AX_OPAL_AUDIO_PKT_CALLBACK_T);

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}

OPAL_API
AX_S32 AX_OPAL_Audio_UnRegisterPacketCallback(AX_S32 nChnId) {
    AX_S32 nRet = AX_OPAL_SUCC;
    API_LOCK;

    AX_OPAL_MAL_PROCESS_DATA_T stProcData;
    memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
    stProcData.nUniGrpId = -1;
    stProcData.nUniChnId = nChnId;
    stProcData.eMainCmdType = AX_OPAL_MAINCMD_AUDIO_UNREGISTERPACKETCALLBACK;
    stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;

    nRet = AX_OPAL_MAL_PPL_Process(g_pipeline, &stProcData);
    if (nRet != AX_SUCCESS) {
        nRet = AX_ERR_OPAL_GENERIC;
    }

    API_UNLOCK;
    return nRet;
}