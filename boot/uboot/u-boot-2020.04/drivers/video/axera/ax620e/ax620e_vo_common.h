/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX620E_VO_COMMON_H
#define __AX620E_VO_COMMON_H

#include <common.h>

#define PYHS_ADDR_HIGH(x)	(((x) >> 32) & 0x1F)
#define PYHS_ADDR_LOW(x)	((x) & 0xFFFFFFFF)

#define LAYER_TYPE(x)		((x) << 7)
#define RB_SWAP(x)		((x) << 6)
#define FBDC_EN(x)		((x) << 5)
#define PIXEL_FMT(x)		((x) & 0x1F)

#define SUPPORTED_CHNS_MAX	1
#define SUPPORTED_LAYER_MAX	1
#define DRAW_V0_REG_NUM		40
#define DRAW_G0_REG_NUM		46
#define DRAW_OTHER_REG_NUM	32
#define DRAW_CDMA_EXTRA_SIZE	(((SUPPORTED_CHNS_MAX * 16) + 0xFF) & (~0xFF))
#define DRAW_CDMA_QUEUE_H_SIZE	(128)	/* Related to cdma-mode0 format */
#define DRAW_MULTI_CFG_IOMEM_SIZE (DRAW_CDMA_EXTRA_SIZE + DRAW_CDMA_QUEUE_H_SIZE + (DRAW_V0_REG_NUM + DRAW_G0_REG_NUM + DRAW_OTHER_REG_NUM) * 8)
#define DRAW_SINGLE_CFG_IOMEM_SIZE ((SUPPORTED_CHNS_MAX * (DRAW_CDMA_QUEUE_H_SIZE + DRAW_G0_REG_NUM + DRAW_OTHER_REG_NUM)) * 8 + DRAW_CDMA_EXTRA_SIZE)


#define DRAW_ADDR_ALIGN			(256)
#define DRAW_ALIGNED_BYTES		(8)
#define DRAW_FBCDC_ALIGNED_PIXEL	(128)
#define DRAW_WIDTH_MIN			(2)
#define DRAW_HEIGHT_MIN			(2)
#define DRAW_WIDTH_MAX			(0x2000)
#define DRAW_HEIGHT_MAX			(0x2000)
#define DRAW_STRIDE_MAX			(0x10000)

#define COLORKEY_VAL_FIX_POINT_SHIFT	(2)
#define SET_HW_COLORKEY_VAL(val)    (((((val >> 16) & 0xFF) << COLORKEY_VAL_FIX_POINT_SHIFT) << 20) \
				  | ((((val >> 8) & 0xFF) << COLORKEY_VAL_FIX_POINT_SHIFT) << 10) \
				  | ((val & 0xFF) << COLORKEY_VAL_FIX_POINT_SHIFT))


#define VO_RGB2Y(R, G, B)  ((299 * R + 587 * G + 114 * B) / 1000)
#define VO_RGB2U(R, G, B)  (((-169 * R - 331 * G + 500 * B) / 1000))
#define VO_RGB2V(R, G, B)  ((((500* R - 419 *G - 81* B) / 1000)))

/* u2.16, 1/360 value */
#define DISPC_INV_HUE_MAX		(182)
/* u5.18, 1/1.0 value */
#define DISPC_INV_SATU_MAX		(262144)
#define HSV_H_LUT			(33)
#define HSV_V_LUT			(33)
#define HSV_LUT_H_ACC			(7)
#define HSV_LUT_S_ACC			(15)

#define DPU_CLK_RATE			(533333333)

#define DISPC_RESO_HEIGHT_SHIFT		(16)
#define DISPC_H_SHIFT			(16)

#define HW_FMT_EXTRA(fmt) ((fmt) & 0x1F)

enum {
	FORMAT_ARGB1555 = 0,
	FORMAT_ARGB4444 = 1,
	FORMAT_ARGB8565 = 2,
	FORMAT_ARGB8888 = 3,
	FORMAT_RGBA5551 = 4,
	FORMAT_RGBA4444 = 5,
	FORMAT_RGBA5658 = 6,
	FORMAT_RGBA8888 = 7,
	FORMAT_RGB565 = 8,
	FORMAT_RGB888 = 9,
	FORMAT_YUV420_8 = 10,
	FORMAT_YUV422_8 = 13,
	FORMAT_SINGLE_Y = 16,
	FORMAT_YUYV_8 = 19,

	FORMAT_BIT_MAP = 31,
	FORMAT_NO_SUPPORT,
};

