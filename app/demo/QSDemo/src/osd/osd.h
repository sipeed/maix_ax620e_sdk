/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _OSD_H_
#define _OSD_H_

#include <stddef.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include "ax_base_type.h"
#include "ax_osd_type.h"

#ifdef FONT_USE_FREETYPE
#include "freetype.h"
#include "ft2build.h"
#include "ftglyph.h"
#endif

AX_S32 OSD_Init(AX_VOID);

AX_S32 OSD_DeInit(AX_VOID);

AX_VOID *OSD_GenStrARGB(wchar_t *pTextStr, AX_U16 *pArgbBuffer, AX_U32 u32OSDWidth, AX_U32 u32OSDHeight, AX_S16 sX, AX_S16 sY,
                                 AX_U16 uFontSize, AX_BOOL bIsBrushSide, AX_U32 uFontColor, AX_U32 uBgColor,
                                 AX_U32 uSideColor, AX_OSD_ALIGN_TYPE_E enAlign);

AX_VOID *OSD_GenStrBitmap(wchar_t *pTextStr, AX_U8 *pBitmapBuffer, AX_U32 u32OSDWidth, AX_U32 u32OSDHeight, AX_S16 sX, AX_S16 sY,
                                   AX_U16 uFontSize, AX_OSD_ALIGN_TYPE_E enAlign);

AX_S32 OSD_CalcStrSize(wchar_t *pTextStr, AX_U16 uFontSize, AX_U32 *u32OSDWidth, AX_U32 *u32OSDHeight);

wchar_t*  OSD_GetCurrDateStr(wchar_t *szOut, AX_OSD_DATE_FORMAT_E eDateFmt, AX_S32 *nOutCharLen);

AX_U32 OSD_GetTimeFontSize(AX_U32 nWidth, AX_U32 nHeight);

AX_U32 OSD_GetFontBoundaryX(AX_U32 nWidth, AX_U32 nHeight);

AX_U32 OSD_GetFontBoundaryY(AX_U32 nWidth, AX_U32 nHeight);

AX_S32 OSD_OverlayOffsetX(AX_S32 nWidth, AX_S32 nOsdWidth, AX_S32 nXMargin, AX_OSD_ALIGN_TYPE_E eAlign);

AX_S32 OSD_OverlayOffsetY(AX_S32 nHeight, AX_S32 nOsdHeight, AX_S32 nYMargin, AX_OSD_ALIGN_TYPE_E eAlign);

// ppVirAddr should be free
AX_S32 OSD_LoadImage(const AX_CHAR* pszImge, AX_VOID** ppVirAddr, AX_U32 nImgSize);

#ifdef FONT_USE_FREETYPE
FT_Bitmap *OSD_FTGetGlpyhBitMap(AX_U16 u16CharCode);
#endif
#endif
