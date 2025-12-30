/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_elecam.h"

#include "ax_opal_mal_utils.h"
#include "ax_opal_hal_cam_parser.h"
#include "ax_opal_hal_cam.h"
#include "ax_opal_hal_sns.h"
#include "ax_opal_hal_sps.h"
#include "ax_opal_hal_hnb.h"

#include "ax_opal_log.h"
#include "ax_opal_utils.h"
#include "ax_opal_mal_element.h"

#define LOG_TAG ("ELECAM")

// interface vtable
AX_OPAL_MAL_ELE_Interface cam_interface = {
    .start = AX_OPAL_MAL_ELECAM_Start,
    .stop = AX_OPAL_MAL_ELECAM_Stop,
    .event_proc = AX_OPAL_MAL_ELECAM_Process,
};

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELECAM_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr) {

    AX_OPAL_MAL_ELE_HANDLE handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_ELECAM_T));
    if (handle == AX_NULL) {
        LOG_M_E(LOG_TAG, "malloc failed.");
        return AX_NULL;
    }

    AX_OPAL_MAL_ELECAM_T* pEle = (AX_OPAL_MAL_ELECAM_T*)handle;
    memset(pEle, 0x0, sizeof(AX_OPAL_MAL_ELECAM_T));
    pEle->stBase.nId = pEleAttr->nId;
    pEle->stBase.pParent = parent;
    pEle->stBase.vTable = &cam_interface;
    memcpy(&pEle->stBase.stAttr, pEleAttr, sizeof(AX_OPAL_ELEMENT_ATTR_T));
    AX_OPAL_MAL_ELE_MappingGrpChn(handle);

    /* parse attr */
    AX_OPAL_MAL_SUBPPL_T* pSubPpl = (AX_OPAL_MAL_SUBPPL_T*)pEle->stBase.pParent;
    AX_OPAL_MAL_PPL_T* pPipeline = (AX_OPAL_MAL_PPL_T*)pSubPpl->pParent;
    AX_CHAR *cCfgIniPath = pPipeline->stOpalAttr.stVideoAttr[pSubPpl->nId].szSnsConfigPath;
    AX_S32 nRet = AX_OPAL_HAL_CAM_Parse(cCfgIniPath, &pEle->stAttr.tCamCfg, &pEle->stId);
    if (0 != nRet) {
        AX_OPAL_FREE(pEle);
        return AX_NULL;
    }
    pPipeline->stOpalAttr.stVideoAttr[pSubPpl->nId].stSnsAttr.fFrameRate = pEle->stAttr.tCamCfg.tSensorCfg.fFrameRate;
    pPipeline->stOpalAttr.stVideoAttr[pSubPpl->nId].stSnsAttr.nWidth = pEle->stAttr.tCamCfg.tPipeCfg.nWidth;
    pPipeline->stOpalAttr.stVideoAttr[pSubPpl->nId].stSnsAttr.nHeight = pEle->stAttr.tCamCfg.tPipeCfg.nHeight;

    /* set w and h into subppl */
    /* main channel input  */
    pSubPpl->stAttr.nInWidth = pEle->stAttr.tCamCfg.tPipeCfg.nWidth;
    pSubPpl->stAttr.nInHeight = pEle->stAttr.tCamCfg.tPipeCfg.nHeight;
    /* main channel input */
    pSubPpl->stAttr.nOutWidth = pPipeline->stOpalAttr.stVideoAttr[pSubPpl->nId].stPipeAttr.stVideoChnAttr[0].nWidth;
    pSubPpl->stAttr.nOutHeight = pPipeline->stOpalAttr.stVideoAttr[pSubPpl->nId].stPipeAttr.stVideoChnAttr[0].nHeight;

    /* load sensor lib */
    nRet = AX_OPAL_HAL_SNS_Init(pEle->stAttr.tCamCfg.tSensorCfg.cLibName, pEle->stAttr.tCamCfg.tSensorCfg.cObjName, &pEle->stAttr.tSnsLibAttr);
    if (0 != nRet) {
        AX_OPAL_FREE(pEle);
        return AX_NULL;
    }

    AX_OPAL_HAL_SPS_SetSnsInfo(pEle->stId.nDevId, pEle->stId.nPipeId, pEle->stId.nChnId);
    AX_OPAL_HAL_HNB_SetSnsInfo(pEle->stId.nDevId, pEle->stId.nPipeId, pEle->stId.nChnId, pEle->stAttr.tSnsLibAttr.pSensorHandle);

    return handle;
}

