/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_cam_parser.h"

#include "iniparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "ax_opal_log.h"
#define LOG_TAG     "INI_CAM"

#define SEC_SENSOR  ("sensor")
#define SEC_MIPI    ("mipi")
#define SEC_DEV     ("dev")
#define SEC_PIPE    ("pipe")
#define SEC_CHN     ("chn0")
#define MAX_INI_KEY_LENGTH  (64)

static AX_BOOL get_inistring(opal_dictionary *dict, AX_CHAR *section, AX_CHAR *key, AX_CHAR *value, AX_S32 value_len) {

    char full_key[MAX_INI_KEY_LENGTH] = {0};
    snprintf(full_key, MAX_INI_KEY_LENGTH, "%s:%s", section, key);
    const char* tmp = opal_iniparser_getstring(dict, full_key, NULL);
    if (tmp == NULL) {
        LOG_M_E(LOG_TAG, "opal_iniparser_getstring failed, section: %s, key: %s", section, key);
        return AX_FALSE;
    }
    if (strlen(tmp) >= value_len) {
        LOG_M_E(LOG_TAG, "parse ini failed, value too long, section: %s, key: %s", section, key);
        return AX_FALSE;
    }
    memcpy(value, tmp, strlen(tmp));
    return AX_TRUE;
}

static AX_BOOL get_ini_f32(opal_dictionary *dict, AX_CHAR *section, AX_CHAR *key, AX_F32 *value) {
    char str_value[MAX_INI_KEY_LENGTH] = {0};
    if (!get_inistring(dict, section, key, str_value, MAX_INI_KEY_LENGTH)) {
        LOG_M_E(LOG_TAG, "Failed to get string from INI for float value, section: %s, key: %s", section, key);
        return AX_FALSE;
    }

    // LOG_M_N(LOG_TAG, "section: %s, key: %s, value: %s", section, key, str_value);
    char *endptr;
    *value = strtof(str_value, &endptr);
    if (*endptr != '\0') {
        LOG_M_E(LOG_TAG, "Invalid int value in INI, section: %s, key: %s", section, key);
        return AX_FALSE;
    }
    return AX_TRUE;
}

static AX_BOOL get_ini_s32(opal_dictionary *dict, AX_CHAR *section, AX_CHAR *key, int *value) {
    char str_value[MAX_INI_KEY_LENGTH]  = {0};
    if (!get_inistring(dict, section, key, str_value, MAX_INI_KEY_LENGTH)) {
        LOG_M_E(LOG_TAG, "Failed to get string from INI for int value, section: %s, key: %s", section, key);
        return AX_FALSE;
    }
    // LOG_M_N(LOG_TAG, "section: %s, key: %s, value: %s", section, key, str_value);
    char *endptr;
    long int val = strtol(str_value, &endptr, 10);
    if (*endptr != '\0') {
        LOG_M_E(LOG_TAG, "Invalid int value in INI, section: %s, key: %s", section, key);
        return AX_FALSE;
    }

    if ((val < INT_MIN) || (val > INT_MAX)) {
        LOG_M_E(LOG_TAG, "Int value out of range, section: %s, key: %s", section, key);
        return AX_FALSE;
    }

    *value = (int)val;
    return AX_TRUE;
}


static AX_S32 parse_cam(opal_dictionary* dict, AX_OPAL_MAL_CAM_ID_T* ptCamId) {
    AX_S32 nRet = -1;
    do {
        ptCamId->nRxDevId = opal_iniparser_getint(dict, "cam:nRxDev", -1);
        if (ptCamId->nRxDevId == -1) {
            break;
        }

        ptCamId->nDevId = opal_iniparser_getint(dict, "cam:nDevId", -1);
        if (ptCamId->nDevId == -1) {
            break;
        }
        ptCamId->nPipeId = opal_iniparser_getint(dict, "cam:nPipeId", -1);
        if (ptCamId->nPipeId == -1) {
            break;
        }

        ptCamId->nChnId = 0;
        nRet = 0;

    } while(0);
    return nRet;
}

