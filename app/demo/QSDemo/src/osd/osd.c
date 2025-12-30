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
#include "osd.h"
#include "osd_font.h"
#include "qs_log.h"

#define OSD_TTFFILE_STR "/customer/qsres/GB2312.ttf"

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) (((x) / (align)) * (align))
#endif

typedef struct _OSD_FONT_STYLE {
    AX_U32 nMagic;
    AX_U32 nTimeFontSize;
    AX_U32 nRectLineWidth;
    AX_U32 nBoundaryX;
    AX_U32 nBoundaryY;
} OSD_FONT_STYLE;

/* must be ordered as nMagic */
const static OSD_FONT_STYLE g_arrOsdStyle[] = {
    {3840 * 2160, 128, 4, 56, 8}, {3072 * 2048, 128, 4, 56, 8}, {3072 * 1728, 128, 4, 48, 8}, {2624 * 1944, 96, 4, 48, 8}, {2688 * 1520, 96, 4, 48, 8},
    {2048 * 1536, 96, 4, 48, 8},  {2304 * 1296, 96, 4, 48, 8},  {1920 * 1080, 48, 2, 24, 8},  {1280 * 720, 32, 2, 20, 8},  {1024 * 768, 24, 2, 16, 8},
    {720 * 576, 16, 2, 12, 8},    {704 * 576, 16, 2, 12, 8},    {640 * 480, 16, 2, 8, 8},     {384 * 288, 16, 2, 2, 8}};

AX_S32 OSD_Init(AX_VOID) {
    AX_BOOL bRet = OSDFONT_Init(OSD_TTFFILE_STR);

    return bRet ? 0 : -1;
}

AX_S32 OSD_DeInit(AX_VOID) {
    AX_BOOL bRet = OSDFONT_DeInit();

    return bRet ? 0 : -1;
}

AX_VOID *OSD_GenStrARGB(wchar_t *pTextStr, AX_U16 *pArgbBuffer, AX_U32 u32OSDWidth, AX_U32 u32OSDHeight, AX_S16 sX, AX_S16 sY,
                                 AX_U16 uFontSize, AX_BOOL bIsBrushSide, AX_U32 uFontColor, AX_U32 uBgColor,
                                 AX_U32 uSideColor, AX_OSD_ALIGN_TYPE_E enAlign) {
    AX_VOID *pArgb = OSDFONT_GenARGB(pTextStr, pArgbBuffer, u32OSDWidth, u32OSDHeight, sX, sY,
                                    uFontSize, bIsBrushSide, uFontColor, uBgColor, uSideColor, (OSDFONT_ALIGN_TYPE_E)enAlign);

    return pArgb;
}

AX_VOID *OSD_GenStrBitmap(wchar_t *pTextStr, AX_U8 *pBitmapBuffer, AX_U32 u32OSDWidth, AX_U32 u32OSDHeight, AX_S16 sX, AX_S16 sY,
                                   AX_U16 uFontSize, AX_OSD_ALIGN_TYPE_E enAlign) {
    AX_VOID *pBitmap = OSDFONT_GenBitmap(pTextStr, pBitmapBuffer, u32OSDWidth, u32OSDHeight, sX, sY, uFontSize, (OSDFONT_ALIGN_TYPE_E)enAlign);

    return pBitmap;
}

AX_S32 OSD_CalcStrSize(wchar_t *pTextStr, AX_U16 uFontSize, AX_U32 *u32OSDWidth, AX_U32 *u32OSDHeight) {
    return OSDFONT_CalcStrSize(pTextStr, uFontSize, u32OSDWidth, u32OSDHeight);
}

wchar_t* OSD_GetCurrDateStr(wchar_t *szOut, AX_OSD_DATE_FORMAT_E eDateFmt, AX_S32 *nOutCharLen) {
    time_t t;
    struct tm tm;

    if (!szOut || !nOutCharLen) {
        return NULL;
    }

    t = time(NULL);
    localtime_r(&t, &tm);

    AX_U32 nDateLen = MAX_OSD_TIME_CHAR_LEN*2;

    switch (eDateFmt) {
        case AX_OSD_DATE_FORMAT_YYMMDD1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case AX_OSD_DATE_FORMAT_MMDDYY1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d-%02d-%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case AX_OSD_DATE_FORMAT_DDMMYY1: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case AX_OSD_DATE_FORMAT_YYMMDD2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d年%02d月%02d日", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case AX_OSD_DATE_FORMAT_MMDDYY2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d月%02d日%04d年", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case AX_OSD_DATE_FORMAT_DDMMYY2: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d日%02d月%04d年", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case AX_OSD_DATE_FORMAT_YYMMDD3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d/%02d/%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            break;
        }
        case AX_OSD_DATE_FORMAT_MMDDYY3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d/%02d/%04d", tm.tm_mon + 1, tm.tm_mday, tm.tm_year + 1900);
            break;
        }
        case AX_OSD_DATE_FORMAT_DDMMYY3: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            break;
        }
        case AX_OSD_DATE_FORMAT_YYMMDDWW1: {
            const wchar_t *wday[] = {L"星期天", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"};
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %ls", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, wday[tm.tm_wday]);
            break;
        }
        case AX_OSD_DATE_FORMAT_HHmmSS: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
            break;
        }
        case AX_OSD_DATE_FORMAT_YYMMDDHHmmSS: {
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                   tm.tm_hour, tm.tm_min, tm.tm_sec);
            break;
        }
        case AX_OSD_DATE_FORMAT_YYMMDDHHmmSSWW: {
            const wchar_t *wday[] = {L"星期天", L"星期一", L"星期二", L"星期三", L"星期四", L"星期五", L"星期六"};
            *nOutCharLen = swprintf(szOut, nDateLen, L"%04d-%02d-%02d %02d:%02d:%02d %ls", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                                   tm.tm_hour, tm.tm_min, tm.tm_sec, wday[tm.tm_wday]);
            break;
        }
        default: {
            *nOutCharLen = 0;
            ALOGE("Not supported date format: %d.", eDateFmt);
            return NULL;
        }
    }

    return szOut;
}

