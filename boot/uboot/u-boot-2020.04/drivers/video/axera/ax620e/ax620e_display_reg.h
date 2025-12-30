/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX620E_DISPLAY_REG_H__
#define __AX620E_DISPLAY_REG_H__

#define MM_CLK_MUX_0				0x0
#define MM_CLK_MUX_0_MM_GLB_SEL			21
#define MM_CLK_MUX_0_DPU_SRC_SEL		12
#define MM_CLK_MUX_0_DPU_SRC_SEL_MASK		0x7
#define MM_CLK_MUX_0_DPU_OUT_SEL		10
#define MM_CLK_MUX_0_DPU_OUT_SEL_MASK		0x3
#define MM_CLK_MUX_0_DPU_LITE_SRC_SEL		7
#define MM_CLK_MUX_0_DPU_LITE_SEL_MASK		0x7
#define MM_CLK_MUX_0_DPU_LITE_OUT_SEL		5
#define MM_CLK_MUX_0_DPU_LITE_OUT_SEL_MASK	0x3

#define MM_CLK_SEL_533M				0x5

#define MM_CLK_EB_0				0x4
#define MM_CLK_EB_0_DPU_OUT_EB			2
#define MM_CLK_EB_0_DPU_LITE_OUT_EB		1

#define MM_CLK_EB_1				0x8
#define MM_CLK_EB_1_PCLK_DPU_LITE_EB		21
#define MM_CLK_EB_1_PCLK_DPU_EB			20
#define MM_CLK_EB_1_PCLK_CMD_EB			19
#define MM_CLK_EB_1_CLK_DPU_LITE_EB		5
#define MM_CLK_EB_1_CLK_DPU_EB			4
#define MM_CLK_EB_1_CLK_CMD_EB			3

#define MM_CLK_DIV_0				0xC
#define MM_CLK_DIV_0_DPU_OUT_DIVN_UPDATE	9
#define MM_CLK_DIV_0_DPU_OUT_DIVN		5
#define MM_CLK_DIV_0_DPU_LITE_OUT_DIVN_UPDATE	4
#define MM_CLK_DIV_0_DPU_LITE_OUT_DIVN		0

#define MM_SW_RST_0				0x10
#define MM_SW_RST_0_DPU_SW_RST			12
#define MM_SW_RST_0_DPU_SW_PRST			11
#define MM_SW_RST_0_DPU_OUT_SW_PRST		10
#define MM_SW_RST_0_DPU_LITE_SW_RST		9
#define MM_SW_RST_0_DPU_LITE_SW_PRST		8
#define MM_SW_RST_0_DPU_LITE_OUT_SW_PRST	7
#define MM_SW_RST_0_CMD_SW_RST			5
#define MM_SW_RST_0_CMD_SW_PRST			4


#define MM_SET_OFFS(OFFS)			(((OFFS) >> 2) * 8 + 0xA4)
#define MM_CLR_OFFS(OFFS)			(((OFFS) >> 2) * 8 + 0xA8)

#define FLASH_CLK_MUX_0				0x0
#define FLASH_CLK_MUX_0_NX_VO1_SEL		14
#define FLASH_CLK_MUX_0_NX_VO1_SEL_MASK		0x3
#define FLASH_CLK_MUX_0_NX_VO0_SEL		12
#define FLASH_CLK_MUX_0_NX_VO0_SEL_MASK		0x3
#define FLASH_CLK_MUX_0_1X_VO1_SEL		2
#define FLASH_CLK_MUX_0_1X_VO1_SEL_MASK		0x3
#define FLASH_CLK_MUX_0_1X_VO0_SEL		0
#define FLASH_CLK_MUX_0_1X_VO0_SEL_MASK		0x3

#define FLASH_CLK_EB_0				0x4
#define FLASH_CLK_EB_0_NX_VO1_EB		8
#define FLASH_CLK_EB_0_NX_VO0_EB		7
#define FLASH_CLK_EB_0_1X_VO1_EB		1
#define FLASH_CLK_EB_0_1X_VO0_EB		0

#define FLASH_CLK_DIV_0				0xC
#define FLASH_CLK_DIV_0_NX_VO1_DIVN_UPDATE	19
#define FLASH_CLK_DIV_0_NX_VO1_DIVN		15
#define FLASH_CLK_DIV_0_NX_VO0_DIVN_UPDATE	14
#define FLASH_CLK_DIV_0_NX_VO0_DIVN		10
#define FLASH_CLK_DIV_0_1X_VO1_DIVN_UPDATE	9
#define FLASH_CLK_DIV_0_1X_VO1_DIVN		5
#define FLASH_CLK_DIV_0_1X_VO0_DIVN_UPDATE	4
#define FLASH_CLK_DIV_0_1X_VO0_DIVN		0

#define FLASH_SW_RST_0				0x14
#define FLASH_SW_RST_0_NX_VO1_SW_RST		29
#define FLASH_SW_RST_0_1X_VO1_SW_RST		28
#define FLASH_SW_RST_0_NX_VO0_SW_RST		27
#define FLASH_SW_RST_0_1X_VO0_SW_RST		26

#define FLASH_IMAGE_TX				0x1B8
#define FLASH_IMAGE_TX_CLKING_MODE		5
#define FLASH_IMAGE_TX_DLY_SEL			1
#define FLASH_IMAGE_TX_EN			0

#define FLASH_SET_OFFS(OFFS)			((OFFS) + 0x4000)
#define FLASH_CLR_OFFS(OFFS)			((OFFS) + 0x8000)

