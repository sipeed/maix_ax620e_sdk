/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_hal_osd.h"

#include "ax_ivps_api.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "ax_opal_utils.h"
#include "ax_opal_log.h"

#define LOG_TAG  ("HAL_OSD")

static wchar_t *GetCurrDateStr(wchar_t *szOut, AX_U16 nDateFmt, AX_S32 *nOutCharLen) {
    time_t t;
    struct tm tm;

    t = time(NULL);
    localtime_r(&t, &tm);

    AX_U32 nDateLen = 64;
    AX_OPAL_OSD_DATETIME_FORMAT_E eDateFmt = (AX_OPAL_OSD_DATETIME_FORMAT_E)nDateFmt;
    switch (eDateFmt) {
        case AX_OPAL_OSD_DATETIME_FORMAT_YYMMDD1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_MMDDYY1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d-%02d-%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_DDMMYY1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_YYMMDD2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d年%02d月%02d日", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_MMDDYY2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d月%02d日%04d年", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_DDMMYY2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d日%02d月%04d年", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_YYMMDD3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d/%02d/%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_MMDDYY3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d/%02d/%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_DDMMYY3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_YYMMDDWW1: {
            const wchar_t *wday[] = {L"星期天", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"};
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %ls", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, wday[tm.tm_wday]);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_HHMMSS1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_YYMMDDHHMMSS1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                   tm.tm_hour, tm.tm_min, tm.tm_sec);
            break;
        }
        case AX_OPAL_OSD_DATETIME_FORMAT_YYMMDDHHMMSSWW1: {
            const wchar_t *wday[] = {L"星期天", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"};
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %02d:%02d:%02d %ls", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                   tm.tm_hour, tm.tm_min, tm.tm_sec, wday[tm.tm_wday]);
            break;
        }
        default: {
            *nOutCharLen = 0;
            // ALOGE("Not supported date format: %d.", eDateFmt);
            return NULL;
        }
    }

    return szOut;
}

static AX_S32 OverlayOffsetX(AX_S32 nWidth, AX_S32 nOsdWidth, AX_S32 nXMargin, AX_OPAL_OSD_ALIGN_E eAlign) {
        AX_S32 Offset = 0;
        if (AX_OPAL_OSD_ALIGN_LEFT_TOP == eAlign || AX_OPAL_OSD_ALIGN_LEFT_BOTTOM == eAlign) {
            if (nWidth < nOsdWidth) {
                Offset = nXMargin;
            } else {
                if (nWidth - nOsdWidth > nXMargin) {
                    Offset = nXMargin;
                } else {
                    Offset = nWidth - nOsdWidth;
                }
            }
            Offset = ALIGN_DOWN(Offset, OSD_ALIGN_X_OFFSET);

        } else if (AX_OPAL_OSD_ALIGN_RIGHT_TOP == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM == eAlign) {
            if (nWidth < nOsdWidth) {
                Offset = 0;
            } else {
                if (nWidth - nOsdWidth > nXMargin) {
                    Offset = nWidth - (nOsdWidth + nXMargin) - (OSD_ALIGN_X_OFFSET - 1);
                    if (Offset < 0) {
                        Offset = 0;
                    }
                    Offset = ALIGN_DOWN(Offset, OSD_ALIGN_X_OFFSET);
                } else {
                    Offset = 0;
                }
            }
        }
        if (Offset < 0) {
            Offset = 0;
        }
        return Offset;
    }

static AX_S32 OverlayOffsetY(AX_S32 nHeight, AX_S32 nOsdHeight, AX_S32 nYMargin, AX_OPAL_OSD_ALIGN_E eAlign) {
    AX_S32 Offset = 0;
    if (AX_OPAL_OSD_ALIGN_LEFT_TOP == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_TOP == eAlign) {
        if (nHeight < nOsdHeight) {
            Offset = nYMargin;
        } else {
            if (nHeight - nOsdHeight > nYMargin) {
                Offset = nYMargin;
            } else {
                Offset = nHeight - nOsdHeight;
            }
        }
        Offset = ALIGN_DOWN(Offset, OSD_ALIGN_Y_OFFSET);
    } else if (AX_OPAL_OSD_ALIGN_LEFT_BOTTOM == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM == eAlign) {
        if (nHeight < nOsdHeight) {
            Offset = 0;
        } else {
            if (nHeight - nOsdHeight > nYMargin) {
                Offset = nHeight - (nOsdHeight + nYMargin) - (OSD_ALIGN_Y_OFFSET - 1);
                if (Offset < 0) {
                    Offset = 0;
                }
                Offset = ALIGN_DOWN(Offset, OSD_ALIGN_Y_OFFSET);
            } else {
                Offset = 0;
            }
        }
    }
    if (Offset < 0) {
        Offset = 0;
    }
    return Offset;
}

static AX_BOOL LoadImage(const AX_CHAR* pszImge, AX_VOID** ppVirAddr, AX_U32 nImgSize) {
    if (NULL == pszImge || 0 == nImgSize) {
        LOG_M_E(LOG_TAG, "invailed param");
        return AX_FALSE;
    }

    FILE* fp = fopen(pszImge, "rb");
    if (fp == NULL) {
        LOG_M_E(LOG_TAG, "open image %s failed", pszImge);
        return AX_FALSE;
    }

    fseek(fp, 0, SEEK_END);
    AX_U32 nFileSize = ftell(fp);
    if (nImgSize != nFileSize) {
        LOG_M_E(LOG_TAG, "file %s size not right, %d != %d", pszImge, nImgSize, nFileSize);
        fclose(fp);
        return AX_FALSE;
    }
    fseek(fp, 0, SEEK_SET);
    *ppVirAddr = malloc(nFileSize);
    if (*ppVirAddr == NULL) {
        LOG_M_E(LOG_TAG, "file %s malloc size %d failed", pszImge, nFileSize);
        fclose(fp);
        return AX_FALSE;
    }
    if (fread(*ppVirAddr, 1, nFileSize, fp) != nFileSize) {
        LOG_M_E(LOG_TAG, "file %s fread failed", pszImge);
        fclose(fp);
        return AX_FALSE;
    }

    fclose(fp);
    return AX_TRUE;
}

IVPS_RGN_HANDLE AX_OPAL_HAL_OSD_CreateRgn(AX_S32 nGrpId, AX_S32 nChnId, AX_OPAL_OSD_TYPE_E eType) {
    AX_S32 nRet = AX_SUCCESS;
    IVPS_RGN_HANDLE hRgn = AX_IVPS_INVALID_REGION_HANDLE;
    hRgn = AX_IVPS_RGN_Create();
    if (AX_IVPS_INVALID_REGION_HANDLE != hRgn) {
        AX_S32 nFilter = -1;
        if (eType == AX_OPAL_OSD_TYPE_PRIVACY) {
            nFilter = APP_OSD_GROUP_FILTER_1;   // must be grop filter 1 for gdc online vpp,
        } else {
            nFilter = ((nChnId + 1) << 4) + APP_OSD_CHANNEL_FILTER_1;
        }
        nRet = AX_IVPS_RGN_AttachToFilter(hRgn, nGrpId, nFilter);
        if (AX_SUCCESS != nRet) {
            LOG_M_E(LOG_TAG, "AX_IVPS_RGN_AttachToFilter(Grp: %d, Chn: %d, Filter: 0x%x, Handle: %d) failed, ret=0x%x",
                            nGrpId, nChnId, nFilter, hRgn, nRet);
            AX_IVPS_RGN_Destroy(hRgn);
            hRgn = AX_IVPS_INVALID_REGION_HANDLE;
        }
    }
    return hRgn;
}

AX_S32 AX_OPAL_HAL_OSD_DestoryRgn(AX_S32 nGrpId, AX_S32 nChnId, IVPS_RGN_HANDLE hRgn, AX_OPAL_OSD_TYPE_E eType) {
    AX_S32 nRet = AX_SUCCESS;

    AX_S32 nFilter = -1;
    if (eType == AX_OPAL_OSD_TYPE_PRIVACY) {
        nFilter = APP_OSD_GROUP_FILTER_1;   // must be grop filter 1 for gdc online vpp,
    } else {
        nFilter = ((nChnId + 1) << 4) + APP_OSD_CHANNEL_FILTER_1;
    }

    nRet = AX_IVPS_RGN_DetachFromFilter(hRgn, nGrpId, nFilter);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_RGN_DetachFromFilter(Grp: %d, Chn: %d, Filter: 0x%x, Handle: %d) failed, ret=0x%x",
                    nGrpId, nChnId, nFilter, hRgn, nRet);
        return nRet;
    }

    nRet = AX_IVPS_RGN_Destroy(hRgn);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_RGN_Destroy(Handle: %d) failed, ret=0x%x", hRgn, nRet);
        return nRet;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_OSD_UpdatePrivacy(IVPS_RGN_HANDLE hRgn, const AX_OPAL_OSD_ATTR_T *pOsdCfg, AX_U32 nSrcWidth, AX_U32 nSrcHeight) {

    AX_S32 nRet = 0;

    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));

    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
    tDisp.tChnAttr.nZindex = pOsdCfg->eType;
    tDisp.tChnAttr.bSingleCanvas = AX_TRUE;
    tDisp.arrDisp[0].eType = (AX_IVPS_RGN_TYPE_E)(pOsdCfg->stPrivacyAttr.eType);
    tDisp.arrDisp[0].bShow = pOsdCfg->bEnable;
    if (tDisp.arrDisp[0].eType == AX_IVPS_RGN_TYPE_LINE) {
        tDisp.arrDisp[0].uDisp.tLine.nLineWidth = pOsdCfg->stPrivacyAttr.nLineWidth;
        tDisp.arrDisp[0].uDisp.tLine.nColor = pOsdCfg->nARGB;
        tDisp.arrDisp[0].uDisp.tLine.nAlpha = 255;
        tDisp.arrDisp[0].uDisp.tLine.tPTs[0].nX = pOsdCfg->stPrivacyAttr.stPoints[0].fX;
        tDisp.arrDisp[0].uDisp.tLine.tPTs[0].nY = pOsdCfg->stPrivacyAttr.stPoints[0].fY;
        tDisp.arrDisp[0].uDisp.tLine.tPTs[1].nX = pOsdCfg->stPrivacyAttr.stPoints[1].fX;
        tDisp.arrDisp[0].uDisp.tLine.tPTs[1].nY = pOsdCfg->stPrivacyAttr.stPoints[1].fY;
    } else if (tDisp.arrDisp[0].eType == AX_IVPS_RGN_TYPE_RECT) {
        tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = 0;
        tDisp.arrDisp[0].uDisp.tPolygon.nColor = pOsdCfg->nARGB;
        tDisp.arrDisp[0].uDisp.tPolygon.nAlpha = 255;
        tDisp.arrDisp[0].uDisp.tPolygon.bSolid = pOsdCfg->stPrivacyAttr.bSolid;

        if (pOsdCfg->stPrivacyAttr.bSolid) {
            tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = 0;
        } else {
            tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = pOsdCfg->stPrivacyAttr.nLineWidth;
        }

        tDisp.arrDisp[0].uDisp.tPolygon.tRect.nX = pOsdCfg->stPrivacyAttr.stPoints[0].fX;
        tDisp.arrDisp[0].uDisp.tPolygon.tRect.nY = pOsdCfg->stPrivacyAttr.stPoints[0].fY;
        tDisp.arrDisp[0].uDisp.tPolygon.tRect.nW =
            pOsdCfg->stPrivacyAttr.stPoints[2].fX - pOsdCfg->stPrivacyAttr.stPoints[0].fX;
        tDisp.arrDisp[0].uDisp.tPolygon.tRect.nH =
            pOsdCfg->stPrivacyAttr.stPoints[2].fY - pOsdCfg->stPrivacyAttr.stPoints[0].fY;
    } else if (tDisp.arrDisp[0].eType == AX_IVPS_RGN_TYPE_POLYGON) {
        tDisp.arrDisp[0].uDisp.tPolygon.nColor = pOsdCfg->nARGB;
        tDisp.arrDisp[0].uDisp.tPolygon.nAlpha = 255;
        tDisp.arrDisp[0].uDisp.tPolygon.bSolid = pOsdCfg->stPrivacyAttr.bSolid;
        if (pOsdCfg->stPrivacyAttr.bSolid) {
            tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = 0;
        } else {
            tDisp.arrDisp[0].uDisp.tPolygon.nLineWidth = pOsdCfg->stPrivacyAttr.nLineWidth;
        }

        tDisp.arrDisp[0].uDisp.tPolygon.nPointNum = 4;
        tDisp.arrDisp[0].uDisp.tPolygon.tPTs[0].nX = pOsdCfg->stPrivacyAttr.stPoints[0].fX;
        tDisp.arrDisp[0].uDisp.tPolygon.tPTs[0].nY = pOsdCfg->stPrivacyAttr.stPoints[0].fY;
        tDisp.arrDisp[0].uDisp.tPolygon.tPTs[1].nX = pOsdCfg->stPrivacyAttr.stPoints[1].fX;
        tDisp.arrDisp[0].uDisp.tPolygon.tPTs[1].nY = pOsdCfg->stPrivacyAttr.stPoints[1].fY;
        tDisp.arrDisp[0].uDisp.tPolygon.tPTs[2].nX = pOsdCfg->stPrivacyAttr.stPoints[2].fX;
        tDisp.arrDisp[0].uDisp.tPolygon.tPTs[2].nY = pOsdCfg->stPrivacyAttr.stPoints[2].fY;
        tDisp.arrDisp[0].uDisp.tPolygon.tPTs[3].nX = pOsdCfg->stPrivacyAttr.stPoints[3].fX;
        tDisp.arrDisp[0].uDisp.tPolygon.tPTs[3].nY = pOsdCfg->stPrivacyAttr.stPoints[3].fY;
    } else if (tDisp.arrDisp[0].eType == AX_IVPS_RGN_TYPE_MOSAIC) {
        tDisp.arrDisp[0].uDisp.tMosaic.eBklSize = (AX_IVPS_MOSAIC_BLK_SIZE_E)pOsdCfg->stPrivacyAttr.nLineWidth;
        tDisp.arrDisp[0].uDisp.tMosaic.tRect.nX = pOsdCfg->stPrivacyAttr.stPoints[0].fX;
        tDisp.arrDisp[0].uDisp.tMosaic.tRect.nY = pOsdCfg->stPrivacyAttr.stPoints[0].fY;
        tDisp.arrDisp[0].uDisp.tMosaic.tRect.nW =
            pOsdCfg->stPrivacyAttr.stPoints[2].fX - pOsdCfg->stPrivacyAttr.stPoints[0].fX;
        tDisp.arrDisp[0].uDisp.tMosaic.tRect.nH =
            pOsdCfg->stPrivacyAttr.stPoints[2].fY - pOsdCfg->stPrivacyAttr.stPoints[0].fY;
    }

    /* Region update */
    nRet = AX_IVPS_RGN_Update(hRgn, &tDisp);
    if ((AX_SUCCESS != nRet)) {
        LOG_M_E(LOG_TAG, "AX_IVPS_RGN_Update failed, ret=0x%x, handle=%d", nRet, hRgn);
    }

    return nRet;
}

