/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_sys.h"

#include "ax_engine_api.h"
#include "ax_vin_api.h"
#include "ax_sys_api.h"
#include "ax_buffer_tool.h"

#include "ax_opal_log.h"
#define LOG_TAG ("HAL_SYS")

extern AX_U32 add_plan(AX_POOL_FLOORPLAN_T *pPoolFloorPlan, AX_U32 nCfgCnt, AX_POOL_CONFIG_T *pPoolConfig);
extern AX_S32 calc_pool(AX_OPAL_HAL_POOL_CFG_T *pPoolCfg, AX_U32 nPoolCnt, AX_POOL_FLOORPLAN_T *pPoolFloorPlan);

extern AX_S32 cvtOpalMod2Mod(AX_OPAL_UNIT_TYPE_E eModType);

AX_S32 AX_OPAL_HAL_SYS_Init(AX_VOID) {
	AX_S32 nRet = 0;

	nRet = AX_SYS_Init();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SYS_Init failed, ret=0x%x", nRet);
        return -1;
    }

    AX_ENGINE_NPU_ATTR_T attr;
    memset(&attr, 0, sizeof(AX_ENGINE_NPU_ATTR_T));
    attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_ENABLE;
    nRet = AX_ENGINE_Init(&attr);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ENGINE_Init failed, ret=0x%x.", nRet);
        return nRet;
    }

	return 0;
}

AX_S32 AX_OPAL_HAL_SYS_Deinit(void) {
	AX_S32 nRet = 0;

	nRet = AX_ENGINE_Deinit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_ENGINE_Deinit failed, ret=0x%x", nRet);
        return -1;
    }

    nRet = AX_SYS_Deinit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SYS_Deinit failed, ret=0x%x", nRet);
        return -1;
    }

	return 0;
}

