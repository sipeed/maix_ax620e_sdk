/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_ivps_parser.h"

#include <string.h>
#include "iniparser.h"

#include "ax_opal_log.h"
#define LOG_TAG "INI_IVPS"


#define SEC_MAX_LEN                 (16)
#define SEC_KEY_MAX_LEN             (64)

static AX_U32 cvt_enginge(const AX_CHAR* pName) {
    if (strcmp(pName, "SCL") == 0)  return 0;
    if (strcmp(pName, "TDP") == 0)  return 1;
    if (strcmp(pName, "GDC") == 0)  return 2;
    if (strcmp(pName, "VPP") == 0)  return 3;
    if (strcmp(pName, "VO") == 0)   return 4;
    return AX_IVPS_ENGINE_BUTT;
}

static AX_BOOL parse_grp(opal_dictionary* dict,  AX_OPAL_IVPS_GRP_ATTR_T* pGrpAttr) {

    AX_S32 nTmp = 0;

    const AX_CHAR* pEngineTypeName0 = opal_iniparser_getstring(dict, "GRP:engine_filter_0", AX_NULL);
    if (pEngineTypeName0 == AX_NULL) return AX_FALSE;
    pGrpAttr->eEngineType0 = cvt_enginge(pEngineTypeName0);

    const AX_CHAR* pEngineTypeName1 = opal_iniparser_getstring(dict, "GRP:engine_filter_1", AX_NULL);
    if (pEngineTypeName1 == AX_NULL) return AX_FALSE;
    pGrpAttr->eEngineType1 = cvt_enginge(pEngineTypeName1);

    pGrpAttr->tFramerate.fSrc = (AX_F32)opal_iniparser_getdouble(dict, "GRP:framerate_src", 0);
    if (pGrpAttr->tFramerate.fSrc == 0) return AX_FALSE;

    pGrpAttr->tFramerate.fDst = (AX_F32)opal_iniparser_getdouble(dict, "GRP:framerate_dst", 0);
    if (pGrpAttr->tFramerate.fDst == 0) return AX_FALSE;

    pGrpAttr->tResolution.nWidth = opal_iniparser_getint(dict, "GRP:resolution_w", 0);
    if (pGrpAttr->tResolution.nWidth == 0) return AX_FALSE;

    pGrpAttr->tResolution.nHeight = opal_iniparser_getint(dict, "GRP:resolution_h", 0);
    if (pGrpAttr->tResolution.nHeight == 0) return AX_FALSE;

    pGrpAttr->nFifoDepth = opal_iniparser_getint(dict, "GRP:in_fifo_depth", -1);
    if (pGrpAttr->nFifoDepth == -1) return AX_FALSE;

    nTmp = opal_iniparser_getint(dict, "GRP:compress_mode", -1);
    if (nTmp == -1) return AX_FALSE;
    pGrpAttr->tCompress.enCompressMode = nTmp;

    nTmp = opal_iniparser_getint(dict, "GRP:compress_level", -1);
    if (nTmp == -1) return AX_FALSE;
    pGrpAttr->tCompress.u32CompressLevel = nTmp;

    nTmp = opal_iniparser_getint(dict, "GRP:inplace_filter_0", -1);
    if (nTmp == -1) return AX_FALSE;
    pGrpAttr->bInplace0 = (AX_BOOL)nTmp;

    nTmp = opal_iniparser_getint(dict, "GRP:inplace_filter_1", 0);
    if (nTmp == -1) return AX_FALSE;
    pGrpAttr->bInplace1 = (AX_BOOL)nTmp;

    return AX_TRUE;
}

