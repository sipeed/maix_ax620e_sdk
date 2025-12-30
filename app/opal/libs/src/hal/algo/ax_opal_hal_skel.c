/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_opal_hal_skel.h"
#include "cJSON.h"
#include "ax_opal_log.h"
#include <stdio.h>
#include <string.h>

#define LOG_TAG  ("HAL_SKEL")

#ifndef AX_MAX
#define AX_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef AX_MIN
#define AX_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static const AX_CHAR *g_pstrObjectCategory[AX_OPAL_ALGO_HVCFP_TYPE_BUTT] = {"body", "vehicle", "cycle", "face", "plate"};

#if 0
static AX_OPAL_ALGO_HVCFP_TYPE_E GetHvcfpType(const AX_CHAR * pstrObjectCategory) {
    if (!pstrObjectCategory) {
        return AX_OPAL_ALGO_HVCFP_TYPE_BUTT;
    }
    for (AX_S32 i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; i++) {
        if (strcmp(g_pstrObjectCategory[i], pstrObjectCategory) == 0) {
            return (AX_OPAL_ALGO_HVCFP_TYPE_E)i;
        }
    }
    return AX_OPAL_ALGO_HVCFP_TYPE_BUTT;
}
#endif

static const AX_CHAR*  GetHvcfpStr(const AX_OPAL_ALGO_HVCFP_TYPE_E eType) {
    if (eType < AX_OPAL_ALGO_HVCFP_TYPE_BUTT) {
        return g_pstrObjectCategory[eType];
    }
    return "";
}

static AX_OPAL_ALGO_PLATE_COLOR_TYPE_E GetPlateColorType(const AX_CHAR* strPlateColor) {
    if (!strPlateColor) {
        return AX_OPAL_ALGO_PLATE_COLOR_UNKOWN;
    }

    if (strcmp(strPlateColor, "blue") == 0) {
        return AX_OPAL_ALGO_PLATE_COLOR_BLUE;
    } else if (strcmp(strPlateColor,"yellow") == 0) {
        return AX_OPAL_ALGO_PLATE_COLOR_YELLOW;
    } else if (strcmp(strPlateColor, "black") == 0) {
        return AX_OPAL_ALGO_PLATE_COLOR_BLACK;
    } else if (strcmp(strPlateColor, "white") == 0) {
        return AX_OPAL_ALGO_PLATE_COLOR_WHITE;
    } else if (strcmp(strPlateColor, "green") == 0) {
        return AX_OPAL_ALGO_PLATE_COLOR_GREEN;
    } else if (strcmp(strPlateColor, "small_new_energy") == 0) {
        return AX_OPAL_ALGO_PLATE_COLOR_NEW_ENERGY;
    } else if (strcmp(strPlateColor, "large_new_energy") == 0) {
        return AX_OPAL_ALGO_PLATE_COLOR_NEW_ENERGY;
    }

    return AX_OPAL_ALGO_PLATE_COLOR_UNKOWN;
}

static AX_VOID FaceAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItem, AX_OPAL_ALGO_HVCFP_ITEM_T* pstItem) {
    pstItem->stFaceAttr.bExist = pstObjectItem->bFaceAttr;

    if (pstObjectItem->bFaceAttr) {
        pstItem->stFaceAttr.nAge = pstObjectItem->stFaceAttr.nAge;
        pstItem->stFaceAttr.nGender = pstObjectItem->stFaceAttr.nGender;
        if (pstObjectItem->stFaceAttr.nMask == 0) {
            pstItem->stFaceAttr.eRespirator = AX_OPAL_ALGO_FACE_RESPIRATOR_NONE;
        } else {
            pstItem->stFaceAttr.eRespirator = AX_OPAL_ALGO_FACE_RESPIRATOR_COMMON;
        }

        pstItem->stFaceAttr.stFacePoseBlur.fPitch = pstObjectItem->stFaceAttr.stPoseBlur.fPitch;
        pstItem->stFaceAttr.stFacePoseBlur.fYaw = pstObjectItem->stFaceAttr.stPoseBlur.fYaw;
        pstItem->stFaceAttr.stFacePoseBlur.fRoll = pstObjectItem->stFaceAttr.stPoseBlur.fRoll;
        pstItem->stFaceAttr.stFacePoseBlur.fBlur = pstObjectItem->stFaceAttr.stPoseBlur.fBlur;
    }
}

