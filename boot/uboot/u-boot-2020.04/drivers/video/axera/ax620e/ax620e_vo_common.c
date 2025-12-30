/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_vo.h"

const struct color_space_cfg cs_cfgs = {
	.yuv2rgb_cfg = { \
		.matrix = { \
			{0x100, 0x0, 0x193}, \
			{0x100, 0x7d0, 0x788}, \
			{0x100, 0x1db, 0x0} \
		}, \
		.offset = { \
			{0x0, 0x0, 0x0}, \
			{0x0, 0x0, 0x0}  \
		},\
	},
	.rgb2yuv_cfg = { \
		.matrix = { \
			{0x36, 0xb7, 0x12}, \
			{0x7e3, 0x79d, 0x80}, \
			{0x80, 0x78c, 0x7f4} \
		}, \
		.offset = { \
			{0x0, 0x0, 0x0}, \
			{0x0, 0x0, 0x0} \
		}, \
		.decimation_h = {0x0, 0x0, 0x0, 0x10, 0x10, 0x0, 0x0}, \
		.uv_offbin_en = 0, \
		.uv_seq_sel = 0, \
	},
};

struct fbcdc_comp_level g_fbcdc_comp_level[] = {
	{0, 256}, /* 8bit lossless */
	{1, 32},
	{2, 64},
	{3, 96},
	{4, 128},
	{5, 160},
	{6, 192},
	{7, 224},
};

int display_out_mode_convert(struct ax_disp_mode *mode, struct dispc_out_mode *dispc_out)
{
	int fmt_out = mode->fmt_out;

	VO_INFO("fmt_out: %d\n", fmt_out);

	switch (mode->type) {
	case AX_DISP_OUT_MODE_BT601:
		dispc_out->mode = OUT_MODE_BT601;
		break;
	case AX_DISP_OUT_MODE_BT656:
		dispc_out->mode = OUT_MODE_BT656;
		break;
	case AX_DISP_OUT_MODE_BT1120:
		dispc_out->mode = OUT_MODE_BT1120;
		break;
	case AX_DISP_OUT_MODE_DPI:
		dispc_out->mode = OUT_MODE_DPI;
		break;
	case AX_DISP_OUT_MODE_DSI_SDI_VIDEO:
		dispc_out->mode = OUT_MODE_DSI_SDI_VIDEO;
		break;
	case AX_DISP_OUT_MODE_DSI_SDI_CMD:
		dispc_out->mode = OUT_MODE_DSI_SDI_CMD;
		break;
	case AX_DISP_OUT_MODE_DSI_DPI_VIDEO:
		dispc_out->mode = OUT_MODE_DSI_DPI_VIDEO;
		break;
	case AX_DISP_OUT_MODE_LVDS:
		dispc_out->mode = OUT_MODE_LVDS;
		break;

	default:
		VO_ERROR("unsupported mode type, mode = %d\n", mode->type);
		return -EINVAL;
	}

	switch (fmt_out) {
	case AX_DISP_OUT_FMT_RGB565:
		dispc_out->fmt_out = FMT_OUT_RGB565;
		break;
	case AX_DISP_OUT_FMT_RGB666:
		dispc_out->fmt_out = FMT_OUT_RGB666;
		break;
	case AX_DISP_OUT_FMT_RGB666LP:
		dispc_out->fmt_out = FMT_OUT_RGB666LP;
		break;
	case AX_DISP_OUT_FMT_RGB888:
		dispc_out->fmt_out = FMT_OUT_RGB888;
		break;
	case AX_DISP_OUT_FMT_YUV422:
		dispc_out->fmt_out = FMT_OUT_YUV422;
		break;
	default:
		VO_ERROR("unsupported fmt_out, fmt_out = %d\n", fmt_out);
		return -EINVAL;
	}

	return 0;
}

u32 vo_fmt2hw_fmt(u32 format)
{
	u32 hw_fmt = FORMAT_NO_SUPPORT;

	switch (format) {
	case AX_VO_FORMAT_NV21:
		hw_fmt = 0x40 | FORMAT_YUV420_8;
		break;
	case AX_VO_FORMAT_NV12:
		hw_fmt = FORMAT_YUV420_8;
		break;
	case AX_VO_FORMAT_ARGB1555:
		hw_fmt = FORMAT_ARGB1555;
		break;
	case AX_VO_FORMAT_ARGB4444:
		hw_fmt = FORMAT_ARGB4444;
		break;
	case AX_VO_FORMAT_RGBA5658:
		hw_fmt = FORMAT_RGBA5658;
		break;
	case AX_VO_FORMAT_ARGB8888:
		hw_fmt = FORMAT_ARGB8888;
		break;
	case AX_VO_FORMAT_BGR565:
		hw_fmt = 0x20 | FORMAT_RGB565;
		break;
	case AX_VO_FORMAT_RGB565:
		hw_fmt = FORMAT_RGB565;
		break;
	case AX_VO_FORMAT_BGR888:
		hw_fmt = 0x20 | FORMAT_RGB888;
		break;
	case AX_VO_FORMAT_RGB888:
		hw_fmt = FORMAT_RGB888;
		break;
	case AX_VO_FORMAT_RGBA4444:
		hw_fmt = FORMAT_RGBA4444;
		break;
	case AX_VO_FORMAT_RGBA5551:
		hw_fmt = FORMAT_RGBA5551;
		break;
	case AX_VO_FORMAT_RGBA8888:
		hw_fmt = FORMAT_RGBA8888;
		break;
	case AX_VO_FORMAT_ARGB8565:
		hw_fmt = FORMAT_ARGB8565;
		break;
	case AX_VO_FORMAT_NV16:
		hw_fmt = FORMAT_YUV422_8;
		break;
	case AX_VO_FORMAT_BITMAP:
		hw_fmt = FORMAT_BIT_MAP;
		break;
	default:
		VO_ERROR("unsupported format(%d) and set default format to single-y\n", format);
		hw_fmt = FORMAT_SINGLE_Y;
		break;
	}

	return hw_fmt;
}

u32 vo_fbcdc_tiles_calc(u32 fmt, u32 stride, u32 height)
{
	return (stride / 128) * (height / 2);
}
