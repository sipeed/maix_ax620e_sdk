/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <common.h>
#include <cpu_func.h>
#include <splash.h>
#include <clk.h>
#include <display.h>
#include <dm.h>
#include <malloc.h>
#include <mapmem.h>
#include <edid.h>
#include <regmap.h>
#include <syscon.h>
#include <video.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/unaligned.h>
#include <linux/err.h>
#include <linux/compiler.h>
#include <power/regulator.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch/ax620e.h>

#include "ax_vo.h"
#include "ax620e/ax620e_vo_rst_ck_mux.h"

struct display_timing fixed_timigs[AX_VO_OUTPUT_BUTT] = {
	{ /* AX_VO_OUTPUT_576P50 */
		.pixelclock = {0, 27000000, 0},
		.hactive = {0, 720, 0},
		.hfront_porch = {0, 12, 0},
		.hback_porch = {0, 68, 0},
		.hsync_len = {0, 64, 0},
		.vactive = {0, 576, 0},
		.vfront_porch = {0, 5, 0},
		.vback_porch = {0, 39, 0},
		.vsync_len = {0, 5, 0},
		.flags = 0x5,
	},
	{ /* AX_VO_OUTPUT_480P60 */
		.pixelclock = {0, 27000000, 0},
		.hactive = {0, 720, 0},
		.hfront_porch = {0, 19, 0},
		.hback_porch = {0, 57, 0},
		.hsync_len = {0, 62, 0},
		.vactive = {0, 480, 0},
		.vfront_porch = {0, 9, 0},
		.vback_porch = {0, 30, 0},
		.vsync_len = {0, 6, 0},
		.flags = 0x5,
	},
	{ /* AX_VO_OUTPUT_720P25 */
		.pixelclock = {0, 37125000, 0},
		.hactive = {0, 1280, 0},
		.hfront_porch = {0, 440, 0},
		.hback_porch = {0, 220, 0},
		.hsync_len = {0, 40, 0},
		.vactive = {0, 720, 0},
		.vfront_porch = {0, 5, 0},
		.vback_porch = {0, 20, 0},
		.vsync_len = {0, 5, 0},
		.flags = 0x5,
	},
	{ /* AX_VO_OUTPUT_720P30 */
		.pixelclock = {0, 37125000, 0},
		.hactive = {0, 1280, 0},
		.hfront_porch = {0, 110, 0},
		.hback_porch = {0, 220, 0},
		.hsync_len = {0, 40, 0},
		.vactive = {0, 720, 0},
		.vfront_porch = {0, 5, 0},
		.vback_porch = {0, 20, 0},
		.vsync_len = {0, 5, 0},
		.flags = 0x5,
	},
	{ /* AX_VO_OUTPUT_1080P25 */
		.pixelclock = {0, 74250000, 0},
		.hactive = {0, 1920, 0},
		.hfront_porch = {0, 528, 0},
		.hback_porch = {0, 148, 0},
		.hsync_len = {0, 44, 0},
		.vactive = {0, 1080, 0},
		.vfront_porch = {0, 4, 0},
		.vback_porch = {0, 36, 0},
		.vsync_len = {0, 5, 0},
		.flags = 0xa,
	},
	{ /* AX_VO_OUTPUT_1080P30 */
		.pixelclock = {0, 74250000, 0},
		.hactive = {0, 1920, 0},
		.hfront_porch = {0, 88, 0},
		.hback_porch = {0, 148, 0},
		.hsync_len = {0, 44, 0},
		.vactive = {0, 1080, 0},
		.vfront_porch = {0, 4, 0},
		.vback_porch = {0, 36, 0},
		.vsync_len = {0, 5, 0},
		.flags = 0xa,
	},
	{ /* AX_VO_OUTPUT_1080P60 */
		.pixelclock = {0, 148500000, 0},
		.hactive = {0, 1920, 0},
		.hfront_porch = {0, 88, 0},
		.hback_porch = {0, 148, 0},
		.hsync_len = {0, 44, 0},
		.vactive = {0, 1080, 0},
		.vfront_porch = {0, 4, 0},
		.vback_porch = {0, 36, 0},
		.vsync_len = {0, 5, 0},
		.flags = 0xa,
	},
	{ /* AX_VO_OUTPUT_800_480_60 */
		.pixelclock = {0, 29700000, 0},
		.hactive = {0, 800, 0},
		.hfront_porch = {0, 10, 0},
		.hback_porch = {0, 88, 0},
		.hsync_len = {0, 30, 0},
		.vactive = {0, 480, 0},
		.vfront_porch = {0, 13, 0},
		.vback_porch = {0, 32, 0},
		.vsync_len = {0, 8, 0},
		.flags = 0x15,
	},
	{ /* AX_VO_OUTPUT_1080x1920_60 */
		.pixelclock = {0, 148500000, 0},
		.hactive = {0, 1080, 0},
		.hfront_porch = {0, 20, 0},
		.hback_porch = {0, 30, 0},
		.hsync_len = {0, 70, 0},
		.vactive = {0, 1920, 0},
		.vfront_porch = {0, 50, 0},
		.vback_porch = {0, 42, 0},
		.vsync_len = {0, 50, 0},
		.flags = 0xa,
	},
// ### SIPEED EDIT ###
	{ /* AX_VO_OUTPUT_480x640_60 */
		.pixelclock = {0, 24750000, 0},
		.hactive = {0, 480, 0},
		.hfront_porch = {0, 30, 0},
		.hback_porch = {0, 30, 0},
		.hsync_len = {0, 40, 0},
		.vactive = {0, 640, 0},
		.vfront_porch = {0, 30, 0},
		.vback_porch = {0, 30, 0},
		.vsync_len = {0, 11, 0},
		.flags = 0xa,
	},
// ### SIPEED EDIT END ###
};

