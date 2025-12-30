
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_ppl_parser.h"
#include "iniparser.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ax_opal_log.h"
#define LOG_TAG ("INI_PPL")

#define SEC_MAX_LEN                 (16)
#define SEC_KEY_MAX_LEN             (64)


static AX_S32 parse_id(AX_CHAR *pName) {
    AX_S32 nId = -1;
    const char *underscore = strchr(pName, '_');
    if (underscore) {
        nId = atoi(underscore + 1);
    }
    return nId;
}

static AX_S32 parse_grp(opal_dictionary* dict, AX_CHAR* pSection, AX_OPAL_GRP_ATTR_T* ptGrpAttr) {
    AX_S32 nGrpCnt = 0;
    char section_key[SEC_KEY_MAX_LEN] = {0};

    for (AX_S32 iGrp = 0; iGrp < AX_OPAL_MAX_GRP_CNT; ++iGrp) {

        ptGrpAttr[iGrp].nGrpId = -1;
        ptGrpAttr[iGrp].nUniGrpId = -1;

        memset(section_key, 0, SEC_KEY_MAX_LEN);
        snprintf(section_key, SEC_KEY_MAX_LEN, "%s:GRP_%d", pSection, iGrp);
        if (opal_iniparser_find_entry(dict, section_key)) {
            /* nGrpId */
            ptGrpAttr[iGrp].nGrpId = opal_iniparser_getint(dict, section_key, -1);
            if (ptGrpAttr[iGrp].nGrpId == -1) {
                break;
            }
            nGrpCnt++;

            /* channel attr */
            ptGrpAttr[iGrp].nChnCnt = 0;
            for (AX_S32 iChn = 0; iChn < AX_OPAL_MAX_CHN_CNT; ++iChn) {

                ptGrpAttr[iGrp].nChnId[iChn] = -1;
                ptGrpAttr[iGrp].nUniChnId[iChn] = -1;

                memset(section_key, 0, SEC_KEY_MAX_LEN);
                snprintf(section_key, SEC_KEY_MAX_LEN, "%s:GRP_%d.CHN_%d", pSection, iGrp, iChn);
                if (opal_iniparser_find_entry(dict, section_key)) {
                    /* nChnId */
                    ptGrpAttr[iGrp].nChnId[iChn] = opal_iniparser_getint(dict, section_key, -1);
                    /* nChnCnt */
                    ptGrpAttr[iGrp].nChnCnt++;
                }
            }
        }
    }

    return nGrpCnt;
}

static AX_OPAL_UNIT_TYPE_E cvtEleTypeStr2Enum(AX_CHAR* pEleSec) {
    const char *underscore = strchr(pEleSec, '_');
    if (underscore) {
        /* eType cTypeName */
        /* FIX-ME: need to add element parsing here */
        char type_name[SEC_MAX_LEN];
        strncpy(type_name, pEleSec, underscore - pEleSec);
        type_name[underscore - pEleSec] = '\0';
        if (strcmp(type_name, "CAM") == 0) {
            return AX_OPAL_ELE_CAM;
        } else if (strcmp(type_name, "IVPS") == 0) {
            return AX_OPAL_ELE_IVPS;
        } else if (strcmp(type_name, "VENC") == 0) {
            return AX_OPAL_ELE_VENC;
        } else if (strcmp(type_name, "ALGO") == 0) {
            return AX_OPAL_ELE_ALGO;
        } else if (strcmp(type_name, "SUBPPL") == 0) {
            return AX_OPAL_SUBPPL;
        }
        else {
            LOG_M_E(LOG_TAG, "Invalid element section (%s)", pEleSec);
            return AX_OPAL_UNIT_TYPE_BUTT;
        }
    }
    LOG_M_E(LOG_TAG, "Invalid element section (%s)", pEleSec);
    return AX_OPAL_UNIT_TYPE_BUTT;
}

static AX_BOOL parse_ele(opal_dictionary* dict, AX_CHAR* pEleSection, AX_OPAL_ELEMENT_ATTR_T* ptEleAttr) {
    ptEleAttr->eType = cvtEleTypeStr2Enum(pEleSection);
    ptEleAttr->nGrpCnt = parse_grp(dict, pEleSection, ptEleAttr->arrGrpAttr);
    if (ptEleAttr->nGrpCnt == 0) {
        return AX_FALSE;
    }
    return AX_TRUE;
}

