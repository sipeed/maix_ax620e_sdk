/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_pipeline.h"
#include "ax_opal_mal_ppl_parser.h"
#include "ax_opal_mal_utils.h"

#include "ax_opal_mal_element.h"
#include "ax_opal_mal_elecam.h"
#include "ax_opal_mal_elealgo.h"
#include "ax_opal_mal_eleivps.h"
#include "ax_opal_mal_elevenc.h"
#include "ax_opal_mal_elesys.h"
#include "ax_opal_mal_eleaudio.h"

#include "ax_opal_hal_sys.h"
#include "ax_opal_hal_cam.h"
#include "ax_opal_hal_ivps.h"
#include "ax_opal_hal_algo.h"
#include "ax_opal_hal_venc.h"
#include "ax_opal_hal_audio_cap.h"
#include "ax_opal_hal_audio_play.h"

#include "ax_opal_log.h"
#include "ax_opal_utils.h"

#define LOG_TAG ("MALPPL")

AX_OPAL_HAL_POOL_ATTR_T g_stPoolAttr = {
    .stPoolAttr = {
        [0] = {
            .nCommPoolCfgCnt = 3,
            .arrCommPoolCfg =  {
                {-1, -1, AX_FORMAT_YUV420_SEMIPLANAR, 10, AX_COMPRESS_MODE_LOSSY, 4},
                {1280, 720, AX_FORMAT_YUV420_SEMIPLANAR, 6, AX_COMPRESS_MODE_NONE, 0},
                {720, 576, AX_FORMAT_YUV420_SEMIPLANAR,  6, AX_COMPRESS_MODE_LOSSY, 4},
            },
            .nPrivPoolCfgCnt = 1,
            .arrPrivPoolCfg = {
                {-1, -1, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 18, AX_COMPRESS_MODE_LOSSY, 4},
            },
        }
    }
};

static AX_S32 subppl_ele_eventproc(AX_OPAL_MAL_SUBPPL_HANDLE subppl,
                                   AX_OPAL_UNIT_TYPE_E eUnitType,
                                   AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData,
                                   AX_OPAL_SUB_CMD_E eSubCmd) {

    AX_S32 nRet = AX_SUCCESS;
    pPorcessData->eSubCmdType = eSubCmd;
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)subppl;
    AX_OPAL_MAL_ELE_T *pEle = AX_NULL;
    for (AX_S32 iEle = 0; iEle < pSubPipeline->stAttr.nEleCnt; ++iEle) {
        pEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[iEle];
        if (pEle && pEle->stAttr.eType == eUnitType && pEle->vTable && pEle->vTable->event_proc) {
            nRet = pEle->vTable->event_proc((AX_OPAL_MAL_ELE_HANDLE)pEle, pPorcessData);
            if (nRet != AX_SUCCESS) {
                return -1;
            }
        }
    }

    return nRet;
}

static AX_S32 update_rotation(AX_OPAL_MAL_SUBPPL_HANDLE pSubPplHdl, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {

    AX_S32 nRet = AX_SUCCESS;
    // // Step 0. ivps disable
    // nRetTmp = subppl_ele_eventproc(pSubPplHdl, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_PREPROC);
    // if (AX_SUCCESS != nRetTmp) {
    //     break;
    // }
    // Step 1. venc/jenc stop and reset
    nRet = subppl_ele_eventproc(pSubPplHdl, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_PREPROC);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }
    // Step 2. cam update rotation
    nRet = subppl_ele_eventproc(pSubPplHdl, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_ROTATION);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }
    // Step 3. ivps update rotation, update osd
    nRet = subppl_ele_eventproc(pSubPplHdl, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_ROTATION);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }
    // Step 4. venc/jenc update rotation
    nRet = subppl_ele_eventproc(pSubPplHdl, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_ROTATION);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }
    // Step 5. algo(ives) update rotation
    nRet = subppl_ele_eventproc(pSubPplHdl, AX_OPAL_ELE_ALGO, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_ROTATION);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }
    // Step 6. venc/jenc start
    nRet = subppl_ele_eventproc(pSubPplHdl, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_POSTPROC);
    if (AX_SUCCESS != nRet) {
        return nRet;
    }
    // // Step 7. ivps enanle
    // nRetTmp = subppl_ele_eventproc(pSubPplHdl, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_POSTPROC);
    // if (AX_SUCCESS != nRetTmp) {
    //     return nRet;
    // }
    return nRet;
}