static AX_U32 GetOsdStyleIndex(AX_U32 nWidth, AX_U32 nHeight) {
    AX_U32 nMagic = nWidth * nHeight;
    AX_U32 nCount = sizeof(g_arrOsdStyle) / sizeof(g_arrOsdStyle[0]);
    for (AX_U32 i = 0; i < nCount; i++) {
        if (nMagic >= g_arrOsdStyle[i].nMagic) {
            return i;
        }
    }
    return nCount - 1;
}

AX_U32 OSD_GetTimeFontSize(AX_U32 nWidth, AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nWidth, nHeight);

    return g_arrOsdStyle[nIndex].nTimeFontSize;
}

AX_U32 OSD_GetFontBoundaryX(AX_U32 nWidth, AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nWidth, nHeight);

    return g_arrOsdStyle[nIndex].nBoundaryX;
}

AX_U32 OSD_GetFontBoundaryY(AX_U32 nWidth, AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nWidth, nHeight);

    return g_arrOsdStyle[nIndex].nBoundaryY;
}

AX_S32 OSD_OverlayOffsetX(AX_S32 nWidth, AX_S32 nOsdWidth, AX_S32 nXMargin, AX_OSD_ALIGN_TYPE_E eAlign) {
    AX_S32 Offset = 0;

    if (AX_OSD_ALIGN_TYPE_LEFT_TOP == eAlign || AX_OSD_ALIGN_TYPE_LEFT_BOTTOM == eAlign) {
        if (nWidth < nOsdWidth) {
            Offset = nXMargin;
        } else {
            if (nWidth - nOsdWidth > nXMargin) {
                Offset = nXMargin;
            } else {
                Offset = nWidth - nOsdWidth;
            }
        }
        Offset = ALIGN_DOWN(Offset, AX_OSD_ALIGN_X_OFFSET);

    } else if (AX_OSD_ALIGN_TYPE_RIGHT_TOP == eAlign || AX_OSD_ALIGN_TYPE_RIGHT_BOTTOM == eAlign) {
        if (nWidth < nOsdWidth) {
            Offset = 0;
        } else {
            if (nWidth - nOsdWidth > nXMargin) {
                Offset = nWidth - (nOsdWidth + nXMargin) - (AX_OSD_ALIGN_X_OFFSET - 1);
                if (Offset < 0) {
                    Offset = 0;
                }
                Offset = ALIGN_DOWN(Offset, AX_OSD_ALIGN_X_OFFSET);
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

AX_S32 OSD_OverlayOffsetY(AX_S32 nHeight, AX_S32 nOsdHeight, AX_S32 nYMargin, AX_OSD_ALIGN_TYPE_E eAlign) {
    AX_S32 Offset = 0;

    if (AX_OSD_ALIGN_TYPE_LEFT_TOP == eAlign || AX_OSD_ALIGN_TYPE_RIGHT_TOP == eAlign) {
        if (nHeight < nOsdHeight) {
            Offset = nYMargin;
        } else {
            if (nHeight - nOsdHeight > nYMargin) {
                Offset = nYMargin;
            } else {
                Offset = nHeight - nOsdHeight;
            }
        }
        Offset = ALIGN_DOWN(Offset, AX_OSD_ALIGN_Y_OFFSET);
    } else if (AX_OSD_ALIGN_TYPE_LEFT_BOTTOM == eAlign || AX_OSD_ALIGN_TYPE_RIGHT_BOTTOM == eAlign) {
        if (nHeight < nOsdHeight) {
            Offset = 0;
        } else {
            if (nHeight - nOsdHeight > nYMargin) {
                Offset = nHeight - (nOsdHeight + nYMargin) - (AX_OSD_ALIGN_Y_OFFSET - 1);
                if (Offset < 0) {
                    Offset = 0;
                }
                Offset = ALIGN_DOWN(Offset, AX_OSD_ALIGN_Y_OFFSET);
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

AX_S32 OSD_LoadImage(const AX_CHAR* pszImge, AX_VOID** ppVirAddr, AX_U32 nImgSize) {
    if (!pszImge || 0 == nImgSize) {
        ALOGE("invailed param");
        return -1;
    }

    FILE* fp = fopen(pszImge, "rb");

    if (fp) {
        fseek(fp, 0, SEEK_END);

        AX_U32 nFileSize = ftell(fp);

        if (nImgSize != nFileSize) {
            ALOGE("file size not right, %d != %d", nImgSize, nFileSize);
            fclose(fp);
            return -1;
        }

        fseek(fp, 0, SEEK_SET);
        *ppVirAddr = malloc(nFileSize);

        if (! *ppVirAddr) {
            ALOGE("malloc failed");
            fclose(fp);
            return -1;
        }
        if (fread(*ppVirAddr, 1, nFileSize, fp) != nFileSize) {
            ALOGE("fread fail, %s", strerror(errno));
            fclose(fp);
            return -1;
        }

        fclose(fp);
        return 0;
    } else {
        ALOGE("fopen %s fail, %s", pszImge, strerror(errno));
        return -1;
    }
}

#ifdef FONT_USE_FREETYPE
FT_Bitmap *OSD_FTGetGlpyhBitMap(AX_U16 u16CharCode) {
    return OSDFONT_FTGetGlpyhBitMap(u16CharCode);
}
#endif