static AX_S32 parse_sensor(opal_dictionary* dict, AX_OPAL_CAM_SENSOR_CFG_T* pSensorCfg) {
    AX_S32 nRet = -1;
    do {
        if (!get_inistring(dict, SEC_SENSOR, "ObjName", pSensorCfg->cObjName, AX_OPAL_MAX_NAME_SIZE)) {
            break;
        }

        if (!get_inistring(dict, SEC_SENSOR, "LibName", pSensorCfg->cLibName, AX_OPAL_MAX_NAME_SIZE)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "nWidth", &pSensorCfg->nWidth)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "nHeight", &pSensorCfg->nHeight)) {
            break;
        }

        if (!get_ini_f32(dict, SEC_SENSOR, "fFrameRate", &pSensorCfg->fFrameRate)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "eSnsMode", (AX_S32*)&pSensorCfg->eSnsMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "eRawType", (AX_S32*)&pSensorCfg->eRawType)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "eBayerPattern", (AX_S32*)&pSensorCfg->eBayerPattern)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "bTestPatternEnable", &pSensorCfg->bTestPatternEnable)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "bTestPatternEnable", &pSensorCfg->eMasterSlaveSel)) {
            break;
        }

        AX_CHAR cSettingIndex[AX_OPAL_MAX_NAME_SIZE];
        if (!get_inistring(dict, SEC_SENSOR, "nSettingIndex", cSettingIndex, AX_OPAL_MAX_NAME_SIZE)) {
            break;
        }
        {
            char *token = strtok(cSettingIndex, ",");
            if (token == NULL) {
                break;
            }
            AX_S32 index = 0;
            while (token != NULL) {
                pSensorCfg->nSettingIndex[index++] = atoi(token);
                token = strtok(NULL, ",");
            }
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "nBusType", &pSensorCfg->nBusType)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "nDevNode", &pSensorCfg->nDevNode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "nI2cAddr", &pSensorCfg->nI2cAddr)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "nSnsClkIdx", &pSensorCfg->nSnsClkIdx)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_SENSOR, "eSnsClkRate", &pSensorCfg->eSnsClkRate)) {
            break;
        }

        nRet = 0;
    } while(0);

    return nRet;
}

static AX_S32 parse_mipi(opal_dictionary* dict, AX_OPAL_CAM_MIPI_CFG_T* pMipiCfg) {
    AX_S32 nRet = -1;
    do {
        if (!get_ini_s32(dict, SEC_MIPI, "nResetGpio", &pMipiCfg->nResetGpio)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "eLaneComboMode", &pMipiCfg->eLaneComboMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "nMipiRxID", &pMipiCfg->nMipiRxID)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "eInputMode", (AX_S32*)&pMipiCfg->eInputMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "ePhyMode", (AX_S32*)&pMipiCfg->ePhyMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "eLaneNum", &pMipiCfg->eLaneNum)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "nDataRate", &pMipiCfg->nDataRate)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "nDataLaneMap0", &pMipiCfg->nDataLaneMap0)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "nDataLaneMap1", &pMipiCfg->nDataLaneMap1)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "nDataLaneMap2", &pMipiCfg->nDataLaneMap2)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "nDataLaneMap3", &pMipiCfg->nDataLaneMap3)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "nClkLane0", &pMipiCfg->nClkLane0)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_MIPI, "nClkLane1", &pMipiCfg->nClkLane1)) {
            break;
        }

        nRet = 0;
    } while(0);
    return nRet;
}

static AX_S32 parse_dev(opal_dictionary* dict, AX_OPAL_CAM_DEV_CFG_T* pDevCfg) {
    AX_S32 nRet = -1;
    do {
        if (!get_ini_s32(dict, SEC_DEV, "eSnsIntfType", (AX_S32*)&pDevCfg->eSnsIntfType)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "tDevImgRgn.nStartX", &pDevCfg->tDevImgRgnStartX)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "tDevImgRgn.nStartY", &pDevCfg->tDevImgRgnStartY)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "tDevImgRgn.nWidth", &pDevCfg->tDevImgRgnWidth)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "tDevImgRgn.nHeight", &pDevCfg->tDevImgRgnHeight)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "ePixelFmt", (AX_S32*)&pDevCfg->ePixelFmt)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "eBayerPattern", (AX_S32*)&pDevCfg->eBayerPattern)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "eSkipFrame", &pDevCfg->eSkipFrame)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "eSnsMode", (AX_S32*)&pDevCfg->eSnsMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "eDevMode", &pDevCfg->eDevMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "bImgDataEnable", &pDevCfg->bImgDataEnable)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "bNonImgEnable", &pDevCfg->bNonImgEnable)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "tMipiIntfAttr.szImgVc", &pDevCfg->tMipiIntfAttrImgVc)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "tMipiIntfAttr.szImgDt", &pDevCfg->tMipiIntfAttrImgDt)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "tMipiIntfAttr.szInfoVc", &pDevCfg->tMipiIntfAttrInfoVc)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "tMipiIntfAttr.szInfoDt", &pDevCfg->tMipiIntfAttrInfoDt)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_DEV, "eSnsOutputMode", &pDevCfg->eSnsOutputMode)) {
            break;
        }

        nRet = 0;
    } while(0);
    return nRet;

}

