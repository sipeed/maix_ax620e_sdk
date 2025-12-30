/*
 * (C) Copyright 2023 AXERA Co., Ltd
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <asm/io.h>
#include <linux/bitops.h>
#include <asm/arch-axera/ax620e.h>
#include <asm/arch/boot_mode.h>

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
};

static int ax_pinmux_index_conv(int index)
{
	int ret;

	switch (index) {
	// ### SIPEED EDIT ###
	case AX630C_AX631_MAIXCAM2_SOM_0_5G:
	case AX630C_AX631_MAIXCAM2_SOM_1G:
	case AX630C_AX631_MAIXCAM2_SOM_2G:
	case AX630C_AX631_MAIXCAM2_SOM_4G:
	// ### SIPEED EDIT END ###
	case AX630C_DEMO_DDR3_V1_0:
	case AX630C_DEMO_LP4_V1_0:
	case AX630C_DEMO_V1_1:
		ret = AX630C_DEMO_V1_0;
		break;
	case AX620Q_LP4_DEMO_V1_1:
		ret = AX620Q_LP4_DEMO_V1_0;
		break;
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

	if (index < 0 || index > AX620E_BOARD_MAX - 1)
		return 0;
	index = ax_pinmux_index_conv(index);

	for (i = 0; i < ax620E_pinmux_tbl[index].size; i += 2) {
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
