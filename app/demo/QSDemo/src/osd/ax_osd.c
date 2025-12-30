/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include <time.h>
#include <wchar.h>
#include <sys/prctl.h>
#include "ax_osd.h"
#include "ax_ivps_api.h"

extern SAMPLE_CHN_ATTR_T gOutChnAttr[];

static AX_BOOL g_Osd_nRotation = 0; // 0: 0; 1: 90; 2: 180; 3: 270
static AX_BOOL g_Osd_bAovStatus = AX_FALSE;

static AX_OSD_REGION_PARAM_T g_rgn_param[MAX_REGION_COUNT];

static AX_VOID UpdateOSDLogo(AX_OSD_REGION_PARAM_T* ptRgn) {
    IVPS_RGN_HANDLE hRgn = ptRgn->hRgn;
    AX_S32 nVencChn = ptRgn->nVencChn;
    AX_OSD_ALIGN_TYPE_E eAlign = ptRgn->eAlign;

    const AX_CHAR *pszFileName = (const AX_CHAR *)ptRgn->tPicAttr.szFileName;

    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
    tDisp.tChnAttr.nZindex = ptRgn->nZindex;
    tDisp.tChnAttr.bSingleCanvas = AX_TRUE;

    AX_U32 nSrcWidth = gOutChnAttr[nVencChn].nWidth;
    AX_U32 nSrcHeight = gOutChnAttr[nVencChn].nHeight;

    if (g_Osd_nRotation == 1 || g_Osd_nRotation == 3) {
        AX_S32 nTmp = nSrcWidth;
        nSrcWidth = nSrcHeight;
        nSrcHeight = nTmp;
    }

    AX_U32 nSrcOffset = 0;

    AX_U32 nPicWidth = ALIGN_UP(ptRgn->tPicAttr.nWidth, AX_OSD_ALIGN_HEIGHT);
    AX_U32 nPicHeight = ALIGN_UP(ptRgn->tPicAttr.nHeight, AX_OSD_ALIGN_HEIGHT);

    /* Config picture OSD */
    AX_U32 nPicMarginX = ptRgn->nBoundaryX;
    AX_U32 nPicMarginY = ptRgn->nBoundaryY;

    AX_U32 nSrcBlock = nSrcWidth / AX_OSD_ALIGN_X_OFFSET;
    AX_U32 nGap = nSrcWidth % AX_OSD_ALIGN_X_OFFSET;

    AX_U32 nBlockBollowed = ceil((AX_F32)(nPicWidth + nPicMarginX - nGap) / AX_OSD_ALIGN_X_OFFSET);

    if (nBlockBollowed < 0) {
        nBlockBollowed = 0;
    }

    AX_U32 nOffsetX = nSrcOffset + (nSrcBlock - nBlockBollowed) * AX_OSD_ALIGN_X_OFFSET;
    AX_U32 nOffsetY = OSD_OverlayOffsetY(nSrcHeight, nPicHeight, nPicMarginY, eAlign);

    if (0 != OSD_LoadImage(pszFileName, (AX_VOID**)&tDisp.arrDisp[0].uDisp.tOSD.pBitmap, nPicWidth * nPicHeight * 2)) {
        ALOGE("Load logo(%s) failed.", pszFileName);
        return;
    }

    tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
    tDisp.arrDisp[0].bShow = ptRgn->bEnable;
    tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_ARGB1555;
    tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = 50;
    tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = nOffsetX;
    tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = nOffsetY;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = nPicWidth;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = nPicHeight;

    AX_S32 ret = AX_IVPS_RGN_Update(hRgn, &tDisp);

    if (AX_SUCCESS != ret) {
        ALOGE("AX_IVPS_RGN_Update fail, ret=0x%x, hRgn=%d", ret, hRgn);
    }

    if (tDisp.arrDisp[0].uDisp.tOSD.pBitmap) {
        free(tDisp.arrDisp[0].uDisp.tOSD.pBitmap);
    }

    return;
}

