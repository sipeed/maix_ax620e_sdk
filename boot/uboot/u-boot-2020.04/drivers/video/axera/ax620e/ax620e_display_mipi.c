/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/


#include <asm/io.h>
#include <asm/arch-axera/ax620e.h>
#include <asm/arch/boot_mode.h>
#include <common.h>
#include <asm/gpio.h>
#include <div64.h>
#include <linux/kernel.h>

#include "ax_vo.h"
#include "ax620e_display_reg.h"
#include "ax620e_display_dphy_reg.h"
#include "ax_vo_common.h"

extern void __iomem *common_sys_glb_regs;
static void __iomem *common_dphytx_regs = (void __iomem *)COMMMON_DPHYTX_BASE_ADDR;

struct dphy_pll_cfg {
	u8 pll_pre_div;
	u16 pll_fbk_int;
	u32 pll_fbk_fra;

	u8 extd_cycle_sel;

	u8 dlane_hs_pre_time;
	u8 dlane_hs_zero_time;
	u8 dlane_hs_trail_time;

	u8 clane_hs_pre_time;
	u8 clane_hs_zero_time;
	u8 clane_hs_trail_time;
	u8 clane_hs_clk_pre_time;
	u8 clane_hs_clk_post_time;
};

struct ax_dphy_pll_cfg {
	unsigned long long lane_bps;
	struct dphy_pll_cfg pll_cfg;
};