static AX_OPAL_UNITLINK_TYPE_E cvtLinkTypeStr2Enum(AX_CHAR* pLinkType) {

    if (strcmp(pLinkType, "link_in") == 0) {
        return AX_OPAL_SUBPPL_IN;
    } else if (strcmp(pLinkType, "link_out") == 0) {
        return AX_OPAL_SUBPPL_OUT;
    } else if (strcmp(pLinkType, "link") == 0) {
        return AX_OPAL_ELE_LINK;
    } else if (strcmp(pLinkType, "link_frm") == 0) {
        return AX_OPAL_ELE_NONLINK_FRM;
    }
    LOG_M_E(LOG_TAG, "Invalid link type section (%s)", pLinkType);
    return AX_OPAL_LINK_BUTT;
}

static AX_BOOL parse_link(opal_dictionary* dict, AX_CHAR* pLinkValue, AX_OPAL_LINK_ATTR_T* plinkAttr) {

    AX_CHAR *token;
    AX_CHAR *last = pLinkValue;
    AX_CHAR *delimiter = ".";
    AX_CHAR section_key[SEC_KEY_MAX_LEN] = {0};

    AX_CHAR srcsec[SEC_MAX_LEN] = {0};
    AX_CHAR srcgrp[SEC_MAX_LEN] = {0};
    AX_CHAR srcchn[SEC_MAX_LEN] = {0};

    AX_CHAR link[SEC_MAX_LEN] = {0};

    AX_CHAR dstsec[SEC_MAX_LEN] = {0};
    AX_CHAR dstgrp[SEC_MAX_LEN] = {0};
    AX_CHAR dstchn[SEC_MAX_LEN] = {0};

    AX_S32 index = 0;
    while ((token = strsep(&last, delimiter)) != NULL) {
        if (index == 0) strcpy(srcsec, token);
        if (index == 1) strcpy(srcgrp, token);
        if (index == 2) strcpy(srcchn, token);
        if (index == 3) strcpy(link, token);
        if (index == 4) strcpy(dstsec, token);
        if (index == 5) strcpy(dstgrp, token);
        if (index == 6) strcpy(dstchn, token);
        index++;
    }

    /* src */
    plinkAttr->eSrcType = cvtEleTypeStr2Enum(srcsec);

    plinkAttr->nSrcEleId = parse_id(srcsec);

    memset(section_key, 0, SEC_KEY_MAX_LEN);
    snprintf(section_key, SEC_KEY_MAX_LEN, "%s:%s", srcsec, srcgrp);
    plinkAttr->nSrcGrpId = opal_iniparser_getint(dict, section_key, -1);

    memset(section_key, 0, SEC_KEY_MAX_LEN);
    snprintf(section_key, SEC_KEY_MAX_LEN, "%s:%s.%s", srcsec, srcgrp, srcchn);
    plinkAttr->nSrcChnId = opal_iniparser_getint(dict, section_key, -1);

    /* dst */
    plinkAttr->eDstType = cvtEleTypeStr2Enum(dstsec);

    plinkAttr->nDstEleId = parse_id(dstsec);

    memset(section_key, 0, SEC_KEY_MAX_LEN);
    snprintf(section_key, SEC_KEY_MAX_LEN, "%s:%s", dstsec, dstgrp);
    plinkAttr->nDstGrpId = opal_iniparser_getint(dict, section_key, -1);

    memset(section_key, 0, SEC_KEY_MAX_LEN);
    snprintf(section_key, SEC_KEY_MAX_LEN, "%s:%s.%s", dstsec, dstgrp, dstchn);
    plinkAttr->nDstChnId = opal_iniparser_getint(dict, section_key, -1);

    /* link type */
    plinkAttr->eLinkType = cvtLinkTypeStr2Enum(link);

    return AX_TRUE;
}

