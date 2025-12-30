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
#include "ax620e_display_reg.h"

void display_dpi_pixel_clk_set_rate(u32 id, int sel, int div)
{
	u32 val;

	val = ((id == 0) ? (sel << COMMON_CLK_MUX_1_1X_VO0_SEL) : (sel << COMMON_CLK_MUX_1_1X_VO1_SEL)) |
	      ((id == 0) ? (sel << COMMON_CLK_MUX_1_NX_VO0_SEL) : (sel << COMMON_CLK_MUX_1_NX_VO1_SEL));
	dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_CLK_MUX_1), val);

	val = ((id == 0) ? (div << COMMON_CLK_DIV_1_NX_VO0_DIVN) : (div << COMMON_CLK_DIV_1_NX_VO1_DIVN)) |
	      ((id == 0) ? (div << COMMON_CLK_DIV_1_1X_VO0_DIVN) : (div << COMMON_CLK_DIV_1_1X_VO1_DIVN)) |
	      ((id == 0) ? (1 << COMMON_CLK_DIV_1_1X_VO0_DIVN_UPDATE) : (1 << COMMON_CLK_DIV_1_1X_VO1_DIVN_UPDATE)) |
	      ((id == 0) ? (1 << COMMON_CLK_DIV_1_NX_VO0_DIVN_UPDATE) : (1 << COMMON_CLK_DIV_1_NX_VO1_DIVN_UPDATE));
	dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_CLK_DIV_1), val);

	val = ((id == 0) ? (1 << COMMON_CLK_DIV_1_1X_VO0_DIVN_UPDATE) : (1 << COMMON_CLK_DIV_1_1X_VO1_DIVN_UPDATE)) |
	      ((id == 0) ? (1 << COMMON_CLK_DIV_1_NX_VO0_DIVN_UPDATE) : (1 << COMMON_CLK_DIV_1_NX_VO1_DIVN_UPDATE));
	dpu_writel(common_sys_glb_regs, COMMON_CLR_OFFS(COMMON_CLK_DIV_1), val);

	val = (sel << FLASH_CLK_MUX_0_1X_VO0_SEL) | (sel << FLASH_CLK_MUX_0_1X_VO1_SEL) |
	      (sel << FLASH_CLK_MUX_0_NX_VO0_SEL) | (sel << FLASH_CLK_MUX_0_NX_VO1_SEL);
	dpu_writel(flash_sys_glb_regs, FLASH_SET_OFFS(FLASH_CLK_MUX_0), val);

	val = (div << FLASH_CLK_DIV_0_1X_VO0_DIVN) | (div << FLASH_CLK_DIV_0_1X_VO1_DIVN) |
	      (1 << FLASH_CLK_DIV_0_1X_VO0_DIVN_UPDATE) | (1 << FLASH_CLK_DIV_0_1X_VO1_DIVN_UPDATE) |
	      (div << FLASH_CLK_DIV_0_NX_VO0_DIVN) | (div << FLASH_CLK_DIV_0_NX_VO1_DIVN) |
	      (1 << FLASH_CLK_DIV_0_NX_VO0_DIVN_UPDATE) | (1 << FLASH_CLK_DIV_0_NX_VO1_DIVN_UPDATE);
	dpu_writel(flash_sys_glb_regs, FLASH_SET_OFFS(FLASH_CLK_DIV_0), val);

	val = (1 << FLASH_CLK_DIV_0_1X_VO0_DIVN_UPDATE) | (1 << FLASH_CLK_DIV_0_1X_VO1_DIVN_UPDATE) |
	      (1 << FLASH_CLK_DIV_0_NX_VO0_DIVN_UPDATE) | (1 << FLASH_CLK_DIV_0_NX_VO1_DIVN_UPDATE);
	dpu_writel(flash_sys_glb_regs, FLASH_CLR_OFFS(FLASH_CLK_DIV_0), val);
}

bool rgmii_mux_sel = true;