AX_S32 AX_OPAL_MAL_ELECAM_Destroy(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELECAM_T* pEle = (AX_OPAL_MAL_ELECAM_T*)self;
    /* close sensor lib */
    nRet = AX_OPAL_HAL_SNS_Deinit(&pEle->stAttr.tSnsLibAttr);
    if (nRet != 0) {
        AX_OPAL_FREE(pEle);
        return -1;
    }

    AX_OPAL_FREE(self);
    return nRet;
}

AX_S32 AX_OPAL_MAL_ELECAM_Start(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELECAM_T* pEle = (AX_OPAL_MAL_ELECAM_T*)self;
    nRet = AX_OPAL_HAL_CAM_Open(&pEle->stId, &pEle->stAttr);
    if (nRet != 0) {
        return -1;
    }

    /* init with sensor attr */
    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = AX_OPAL_MAL_GetSnsAttr(self);

    nRet |= AX_OPAL_HAL_CAM_SetFps(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetFlip(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetMirror(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetDayNight(&pEle->stId, pstSnsAttr);
    // nRet |= AX_OPAL_HAL_CAM_SetRotation(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetEZoom(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetColor(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetDis(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetLdc(&pEle->stId, pstSnsAttr);

    nRet = AX_OPAL_HAL_CAM_Start(&pEle->stId);
    if (nRet != 0) {
        return -1;
    }
    return nRet;
}

AX_S32 AX_OPAL_MAL_ELECAM_Stop(AX_OPAL_MAL_ELE_HANDLE self) {
    AX_S32 nRet = AX_SUCCESS;
    LOG_M_D(LOG_TAG, "+++");
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    AX_OPAL_MAL_ELECAM_T* pEle = (AX_OPAL_MAL_ELECAM_T*)self;
    nRet = AX_OPAL_HAL_CAM_Stop(&pEle->stId);
    if (nRet != 0) {
        return -1;
    }

    nRet = AX_OPAL_HAL_CAM_Close(&pEle->stId, &pEle->stAttr);
    if (nRet != 0) {
        return -1;
    }

    LOG_M_D(LOG_TAG, "---");
    return nRet;
}

static AX_S32 switch_snsmode(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_OPAL_MAL_ELECAM_T* pEle = (AX_OPAL_MAL_ELECAM_T*)self;
    AX_OPAL_VIDEO_SNS_ATTR_T *pAttr = (AX_OPAL_VIDEO_SNS_ATTR_T *)pPorcessData->pData;

    AX_S32 nRet = AX_OPAL_HAL_CAM_Stop(&pEle->stId);
    if (nRet != 0) {
        return -1;
    }
    nRet = AX_OPAL_HAL_CAM_Close(&pEle->stId, &pEle->stAttr);
    if (nRet != 0) {
        return -1;
    }

    /* set fps to sensor config */
    pEle->stAttr.tCamCfg.tSensorCfg.fFrameRate = pAttr->fFrameRate;
    pEle->stAttr.tCamCfg.tSensorCfg.eSnsMode = pAttr->eMode;
    pEle->stAttr.tCamCfg.tDevCfg.eSnsMode = pAttr->eMode;
    pEle->stAttr.tCamCfg.tPipeCfg.eSnsMode = pAttr->eMode;

    nRet = AX_OPAL_HAL_CAM_Open(&pEle->stId, &pEle->stAttr);
    if (nRet != 0) {
        return -1;
    }

    AX_OPAL_VIDEO_SNS_ATTR_T *pstSnsAttr = AX_OPAL_MAL_GetSnsAttr(self);
    nRet |= AX_OPAL_HAL_CAM_SetFps(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetFlip(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetMirror(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetDayNight(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetRotation(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetEZoom(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetColor(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetDis(&pEle->stId, pstSnsAttr);
    nRet |= AX_OPAL_HAL_CAM_SetLdc(&pEle->stId, pstSnsAttr);

    nRet = AX_OPAL_HAL_CAM_Start(&pEle->stId);
    if (nRet != 0) {
        return -1;
    }

    return 0;
}

AX_S32 AX_OPAL_MAL_ELECAM_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;
    if (self == AX_NULL) {
        return AX_ERR_OPAL_NULL_PTR;
    }

    if (!AX_OPAL_MAL_ELE_CheckUniGrpChnId(self, pPorcessData->nUniGrpId, pPorcessData->nUniChnId)) {
        return AX_ERR_OPAL_UNEXIST;
    }

    AX_OPAL_MAL_ELECAM_T* pEle = (AX_OPAL_MAL_ELECAM_T*)self;
    if (AX_OPAL_MAINCMD_VIDEO_SETSNSATTR == pPorcessData->eMainCmdType) {
        switch (pPorcessData->eSubCmdType) {
            case AX_OPAL_SUBCMD_SNSATTR_SDRHDRMODE:
                nRet = switch_snsmode(self, pPorcessData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_FPS:
                nRet = AX_OPAL_HAL_CAM_SetFps(&pEle->stId, pPorcessData->pData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_FLIP:
                nRet = AX_OPAL_HAL_CAM_SetFlip(&pEle->stId, pPorcessData->pData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_MIRROR:
                nRet = AX_OPAL_HAL_CAM_SetMirror(&pEle->stId, pPorcessData->pData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_ROTATION:
                nRet = AX_OPAL_HAL_CAM_SetRotation(&pEle->stId, pPorcessData->pData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_DAYNIGHT:
                nRet = AX_OPAL_HAL_CAM_SetDayNight(&pEle->stId, pPorcessData->pData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_EZOOM:
                nRet = AX_OPAL_HAL_CAM_SetEZoom(&pEle->stId, pPorcessData->pData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_COLOR:
                nRet = AX_OPAL_HAL_CAM_SetColor(&pEle->stId, pPorcessData->pData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_LDC:
                nRet = AX_OPAL_HAL_CAM_SetLdc(&pEle->stId, pPorcessData->pData);
                break;
            case AX_OPAL_SUBCMD_SNSATTR_DIS:
                nRet = AX_OPAL_HAL_CAM_SetDis(&pEle->stId, pPorcessData->pData);
                break;
            default:
                break;
        }
    }
#if 0 /* AX_OPAL_MAINCMD_VIDEO_GETSNSATTR: get attr from attr of subppl, not run here */
    else if (AX_OPAL_MAINCMD_VIDEO_GETSNSATTR == pPorcessData->eMainCmdType) {

        nRet = AX_OPAL_HAL_CAM_GetFps(&pEle->stId, pPorcessData->pData);
        nRet |= AX_OPAL_HAL_CAM_GetFlip(&pEle->stId, pPorcessData->pData);
        nRet |= AX_OPAL_HAL_CAM_GetMirror(&pEle->stId, pPorcessData->pData);
        // nRet |= AX_OPAL_HAL_CAM_GetMode(&pEle->stId, pPorcessData->pData);
        nRet |= AX_OPAL_HAL_CAM_GetRotation(&pEle->stId, pPorcessData->pData);
        nRet |= AX_OPAL_HAL_CAM_GetDayNight(&pEle->stId, pPorcessData->pData);
        nRet |= AX_OPAL_HAL_CAM_GetEZoom(&pEle->stId, pPorcessData->pData);
        nRet |= AX_OPAL_HAL_CAM_GetColor(&pEle->stId, pPorcessData->pData);
        nRet |= AX_OPAL_HAL_CAM_GetLdc(&pEle->stId, pPorcessData->pData);
        nRet |= AX_OPAL_HAL_CAM_GetDis(&pEle->stId, pPorcessData->pData);
    }
#endif
    else if (AX_OPAL_MAINCMD_VIDEO_GETSNSSOFTPHOTOSENSITIVITYATTR == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_SPS_GetAttr(pEle->stId.nPipeId, pPorcessData->pData);
    } else if (AX_OPAL_MAINCMD_VIDEO_SETSNSSOFTPHOTOSENSITIVITYATTR == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_SPS_SetAttr(pEle->stId.nPipeId, pPorcessData->pData);
    } else if (AX_OPAL_MAINCMD_VIDEO_REGISTERSNSSOFTPHOTOSENSITIVITYCALLBACK == pPorcessData->eMainCmdType) {
        AX_OPAL_VIDEO_SOFTPHOTOSENSITIVITY_CALLBACK_T *pData = (AX_OPAL_VIDEO_SOFTPHOTOSENSITIVITY_CALLBACK_T *)(pPorcessData->pData);
        nRet = AX_OPAL_HAL_SPS_RegisterCallback(pEle->stId.nPipeId, pData->callback, pData->pUserData);
    } else if (AX_OPAL_MAINCMD_VIDEO_UNREGISTERSNSSOFTPHOTOSENSITIVITYCALLBACK == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_SPS_UnRegisterCallback(pEle->stId.nPipeId);
    } else if (AX_OPAL_MAINCMD_VIDEO_GETSNSHOTNOISEBALANCEATTR == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_HNB_GetAttr(pEle->stId.nPipeId, pPorcessData->pData);
    } else if (AX_OPAL_MAINCMD_VIDEO_SETSNSHOTNOISEBALANCEATTR == pPorcessData->eMainCmdType) {
        nRet = AX_OPAL_HAL_HNB_SetAttr(pEle->stId.nPipeId, pPorcessData->pData);
    }

    return nRet;
}