AX_OPAL_MAL_PPL_HANDLE AX_OPAL_MAL_PPL_Create(AX_OPAL_PPL_ATTR_T *pPplAttr, const AX_OPAL_ATTR_T *pstAttr) {
    LOG_M_D(LOG_TAG, "+++");

    AX_OPAL_MAL_PPL_HANDLE handle = AX_NULL;

    AX_BOOL bRet = AX_FALSE;
    do {

        /* verify param */
        if (pPplAttr == AX_NULL) {
            LOG_M_E(LOG_TAG, "invalid ppl attr param.");
            break;
        }

        if (pPplAttr->nSubPplCnt <= 0 || pPplAttr->nSubPplCnt > AX_OPAL_MAX_SUBPPL_CNT) {
            LOG_M_E(LOG_TAG, "invalid sub pipeline count %d [1, %d].", pPplAttr->nSubPplCnt, AX_OPAL_MAX_SUBPPL_CNT);
            break;
        }

        /* create pipeline */
        handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_PPL_T));
        if (!handle) {
            LOG_M_E(LOG_TAG, "malloc ppl failed.");
            break;
        }
        AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)handle;
        memset(pPipeline, 0, sizeof(AX_OPAL_MAL_PPL_T));

        /* save opal all attr: vedio and audio ... */
        memcpy(&(pPipeline->stOpalAttr), pstAttr, sizeof(AX_OPAL_ATTR_T));
        memcpy(&(pPipeline->stPplAttr), pPplAttr, sizeof(AX_OPAL_PPL_ATTR_T));

        /* video sub-pipeline: sub-pipeline index mapping index of AX_OPAL_ATTR_T.stVideoAttr */
        /* verify video sub-pipeline count */
        AX_S32 nVideoPplCnt = 0;
        for (AX_S32 iVideo = 0; iVideo < AX_OPAL_SNS_ID_BUTT; ++iVideo) {
            if (pPipeline->stOpalAttr.stVideoAttr[iVideo].bEnable) {
                ++nVideoPplCnt;
            }
        }

        if (nVideoPplCnt != pPplAttr->nSubPplCnt) {
            LOG_M_I(LOG_TAG, "sub pipeline count does not match. pipeline has sub video pipeline %d, opal has %d.", pPplAttr->nSubPplCnt, nVideoPplCnt);
            if (nVideoPplCnt > pPplAttr->nSubPplCnt) {
                LOG_M_E(LOG_TAG, "invalid video pipeline count. subpipeline count=%d, but video attr coubt=%d", pPplAttr->nSubPplCnt, nVideoPplCnt);
                break;
            }
        }

        AX_S32 nSubPplCntInit = 0;
        pPipeline->nSubPplCnt = nVideoPplCnt; // pPplAttr->nSubPplCnt;
        for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++iSubPpl) {
            /* create sub-pipeline */
            pPipeline->arrSubPpl[iSubPpl] = AX_OPAL_MAL_SUBPPL_Create(handle, &pPplAttr->arrSubPplAttr[iSubPpl]);
            if (pPipeline->arrSubPpl[iSubPpl] == AX_NULL) {
                LOG_M_E(LOG_TAG, "create sub pipeline %d failed.", iSubPpl);
                break;
            }
            nSubPplCntInit++;
        }

        if (nSubPplCntInit != pPipeline->nSubPplCnt) {
            break;
        }

        /* audio: pipeline attr built-in*/
        AX_OPAL_SUBPPL_ATTR_T stAudioSubPplAttr = {
            .nId = -1,
            .nGrpCnt = 0,
            .nEleCnt = 1,
            .arrEleAttr = {
                [0] = {
                    .nId = 0,
                    .eType = AX_OPAL_ELE_AUDIO,
                    .nGrpCnt = 0,
                }
            },
            .nLinkCnt = 0,
        };

        pPipeline->audioPpl = AX_OPAL_MAL_SUBPPL_Create(handle, &stAudioSubPplAttr);
        if (pPipeline->audioPpl == AX_NULL) {
            LOG_M_E(LOG_TAG, "create audio pipeline failed.");
            break;
        }

        bRet = AX_TRUE;

    } while (0);

    if (!bRet) {
        if (handle) {
            AX_OPAL_MAL_PPL_Destroy(handle);
        }
        handle = AX_NULL;
    }

    LOG_M_D(LOG_TAG, "---");
    return handle;
}

AX_S32 AX_OPAL_MAL_PPL_Destroy(AX_OPAL_MAL_PPL_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    AX_OPAL_MAL_PPL_T *pPipeline = (AX_OPAL_MAL_PPL_T*)self;
    /* video */
    for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++iSubPpl) {
        AX_OPAL_MAL_SUBPPL_Destroy(pPipeline->arrSubPpl[iSubPpl]);
        pPipeline->arrSubPpl[iSubPpl] = AX_NULL;
    }
    pPipeline->nSubPplCnt = 0;

    /* audio */
    AX_OPAL_MAL_SUBPPL_Destroy(pPipeline->audioPpl);
    pPipeline->audioPpl = AX_NULL;

    AX_OPAL_FREE(self);
    return nRet;
}

AX_OPAL_MAL_SUBPPL_HANDLE AX_OPAL_MAL_SUBPPL_Create(AX_OPAL_MAL_PPL_HANDLE parent, AX_OPAL_SUBPPL_ATTR_T *pSubPplAttr) {
    AX_OPAL_MAL_SUBPPL_HANDLE handle = AX_NULL;

    AX_BOOL bRet = AX_FALSE;
    do {
        /* verify param */
        if (pSubPplAttr == AX_NULL) {
            LOG_M_E(LOG_TAG, "invalid subppl attr param.");
            break;
        }

        if (pSubPplAttr->nEleCnt <= 0 || pSubPplAttr->nEleCnt > AX_OPAL_MAX_ELE_CNT) {
            LOG_M_E(LOG_TAG, "invalid element count %d [1, %d].", pSubPplAttr->nEleCnt, AX_OPAL_MAX_ELE_CNT);
            break;
        }

        handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_SUBPPL_T));
        if (!handle) {
            LOG_M_E(LOG_TAG, "malloc subppl failed.");
            break;
        }

        /* create sub-pipeline */
        AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)handle;
        memset(pSubPipeline, 0, sizeof(AX_OPAL_MAL_SUBPPL_T));
        memcpy(&pSubPipeline->stAttr, pSubPplAttr, sizeof(AX_OPAL_SUBPPL_ATTR_T));
        pSubPipeline->nId = pSubPplAttr->nId;
        pSubPipeline->pParent = parent;
        // pSubPipeline->vTable = &subppl_interface;

        for (AX_S32 iEle = 0; iEle < pSubPplAttr->nEleCnt; ++iEle) {
            /* create element */
            AX_OPAL_ELEMENT_ATTR_T *pEleAttr = &pSubPplAttr->arrEleAttr[iEle];
            switch (pEleAttr->eType) {
                case AX_OPAL_ELE_CAM:
                    pSubPipeline->arrEle[iEle] = AX_OPAL_MAL_ELECAM_Create(handle, pEleAttr);
                    break;
                case AX_OPAL_ELE_IVPS:
                    pSubPipeline->arrEle[iEle] = AX_OPAL_MAL_ELEIVPS_Create(handle, pEleAttr);
                    break;
                case AX_OPAL_ELE_VENC:
                    pSubPipeline->arrEle[iEle] = AX_OPAL_MAL_ELEVENC_Create(handle, pEleAttr);
                    break;
                case AX_OPAL_ELE_ALGO:
                    pSubPipeline->arrEle[iEle] = AX_OPAL_MAL_ELEALGO_Create(handle, pEleAttr);
                    break;
                case AX_OPAL_ELE_AUDIO:
                    pSubPipeline->arrEle[iEle] = AX_OPAL_MAL_ELEAUDIO_Create(handle, pEleAttr);
                    break;
                default:
                    LOG_M_E(LOG_TAG, "invalid element type.");
                    break;
            }
            if (pSubPipeline->arrEle[iEle] == NULL) {
                LOG_M_E(LOG_TAG, "create element failed. ele type=%d, id=%d", pEleAttr->eType, pEleAttr->nId);
                break;
            }
            pSubPipeline->nEleCnt++;
        }
        if (pSubPplAttr->nEleCnt != pSubPipeline->nEleCnt) {
            break;
        }

        bRet = AX_TRUE;
    } while (0);

    if (AX_FALSE == bRet) {
        AX_OPAL_MAL_SUBPPL_Destroy(handle);
        handle = AX_NULL;
    }

    return handle;
}