static AX_VOID PlateAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItem, AX_OPAL_ALGO_HVCFP_ITEM_T* pstItem) {
    opalJSON *jObj = NULL;
    opalJSON *jItem = NULL;
    opalJSON *jValue = NULL;

    for (size_t i = 0; i < pstObjectItem->nMetaInfoSize; i++) {
        if (strcmp(pstObjectItem->pstMetaInfo[i].pstrType, "plate_attr") == 0) {
            jObj = opalJSON_Parse(pstObjectItem->pstMetaInfo[i].pstrValue);
            if (!jObj) {
                break;
            }
            pstItem->stPlateAttr.bExist = AX_TRUE;
            pstItem->stPlateAttr.ePlateColor = AX_OPAL_ALGO_PLATE_COLOR_UNKOWN;

            // color
            jItem = opalJSON_GetObjectItem(jObj, "color");
            if (jItem) {
                jValue = opalJSON_GetObjectItem(jItem, "name");
                pstItem->stPlateAttr.ePlateColor = GetPlateColorType(jValue->valuestring);
            }

            // code
            jValue = opalJSON_GetObjectItem(jObj, "code_result");
            strncpy(pstItem->stPlateAttr.szPlateCode, jValue->valuestring, sizeof(pstItem->stPlateAttr.szPlateCode) - 1);

            jValue = opalJSON_GetObjectItem(jObj, "code_killed");
            if (jValue->valuedouble) {
                pstItem->stPlateAttr.bValid = AX_FALSE;
            } else {
                pstItem->stPlateAttr.bValid = AX_TRUE;
            }
        }
    }
}

static AX_VOID VehicleAttrResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItem, AX_OPAL_ALGO_HVCFP_ITEM_T* pstItem) {
    opalJSON *jObj = NULL;
    opalJSON *jItem = NULL;
    opalJSON *jValue = NULL;

    for (size_t i = 0; i < pstObjectItem->nMetaInfoSize; i++) {
        if (strcmp(pstObjectItem->pstMetaInfo[i].pstrType, "plate_attr") == 0) {
            jObj = opalJSON_Parse(pstObjectItem->pstMetaInfo[i].pstrValue);
            if (!jObj) {
                break;
            }

            pstItem->stVehicleAttr.bExist = AX_TRUE;
            pstItem->stVehicleAttr.stPlateAttr.bExist = AX_TRUE;
            pstItem->stVehicleAttr.stPlateAttr.ePlateColor = AX_OPAL_ALGO_PLATE_COLOR_UNKOWN;

            // color
            jItem = opalJSON_GetObjectItem(jObj, "color");
            if (jItem) {
                jValue = opalJSON_GetObjectItem(jItem, "name");
                pstItem->stVehicleAttr.stPlateAttr.ePlateColor = GetPlateColorType(jValue->valuestring);
            }

            // code
            jValue = opalJSON_GetObjectItem(jObj, "code_result");
            strncpy(pstItem->stVehicleAttr.stPlateAttr.szPlateCode, jValue->valuestring, sizeof(pstItem->stPlateAttr.szPlateCode) - 1);

            jValue = opalJSON_GetObjectItem(jObj, "code_killed");
            if (jValue->valuedouble) {
                pstItem->stVehicleAttr.stPlateAttr.bValid = AX_FALSE;
            } else {
                pstItem->stVehicleAttr.stPlateAttr.bValid = AX_TRUE;
            }
        }
    }
}

AX_VOID AX_OPAL_HAL_GetSkelResult(const AX_SKEL_OBJECT_ITEM_T *pstObjectItems, AX_OPAL_ALGO_HVCFP_ITEM_T* pstItem) {
    if (!pstObjectItems || !pstItem) {
        return;
    }

    if (AX_OPAL_ALGO_HVCFP_FACE == pstItem->eType) {
        FaceAttrResult(pstObjectItems, pstItem);
    }
    else if (AX_OPAL_ALGO_HVCFP_PLATE == pstItem->eType) {
        PlateAttrResult(pstObjectItems, pstItem);
    }
    else if (AX_OPAL_ALGO_HVCFP_VEHICLE == pstItem->eType) {
        VehicleAttrResult(pstObjectItems, pstItem);
    }
}