extern struct dpu_hw_ops ax620e_dpu_hw_ops;
extern struct dpu_hw_ops ax620e_dpu_lite_hw_ops;

static struct ax_dpu_device g_dpu_devices[VO_NR] = {
	{
		.id = 0,
		.hdev = {
			.id = 0,
			.regs = (void __iomem *)AX_VO_DPU0_BASE_ADDR,
		},
		.ops = &ax620e_dpu_hw_ops,
	},
	{
		.id = 1,
		.hdev = {
			.id = 1,
			.regs = (void __iomem *)AX_VO_DPU1_BASE_ADDR,
		},
		.ops = &ax620e_dpu_lite_hw_ops,
	},
};

void ax_dispc_enable(struct ax_dpu_device *ddev)
{
	if (ddev->ops && ddev->ops->dispc_enable)
		ddev->ops->dispc_enable(&ddev->hdev);
}

int ax_dispc_config(struct ax_dpu_device *ddev, struct ax_disp_mode *mode)
{
	int ret = 0;

	if (ddev->ops && ddev->ops->dispc_config)
		ret = ddev->ops->dispc_config(&ddev->hdev, mode);

	return ret;
}

void ax_dispc_set_buffer(struct ax_dpu_device *ddev, struct layer_info *li)
{
	if (ddev->ops && ddev->ops->dispc_set_buffer)
		ddev->ops->dispc_set_buffer(&ddev->hdev, li);
}

int ax_dpu_init(struct ax_dpu_device *ddev)
{
	int ret = 0;

	if (ddev->ops && ddev->ops->dpu_init)
		ret = ddev->ops->dpu_init(&ddev->hdev);

	return ret;
}

int ax_draw_task_commit(struct ax_dpu_device *ddev, struct draw_task *task)
{
	int ret = 0;

	if (ddev->ops && ddev->ops->draw_start)
		ret = ddev->ops->draw_start(task);

	return ret;
}

static int draw_task_check(struct ax_dpu_device *ddev, struct draw_task *task)
{
	int ret = 0;

	if (ddev->ops && ddev->ops->draw_start)
		ret = ddev->ops->task_valid(task);

	return ret;
}