static AX_VOID UpdateOSDTime(AX_OSD_REGION_PARAM_T *ptRgn) {
    IVPS_RGN_HANDLE hRgn = ptRgn->hRgn;
    AX_S32 nVencChn = ptRgn->nVencChn;
    AX_U32 nRGB = ptRgn->tTimeAttr.nColor;
    AX_OSD_ALIGN_TYPE_E eAlign = ptRgn->eAlign;

    AX_U32 nSrcWidth = gOutChnAttr[nVencChn].nWidth;
    AX_U32 nSrcHeight = gOutChnAttr[nVencChn].nHeight;

    if (g_Osd_nRotation == 1 || g_Osd_nRotation == 3) {
        AX_S32 nTmp = nSrcWidth;
        nSrcWidth = nSrcHeight;
        nSrcHeight = nTmp;
    }

    AX_U32 nSrcOffset = 0;

    AX_U32 nFontSize = OSD_GetTimeFontSize(nSrcWidth, nSrcHeight);
    nFontSize = ALIGN_UP(nFontSize, BASE_FONT_SIZE);

    AX_U32 nMarginX = OSD_GetFontBoundaryX(nSrcWidth, nSrcHeight);
    AX_U32 nMarginY = OSD_GetFontBoundaryY(nSrcWidth, nSrcHeight);;

    AX_U32 nPicOffset = nMarginX % AX_OSD_ALIGN_WIDTH;
    AX_U32 nPicOffsetBlock = nMarginX / AX_OSD_ALIGN_WIDTH;

    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    AX_U16* pArgbData = NULL;

    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));

    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.nZindex = ptRgn->nZindex;
    tDisp.tChnAttr.bSingleCanvas = AX_FALSE;

    wchar_t wszOsdDate[MAX_OSD_TIME_CHAR_LEN] = {0};
    memset(&wszOsdDate[0], 0, sizeof(wchar_t) * MAX_OSD_TIME_CHAR_LEN);

    AX_S32 nCharLen = 0;
    AX_OSD_DATE_FORMAT_E eFormat = AX_OSD_DATE_FORMAT_YYMMDDHHmmSS;

    if (!OSD_GetCurrDateStr(&wszOsdDate[0], eFormat, &nCharLen)) {
        ALOGE("Failed to get current date string.");
        return;
    }

    if (g_Osd_bAovStatus) {
        nCharLen += swprintf(&wszOsdDate[nCharLen], (MAX_OSD_TIME_CHAR_LEN - nCharLen)*2, L" AOV");
    }

    AX_U32 nPixWidth = ALIGN_UP(nFontSize / 2 * nCharLen, BASE_FONT_SIZE);
    AX_U32 nPixHeight = ALIGN_UP(nFontSize, AX_OSD_ALIGN_HEIGHT);

    OSD_CalcStrSize(wszOsdDate, nFontSize, &nPixWidth, &nPixHeight);

    nPixWidth = ALIGN_UP(nPixWidth + nPicOffset, 8);
    AX_U32 nPicSize = nPixWidth * nPixHeight * 2;
    AX_U32 nFontColor = nRGB;
    nFontColor |= (1 << 24);
    AX_U32 nOffsetX = nSrcOffset + OSD_OverlayOffsetX(nSrcWidth, nPixWidth, (nPicOffset > 0 ? nPicOffsetBlock * AX_OSD_ALIGN_WIDTH : nMarginX), eAlign);
    AX_U32 nOffsetY = OSD_OverlayOffsetY(nSrcHeight, nPixHeight, nMarginY, eAlign);

    tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
    tDisp.arrDisp[0].bShow = ptRgn->bEnable;
    tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = (AX_F32)(nRGB >> 24) / 0xFF * 1024;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = nPixHeight;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = nPixWidth;

    tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;

    /* ARGB1555 */
    {
        pArgbData = (AX_U16*)malloc(nPicSize);
        memset(pArgbData, 0x0, nPicSize);

        if (!OSD_GenStrARGB((wchar_t*)&wszOsdDate[0], pArgbData, nPixWidth, nPixHeight, nPicOffset, 0,
                                nFontSize, AX_TRUE, nFontColor, 0xFFFFFF, 0xFF000000, eAlign)) {
            ALOGE("Failed to generate bitmap for date string.");

            if (pArgbData) {
                free(pArgbData);
                pArgbData = NULL;
            }
            return;
        }

        tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
        tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_ARGB1555;
        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = ALIGN_UP(nOffsetX, AX_OSD_ALIGN_X_OFFSET);
        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = ALIGN_UP(nOffsetY, AX_OSD_ALIGN_Y_OFFSET);
    }

    tDisp.arrDisp[0].uDisp.tOSD.pBitmap = (AX_U8*)pArgbData;

    AX_S32 ret = AX_IVPS_RGN_Update(hRgn, &tDisp);

    if (AX_SUCCESS != ret) {
        ALOGE("AX_IVPS_RGN_Update fail, ret=0x%x, hRgn=%d", ret, hRgn);
    }

    if (pArgbData) {
        free(pArgbData);
        pArgbData = NULL;
    }

    return;
}