static AX_BOOL SetPushStrategy(AX_U32 nSnsId, AX_SKEL_HANDLE pSkel,
                               AX_OPAL_ALGO_PUSH_STRATEGY_T* pstPushStrategyCur,
                               const AX_OPAL_ALGO_PUSH_STRATEGY_T* pstPushStrategyNew) {
    if (!pSkel) {
        return AX_TRUE;
    }

    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    AX_SKEL_PUSH_STRATEGY_T stPushStrategyConfig;

    // set push_strategy
    stPushStrategyConfig.ePushMode = (AX_SKEL_PUSH_MODE_E)pstPushStrategyNew->ePushMode;
    stPushStrategyConfig.nIntervalTimes = pstPushStrategyNew->nInterval;
    stPushStrategyConfig.nPushCounts = pstPushStrategyNew->nPushCount;
    stPushStrategyConfig.bPushSameFrame = AX_TRUE;

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"push_strategy";
    stConfigItem.pstrValue = (AX_VOID *)&stPushStrategyConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_PUSH_STRATEGY_T);

    stConfig.pstItems = &stConfigItem;
    stConfig.nSize = 1;

    AX_S32 nRet = AX_SKEL_SetConfig(pSkel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SKEL_SetConfig failed, ret=0x%08X", nRet);
        return AX_FALSE;
    }

    if (pstPushStrategyCur) {
        *pstPushStrategyCur = *pstPushStrategyNew;
    }

    LOG_M_I(LOG_TAG, "push strategy(mode:%d, interval:%d, count:%d)", pstPushStrategyNew->ePushMode, pstPushStrategyNew->nInterval,
            pstPushStrategyNew->nPushCount);

    return AX_TRUE;
}

static AX_BOOL SetRoi(AX_U32 nSnsId, AX_SKEL_HANDLE pSkel,
                      AX_OPAL_ALGO_ROI_CONFIG_T* pstRoiCur,
                      const AX_OPAL_ALGO_ROI_CONFIG_T* pstRoiNew) {
    if (!pSkel) {
        return AX_TRUE;
    }

    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_ROI_POLYGON_CONFIG_T stRoiConfig;

    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    stRoiConfig.bEnable = pstRoiNew->bEnable;
    stRoiConfig.nPointNum = AX_MIN(pstRoiNew->stPolygon.nPointNum, AX_SKEL_ROI_POINT_MAX);

    AX_SKEL_POINT_T stPoints[AX_SKEL_ROI_POINT_MAX];
    memset(stPoints, 0, sizeof(stPoints));

    for (AX_U32 i = 0; i < stRoiConfig.nPointNum; i++) {
        stPoints[i].fX = pstRoiNew->stPolygon.stPoints[i].fX;
        stPoints[i].fY = pstRoiNew->stPolygon.stPoints[i].fY;
    }

    stRoiConfig.pstPoint = (AX_SKEL_POINT_T *)stPoints;

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"detect_roi_polygon";
    stConfigItem.pstrValue = (AX_VOID *)&stRoiConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_ROI_POLYGON_CONFIG_T);

    stConfig.pstItems = &stConfigItem;
    stConfig.nSize = 1;

    AX_S32 nRet = AX_SKEL_SetConfig(pSkel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SKEL_SetConfig failed, ret=0x%08X", nRet);
        return AX_FALSE;
    }

    if (pstRoiCur) {
        *pstRoiCur = *pstRoiNew;
    }

    LOG_M_I(LOG_TAG, "roi(enable:%d)", pstRoiNew->bEnable);

    return AX_TRUE;
}