static int ax_vo_display(struct ax_dpu_device *ddev, struct display_info *dp_info)
{
	int ret = 0, img_szie;
	struct layer_info li = {0};
	struct draw_task task = {0};

	li.w = ddev->mode.hdisplay;
	li.h = ddev->mode.vdisplay;

	if (ddev->id == 0 && (dp_info->display_x || dp_info->display_y ||
	    (dp_info->img_width < li.w) || (dp_info->img_height < li.h))) {
		task.src_w = dp_info->img_width;
		task.src_h = dp_info->img_height;
		task.src_fmt = dp_info->img_fmt;

		task.src_stride_y = dp_info->img_stride;
		if (task.src_fmt <= AX_VO_FORMAT_NV21)
			task.src_stride_c = task.src_stride_y;
		else
			task.src_stride_c = 0;

		task.src_phy_addr_y = dp_info->img_addr[0];
		task.src_phy_addr_c = dp_info->img_addr[1];
		task.dst_w = ddev->mode.hdisplay;
		task.dst_h = ddev->mode.vdisplay;

		/* background color, R:[23,16], G:[15,8] and B:[7,0] */
		task.bk_pixel = 0x0;

		if (dp_info->display_x >= task.dst_w)
			task.dst_x = 0;
		else
			task.dst_x = dp_info->display_x & (~0x1);

		if (task.dst_x + task.src_w > task.dst_w)
			task.src_w = task.dst_w - task.dst_x;

		if (dp_info->display_y >= task.dst_h)
			task.dst_y = 0;
		else
			task.dst_y = dp_info->display_y & (~0x1);

		if (task.dst_y + task.src_h > task.dst_h)
			task.src_h = task.dst_h - task.dst_y;

		task.dst_fmt = AX_VO_FORMAT_NV12;
		task.dst_stride_y = (task.dst_w + 0xf) & (~0xf);
		task.dst_stride_c = task.dst_stride_y;
		task.dst_phy_addr_y = dp_info->display_addr;
		task.dst_phy_addr_c = task.dst_phy_addr_y + task.dst_stride_y * task.dst_h;

		task.data = &ddev->hdev;

		ret = draw_task_check(ddev, &task);
		if (ret) {
			VO_ERROR("dpu%d draw task illegal parameters\n", ddev->id);
			return ret;
		}

		img_szie = task.src_stride_y * task.src_h;
		flush_dcache_range(task.src_phy_addr_y & (~(CONFIG_SYS_CACHELINE_SIZE - 1)),
		                   ALIGN(task.src_phy_addr_y + img_szie, CONFIG_SYS_CACHELINE_SIZE));

		if (task.src_fmt <= AX_VO_FORMAT_NV21) {
			img_szie = task.src_stride_y * task.src_h / 2;
			flush_dcache_range(task.src_phy_addr_c & (~(CONFIG_SYS_CACHELINE_SIZE - 1)),
				ALIGN(task.src_phy_addr_c + img_szie, CONFIG_SYS_CACHELINE_SIZE));
		}

		ret = ax_draw_task_commit(ddev, &task);
		if (ret) {
			VO_ERROR("dpu%d draw task commit failed, ret = %d\n", ddev->id, ret);
			return ret;
		}

		li.format = task.dst_fmt;
		li.stride_y = task.dst_stride_y;
		li.stride_c = task.dst_stride_c;
		li.phy_addr_y = task.dst_phy_addr_y;
		li.phy_addr_c = task.dst_phy_addr_c;

	} else {
		li.format = dp_info->img_fmt;
		li.stride_y = dp_info->img_stride;
		li.phy_addr_y = dp_info->img_addr[0];
		li.phy_addr_c = dp_info->img_addr[1];
		li.stride_c = li.phy_addr_c ? dp_info->img_stride : 0;

		img_szie = logo_image_size(li.stride_y, li.h, li.format);
		flush_dcache_range(li.phy_addr_y & (~(CONFIG_SYS_CACHELINE_SIZE - 1)),
				   ALIGN(li.phy_addr_y + img_szie, CONFIG_SYS_CACHELINE_SIZE));
	}

	dp_info->reserved_mem_addr = li.phy_addr_y;
	dp_info->reserved_mem_size = logo_image_size(li.stride_y, li.h, li.format);

	ax_dispc_set_buffer(ddev, &li);

	ax_dispc_enable(ddev);

	return 0;
}

