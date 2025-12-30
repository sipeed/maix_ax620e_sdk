/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_osd_utils.h"

typedef struct _OSD_FONT_STYLE {
    AX_U32 nMagic;
    AX_U32 nTimeFontSize;
    AX_U32 nRectLineWidth;
    AX_U32 nBoundaryX;
} OSD_FONT_STYLE;

/* must be ordered as nMagic */
const static OSD_FONT_STYLE g_arrOsdStyle[] = {
    {3840 * 2160, 128, 4, 56}, {3072 * 2048, 128, 4, 56}, {3072 * 1728, 128, 4, 48}, {2624 * 1944, 96, 4, 48}, {2688 * 1520, 96, 4, 48},
    {2048 * 1536, 96, 4, 48},  {2304 * 1296, 96, 4, 48},  {1920 * 1080, 48, 2, 24},  {1280 * 720, 48, 2, 20},  {1024 * 768, 32, 2, 16},
    {720 * 576, 32, 2, 12},    {704 * 576, 32, 2, 12},    {640 * 480, 16, 2, 8},     {384 * 288, 16, 2, 2}};


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


AX_U32 GetTimeFontSize(AX_U32 nWidth, AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nWidth, nHeight);
    return g_arrOsdStyle[nIndex].nTimeFontSize;
}

AX_U32 GetRectLineWidth(AX_U32 nWidth, AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nWidth, nHeight);
    return g_arrOsdStyle[nIndex].nRectLineWidth;
}

AX_U32 GetBoundaryX(AX_U32 nWidth, AX_U32 nHeight) {
    AX_U32 nIndex = GetOsdStyleIndex(nWidth, nHeight);
    return g_arrOsdStyle[nIndex].nBoundaryX;
}

AX_S32 CalcOsdOffsetX(AX_S32 nImgWidth, AX_S32 nOsdWidth, AX_S32 nXMargin, AX_OPAL_OSD_ALIGN_E eAlign) {
    AX_S32 Offset = 0;
    if (AX_OPAL_OSD_ALIGN_LEFT_TOP == eAlign || AX_OPAL_OSD_ALIGN_LEFT_BOTTOM == eAlign) {
        if (nImgWidth < nOsdWidth) {
            Offset = nXMargin;
        } else {
            if (nImgWidth - nOsdWidth > nXMargin) {
                Offset = nXMargin;
            } else {
                Offset = nImgWidth - nOsdWidth;
            }
        }
        Offset = ALIGN_UP(Offset, OSD_ALIGN_X_OFFSET);

    } else if (AX_OPAL_OSD_ALIGN_RIGHT_TOP == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM == eAlign) {
        if (nImgWidth < nOsdWidth) {
            Offset = 0;
        } else {
            if (nImgWidth - nOsdWidth > nXMargin) {
                Offset = nImgWidth - (nOsdWidth + nXMargin) - (OSD_ALIGN_X_OFFSET - 1);
                if (Offset < 0) {
                    Offset = 0;
                }
                Offset = ALIGN_UP(Offset, OSD_ALIGN_X_OFFSET);
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
AX_S32 CalcOsdOffsetY(AX_S32 nImgHeight, AX_S32 nOsdHeight, AX_S32 nYMargin, AX_OPAL_OSD_ALIGN_E eAlign) {
    AX_S32 Offset = 0;
    if (AX_OPAL_OSD_ALIGN_LEFT_TOP == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_TOP == eAlign) {
        if (nImgHeight < nOsdHeight) {
            Offset = nYMargin;
        } else {
            if (nImgHeight - nOsdHeight > nYMargin) {
                Offset = nYMargin;
            } else {
                Offset = nImgHeight - nOsdHeight;
            }
        }
        Offset = ALIGN_UP(Offset, OSD_ALIGN_Y_OFFSET);
    } else if (AX_OPAL_OSD_ALIGN_LEFT_BOTTOM == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM == eAlign) {
        if (nImgHeight < nOsdHeight) {
            Offset = 0;
        } else {
            if (nImgHeight - nOsdHeight > nYMargin) {
                Offset = nImgHeight - (nOsdHeight + nYMargin) - (OSD_ALIGN_Y_OFFSET - 1);
                if (Offset < 0) {
                    Offset = 0;
                }
                Offset = ALIGN_UP(Offset, OSD_ALIGN_Y_OFFSET);
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

AX_S32 CalcOsdBoudingX(AX_S32 nImgWidth, AX_S32 nOsdWidth, AX_S32 nBoudingX, AX_OPAL_OSD_ALIGN_E eAlign) {
    AX_S32 x = 0;
    if (AX_OPAL_OSD_ALIGN_LEFT_TOP == eAlign || AX_OPAL_OSD_ALIGN_LEFT_BOTTOM == eAlign) {
        if (nImgWidth < nOsdWidth) {
            x = nBoudingX;
        } else {
            if (nImgWidth - nOsdWidth > nBoudingX) {
                x = nBoudingX;
            } else {
                x = nImgWidth - nOsdWidth;
            }
        }
    } else if (AX_OPAL_OSD_ALIGN_RIGHT_TOP == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM == eAlign) {
        if (nImgWidth < nOsdWidth) {
            x = 0;
        } else {
            if (nImgWidth - nOsdWidth > nBoudingX) {
                x = nImgWidth - (nOsdWidth + nBoudingX);
            } else {
                x = 0;
            }
        }
    }
    if (x < 0) {
        x = 0;
    }
    return x;
}

AX_S32 CalcOsdBoudingY(AX_S32 nImgHeight, AX_S32 nOsdHeight, AX_S32 nBoudingY, AX_OPAL_OSD_ALIGN_E eAlign) {
    AX_S32 y = 0;
    if (AX_OPAL_OSD_ALIGN_LEFT_TOP == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_TOP == eAlign) {
        if (nImgHeight < nOsdHeight) {
            y = nBoudingY;
        } else {
            if (nImgHeight - nOsdHeight > nBoudingY) {
                y = nBoudingY;
            } else {
                y = nImgHeight - nOsdHeight;
            }
        }
    } else if (AX_OPAL_OSD_ALIGN_LEFT_BOTTOM == eAlign || AX_OPAL_OSD_ALIGN_RIGHT_BOTTOM == eAlign) {
        if (nImgHeight < nOsdHeight) {
            y = 0;
        } else {
            if (nImgHeight - nOsdHeight > nBoudingY) {
                y = nImgHeight - (nOsdHeight + nBoudingY);
            } else {
                y = 0;
            }
        }
    }
    if (y < 0) {
        y = 0;
    }
    return y;
}



