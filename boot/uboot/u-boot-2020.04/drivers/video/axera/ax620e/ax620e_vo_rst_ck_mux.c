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
#include <asm/io.h>

#include "ax_vo.h"
#include "ax620e_display_reg.h"
#include "ax620e_display_dpi.h"
#include "ax620e_display_mipi.h"

#define SRC_CLK_NUM 4
#define CLK_DIV_NUM 16

#define VPLL_297 297000000
#define VPLL_198 198000000
#define VPLL_118P8 118800000
#define VPLL_108 108000000

static void __iomem *mm_sys_glb_regs = (void __iomem *)MM_SYS_GLB_BASE_ADDR;
void __iomem *common_sys_glb_regs = (void __iomem *)COMMON_SYS_GLB_BASE_ADDR;
void __iomem *flash_sys_glb_regs = (void __iomem *)FLASH_SYS_GLB_BASE_ADDR;
void __iomem *display_sys_glb_regs = (void __iomem *)DISPLAY_SYS_GLB_BASE_ADDR;

static int supported_dpi_clk[SRC_CLK_NUM][CLK_DIV_NUM] = {0};
static int pixel_clk_sel(int clk, int *sel, int *div)
{
	int i, j;
	int diff = -1, min = VPLL_108 / CLK_DIV_NUM;

	*sel = -1;
	*div = -1;

	for (i = 0; i < SRC_CLK_NUM; i++) {
		for (j = 0; j < CLK_DIV_NUM; j++) {
			if (clk >= supported_dpi_clk[i][j]) {
				diff = clk - supported_dpi_clk[i][j];
				if (!diff) {
					*sel = i;
					*div = j;
					goto exit;
				}

				if (diff < min) {
					*sel = i;
					*div = j;
					min = diff;
				}
			}
		}
	}

exit:
	if (*sel < 0) {
		VO_ERROR("unsupported frequency point, clk = %d\n", clk);
		return -EINVAL;
	}

	VO_DEBUG("expected clk: %d, actual clk: %d, sel: %d, div: %d\n",
	         clk, supported_dpi_clk[i][j], i, j);
	return 0;
}

static int pixel_clk_set_rate(u32 id, int clk, u32 out_mode)
{
	int ret;
	int sel = 0,div = 0;
	u32 val;

	ret = pixel_clk_sel(clk, &sel, &div);
	if (ret) {
		VO_ERROR("set clk fail\n");
		return ret;
	}

	val = (id == 0) ? (sel << MM_CLK_MUX_0_DPU_OUT_SEL) : (sel << MM_CLK_MUX_0_DPU_LITE_OUT_SEL);
	dpu_writel(mm_sys_glb_regs, MM_SET_OFFS(MM_CLK_MUX_0), val);

	val = ((id == 0) ? (div << MM_CLK_DIV_0_DPU_OUT_DIVN) : (div << MM_CLK_DIV_0_DPU_LITE_OUT_DIVN)) |
	      ((id == 0) ? (1 << MM_CLK_DIV_0_DPU_OUT_DIVN_UPDATE) : (1 << MM_CLK_DIV_0_DPU_LITE_OUT_DIVN_UPDATE));
	dpu_writel(mm_sys_glb_regs, MM_SET_OFFS(MM_CLK_DIV_0), val);
	dpu_writel(mm_sys_glb_regs, MM_CLR_OFFS(MM_CLK_DIV_0), (id == 0) ? (1 << MM_CLK_DIV_0_DPU_OUT_DIVN_UPDATE) : (1 << MM_CLK_DIV_0_DPU_LITE_OUT_DIVN_UPDATE));

	switch (out_mode) {
	case AX_DISP_OUT_MODE_BT601:
	case AX_DISP_OUT_MODE_BT656:
	case AX_DISP_OUT_MODE_BT1120:
	case AX_DISP_OUT_MODE_DPI:
		display_dpi_pixel_clk_set_rate(id, sel, div);
		break;
	case AX_DISP_OUT_MODE_DSI_DPI_VIDEO:
		display_mipi_dphy_config(clk);
		break;
	default:
		VO_ERROR("unsupported out mode(%d)\n", out_mode);
		return -EINVAL;
	}

	return 0;
}

int pixel_clk_get_rate(u32 id)
{
	return 0;
}

static void pixel_clk_init(void)
{
	int i, j;
	int src[SRC_CLK_NUM] = {VPLL_108, VPLL_118P8, VPLL_198, VPLL_297};

	for (i = 0; i < SRC_CLK_NUM; i++) {
		for (j = 0; j < CLK_DIV_NUM; j++) {
			if (j == 0)
				supported_dpi_clk[i][j] = src[i];
			else
				supported_dpi_clk[i][j] = src[i] / (j * 2);

			VO_DEBUG("supported_dpi_clk[%d][%d] = %d\n", i, j, supported_dpi_clk[i][j]);
		}
	}
}