AX_S32 AX_OPAL_HAL_OSD_UpdateStr(IVPS_RGN_HANDLE hRgn, const AX_OPAL_OSD_ATTR_T* pOsdCfg, AX_U32 nSrcWidth, AX_U32 nSrcHeight) {

    AX_U32 nRGB = pOsdCfg->nARGB;

    AX_U16* pArgbData = NULL;

    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.nZindex = pOsdCfg->eType;
    tDisp.tChnAttr.bSingleCanvas = AX_FALSE;

    /* make wchar string */
    wchar_t wszOsdStr[MAX_OSD_STR_CHAR_LEN] = {0};
    memset(&wszOsdStr[0], 0, sizeof(wchar_t) * MAX_OSD_STR_CHAR_LEN);
    const AX_U8 *pStart = (AX_U8 *)pOsdCfg->stStrAttr.pStr;
    const AX_U8 *pEnd = AX_NULL;
    AX_U32 nCharSize = 0;
    while (nCharSize < MAX_OSD_STR_CHAR_LEN - 1) {
        AX_S32 chOut = utf8_to_ucs2(pStart, &pEnd);
        if (chOut < 0 || pEnd == AX_NULL) {
            break;
        } else {
            wszOsdStr[nCharSize++] = chOut;
            pStart = pEnd;
            pEnd = AX_NULL;
        }
    }

    // AX_U32 nSrcOffsetX = 0; // TODO: mirror
    AX_U32 nFontSize = pOsdCfg->stStrAttr.nFontSize;
    AX_U32 nMarginX = pOsdCfg->nXBoundary;
    AX_U32 nMarginY = pOsdCfg->nYBoundary;
    AX_OPAL_OSD_ALIGN_E eAlign = pOsdCfg->eAlign;

    AX_U32 nPixWidth = ALIGN_UP(nFontSize / 2 * nCharSize, BASE_FONT_SIZE);
    AX_U32 nPixHeight = ALIGN_UP(nFontSize, OSD_ALIGN_HEIGHT);
    CalcStrSize(wszOsdStr, nFontSize, &nPixWidth, &nPixHeight);
    AX_U32 nPicOffset = nMarginX % OSD_ALIGN_WIDTH;
    AX_U32 nPicOffsetBlock = nMarginX / OSD_ALIGN_WIDTH;
    AX_U32 nSrcOffset = 0;

    nPixWidth = ALIGN_UP(nPixWidth + nPicOffset, 8);
    AX_U32 nPicSize = nPixWidth * nPixHeight * 2;
    AX_U32 nFontColor = nRGB;
    nFontColor |= (1 << 24);
    AX_U32 nOffsetX = nSrcOffset + OverlayOffsetX(nSrcWidth, nPixWidth, (nPicOffset > 0 ? nPicOffsetBlock * OSD_ALIGN_WIDTH : nMarginX), eAlign);
    AX_U32 nOffsetY = OverlayOffsetY(nSrcHeight, nPixHeight, nMarginY, eAlign);

    tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
    tDisp.arrDisp[0].bShow = (nCharSize > 0) ? pOsdCfg->bEnable : AX_FALSE;
    tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = (AX_F32)(nRGB >> 24) / 0xFF * 1024;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = nPixHeight;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = nPixWidth;
    tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;

    if (tDisp.arrDisp[0].bShow && pOsdCfg->stStrAttr.bInvEnable) {
        /* Bitmap */
        nPicSize /= 16;
        pArgbData = (AX_U16*)malloc(nPicSize);
        memset(pArgbData, 0x0, nPicSize);

        if (!GenBitmap((wchar_t*)&wszOsdStr[0], (AX_U8*)pArgbData, nPixWidth, nPixHeight, nPicOffset, 0, nFontSize, eAlign)) {
            LOG_M_E(LOG_TAG, "Failed to generate bitmap for ods.");
            if (pArgbData) {
                free(pArgbData);
                pArgbData = NULL;
            }
            return -1;
        }

        tDisp.tChnAttr.eFormat = AX_FORMAT_BITMAP;
        tDisp.arrDisp[0].uDisp.tOSD.u32Color = nRGB;
        tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_BITMAP;
        tDisp.tChnAttr.nBitColor.bColorInv = AX_TRUE;
        tDisp.tChnAttr.nBitColor.nColor = nRGB;
        tDisp.tChnAttr.nBitColor.nColorInv = pOsdCfg->stStrAttr.nColorInv;
        tDisp.tChnAttr.nBitColor.nColorInvThr = 0x808080;

        tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = ALIGN_UP(nPixWidth, OSD_ALIGN_WIDTH);
        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = ALIGN_UP(nOffsetX, OSD_BMP_ALIGN_X_OFFSET);
        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = ALIGN_UP(nOffsetY, OSD_BMP_ALIGN_Y_OFFSET);
    }  else if (tDisp.arrDisp[0].bShow) {
        /* ARGB1555 */
        pArgbData = (AX_U16*)malloc(nPicSize);
        memset(pArgbData, 0x0, nPicSize);
        if (!GenARGB((wchar_t*)&wszOsdStr[0], pArgbData, nPixWidth, nPixHeight, nPicOffset, 0, nFontSize, AX_TRUE, nFontColor, 0xFFFFFF, 0xFF000000, eAlign)) {
            LOG_M_E(LOG_TAG, "Failed to generate argb for osd.");
            if (pArgbData) {
                free(pArgbData);
                pArgbData = NULL;
            }
            return -1;
        }

        tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
        tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_ARGB1555;
        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = ALIGN_UP(nOffsetX, OSD_ALIGN_X_OFFSET);
        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = ALIGN_UP(nOffsetY, OSD_ALIGN_Y_OFFSET);
    }

    tDisp.arrDisp[0].uDisp.tOSD.pBitmap = (AX_U8*)pArgbData;

    AX_S32 nRet = AX_IVPS_RGN_Update(hRgn, &tDisp);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_RGN_Update fail, ret=0x%x, handle=%d", nRet, hRgn);
    }

    if (pArgbData) {
        free(pArgbData);
        pArgbData = NULL;
    }
    return nRet;
}

