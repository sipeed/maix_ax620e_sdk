/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AXEAR_VO_COMMON_H__
#define __AXEAR_VO_COMMON_H__

#include <common.h>
#include <asm/io.h>

/* #define VO_LOG_ON */
#ifdef VO_LOG_ON
#define VO_INFO(fmt,...) printf("[VO][I][%s:%d] "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define VO_DEBUG(fmt,...) printf("[VO][D][%s:%d] "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define VO_INFO(fmt,...)
#define VO_DEBUG(fmt,...)
#endif

#define VO_WARN(fmt,...) printf("[VO][W][%s:%d] "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define VO_ERROR(fmt,...) printf("[VO][E][%s:%d] "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);

enum {
	AX_DISP_OUT_FMT_RGB565 = 0,
	AX_DISP_OUT_FMT_RGB666,
	AX_DISP_OUT_FMT_RGB666LP,
	AX_DISP_OUT_FMT_RGB888,
	AX_DISP_OUT_FMT_RGB101010,
	AX_DISP_OUT_FMT_YUV422,
	AX_DISP_OUT_FMT_YUV422_10,
};

enum {
	AX_VO_FORMAT_NV12,
	AX_VO_FORMAT_NV21,
	AX_VO_FORMAT_ARGB1555,
	AX_VO_FORMAT_ARGB4444,
	AX_VO_FORMAT_RGBA5658,
	AX_VO_FORMAT_ARGB8888,
	AX_VO_FORMAT_RGB565,
	AX_VO_FORMAT_RGB888,
	AX_VO_FORMAT_BGR565,
	AX_VO_FORMAT_BGR888,
	AX_VO_FORMAT_RGBA4444,
	AX_VO_FORMAT_RGBA5551,
	AX_VO_FORMAT_RGBA8888,
	AX_VO_FORMAT_ARGB8565,
	AX_VO_FORMAT_P010,
	AX_VO_FORMAT_P016,
	AX_VO_FORMAT_NV16,
	AX_VO_FORMAT_P210,
	AX_VO_FORMAT_P216,
	AX_VO_FORMAT_BITMAP,
	AX_VO_FORMAT_BUT,
};

enum {
	AX_VO_OUTPUT_576P50,                /* 720  x  576 at 50 Hz */
	AX_VO_OUTPUT_480P60,                /* 720  x  480 at 60 Hz */
	AX_VO_OUTPUT_720P25,                /* 1280 x  720 at 25 Hz */
	AX_VO_OUTPUT_720P30,                /* 1280 x  720 at 30 Hz */
	AX_VO_OUTPUT_1080P25,               /* 1920 x 1080 at 25 Hz. */
	AX_VO_OUTPUT_1080P30,               /* 1920 x 1080 at 30 Hz. */
	AX_VO_OUTPUT_1080P60,               /* 1920 x 1080 at 60 Hz. */
	AX_VO_OUTPUT_800_480_60,            /* 800 x 480 at 60 Hz. */
	AX_VO_OUTPUT_1080x1920_60,            /* 1080 x 1920 at 60 Hz. */
// ### SIPEED EDIT ###
	AX_VO_OUTPUT_480x640_60,			/* 480 x 640 at 60 Hz. */
// ### SIPEED EDIT END ###
	AX_VO_OUTPUT_BUTT
};

#define MODE_FLAG_INTERLACE			(1 << 0)	/* 1: interlace */
#define MODE_FLAG_SYNC_TYPE			(1 << 1)	/* 0: internal sync. 1: external sync */
#define MODE_FLAG_FIELD1_FIRST			(1 << 2)

struct ax_disp_mode {
	int type;
	int fmt_in;
	int fmt_out;
	int flags;

	int clock;  /* in kHz */
	int vrefresh;
	int hdisplay;
	int hsync_start;
	int hsync_end;
	int htotal;
	int vdisplay;
	int vsync_start;
	int vsync_end;
	int vtotal;

	int hp_pol;
	int vp_pol;
	int de_pol;
};

static inline void dpu_writel(void __iomem *regs, u32 offset, u32 val)
{
	__raw_writel(val, regs + offset);
}

static inline u32 dpu_readl(void __iomem *regs, u32 offset)
{
	return __raw_readl(regs + offset);
}

#endif