static AX_BOOL SetObjectFilter(AX_U32 nSnsId, AX_SKEL_HANDLE pSkel, AX_OPAL_ALGO_HVCFP_TYPE_E eType,
                               AX_OPAL_ALGO_HVCFP_FILTER_CONFIG_T* pstObjectFliterCur,
                               const AX_OPAL_ALGO_HVCFP_FILTER_CONFIG_T* pstObjectFliterNew) {
    if (!pSkel) {
        return AX_TRUE;
    }

    if (eType >= AX_OPAL_ALGO_HVCFP_TYPE_BUTT) {
        return AX_FALSE;
    }

    const AX_CHAR* strObject = GetHvcfpStr(eType);

    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem[2];
    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T) * 2);
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stConfidenceConfig;
    AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T stObjectSizeFilterConfig;

    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    memset(&stObjectSizeFilterConfig, 0, sizeof(stObjectSizeFilterConfig));
    stObjectSizeFilterConfig.nWidth = pstObjectFliterNew->nWidth;
    stObjectSizeFilterConfig.nHeight = pstObjectFliterNew->nHeight;

    AX_CHAR szObject0[64] = {0};
    sprintf(szObject0, "%s_min_size", strObject);

    stConfigItem[0].pstrType = szObject0;
    stConfigItem[0].pstrValue = (AX_VOID *)&stObjectSizeFilterConfig;
    stConfigItem[0].nValueSize = sizeof(AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T);

    AX_CHAR szObject1[64] = {0};
    sprintf(szObject1, "%s_confidence", strObject);
    stConfidenceConfig.fValue = (AX_F32)pstObjectFliterNew->fConfidence;
    stConfigItem[1].pstrType = szObject1;
    stConfigItem[1].pstrValue = (AX_VOID *)&stConfidenceConfig;
    stConfigItem[1].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);

    stConfig.pstItems = &stConfigItem[0];
    stConfig.nSize = 2;

    AX_S32 nRet = AX_SKEL_SetConfig(pSkel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SKEL_SetConfig failed, ret=0x%08X", nRet);
        return AX_FALSE;
    }

    if (pstObjectFliterCur) {
        *pstObjectFliterCur = *pstObjectFliterNew;
    }

    LOG_M_I(LOG_TAG, "%s filter(%d X %d, confidence: %.2f)", strObject, pstObjectFliterNew->nWidth, pstObjectFliterNew->nHeight,
            pstObjectFliterNew->fConfidence);

    return AX_TRUE;
}

static AX_BOOL SetTrackSize(AX_U32 nSnsId, AX_SKEL_HANDLE pSkel,
                            AX_OPAL_ALGO_TRACK_SIZE_T* pstTrackSizeCur,
                            const AX_OPAL_ALGO_TRACK_SIZE_T* pstTrackSizeNew) {
    if (!pSkel) {
       return AX_TRUE;
    }

    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem[3];
    memset(&stConfigItem[0], 0, sizeof(AX_SKEL_CONFIG_ITEM_T)*3);

    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    // set max track human size
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stMaxTrackHumanSize;
    stMaxTrackHumanSize.fValue = (AX_F32)pstTrackSizeNew->nTrackHumanSize;
    stConfigItem[0].pstrType = (AX_CHAR *)"body_max_target_count";
    stConfigItem[0].pstrValue = (AX_VOID *)&stMaxTrackHumanSize;
    stConfigItem[0].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);

    // set max track vehicle size
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stMaxTrackVehicleSize;
    stMaxTrackVehicleSize.fValue = (AX_F32)pstTrackSizeNew->nTrackVehicleSize;
    stConfigItem[1].pstrType = (AX_CHAR *)"vehicle_max_target_count";
    stConfigItem[1].pstrValue = (AX_VOID *)&stMaxTrackVehicleSize;
    stConfigItem[1].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);

    // set max track cycle size
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stMaxTrackCycleSize;
    stMaxTrackCycleSize.fValue = (AX_F32)pstTrackSizeNew->nTrackCycleSize;
    stConfigItem[2].pstrType = (AX_CHAR *)"cycle_max_target_count";
    stConfigItem[2].pstrValue = (AX_VOID *)&stMaxTrackCycleSize;
    stConfigItem[2].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);

    stConfig.pstItems = &stConfigItem[0];
    stConfig.nSize = 3;

    AX_S32 nRet = AX_SKEL_SetConfig(pSkel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SKEL_SetConfig failed, ret=0x%08X", nRet);
        return AX_FALSE;
    }

    if (pstTrackSizeCur) {
        *pstTrackSizeCur = *pstTrackSizeNew;
    }

    LOG_M_I(LOG_TAG, "Track size(human: %d, vehicle: %d, cycle: %d)", pstTrackSizeNew->nTrackHumanSize, pstTrackSizeNew->nTrackVehicleSize,
            pstTrackSizeNew->nTrackCycleSize);

    return AX_TRUE;
}