static AX_BOOL parse_subppl(opal_dictionary* dict, AX_CHAR *pSubPplSecName, AX_OPAL_SUBPPL_ATTR_T *pSubPplAttr) {

    char section_key[SEC_KEY_MAX_LEN] = {0};

    /* id */
    pSubPplAttr->nId = parse_id(pSubPplSecName);
    if (pSubPplAttr->nId == -1) {
        return AX_FALSE;
    }

    /* sub-ppl attr */
    pSubPplAttr->nGrpCnt = parse_grp(dict, pSubPplSecName, pSubPplAttr->arrGrpAttr);

    /* eles */
    memset(section_key, 0, SEC_KEY_MAX_LEN);
    snprintf(section_key, SEC_KEY_MAX_LEN, "%s:ELES", pSubPplSecName);
    if (opal_iniparser_find_entry(dict, section_key)) {
        AX_CHAR* pEles = (AX_CHAR*)opal_iniparser_getstring(dict, section_key, NULL);
        if (pEles) {
            char *token;
            const char delimiters[] = ",";
            token = strtok(pEles, delimiters);
            AX_S32 nEleIndex = 0;
            while (token != NULL) {
                AX_CHAR *pEleSecName = (AX_CHAR*)token;
                /* ele id */
                pSubPplAttr->arrEleAttr[nEleIndex].nId = parse_id(pEleSecName);
                if (pSubPplAttr->arrEleAttr[nEleIndex].nId == -1) {
                    return AX_FALSE;
                }
                /* ele attr */
                if (AX_FALSE == parse_ele(dict, pEleSecName, &pSubPplAttr->arrEleAttr[nEleIndex])) {
                    return AX_FALSE;
                }
                nEleIndex++;
                token = strtok(NULL, delimiters);
            }
            pSubPplAttr->nEleCnt = nEleIndex;
        }
    }

    /* links */
    memset(section_key, 0, SEC_KEY_MAX_LEN);
    snprintf(section_key, SEC_KEY_MAX_LEN, "%s:LINKS", pSubPplSecName);
    if (opal_iniparser_find_entry(dict, section_key)) {
        AX_CHAR* pLinkSecName = (AX_CHAR*)opal_iniparser_getstring(dict, section_key, NULL);
        if (pLinkSecName == AX_NULL) {
            return AX_FALSE;
        }
        pSubPplAttr->nLinkCnt = 0;
        for (AX_S32 iLink = 0; iLink < AX_OPAL_MAX_LINK_CNT; ++iLink) {
            memset(section_key, 0, SEC_KEY_MAX_LEN);
            snprintf(section_key, SEC_KEY_MAX_LEN, "%s:LINK_%d", pLinkSecName, iLink);
            AX_CHAR* pLink= (AX_CHAR*)opal_iniparser_getstring(dict, section_key, NULL);
            if (pLink == AX_NULL) {
                return AX_FALSE;
            }

            if (AX_FALSE == parse_link(dict, pLink, &pSubPplAttr->arrLinkAttr[iLink])) {
                return AX_FALSE;
            }
            pSubPplAttr->nLinkCnt++;
        }
    }

    return AX_TRUE;
}

AX_S32 AX_OPAL_MAL_PPL_Parse(const AX_CHAR* pFileName, AX_OPAL_PPL_ATTR_T* pPplAttr) {

    AX_S32 nRet = AX_SUCCESS;

    opal_dictionary* dict = NULL;
    dict = opal_iniparser_load(pFileName);
    if (!dict) {
        LOG_M_E(LOG_TAG, "opal_iniparser_load(pipeline ini file:%s) failed", pFileName);
        return -1;
    }

    /* arrSubPpls */
    char section_key[SEC_KEY_MAX_LEN] = {0};
    for (AX_S32 iSubPpl = 0; iSubPpl < AX_OPAL_MAX_SUBPPL_CNT; ++iSubPpl) {
        memset(section_key, 0, SEC_KEY_MAX_LEN);
        snprintf(section_key, SEC_KEY_MAX_LEN, "PPL:SUBPPL_%d", iSubPpl);
        // check if subppl exist
        if (opal_iniparser_find_entry(dict, section_key)) {
            /* parse subppl */
            AX_CHAR* pSubPplSecName = (AX_CHAR*)opal_iniparser_getstring(dict, section_key, NULL);
            if (!parse_subppl(dict, pSubPplSecName, &pPplAttr->arrSubPplAttr[iSubPpl])) {
                return -1;
            }
            pPplAttr->nSubPplCnt++;
        }
    }

    if (dict != NULL) {
        opal_iniparser_freedict(dict);
    }
    return nRet;
}