static struct ax_dphy_pll_cfg ref12m_dphy_pll_cfg[] = {
	{
		.lane_bps = 79000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2b,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x71,
		}
	},
	{
		.lane_bps = 81000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x71,
		}
	},
	{
		.lane_bps = 83200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x71,
		}
	},
	{
		.lane_bps = 83250000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x71,
		}
	},
	{
		.lane_bps = 88800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x75,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xf,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x72,
		}
	},
	{
		.lane_bps = 96000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x80,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x73,
		}
	},
	{
		.lane_bps = 99000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x73,
		}
	},
	{
		.lane_bps = 99900000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x73,
		}
	},
	{
		.lane_bps = 104000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x73,
		}
	},
	{
		.lane_bps = 108000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x90,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x11,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x18,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3a,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x74,
		}
	},
	{
		.lane_bps = 111000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x11,
			.dlane_hs_zero_time = 0x23,
			.dlane_hs_trail_time = 0x18,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x74,
		}
	},
	{
		.lane_bps = 118800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9d,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x12,
			.dlane_hs_zero_time = 0x24,
			.dlane_hs_trail_time = 0x19,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x75,
		}
	},
	{
		.lane_bps = 124800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x12,
			.dlane_hs_zero_time = 0x26,
			.dlane_hs_trail_time = 0x1a,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x76,
		}
	},
	{
		.lane_bps = 124875000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x12,
			.dlane_hs_zero_time = 0x26,
			.dlane_hs_trail_time = 0x1a,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x76,
		}
	},
	{
		.lane_bps = 128000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xaa,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x13,
			.dlane_hs_zero_time = 0x26,
			.dlane_hs_trail_time = 0x1a,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x44,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x76,
		}
	},
	{
		.lane_bps = 132000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x13,
			.dlane_hs_zero_time = 0x27,
			.dlane_hs_trail_time = 0x1b,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x46,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x77,
		}
	},
	{
		.lane_bps = 133200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb1,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x13,
			.dlane_hs_zero_time = 0x28,
			.dlane_hs_trail_time = 0x1b,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x47,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x77,
		}
	},
	{
		.lane_bps = 144000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x14,
			.dlane_hs_zero_time = 0x2a,
			.dlane_hs_trail_time = 0x1c,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4c,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x78,
		}
	},
	{
		.lane_bps = 148000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x14,
			.dlane_hs_zero_time = 0x2b,
			.dlane_hs_trail_time = 0x1d,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x79,
		}
	},
	{
		.lane_bps = 148500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x14,
			.dlane_hs_zero_time = 0x2b,
			.dlane_hs_trail_time = 0x1d,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x79,
		}
	},
	{
		.lane_bps = 151050000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc9,
			.pll_fbk_fra = 0x666666,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x15,
			.dlane_hs_zero_time = 0x2b,
			.dlane_hs_trail_time = 0x1d,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x51,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x79,
		}
	},
	{
		.lane_bps = 156000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x15,
			.dlane_hs_zero_time = 0x2c,
			.dlane_hs_trail_time = 0x1e,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x7a,
		}
	},
	{
		.lane_bps = 158400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x69,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2a,
			.clane_hs_trail_time = 0xc,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3c,
		}
	},
	{
		.lane_bps = 162000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3d,
		}
	},
	{
		.lane_bps = 166400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3d,
		}
	},
	{
		.lane_bps = 166500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3d,
		}
	},
	{
		.lane_bps = 177600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3e,
		}
	},
	{
		.lane_bps = 178200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3e,
		}
	},
	{
		.lane_bps = 192000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x80,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 198000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 199000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 199800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 208000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1c,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 216000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x90,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1c,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3a,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x40,
		}
	},
	{
		.lane_bps = 222000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x40,
		}
	},
	{
		.lane_bps = 222750000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x40,
		}
	},
	{
		.lane_bps = 237600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9e,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x41,
		}
	},
	{
		.lane_bps = 249600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x42,
		}
	},
	{
		.lane_bps = 249750000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x42,
		}
	},
	{
		.lane_bps = 264000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xf,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x46,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x43,
		}
	},
	{
		.lane_bps = 266400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb1,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xf,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x47,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x43,
		}
	},
	{
		.lane_bps = 288000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x24,
			.dlane_hs_trail_time = 0x18,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4c,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x44,
		}
	},
	{
		.lane_bps = 296000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x25,
			.dlane_hs_trail_time = 0x19,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x45,
		}
	},
	{
		.lane_bps = 297000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x25,
			.dlane_hs_trail_time = 0x19,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x45,
		}
	},
	{
		.lane_bps = 300000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x11,
			.dlane_hs_zero_time = 0x25,
			.dlane_hs_trail_time = 0x19,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x50,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x45,
		}
	},
	{
		.lane_bps = 312000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x11,
			.dlane_hs_zero_time = 0x26,
			.dlane_hs_trail_time = 0x1a,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x46,
		}
	},
	{
		.lane_bps = 316800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x69,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2a,
			.clane_hs_trail_time = 0xc,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x22,
		}
	},
	{
		.lane_bps = 324000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x23,
		}
	},
	{
		.lane_bps = 332800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x15,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x23,
		}
	},
	{
		.lane_bps = 333000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x15,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x23,
		}
	},
	{
		.lane_bps = 334125000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x600000,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x23,
		}
	},
	{
		.lane_bps = 356400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x24,
		}
	},
	{
		.lane_bps = 384000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x80,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 396000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 399000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 399600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 416000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 432000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x90,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3a,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x26,
		}
	},
	{
		.lane_bps = 444000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x26,
		}
	},
	{
		.lane_bps = 445500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x26,
		}
	},
	{
		.lane_bps = 475200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9e,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x27,
		}
	},
	{
		.lane_bps = 499000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x28,
		}
	},
	{
		.lane_bps = 499200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x28,
		}
	},
	{
		.lane_bps = 499500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x28,
		}
	},
	{
		.lane_bps = 528000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x46,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x29,
		}
	},
	{
		.lane_bps = 532800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb1,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x47,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x29,
		}
	},
	{
		.lane_bps = 576000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4c,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x2a,
		}
	},
	{
		.lane_bps = 594000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x2b,
		}
	},
	{
		.lane_bps = 600000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x23,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x50,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x2b,
		}
	},
	{
		.lane_bps = 624000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xf,
			.dlane_hs_zero_time = 0x23,
			.dlane_hs_trail_time = 0x18,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x2c,
		}
	},
	{
		.lane_bps = 648000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x13,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 666000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 668250000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x600000,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x13,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 699000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x74,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 712800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x17,
		}
	},
	{
		.lane_bps = 792000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 799000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 799200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x2aaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 832000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 888000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x19,
		}
	},
	{
		.lane_bps = 891000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x19,
		}
	},
	{
		.lane_bps = 900000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x96,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x19,
		}
	},
	{
		.lane_bps = 950400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9e,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1a,
		}
	},
	{
		.lane_bps = 999000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1c,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1b,
		}
	},
	{
		.lane_bps = 1099000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb7,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x4a,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1d,
		}
	},
	{
		.lane_bps = 1136500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xbd,
			.pll_fbk_fra = 0x6aaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4b,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1d,
		}
	},
	{
		.lane_bps = 1188000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1e,
		}
	},
	{
		.lane_bps = 1200000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x50,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1e,
		}
	},
	{
		.lane_bps = 1248000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1f,
		}
	},
	{
		.lane_bps = 1299000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x12,
			.dlane_hs_trail_time = 0xd,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0xf,
		}
	},
	{
		.lane_bps = 1332000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x12,
			.dlane_hs_trail_time = 0xd,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0xf,
		}
	},
	{
		.lane_bps = 1399000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x74,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x10,
		}
	},
	{
		.lane_bps = 1425600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xc00000,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x13,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x10,
		}
	},
	{
		.lane_bps = 1500000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x7d,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x32,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x11,
		}
	},
	{
		.lane_bps = 1584000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x11,
		}
	},
	{
		.lane_bps = 1599000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x15,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x12,
		}
	},
	{
		.lane_bps = 1664000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x12,
		}
	},
	{
		.lane_bps = 1699000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8d,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x39,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x12,
		}
	},
	{
		.lane_bps = 1782000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x13,
		}
	},
	{
		.lane_bps = 1800000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x96,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x13,
		}
	},
	{
		.lane_bps = 1899000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9e,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x14,
		}
	},
	{
		.lane_bps = 1998000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x14,
		}
	},
	{
		.lane_bps = 1999000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x15,
		}
	},
	{
		.lane_bps = 2100000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xaf,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1c,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x46,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x15,
		}
	},
	{
		.lane_bps = 2199000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb7,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x4a,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 2299000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xbf,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4c,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x17,
		}
	},
	{
		.lane_bps = 2376000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x17,
		}
	},
	{
		.lane_bps = 2400000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x50,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 2496000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 2499000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x18,
		}
	},
};