AX_S32 AX_OPAL_HAL_OSD_UpdateTime(IVPS_RGN_HANDLE hRgn, const AX_OPAL_OSD_ATTR_T *pOsdCfg, AX_U32 nSrcWidth, AX_U32 nSrcHeight) {

    AX_U32 nFontSize = pOsdCfg->stDatetimeAttr.nFontSize;
    nFontSize = ALIGN_UP(nFontSize, BASE_FONT_SIZE);

    AX_U32 nMarginX = pOsdCfg->nXBoundary;
    AX_U32 nMarginY = pOsdCfg->nYBoundary;

    OSD_ALIGN_TYPE_E eAlign = OSD_ALIGN_TYPE_LEFT_TOP;
    AX_U32 nPicOffset = nMarginX % OSD_ALIGN_WIDTH;
    AX_U32 nPicOffsetBlock = nMarginX / OSD_ALIGN_WIDTH;


    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    AX_U16* pArgbData = NULL;

    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
    AX_U32 nRGB = pOsdCfg->nARGB;

    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.nZindex = pOsdCfg->eType;
    tDisp.tChnAttr.bSingleCanvas = AX_FALSE;

    wchar_t wszOsdDate[MAX_OSD_TIME_CHAR_LEN] = {0};
    memset(&wszOsdDate[0], 0, sizeof(wchar_t) * MAX_OSD_TIME_CHAR_LEN);

    AX_S32 nCharLen = 0;
    AX_OPAL_OSD_DATETIME_FORMAT_E eFormat = pOsdCfg->stDatetimeAttr.eFormat;

    if (!GetCurrDateStr(&wszOsdDate[0], eFormat, &nCharLen)) {
        LOG_M_E(LOG_TAG, "Failed to get current date string.");
        return -1;
    }

    AX_U32 nPixWidth = ALIGN_UP(nFontSize / 2 * nCharLen, BASE_FONT_SIZE);
    AX_U32 nPixHeight = ALIGN_UP(nFontSize, OSD_ALIGN_HEIGHT);
    AX_U32 nSrcOffset = 0; // mirror rotation?

    CalcStrSize(wszOsdDate, nFontSize, &nPixWidth, &nPixHeight);

    nPixWidth = ALIGN_UP(nPixWidth + nPicOffset, 8);
    AX_U32 nPicSize = nPixWidth * nPixHeight * 2;
    AX_U32 nFontColor = nRGB;
    nFontColor |= (1 << 24);
    AX_U32 nOffsetX = nSrcOffset + OverlayOffsetX(nSrcWidth, nPixWidth, (nPicOffset > 0 ? nPicOffsetBlock * OSD_ALIGN_WIDTH : nMarginX), eAlign);
    AX_U32 nOffsetY = OverlayOffsetY(nSrcHeight, nPixHeight, nMarginY, eAlign);
    LOG_M_D(LOG_TAG, "PixWidth:%d, nPixHeight:%d, nOffsetX:%d, nOffsetY:%d, nCharLen:%d",
                nPixWidth, nPixHeight, nOffsetX, nOffsetY, nCharLen);

    tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
    tDisp.arrDisp[0].bShow = pOsdCfg->bEnable;
    tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = (AX_F32)(nRGB >> 24) / 0xFF * 1024;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = nPixHeight;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = nPixWidth;

    tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;

    if (tDisp.arrDisp[0].bShow && pOsdCfg->stDatetimeAttr.bInvEnable) {
        /* Bitmap */
        nPicSize /= 16;
        pArgbData = (AX_U16*)malloc(nPicSize);
        memset(pArgbData, 0x0, nPicSize);

        if (!GenBitmap((wchar_t*)&wszOsdDate[0], (AX_U8*)pArgbData, nPixWidth, nPixHeight, nPicOffset, 0, nFontSize, eAlign)) {
            LOG_M_E(LOG_TAG, "Failed to generate bitmap for date string.");
            if (pArgbData) {
                free(pArgbData);
                pArgbData = NULL;
            }
            return -1;
        }

        tDisp.tChnAttr.eFormat = AX_FORMAT_BITMAP;
        tDisp.arrDisp[0].uDisp.tOSD.u32Color = nRGB;
        tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_BITMAP;
        tDisp.tChnAttr.nBitColor.bColorInv = AX_TRUE;
        tDisp.tChnAttr.nBitColor.nColor = nRGB;
        tDisp.tChnAttr.nBitColor.nColorInv = pOsdCfg->stDatetimeAttr.nColorInv;
        tDisp.tChnAttr.nBitColor.nColorInvThr = 0x808080;

        tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = ALIGN_UP(nPixWidth, OSD_ALIGN_WIDTH);
        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = ALIGN_UP(nOffsetX, OSD_BMP_ALIGN_X_OFFSET);
        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = ALIGN_UP(nOffsetY, OSD_BMP_ALIGN_Y_OFFSET);
    }  else if (tDisp.arrDisp[0].bShow) {
        /* ARGB1555 */
        pArgbData = (AX_U16*)malloc(nPicSize);
        memset(pArgbData, 0x0, nPicSize);

        if (!GenARGB((wchar_t*)&wszOsdDate[0], pArgbData, nPixWidth, nPixHeight, nPicOffset, 0, nFontSize, AX_TRUE, nFontColor, 0xFFFFFF, 0xFF000000, eAlign)) {
            LOG_M_E(LOG_TAG, "Failed to generate argb for date string.");
            if (pArgbData) {
                free(pArgbData);
                pArgbData = NULL;
            }
            return -1;
        }

        tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
        tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_ARGB1555;
        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = ALIGN_UP(nOffsetX, OSD_ALIGN_X_OFFSET);
        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = ALIGN_UP(nOffsetY, OSD_ALIGN_Y_OFFSET);
    }

    tDisp.arrDisp[0].uDisp.tOSD.pBitmap = (AX_U8*)pArgbData;

    AX_S32 nRet = AX_IVPS_RGN_Update(hRgn, &tDisp);
    if (AX_SUCCESS != nRet) {
        LOG_M_E(LOG_TAG, "AX_IVPS_RGN_Update fail, ret=0x%x, handle=%d", nRet, hRgn);
    }

    if (pArgbData) {
        free(pArgbData);
        pArgbData = NULL;
    }

    return 0;
}