AX_S32 AX_OPAL_MAL_SUBPPL_Destroy(AX_OPAL_MAL_SUBPPL_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)self;

    for (AX_S32 iEle = 0; iEle < pSubPipeline->stAttr.nEleCnt; ++iEle) {
        AX_OPAL_ELEMENT_ATTR_T *pEleAttr = &pSubPipeline->stAttr.arrEleAttr[iEle];
        switch (pEleAttr->eType) {
            case AX_OPAL_ELE_CAM:
                AX_OPAL_MAL_ELECAM_Destroy(pSubPipeline->arrEle[iEle]);
                break;
            case AX_OPAL_ELE_IVPS:
                AX_OPAL_MAL_ELEIVPS_Destroy(pSubPipeline->arrEle[iEle]);
                break;
            case AX_OPAL_ELE_VENC:
                AX_OPAL_MAL_ELEVENC_Destroy(pSubPipeline->arrEle[iEle]);
                break;
            case AX_OPAL_ELE_ALGO:
                AX_OPAL_MAL_ELEALGO_Destroy(pSubPipeline->arrEle[iEle]);
                break;
            case AX_OPAL_ELE_AUDIO:
                AX_OPAL_MAL_ELEAUDIO_Destroy(pSubPipeline->arrEle[iEle]);
                break;
            default:
                LOG_M_E(LOG_TAG, "invalid element type.");
                break;
        }
    }

    AX_OPAL_FREE(self);
    return nRet;
}