static AX_VOID *RgnTimeUpdateThread(AX_VOID *pThreadParam) {
    const AX_U32 timeout_ms = 250; // 250ms
    AX_OSD_REGION_PARAM_T *ptRgn = (AX_OSD_REGION_PARAM_T *)pThreadParam;
    AX_OSD_TIME_ATTR_T *ptTimeAttr = &ptRgn->tTimeAttr;

    ptTimeAttr->bExit = AX_FALSE;

    AX_CHAR szName[50] = {0};
    sprintf(szName, "rgn_%d_%d_%d", ptRgn->nIvpsGrp, ptRgn->nIvpsChn, ptRgn->nVencChn);
    prctl(PR_SET_NAME, szName);

    while (!ptTimeAttr->bExit) {
        {
            pthread_mutex_lock(&ptTimeAttr->mtxUpdate);

            struct timespec tv;
            clock_gettime(CLOCK_MONOTONIC, &tv);
            tv.tv_sec += timeout_ms / 1000;
            tv.tv_nsec += (timeout_ms  % 1000) * 1000000;
            if (tv.tv_nsec >= 1000000000) {
                tv.tv_nsec -= 1000000000;
                tv.tv_sec += 1;
            }

            pthread_cond_timedwait(&ptTimeAttr->cvUpdate, &ptTimeAttr->mtxUpdate, &tv);
            pthread_mutex_unlock(&ptTimeAttr->mtxUpdate);
        }

        if (ptTimeAttr->bExit) {
            break;
        }

        if (g_Osd_bAovStatus
            && !ptTimeAttr->bForceUpdate) {
            continue;
        }

        ptTimeAttr->bForceUpdate = AX_FALSE;

        UpdateOSDTime(ptRgn);
    }

    return (AX_VOID *)0;
}

static AX_S32 RgnUpdateStart() {
    for (AX_S32 i = 0; i < MAX_REGION_COUNT; i++) {
        if (g_rgn_param[i].bEnable
            && g_rgn_param[i].hRgn != AX_IVPS_INVALID_REGION_HANDLE) {
            if (g_rgn_param[i].eOsdType == AX_OSD_TYPE_TIME) {
                UpdateOSDTime(&g_rgn_param[i]);

                if (0 != pthread_create(&g_rgn_param[i].tTimeAttr.nTid, NULL, RgnTimeUpdateThread, (AX_VOID *)&g_rgn_param[i])){
                    return -1;
                }
            } else if (g_rgn_param[i].eOsdType == AX_OSD_TYPE_LOGO) {
                UpdateOSDLogo(&g_rgn_param[i]);
            }
        }
    }

    return 0;
}