static AX_BOOL SetPanorama(AX_U32 nSnsId, AX_SKEL_HANDLE pSkel, AX_U32 nCropEncoderQpLevel,
                           AX_OPAL_ALGO_PANORAMA_T* pstPanoramaCur,
                           const AX_OPAL_ALGO_PANORAMA_T* pstPanoramaNew) {
    if (!pSkel) {
        return AX_TRUE;
    }

    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_PUSH_PANORAMA_CONFIG_T stPanoramaConfig;

    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    stPanoramaConfig.bEnable = pstPanoramaNew->bEnable;
    stPanoramaConfig.nQuality = nCropEncoderQpLevel;

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"push_panorama";
    stConfigItem.pstrValue = (AX_VOID *)&stPanoramaConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_PUSH_PANORAMA_CONFIG_T);

    stConfig.pstItems = &stConfigItem;
    stConfig.nSize = 1;

    AX_S32 nRet = AX_SKEL_SetConfig(pSkel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SKEL_SetConfig failed, ret=0x%08X", nRet);
        return AX_FALSE;
    }

    if (pstPanoramaCur) {
        *pstPanoramaCur = *pstPanoramaNew;
    }

    LOG_M_I(LOG_TAG, "Panorama(enable:%d)", pstPanoramaNew->bEnable);

    return AX_TRUE;
}

static AX_BOOL SetCropEncoderQpLevel(AX_U32 nSnsId, AX_SKEL_HANDLE pSkel,
                                     AX_U8 *pCropEncoderQpLevelCur,
                                     const AX_U8 *pCropEncoderQpLevelNew) {
    if (!pSkel) {
        return AX_TRUE;
    }

    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;

    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));

    // set crop encoder qpLevel
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stCropEncoderQpLevelThreshold;
    stCropEncoderQpLevelThreshold.fValue = (AX_F32)(*pCropEncoderQpLevelNew);
    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = (AX_CHAR *)"crop_encoder_qpLevel";
    stConfigItem.pstrValue = (AX_VOID *)&stCropEncoderQpLevelThreshold;
    stConfigItem.nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);

    stConfig.pstItems = &stConfigItem;
    stConfig.nSize = 1;

    AX_S32 nRet = AX_SKEL_SetConfig(pSkel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SKEL_SetConfig failed, ret=0x%08X", nRet);
        return AX_FALSE;
    }

    if (pCropEncoderQpLevelCur) {
        *pCropEncoderQpLevelCur = *pCropEncoderQpLevelNew;
    }

    LOG_M_I(LOG_TAG, "fCropEncoderQpLevel (%d)", *pCropEncoderQpLevelNew);

    return AX_TRUE;
}

#if 0
static AX_BOOL SetCropThreshold(AX_U32 nSnsId, AX_SKEL_HANDLE pSkel, AX_OPAL_ALGO_HVCFP_TYPE_E eType,
                                AX_OPAL_ALGO_CROP_THRESHOLD_CONFIG_T* pstCropThresholdCur,
                                const AX_OPAL_ALGO_CROP_THRESHOLD_CONFIG_T* pstCropThresholdNew) {
    if (!pSkel) {
        return AX_TRUE;
    }

    if (eType >= AX_OPAL_ALGO_HVCFP_TYPE_BUTT) {
        return AX_FALSE;
    }

    const AX_CHAR *strObject = GetHvcfpStr(eType);

    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T stCropThresholdConfig;
    memset(&stCropThresholdConfig, 0, sizeof(stCropThresholdConfig));

    stCropThresholdConfig.fScaleLeft = pstCropThresholdNew->fScaleLeft;
    stCropThresholdConfig.fScaleRight = pstCropThresholdNew->fScaleRight;
    stCropThresholdConfig.fScaleTop = pstCropThresholdNew->fScaleTop;
    stCropThresholdConfig.fScaleBottom = pstCropThresholdNew->fScaleBottom;

    AX_CHAR szObject[64] = {0};
    sprintf(szObject, "%s_crop_encoder", strObject);

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = szObject;
    stConfigItem.pstrValue = (AX_VOID *)&stCropThresholdConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T);

    stConfig.pstItems = &stConfigItem;
    stConfig.nSize = 1;

    AX_S32 nRet = AX_SKEL_SetConfig(pSkel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SKEL_SetConfig failed, ret=0x%08X", nRet);
        return AX_FALSE;
    }

    if(pstCropThresholdCur) {
        *pstCropThresholdCur = *pstCropThresholdNew;
    }
    LOG_M_I(LOG_TAG, "%s CropThreshold(%.2f, %.2f, %.2f, %.2f)", strObject, pstCropThresholdNew->fScaleLeft,
            pstCropThresholdNew->fScaleRight, pstCropThresholdNew->fScaleTop, pstCropThresholdNew->fScaleBottom);

    return AX_TRUE;
}
#endif