enum {
	VO_COMPRESS_LEVEL_0,	/* lossless */
	VO_COMPRESS_LEVEL_1,
	VO_COMPRESS_LEVEL_2,
	VO_COMPRESS_LEVEL_3,
	VO_COMPRESS_LEVEL_4,
	VO_COMPRESS_LEVEL_5,
	VO_COMPRESS_LEVEL_6,
	VO_COMPRESS_LEVEL_7,
	VO_COMPRESS_LEVEL_BUTT,
};

struct dispc_dither {
	u32 dither_r_seed;
	u32 dither_r_pmask;
	u32 dither_r_acc;
	u32 dither_g_seed;
	u32 dither_g_pmask;
	u32 dither_g_acc;
	u32 dither_b_seed;
	u32 dither_b_pmask;
	u32 dither_b_acc;
};

struct yuv2rgb_regs {
	u16 matrix[3][3];
	u16 offset[2][3];
};

struct csc1_reg {
	u32 en;
	u32 c_offbin_en;
};

struct csc_reg {
	struct yuv2rgb_regs csc0;
	struct csc1_reg csc1;
};

struct rgb2yuv_regs {
	u16 matrix[3][3];
	u16 offset[2][3];
	u16 decimation_h[7];
	u8 uv_seq_sel;
	u8 uv_offbin_en;
};

enum out_mode {
	OUT_MODE_BT601 = 0,
	OUT_MODE_BT656,
	OUT_MODE_BT1120,
	OUT_MODE_DPI,
	OUT_MODE_DSI_DPI_VIDEO,
	OUT_MODE_DSI_SDI_VIDEO,
	OUT_MODE_DSI_SDI_CMD,
	OUT_MODE_LVDS,
};

enum dispc_format_in {
	FMT_IN_ARGB1555 = 0,
	FMT_IN_ARGB4444 = 1,
	FMT_IN_ARGB8565 = 2,
	FMT_IN_ARGB8888 = 3,
	FMT_IN_RGBA5551 = 4,
	FMT_IN_RGBA4444 = 5,
	FMT_IN_RGBA5658 = 6,
	FMT_IN_RGBA8888 = 7,
	FMT_IN_RGB565 = 8,
	FMT_IN_RGB888 = 9,
	FMT_IN_YUV420_8 = 10,
	FMT_IN_YUV422_8 = 13,
	FMT_IN_SINGLEY_8 = 16,
	FMT_IN_YUYV_8 = 19,
};

enum dispc_format_out {
	FMT_OUT_RGB565 = 0,
	FMT_OUT_RGB666 = 1,
	FMT_OUT_RGB666LP = 2,
	FMT_OUT_RGB888 = 3,
	FMT_OUT_YUV422 = 5,
};

enum dispc_dither_acc {
	DITHER_ACC_8BIT = 0,
	DITHER_ACC_6BIT = 1,
	DITHER_ACC_5BIT = 2,
	DITHER_ACC_4BIT = 3,
};

enum dispc_scan_mode {
	DISPC_SCAN_MODE_FRAME = 0,
	DISPC_SCAN_MODE_FIELD = 1,
};

struct dispc_input_mode {
	u32 fmt_in;
	const struct rgb2yuv_regs *rgb2yuv_matrix;
};

struct dispc_out_mode {
	enum dispc_format_in fmt_in;
	enum dispc_format_out fmt_out;
	enum out_mode mode;

	bool matrix_need_update;
	const struct yuv2rgb_regs *yuv2rgb_matrix;
};

struct fbcdc_comp_level {
	u16 level;
	u16 size;
};

struct draw_reg {
	u32 offs;
	u32 val;
};

struct color_space_cfg {
	struct yuv2rgb_regs yuv2rgb_cfg;
	struct rgb2yuv_regs rgb2yuv_cfg;
};

struct dpu_hw_device {
	int id;

	void __iomem *regs;

	u8 dispc_csc_type;
	const struct rgb2yuv_regs *dispc_rgb2yuv_matrix;
	const struct yuv2rgb_regs *dispc_yuv2rgb_matrix;

	bool is_online;

	struct ax_disp_mode mode;

	struct dispc_input_mode input_mode;
	struct dispc_out_mode out_mode;
};

extern struct fbcdc_comp_level g_fbcdc_comp_level[];

int display_out_mode_convert(struct ax_disp_mode *mode, struct dispc_out_mode *dispc_out);
u32 vo_fmt2hw_fmt(u32 format);
u32 vo_fbcdc_tiles_calc(u32 fmt, u32 stride, u32 height);

#endif /* __AX620E_VO_COMMON_H */
