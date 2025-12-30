/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/dma-mapping.h>
#include <common.h>

#include "ax620e_vo_lite_reg.h"
#include "ax_vo.h"
#include "ax620e_vo_rst_ck_mux.h"

extern const struct color_space_cfg cs_cfgs;

static void dpu_intr_mask_all(struct dpu_hw_device *hdev)
{
	dpu_writel(hdev->regs, DPU_LITE_INT_MASK, 0x7F);
}

static void dpu_intr_sts_clr_all(struct dpu_hw_device *hdev)
{
	dpu_writel(hdev->regs, DPU_LITE_INT_CLR, (DPU_INT_DISP_EOF | DPU_INT_DISP_SOF | DPU_INT_AXI_RERR | DPU_INT_DISP_UNDER));
}

static void dispc_update_lock(struct dpu_hw_device *hdev)
{
	dpu_writel(hdev->regs, DPU_LITE_DISP_UP, 0);
}

static void dispc_update_unlock(struct dpu_hw_device *hdev)
{
	dpu_writel(hdev->regs, DPU_LITE_DISP_UP, DISP_LITE_UPDATE);
}

static void dispc_enable(struct dpu_hw_device *hdev)
{
	dpu_writel(hdev->regs, DPU_LITE_DISP_EN, 0x1);
}

static void dispc_disable(struct dpu_hw_device *hdev)
{
	dpu_writel(hdev->regs, DPU_LITE_DISP_EN, 0x0);
}

static void dispc_set_out_mode(struct dpu_hw_device *hdev, u32 mode)
{
	dpu_writel(hdev->regs, DPU_LITE_OUT_MODE, mode);
}

static void dispc_set_format_out(struct dpu_hw_device *hdev, u32 fmt)
{
	dpu_writel(hdev->regs, DPU_LITE_DISP_FORMAT, fmt);
}

static void dispc_set_bt_mode(struct dpu_hw_device *hdev, u32 mode)
{
	dpu_writel(hdev->regs, DPU_LITE_BT_MODE, mode);
}

static void dispc_set_disp_reso(struct dpu_hw_device *hdev, u32 reso)
{
	dpu_writel(hdev->regs, DPU_LITE_DISP_RESO, reso);
}

static void dispc_set_timings(struct dpu_hw_device *hdev, struct ax_disp_mode *mode)
{
	u32 hp_pol, vp_pol, de_pol;
	u32 hbp, hfp, vbp, vfp, hpw, vpw;
	u32 hhalf;

	hp_pol = mode->hp_pol;
	vp_pol = mode->vp_pol;
	de_pol = mode->de_pol;

	VO_DEBUG("hp_pol = %d, vp_pol = %d, de_pol = %d, out_mode:%d\n", hp_pol, vp_pol, de_pol, hdev->out_mode.mode);

	hpw = mode->hsync_end - mode->hsync_start;
	vpw = mode->vsync_end - mode->vsync_start;

	hbp = mode->htotal - mode->hsync_end;
	hfp = mode->hsync_start - mode->hdisplay;

	vbp = mode->vtotal - mode->vsync_end;
	vfp = mode->vsync_start - mode->vdisplay;
	if (mode->flags & MODE_FLAG_INTERLACE) {
		vbp = (vbp >> 1);
		vfp = (vfp >> 1);
	}

	hhalf = mode->htotal >> 1;

	if (hdev->out_mode.mode == OUT_MODE_BT1120) {
		hpw = mode->htotal - mode->hdisplay - 4;
		vpw = mode->vtotal - mode->vsync_start;
	} else if (hdev->out_mode.mode == OUT_MODE_BT656) {
		hpw = mode->htotal - mode->hdisplay - 2;
		vpw = mode->vtotal - mode->vsync_start;
	}
	if (mode->flags & MODE_FLAG_INTERLACE)
		vpw = (vpw >> 1);

	VO_DEBUG("hbp = %d, hfp = %d, hpw = %d\n", hbp, hfp, hpw);
	VO_DEBUG("vbp = %d, vfp = %d, vpw = %d\n", vbp, vfp, vpw);

	dpu_writel(hdev->regs, DPU_LITE_DISP_SYNC, (hpw << DISPC_H_SHIFT) | vpw);
	dpu_writel(hdev->regs, DPU_LITE_DISP_HSYNC, (hbp << DISPC_H_SHIFT) | hfp);
	dpu_writel(hdev->regs, DPU_LITE_DISP_VSYNC, (vbp << DISPC_H_SHIFT) | vfp);
	dpu_writel(hdev->regs, DPU_LITE_DISP_HHALF, hhalf);

	if (mode->flags & MODE_FLAG_INTERLACE)
		dpu_writel(hdev->regs, DPU_LITE_DISP_VTOTAL, !(mode->vtotal & 1));

	dpu_writel(hdev->regs, DPU_LITE_DISP_POLAR, de_pol << 2 | hp_pol << 1 | vp_pol);
}