AX_S32 AX_OPAL_HAL_POOL_Init(AX_OPAL_HAL_POOL_ATTR_T *ptPoolAttr) {

    AX_S32 nRet = 0;

	AX_POOL_FLOORPLAN_T tCommFloorplan;
	AX_POOL_FLOORPLAN_T tPrivFloorplan;
	memset(&tCommFloorplan, 0, sizeof(AX_POOL_FLOORPLAN_T));
	memset(&tPrivFloorplan, 0, sizeof(AX_POOL_FLOORPLAN_T));

    /* Release last Pool */
    nRet = AX_POOL_Exit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_POOL_Exit failed, ret=0x%x", nRet);
    }

    /* common pool config */
    /* Calc Pool BlkSize/BlkCnt */
    AX_OPAL_HAL_POOL_CFG_T arrCommPoolCfg[AX_OPAL_HAL_MAX_POOL_CNT*AX_OPAL_SNS_ID_BUTT];
    memset(arrCommPoolCfg, 0x0, sizeof(AX_OPAL_HAL_POOL_CFG_T)*AX_OPAL_HAL_MAX_POOL_CNT*AX_OPAL_SNS_ID_BUTT);
    AX_S32 nCommPoolCfgCnt = 0;
    /* sensor 0 */
    if (ptPoolAttr->stPoolAttr[0].nCommPoolCfgCnt != 0) {
        nCommPoolCfgCnt = ptPoolAttr->stPoolAttr[0].nCommPoolCfgCnt;
        memcpy(arrCommPoolCfg, ptPoolAttr->stPoolAttr[0].arrCommPoolCfg, sizeof(AX_OPAL_HAL_POOL_CFG_T)*nCommPoolCfgCnt);
    }
    /* merge other */
    for (AX_S32 iSns = 1; iSns < AX_OPAL_SNS_ID_BUTT; ++iSns) {
        AX_OPAL_POOL_ATTR_T *pPoolAttr = &ptPoolAttr->stPoolAttr[iSns];
        if (pPoolAttr->nCommPoolCfgCnt == 0) {
            continue;
        }
        for (AX_S32 iPool = 0; iPool < pPoolAttr->nCommPoolCfgCnt; ++iPool) {
            for (AX_S32 iPoolUse = 0; iPoolUse < nCommPoolCfgCnt; ++iPoolUse) {
                if (arrCommPoolCfg[iPoolUse].nWidth == pPoolAttr->arrCommPoolCfg[iPool].nWidth
                    && arrCommPoolCfg[iPoolUse].nHeight == pPoolAttr->arrCommPoolCfg[iPool].nHeight
                    && arrCommPoolCfg[iPoolUse].nFmt == pPoolAttr->arrCommPoolCfg[iPool].nFmt
                    && arrCommPoolCfg[iPoolUse].enCompressMode == pPoolAttr->arrCommPoolCfg[iPool].enCompressMode
                    && arrCommPoolCfg[iPoolUse].u32CompressLevel == pPoolAttr->arrCommPoolCfg[iPool].u32CompressLevel) {
                    arrCommPoolCfg[iPoolUse].nBlkCnt += pPoolAttr->arrCommPoolCfg[iPool].nBlkCnt;
                    break;
                } else {
                    memcpy(&arrCommPoolCfg[nCommPoolCfgCnt], &pPoolAttr->arrCommPoolCfg[iPool], sizeof(AX_OPAL_HAL_POOL_CFG_T));
                    nCommPoolCfgCnt += 1;
                    break;
                }
            }
        }
    }

    nRet = calc_pool(arrCommPoolCfg, nCommPoolCfgCnt, &tCommFloorplan);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "calc_pool failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_POOL_SetConfig(&tCommFloorplan);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_POOL_SetConfig failed, ret=0x%x", nRet);
        return -1;
    }

    nRet = AX_POOL_Init();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_POOL_Init failed, ret=0x%x", nRet);
        return -1;
    }

    /* private pool config */
    /* Calc Pool BlkSize/BlkCnt */
    AX_OPAL_HAL_POOL_CFG_T arrPrivPoolCfg[AX_OPAL_HAL_MAX_POOL_CNT*AX_OPAL_SNS_ID_BUTT];
    memset(arrPrivPoolCfg, 0x0, sizeof(AX_OPAL_HAL_POOL_CFG_T)*AX_OPAL_HAL_MAX_POOL_CNT*AX_OPAL_SNS_ID_BUTT);
    AX_S32 nPrivPoolCfgCnt = 0;
    /* sensor 0 */
    if (ptPoolAttr->stPoolAttr[0].nPrivPoolCfgCnt != 0) {
        nPrivPoolCfgCnt = ptPoolAttr->stPoolAttr[0].nPrivPoolCfgCnt;
        memcpy(arrPrivPoolCfg, ptPoolAttr->stPoolAttr[0].arrPrivPoolCfg, sizeof(AX_OPAL_HAL_POOL_CFG_T)*nPrivPoolCfgCnt);
    }

    for (AX_S32 iSns = 1; iSns < AX_OPAL_SNS_ID_BUTT; ++iSns) {
        AX_OPAL_POOL_ATTR_T *pPoolAttr = &ptPoolAttr->stPoolAttr[iSns];
        if (pPoolAttr->nPrivPoolCfgCnt == 0) {
            continue;
        }

        for (AX_S32 iPool = 0; iPool < pPoolAttr->nPrivPoolCfgCnt; ++iPool) {
            for (AX_S32 iPoolUse = 0; iPoolUse < nPrivPoolCfgCnt; ++iPoolUse) {
                if (arrPrivPoolCfg[iPoolUse].nWidth == pPoolAttr->arrPrivPoolCfg[iPool].nWidth
                    && arrPrivPoolCfg[iPoolUse].nHeight == pPoolAttr->arrPrivPoolCfg[iPool].nHeight
                    && arrPrivPoolCfg[iPoolUse].nFmt == pPoolAttr->arrPrivPoolCfg[iPool].nFmt
                    && arrPrivPoolCfg[iPoolUse].enCompressMode == pPoolAttr->arrPrivPoolCfg[iPool].enCompressMode
                    && arrPrivPoolCfg[iPoolUse].u32CompressLevel == pPoolAttr->arrPrivPoolCfg[iPool].u32CompressLevel) {
                    arrPrivPoolCfg[iPoolUse].nBlkCnt += pPoolAttr->arrPrivPoolCfg[iPool].nBlkCnt;
                    break;
                } else {
                    memcpy(&arrPrivPoolCfg[nPrivPoolCfgCnt], &pPoolAttr->arrPrivPoolCfg[iPool], sizeof(AX_OPAL_HAL_POOL_CFG_T));
                    nPrivPoolCfgCnt += 1;
                    break;
                }
            }
        }
    }
    nRet = calc_pool(arrPrivPoolCfg, nPrivPoolCfgCnt, &tPrivFloorplan);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "CalcPool failed, ret=0x%x.", nRet);
        return -1;
    }

    nRet = AX_VIN_SetPoolAttr(&tPrivFloorplan);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG,"AX_VIN_SetPoolAttr failed, ret=0x%x", nRet);
        return -1;
    }

	return nRet;
}

