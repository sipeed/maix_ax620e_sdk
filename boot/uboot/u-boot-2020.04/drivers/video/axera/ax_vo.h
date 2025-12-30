/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_VO_H
#define __AX_VO_H

#include "ax_vo_common.h"
#include "ax620e/ax620e_vo_common.h"
#include "ax_simple_logo.h"

struct display_info {
	u32 img_width;
	u32 img_height;
	u32 img_fmt;
	u32 img_stride;
	u64 img_addr[3];

	u32 display_x;
	u32 display_y;
	u64 display_addr;

	u64 reserved_mem_addr;
	u32 reserved_mem_size;
};

struct layer_info {
	u16 format;
	u16 w;
	u16 h;
	u16 stride_y;
	u16 stride_c;
	u64 phy_addr_y;
	u64 phy_addr_c;
};

struct draw_task {
	u32 src_w: 16;
	u32 src_h: 16;
	u32 src_fmt;
	u32 src_stride_y: 16;
	u32 src_stride_c: 16;
	phys_addr_t src_phy_addr_y;
	phys_addr_t src_phy_addr_c;

	u32 dst_x: 16;
	u32 dst_y: 16;
	u32 dst_w: 16;
	u32 dst_h: 16;
	u32 dst_fmt;
	u32 dst_stride_y: 16;
	u32 dst_stride_c: 16;
	phys_addr_t dst_phy_addr_y;
	phys_addr_t dst_phy_addr_c;

	u32 bk_pixel;

	void *data;
};

struct dpu_hw_ops {
	int (*dpu_init)(struct dpu_hw_device *hdev);
	void (*dpu_deinit)(struct dpu_hw_device *hdev);

	int (*dispc_config)(struct dpu_hw_device *hdev, struct ax_disp_mode *mode);
	void (*dispc_enable)(struct dpu_hw_device *hdev);
	void (*dispc_disable)(struct dpu_hw_device *hdev);
	void (*dispc_set_buffer)(struct dpu_hw_device *hdev, struct layer_info *li);

	int (*task_valid)(struct draw_task *task);
	int (*draw_start)(struct draw_task *task);
};

struct ax_dpu_device {
	int id;

	struct ax_disp_mode mode;
	struct display_timing timing;

	struct dpu_hw_device hdev;

	struct dpu_hw_ops *ops;
};

static inline u32 logo_image_size(u32 w, u32 h, u32 fmt)
{
	u32 size = w * h * 3;

	switch (fmt) {
	case AX_VO_FORMAT_NV21:
	case AX_VO_FORMAT_NV12:
		size = w * h * 3 / 2;
		break;
	case AX_VO_FORMAT_RGB565:
	case AX_VO_FORMAT_BGR565:
		size = w * h * 2;
		break;
	case AX_VO_FORMAT_RGB888:
	case AX_VO_FORMAT_BGR888:
		size = w * h * 3;
		break;
	}

	return size;
}

int ax_start_vo(u32 dev, u32 type, u32 sync, struct display_info *dp_info);
int fdt_fixup_vo_init_mode(int dev, void *fdt);

#endif /* __AX_VO_H */
