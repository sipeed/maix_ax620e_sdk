/*
 * (C) Copyright 2023 AXERA Co., Ltd
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <asm/io.h>
#include <linux/bitops.h>
#include <asm/arch-axera/ax620e.h>
#include <asm/arch/boot_mode.h>
// ### SIPEED EDIT ###
#include <init.h>
#include <linux/delay.h>
// ### SIPEED EDIT END ###


#define DPHYTX_BASE           0x230A000UL
#define DPHY_REG_LEN          0x1000
#define DPHYTX_SW_RST_SET     0x46000B8
#define DPHYTX_SW_RST_SHIFT   BIT(6)
#define DPHYTX_MIPI_EN        0x23F110C
#define PINMUX_FUNC_SEL       GENMASK(18, 16)

static unsigned int ax620Q_EVB_pinmux[] = {
#include "AX620Q_EVB_pinmux.h"
};

static unsigned int ax620Q_Demo_pinmux[] = {
#include "AX620Q_DEMO_pinmux.h"
};

static unsigned int ax630C_EVB_pinmux[] = {
#include "AX630C_EVB_pinmux.h"
};

static unsigned int ax630C_Demo_pinmux[] = {
#include "AX630C_DEMO_pinmux.h"
};

static unsigned int ax620QZ_Demo_pinmux[] = {
#include "AX620QZ_DEMO_pinmux.h"
};

struct pinmux {
	unsigned int *data;
	unsigned int size;
};

static struct pinmux ax620E_pinmux_tbl[AX620E_BOARD_MAX] = {
	[AX620Q_LP4_EVB_V1_0] =
	    {ax620Q_EVB_pinmux,
	     sizeof(ax620Q_EVB_pinmux) / sizeof(unsigned int)},
	[AX620Q_LP4_DEMO_V1_0] =
	    {ax620Q_Demo_pinmux,
	     sizeof(ax620Q_Demo_pinmux) / sizeof(unsigned int)},
	[AX630C_EVB_V1_0] =
	    {ax630C_EVB_pinmux,
	     sizeof(ax630C_EVB_pinmux) / sizeof(unsigned int)},
	[AX630C_DEMO_V1_0] =
	    {ax630C_Demo_pinmux,
	     sizeof(ax630C_Demo_pinmux) / sizeof(unsigned int)},
	[AX630C_DEMO_V1_1] =
	    {ax630C_Demo_pinmux,
	     sizeof(ax630C_Demo_pinmux) / sizeof(unsigned int)},
	[AX620QZ_DEMO_LP4_V1_0] =
	    {ax620QZ_Demo_pinmux,
	     sizeof(ax620QZ_Demo_pinmux) / sizeof(unsigned int)},
};

static int ax_pinmux_index_conv(int index)
{
	int ret;

	switch (index) {
	case AX630C_DEMO_DDR3_V1_0:
	// ### SIPEED EDIT ###
	case AX630C_AX631_MAIXCAM2_SOM_0_5G:
	case AX630C_AX631_MAIXCAM2_SOM_1G:
	case AX630C_AX631_MAIXCAM2_SOM_2G:
	case AX630C_AX631_MAIXCAM2_SOM_4G:
	case AX630C_DEMO_LP4_V1_0:
	case AX630C_DEMO_V1_1:
		ret = AX630C_DEMO_V1_0;
		break;
	case AX620Q_LP4_DEMO_V1_1:
	case AX620Q_LP4_NANOAGENT_256M:
		ret = AX620Q_LP4_DEMO_V1_0;
		break;
	case AX620QE_LP4_NANOAGENT_512M:
	case AX620QF_LP4_NANOAGENT_256M:
		ret = AX620QZ_DEMO_LP4_V1_0;
		break;
	// ### SIPEED EDIT END ###
	default :
		ret = index;
		break;
	}
	return ret;
}

int pinmux_init(void)
{
	int i;
	int index = get_board_id();
	u8 is_dphytx;

	// ### SIPEED EDIT ###
	int match_count = 0;
	int uart3_short = 0;
	int rx_high = 0;
	int rx_low = 0;
	u32 val;

	misc_info_t *info = (misc_info_t *)MISC_INFO_ADDR;
	if ((info->chip_type == 0x8 && info->board_id == 0x3 &&
	     info->phy_board_id == 0x3) ||
	    (info->chip_type == 0x9 && info->board_id == 0x1 &&
	     info->phy_board_id == 0x1)) {
		timer_init();

		val = readl(0x0480100C);
		val = (val | BIT(1)) & ~BIT(0);
		writel(val, 0x0480100C);
		clrbits_le32(0x04801010, BIT(1));

		writel(0x00060043, 0x02304090);
		writel(0x00060003, 0x02304084);

		for (i = 0; i < 3; i++) {
			clrbits_le32(0x0480100C, BIT(0));
			udelay(10);
			rx_low = !(readl(0x0480108C) & BIT(3));
			setbits_le32(0x0480100C, BIT(0));
			udelay(10);
			rx_high = !!(readl(0x0480108C) & BIT(3));
			if (rx_low && rx_high)
				match_count++;
			clrbits_le32(0x0480100C, BIT(0));
			if (i < 2)
				udelay(30000);
		}
		writel(0x00060043, 0x02304084);
		writel(0x00060043, 0x02304090);
		clrbits_le32(0x0480100C, BIT(1));
		uart3_short = match_count == 3;
	}
	// ### SIPEED EDIT END ###

	if (index < 0 || index > AX620E_BOARD_MAX - 1)
		return 0;
	index = ax_pinmux_index_conv(index);

	for (i = 0; i < ax620E_pinmux_tbl[index].size; i += 2) {
		// ### SIPEED EDIT ###
		if (uart3_short &&
		    (ax620E_pinmux_tbl[index].data[i] == 0x02304084 ||
		     ax620E_pinmux_tbl[index].data[i] == 0x02304090))
			continue;
		// ### SIPEED EDIT END ###

		is_dphytx = ax620E_pinmux_tbl[index].data[i] - DPHYTX_BASE < DPHY_REG_LEN ? 1 : 0;
		//when dphytx select gpio func 1.set reset 2.mipi disable 3.func sel & config
		if (is_dphytx && (ax620E_pinmux_tbl[index].data[i + 1] & PINMUX_FUNC_SEL)) {
			writel(DPHYTX_SW_RST_SHIFT, DPHYTX_SW_RST_SET);
			writel(0, DPHYTX_MIPI_EN);
		}
		writel(ax620E_pinmux_tbl[index].data[i + 1],
			(long)ax620E_pinmux_tbl[index].data[i]);
	}

	return 0;
}