static AX_S32 parse_pool(opal_dictionary* dict, AX_CHAR* section_key, AX_OPAL_HAL_POOL_CFG_T* ptPool) {

    AX_S32 nRet = -1;
    LOG_M_D(LOG_TAG, "+++ %s", section_key);

    AX_CHAR *token = NULL;

    /* get value */
    ptPool->nBlkCnt = opal_iniparser_getint(dict, section_key, 0);
    LOG_M_D(LOG_TAG, "nBlkCnt=%d", ptPool->nBlkCnt);

    /* get key */
    AX_CHAR sec[SEC_KEY_MAX_LEN] = {0};
    AX_CHAR key[SEC_KEY_MAX_LEN] = {0};
    {
        AX_S32 index = 0;
        AX_CHAR *delimiter = ":";
        AX_CHAR *last = section_key;
        while ((token = strsep(&last, delimiter)) != NULL) {
            if (index == 0) strcpy(sec, token);
            if (index == 1) strcpy(key, token);
            index++;
        }
        LOG_M_D(LOG_TAG, "section_key=%s sec=%s key=%s", section_key, sec, key);
    }

    /* parse key name string */
    const char* prefix_raw = "RAW";
    const char* prefix_yuv = "YUV";
    if (0 == strncmp(key, prefix_raw, strlen(prefix_raw))) {
        nRet = 0; // private
        // RAW_133_MAX_FBC_2_4

        AX_CHAR raw[SEC_MAX_LEN] = {0};
        AX_CHAR fmt[SEC_MAX_LEN] = {0};
        AX_CHAR res[SEC_MAX_LEN] = {0};
        AX_CHAR fbc[SEC_MAX_LEN] = {0};
        AX_CHAR fbc_type[SEC_MAX_LEN] = {0};
        AX_CHAR fbc_level[SEC_MAX_LEN] = {0};

        AX_S32 index = 0;
        AX_CHAR *delimiter = "_";
        AX_CHAR *last = key;
        while ((token = strsep(&last, delimiter)) != NULL) {

            if (index == 0) strcpy(raw, token);
            if (index == 1) strcpy(fmt, token);
            if (index == 2) strcpy(res, token);
            if (index == 3) strcpy(fbc, token);
            if (index == 4) strcpy(fbc_type, token);
            if (index == 5) strcpy(fbc_level, token);
            index++;
        }

        ptPool->nFmt = (AX_IMG_FORMAT_E)atoi(fmt);
        ptPool->enCompressMode = (AX_COMPRESS_MODE_E)atoi(fbc_type);
        ptPool->u32CompressLevel = atoi(fbc_level);
        if (strcmp(res, "MAX") == 0) {
            ptPool->nWidth = -1;
            ptPool->nHeight = -1;
        } else {
            AX_CHAR w[SEC_MAX_LEN] = {0};
            AX_CHAR h[SEC_MAX_LEN] = {0};
            AX_S32 index = 0;
            AX_CHAR *delimiter = "x";
            AX_CHAR *last = res;
            while ((token = strsep(&last, delimiter)) != NULL) {
                if (index == 0) strcpy(w, token);
                if (index == 1) strcpy(h, token);
                index++;
            }
            ptPool->nWidth = atoi(w);
            ptPool->nHeight = atoi(h);
        }

        LOG_M_D(LOG_TAG, "nRet=%d key=%s (%d, %d) fmt=%d comp=(%d, %d)\n", nRet, key,
                ptPool->nWidth, ptPool->nHeight, ptPool->nFmt, ptPool->enCompressMode, ptPool->u32CompressLevel);
    }
    else if (0 == strncmp(key, prefix_yuv, strlen(prefix_yuv))) {
        nRet = 1; // common
        // YUV_720x576_FBC_2_4
        AX_CHAR raw[SEC_MAX_LEN] = {0};
        AX_CHAR res[SEC_MAX_LEN] = {0};
        AX_CHAR fbc[SEC_MAX_LEN] = {0};
        AX_CHAR fbc_type[SEC_MAX_LEN] = {0};
        AX_CHAR fbc_level[SEC_MAX_LEN] = {0};

        AX_S32 index = 0;
        AX_CHAR *delimiter = "_";
        AX_CHAR *last = key;
        while ((token = strsep(&last, delimiter)) != NULL) {
            if (index == 0) strcpy(raw, token);
            if (index == 1) strcpy(res, token);
            if (index == 2) strcpy(fbc, token);
            if (index == 3) strcpy(fbc_type, token);
            if (index == 4) strcpy(fbc_level, token);
            index++;
        }
        ptPool->nFmt = AX_FORMAT_YUV420_SEMIPLANAR;
        ptPool->enCompressMode = (AX_COMPRESS_MODE_E)atoi(fbc_type);
        ptPool->u32CompressLevel = atoi(fbc_level);
        if (strcmp(res, "MAX") == 0) {
            ptPool->nWidth = -1;
            ptPool->nHeight = -1;
        } else {
            AX_CHAR w[SEC_MAX_LEN] = {0};
            AX_CHAR h[SEC_MAX_LEN] = {0};
            AX_S32 index = 0;
            AX_CHAR *delimiter = "x";
            AX_CHAR *last = res;
            while ((token = strsep(&last, delimiter)) != NULL) {
                if (index == 0) strcpy(w, token);
                if (index == 1) strcpy(h, token);
                index++;
            }
            ptPool->nWidth = atoi(w);
            ptPool->nHeight = atoi(h);
        }
        LOG_M_D(LOG_TAG, "nRet=%d key=%s (%d, %d) fmt=%d comp=(%d, %d)\n", nRet, key,
                ptPool->nWidth, ptPool->nHeight, ptPool->nFmt, ptPool->enCompressMode, ptPool->u32CompressLevel);
    }
    else {
        nRet = -1;
        LOG_M_E(LOG_TAG, "unknown key: %s", key);
    }

    return nRet;
}