static AX_S32 RgnInit(SAMPLE_ENTRY_PARAM_T *pEntryParam) {
    memset(&g_rgn_param[0], 0x00, sizeof(AX_OSD_REGION_PARAM_T));
    for (AX_S32 i = 0; i < MAX_REGION_COUNT; i++) {
        g_rgn_param[i].hRgn = AX_IVPS_INVALID_REGION_HANDLE;
    }

    IVPS_GRP IvpsGrp = 0;
    AX_S32 nChnIdx = 0;
    AX_U32 nRgnIndex = 0;

    for (IvpsGrp = 0; IvpsGrp < pEntryParam->nCamCnt; IvpsGrp++) {
        AX_S16 nZindex = 0;

        for (AX_S32 i = 0; i < IVPSChannelNumber; i++) {
            IVPS_RGN_HANDLE hRgn = AX_IVPS_INVALID_REGION_HANDLE;
            AX_S32 nFilter = ((i + 1) << 4) + 1;
            AX_S32 ret = 0;

            nChnIdx = IvpsGrp * IVPSChannelNumber + i;

            // rect rgn only for venc channel
            if (gOutChnAttr[nChnIdx].bVenc) {
                // rect
                if (nRgnIndex >= MAX_REGION_COUNT) {
                    break;
                }

                hRgn = AX_IVPS_RGN_Create();
                if (AX_IVPS_INVALID_REGION_HANDLE == hRgn) {
                    ALOGE("AX_IVPS_RGN_Create %d failed", nChnIdx);
                    continue;
                }

                ret = AX_IVPS_RGN_AttachToFilter(hRgn, IvpsGrp, nFilter);
                if (ret != 0) {
                    ALOGE("AX_IVPS_RGN_AttachToFilter filter=%d failed. ret=%d", i + 1, ret);
                    AX_IVPS_RGN_Destroy(hRgn);
                } else {
                    g_rgn_param[nRgnIndex].bEnable = AX_TRUE;
                    g_rgn_param[nRgnIndex].nIvpsGrp = IvpsGrp;
                    g_rgn_param[nRgnIndex].nIvpsChn = i;
                    g_rgn_param[nRgnIndex].nVencChn = nChnIdx;
                    g_rgn_param[nRgnIndex].eOsdType = AX_OSD_TYPE_RECT;
                    g_rgn_param[nRgnIndex].hRgn = hRgn;
                    g_rgn_param[nRgnIndex].nFilter = nFilter;
                    g_rgn_param[nRgnIndex].nZindex = nZindex;

                    g_rgn_param[nRgnIndex].tRectAttr.nColor = 0xFFFFFF;
                    g_rgn_param[nRgnIndex].tRectAttr.nLineWidth = 3;

                    // set venc rect rgn
                    gOutChnAttr[nChnIdx].ptRgn = &g_rgn_param[nRgnIndex];

                    nRgnIndex ++;
                    nZindex ++;
                }
            }

            // time
            if (nRgnIndex >= MAX_REGION_COUNT) {
                break;
            }

            hRgn = AX_IVPS_RGN_Create();
            if (AX_IVPS_INVALID_REGION_HANDLE == hRgn) {
                ALOGE("AX_IVPS_RGN_Create %d failed", nChnIdx);
                continue;
            }

            ret = AX_IVPS_RGN_AttachToFilter(hRgn, IvpsGrp, nFilter);
            if (ret != 0) {
                ALOGE("AX_IVPS_RGN_AttachToFilter filter=%d failed. ret=%d", i + 1, ret);
                AX_IVPS_RGN_Destroy(hRgn);
            } else {
                g_rgn_param[nRgnIndex].bEnable = AX_TRUE;
                g_rgn_param[nRgnIndex].nIvpsGrp = IvpsGrp;
                g_rgn_param[nRgnIndex].nIvpsChn = i;
                g_rgn_param[nRgnIndex].nVencChn = nChnIdx;
                g_rgn_param[nRgnIndex].eOsdType = AX_OSD_TYPE_TIME;
                g_rgn_param[nRgnIndex].hRgn = hRgn;
                g_rgn_param[nRgnIndex].nFilter = nFilter;
                g_rgn_param[nRgnIndex].nZindex = nZindex;

                g_rgn_param[nRgnIndex].eAlign = AX_OSD_ALIGN_TYPE_LEFT_TOP;
                g_rgn_param[nRgnIndex].nBoundaryX = 0; // no use, will automatic caculate when update time
                g_rgn_param[nRgnIndex].nBoundaryY = 0; // no use, will automatic caculate when update time
                g_rgn_param[nRgnIndex].tTimeAttr.eFormat = AX_OSD_DATE_FORMAT_YYMMDDHHmmSS;
                g_rgn_param[nRgnIndex].tTimeAttr.nColor = 0xFFFFFF;
                g_rgn_param[nRgnIndex].tTimeAttr.nFontSize = 0; // will automatic caculate when update time

                pthread_mutex_init(&g_rgn_param[nRgnIndex].tTimeAttr.mtxUpdate, NULL);

                pthread_condattr_t cattr;
                ret = pthread_condattr_init(&cattr);
                if (ret != 0) {
                    ALOGE("pthread_condattr_init failed");
                    break;
                }
                pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
                pthread_cond_init(&g_rgn_param[nRgnIndex].tTimeAttr.cvUpdate, &cattr);

                nRgnIndex ++;
                nZindex ++;
            }

            // logo
            if (nRgnIndex >= MAX_REGION_COUNT) {
                break;
            }

            hRgn = AX_IVPS_RGN_Create();

            if (AX_IVPS_INVALID_REGION_HANDLE == hRgn) {
                ALOGE("AX_IVPS_RGN_Create %d failed", nChnIdx);
                continue;
            }

            ret = AX_IVPS_RGN_AttachToFilter(hRgn, IvpsGrp, nFilter);
            if (ret != 0) {
                ALOGE("AX_IVPS_RGN_AttachToFilter filter=%d failed. ret=%d", i + 1, ret);
                AX_IVPS_RGN_Destroy(hRgn);
            } else {
                g_rgn_param[nRgnIndex].bEnable = AX_TRUE;
                g_rgn_param[nRgnIndex].nIvpsGrp = IvpsGrp;
                g_rgn_param[nRgnIndex].nIvpsChn = i;
                g_rgn_param[nRgnIndex].nVencChn = nChnIdx;
                g_rgn_param[nRgnIndex].eOsdType = AX_OSD_TYPE_LOGO;
                g_rgn_param[nRgnIndex].hRgn = hRgn;
                g_rgn_param[nRgnIndex].nFilter = nFilter;
                g_rgn_param[nRgnIndex].nZindex = nZindex;

                g_rgn_param[nRgnIndex].eAlign = AX_OSD_ALIGN_TYPE_RIGHT_BOTTOM;

                if (gOutChnAttr[nChnIdx].nWidth >= 1920) {
                    g_rgn_param[nRgnIndex].nBoundaryX = 32;
                    g_rgn_param[nRgnIndex].nBoundaryY = 48;
                    g_rgn_param[nRgnIndex].tPicAttr.nWidth = 256;
                    g_rgn_param[nRgnIndex].tPicAttr.nHeight = 64;
                    strcpy(g_rgn_param[nRgnIndex].tPicAttr.szFileName, "/customer/qsres/axera_logo_256x64.argb1555");
                } else {
                    g_rgn_param[nRgnIndex].nBoundaryX = 8;
                    g_rgn_param[nRgnIndex].nBoundaryY = 8;
                    g_rgn_param[nRgnIndex].tPicAttr.nWidth = 96;
                    g_rgn_param[nRgnIndex].tPicAttr.nHeight = 28;
                    strcpy(g_rgn_param[nRgnIndex].tPicAttr.szFileName, "/customer/qsres/axera_logo_96x28.argb1555");
                }

                nRgnIndex ++;
                nZindex ++;
            }
        }
    }

    return 0;
}