static AX_BOOL IsEnableLowMem(AX_OPAL_MAL_PPL_HANDLE self) {
    AX_OPAL_MAL_PPL_T *pPipeline = (AX_OPAL_MAL_PPL_T*)self;
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = NULL;

    if (IS_AX620Q) {
        AX_S32 nSnsEnableCnt = 0;
        for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++iSubPpl) {
            pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->arrSubPpl[iSubPpl];
            AX_S32 nUniGrpId = pSubPipeline->stAttr.arrGrpAttr[0].nGrpId;
            if (pPipeline->stOpalAttr.stVideoAttr[nUniGrpId].bEnable) {
                ++ nSnsEnableCnt;
            }
        }
        if (nSnsEnableCnt == 1) {
            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

static AX_S32 ppl_init(AX_OPAL_MAL_PPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }
    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)self;

    nRet = AX_OPAL_HAL_SYS_Init();
    if (0 != nRet) {
        return -1;
    }

    nRet = AX_OPAL_HAL_CAM_Init();
    if (0 != nRet) {
        return -1;
    }

    nRet = AX_OPAL_HAL_IVPS_Init();
    if (0 != nRet) {
        return -1;
    }

    nRet = AX_OPAL_HAL_VENC_Init();
    if (0 != nRet) {
        return -1;
    }

    AX_OPAL_HAL_POOL_ATTR_T stPoolAttr;
    memset(&stPoolAttr, 0x0, sizeof(AX_OPAL_HAL_POOL_ATTR_T));
    if (pPipeline->stOpalAttr.szPoolConfigPath[0] != '\0') {
        nRet = AX_OPAL_MAL_POOL_Parse(pPipeline->stOpalAttr.szPoolConfigPath, &stPoolAttr.stPoolAttr[0]);
        if (0 != nRet) {
            return -1;
        }
    } else {
        memcpy(&stPoolAttr.stPoolAttr[0], &g_stPoolAttr.stPoolAttr[0], sizeof(AX_OPAL_POOL_ATTR_T));
    }

    AX_S32 nMaxCommWidth = 0;
    AX_S32 nMaxCommHeight = 0;
    AX_S32 nMaxPrivateWidth = 0;
    AX_S32 nMaxPrivateHeight = 0;
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = AX_NULL;
    for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++ iSubPpl) {
        pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->arrSubPpl[iSubPpl];
        if (AX_FALSE == pPipeline->stOpalAttr.stVideoAttr[iSubPpl].bEnable) {
            continue;
        }

        for (AX_S32 iSns = 0; iSns < AX_OPAL_SNS_ID_BUTT; ++iSns) {

            AX_S32 nTemp = pSubPipeline->stAttr.nOutWidth == -1 ? pSubPipeline->stAttr.nInWidth : pSubPipeline->stAttr.nOutWidth;
            if (nTemp > nMaxCommWidth) {
                nMaxCommWidth = nTemp;
            }
            nTemp = pSubPipeline->stAttr.nOutHeight == -1 ? pSubPipeline->stAttr.nInHeight : pSubPipeline->stAttr.nOutHeight;
            if (nTemp > nMaxCommHeight) {
                nMaxCommHeight = nTemp;
            }

            nTemp = pSubPipeline->stAttr.nInWidth;
            if (nTemp > nMaxPrivateWidth) {
                nMaxPrivateWidth = nTemp;
            }

            nTemp = pSubPipeline->stAttr.nInHeight;
            if (nTemp > nMaxPrivateHeight) {
                nMaxPrivateHeight = nTemp;
            }
        }
    }

    AX_OPAL_POOL_ATTR_T *pPoolAttr = &stPoolAttr.stPoolAttr[0];
    for (AX_S32 iPool = 0; iPool < pPoolAttr->nCommPoolCfgCnt ; ++iPool) {
        if (pPoolAttr->arrCommPoolCfg[iPool].nWidth == -1
            && pPoolAttr->arrCommPoolCfg[iPool].nHeight == -1) {
            pPoolAttr->arrCommPoolCfg[iPool].nWidth = nMaxCommWidth;
            pPoolAttr->arrCommPoolCfg[iPool].nHeight = nMaxCommHeight;
        }
    }

    for (AX_S32 iPool = 0; iPool < pPoolAttr->nPrivPoolCfgCnt ; ++iPool) {
        if (pPoolAttr->arrPrivPoolCfg[iPool].nWidth == -1
            && pPoolAttr->arrPrivPoolCfg[iPool].nHeight == -1) {
            pPoolAttr->arrPrivPoolCfg[iPool].nWidth = nMaxPrivateWidth;
            pPoolAttr->arrPrivPoolCfg[iPool].nHeight = nMaxPrivateHeight;
        }
    }

    nRet = AX_OPAL_HAL_POOL_Init(&stPoolAttr);
    if (0 != nRet) {
        return -1;
    }

    AX_OPAL_ALGO_ATTR_T stAlgoAttr = {0};
    for (AX_S32 i = 0; i < AX_OPAL_SNS_ID_BUTT; i++) {
        if (pPipeline->stOpalAttr.stVideoAttr[i].bEnable) {
            stAlgoAttr.nAlgoType |= pPipeline->stOpalAttr.stVideoAttr[i].stAlgoAttr.nAlgoType;
            if (!pPipeline->stOpalAttr.stVideoAttr[i].stAlgoAttr.strDetectModelsPath) {
                stAlgoAttr.strDetectModelsPath = pPipeline->stOpalAttr.stVideoAttr[i].stAlgoAttr.strDetectModelsPath;
            }
        }
    }
    nRet = AX_OPAL_HAL_ALGO_Init(&stAlgoAttr);
    if (0 != nRet) {
        return -1;
    }

    if (pPipeline->stOpalAttr.stAudioAttr.stCapAttr.bEnable) {
        nRet = AX_OPAL_HAL_AUDIO_CAP_Init();
        if (0 != nRet) {
            return -1;
        }
    }

    if (pPipeline->stOpalAttr.stAudioAttr.stPlayAttr.bEnable) {
        nRet = AX_OPAL_HAL_AUDIO_PLAY_Init();
        if (0 != nRet) {
            return -1;
        }
    }

    // SetMode
    // - AX_SYS_SetVINIVPSMode
    AX_S32 nVinPipe = 0;
    AX_S32 nIvpsGrp = 0;
    for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++iSubPpl) {
        nVinPipe = -1;
        nIvpsGrp = -1;
        AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->arrSubPpl[iSubPpl];
        for (AX_S32 iLink = 0; iLink < pSubPipeline->stAttr.nLinkCnt; ++iLink) {
            AX_OPAL_LINK_ATTR_T *pLinkAttr = &pSubPipeline->stAttr.arrLinkAttr[iLink];
            if (pLinkAttr->eSrcType == AX_OPAL_ELE_CAM && pLinkAttr->eDstType == AX_OPAL_ELE_IVPS) {
                nVinPipe = pLinkAttr->nSrcGrpId;
                nIvpsGrp = pLinkAttr->nDstGrpId;
                break;
            }
        }
        if (nVinPipe != -1 && nIvpsGrp != -1) {
            // TODO: config with ini file: vin to ivps mode
            /* "-1:Disable, 0:AX_ITP_OFFLINE_VPP, 1:AX_GDC_ONLINE_VPP, 2:AX_ITP_ONLINE_VPP" */
            nRet = AX_OPAL_HAL_SetVinIvpsMode(nVinPipe, nIvpsGrp, 1);
            if (0 != nRet) {
                return -1;
            }
        }
    }
    // LowMemMode
    {
        AX_VIN_LOW_MEM_MODE_E eLowMemMode = AX_VIN_LOW_MEM_DISABLE;
        const char *envValue = getenv("VIN_LOWMEM_MODE");
        if (envValue != NULL) {
            eLowMemMode = (AX_VIN_LOW_MEM_MODE_E)atoi(envValue);
        } else {
            if (IsEnableLowMem(self)) {
                eLowMemMode = AX_VIN_LOW_MEM_ENABLE;
            }
        }

        if (AX_VIN_LOW_MEM_DISABLE != eLowMemMode) {
            nRet = AX_VIN_SetLowMemMode(eLowMemMode);
            if (nRet != 0) {
                LOG_M_E(LOG_TAG, "AX_VIN_SetLowMemMode mode[%d] failed, ret = 0x%04x", eLowMemMode, nRet);
            } else {
                LOG_M_C(LOG_TAG, "AX_VIN_SetLowMemMode mode[%d]", eLowMemMode);
            }
        }
    }

    return nRet;
}

AX_S32 ppl_deinit(AX_OPAL_MAL_PPL_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }
    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)self;

    if (pPipeline->stOpalAttr.stAudioAttr.stCapAttr.bEnable) {
        AX_OPAL_HAL_AUDIO_PLAY_Deinit();
    }
    if (pPipeline->stOpalAttr.stAudioAttr.stPlayAttr.bEnable) {
        AX_OPAL_HAL_AUDIO_CAP_Deinit();
    }

    AX_OPAL_HAL_ALGO_Deinit();
    AX_OPAL_HAL_VENC_Deinit();
    AX_OPAL_HAL_IVPS_Deinit();
    AX_OPAL_HAL_CAM_Deinit();

    AX_OPAL_HAL_POOL_Deinit();
    AX_OPAL_HAL_SYS_Deinit();

    return nRet;
}