AX_S32 AX_OPAL_HAL_POOL_Deinit(AX_VOID) {
    AX_S32 nRet = AX_POOL_Exit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_POOL_Exit failed, ret=0x%x.", nRet);
        return -1;
    }
	return 0;
}

AX_S32 AX_OPAL_HAL_MOD_Link(AX_OPAL_LINK_ATTR_T *pLinkAttr) {
    AX_MOD_INFO_T stSrc = {cvtOpalMod2Mod(pLinkAttr->eSrcType), pLinkAttr->nSrcGrpId, pLinkAttr->nSrcChnId};
    AX_MOD_INFO_T stDst = {cvtOpalMod2Mod(pLinkAttr->eDstType), pLinkAttr->nDstGrpId, pLinkAttr->nDstChnId};
    return AX_SYS_Link(&stSrc, &stDst);
}

AX_S32 AX_OPAL_HAL_MOD_UnLink(AX_OPAL_LINK_ATTR_T *pLinkAttr) {
    AX_MOD_INFO_T stSrc = {cvtOpalMod2Mod(pLinkAttr->eSrcType), pLinkAttr->nSrcGrpId, pLinkAttr->nSrcChnId};
    AX_MOD_INFO_T stDst = {cvtOpalMod2Mod(pLinkAttr->eDstType), pLinkAttr->nDstGrpId, pLinkAttr->nDstChnId};
    return AX_SYS_UnLink(&stSrc, &stDst);
}

AX_S32 AX_OPAL_HAL_SetVinIvpsMode(AX_S32 nVinPipe, AX_S32 IvpsGrp, AX_S32 eMode) {
    AX_S32 nRet = AX_SYS_SetVINIVPSMode(nVinPipe, IvpsGrp, (AX_VIN_IVPS_MODE_E)eMode);
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_SYS_SetVINIVPSMode failed, ret=0x%x.", nRet);
        return -1;
    }
	return 0;
}

AX_POOL AX_OPAL_HAL_SYS_CreatePool(AX_U32 nFrameSize, AX_U32 nDepth, const AX_CHAR* PoolName) {
    AX_POOL_CONFIG_T stPoolConfig;
    memset(&stPoolConfig, 0, sizeof(AX_POOL_CONFIG_T));
    stPoolConfig.MetaSize = 4096;
    stPoolConfig.BlkCnt = (nDepth == 0) ? 1 : nDepth;
    stPoolConfig.BlkSize = nFrameSize;
    stPoolConfig.CacheMode = AX_POOL_CACHE_MODE_NONCACHE;
    memset(stPoolConfig.PartitionName, 0, sizeof(stPoolConfig.PartitionName));
    strcpy((AX_CHAR *)stPoolConfig.PartitionName, "anonymous");

    if (PoolName) {
        strncpy((AX_CHAR *)stPoolConfig.PoolName, PoolName, AX_MAX_POOL_NAME_LEN - 1);
    }

    return AX_POOL_CreatePool(&stPoolConfig);
}