static void dsi_dphy_pll_cfg(struct dphy_pll_cfg *cfg)
{
	void __iomem *regs = common_dphytx_regs;

	writel(cfg->pll_pre_div, regs + DPHY_TX0_REG2_ADDR);
	writel(cfg->pll_fbk_int, regs + DPHY_TX0_REG0_ADDR);
	writel(cfg->pll_fbk_fra, regs + DPHY_TX0_REG1_ADDR);

	writel(cfg->extd_cycle_sel, regs + DPHY_TX0_REG9_ADDR);

	writel(cfg->dlane_hs_pre_time, regs + DPHY_TX0_REG10_ADDR);
	writel(cfg->dlane_hs_zero_time, regs + DPHY_TX0_REG11_ADDR);
	writel(cfg->dlane_hs_trail_time, regs + DPHY_TX0_REG12_ADDR);

	writel(cfg->clane_hs_pre_time, regs + DPHY_TX0_REG13_ADDR);
	writel(cfg->clane_hs_zero_time, regs + DPHY_TX0_REG14_ADDR);
	writel(cfg->clane_hs_trail_time, regs + DPHY_TX0_REG15_ADDR);
	writel(cfg->clane_hs_clk_pre_time, regs + DPHY_TX0_REG16_ADDR);
	writel(cfg->clane_hs_clk_post_time, regs + DPHY_TX0_REG17_ADDR);
}