static AX_BOOL parse_chn(opal_dictionary* dict, const AX_CHAR* pSec, AX_OPAL_IVPS_CHN_ATTR_T* pChnAttr) {
    AX_S32 nTmp = 0;
    AX_CHAR sec_key[SEC_KEY_MAX_LEN];

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "engine_filter_0");
    const AX_CHAR* pEngineTypeName0 = opal_iniparser_getstring(dict, sec_key, AX_NULL);
    if (pEngineTypeName0 == AX_NULL) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->eEngineType0 = cvt_enginge(pEngineTypeName0);

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "engine_filter_1");
    const AX_CHAR* pEngineTypeName1 = opal_iniparser_getstring(dict, sec_key, AX_NULL);
    if (pEngineTypeName1 == AX_NULL) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->eEngineType1 = cvt_enginge(pEngineTypeName1);

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "framerate_src");
    pChnAttr->tFramerate.fSrc = (AX_F32)opal_iniparser_getdouble(dict, sec_key, 0);
    if (pChnAttr->tFramerate.fSrc == 0) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "framerate_dst");
    pChnAttr->tFramerate.fDst = (AX_F32)opal_iniparser_getdouble(dict, sec_key, 0);
    if (pChnAttr->tFramerate.fDst == 0) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "resolution_w");
    pChnAttr->tResolution.nWidth = opal_iniparser_getint(dict, sec_key, 0);
    if (pChnAttr->tResolution.nWidth == 0) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "resolution_h");
    pChnAttr->tResolution.nHeight = opal_iniparser_getint(dict, sec_key, 0);
    if (pChnAttr->tResolution.nHeight == 0) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "out_fifo_depth");
    pChnAttr->nFifoDepth = opal_iniparser_getint(dict, sec_key, -1);
    if (pChnAttr->nFifoDepth == -1) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "compress_mode");
    nTmp = opal_iniparser_getint(dict, sec_key, -1);
    if (nTmp == -1) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->tCompress.enCompressMode = nTmp;

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "compress_level");
    nTmp = opal_iniparser_getint(dict, sec_key, -1);
    if (nTmp == -1) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->tCompress.u32CompressLevel = nTmp;

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "inplace_filter_0");
    nTmp = opal_iniparser_getint(dict, sec_key, -1);
    if (nTmp == -1) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->bInplace0 = (AX_BOOL)nTmp;

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "inplace_filter_1");
    nTmp = opal_iniparser_getint(dict, sec_key, -1);
    if (nTmp == -1) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->bInplace1 = (AX_BOOL)nTmp;

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "scale_type");
    nTmp = opal_iniparser_getint(dict, sec_key, -1);
    if (nTmp == -1) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->eSclType = nTmp;

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "vo_osd_enable");
    nTmp = opal_iniparser_getint(dict, sec_key, -1);
    if (nTmp == -1) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->bVoOsd = (AX_BOOL)nTmp;

    memset(sec_key, 0, SEC_KEY_MAX_LEN);
    snprintf(sec_key, SEC_KEY_MAX_LEN, "%s:%s", pSec, "vo_rect_enable");
    nTmp = opal_iniparser_getint(dict, sec_key, -1);
    if (nTmp == -1) {
        LOG_M_E(LOG_TAG, "parse channel attr %s failed.", sec_key);
        return AX_FALSE;
    }
    pChnAttr->bVoRect = (AX_BOOL)nTmp;

    // TODO: OSD

    return AX_TRUE;
}

AX_S32 AX_OPAL_HAL_IVPS_Parse(const AX_CHAR* pFileName, AX_OPAL_IVPS_ATTR_T* ptIvpsAttr) {
    AX_S32 nRet = -1;
    opal_dictionary* dict = AX_NULL;
    do {
        dict = opal_iniparser_load(pFileName);
        if (!dict) {
            LOG_M_E(LOG_TAG, "opal_iniparser_load(%s) failed", pFileName);
            nRet = -1;
            break;
        }

        /* parse grp and id */
        AX_S32 nGrpId = opal_iniparser_getint(dict, "IVPS:grp_id", -1);
        if (nGrpId < 0) {
            LOG_M_E(LOG_TAG, "invalid grp id IVPS:grp_id (%d).", nGrpId);
            break;
        }
        AX_S32 nChnCnt = opal_iniparser_getint(dict, "IVPS:chn_cnt", -1);
        if (nGrpId < 0) {
            LOG_M_E(LOG_TAG, "invalid grp chn count IVPS:chn_cnt (%d).", nChnCnt);
            break;
        }

        // memset(ptIvpsId, -1, sizeof(AXOP_HAL_CAM_ID_T));
        // for (AX_S32 iChn = 0; iChn < nChnCnt; iChn++) {
        //     ptIvpsId->arrGroChnId[iChn].nGrpId = nGrpId;
        //     ptIvpsId->arrGroChnId[iChn].nChnId = iChn;
        // }

        /* parse grp attr */
        // ptIvpsAttr->nGrpId = nGrpId;
        if (!parse_grp(dict, &ptIvpsAttr->tGrpAttr)) {
            LOG_M_E(LOG_TAG, "parse group attr failed, file=%s", pFileName);
            break;
        }

        /* parse grp-chn attr */
        ptIvpsAttr->nGrpChnCnt = nChnCnt;
        char sec[SEC_MAX_LEN];
        for (AX_S32 iChn = 0;  iChn < nChnCnt; ++iChn) {
            memset(sec, 0, SEC_MAX_LEN);
            snprintf(sec, SEC_MAX_LEN, "CHN_%d", iChn);
            if (!parse_chn(dict, sec, &ptIvpsAttr->arrChnAttr[iChn])) {
                LOG_M_E(LOG_TAG, "parse chn[%d] attr failed, file=%s", iChn, pFileName);
                return nRet;
            }
        }

        nRet = 0;
    } while(0);

    if (dict != AX_NULL) {
        opal_iniparser_freedict(dict);
    }
    return nRet;
}