static AX_S32 ppl_start(AX_OPAL_MAL_PPL_HANDLE self) {

    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    AX_OPAL_MAL_PPL_T *pPipeline = (AX_OPAL_MAL_PPL_T*)self;

    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = AX_NULL;
    /* video */
    for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++iSubPpl) {
        pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->arrSubPpl[iSubPpl];

        for (AX_S32 iEle = 0; iEle < pSubPipeline->stAttr.nEleCnt; ++iEle) {
            AX_OPAL_MAL_ELE_T *pEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[iEle];
            if (pEle && pEle->vTable && pEle->vTable->start) {
                nRet = pEle->vTable->start(pEle);
                if (nRet != 0) {
                    goto EXIT;
                }
            }
        }

        // link all element
        for (AX_S32 iLink = 0; iLink < pSubPipeline->stAttr.nLinkCnt; ++iLink) {
            AX_OPAL_LINK_ATTR_T *pLink = &(pSubPipeline->stAttr.arrLinkAttr[iLink]);
            if (pLink->eLinkType == AX_OPAL_ELE_LINK) {
                nRet = AX_OPAL_HAL_MOD_Link(pLink);
                if (0 != nRet) {
                    goto EXIT;
                }
            } else if (pLink->eLinkType == AX_OPAL_ELE_NONLINK_FRM) {
                AX_OPAL_MAL_ELE_T *pSrcEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[pLink->nSrcEleId];
                AX_OPAL_MAL_ELE_T *pDstEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[pLink->nDstEleId];

                AX_OPAL_GRP_ATTR_T *pSrcGrpAttr = &pSrcEle->stAttr.arrGrpAttr[0];
                if (pSrcGrpAttr->nGrpId != pLink->nSrcGrpId) {
                    continue;
                }

                for (AX_S32 iChn = 0; iChn < pSrcGrpAttr->nChnCnt; ++iChn) {
                    if (pSrcGrpAttr->nChnId[iChn] != pLink->nSrcChnId) {
                        continue;
                    }
                    AX_OPAL_MAL_OBS_T *parrObs = pSrcGrpAttr->arrObs[iChn];
                    for (AX_S32 iObs = 0; iObs < AX_OPAL_MAL_OBS_CNT; ++iObs) {
                        if (parrObs[iObs].pEleHdl == AX_NULL) {
                            parrObs[iObs].eLinkType = pLink->eLinkType;
                            parrObs[iObs].pEleHdl = pDstEle;
                            break;
                        }
                    }
                }
            }
        }
    }

    /* audio */
    pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->audioPpl;
    for (AX_S32 iEle = 0; iEle < pSubPipeline->stAttr.nEleCnt; ++iEle) {
        AX_OPAL_MAL_ELE_T *pEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[iEle];
        if (pEle && pEle->vTable && pEle->vTable->start) {
            nRet = pEle->vTable->start(pEle);
            if (nRet != 0) {
                goto EXIT;
            }
        }
    }

EXIT:
    return nRet;
}

static AX_S32 ppl_afterstart(AX_OPAL_MAL_PPL_HANDLE self) {

    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    AX_OPAL_MAL_PPL_T *pPipeline = (AX_OPAL_MAL_PPL_T*)self;
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = NULL;
    for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++iSubPpl) {
        pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->arrSubPpl[iSubPpl];
        AX_S32 nUniGrpId = pSubPipeline->stAttr.arrGrpAttr[0].nGrpId;
        AX_OPAL_SNS_ROTATION_E eRotation = pPipeline->stOpalAttr.stVideoAttr[nUniGrpId].stSnsAttr.eRotation;
        if (eRotation != AX_OPAL_SNS_ROTATION_0) {
            AX_OPAL_MAL_PROCESS_DATA_T stProcData;
            memset(&stProcData, 0x0, sizeof(AX_OPAL_MAL_PROCESS_DATA_T));
            stProcData.nUniGrpId = nUniGrpId;
            stProcData.nUniChnId = -1;
            stProcData.eMainCmdType = AX_OPAL_MAINCMD_VIDEO_SETSNSATTR;
            stProcData.eSubCmdType = AX_OPAL_SUBCMD_NON;
            stProcData.pData = (AX_VOID*)&pPipeline->stOpalAttr.stVideoAttr[stProcData.nUniGrpId].stSnsAttr;
            stProcData.nDataSize = sizeof(AX_OPAL_VIDEO_SNS_ATTR_T);
            nRet = update_rotation(pSubPipeline, &stProcData);
            if (nRet != AX_SUCCESS) {
                return nRet;
            }
        }
    }

    return nRet;
}

static AX_S32 ppl_stop(AX_OPAL_MAL_PPL_HANDLE self) {

    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    AX_OPAL_MAL_PPL_T *pPipeline = (AX_OPAL_MAL_PPL_T*)self;
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = NULL;

    /* audio */
    pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->audioPpl;
    for (AX_S32 iEle = 0; iEle < pSubPipeline->stAttr.nEleCnt; ++iEle) {
        AX_OPAL_MAL_ELE_T *pEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[iEle];
        if (pEle && pEle->vTable && pEle->vTable->start) {
            nRet = pEle->vTable->stop(pEle);
            if (nRet != 0) {
                goto EXIT;
            }
        }
    }

    /* video */
    for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++iSubPpl) {
        pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->arrSubPpl[iSubPpl];

        for (AX_S32 iLink = 0; iLink < pSubPipeline->stAttr.nLinkCnt; ++iLink) {
            AX_OPAL_LINK_ATTR_T *pLink = &(pSubPipeline->stAttr.arrLinkAttr[iLink]);
            if (pLink->eLinkType == AX_OPAL_ELE_LINK) {
                nRet = AX_OPAL_HAL_MOD_UnLink(pLink);
                if (0 != nRet) {
                    goto EXIT;
                }
                else if (pLink->eLinkType == AX_OPAL_ELE_NONLINK_FRM) {
                    AX_OPAL_MAL_ELE_T *pSrcEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[pLink->nSrcEleId];
                    // AX_OPAL_MAL_ELE_T *pDstEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[pLink->nDstEleId];
                    AX_OPAL_GRP_ATTR_T *pSrcGrpAttr = &pSrcEle->stAttr.arrGrpAttr[0];
                    if (pSrcGrpAttr->nGrpId != pLink->nSrcGrpId) {
                        continue;
                    }

                    for (AX_S32 iChn = 0; iChn < pSrcGrpAttr->nChnCnt; ++iChn) {
                        if (pSrcGrpAttr->nChnId[iChn] != pLink->nSrcChnId) {
                            continue;
                        }
                        AX_OPAL_MAL_OBS_T *parrObs = pSrcGrpAttr->arrObs[iChn];
                        for (AX_S32 iObs = 0; iObs < AX_OPAL_MAL_OBS_CNT; ++iObs) {
                            parrObs[iObs].pEleHdl = AX_NULL;
                            parrObs[iObs].eLinkType = AX_OPAL_LINK_BUTT;
                        }
                    }
                }
            }
        }

        for (AX_S32 iEle = 0; iEle < pSubPipeline->stAttr.nEleCnt; ++iEle) {
            AX_OPAL_MAL_ELE_T *pEle = (AX_OPAL_MAL_ELE_T *)pSubPipeline->arrEle[iEle];
            if (pEle && pEle->vTable && pEle->vTable->stop) {
                nRet = pEle->vTable->stop(pEle);
                if (nRet != 0) {
                    goto EXIT;
                }
            }
        }
    }