#define COMMON_CLK_MUX_1			0xC
#define COMMON_CLK_MUX_1_NX_VO1_SEL		22
#define COMMON_CLK_MUX_1_NX_VO1_SEL_MASK	0x3
#define COMMON_CLK_MUX_1_NX_VO0_SEL		20
#define COMMON_CLK_MUX_1_NX_VO0_SEL_MASK	0x3
#define COMMON_CLK_MUX_1_1X_VO1_SEL		14
#define COMMON_CLK_MUX_1_1X_VO1_SEL_MASK	0x3
#define COMMON_CLK_MUX_1_1X_VO0_SEL		12
#define COMMON_CLK_MUX_1_1X_VO0_SEL_MASK	0x3

#define COMMON_CLK_EB_0				0x24
#define COMMON_CLK_EB_0_NX_VO1_EB		13
#define COMMON_CLK_EB_0_NX_VO0_EB		12
#define COMMON_CLK_EB_0_DPHYTX_TLB_EB		9
#define COMMON_CLK_EB_0_1X_VO1_EB		7
#define COMMON_CLK_EB_0_1X_VO0_EB		6

#define COMMON_CLK_DIV_1			0x48
#define COMMON_CLK_DIV_1_NX_VO1_DIVN_UPDATE	19
#define COMMON_CLK_DIV_1_NX_VO1_DIVN		15
#define COMMON_CLK_DIV_1_NX_VO0_DIVN_UPDATE	14
#define COMMON_CLK_DIV_1_NX_VO0_DIVN		10
#define COMMON_CLK_DIV_1_1X_VO1_DIVN_UPDATE	9
#define COMMON_CLK_DIV_1_1X_VO1_DIVN		5
#define COMMON_CLK_DIV_1_1X_VO0_DIVN_UPDATE	4
#define COMMON_CLK_DIV_1_1X_VO0_DIVN		0

#define COMMON_SW_RST_0				0x54
#define COMMON_SW_RST_0_NX_VO1_SW_RST		29
#define COMMON_SW_RST_0_1X_VO1_SW_RST		28
#define COMMON_SW_RST_0_NX_VO0_SW_RST		27
#define COMMON_SW_RST_0_1X_VO0_SW_RST		26

#define COMMON_VO_CFG				0x424
#define COMMON_DPU_LITE_TX_DLY_SEL		5
#define COMMON_DPU_LITE_TX_CLKING_MODE		3
#define COMMON_DPU_LITE_DPHY_TX_EN		2
#define COMMON_DPU_LITE_DMUX_SEL		1
#define COMMON_LCD_VO_MUX_SEL			0

#define COMMON_SET_OFFS(OFFS)			((OFFS) + 0x4)
#define COMMON_CLR_OFFS(OFFS)			((OFFS) + 0x8)

#define DISPLAY_CLK_MUX_0			0x0
#define DISPLAY_CLK_EB_0			0x4
#define DISPLAY_CLK_EB_1			0x8
#define DISPLAY_SW_RST_0			0xC
#define DISPLAY_DSI				0x14
#define DISPLAY_DSI_SDI_CLK_SEL			0x48
#define DISPLAY_LVDS_CLK_SEL			0x4C

#define DISPLAY_CLK_MUX_0_SET			0xA0
#define DISPLAY_CLK_MUX_0_CLR			0xA4
#define DISPLAY_CLK_MUX_0_SEL_24M		0
#define DISPLAY_CLK_MUX_0_SEL_100M		1
#define DISPLAY_CLK_MUX_0_SEL_208M		2
#define DISPLAY_CLK_MUX_0_SEL_312M		3
#define DISPLAY_CLK_MUX_0_SEL_416M		4

#define DISPLAY_CLK_EB_0_SET			0xA8
#define DISPLAY_CLK_EB_0_CLR			0xAC
#define DISPLAY_CLK_EB_0_DPHY_TX_REF_EB		1
#define DISPLAY_CLK_EB_0_DPHY_TX_ESC_EB		0

#define DISPLAY_CLK_EB_1_SET			0xB0
#define DISPLAY_CLK_EB_1_CLR			0xB4
#define DISPLAY_CLK_EB_1_DSI_EB			8
#define DISPLAY_CLK_EB_1_DSI_TX_ESC_EB		6
#define DISPLAY_CLK_EB_1_DSI_SYS_EB		5
#define DISPLAY_CLK_EB_1_DPHY2DSI_HS_EB		2

#define DISPLAY_SW_RST_0_SET			0xB8
#define DISPLAY_SW_RST_0_CLR			0xBC
#define DISPLAY_SW_RST_0_PRST_DSI_SW_RST	12
#define DISPLAY_SW_RST_0_DSI_TX_PIX_SW_RST	10
#define DISPLAY_SW_RST_0_DSI_TX_ESC_SW_RST	9
#define DISPLAY_SW_RST_0_DSI_SYS_SW_RST		8
#define DISPLAY_SW_RST_0_DSI_RX_ESC_SW_RST	7
#define DISPLAY_SW_RST_0_DPHYTX_SW_RST		6
#define DISPLAY_SW_RST_0_DPHY2DSI_SW_RST	3

#define DISPLAY_DSI_SET				0xC0
#define DISPLAY_DSI_CLR				0xC4
#define DISPLAY_DSI_PPI_C_TX_READY_HS0		8
#define DISPLAY_DSI_DSI0_DPHY_PLL_LOCK		4
#define DISPLAY_DSI_DSI0_MODE			0

#define DISPLAY_LVDS_CLK_SEL_SET		0x120
#define DISPLAY_DSI_AXI2CSI_SHARE_MEM_SEL_SET	0x178

extern void __iomem *common_sys_glb_regs;
extern void __iomem *flash_sys_glb_regs;
extern void __iomem *display_sys_glb_regs;

#endif //__AX620E_DISPLAY_REG_H__