static AX_BOOL SetPushFilter(AX_U32 nSnsId, AX_SKEL_HANDLE pSkel, AX_OPAL_ALGO_HVCFP_TYPE_E eType,
                             AX_OPAL_ALGO_PUSH_FILTER_CONFIG_T* pstPushFilterCur,
                             const AX_OPAL_ALGO_PUSH_FILTER_CONFIG_T* pstPushFilterNew) {
    if (!pSkel) {
        return AX_TRUE;
    }

    if (eType >= AX_OPAL_ALGO_HVCFP_TYPE_BUTT) {
        return AX_FALSE;
    }

    const AX_CHAR *strObject = GetHvcfpStr(eType);

    AX_SKEL_CONFIG_T stConfig;
    AX_SKEL_CONFIG_ITEM_T stConfigItem;
    AX_SKEL_ATTR_FILTER_CONFIG_T stPushFilterConfig;

    memset(&stConfig, 0, sizeof(AX_SKEL_CONFIG_T));
    memset(&stPushFilterConfig, 0, sizeof(stPushFilterConfig));

    if (AX_OPAL_ALGO_HVCFP_FACE == eType) {
        stPushFilterConfig.stFaceAttrFilterConfig.nWidth = pstPushFilterNew->stFacePushFilterConfig.nWidth;
        stPushFilterConfig.stFaceAttrFilterConfig.nHeight = pstPushFilterNew->stFacePushFilterConfig.nHeight;
        stPushFilterConfig.stFaceAttrFilterConfig.stPoseblur.fPitch = pstPushFilterNew->stFacePushFilterConfig.stFacePoseBlur.fPitch;
        stPushFilterConfig.stFaceAttrFilterConfig.stPoseblur.fYaw = pstPushFilterNew->stFacePushFilterConfig.stFacePoseBlur.fYaw;
        stPushFilterConfig.stFaceAttrFilterConfig.stPoseblur.fRoll = pstPushFilterNew->stFacePushFilterConfig.stFacePoseBlur.fRoll;
        stPushFilterConfig.stFaceAttrFilterConfig.stPoseblur.fBlur = pstPushFilterNew->stFacePushFilterConfig.stFacePoseBlur.fBlur;

    } else {
        stPushFilterConfig.stCommonAttrFilterConfig.fQuality = pstPushFilterNew->stCommonPushFilterConfig.fQuality;
    }

    AX_CHAR szObject[64] = {0};
    sprintf(szObject, "push_quality_%s", strObject);

    memset(&stConfigItem, 0, sizeof(AX_SKEL_CONFIG_ITEM_T));
    stConfigItem.pstrType = szObject;
    stConfigItem.pstrValue = (AX_VOID *)&stPushFilterConfig;
    stConfigItem.nValueSize = sizeof(AX_SKEL_ATTR_FILTER_CONFIG_T);

    stConfig.pstItems = &stConfigItem;
    stConfig.nSize = 1;

    AX_S32 nRet = AX_SKEL_SetConfig(pSkel, &stConfig);

    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SKEL_SetConfig failed, ret=0x%08X", nRet);
        return AX_FALSE;
    }

    if (AX_OPAL_ALGO_HVCFP_FACE == eType) {
        if (pstPushFilterCur) {
            pstPushFilterCur->stFacePushFilterConfig = pstPushFilterNew->stFacePushFilterConfig;
        }
        LOG_M_I(LOG_TAG, "%s push filter(%d X %d, p: %.2f, y: %.2f, r: %.2f, b: %.2f)", strObject,
                pstPushFilterNew->stFacePushFilterConfig.nWidth, pstPushFilterNew->stFacePushFilterConfig.nHeight,
                pstPushFilterNew->stFacePushFilterConfig.stFacePoseBlur.fPitch, pstPushFilterNew->stFacePushFilterConfig.stFacePoseBlur.fYaw,
                pstPushFilterNew->stFacePushFilterConfig.stFacePoseBlur.fRoll, pstPushFilterNew->stFacePushFilterConfig.stFacePoseBlur.fBlur);
    } else {
        if (pstPushFilterCur) {
            pstPushFilterCur->stCommonPushFilterConfig = pstPushFilterNew->stCommonPushFilterConfig;
        }
        LOG_M_I(LOG_TAG, "%s push filter(Quality: %.2f)", strObject, pstPushFilterNew->stCommonPushFilterConfig.fQuality);
    }

    return AX_TRUE;
}