AX_S32 AX_OSD_Init(SAMPLE_ENTRY_PARAM_T *pEntryParam) {
    AX_BOOL bRet = OSD_Init();

    // rotation: 90
    if (pEntryParam->nVinIvpsMode == AX_GDC_ONLINE_VPP
        && pEntryParam->nGdcOnlineVppTest) {
        g_Osd_nRotation = 1;
    }

    RgnInit(pEntryParam);

    RgnUpdateStart();

    return bRet ? 0 : -1;
}

AX_S32 AX_OSD_DeInit(SAMPLE_ENTRY_PARAM_T *pEntryParam) {
    AX_BOOL bRet = OSD_DeInit();

    for (AX_S32 i = 0; i < MAX_REGION_COUNT; i++) {
        if (g_rgn_param[i].hRgn != AX_IVPS_INVALID_REGION_HANDLE) {
            if (g_rgn_param[i].eOsdType == AX_OSD_TYPE_TIME) {
                pthread_mutex_lock(&g_rgn_param[i].tTimeAttr.mtxUpdate);
                g_rgn_param[i].tTimeAttr.bExit = AX_TRUE;
                pthread_cond_broadcast(&g_rgn_param[i].tTimeAttr.cvUpdate);
                pthread_mutex_unlock(&g_rgn_param[i].tTimeAttr.mtxUpdate);

                pthread_join(g_rgn_param[i].tTimeAttr.nTid, NULL);
                g_rgn_param[i].tTimeAttr.nTid = 0;

                pthread_mutex_destroy(&g_rgn_param[i].tTimeAttr.mtxUpdate);
                pthread_cond_destroy(&g_rgn_param[i].tTimeAttr.cvUpdate);
            }

            IVPS_GRP IvpsGrp = g_rgn_param[i].nIvpsGrp;
            IVPS_RGN_HANDLE hRgn = g_rgn_param[i].hRgn;
            AX_S32 nFilter = g_rgn_param[i].nFilter;
            AX_S32 ret = AX_IVPS_RGN_DetachFromFilter(hRgn, IvpsGrp, nFilter);

            if (ret != 0) {
                ALOGE("AX_IVPS_RGN_DetachFromFilter failed, ret=0x%x.", ret);
                continue;
            }

            ret = AX_IVPS_RGN_Destroy(hRgn);
            if (ret != 0) {
                ALOGE("AX_IVPS_RGN_Destroy hRgn:%d failed, ret=0x%x.", hRgn, ret);
                continue;
            }

            g_rgn_param[i].hRgn = AX_IVPS_INVALID_REGION_HANDLE;

            // clear venc rect rgn
            AX_S32 nChnIdx = g_rgn_param[i].nVencChn;
            gOutChnAttr[nChnIdx].ptRgn = NULL;
        }
    }

    return bRet ? 0 : -1;
}