AX_S32 AX_OPAL_HAL_OSD_UpdatePic(IVPS_RGN_HANDLE hRgn, const AX_OPAL_OSD_ATTR_T* pOsdCfg, AX_U32 nSrcWidth, AX_U32 nSrcHeight) {
    AX_S32 s32Ret = 0;
    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
    tDisp.nNum = 1;
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
    tDisp.tChnAttr.nZindex = pOsdCfg->eType;
    tDisp.tChnAttr.bSingleCanvas = AX_TRUE;

    AX_U32 nSrcOffset = 0;
    AX_U32 nPicWidth = ALIGN_UP(pOsdCfg->stPicAttr.nWidth, OSD_ALIGN_HEIGHT);
    AX_U32 nPicHeight = ALIGN_UP(pOsdCfg->stPicAttr.nHeight, OSD_ALIGN_HEIGHT);

    /* Config picture OSD */
    AX_U32 nPicMarginX = pOsdCfg->nXBoundary;
    AX_U32 nPicMarginY = pOsdCfg->nYBoundary;
    AX_OPAL_OSD_ALIGN_E eAlign = pOsdCfg->eAlign;

    AX_U32 nSrcBlock = nSrcWidth / OSD_ALIGN_X_OFFSET;
    AX_U32 nGap = nSrcWidth % OSD_ALIGN_X_OFFSET;

    AX_U32 nBlockBollowed = ceil((AX_F32)(nPicWidth + nPicMarginX - nGap) / OSD_ALIGN_X_OFFSET);
    if (nBlockBollowed < 0) {
        nBlockBollowed = 0;
    }

    AX_U32 nOffsetX = nSrcOffset + (nSrcBlock - nBlockBollowed) * OSD_ALIGN_X_OFFSET;
    AX_U32 nOffsetY = OverlayOffsetY(nSrcHeight, nPicHeight, nPicMarginY, eAlign);
    if (AX_FALSE ==
        LoadImage(pOsdCfg->stPicAttr.pstrFileName, (AX_VOID**)&tDisp.arrDisp[0].uDisp.tOSD.pBitmap, nPicWidth * nPicHeight * 2)) {
        LOG_M_E(LOG_TAG, "Load logo(%s) failed.", pOsdCfg->stPicAttr.pstrFileName);
        return -1;
    }

    tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
    tDisp.arrDisp[0].bShow = pOsdCfg->bEnable;
    tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_ARGB1555;
    tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = 50;
    tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = nOffsetX;
    tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = nOffsetY;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = nPicWidth;
    tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = nPicHeight;
    s32Ret = AX_IVPS_RGN_Update(hRgn, &tDisp);
    if (AX_SUCCESS != s32Ret) {
        LOG_M_E(LOG_TAG, "AX_IVPS_RGN_Update fail, ret=0x%x, hRgn=%d", s32Ret, hRgn);
    }

    if (tDisp.arrDisp[0].uDisp.tOSD.pBitmap) {
        free(tDisp.arrDisp[0].uDisp.tOSD.pBitmap);
    }
    return s32Ret;
}

