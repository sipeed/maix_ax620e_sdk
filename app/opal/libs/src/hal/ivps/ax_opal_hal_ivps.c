/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_ivps.h"
#include "ax_opal_hal_osd.h"

#include "ax_opal_utils.h"
#include "ax_opal_log.h"
#define LOG_TAG  ("HAL_IVPS")

#define AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL   (16)
#define AX_IVPS_FBC_STRIDE_ALIGN_VAL        (128)

#define AX_VIDEO_OSD_TTF_PATH "./res/GB2312.ttf"

extern AX_VOID update_compress(AX_FRAME_COMPRESS_INFO_T *pCompressInfo, AX_U16 nChnWidth);
extern AX_S32 malloc_jpeg(AX_JPEG_ENCODE_ONCE_PARAMS_T* pStJpegEncodeOnceParam, AX_U32 u32Width, AX_U32 u32Height);


AX_S32 AX_OPAL_HAL_IVPS_Init(AX_VOID) {
    AX_S32 nRet = 0;

    nRet = AX_IVPS_Init();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_Init failed, ret=0x%x", nRet);
        return -1;
    }

    if (!InitOSDHandler(AX_VIDEO_OSD_TTF_PATH)) {
        LOG_M_E(LOG_TAG, "InitOSDHandler failed, ttf: %s.", AX_VIDEO_OSD_TTF_PATH);
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_IVPS_Deinit(AX_VOID) {
    AX_S32 nRet = 0;

	nRet = AX_IVPS_Deinit();
    if (0 != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_Deinit failed, ret=0x%x", nRet);
        return -1;
    }

    DeinitOSDHandler();
	return 0;
}

AX_S32 AX_OPAL_HAL_IVPS_Open(AX_S32 nGrpId, AX_OPAL_IVPS_ATTR_T *ptIvpsAttr) {

 	AX_S32 nRet = 0;
	if (ptIvpsAttr == NULL) {
		return -1;
	}

	AX_IVPS_GRP_ATTR_T tGrpAttr;
	memset(&tGrpAttr, 0, sizeof(AX_IVPS_GRP_ATTR_T));
    AX_IVPS_PIPELINE_ATTR_T tPipelineAttr;
	memset(&tPipelineAttr, 0, sizeof(AX_IVPS_PIPELINE_ATTR_T));

	/* config AX_IVPS_GRP_ATTR_T */
    tGrpAttr.nInFifoDepth = ptIvpsAttr->tGrpAttr.nFifoDepth;
	tGrpAttr.ePipeline = AX_IVPS_PIPELINE_DEFAULT;

	/* Config pipeline attr */
    tPipelineAttr.nOutChnNum = ptIvpsAttr->nGrpChnCnt;
    tPipelineAttr.nInDebugFifoDepth = 0;
    for (AX_U8 i = 0; i < ptIvpsAttr->nGrpChnCnt; ++i) {
        tPipelineAttr.nOutFifoDepth[i] = ptIvpsAttr->arrChnAttr[i].nFifoDepth;
    }

    /* Config group filter 0 */
    tPipelineAttr.tFilter[0][0].eEngine = ptIvpsAttr->tGrpAttr.eEngineType0;
    if (tPipelineAttr.tFilter[0][0].eEngine != AX_IVPS_ENGINE_BUTT) {
        tPipelineAttr.tFilter[0][0].bEngage = AX_TRUE;
        tPipelineAttr.tFilter[0][0].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        tPipelineAttr.tFilter[0][0].nDstPicWidth = ptIvpsAttr->tGrpAttr.tResolution.nWidth;
        tPipelineAttr.tFilter[0][0].nDstPicHeight = ptIvpsAttr->tGrpAttr.tResolution.nHeight;
        tPipelineAttr.tFilter[0][0].nDstPicStride = ALIGN_UP(ptIvpsAttr->tGrpAttr.tResolution.nWidth, AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL);
        tPipelineAttr.tFilter[0][0].tFRC.fSrcFrameRate = ptIvpsAttr->tGrpAttr.tFramerate.fSrc;
        tPipelineAttr.tFilter[0][0].tFRC.fDstFrameRate = ptIvpsAttr->tGrpAttr.tFramerate.fDst;
        tPipelineAttr.tFilter[0][0].tCompressInfo.enCompressMode = ptIvpsAttr->tGrpAttr.tCompress.enCompressMode;
        tPipelineAttr.tFilter[0][0].tCompressInfo.u32CompressLevel = ptIvpsAttr->tGrpAttr.tCompress.u32CompressLevel;
        if (AX_IVPS_ENGINE_GDC == tPipelineAttr.tFilter[0][0].eEngine) {
            tPipelineAttr.tFilter[0][0].tGdcCfg.eDewarpType = AX_IVPS_DEWARP_BYPASS;
        } else if (AX_IVPS_ENGINE_TDP == tPipelineAttr.tFilter[0][0].eEngine) {
            tPipelineAttr.tFilter[0][0].bInplace = ptIvpsAttr->tGrpAttr.bInplace0;
        }
    }

    /* Config group filter 1 */
    tPipelineAttr.tFilter[0][1].eEngine = ptIvpsAttr->tGrpAttr.eEngineType1;
    if (tPipelineAttr.tFilter[0][1].eEngine != AX_IVPS_ENGINE_BUTT) {
        tPipelineAttr.tFilter[0][1].bEngage = AX_TRUE;
        tPipelineAttr.tFilter[0][1].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
		/* Group filter 1 has not ability of resizing, just use group filter0's out resolution */
        tPipelineAttr.tFilter[0][1].nDstPicWidth = ptIvpsAttr->tGrpAttr.tResolution.nWidth;
        tPipelineAttr.tFilter[0][1].nDstPicHeight = ptIvpsAttr->tGrpAttr.tResolution.nHeight;
        tPipelineAttr.tFilter[0][1].nDstPicStride = ALIGN_UP(ptIvpsAttr->tGrpAttr.tResolution.nWidth, AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL); //ALIGN_UP(tPipelineAttr.tFilter[0][0].nDstPicWidth, AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL);
        tPipelineAttr.tFilter[0][1].tCompressInfo.enCompressMode = ptIvpsAttr->tGrpAttr.tCompress.enCompressMode;
        tPipelineAttr.tFilter[0][1].tCompressInfo.u32CompressLevel = ptIvpsAttr->tGrpAttr.tCompress.u32CompressLevel;
        if (AX_IVPS_ENGINE_VPP == tPipelineAttr.tFilter[0][1].eEngine) {
            tPipelineAttr.tFilter[0][1].bInplace = ptIvpsAttr->tGrpAttr.bInplace1;
        }

        LOG_M_D(LOG_TAG, "[%d] Grp filter 0x01: engine:%d, w:%d, h:%d, s:%d, frameRate[%f,%f]", nGrpId,
              tPipelineAttr.tFilter[0][1].eEngine,
              tPipelineAttr.tFilter[0][1].nDstPicWidth, tPipelineAttr.tFilter[0][1].nDstPicHeight,
              tPipelineAttr.tFilter[0][1].nDstPicStride, tPipelineAttr.tFilter[0][0].tFRC.fSrcFrameRate,
              tPipelineAttr.tFilter[0][0].tFRC.fDstFrameRate);
    }

    /* Config channel filter 0 */
    for (AX_U8 i = 0; i < ptIvpsAttr->nGrpChnCnt; i++) {
        if (ptIvpsAttr->arrChnAttr[i].eEngineType0 != AX_IVPS_ENGINE_BUTT) {
            AX_U8 nChnFilter = i + 1;
            tPipelineAttr.tFilter[nChnFilter][0].bEngage = AX_TRUE;
            tPipelineAttr.tFilter[nChnFilter][0].eEngine = ptIvpsAttr->arrChnAttr[i].eEngineType0;
            tPipelineAttr.tFilter[nChnFilter][0].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;

            if (tPipelineAttr.tFilter[nChnFilter][0].eEngine == AX_IVPS_ENGINE_TDP) {
                tPipelineAttr.tFilter[nChnFilter][0].bInplace = ptIvpsAttr->arrChnAttr[i].bInplace0;
            } else if (tPipelineAttr.tFilter[nChnFilter][0].eEngine == AX_IVPS_ENGINE_VPP) {
                tPipelineAttr.tFilter[nChnFilter][0].bInplace = ptIvpsAttr->arrChnAttr[i].bInplace0;
            } else if (tPipelineAttr.tFilter[nChnFilter][0].eEngine == AX_IVPS_ENGINE_SCL) {
                tPipelineAttr.tFilter[nChnFilter][0].eSclType = ptIvpsAttr->arrChnAttr[i].eSclType;
            }
            tPipelineAttr.tFilter[nChnFilter][0].nDstPicWidth = ptIvpsAttr->arrChnAttr[i].tResolution.nWidth;
            tPipelineAttr.tFilter[nChnFilter][0].nDstPicHeight = ptIvpsAttr->arrChnAttr[i].tResolution.nHeight;
            tPipelineAttr.tFilter[nChnFilter][0].tFRC.fSrcFrameRate = ptIvpsAttr->arrChnAttr[i].tFramerate.fSrc;
            tPipelineAttr.tFilter[nChnFilter][0].tFRC.fDstFrameRate = ptIvpsAttr->arrChnAttr[i].tFramerate.fDst;
            tPipelineAttr.tFilter[nChnFilter][0].tCompressInfo.enCompressMode = ptIvpsAttr->arrChnAttr[i].tCompress.enCompressMode;
            tPipelineAttr.tFilter[nChnFilter][0].tCompressInfo.u32CompressLevel = ptIvpsAttr->arrChnAttr[i].tCompress.u32CompressLevel;
            update_compress(&(tPipelineAttr.tFilter[nChnFilter][0].tCompressInfo), tPipelineAttr.tFilter[nChnFilter][0].nDstPicWidth);
            AX_U32 nStride = tPipelineAttr.tFilter[nChnFilter][0].tCompressInfo.enCompressMode > AX_COMPRESS_MODE_NONE ?
																							AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL;
            tPipelineAttr.tFilter[nChnFilter][0].nDstPicStride = ALIGN_UP(tPipelineAttr.tFilter[nChnFilter][0].nDstPicWidth, nStride);
        }
    }

    /* Config channel filter 1 */
    for (AX_U8 i = 0; i < ptIvpsAttr->nGrpChnCnt; i++) {
        if (ptIvpsAttr->arrChnAttr[i].eEngineType1 != AX_IVPS_ENGINE_BUTT) {
            AX_U8 nChnFilter = i + 1;
            tPipelineAttr.tFilter[nChnFilter][1].bEngage = AX_TRUE;
            tPipelineAttr.tFilter[nChnFilter][1].eEngine = ptIvpsAttr->arrChnAttr[i].eEngineType1;
            tPipelineAttr.tFilter[nChnFilter][1].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
            tPipelineAttr.tFilter[nChnFilter][1].nDstPicWidth = ptIvpsAttr->arrChnAttr[i].tResolution.nWidth;
            tPipelineAttr.tFilter[nChnFilter][1].nDstPicHeight = ptIvpsAttr->arrChnAttr[i].tResolution.nHeight;
            if (!tPipelineAttr.tFilter[nChnFilter][0].bEngage) {
                /* Make sure only one frameCtrl for channel filter */
                tPipelineAttr.tFilter[nChnFilter][1].tFRC.fSrcFrameRate = ptIvpsAttr->arrChnAttr[i].tFramerate.fSrc;
                tPipelineAttr.tFilter[nChnFilter][1].tFRC.fDstFrameRate = ptIvpsAttr->arrChnAttr[i].tFramerate.fDst;
            }

            tPipelineAttr.tFilter[nChnFilter][1].tCompressInfo.enCompressMode = ptIvpsAttr->arrChnAttr[i].tCompress.enCompressMode;
            tPipelineAttr.tFilter[nChnFilter][1].tCompressInfo.u32CompressLevel = ptIvpsAttr->arrChnAttr[i].tCompress.u32CompressLevel;
            /* Update compress info */ // TODO
            update_compress(&(tPipelineAttr.tFilter[nChnFilter][1].tCompressInfo), tPipelineAttr.tFilter[nChnFilter][1].nDstPicWidth);
            AX_U32 nStride =
                tPipelineAttr.tFilter[nChnFilter][1].tCompressInfo.enCompressMode > AX_COMPRESS_MODE_NONE ?
														AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL;
            tPipelineAttr.tFilter[nChnFilter][1].nDstPicStride = ALIGN_UP(tPipelineAttr.tFilter[nChnFilter][1].nDstPicWidth, nStride);

            if (tPipelineAttr.tFilter[nChnFilter][1].eEngine == AX_IVPS_ENGINE_TDP) {
                tPipelineAttr.tFilter[nChnFilter][1].bInplace = ptIvpsAttr->arrChnAttr[i].bInplace1;
                tPipelineAttr.tFilter[nChnFilter][1].tTdpCfg.bVoOsd = ptIvpsAttr->arrChnAttr[i].bVoOsd;
            } else if (tPipelineAttr.tFilter[nChnFilter][1].eEngine == AX_IVPS_ENGINE_VPP) {
                tPipelineAttr.tFilter[nChnFilter][1].bInplace = ptIvpsAttr->arrChnAttr[i].bInplace1;
            }
        }
    }

    nRet = AX_IVPS_CreateGrp(nGrpId, &tGrpAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_CreateGrp(Grp: %d) failed, ret=0x%x", nGrpId, nRet);
        return nRet;
    }

    nRet = AX_IVPS_SetPipelineAttr(nGrpId, &tPipelineAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d) failed, ret=0x%x", nGrpId, nRet);
        return nRet;
    }

    for (AX_U8 iChn = 0; iChn < tPipelineAttr.nOutChnNum; ++iChn) {
        nRet = AX_IVPS_EnableChn(nGrpId, iChn);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVPS_EnableChn(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, iChn, nRet);
            return AX_FALSE;
        }
        ptIvpsAttr->arrChnAttr[iChn].bEnable = AX_TRUE;
    }

	nRet = AX_IVPS_StartGrp(nGrpId);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_StartGrp(Grp: %d) failed, ret=0x%x", nGrpId, nRet);
        return nRet;
    }

	return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_Close(AX_S32 nGrpId) {
    AX_S32 nRet = AX_SUCCESS;
    AX_IVPS_PIPELINE_ATTR_T tPipelineAttr;
	memset(&tPipelineAttr, 0, sizeof(AX_IVPS_PIPELINE_ATTR_T));

    nRet = AX_IVPS_StopGrp(nGrpId);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_StopGrp(Grp: %d) failed, ret=0x%x", nGrpId, nRet);
		return nRet;
    }

    nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d) failed, ret=0x%x", nGrpId, nRet);
        return nRet;
    }

    for (AX_U8 iChn = 0; iChn < tPipelineAttr.nOutChnNum; ++iChn) {
        nRet = AX_IVPS_DisableChn(nGrpId, iChn);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVPS_DisableChn(Grp: %d, Channel: %d) failed, ret=0x%x", nGrpId, iChn, nRet);
        }
    }

    nRet = AX_IVPS_DestoryGrp(nGrpId);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_DestoryGrp(Grp: %d) failed, ret=0x%x", nGrpId, nRet);
        return nRet;
    }

	return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_Start(AX_S32 nGrpId, AX_OPAL_IVPS_ATTR_T *ptIvpsAttr) {
    AX_S32 nRet = AX_SUCCESS;

    // AX_IVPS_PIPELINE_ATTR_T tPipelineAttr;
    // memset(&tPipelineAttr, 0x0, sizeof(AX_IVPS_PIPELINE_ATTR_T));
    // nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
    // if (AX_SUCCESS != nRet) {
    //      LOG_M_E(LOG_TAG, "AX_IVPS_GetPipelineAttr(Grp: %d) failed, ret=0x%x", nGrpId, nRet);
    //      return nRet;
    // }

    // for (AX_U8 iChn = 0; iChn < tPipelineAttr.nOutChnNum; ++iChn) {
    //     nRet = AX_IVPS_EnableChn(nGrpId, iChn);
    //     if (AX_SUCCESS != nRet) {
    //         LOG_M_E(LOG_TAG, "AX_IVPS_EnableChn(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, iChn, nRet);
    //         return AX_FALSE;
    //     }
    //     // ptIvpsAttr->arrChnAttr[iChn].bEnable = AX_TRUE;
    // }

	// nRet = AX_IVPS_StartGrp(nGrpId);
    // if (AX_SUCCESS != nRet) {
    //     LOG_M_E(LOG_TAG, "AX_IVPS_StartGrp(Grp: %d) failed, ret=0x%x", nGrpId, nRet);
    //     return nRet;
    // }

	return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_Stop(AX_S32 nGrpId) {
	return 0;
}

AX_S32 AX_OPAL_HAL_IVPS_EnableChn(AX_S32 nGrpId, AX_S32 nChnId) {
	AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

	if (nChnId >= AX_OPAL_MAX_CHN_CNT){
	     LOG_M_E(LOG_TAG, "nChn [%d] is invalid", nChnId);
	     return -1;
	}

    nRet = AX_IVPS_EnableChn(nGrpId, nChnId);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_EnableChn(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        return nRet;
    }

    LOG_M_D(LOG_TAG, "---");
	return nRet;
}


AX_S32 AX_OPAL_HAL_IVPS_DisableChn(AX_S32 nGrpId, AX_S32 nChnId) {
	AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

	if (nChnId >= AX_OPAL_MAX_CHN_CNT){
	     LOG_M_E(LOG_TAG, "nChn [%d] is invalid", nChnId);
	     return -1;
	}

    nRet = AX_IVPS_DisableChn(nGrpId, nChnId);
    if (AX_SUCCESS != nRet) {
         LOG_M_E(LOG_TAG, "AX_IVPS_DisableChn(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
         return nRet;
    }
    LOG_M_D(LOG_TAG, "---");
	return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_SetResolution(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotation, AX_S32 nWidth, AX_S32 nHeight) {
	AX_S32 nRet = 0;
    if (nChnId >= AX_OPAL_MAX_CHN_CNT){
	     LOG_M_E(LOG_TAG, "nChn [%d] is invalid", nChnId);
	     return -1;
	}

    AX_U32 nStrideAlign = 0;

    AX_IVPS_PIPELINE_ATTR_T tPipelineAttr;
    memset(&tPipelineAttr, 0x0, sizeof(AX_IVPS_PIPELINE_ATTR_T));
    nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
    if (AX_SUCCESS != nRet) {
         LOG_M_E(LOG_TAG, "AX_IVPS_GetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
         return nRet;
    }

    /* set resolution */
    AX_S32 nW = nWidth;
    AX_S32 nH = nHeight;
    if (eRotation == AX_OPAL_SNS_ROTATION_90 || eRotation == AX_OPAL_SNS_ROTATION_270) {
        AX_SWAP(nW, nH);
    }

    update_compress(&tPipelineAttr.tFilter[nChnId + 1][0].tCompressInfo, nW);
    tPipelineAttr.tFilter[nChnId + 1][1].tCompressInfo = tPipelineAttr.tFilter[nChnId + 1][0].tCompressInfo;
    AX_BOOL bFBC = tPipelineAttr.tFilter[nChnId + 1][0].tCompressInfo.enCompressMode > AX_COMPRESS_MODE_NONE ? AX_TRUE : AX_FALSE;

    /* update Grp */
    if (0 == nChnId) {
        /* Update Grp filter0 resolution */
        if (tPipelineAttr.tFilter[0][0].bEngage) {
            nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL;
            tPipelineAttr.tFilter[0][0].nDstPicWidth = nW;
            tPipelineAttr.tFilter[0][0].nDstPicHeight = nH;
            tPipelineAttr.tFilter[0][0].nDstPicStride = ALIGN_UP(nW, nStrideAlign);
        }

        /* Update Grp filter1 resolution */
        if (tPipelineAttr.tFilter[0][1].bEngage) {
            nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL;
            tPipelineAttr.tFilter[0][1].nDstPicWidth = nW;
            tPipelineAttr.tFilter[0][1].nDstPicHeight = nH;
            tPipelineAttr.tFilter[0][1].nDstPicStride = ALIGN_UP(nW, nStrideAlign);
        }
    }

    /* Update resolution of channel filter 0 */
    if (tPipelineAttr.tFilter[nChnId + 1][0].bEngage) {
        nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL;
        tPipelineAttr.tFilter[nChnId + 1][0].nDstPicWidth = nW;
        tPipelineAttr.tFilter[nChnId + 1][0].nDstPicHeight = nH;
        tPipelineAttr.tFilter[nChnId + 1][0].nDstPicStride = ALIGN_UP(nW, nStrideAlign);
    }

    /* Update resolution of channel filter 1 */
    if (tPipelineAttr.tFilter[nChnId + 1][1].bEngage) {
        nStrideAlign = bFBC ? AX_IVPS_FBC_STRIDE_ALIGN_VAL : AX_IVPS_NONE_FBC_STRIDE_ALIGN_VAL;
        tPipelineAttr.tFilter[nChnId + 1][1].nDstPicWidth = nW;
        tPipelineAttr.tFilter[nChnId + 1][1].nDstPicHeight = nH;
        tPipelineAttr.tFilter[nChnId + 1][1].nDstPicStride = ALIGN_UP(nW, nStrideAlign);
    }

    nRet = AX_IVPS_SetPipelineAttr(nGrpId, &tPipelineAttr);
    if (AX_SUCCESS != nRet) {
         LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
         return nRet;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_SetRotation(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_SNS_ROTATION_E eRotationType) {
    AX_S32 nRet = 0;
    LOG_M_D(LOG_TAG, "+++");

    AX_IVPS_CHN_ATTR_T tChnAttr;
    memset(&tChnAttr, 0x0, sizeof(AX_IVPS_CHN_ATTR_T));
    nRet = AX_IVPS_GetChnAttr(nGrpId, nChnId, 0, &tChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_GetChnAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        return nRet;
    }

    nRet = AX_OPAL_HAL_IVPS_SetResolution(nGrpId, nChnId, eRotationType, tChnAttr.nDstPicWidth, tChnAttr.nDstPicHeight);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_GetResolution(AX_S32 nGrpId, AX_S32 nChnId, AX_S32* nWidth, AX_S32* nHeight) {
    AX_S32 nRet = 0;
    AX_IVPS_CHN_ATTR_T tChnAttr;
    memset(&tChnAttr, 0x0, sizeof(AX_IVPS_CHN_ATTR_T));
    nRet = AX_IVPS_GetChnAttr(nGrpId, nChnId, 0, &tChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_GetChnAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        return nRet;
    }

    *nWidth = tChnAttr.nDstPicWidth;
    *nHeight = tChnAttr.nDstPicHeight;

    return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_SetFps(AX_S32 nGrpId, AX_S32 nChnId, AX_F32 fSrcFps, AX_F32 fDstFps) {

    AX_S32 nRet = 0;
    AX_U8 nChnFilter = 0;
    AX_IVPS_CHN_ATTR_T tChnAttr;
    memset(&tChnAttr, 0x0, sizeof(AX_IVPS_CHN_ATTR_T));
    nRet = AX_IVPS_GetChnAttr(nGrpId, nChnId, nChnFilter, &tChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_GetChnAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        return nRet;
    }

    if (fSrcFps != -1) {
        tChnAttr.tFRC.fSrcFrameRate = fSrcFps;
    }

    if (fDstFps != -1) {
        tChnAttr.tFRC.fDstFrameRate = fDstFps;
    }

    nRet = AX_IVPS_SetChnAttr(nGrpId, nChnId, nChnFilter, &tChnAttr);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_SetChnAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        return nRet;
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_SnapShot(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_VIDEO_SNAPSHOT_T* ptSnapShot) {

    AX_S32 nRet = -1;
    LOG_M_D(LOG_TAG, "+++");

    AX_VIDEO_FRAME_T tVideoFrame;
    AX_S32 nTimeOutMS = 100;
    AX_JPEG_ENCODE_ONCE_PARAMS_T stJpegEncodeOnceParam;
    AX_BOOL bNeedResetDepth = AX_FALSE;
    do {
        AX_IVPS_PIPELINE_ATTR_T tPipelineAttr = {0};
        nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
        if (AX_SUCCESS == nRet) {
            if (0 == tPipelineAttr.nOutFifoDepth[nChnId]) {
                tPipelineAttr.nOutFifoDepth[nChnId] = 1;
                nRet = AX_IVPS_SetPipelineAttr(nGrpId, &tPipelineAttr);
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
                    break;
                }
                bNeedResetDepth = AX_TRUE;
            }

        } else {
            LOG_M_E(LOG_TAG, "AX_IVPS_GetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
            break;
        }

        AX_S32 nRetryCnt = 3;
        do {
            nRet = AX_IVPS_GetChnFrame(nGrpId, nChnId, &tVideoFrame, nTimeOutMS);
            if (AX_SUCCESS != nRet) {
                continue;
            }
            else {
                break;
            }
            nRetryCnt--;
        } while(nRetryCnt > 0);

        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVPS_GetChnFrame(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        }

        if (bNeedResetDepth) {
            nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
            if (AX_SUCCESS == nRet) {
                tPipelineAttr.nOutFifoDepth[nChnId] = 0;
                nRet = AX_IVPS_SetPipelineAttr(nGrpId, &tPipelineAttr);
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
                    break;
                }
            } else {
                LOG_M_E(LOG_TAG, "AX_IVPS_GetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
                break;
            }
        }


        memset(&stJpegEncodeOnceParam, 0, sizeof(stJpegEncodeOnceParam));
        stJpegEncodeOnceParam.stJpegParam.u32Qfactor = ptSnapShot->nQpLevel;
        for (AX_U8 i = 0; i < 3; ++i) {
            stJpegEncodeOnceParam.u64PhyAddr[i] = tVideoFrame.u64PhyAddr[i];
            stJpegEncodeOnceParam.u32PicStride[i] = tVideoFrame.u32PicStride[i];
        }
        stJpegEncodeOnceParam.u32Width = tVideoFrame.u32Width;
        stJpegEncodeOnceParam.u32Height = tVideoFrame.u32Height;
        stJpegEncodeOnceParam.enImgFormat = tVideoFrame.enImgFormat;
        stJpegEncodeOnceParam.stCompressInfo = tVideoFrame.stCompressInfo;
        nRet = malloc_jpeg(&stJpegEncodeOnceParam, tVideoFrame.u32Width, tVideoFrame.u32Height);
        if (nRet == AX_SUCCESS) {
            nRet = AX_VENC_JpegEncodeOneFrame(&stJpegEncodeOnceParam);
            if (nRet != AX_SUCCESS) {
                LOG_M_E(LOG_TAG, "AX_VENC_JpegEncodeOneFrame(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
                break;
            }
        }
        if (nRet == AX_SUCCESS && ptSnapShot->nImageBufSize >= stJpegEncodeOnceParam.u32Len && NULL != stJpegEncodeOnceParam.pu8Addr) {
            memcpy(ptSnapShot->pImageBuf, stJpegEncodeOnceParam.pu8Addr, stJpegEncodeOnceParam.u32Len);
            LOG_M_N(LOG_TAG, "AX_OPAL_HAL_IVPS_SnapShot(Grp: %d, Chn: %d) success", nGrpId, nChnId);
        } else {
            LOG_M_E(LOG_TAG, "The allocated buffer size is insufficien (Grp: %d, Chn: %d), ret=0x%x", nGrpId, nChnId, nRet);
            break;
        }
        *(ptSnapShot->pActSize) = stJpegEncodeOnceParam.u32Len;

        nRet = AX_SUCCESS;
    } while (0);

    if (0 != stJpegEncodeOnceParam.ulPhyAddr && NULL != stJpegEncodeOnceParam.pu8Addr) {
        AX_SYS_MemFree(stJpegEncodeOnceParam.ulPhyAddr, stJpegEncodeOnceParam.pu8Addr);
    }

    AX_IVPS_ReleaseChnFrame(nGrpId, nChnId, &tVideoFrame);

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

AX_S32 AX_OPAL_HAL_IVPS_CaptureFrame(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_VIDEO_CAPTUREFRAME_T* ptCaptureFrame) {

    AX_S32 nRet = -1;
    LOG_M_D(LOG_TAG, "+++");

    AX_VIDEO_FRAME_T tVideoFrame;
    AX_S32 nTimeOutMS = 100;

    AX_U64 phyBuff = 0;
    AX_VOID* virBuff = NULL;
    AX_U32 frameSize = ptCaptureFrame->nFrameBufSize;
    if (ptCaptureFrame->nFrameBufSize != ptCaptureFrame->nWidth*ptCaptureFrame->nHeight*3/2) {
        LOG_M_E(LOG_TAG, "The parameter nFrameBufSize is invalid and does not match the width and height (%d, %d).", ptCaptureFrame->nWidth, ptCaptureFrame->nHeight);
        return AX_ERR_OPAL_ILLEGAL_PARAM;
    }

    AX_BOOL bNeedResetDepth = AX_FALSE;
    do {
        /* get ivps channel frame */
        AX_IVPS_PIPELINE_ATTR_T tPipelineAttr = {0};
        nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
        if (AX_SUCCESS == nRet) {
            if (0 == tPipelineAttr.nOutFifoDepth[nChnId]) {
                tPipelineAttr.nOutFifoDepth[nChnId] = 1;
                nRet = AX_IVPS_SetPipelineAttr(nGrpId, &tPipelineAttr);
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
                    break;
                }
                bNeedResetDepth = AX_TRUE;
            }

        } else {
            LOG_M_E(LOG_TAG, "AX_IVPS_GetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
            break;
        }

        AX_S32 nRetryCnt = 3;
        do {
            nRet = AX_IVPS_GetChnFrame(nGrpId, nChnId, &tVideoFrame, nTimeOutMS);
            if (AX_SUCCESS != nRet) {
                continue;
            }
            else {
                break;
            }
            nRetryCnt--;
        } while(nRetryCnt > 0);

        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVPS_GetChnFrame(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
        }

        if (bNeedResetDepth) {
            nRet = AX_IVPS_GetPipelineAttr(nGrpId, &tPipelineAttr);
            if (AX_SUCCESS == nRet) {
                tPipelineAttr.nOutFifoDepth[nChnId] = 0;
                nRet = AX_IVPS_SetPipelineAttr(nGrpId, &tPipelineAttr);
                if (AX_SUCCESS != nRet) {
                    LOG_M_E(LOG_TAG, "AX_IVPS_SetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
                    break;
                }
            } else {
                LOG_M_E(LOG_TAG, "AX_IVPS_GetPipelineAttr(Grp: %d, Chn: %d) failed, ret=0x%x", nGrpId, nChnId, nRet);
                break;
            }
        }

        nRet = AX_SYS_MemAlloc(&phyBuff, &virBuff, frameSize, 0, (AX_S8*)"CAP_FRM");
        if (nRet != AX_SUCCESS) {
            LOG_M_E(LOG_TAG, "AX_SYS_MemAlloc failed, err=0x%x", nRet);
            break;
        }

        AX_VIDEO_FRAME_T tDstFrame;
        memset(&tDstFrame, 0x00, sizeof(tDstFrame));
        tDstFrame.u32Width = ptCaptureFrame->nWidth;
        tDstFrame.u32Height = ptCaptureFrame->nHeight;
        tDstFrame.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        tDstFrame.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
        tDstFrame.u32PicStride[0] = tDstFrame.u32Width;
        tDstFrame.u32PicStride[1] = tDstFrame.u32Width;
        tDstFrame.u32PicStride[2] = tDstFrame.u32Width;
        tDstFrame.u64PhyAddr[0] = phyBuff;
        tDstFrame.u64VirAddr[0] = (AX_ULONG)virBuff;
        tDstFrame.u32FrameSize = frameSize;

        AX_IVPS_CROP_RESIZE_ATTR_T tCropResizeAttr;
        memset(&tCropResizeAttr, 0, sizeof(tCropResizeAttr));
        tCropResizeAttr.tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_STRETCH;
        nRet = AX_IVPS_CropResizeVpp(&tVideoFrame, &tDstFrame, &tCropResizeAttr);
        if (nRet != AX_SUCCESS) {
            LOG_M_E(LOG_TAG, "AX_IVPS_CropResizeVpp failed, err=0x%x", nRet);
            break;
        }
        memcpy(ptCaptureFrame->pFrameBuf, virBuff, frameSize);
    } while (0);

    if (0 != phyBuff && NULL != virBuff) {
        AX_SYS_MemFree(phyBuff, virBuff);
    }
    AX_IVPS_ReleaseChnFrame(nGrpId, nChnId, &tVideoFrame);

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

IVPS_RGN_HANDLE AXOP_HAL_IVPS_CreateOsd(AX_S32 nGrpId, AX_S32 nChnId, const AX_OPAL_OSD_ATTR_T* pOsdAttr) {
    AX_S32 s32Ret = AX_SUCCESS;

    IVPS_RGN_HANDLE hdlRgn = AX_IVPS_INVALID_REGION_HANDLE;
    do {
        hdlRgn = AX_OPAL_HAL_OSD_CreateRgn(nGrpId, nChnId, pOsdAttr->eType);
        if (hdlRgn == AX_IVPS_INVALID_REGION_HANDLE) {
            LOG_M_E(LOG_TAG, "AX_OPAL_HAL_OSD_CreateRgn(Grp: %d, Chn: %d) failed.", nGrpId, nChnId);
            break;
        }

    } while (0);

    if (AX_SUCCESS != s32Ret && hdlRgn != AX_IVPS_INVALID_REGION_HANDLE) {
        AX_OPAL_HAL_OSD_DestoryRgn(nGrpId, nChnId, hdlRgn, pOsdAttr->eType);
    }
    return hdlRgn;
}

AX_S32 AXOP_HAL_IVPS_DestroyOsd(AX_S32 nGrpId, AX_S32 nChnId, IVPS_RGN_HANDLE hdlOsd, const AX_OPAL_OSD_ATTR_T* pOsdAttr) {
    AX_S32 s32Ret = AX_SUCCESS;
    s32Ret = AX_OPAL_HAL_OSD_DestoryRgn(nGrpId, nChnId, hdlOsd, pOsdAttr->eType);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(LOG_TAG, "AXOP_HAL_OSD_DestoryRgn(Grp: %d, Chn: %d, Rgn: %d) failed, ret=0x%x", nGrpId, nChnId, hdlOsd, s32Ret);
    }
    return s32Ret;
}

AX_S32 malloc_jpeg(AX_JPEG_ENCODE_ONCE_PARAMS_T* pStJpegEncodeOnceParam, AX_U32 u32Width, AX_U32 u32Height) {
    AX_U64 phyBuff = 0;
    AX_VOID* virBuff = NULL;
    AX_S32 nRet = AX_SUCCESS;
    AX_S8 JPEG_ENCODE_ONCE_NAME[] = "JENC_CAP";
    AX_U32 nMaxOutBufferSize = 1572864; // 1.5 * 1024 * 1024

    AX_U32 maxFrameSize = AX_MAX(nMaxOutBufferSize, u32Width * u32Height * 3 / 16);
    AX_U32 minFrameSize = AX_MIN(nMaxOutBufferSize, u32Width * u32Height * 3 / 16);

    AX_U32 frameSize = maxFrameSize;
    nRet = AX_SYS_MemAlloc(&phyBuff, &virBuff, frameSize, 0, (AX_S8*)JPEG_ENCODE_ONCE_NAME);
    if (AX_SUCCESS != nRet) {
        frameSize = minFrameSize;
        nRet = AX_SYS_MemAlloc(&phyBuff, &virBuff, frameSize, 0, (AX_S8*)JPEG_ENCODE_ONCE_NAME);
    }
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_SYS_MemAlloc failed, size=%d.", frameSize);
        return -1;
    }

    pStJpegEncodeOnceParam->u32OutBufSize = frameSize;
    pStJpegEncodeOnceParam->ulPhyAddr = phyBuff;
    pStJpegEncodeOnceParam->pu8Addr = (AX_U8*)virBuff;

    return AX_SUCCESS;
}

AX_VOID update_compress(AX_FRAME_COMPRESS_INFO_T *pCompressInfo, AX_U16 nChnWidth) {
    if (pCompressInfo->enCompressMode > AX_COMPRESS_MODE_NONE) {
        if (nChnWidth < 512) {
            // || (tSensorCfg.arrPipeAttr[0].arrChannelAttr[0].nWidth >= 3840 && nChnWidth <= g_ivps_nMinFBCWidthFor4K)) {
            pCompressInfo->enCompressMode = 0;
            pCompressInfo->u32CompressLevel = 0;
        }
    }
}