AX_S32 AX_OPAL_MAL_POOL_Parse(const AX_CHAR* pFileName, AX_OPAL_POOL_ATTR_T* ptPoolCfg) {

    AX_S32 s32Ret = 0;
    LOG_M_D(LOG_TAG, "+++");

    opal_dictionary* dict = NULL;
    dict = opal_iniparser_load(pFileName);
    if (!dict) {
        LOG_M_E(LOG_TAG, "opal_iniparser_load(%s) failed", pFileName);
        return -1;
    }

    const char **keys = NULL;
    do {
        const char *section_name = "POOL";
        AX_S32 nKeyCnt = opal_iniparser_getsecnkeys(dict, section_name);
        keys = malloc(nKeyCnt * sizeof(char *));
        if (keys == NULL) {
            LOG_M_E(LOG_TAG, "Memory allocation for keys failed");
            s32Ret = -1;
            break;
        }

        opal_iniparser_getseckeys(dict, section_name, keys);
        for (AX_S32 iKey = 0; iKey < nKeyCnt; ++iKey) {
            char *section_key = (char*)keys[iKey];

            AX_OPAL_HAL_POOL_CFG_T stPoolCfg;
            memset(&stPoolCfg, 0x0, sizeof(AX_OPAL_HAL_POOL_CFG_T));
            AX_S32 nRet = parse_pool(dict, section_key, &stPoolCfg);
            if (nRet == -1) {
                s32Ret = -1;
                LOG_M_E(LOG_TAG, "parse pool failed");
                break;
            }

            /* private */
            if (nRet == 0) {
                memcpy(&ptPoolCfg->arrPrivPoolCfg[ptPoolCfg->nPrivPoolCfgCnt], &stPoolCfg, sizeof(AX_OPAL_HAL_POOL_CFG_T));
                ptPoolCfg->nPrivPoolCfgCnt ++;
            }

            /* common */
            if (nRet == 1) {
                memcpy(&ptPoolCfg->arrCommPoolCfg[ptPoolCfg->nCommPoolCfgCnt], &stPoolCfg, sizeof(AX_OPAL_HAL_POOL_CFG_T));
                ptPoolCfg->nCommPoolCfgCnt ++;
            }
        }

    } while (0);

    if (keys != NULL) free(keys);
    if (dict != NULL) opal_iniparser_freedict(dict);

    LOG_M_D(LOG_TAG, "---");
    return s32Ret;
}