static AX_S32 parse_pipe(opal_dictionary* dict, AX_OPAL_CAM_PIPE_CFG_T* pPipeCfg) {
    AX_S32 nRet = -1;
    do {
        if (!get_ini_s32(dict, SEC_PIPE, "nWidth", &pPipeCfg->nWidth)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "nHeight", &pPipeCfg->nHeight)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "nWidthStride", &pPipeCfg->nWidthStride)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "eBayerPattern", (AX_S32*)&pPipeCfg->eBayerPattern)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "ePixelFmt", (AX_S32*)&pPipeCfg->ePixelFmt)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "eSnsMode", (AX_S32*)&pPipeCfg->eSnsMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "eWorkMode", &pPipeCfg->eWorkMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "bAiIspEnable", &pPipeCfg->bAiIspEnable)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "enCompressMode", &pPipeCfg->enCompressMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "bHMirror", &pPipeCfg->bHMirror)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "b3DnrIsCompress", &pPipeCfg->b3DnrIsCompress)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "bAinrIsCompress", &pPipeCfg->bAinrIsCompress)) {
            break;
        }

        if (!get_ini_f32(dict, SEC_PIPE, "nSrcFrameRate", &pPipeCfg->fSrcFrameRate)) {
            break;
        }

        if (!get_ini_f32(dict, SEC_PIPE, "nDstFrameRate", &pPipeCfg->fDstFrameRate)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "nLoadRawNode", &pPipeCfg->nLoadRawNode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "nDumpNodeMask", &pPipeCfg->nDumpNodeMask)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "nScalerRatio", &pPipeCfg->nScalerRatio)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "bMotionEst", &pPipeCfg->bMotionEst)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "eVinIvpsMode", &pPipeCfg->eVinIvpsMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "bMotionShare", &pPipeCfg->bMotionShare)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "bMotionComp", &pPipeCfg->bMotionComp)) {
            break;
        }

        if (!get_inistring(dict, SEC_PIPE, "strSdrBinPath", pPipeCfg->stCamIspCfg.cBinPathName[0], AX_OPAL_MAX_BINPATH_SIZE)) {
            break;
        }

        if (!get_inistring(dict, SEC_PIPE, "strHdrBinPath", pPipeCfg->stCamIspCfg.cBinPathName[1], AX_OPAL_MAX_BINPATH_SIZE)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "bNTEnable", &pPipeCfg->stCamNTCfg.bEnable)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "nNTCtrlPort", &pPipeCfg->stCamNTCfg.nCtrlPort)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_PIPE, "nNTStreamPort", &pPipeCfg->stCamNTCfg.nStreamPort)) {
            break;
        }

       nRet = 0;
    } while(0);

    return nRet;
}

static AX_S32 parse_chn(opal_dictionary* dict, AX_OPAL_CAM_CHN_CFG_T* pChnCfg) {
    AX_S32 nRet = -1;
    do {
        if (!get_ini_s32(dict, SEC_CHN, "nWidth", &pChnCfg->nWidth)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_CHN, "nHeight", &pChnCfg->nHeight)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_CHN, "nWidthStride", &pChnCfg->nWidthStride)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_CHN, "ePixelFmt", (AX_S32*)&pChnCfg->ePixelFmt)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_CHN, "bEnable", &pChnCfg->bEnable)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_CHN, "nDepth", &pChnCfg->nDepth)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_CHN, "bFlip", &pChnCfg->bFlip)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_CHN, "enCompressMode", &pChnCfg->enCompressMode)) {
            break;
        }

        if (!get_ini_s32(dict, SEC_CHN, "u32CompressLevel", &pChnCfg->u32CompressLevel)) {
            break;
        }
        nRet = 0;
    } while(0);
    return nRet;

}

AX_S32 AX_OPAL_HAL_CAM_Parse(const AX_CHAR* pFileName, AX_OPAL_CAM_CFG_T* ptCamCfg, AX_OPAL_MAL_CAM_ID_T* ptCamId) {
    AX_S32 nRet = -1;

    opal_dictionary* dict = NULL;
    do {
        dict = opal_iniparser_load(pFileName);
        if (!dict) {
            LOG_M_E(LOG_TAG, "opal_iniparser_load(%s) failed", pFileName);
            break;
        }

        nRet = parse_cam(dict, ptCamId);
        if (nRet != 0) {
            break;
        }

        nRet = parse_sensor(dict, &ptCamCfg->tSensorCfg);
        if (nRet != 0) {
            break;
        }

        nRet = parse_mipi(dict, &ptCamCfg->tMipiCfg);
        if (nRet != 0) {
            break;
        }

        nRet = parse_dev(dict, &ptCamCfg->tDevCfg);
        if (nRet != 0) {
            break;
        }

        nRet = parse_pipe(dict, &ptCamCfg->tPipeCfg);
        if (nRet != 0) {
            break;
        }

        nRet = parse_chn(dict, &ptCamCfg->tChnCfg);
        if (nRet != 0) {
            break;
        }

        nRet = 0;
    } while(0);

    if (dict != NULL) opal_iniparser_freedict(dict);
    return nRet;
}