int display_glb_path_config(u32 id, u32 out_mode, struct ax_disp_mode *mode)
{
	int ret = 0, mul = 1;

	if (!(mode->flags & MODE_FLAG_INTERLACE) && ((out_mode == AX_DISP_OUT_MODE_BT601) || (out_mode == AX_DISP_OUT_MODE_BT656)))
		mul = 2;

	ret = pixel_clk_set_rate(id, mode->clock * 1000 * mul, out_mode);
	if (ret) {
		VO_ERROR("dpu%d mode(%d) set clk failed\n", id, out_mode);
		return -1;
	}

	switch (out_mode) {
	case AX_DISP_OUT_MODE_BT601:
	case AX_DISP_OUT_MODE_BT656:
	case AX_DISP_OUT_MODE_BT1120:
	case AX_DISP_OUT_MODE_DPI:
		display_dpi_glb_path_config(id, out_mode);
		break;

	case AX_DISP_OUT_MODE_DSI_DPI_VIDEO:
		display_mipi_panel_init();
		display_mipi_cdns_config(mode);
		break;

	case OUT_MODE_DSI_SDI_VIDEO:
	case OUT_MODE_DSI_SDI_CMD:
	case OUT_MODE_LVDS:
		break;
	default:
		VO_ERROR("unsupported dpu%d out mode(%d)\n", id, out_mode);
		return -EINVAL;
	}

	return 0;
}

void dpu_glb_init(u32 id)
{
	u32 val;

	pixel_clk_init();

	/* mm sys glb */
	val = ((id == 0) ? (1 << MM_CLK_EB_0_DPU_OUT_EB) : (1 << MM_CLK_EB_0_DPU_LITE_OUT_EB));
	dpu_writel(mm_sys_glb_regs, MM_SET_OFFS(MM_CLK_EB_0), val);

	val = (1 << MM_CLK_EB_1_CLK_CMD_EB) | (1 << MM_CLK_EB_1_PCLK_CMD_EB) |
	      ((id == 0) ? (1 << MM_CLK_EB_1_CLK_DPU_EB) : (1 << MM_CLK_EB_1_CLK_DPU_LITE_EB)) |
	      ((id == 0) ? (1 << MM_CLK_EB_1_PCLK_DPU_EB) : (1 << MM_CLK_EB_1_PCLK_DPU_LITE_EB));
	dpu_writel(mm_sys_glb_regs, MM_SET_OFFS(MM_CLK_EB_1), val);

	val = (1 << MM_SW_RST_0_CMD_SW_PRST) | (1 << MM_SW_RST_0_CMD_SW_RST) |
	      ((id == 0) ? (1 << MM_SW_RST_0_DPU_OUT_SW_PRST) : (1 << MM_SW_RST_0_DPU_LITE_OUT_SW_PRST)) |
	      ((id == 0) ? (1 << MM_SW_RST_0_DPU_SW_PRST) : (1 << MM_SW_RST_0_DPU_LITE_SW_PRST)) |
	      ((id == 0) ? (1 << MM_SW_RST_0_DPU_SW_RST) : (1 << MM_SW_RST_0_DPU_LITE_SW_RST));
	dpu_writel(mm_sys_glb_regs, MM_SET_OFFS(MM_SW_RST_0), val);
	udelay(50);
	dpu_writel(mm_sys_glb_regs, MM_CLR_OFFS(MM_SW_RST_0), val);

	val = ((id == 0) ? (MM_CLK_SEL_533M << MM_CLK_MUX_0_DPU_SRC_SEL) : (MM_CLK_SEL_533M << MM_CLK_MUX_0_DPU_LITE_SRC_SEL)) |
	      (MM_CLK_SEL_533M << MM_CLK_MUX_0_MM_GLB_SEL);
	dpu_writel(mm_sys_glb_regs, MM_SET_OFFS(MM_CLK_MUX_0), val);

}

void display_glb_init(u32 id, u32 type)
{
	switch (type) {
	case AX_DISP_OUT_MODE_BT601:
	case AX_DISP_OUT_MODE_BT656:
	case AX_DISP_OUT_MODE_BT1120:
	case AX_DISP_OUT_MODE_DPI:
		display_dpi_glb_init(id);
		break;
	case OUT_MODE_DSI_DPI_VIDEO:
		display_mipi_glb_init();
		break;
	default:
		VO_ERROR("unsupported dpu%d out mode(%d)\n", id, type);
		break;
	}
}