EXIT:
    return nRet;
}

AX_S32 AX_OPAL_MAL_PPL_Process(AX_OPAL_MAL_PPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    LOG_M_D(LOG_TAG, "+++");

    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    if (pPorcessData == AX_NULL) {
        return AX_ERR_OPAL_INVALID_HANDLE;
    }

    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)self;

    AX_OPAL_MAIN_CMD_E eMainCmdType = pPorcessData->eMainCmdType;
    if (AX_OPAL_MAINCMD_INIT == eMainCmdType) {
        nRet = ppl_init(self, pPorcessData);
    }
    else if (AX_OPAL_MAINCMD_DEINIT == eMainCmdType) {
        nRet = ppl_deinit(self);
    }
    if (AX_OPAL_MAINCMD_START == eMainCmdType) {
        nRet = ppl_start(self);
        nRet |= ppl_afterstart(self);
    }
    else if (AX_OPAL_MAINCMD_STOP == eMainCmdType) {
        nRet = ppl_stop(self);
    }
    /* video */
    else if (AX_OPAL_MAINCMD_VIDEO_BEGIN < eMainCmdType && eMainCmdType < AX_OPAL_MAINCMD_VIDEO_FINISH) {
        AX_OPAL_MAL_SUBPPL_T* pSubPipeline = AX_NULL;
        for (AX_S32 iSubPpl = 0; iSubPpl < pPipeline->nSubPplCnt; ++iSubPpl) {
            pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)pPipeline->arrSubPpl[iSubPpl];
            for (AX_S32 iGrp = 0; iGrp < pSubPipeline->stAttr.nGrpCnt; ++iGrp) {
                if (pPorcessData->nUniGrpId == pSubPipeline->stAttr.arrGrpAttr[iGrp].nGrpId) {
                    nRet = AX_OPAL_MAL_SUBPPL_Process(pSubPipeline, pPorcessData);
                    if (nRet != 0) {
                        goto EXIT;
                    }
                }
            }
        }
    }
    /* audio */
    else if (AX_OPAL_MAINCMD_AUDIO_BEGIN < eMainCmdType && eMainCmdType < AX_OPAL_MAINCMD_AUDIO_FINISH) {
        nRet = AX_OPAL_MAL_SUBPPL_Process(pPipeline->audioPpl, pPorcessData);
        if (nRet != 0) {
            goto EXIT;
        }
    }

EXIT:
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 subppl_get_sns_attr(AX_OPAL_MAL_SUBPPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    if (sizeof(AX_OPAL_VIDEO_SNS_ATTR_T) != pPorcessData->nDataSize) {
        return AX_ERR_OPAL_GENERIC;
    }

#if 0
    /* step1: get cam element */
    AX_OPAL_MAL_ELE_T *pCamEle = subppl_get_cam(self);
    /* step2: do AX_OPAL_MAINCMD_VIDEO_GETSNSATTR */
    if (pCamEle) {
        nRet = pCamEle->vTable->event_proc((AX_OPAL_MAL_ELE_HANDLE)pCamEle, pPorcessData);
    } else {
        nRet = -1;
    }
#else
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)self;
    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)pSubPipeline->pParent;
    memcpy(pPorcessData->pData, &pPipeline->stOpalAttr.stVideoAttr[pPorcessData->nUniGrpId].stSnsAttr, sizeof(AX_OPAL_VIDEO_SNS_ATTR_T));
#endif

    return nRet;
}