AX_VOID AX_OPAL_HAL_SYS_DestroyPool(AX_POOL nPoolId) {
    if (nPoolId != AX_INVALID_POOLID) {
        AX_POOL_DestroyPool(nPoolId);
    }
}


AX_U32 add_plan(AX_POOL_FLOORPLAN_T *pPoolFloorPlan, AX_U32 nCfgCnt, AX_POOL_CONFIG_T *pPoolConfig) {
    AX_U32 i, done = 0;
    AX_POOL_CONFIG_T *pPC;

    for (i = 0; i < nCfgCnt; i++) {
        pPC = &pPoolFloorPlan->CommPool[i];
        if (pPC->BlkSize == pPoolConfig->BlkSize) {
            pPC->BlkCnt += pPoolConfig->BlkCnt;
            done = 1;
        }
    }

    if (!done) {
        pPoolFloorPlan->CommPool[i] = *pPoolConfig;
        nCfgCnt += 1;
    }

    return nCfgCnt;
}

AX_S32 calc_pool(AX_OPAL_HAL_POOL_CFG_T *pPoolCfg, AX_U32 nPoolCnt, AX_POOL_FLOORPLAN_T *pPoolFloorPlan) {
    AX_S32 i, nCfgCnt = 0;
    AX_FRAME_COMPRESS_INFO_T tCompressInfo = {0};
    AX_POOL_CONFIG_T tPoolConfig = {
        .MetaSize  = 4 * 1024,
        .CacheMode = AX_POOL_CACHE_MODE_NONCACHE,
        .PartitionName = "anonymous"
    };

    for (i = 0; i < nPoolCnt; i++) {
        tCompressInfo.enCompressMode = pPoolCfg->enCompressMode;
        tCompressInfo.u32CompressLevel = pPoolCfg->u32CompressLevel;
        tPoolConfig.BlkCnt  = pPoolCfg->nBlkCnt;

        AX_S32 nMaxHeight = 0;
        AX_S32 nWidthStride = 0;
        nWidthStride = ALIGN_UP(pPoolCfg->nWidth, AX_YUV_FBC_STRIDE_ALIGN_VAL);
        if (AX_COMPRESS_MODE_NONE != tCompressInfo.enCompressMode) {
            nMaxHeight = ALIGN_UP(pPoolCfg->nHeight, AX_YUV_FBC_STRIDE_ALIGN_VAL);
        } else {
            nMaxHeight = ALIGN_UP(pPoolCfg->nHeight, AX_YUV_NONE_FBC_STRIDE_ALIGN_VAL);
        }
        AX_S32 nBlkSize = AX_VIN_GetImgBufferSize(pPoolCfg->nHeight, nWidthStride, pPoolCfg->nFmt, &tCompressInfo, 2);
        AX_S32 nBlkSizeRotate90 = AX_VIN_GetImgBufferSize(nWidthStride, nMaxHeight, AX_FORMAT_YUV420_SEMIPLANAR, &tCompressInfo, 2);
        tPoolConfig.BlkSize = AX_MAX(nBlkSize, nBlkSizeRotate90);

        nCfgCnt = add_plan(pPoolFloorPlan, nCfgCnt, &tPoolConfig);
        pPoolCfg += 1;
    }

    return 0;
}

AX_S32 cvtOpalMod2Mod(AX_OPAL_UNIT_TYPE_E eModType) {
    AX_MOD_ID_E retModId = AX_ID_MIN;
    switch(eModType) {
        case AX_OPAL_ELE_CAM:
            retModId = AX_ID_VIN;
            break;
        case AX_OPAL_ELE_IVPS:
            retModId = AX_ID_IVPS;
            break;
        case AX_OPAL_ELE_VENC:
            retModId = AX_ID_VENC;
            break;
        default:break;
    }
    return retModId;
}