void display_dpi_glb_path_config(u32 id, u32 type)
{
	dpu_writel(display_sys_glb_regs, DISPLAY_LVDS_CLK_SEL_SET + 0x4, 0x1);

	if (id == 0) {
		dpu_writel(common_sys_glb_regs, COMMON_CLR_OFFS(COMMON_VO_CFG), (1 << COMMON_LCD_VO_MUX_SEL));
		dpu_writel(display_sys_glb_regs, DISPLAY_LVDS_CLK_SEL_SET, 0x1);
	} else {
		if (rgmii_mux_sel || (type == AX_DISP_OUT_MODE_DPI)) { /* vo1 dpi signals can only be output from rgmii */
			dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_VO_CFG), (1 << COMMON_LCD_VO_MUX_SEL));
			dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_VO_CFG), (1 << COMMON_DPU_LITE_DMUX_SEL));
			if (type != AX_DISP_OUT_MODE_DPI)
				dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_VO_CFG), (1 << COMMON_DPU_LITE_TX_CLKING_MODE));
		} else {
			dpu_writel(common_sys_glb_regs, COMMON_CLR_OFFS(COMMON_VO_CFG), (1 << COMMON_LCD_VO_MUX_SEL));
			dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_VO_CFG), (1 << COMMON_DPU_LITE_TX_CLKING_MODE));
		}

		dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_VO_CFG), (1 << COMMON_DPU_LITE_DPHY_TX_EN));
	}

	dpu_writel(flash_sys_glb_regs, FLASH_SET_OFFS(FLASH_IMAGE_TX), (0x1 << FLASH_IMAGE_TX_EN));
	if ((type == AX_DISP_OUT_MODE_BT601) || (type == AX_DISP_OUT_MODE_BT656))
		dpu_writel(flash_sys_glb_regs, FLASH_SET_OFFS(FLASH_IMAGE_TX), (0x1 << FLASH_IMAGE_TX_CLKING_MODE));
}

void display_dpi_glb_init(u32 id)
{
	u32 val;

	/* common sys glb */
	val = ((id == 0) ? (1 << COMMON_CLK_EB_0_1X_VO0_EB) : (1 << COMMON_CLK_EB_0_1X_VO1_EB)) |
	      ((id == 0) ? (1 << COMMON_CLK_EB_0_NX_VO0_EB) : (1 << COMMON_CLK_EB_0_NX_VO1_EB));
	dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_CLK_EB_0), val);

	val = ((id == 0) ? (1 << COMMON_SW_RST_0_1X_VO0_SW_RST) : (1 << COMMON_SW_RST_0_1X_VO1_SW_RST)) |
	      ((id == 0) ? (1 << COMMON_SW_RST_0_NX_VO0_SW_RST) : (1 << COMMON_SW_RST_0_NX_VO1_SW_RST));
	dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_SW_RST_0), val);
	udelay(50);
	dpu_writel(common_sys_glb_regs, COMMON_CLR_OFFS(COMMON_SW_RST_0), val);

	/* flash sys glb */
	val = (1 << FLASH_CLK_EB_0_1X_VO0_EB) | (1 << FLASH_CLK_EB_0_1X_VO1_EB) |
	      (1 << FLASH_CLK_EB_0_NX_VO0_EB) | (1 << FLASH_CLK_EB_0_NX_VO1_EB);
	dpu_writel(flash_sys_glb_regs, FLASH_SET_OFFS(FLASH_CLK_EB_0), val);

	val = (1 << FLASH_SW_RST_0_1X_VO0_SW_RST) | (1 << FLASH_SW_RST_0_1X_VO1_SW_RST) |
	      (1 << FLASH_SW_RST_0_NX_VO0_SW_RST) | (1 << FLASH_SW_RST_0_NX_VO1_SW_RST);
	dpu_writel(flash_sys_glb_regs, FLASH_SET_OFFS(FLASH_SW_RST_0), val);
	udelay(50);
	dpu_writel(flash_sys_glb_regs, FLASH_CLR_OFFS(FLASH_SW_RST_0), val);
}