static void dispc_set_yuv2rgb_matrix(struct dpu_hw_device *hdev)
{
	int i, j;
	u32 reg_offs = DPU_LITE_DISP_2RGB_MATRIX_00;
	struct dispc_out_mode *out_mode = &hdev->out_mode;
	const struct yuv2rgb_regs *m = out_mode->yuv2rgb_matrix;

	VO_DEBUG("fmt_out:%d\n", out_mode->fmt_out);

	if (out_mode->fmt_out < FMT_OUT_YUV422) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				dpu_writel(hdev->regs, reg_offs, m->matrix[i][j]);
				reg_offs += 4;
			}
		}

		for (i = 0; i < 2; i++) {
			for (j = 0; j < 3; j++) {
				dpu_writel(hdev->regs, reg_offs, m->offset[i][j]);
				reg_offs += 4;
			}
		}

		dpu_writel(hdev->regs, DPU_LITE_DISP_2RGB_CTRL, 0x3);

	} else {
		dpu_writel(hdev->regs, DPU_LITE_DISP_2RGB_CTRL, 0x2);
	}
}

static void dispc_set_rgb2yuv_matrix(struct dpu_hw_device *hdev)
{
	int i, j;
	u32 reg_offs = DPU_LITE_RD_2YUV_MATRIX_00;
	struct dispc_input_mode *input_mode = &hdev->input_mode;
	const struct rgb2yuv_regs *m = input_mode->rgb2yuv_matrix;

	VO_DEBUG("fmt_in:%d\n", input_mode->fmt_in);

	/* rgb to yuv422 */
	if (HW_FMT_EXTRA(input_mode->fmt_in) <= FMT_IN_RGB888) {
		dpu_writel(hdev->regs, DPU_LITE_RD_2YUV_EN, 0x1);
		dpu_writel(hdev->regs, DPU_LITE_RD_2YUV_CTRL, 0x11);

		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				dpu_writel(hdev->regs, reg_offs, m->matrix[i][j]);
				reg_offs += 4;
			}
		}

		for (i = 0; i < 2; i++) {
			for (j = 0; j < 3; j++) {
				dpu_writel(hdev->regs, reg_offs, m->offset[i][j]);
				reg_offs += 4;
			}
		}

		for (i = 0; i < 7; i++) {
			dpu_writel(hdev->regs, reg_offs, m->decimation_h[i]);
			reg_offs += 4;
		}
	} else {
		dpu_writel(hdev->regs, DPU_LITE_RD_2YUV_EN, 0x0);
		dpu_writel(hdev->regs, DPU_LITE_RD_2YUV_CTRL, 0x10);
	}
}