static AX_S32 subppl_set_sns_attr(AX_OPAL_MAL_SUBPPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    if (sizeof(AX_OPAL_VIDEO_SNS_ATTR_T) != pPorcessData->nDataSize) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* internal check */
    if (pPorcessData->nUniChnId != -1) {
        LOG_M_E(LOG_TAG, "processing sensor-related commands, must set the chnid to -1.");
        return -1;
    }

    /* pipeline */
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)self;
    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)pSubPipeline->pParent;

    /* sensor attr */
    AX_OPAL_VIDEO_SNS_ATTR_T *pstCurSnsAttr = &pPipeline->stOpalAttr.stVideoAttr[pPorcessData->nUniGrpId].stSnsAttr;
    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = pPorcessData->pData;

    /* do event */
    if (pstSnsAttr->eMode != pstCurSnsAttr->eMode) {

        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_SDRHDRMODE);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            pstCurSnsAttr->eMode = pstSnsAttr->eMode;
        }
        nRet |= nRetTmp;
    }

    if (pstSnsAttr->fFrameRate != pstCurSnsAttr->fFrameRate) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_FPS);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }

            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_FPS);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }

            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_FPS);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            pstCurSnsAttr->fFrameRate = pstSnsAttr->fFrameRate;
        }
        nRet |= nRetTmp;
    }

    if (pstSnsAttr->bFlip != pstCurSnsAttr->bFlip) {

        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_FLIP);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            pstCurSnsAttr->bFlip = pstSnsAttr->bFlip;
        }
        nRet |= nRetTmp;
    }

    if (pstSnsAttr->bMirror != pstCurSnsAttr->bMirror) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_MIRROR);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            pstCurSnsAttr->bMirror = pstSnsAttr->bMirror;
        }
        nRet |= nRetTmp;
    }

    if (pstSnsAttr->eRotation != pstCurSnsAttr->eRotation) {
        AX_S32 nRet = update_rotation(self, pPorcessData);
        if (nRet == AX_SUCCESS) {
            pstCurSnsAttr->eRotation = pstSnsAttr->eRotation;
        }
    }

    // hal to decide if invoke to set this property, becuase pstCurSnsAttr->eDayNight may not correct for sps auto mode
    //if (pstSnsAttr->eDayNight != pstCurSnsAttr->eDayNight)
    {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_DAYNIGHT);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            pstCurSnsAttr->eDayNight = pstSnsAttr->eDayNight;
        }
        nRet |= nRetTmp;
    }

    if (0 != memcmp(&pstSnsAttr->stEZoomAttr, &pstCurSnsAttr->stEZoomAttr, sizeof(AX_OPAL_SNS_EZOOM_ATTR_T))) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_EZOOM);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            memcpy(&pstCurSnsAttr->stEZoomAttr, &pstSnsAttr->stEZoomAttr, sizeof(AX_OPAL_SNS_EZOOM_ATTR_T));
        }
        nRet |= nRetTmp;
    }

    if (0 != memcmp(&pstSnsAttr->stColorAttr, &pstCurSnsAttr->stColorAttr, sizeof(AX_OPAL_SNS_COLOR_ATTR_T))) {

        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_COLOR);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            memcpy(&pstCurSnsAttr->stColorAttr, &pstSnsAttr->stColorAttr, sizeof(AX_OPAL_SNS_COLOR_ATTR_T));
        }
        nRet |= nRetTmp;
    }

    if (0 != memcmp(&pstSnsAttr->stLdcAttr, &pstCurSnsAttr->stLdcAttr, sizeof(AX_OPAL_SNS_LDC_ATTR_T))) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_LDC);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            memcpy(&pstCurSnsAttr->stLdcAttr, &pstSnsAttr->stLdcAttr, sizeof(AX_OPAL_SNS_LDC_ATTR_T));
        }
        nRet |= nRetTmp;
    }

    if (0 != memcmp(&pstSnsAttr->stDisAttr, &pstCurSnsAttr->stDisAttr, sizeof(AX_OPAL_SNS_DIS_ATTR_T))) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_SNSATTR_DIS);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);
        if (AX_SUCCESS == nRetTmp) {
            memcpy(&pstCurSnsAttr->stDisAttr, &pstSnsAttr->stDisAttr, sizeof(AX_OPAL_SNS_DIS_ATTR_T));
        }
        nRet |= nRetTmp;
    }

    return nRet;
}

static AX_S32 subppl_get_chn_attr(AX_OPAL_MAL_SUBPPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    if (sizeof(AX_OPAL_VIDEO_CHN_ATTR_T) != pPorcessData->nDataSize) {
        return AX_ERR_OPAL_GENERIC;
    }

    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)((AX_OPAL_MAL_SUBPPL_T*)self)->pParent;
    memcpy(pPorcessData->pData,
        &pPipeline->stOpalAttr.stVideoAttr[pPorcessData->nUniGrpId].stPipeAttr.stVideoChnAttr[pPorcessData->nUniChnId],
        sizeof(AX_OPAL_VIDEO_CHN_ATTR_T));

    return nRet;
}

static AX_S32 subppl_set_chn_attr(AX_OPAL_MAL_SUBPPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;

    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    if (sizeof(AX_OPAL_VIDEO_CHN_ATTR_T) != pPorcessData->nDataSize) {
        return AX_ERR_OPAL_GENERIC;
    }

    /* internal check */
    if (pPorcessData->nUniChnId == -1) {
        LOG_M_E(LOG_TAG, "processing sensor-related commands, must set the chnid to not(-1).");
        return -1;
    }

    /* pipeline */
    AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)self;
    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)pSubPipeline->pParent;

    /* sensor attr */
    AX_OPAL_VIDEO_CHN_ATTR_T *pstAttrCur = &pPipeline->stOpalAttr.stVideoAttr[pPorcessData->nUniGrpId].stPipeAttr.stVideoChnAttr[pPorcessData->nUniChnId];
    AX_OPAL_VIDEO_CHN_ATTR_T *pstAttr = pPorcessData->pData;

    /* enable/disable */
    if (pstAttr->bEnable != pstAttrCur->bEnable) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            if (pstAttr->bEnable) {
                // Step 0. venc/jenc start
                nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_POSTPROC);
                if (AX_SUCCESS != nRetTmp) {
                    break;
                }
                // Step 1. ivps enable
                nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_POSTPROC);
                if (AX_SUCCESS != nRetTmp) {
                    break;
                }
            } else {
                // Step 0. ivps disable
                nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_PREPROC);
                if (AX_SUCCESS != nRetTmp) {
                    break;
                }
                // Step 1. venc/jenc stop and reset
                nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_PREPROC);
                if (AX_SUCCESS != nRetTmp) {
                    break;
                }

            }
        } while (0);

        if (AX_SUCCESS == nRetTmp) {
            pstAttrCur->bEnable = pstAttr->bEnable;
        }
        nRet |= nRetTmp;
    }

    /* encode type */
    if (pstAttr->eType != pstAttrCur->eType) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_CHNATTR_ENCTYPE);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);

        if (AX_SUCCESS == nRetTmp) {
            pstAttrCur->eType = pstAttr->eType;
        }
        nRet |= nRetTmp;
    }

    /* resolution */
    if (pstAttr->nHeight != pstAttrCur->nHeight || pstAttr->nWidth != pstAttrCur->nWidth) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            // Step 0. ivps disable
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_PREPROC);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
            // Step 1. venc/jenc stop and reset
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_PREPROC);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
            // Step 2. ivps update rotation, update osd
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_CHNATTR_RESOLUTION);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
            // Step 3. venc/jenc update rotation
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_CHNATTR_RESOLUTION);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
            // Step 4. algo(ives) update rotation
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_ALGO, pPorcessData, AX_OPAL_SUBCMD_CHNATTR_RESOLUTION);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
            // Step 5. venc/jenc stop and reset
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_POSTPROC);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
            // Step 6. ivps disable
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_POSTPROC);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);

        if (AX_SUCCESS == nRetTmp) {
            pstAttrCur->nHeight = pstAttr->nHeight;
            pstAttrCur->nWidth = pstAttr->nWidth;
        }
        nRet |= nRetTmp;
    }

    /* encode config */
    if (0 != memcmp(&pstAttr->stEncoderAttr, &pstAttrCur->stEncoderAttr, sizeof(AX_OPAL_VIDEO_ENCODER_ATTR_T))) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_CHNATTR_ENCCFG);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);

        if (AX_SUCCESS == nRetTmp) {
            memcpy(&pstAttrCur->stEncoderAttr, &pstAttr->stEncoderAttr, sizeof(AX_OPAL_VIDEO_ENCODER_ATTR_T));
        }
        nRet |= nRetTmp;
    }

    /* fps config */
    if (pstAttr->nFramerate != pstAttrCur->nFramerate) {
        AX_S32 nRetTmp = AX_SUCCESS;
        do {
            nRetTmp = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_CHNATTR_FPS);
            if (AX_SUCCESS != nRetTmp) {
                break;
            }
        } while (0);

        if (AX_SUCCESS == nRetTmp) {
            pstAttrCur->nFramerate = pstAttr->nFramerate;
        }
        nRet |= nRetTmp;
    }

    return nRet;
}