static int display_timing2disp_mode(struct display_timing *timing, struct ax_disp_mode *mode)
{
	mode->clock = timing->pixelclock.typ / 1000;		/* in kHz */
	mode->hdisplay = timing->hactive.typ;
	mode->hsync_start = mode->hdisplay + timing->hfront_porch.typ;
	mode->hsync_end = mode->hsync_start + timing->hsync_len.typ;
	mode->htotal = mode->hsync_end + timing->hback_porch.typ;
	mode->vdisplay = timing->vactive.typ;
	mode->vsync_start = mode->vdisplay + timing->vfront_porch.typ;
	mode->vsync_end = mode->vsync_start + timing->vsync_len.typ;
	mode->vtotal = mode->vsync_end + timing->vback_porch.typ;

	mode->hp_pol = timing->flags & DISPLAY_FLAGS_HSYNC_LOW ? 1 : 0;
	mode->vp_pol = timing->flags & DISPLAY_FLAGS_VSYNC_LOW ? 1 : 0;
	mode->de_pol = timing->flags & DISPLAY_FLAGS_DE_LOW ? 1: 0;

	mode->flags |= (timing->flags & DISPLAY_FLAGS_INTERLACED) ? MODE_FLAG_INTERLACE : 0;

	mode->vrefresh = (timing->pixelclock.typ + ((mode->vtotal * mode->htotal) >> 1)) /
	                 (mode->vtotal * mode->htotal);

	VO_DEBUG("reso: %dx%d, flags: 0x%x, pclk: %d\n",
	         timing->hactive.typ, timing->vactive.typ, timing->flags,
	         timing->pixelclock.typ);

	return 0;
}

static int ax_vo_setup_mode(struct ax_dpu_device *ddev, u32 type, u32 sync)
{
	int ret;
	struct ax_disp_mode *mode = &ddev->mode;
	struct display_timing timing = {0};

	VO_DEBUG("enter\n");

	if (sync < AX_VO_OUTPUT_BUTT) {
		timing = fixed_timigs[sync];
	} else {
		VO_ERROR("dpu%d sync(%d) is not supported\n", ddev->id, sync);
	}

	VO_DEBUG("timing: [%d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%x]\n",
		  timing.pixelclock.typ, timing.hactive.typ, timing.hfront_porch.typ,
		  timing.hback_porch.typ, timing.hsync_len.typ, timing.vactive.typ,
		  timing.vfront_porch.typ, timing.vback_porch.typ, timing.vsync_len.typ,
		  timing.flags);

	ret = display_timing2disp_mode(&timing, mode);
	if (ret) {
		VO_ERROR("failed to get dpu%d mode\n", ddev->id);
		return ret;
	}

	mode->type = type;

	display_glb_init(ddev->id, type);

	ret = ax_dpu_init(ddev);
	if (ret) {
		VO_ERROR("failed to init dpu%d\n", ddev->id);
		return ret;
	}

	mode->fmt_in = AX_VO_FORMAT_NV12;

	if (type == AX_DISP_OUT_MODE_DSI_DPI_VIDEO)
		mode->fmt_out = AX_DISP_OUT_FMT_RGB888;
	else if (type == AX_DISP_OUT_MODE_BT656)
		mode->fmt_out = AX_DISP_OUT_FMT_YUV422;
	else
		mode->fmt_out = AX_DISP_OUT_FMT_RGB565;

	ret = ax_dispc_config(ddev, mode);
	if (ret) {
		VO_ERROR("failed to config dpu%d\n", ddev->id);
		return ret;
	}

	ret = display_glb_path_config(ddev->id, type, mode);
	if (ret) {
		VO_ERROR("failed to init display clk%d\n", ddev->id);
		return ret;
	}

	ddev->timing = timing;

	VO_DEBUG("done\n");

	return 0;
}

static int ax_vo_pipe_init(struct ax_dpu_device *ddev, u32 type, u32 sync)
{
	VO_DEBUG("enter\n");

	return ax_vo_setup_mode(ddev, type, sync);
}