static void dsi_dphy_config(int lanes, unsigned long long lane_bps)
{
	int i;
	u32 lane_mask;
	int board_id;

	VO_INFO("%s enter, lanes = %d, lane_bps = %lld\n",
		__func__, lanes, lane_bps);

	for (i = 0, lane_mask = 0; i < lanes; i++) {
		lane_mask |= (1 << i);
	}

	/* leave isolation */
	writel(DPHYTX0_AON_POWER_READY_N_BIT, common_sys_glb_regs + DPHY_POWER_READY_CLR_ADDR);
	/* power on */
	writel(DPHYTX0_PWR_OFF_BIT, common_sys_glb_regs + DPHY_POWER_OFF_CLR_ADDR);
	udelay(20);

	board_id = get_board_id();
// ### SIPEED EDIT ###
	if (board_id == AX630C_AX631_MAIXCAM2_SOM_0_5G || board_id == AX630C_AX631_MAIXCAM2_SOM_1G
		|| board_id == AX630C_AX631_MAIXCAM2_SOM_2G || board_id == AX630C_AX631_MAIXCAM2_SOM_4G
		|| board_id == AX630C_DEMO_LP4_V1_0 || board_id == AX630C_DEMO_V1_1 || board_id == AX620Q_LP4_DEMO_V1_1) {
		writel(1, common_dphytx_regs + DPHY_TX0_REG22_ADDR);
		writel(0, common_dphytx_regs + DPHY_TX0_REG23_ADDR);
		writel(4, common_dphytx_regs + DPHY_TX0_REG24_ADDR);
	}
// ### SIPEED EDIT END ###
	writel(1, common_dphytx_regs + DPHY_PPI_REG_2_SET_ADDR);
	writel(lane_mask, common_dphytx_regs + DPHY_PPI_REG_3_SET_ADDR);
	writel(1, common_dphytx_regs + DPHY_MIPITX0_EN_SET_ADDR);

	for (i = 0; sizeof(ref12m_dphy_pll_cfg) / sizeof(ref12m_dphy_pll_cfg[0]); i++) {
		if (ref12m_dphy_pll_cfg[i].lane_bps >= lane_bps) {
			VO_INFO("%s expected_lane_bps = %lld, actual_lane_bps = %lld\n",
				__func__, lane_bps, ref12m_dphy_pll_cfg[i].lane_bps);

			dsi_dphy_pll_cfg(&ref12m_dphy_pll_cfg[i].pll_cfg);

			break;
		}
	}
}
extern void cdns_dsi_config_enable(struct ax_disp_mode *mode);
extern int display_mipi_cdns_init(void);
extern int mipi_dsi_pixel_format_to_bpp(enum mipi_dsi_pixel_format fmt);
void display_mipi_dphy_config(int clk)
{
	u32 val;
	u32 bpp;
	unsigned long long lane_bps;
	int lanes = g_dsi_panel.nlanes;
	bpp = mipi_dsi_pixel_format_to_bpp(g_dsi_panel.format);
	lane_bps = DIV_ROUND_DOWN_ULL(clk * bpp, lanes);
	dsi_dphy_config(lanes, lane_bps);
	writel((DISPLAY_CLK_MUX_0_SEL_416M), display_sys_glb_regs + DISPLAY_CLK_MUX_0_SET);
	val = (1 << DISPLAY_DSI_DSI0_MODE) | (1 << DISPLAY_DSI_DSI0_DPHY_PLL_LOCK) | (1 << DISPLAY_DSI_PPI_C_TX_READY_HS0);
	writel(val, display_sys_glb_regs + DISPLAY_DSI_SET);
	writel(1, display_sys_glb_regs + DISPLAY_LVDS_CLK_SEL_SET);
	writel(1, display_sys_glb_regs + DISPLAY_DSI_AXI2CSI_SHARE_MEM_SEL_SET);
	udelay(100);
	val = 1 << DISPLAY_SW_RST_0_DPHYTX_SW_RST;
	writel(val, display_sys_glb_regs + DISPLAY_SW_RST_0_CLR);
	VO_DEBUG("done\n");
}

void display_mipi_cdns_config(struct ax_disp_mode *mode)
{
	cdns_dsi_config_enable(mode);
}