AX_S32 AX_OPAL_MAL_SUBPPL_Process(AX_OPAL_MAL_SUBPPL_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    LOG_M_D(LOG_TAG, "+++");

    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NOT_INIT;
    }

    /* pipeline */
    // AX_OPAL_MAL_SUBPPL_T* pSubPipeline = (AX_OPAL_MAL_SUBPPL_T*)self;

    /* process data command type */
    AX_OPAL_MAIN_CMD_E eMainCmdType = pPorcessData->eMainCmdType;
    LOG_M_D(LOG_TAG, "process type=%d sndid=%d chnid=%d", eMainCmdType, pPorcessData->nUniGrpId, pPorcessData->nUniChnId);

    /* command for video */
    if (AX_OPAL_MAINCMD_VIDEO_BEGIN < eMainCmdType && eMainCmdType < AX_OPAL_MAINCMD_VIDEO_FINISH) {

        switch (eMainCmdType) {
            case AX_OPAL_MAINCMD_VIDEO_GETSNSATTR:
                nRet = subppl_get_sns_attr(self, pPorcessData);
                break;
            case AX_OPAL_MAINCMD_VIDEO_SETSNSATTR:
                nRet = subppl_set_sns_attr(self, pPorcessData);
                nRet |= subppl_ele_eventproc(self, AX_OPAL_ELE_ALGO, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            case AX_OPAL_MAINCMD_VIDEO_GETSNSSOFTPHOTOSENSITIVITYATTR:
            case AX_OPAL_MAINCMD_VIDEO_SETSNSSOFTPHOTOSENSITIVITYATTR:
            case AX_OPAL_MAINCMD_VIDEO_REGISTERSNSSOFTPHOTOSENSITIVITYCALLBACK:
            case AX_OPAL_MAINCMD_VIDEO_UNREGISTERSNSSOFTPHOTOSENSITIVITYCALLBACK:
            case AX_OPAL_MAINCMD_VIDEO_GETSNSHOTNOISEBALANCEATTR:
            case AX_OPAL_MAINCMD_VIDEO_SETSNSHOTNOISEBALANCEATTR:
                nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_CAM, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            case AX_OPAL_MAINCMD_VIDEO_GETCHNATTR:
                nRet = subppl_get_chn_attr(self, pPorcessData);
                break;
            case AX_OPAL_MAINCMD_VIDEO_SETCHNATTR:
                nRet = subppl_set_chn_attr(self, pPorcessData);
                break;
            case AX_OPAL_MAINCMD_VIDEO_REGISTERPACKETCALLBACK:
            case AX_OPAL_MAINCMD_VIDEO_UNREGISTERPACKETCALLBACK:
                nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            case AX_OPAL_MAINCMD_VIDEO_REQUESTIDR:
                nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            case AX_OPAL_MAINCMD_VIDEO_SNAPSHOT:
                nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            case AX_OPAL_MAINCMD_VIDEO_CAPTUREFRAME:
                nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            case AX_OPAL_MAINCMD_VIDEO_GETSVCPARAM:
            case AX_OPAL_MAINCMD_VIDEO_SETSVCPARAM:
            case AX_OPAL_MAINCMD_VIDEO_SETSVCREGION:
                nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_VENC, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            case AX_OPAL_MAINCMD_VIDEO_ALGOGETPARAM:
            case AX_OPAL_MAINCMD_VIDEO_ALGOSETPARAM:
            case AX_OPAL_MAINCMD_VIDEO_REGISTERALGOCALLBACK:
            case AX_OPAL_MAINCMD_VIDEO_UNREGISTERALGOCALLBACK:
                nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_ALGO, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            case AX_OPAL_MAINCMD_VIDEO_OSDCREATE:
            case AX_OPAL_MAINCMD_VIDEO_OSDDESTROY:
            case AX_OPAL_MAINCMD_VIDEO_OSDUPDATE:
            case AX_OPAL_MAINCMD_VIDEO_OSDDRAWRECT:
            case AX_OPAL_MAINCMD_VIDEO_OSDCLEARRECT:
            case AX_OPAL_MAINCMD_VIDEO_OSDDRAWPOLYGON:
            case AX_OPAL_MAINCMD_VIDEO_OSDCLEARPOLYGON:
                nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_IVPS, pPorcessData, AX_OPAL_SUBCMD_NON);
                break;
            default:
                LOG_M_E(LOG_TAG, "not support");
                break;
        }
    }
    /* audio */
    else if (AX_OPAL_MAINCMD_AUDIO_BEGIN < eMainCmdType && eMainCmdType < AX_OPAL_MAINCMD_AUDIO_FINISH) {
        nRet = subppl_ele_eventproc(self, AX_OPAL_ELE_AUDIO, pPorcessData, AX_OPAL_SUBCMD_NON);
    }
    LOG_M_D(LOG_TAG, "---");
    return nRet;
}