int fdt_fixup_vo_init_mode(int devid, void *fdt)
{
	int ret, offset, parent_offset;
	uint32_t val;
	char path[128], *name = "init-mode";
	struct ax_dpu_device *ddev = &g_dpu_devices[devid];
	struct display_timing *timing = &ddev->timing;

	if (!timing->pixelclock.typ)
		return -EPERM;

	sprintf(path, "/soc/drm@%d", devid);

	parent_offset = fdt_path_offset(fdt, path);
	if (parent_offset < 0) {
		VO_ERROR("%s not found\n", path);
		return -EINVAL;
	}

	offset = fdt_add_subnode(fdt, parent_offset, name);
	if (offset < 0) {
		VO_ERROR("add %s to %s failed, ret = %d\n", name, path, offset);
		return offset;
	}

	val = cpu_to_fdt32(ddev->mode.type);
	ret = fdt_setprop(fdt, offset, "interface-type", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set interface-type to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->pixelclock.typ);
	ret = fdt_setprop(fdt, offset, "clock-frequency", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set clock-frequency to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->hactive.typ);
	ret = fdt_setprop(fdt, offset, "hactive", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set hactive to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->hfront_porch.typ);
	ret = fdt_setprop(fdt, offset, "hfront-porch", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set hfront-porch to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->hsync_len.typ);
	ret = fdt_setprop(fdt, offset, "hsync-len", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set hsync-len to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->hback_porch.typ);
	ret = fdt_setprop(fdt, offset, "hback-porch", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set hback-porch to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->vactive.typ);
	ret = fdt_setprop(fdt, offset, "vactive", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set vactive to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->vfront_porch.typ);
	ret = fdt_setprop(fdt, offset, "vfront-porch", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set vfront-porch to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->vsync_len.typ);
	ret = fdt_setprop(fdt, offset, "vsync-len", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set vsync-len to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(timing->vback_porch.typ);
	ret = fdt_setprop(fdt, offset, "vback-porch", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set vback-porch to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(!!(timing->flags & DISPLAY_FLAGS_HSYNC_HIGH));
	ret = fdt_setprop(fdt, offset, "hsync-active", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set hsync-active to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(!!(timing->flags & DISPLAY_FLAGS_VSYNC_HIGH));
	ret = fdt_setprop(fdt, offset, "vsync-active", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set vsync-active to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	val = cpu_to_fdt32(!!(timing->flags & DISPLAY_FLAGS_DE_HIGH));
	ret = fdt_setprop(fdt, offset, "de-active", &val, sizeof(val));
	if (ret) {
		VO_ERROR("set de-active to %s failed, ret = %d\n", name, ret);
		goto exit;
	}

	if (!!(timing->flags & DISPLAY_FLAGS_INTERLACED)) {
		ret = fdt_setprop(fdt, offset, "interlaced", NULL, 0);
		if (ret) {
			VO_ERROR("set interlaced to %s failed, ret = %d\n", name, ret);
			goto exit;
		}
	}

	if (!!(timing->flags & DISPLAY_FLAGS_DOUBLESCAN)) {
		ret = fdt_setprop(fdt, offset, "doublescan", NULL, 0);
		if (ret) {
			VO_ERROR("set doublescan to %s failed, ret = %d\n", name, ret);
			goto exit;
		}
	}

exit:
	if (ret)
		fdt_del_node(fdt, offset);

	VO_INFO("add %s node %s\n", name, ret ? "failed" : "success");

	return ret;
}

int ax_start_vo(u32 devid, u32 type, u32 sync, struct display_info *dp_info)
{
	int ret;
	struct ax_dpu_device *ddev = &g_dpu_devices[devid];

	/* Before relocation we don't need to do anything */
	if (!(gd->flags & GD_FLG_RELOC))
		return 0;

	if (devid >= VO_NR) {
		VO_ERROR("invalid devid(%d)\n", devid);
		return -EINVAL;
	}

	if (type >= AX_DISP_OUT_MODE_BUT) {
		VO_ERROR("invalid type(%d)\n", type);
		return -EINVAL;
	}

	ddev = &g_dpu_devices[devid];

	ret = ax_vo_pipe_init(ddev, type, sync);
	if (ret)
		return ret;

	return ax_vo_display(ddev, dp_info);
}