AX_S32 AX_OPAL_HAL_OSD_UpdatePolygon(IVPS_RGN_HANDLE hRgn, AX_U32 nRectCount, const AX_IVPS_RGN_POLYGON_T* pstRects,
                                            AX_U32 nPolygonCount, const AX_IVPS_RGN_POLYGON_T* pstPolygons, AX_BOOL bVoRect) {
    AX_S32 s32Ret = 0;

    AX_IVPS_RGN_DISP_GROUP_T tDisp;
    memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
    tDisp.tChnAttr.nAlpha = 255;
    tDisp.tChnAttr.eFormat = AX_FORMAT_ARGB1555;
    tDisp.tChnAttr.nZindex = AX_OPAL_OSD_TYPE_BUTT;
    tDisp.tChnAttr.bSingleCanvas = AX_TRUE;
    tDisp.tChnAttr.bVoRect = bVoRect;

    AX_U32 index = 0;
    // rect
    for (AX_U32 i = 0; i < nRectCount && index < AX_IVPS_REGION_MAX_DISP_NUM; ++i) {
        if (pstRects[i].tRect.nW != 0 && pstRects[i].tRect.nH != 0) {
            tDisp.arrDisp[index].eType = AX_IVPS_RGN_TYPE_RECT;
            tDisp.arrDisp[index].bShow = AX_TRUE;
            tDisp.arrDisp[index].uDisp.tPolygon = pstRects[i];
            index++;
        }
    }

    // polygon
    for (AX_U32 i = 0; i < nPolygonCount && index < AX_IVPS_REGION_MAX_DISP_NUM; ++i) {
        tDisp.arrDisp[index].eType = AX_IVPS_RGN_TYPE_POLYGON;
        tDisp.arrDisp[index].bShow = AX_TRUE;
        tDisp.arrDisp[index].uDisp.tPolygon = pstPolygons[i];
        index++;
    }
    tDisp.nNum = index;

    s32Ret = AX_IVPS_RGN_Update(hRgn, &tDisp);
    if (AX_SUCCESS != s32Ret && s32Ret != 0x800d0229) {
        LOG_M_E(LOG_TAG, "AX_IVPS_RGN_Update failed, ret=0x%x", s32Ret);
    }

    return s32Ret;
}