AX_S32 AX_OPAL_HAL_SetSkelParam(AX_U8 nSnsId, AX_SKEL_HANDLE pSkel, AX_OPAL_ALGO_HVCFP_PARAM_T *pCurParam, const AX_OPAL_ALGO_HVCFP_PARAM_T *pNewParam) {

    // update ae roi
    memcpy(pCurParam->stAeRoiConfig, pNewParam->stAeRoiConfig, sizeof(pNewParam->stAeRoiConfig));

    // enable
    pCurParam->bEnable = pNewParam->bEnable;
    pCurParam->bPushActive = pNewParam->bPushActive;

    pCurParam->nSrcFramerate = pNewParam->nSrcFramerate;
    pCurParam->nDstFramerate = pNewParam->nDstFramerate;

    // update roi
    SetRoi(nSnsId, pSkel, &pCurParam->stRoiConfig, &pNewParam->stRoiConfig);

    // update push strategy
    SetPushStrategy(nSnsId, pSkel, &pCurParam->stPushStrategy, &pNewParam->stPushStrategy);

    // update object fliter
    for (size_t i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetObjectFilter(nSnsId, pSkel, (AX_OPAL_ALGO_HVCFP_TYPE_E)i, &pCurParam->stObjectFliterConfig[i], &pNewParam->stObjectFliterConfig[i]);
    }

    // update track size
    SetTrackSize(nSnsId, pSkel, &pCurParam->stTrackSize, &pNewParam->stTrackSize);

    // update panorama
    SetPanorama(nSnsId, pSkel, pNewParam->nCropEncoderQpLevel, &pCurParam->stPanoramaConfig, &pNewParam->stPanoramaConfig);

    // update crop encoder qpLevel
    SetCropEncoderQpLevel(nSnsId, pSkel, &pCurParam->nCropEncoderQpLevel, &pNewParam->nCropEncoderQpLevel);

#if 0
    // update crop threshold
    for (size_t i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetCropThreshold(nSnsId, pSkel, (AX_OPAL_ALGO_HVCFP_TYPE_E)i, &pCurParam->stCropThresholdConfig[i], &pCurParam->stCropThresholdConfig[i]);
    }
#endif

    // update push fliter
    for (size_t i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetPushFilter(nSnsId, pSkel, (AX_OPAL_ALGO_HVCFP_TYPE_E)i, &pCurParam->stPushFliterConfig[i], &pNewParam->stPushFliterConfig[i]);
    }

    return 0;
}

AX_S32  AX_OPAL_HAL_InitSkelParam(AX_U8 nSnsId, AX_SKEL_HANDLE pSkel, AX_OPAL_ALGO_HVCFP_PARAM_T *pParam) {

    // update roi
    SetRoi(nSnsId, pSkel, NULL, &pParam->stRoiConfig);

    // update push strategy
    SetPushStrategy(nSnsId, pSkel, NULL, &pParam->stPushStrategy);

    // update object fliter
    for (size_t i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetObjectFilter(nSnsId, pSkel, (AX_OPAL_ALGO_HVCFP_TYPE_E)i, NULL, &pParam->stObjectFliterConfig[i]);
    }

    // update track size
    SetTrackSize(nSnsId, pSkel, NULL, &pParam->stTrackSize);

    // update panorama
    SetPanorama(nSnsId, pSkel, pParam->nCropEncoderQpLevel, NULL, &pParam->stPanoramaConfig);

    // update crop encoder qpLevel
    SetCropEncoderQpLevel(nSnsId, pSkel, NULL, &pParam->nCropEncoderQpLevel);

#if 0
    // update crop threshold
    for (size_t i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetCropThreshold(nSnsId, pSkel, (AX_OPAL_ALGO_HVCFP_TYPE_E)i, NULL, &pParam->stCropThresholdConfig[i]);
    }
#endif

    // update push fliter
    for (size_t i = 0; i < AX_OPAL_ALGO_HVCFP_TYPE_BUTT; i++) {
        SetPushFilter(nSnsId, pSkel, (AX_OPAL_ALGO_HVCFP_TYPE_E)i, NULL, &pParam->stPushFliterConfig[i]);
    }

    return 0;

}