static void ax620e_dispc_set_out_mode(struct dpu_hw_device *hdev, struct dispc_out_mode *out_mode)
{
	u32 out_cfg = 0;

	switch (out_mode->mode) {
	case OUT_MODE_BT601:
	case OUT_MODE_BT656:
	case OUT_MODE_BT1120:
		/*for evb*/
		if(OUT_MODE_BT601 == out_mode->mode) {
			dpu_writel(hdev->regs, DPU_LITE_PIN_SRC_SEL4, 0x101112);
		} else if (OUT_MODE_BT1120 == out_mode->mode) {
			dpu_writel(hdev->regs, DPU_LITE_PIN_SRC_SEL2, 0x09080908);
			dpu_writel(hdev->regs, DPU_LITE_PIN_SRC_SEL3, 0x0d0c0b0a);
			dpu_writel(hdev->regs, DPU_LITE_PIN_SRC_SEL4, 0x11100f0e);
		}
		out_cfg |= (0x1 << 2);
		break;

	case OUT_MODE_DPI:
		dpu_writel(hdev->regs, DPU_LITE_PIN_SRC_SEL4, 0x111210);
		out_cfg |= (0x1 << 3);
		break;
	default:
		VO_ERROR("unsupported out_mode, mode = 0x%x\n", out_mode->mode);
		return;
	}

	dispc_set_format_out(hdev, out_mode->fmt_out);
	dispc_set_out_mode(hdev, out_cfg);
}

static void dispc_dither(struct dpu_hw_device *dev)
{
	if (FMT_OUT_RGB565 == dev->out_mode.fmt_out)
		dpu_writel(dev->regs, DPU_LITE_DITHER_OUT_ACC, (DITHER_ACC_5BIT << 16) | (DITHER_ACC_6BIT << 8) | DITHER_ACC_5BIT);

	dpu_writel(dev->regs, DPU_LITE_DITHER_EN, 1);
	dpu_writel(dev->regs, DPU_LITE_DITHER_UP, 1);
}

static int ax620e_dispc_config(struct dpu_hw_device *hdev, struct ax_disp_mode *mode)
{
	int tmp, ret = 0;
	u32 reso, bt_mode;
	struct dispc_out_mode *out_mode = &hdev->out_mode;

	hdev->mode = *mode;

	hdev->input_mode.fmt_in = FORMAT_YUV420_8;

	ret = display_out_mode_convert(mode, out_mode);
	if (ret)
		goto exit;

	dispc_set_rgb2yuv_matrix(hdev);
	ax620e_dispc_set_out_mode(hdev, out_mode);
	dispc_set_yuv2rgb_matrix(hdev);

	if (hdev->mode.flags & MODE_FLAG_INTERLACE)
		reso = (((mode->vdisplay >> 1) << 16) | mode->hdisplay);
	else
		reso = ((mode->vdisplay << 16) | mode->hdisplay);


	dispc_set_disp_reso(hdev, reso);

	dpu_writel(hdev->regs, DPU_LITE_RD_2YUV_EN, 0x0);
	dpu_writel(hdev->regs, DPU_LITE_RD_2YUV_CTRL, 0x10);

	dispc_set_timings(hdev, &hdev->mode);

	VO_INFO("mode:%d, fmt_out:%d vdisplay:%d, hdisplay:%d, clock:%d\n",
		out_mode->mode, out_mode->fmt_out, mode->vdisplay, mode->hdisplay, mode->clock);

	tmp = out_mode->mode;
	switch (tmp) {
	case OUT_MODE_BT601:
	case OUT_MODE_BT656:
	case OUT_MODE_BT1120:
		if (out_mode->mode == OUT_MODE_BT656)
			bt_mode = 0;
		else if (out_mode->mode == OUT_MODE_BT601)
			bt_mode = 1;
		else
			bt_mode = 2;

		if (hdev->mode.flags & MODE_FLAG_INTERLACE) {
			/* set field mode */
			bt_mode |= (1 << 3);
		}

		dispc_set_bt_mode(hdev, bt_mode);
		break;
	}

	dispc_dither(hdev);

exit:
	VO_INFO("dispc%d config %s\n", hdev->id, ret ? "failed" : "success");

	return ret;
}