void display_mipi_glb_init(void)
{
	u32 val;

	val = (1 << DISPLAY_SW_RST_0_PRST_DSI_SW_RST) | (1 << DISPLAY_SW_RST_0_DSI_TX_PIX_SW_RST) | (1 << DISPLAY_SW_RST_0_DSI_TX_ESC_SW_RST) |
		(1 << DISPLAY_SW_RST_0_DSI_SYS_SW_RST) | (1 << DISPLAY_SW_RST_0_DSI_RX_ESC_SW_RST) | (1 << DISPLAY_SW_RST_0_DPHY2DSI_SW_RST);
	dpu_writel(display_sys_glb_regs, DISPLAY_SW_RST_0_CLR, val);

	val = (1 << DISPLAY_CLK_EB_0_DPHY_TX_REF_EB) | (1 << DISPLAY_CLK_EB_0_DPHY_TX_ESC_EB);
	dpu_writel(display_sys_glb_regs, DISPLAY_CLK_EB_0_SET, val);

	val = (1 << DISPLAY_CLK_EB_1_DSI_EB) | (1 << DISPLAY_CLK_EB_1_DSI_TX_ESC_EB) |
		(1 << DISPLAY_CLK_EB_1_DSI_SYS_EB) | (1 << DISPLAY_CLK_EB_1_DPHY2DSI_HS_EB);
	dpu_writel(display_sys_glb_regs, DISPLAY_CLK_EB_1_SET, val);

	/* common sys glb */
	val = (1 << COMMON_CLK_EB_0_DPHYTX_TLB_EB);
	dpu_writel(common_sys_glb_regs, COMMON_SET_OFFS(COMMON_CLK_EB_0), val);

	display_mipi_cdns_init();
	VO_DEBUG("done\n");
}

extern ssize_t cdns_dsi_transfer(u8 type, void *tx_buf,size_t tx_len);
static int panel_send_dsi_cmds(u8 *data, int len)
{
	int i, delay, cmd_len, ret;
	for (i = 0; i < len; i += (cmd_len + 3)) {
		delay = data[i + 1];
		cmd_len = data[i + 2];
		if (cmd_len + i + 3 > len) {
			VO_ERROR("cmd overflow\n");
			return -EINVAL;
		}

		ret = cdns_dsi_transfer(data[i], &data[i + 3], cmd_len);
		if (ret < 0) {
			VO_ERROR("dcs send failed\n");
			return ret;
		}

		if (delay)
			mdelay(delay);
	}

	VO_DEBUG("dcs send success\n");
	return 0;
}

int display_mipi_panel_init(void)
{
// ### SIPEED EDIT ###
	int ret;
	int num_init_seqs = g_dsi_panel.init_seq_len;
	struct udevice *dev = NULL;
	ret = gpio_request(g_dsi_panel.gpio_num, "mipi_panel");
	if(ret < 0)
	{
		VO_ERROR("gpio request error\n");
		goto exit;
	}

	if (g_dsi_panel.pwm_period != 0) {
		uclass_get_device(UCLASS_PWM, g_dsi_panel.pwms/4, &dev);
		pwm_set_config(dev, g_dsi_panel.pwms%4, g_dsi_panel.pwm_period, g_dsi_panel.pwm_duty);
		pwm_set_enable(dev, g_dsi_panel.pwms%4, true);
	}
// ### SIPEED EDIT END ###
	ret = gpio_direction_output(g_dsi_panel.gpio_num, 0);
	if(ret < 0)
	{
		VO_ERROR("gpio direction error\n");
		goto exit;
	}

	gpio_set_value(g_dsi_panel.gpio_num, 0);
	gpio_set_value(g_dsi_panel.gpio_num, 1);

	mdelay(g_dsi_panel.reset_delay_ms);

	if (num_init_seqs) {
		ret = panel_send_dsi_cmds(g_dsi_panel.panel_init_seq, num_init_seqs);
		if (ret) {
			VO_ERROR("dsi send cmd fail\n");
			goto exit;
		}
	}

	ret = 0;
	VO_DEBUG("done\n");
exit:
	return ret;
}
