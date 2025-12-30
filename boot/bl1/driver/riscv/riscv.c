/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "chip_reg.h"
#include "riscv_sw_int.h"
#include "cmn.h"
#include "riscv.h"
#include "ax_timestamp.h"
#include "boot.h"

void riscv_boot_up(void)
{
	/* reset riscv */
	writel(BIT(16), COMM_SYS_GLB_SW_RST_0_SET);
	/* riscv_base_addr */
	writel(RISCV_BIN_DDR_START, COMM_SYS_GLB_RISCV_BASE_ADDR);
	/* clk_riscv_eb set */
	writel(BIT(8), COMM_SYS_GLB_CLK_EB_1_SET);
	/* riscv_sw_rst clear field */
	writel(BIT(17), COMM_SYS_GLB_SW_RST_0_CLR);
	/* clk_riscv_bus_sel set cpll_416m */
	writel(BIT(25), COMM_SYS_GLB_CLK_MUX1_SET);
	/* riscv_hold_sw_rst clear field */
	writel(BIT(16), COMM_SYS_GLB_SW_RST_0_CLR);
}

void riscv_start_load_rootfs(void)
{
	ax_timestamp(STAMP_SPL_WAIT_RISCV_READY);
	/* If riscv fails to start, wdt is restarted after a timeout. */
	writel(BOOT_KERNEL_FAIL, TOP_CHIPMODE_GLB_BACKUP0);
	while (1) {
		if (readl(COMM_SYS_DUMMY_SW4) & SW4_LOAD_ROOTFS_READY) {
			break;
		}
	}
	ax_timestamp(STAMP_SPL_RISCV_READY);
	riscv_sw_int_trigger(sw_int_group_3, sw_int_channel_29);
}