static void ax620e_dispc_enable(struct dpu_hw_device *hdev)
{
	dispc_enable(hdev);
}

static void ax620e_dispc_disable(struct dpu_hw_device *hdev)
{
	dispc_disable(hdev);
}

static void ax620e_dispc_set_buffer(struct dpu_hw_device *hdev,  struct layer_info *li)
{
	u64 addr_y, addr_uv;
	u32 stride_y, stride_uv, fmt_in;
	u32 stride_factor = (hdev->mode.flags & MODE_FLAG_INTERLACE) ? 1 : 0;

	if (!hdev || !li)
		return;

	addr_y = li->phy_addr_y;
	addr_uv = li->phy_addr_c;
	stride_y = li->stride_y << stride_factor;
	stride_uv = li->stride_c << stride_factor;

	fmt_in = vo_fmt2hw_fmt(li->format);

	VO_DEBUG("vo%d addr:%llx-%llx, stride:%d-%d, fmt:0x%x\n", hdev->id,
		 addr_y, addr_uv,
		 stride_y, stride_uv,
		 fmt_in);

	dispc_update_lock(hdev);

	dpu_writel(hdev->regs, DPU_LITE_RD_FBDC_EN, 0);

	if (fmt_in != hdev->input_mode.fmt_in) {
		hdev->input_mode.fmt_in = fmt_in;
		dispc_set_rgb2yuv_matrix(hdev);
		dpu_writel(hdev->regs, DPU_LITE_RD_FORMAT, fmt_in);
	}

	if (hdev->out_mode.matrix_need_update)
		dispc_set_yuv2rgb_matrix(hdev);

	dpu_writel(hdev->regs, DPU_LITE_RD_ADDR_Y_l, PYHS_ADDR_LOW(addr_y));
	dpu_writel(hdev->regs, DPU_LITE_RD_ADDR_Y_H, PYHS_ADDR_HIGH(addr_y));
	dpu_writel(hdev->regs, DPU_LITE_RD_ADDR_C_l, PYHS_ADDR_LOW(addr_uv));
	dpu_writel(hdev->regs, DPU_LITE_RD_ADDR_C_H, PYHS_ADDR_HIGH(addr_uv));
	dpu_writel(hdev->regs, DPU_LITE_RD_STRIDE_Y, stride_y);
	dpu_writel(hdev->regs, DPU_LITE_RD_STRIDE_C, stride_uv);

	dispc_update_unlock(hdev);
}

static int ax620e_hw_init(struct dpu_hw_device *hdev)
{
	const struct color_space_cfg *cs_cfg = &cs_cfgs;

	hdev->input_mode.rgb2yuv_matrix = &cs_cfg->rgb2yuv_cfg;
	hdev->out_mode.yuv2rgb_matrix = &cs_cfg->yuv2rgb_cfg;

	hdev->is_online = false;

	dpu_glb_init(hdev->id);

	dpu_intr_mask_all(hdev);

	dispc_disable(hdev);

	dpu_intr_sts_clr_all(hdev);

	dpu_writel(hdev->regs, DPU_LITE_AXI_EN, 0x1);

	VO_INFO("dpu%d hw init success\n", hdev->id);

	return 0;
}

static int ax620e_dpu_lite_init(struct dpu_hw_device *hdev)
{
	return ax620e_hw_init(hdev);
}

const struct dpu_hw_ops ax620e_dpu_lite_hw_ops = {
	.dpu_init = ax620e_dpu_lite_init,
	.dispc_config = ax620e_dispc_config,
	.dispc_enable = ax620e_dispc_enable,
	.dispc_disable = ax620e_dispc_disable,
	.dispc_set_buffer = ax620e_dispc_set_buffer,
};