static AX_S32 UpdateImmediately(AX_VOID) {
    for (AX_S32 i = 0; i < MAX_REGION_COUNT; i++) {
        if (g_rgn_param[i].bEnable
            && g_rgn_param[i].hRgn != AX_IVPS_INVALID_REGION_HANDLE) {
            if (g_rgn_param[i].eOsdType == AX_OSD_TYPE_TIME) {
                pthread_mutex_lock(&g_rgn_param[i].tTimeAttr.mtxUpdate);
                g_rgn_param[i].tTimeAttr.bForceUpdate = AX_TRUE;
                pthread_cond_broadcast(&g_rgn_param[i].tTimeAttr.cvUpdate);
                pthread_mutex_unlock(&g_rgn_param[i].tTimeAttr.mtxUpdate);
            }
        }
    }

    return 0;
}

AX_S32 AX_OSD_UpdateImmediately(AX_VOID) {
    return UpdateImmediately();
}

AX_S32 AX_OSD_DrawRect(AX_SKEL_RESULT_T *pstResult, AX_S32 nIvpsChn, AX_U8 nRotation){
    AX_S32 nChnInx = pstResult->nStreamId*IVPSChannelNumber + nIvpsChn;
    AX_OSD_REGION_PARAM_T *ptRgn = gOutChnAttr[nChnInx].ptRgn;

    if (!ptRgn || ptRgn->hRgn == AX_IVPS_INVALID_REGION_HANDLE) {
        return 0;
    }

    IVPS_RGN_HANDLE hRgn = ptRgn->hRgn;
    AX_S32 obj_num = (pstResult->nObjectSize < AX_IVPS_REGION_MAX_DISP_NUM) ? pstResult->nObjectSize : AX_IVPS_REGION_MAX_DISP_NUM;
    AX_IVPS_RGN_DISP_GROUP_T *rgn_grp = (AX_IVPS_RGN_DISP_GROUP_T *)malloc(sizeof(AX_IVPS_RGN_DISP_GROUP_T));
    memset(rgn_grp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));

    rgn_grp->nNum = obj_num;
    rgn_grp->tChnAttr.nZindex = 0;
    rgn_grp->tChnAttr.bSingleCanvas = AX_TRUE;
    rgn_grp->tChnAttr.nAlpha = 255;
    rgn_grp->tChnAttr.eFormat = AX_FORMAT_RGBA8888;
    AX_IVPS_RGN_DISP_T *disp = rgn_grp->arrDisp;

    for (AX_S32 i = 0; i < obj_num; i++) {
        AX_F32 fDetectWidthRatio = gOutChnAttr[nChnInx].fDetectWidthRatio;
        AX_F32 fDetectHeightRatio = gOutChnAttr[nChnInx].fDetectHeightRatio;
        // rotation: 90 or 270
        if (nRotation == 1 || nRotation == 3) {
            AX_F32 fTmp = fDetectWidthRatio;
            fDetectWidthRatio = fDetectHeightRatio;
            fDetectHeightRatio = fTmp;
        }
        disp[i].bShow = AX_TRUE;
        disp[i].eType = AX_IVPS_RGN_TYPE_RECT;
        disp[i].uDisp.tPolygon.tRect.nX = (AX_S32)(pstResult->pstObjectItems[i].stRect.fX * fDetectWidthRatio);
        disp[i].uDisp.tPolygon.tRect.nY = (AX_S32)(pstResult->pstObjectItems[i].stRect.fY * fDetectHeightRatio);
        disp[i].uDisp.tPolygon.tRect.nW = (AX_S32)(pstResult->pstObjectItems[i].stRect.fW * fDetectWidthRatio);
        disp[i].uDisp.tPolygon.tRect.nH = (AX_S32)(pstResult->pstObjectItems[i].stRect.fH * fDetectHeightRatio);
        if (disp[i].uDisp.tPolygon.tRect.nW == 0 ||
            disp[i].uDisp.tPolygon.tRect.nH == 0) {
            disp[i].bShow = AX_FALSE;
        }
        disp[i].uDisp.tPolygon.nLineWidth = ptRgn->tRectAttr.nLineWidth;
        disp[i].uDisp.tPolygon.bSolid = AX_FALSE;
        disp[i].uDisp.tPolygon.tCornerRect.bEnable = AX_FALSE;
        disp[i].uDisp.tPolygon.nAlpha = 255;
        disp[i].uDisp.tPolygon.nColor = ptRgn->tRectAttr.nColor;
        // {"body", "vehicle", "cycle", "face", "plate"}
        if (pstResult->pstObjectItems[i].pstrObjectCategory) {
            if (strcmp(pstResult->pstObjectItems[i].pstrObjectCategory, "body") == 0) {
                disp[i].uDisp.tPolygon.nColor = 0xFFFFFF;  // white
            } else if (strcmp(pstResult->pstObjectItems[i].pstrObjectCategory, "vehicle") == 0) {
                disp[i].uDisp.tPolygon.nColor = 0x22B14C;  // green
            } else if (strcmp(pstResult->pstObjectItems[i].pstrObjectCategory, "cycle") == 0) {
                disp[i].uDisp.tPolygon.nColor = 0x0080FF;  // blue
            }
        }
    }

    AX_S32 ret = AX_IVPS_RGN_Update(hRgn, rgn_grp);
    if (ret != 0) {
        ALOGE("AX_IVPS_RGN_Update failed, rgn=%d ret=0x%x\n", hRgn, ret);
    }

    free(rgn_grp);

    return 0;
}

AX_S32 AX_OSD_SetAovStatus(AX_BOOL bAov) {
    g_Osd_bAovStatus = bAov;

    // exit from aov, should update osd immediately
    if (!bAov) {
        UpdateImmediately();
    }

    return 0;
}
