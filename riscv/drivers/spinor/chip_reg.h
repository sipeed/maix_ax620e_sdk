/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __CHIP_REG_H__
#define __CHIP_REG_H__

#define CPU_SYS_GLB               0x1900000
#define CPU_SYS_GLB_CLK_MUX0      (CPU_SYS_GLB + 0x0)
#define CPU_SYS_GLB_CLK_MUX0_SET  (CPU_SYS_GLB + 0x1000)
#define CPU_SYS_GLB_CLK_MUX0_CLR  (CPU_SYS_GLB + 0x2000)
#define CPU_SYS_GLB_CLK_EB0_SET   (CPU_SYS_GLB + 0x1004)
#define CPU_SYS_GLB_CLK_EB0_CLR   (CPU_SYS_GLB + 0x2004)
#define CPU_SYS_GLB_CLK_DIV0_SET  (CPU_SYS_GLB + 0x100C)
#define CPU_SYS_GLB_CLK_DIV0_CLR  (CPU_SYS_GLB + 0x200C)
#define CPU_SYS_GLB_SW_RST0_SET   (CPU_SYS_GLB + 0x1010)
#define CPU_SYS_GLB_SW_RST0_CLR   (CPU_SYS_GLB + 0x2010)

#define CPU_SYS_GLB_EMMC0_SET     (CPU_SYS_GLB + 0x1034)
#define CPU_SYS_GLB_EMMC1_SET     (CPU_SYS_GLB + 0x1038)
#define CPU_SYS_GLB_EMMC2_SET     (CPU_SYS_GLB + 0x103C)
#define CPU_SYS_GLB_EMMC3_SET     (CPU_SYS_GLB + 0x1040)
#define CPU_SYS_GLB_EMMC4_SET     (CPU_SYS_GLB + 0x1044)
#define CPU_SYS_GLB_EMMC5_SET     (CPU_SYS_GLB + 0x1048)
#define CPU_SYS_GLB_EMMC6_SET     (CPU_SYS_GLB + 0x104C)
#define CPU_SYS_GLB_EMMC7         (CPU_SYS_GLB + 0x50)
#define CPU_SYS_GLB_EMMC7_SET     (CPU_SYS_GLB + 0x1050)

#define CPU_SYS_GLB_EMMC0_CLR     (CPU_SYS_GLB + 0x2034)
#define CPU_SYS_GLB_EMMC1_CLR     (CPU_SYS_GLB + 0x2038)
#define CPU_SYS_GLB_EMMC2_CLR     (CPU_SYS_GLB + 0x203C)
#define CPU_SYS_GLB_EMMC3_CLR     (CPU_SYS_GLB + 0x2040)
#define CPU_SYS_GLB_EMMC4_CLR     (CPU_SYS_GLB + 0x2044)
#define CPU_SYS_GLB_EMMC5_CLR     (CPU_SYS_GLB + 0x2048)
#define CPU_SYS_GLB_EMMC6_CLR     (CPU_SYS_GLB + 0x204C)
#define CPU_SYS_GLB_EMMC7_CLR     (CPU_SYS_GLB + 0x2050)

//already modify
#define COMM_SYS_GLB		        0x02340000
#define COMM_SYS_GLB_CLK_MUX0_SET	(COMM_SYS_GLB + 0x04)
#define COMM_SYS_GLB_CLK_MUX0_CLR	(COMM_SYS_GLB + 0x08)
#define COMM_SYS_GLB_CLK_MUX1_SET	(COMM_SYS_GLB + 0x10)
#define COMM_SYS_GLB_CLK_MUX1_CLR	(COMM_SYS_GLB + 0x14)
#define COMM_SYS_GLB_CLK_MUX2_SET	(COMM_SYS_GLB + 0x1C)
#define COMM_SYS_GLB_CLK_MUX2_CLR	(COMM_SYS_GLB + 0x20)
#define COMM_SYS_CHIP_MODE	        (COMM_SYS_GLB + 0x8C)
#define COMM_SYS_CHIP_BOND	        (COMM_SYS_GLB + 0x90)
#define COMM_SYS_BOND_OPT	        (COMM_SYS_GLB + 0x98)
#define COMM_ABORT_CFG		        (COMM_SYS_GLB + 0xA8)
#define COMM_SYS_CHIP_STA	        (COMM_SYS_GLB + 0x1DC)
#define COMM_SYS_MISC_CTRL	        (COMM_SYS_GLB + 0x380)
#define COMM_SYS_DUMMY_SW5	        (COMM_SYS_GLB + 0x204)

/* For boot mode or download in thm_reset/wdt_reset/soft_rst */
#define TOP_CHIPMODE_GLB                (0x2390000)
#define TOP_CHIPMODE_GLB_SW_PORT        (TOP_CHIPMODE_GLB + 0x0)
#define TOP_CHIPMODE_GLB_SW_PORT_SET    (TOP_CHIPMODE_GLB + 0x4)
#define TOP_CHIPMODE_GLB_SW_PORT_CLR    (TOP_CHIPMODE_GLB + 0x8)
#define TOP_CHIPMODE_GLB_SW             (TOP_CHIPMODE_GLB + 0xC)
#define TOP_CHIPMODE_GLB_SW_SET         (TOP_CHIPMODE_GLB + 0x10)
#define TOP_CHIPMODE_GLB_SW_CLR         (TOP_CHIPMODE_GLB + 0x14)
#define TOP_CHIPMODE_GLB_BACKUP0        (TOP_CHIPMODE_GLB + 0x24)
#define TOP_CHIPMODE_GLB_BACKUP0_SET    (TOP_CHIPMODE_GLB + 0x28)
#define TOP_CHIPMODE_GLB_BACKUP0_CLR    (TOP_CHIPMODE_GLB + 0x2C)
#define TOP_CHIPMODE_GLB_BACKUP1        (TOP_CHIPMODE_GLB + 0x30)
#define TOP_CHIPMODE_GLB_BACKUP1_SET    (TOP_CHIPMODE_GLB + 0x34)
#define TOP_CHIPMODE_GLB_BACKUP1_CLR    (TOP_CHIPMODE_GLB + 0x38)
//already modify
#define PERI_SYS_GLB                    (0x4870000)
#define PERI_SYS_GLB_CLK_MUX0_SET       (PERI_SYS_GLB + 0xA8)
#define PERI_SYS_GLB_CLK_MUX0_CLR       (PERI_SYS_GLB + 0xAC)
#define PERI_SYS_GLB_CLK_EB0_SET        (PERI_SYS_GLB + 0xB0)
#define PERI_SYS_GLB_CLK_EB0_CLR        (PERI_SYS_GLB + 0xB4)
#define PERI_SYS_GLB_CLK_EB1_SET        (PERI_SYS_GLB + 0xB8)
#define PERI_SYS_GLB_CLK_EB1_CLR        (PERI_SYS_GLB + 0xBC)
#define PERI_SYS_GLB_CLK_EB2_SET        (PERI_SYS_GLB + 0xC0)
#define PERI_SYS_GLB_CLK_EB2_CLR        (PERI_SYS_GLB + 0xC4)
#define PERI_SYS_GLB_CLK_EB3_SET        (PERI_SYS_GLB + 0xC8)
#define PERI_SYS_GLB_CLK_EB3_CLR        (PERI_SYS_GLB + 0xCC)
#define PERI_SYS_GLB_CLK_RST0_SET       (PERI_SYS_GLB + 0xD8)
#define PERI_SYS_GLB_CLK_RST0_CLR       (PERI_SYS_GLB + 0xDC)
#define PERI_SYS_GLB_CLK_RST1_SET       (PERI_SYS_GLB + 0xE0)
#define PERI_SYS_GLB_CLK_RST1_CLR       (PERI_SYS_GLB + 0xE4)
#define PERI_SYS_GLB_CLK_RST2_SET       (PERI_SYS_GLB + 0xE8)
#define PERI_SYS_GLB_CLK_RST2_CLR       (PERI_SYS_GLB + 0xEC)
#define PERI_SYS_GLB_CLK_RST3_SET       (PERI_SYS_GLB + 0xF0)
#define PERI_SYS_GLB_CLK_RST3_CLR       (PERI_SYS_GLB + 0xF4)
#define PERI_SYS_GLB_SPI_SET            (PERI_SYS_GLB + 0x110)
#define PERI_SYS_GLB_SPI_CLR            (PERI_SYS_GLB + 0x114)
#define PERI_SYS_GLB_DMA_SEL0_SET       (PERI_SYS_GLB + 0x130) //dma_sel_reg_h_set
#define PERI_SYS_GLB_DMA_SEL0_CLR       (PERI_SYS_GLB + 0x134)
#define PERI_SYS_GLB_DMA_SEL1_SET       (PERI_SYS_GLB + 0x138) //dma_sel_reg_l_set
#define PERI_SYS_GLB_DMA_SEL1_CLR       (PERI_SYS_GLB + 0x13C)
#define PERI_SYS_GLB_DMA_HS_SEL3_SET    (PERI_SYS_GLB + 0x158) //dma_hs_sel0_set [5:0] chan0
#define PERI_SYS_GLB_DMA_HS_SEL3_CLR    (PERI_SYS_GLB + 0x15C)

#define GENERIC_TIMER_BASE      0x1B30000
//already modify
#define SYS_IRAM_BASE		(0x0)
#define SYS_OCM_BASE		0x03000000
#define SYS_DRAM_BASE		0x40000000

//already modify
#define GPIO0_BASE			0x04800000
#define GPIO1_BASE			0x04801000
#define GPIO2_BASE			0x06000000
#define GPIO3_BASE			0x06001000
#define PORTA_DR			0x00
#define PORTA_DDR			0x04
#define PORTA_CTL			0x08

/* vdet enable address
en_vdet_g1 = misc_ctrl_g10[0], default 1 is enable
en_vdet_g6 = misc_ctrl_g10[1], default 1 is enable
en_vdet_g11 = misc_ctrl_g10[2], default 1 is enable
en_vdet_g8 = misc_ctrl_g8[0], default 1 is enable
en_vdet_g9 = misc_ctrl_g8[1], default 1 is enable
en_vdet_G12 = misc_ctrl_g8[2], default 1 is enable
*/
/* in common sys */
#define PIN_MUX_G1_BASE			0x02300000
#define PIN_MUX_G1_PINCTRL		(PIN_MUX_G1_BASE + 0x0)	//bit[8:7]
#define PIN_MUX_G1_PINCTRL_SET	        (PIN_MUX_G1_BASE + 0x4)
#define PIN_MUX_G1_PINCTRL_CLR	        (PIN_MUX_G1_BASE + 0x8)
#define PIN_MUX_G1_PAD_RO0		(PIN_MUX_G1_BASE + 0x90)
#define PIN_MUX_G1_VDET_RO0		(PIN_MUX_G1_BASE + 0x94)	//bit0
#define PIN_MUX_G1_MISC			(PIN_MUX_G1_BASE + 0x98)
#define PIN_MUX_G1_MISC_SET		(PIN_MUX_G1_BASE + 0x9C)
#define PIN_MUX_G1_MISC_CLR		(PIN_MUX_G1_BASE + 0xA0)

#define PIN_MUX_G5_BASE			0x02302000
#define PIN_MUX_G5_EMMC_PWR_EN		(PIN_MUX_G5_BASE + 0x60)
#define PIN_MUX_G5_EMMC_PWR_EN_SET	(PIN_MUX_G5_BASE + 0x64)
#define PIN_MUX_G5_EMMC_PWR_EN_CLR	(PIN_MUX_G5_BASE + 0x68)
#define PIN_MUX_G5_SD_PWR_SW		(PIN_MUX_G5_BASE + 0xC)
#define PIN_MUX_G5_SD_PWR_SW_SET	(PIN_MUX_G5_BASE + 0x10)
#define PIN_MUX_G5_SD_PWR_SW_CLR	(PIN_MUX_G5_BASE + 0x14)


#define PIN_MUX_G6_BASE			0x02304000
#define PIN_MUX_G6_PINCTRL		(PIN_MUX_G6_BASE + 0x0)//bit[8:7]
#define PIN_MUX_G6_PINCTRL_SET	        (PIN_MUX_G6_BASE + 0x4)
#define PIN_MUX_G6_PINCTRL_CLR	        (PIN_MUX_G6_BASE + 0x8)
#define PIN_MUX_G6_UART0_TXD	        (PIN_MUX_G6_BASE + 0x3C)
#define PIN_MUX_G6_UART0_RXD	        (PIN_MUX_G6_BASE + 0x48)
#define PIN_MUX_G6_UART1_TXD	        (PIN_MUX_G6_BASE + 0x54)
#define PIN_MUX_G6_UART1_RXD	        (PIN_MUX_G6_BASE + 0x60)
#define PIN_MUX_G6_PAD_RO0		(PIN_MUX_G6_BASE + 0x9C)
#define PIN_MUX_G6_VDET_RO0		(PIN_MUX_G6_BASE + 0xA0)	//bit0

#define PIN_MUX_G11_BASE		     0x02309000
#define PIN_MUX_G11_PINCTRL		     (PIN_MUX_G11_BASE + 0x0)//bit[8:7]
#define PIN_MUX_G11_PINCTRL_SET	             (PIN_MUX_G11_BASE + 0x4)
#define PIN_MUX_G11_PINCTRL_CLR	             (PIN_MUX_G11_BASE + 0x8)
#define PIN_MUX_G11_PAD_RO0                  (PIN_MUX_G11_BASE + 0x9C)
#define PIN_MUX_G11_VDET_RO0                 (PIN_MUX_G11_BASE + 0xA0)	//bit0
#define PIN_MUX_G11_EMMC_DAT7_SET            (PIN_MUX_G11_BASE + 0x4C)
#define PIN_MUX_G11_EMMC_DAT7_CLR            (PIN_MUX_G11_BASE + 0x50)
#define PIN_MUX_G11_EMMC_DAT6_SET            (PIN_MUX_G11_BASE + 0x34)
#define PIN_MUX_G11_EMMC_DAT6_CLR            (PIN_MUX_G11_BASE + 0x38)
#define PIN_MUX_G11_EMMC_DS_SET              (PIN_MUX_G11_BASE + 0x40)
#define PIN_MUX_G11_EMMC_DS_CLR              (PIN_MUX_G11_BASE + 0x44)
#define PIN_MUX_G11_EMMC_RESET_N_SET         (PIN_MUX_G11_BASE + 0x1C)
#define PIN_MUX_G11_EMMC_RESET_N_CLR         (PIN_MUX_G11_BASE + 0x20)
#define PIN_MUX_G11_EMMC_DAT5_SET            (PIN_MUX_G11_BASE + 0x10)
#define PIN_MUX_G11_EMMC_DAT5_CLR            (PIN_MUX_G11_BASE + 0x14)
#define PIN_MUX_G11_EMMC_DAT4_SET            (PIN_MUX_G11_BASE + 0x28)
#define PIN_MUX_G11_EMMC_DAT4_CLR            (PIN_MUX_G11_BASE + 0x2C)
#define PIN_MUX_G11_EMMC_DAT3_SET            (PIN_MUX_G11_BASE + 0x58)
#define PIN_MUX_G11_EMMC_DAT3_CLR            (PIN_MUX_G11_BASE + 0x5C)
#define PIN_MUX_G11_EMMC_DAT2_SET            (PIN_MUX_G11_BASE + 0x64)
#define PIN_MUX_G11_EMMC_DAT2_CLR            (PIN_MUX_G11_BASE + 0x68)
#define PIN_MUX_G11_EMMC_CLK_SET             (PIN_MUX_G11_BASE + 0x70)
#define PIN_MUX_G11_EMMC_CLK_CLR             (PIN_MUX_G11_BASE + 0x74)
#define PIN_MUX_G11_EMMC_CMD_SET             (PIN_MUX_G11_BASE + 0x88)
#define PIN_MUX_G11_EMMC_CMD_CLR             (PIN_MUX_G11_BASE + 0x8C)
#define PIN_MUX_G11_EMMC_DAT1_SET            (PIN_MUX_G11_BASE + 0x94)
#define PIN_MUX_G11_EMMC_DAT1_CLR            (PIN_MUX_G11_BASE + 0x98)
#define PIN_MUX_G11_EMMC_DAT0_SET            (PIN_MUX_G11_BASE + 0x7C)
#define PIN_MUX_G11_EMMC_DAT0_CLR            (PIN_MUX_G11_BASE + 0x80)


/* in flash sys */
#define PIN_MUX_G8_BASE			0x104F0000
#define PIN_MUX_G8_PINCTRL		(PIN_MUX_G8_BASE + 0x0)	//bit[8:7]
#define PIN_MUX_G8_PINCTRL_SET	        (PIN_MUX_G8_BASE + 0x4)
#define PIN_MUX_G8_PINCTRL_CLR	        (PIN_MUX_G8_BASE + 0x8)
#define PIN_MUX_G8_PAD_RO0		(PIN_MUX_G8_BASE + 0x114)
#define PIN_MUX_G8_VDET_RO0		(PIN_MUX_G8_BASE + 0x118)	//bit0
#define PIN_MUX_G8_EMAC_PTP_PPS2	(PIN_MUX_G8_BASE + 0x24)
#define PIN_MUX_G8_EMAC_PTP_PPS2_SET	(PIN_MUX_G8_BASE + 0x28)
#define PIN_MUX_G8_EMAC_PTP_PPS2_CLR	(PIN_MUX_G8_BASE + 0x2C)

#define PIN_MUX_G9_BASE			0x104F1000
#define PIN_MUX_G9_PINCTRL		(PIN_MUX_G9_BASE + 0x0)	//bit[8:7]
#define PIN_MUX_G9_PINCTRL_SET	        (PIN_MUX_G9_BASE + 0x4)
#define PIN_MUX_G9_PINCTRL_CLR	        (PIN_MUX_G9_BASE + 0x8)
#define PIN_MUX_G9_PAD_RO0		(PIN_MUX_G9_BASE + 0x54)
#define PIN_MUX_G9_VDET_RO0		(PIN_MUX_G9_BASE + 0x58)	//bit0
#define PIN_MUX_G9_SD_DAT0_SET		(PIN_MUX_G9_BASE + 0x10)
#define PIN_MUX_G9_SD_DAT0_CLR		(PIN_MUX_G9_BASE + 0x14)
#define PIN_MUX_G9_SD_DAT1_SET		(PIN_MUX_G9_BASE + 0x1c)
#define PIN_MUX_G9_SD_DAT1_CLR		(PIN_MUX_G9_BASE + 0x20)
#define PIN_MUX_G9_SD_DAT2_SET		(PIN_MUX_G9_BASE + 0x40)
#define PIN_MUX_G9_SD_DAT2_CLR		(PIN_MUX_G9_BASE + 0x44)
#define PIN_MUX_G9_SD_DAT3_SET		(PIN_MUX_G9_BASE + 0x4c)
#define PIN_MUX_G9_SD_DAT3_CLR		(PIN_MUX_G9_BASE + 0x50)
#define PIN_MUX_G9_SD_CMD_SET		(PIN_MUX_G9_BASE + 0x34)
#define PIN_MUX_G9_SD_CMD_CLR		(PIN_MUX_G9_BASE + 0x38)

#define PIN_MUX_G12_BASE		0x104F2000
#define PIN_MUX_G12_PINCTRL		(PIN_MUX_G12_BASE + 0x0)	//bit[8:7]
#define PIN_MUX_G12_PINCTRL_SET	        (PIN_MUX_G12_BASE + 0x4)
#define PIN_MUX_G12_PINCTRL_CLR	        (PIN_MUX_G12_BASE + 0x8)
#define PIN_MUX_G12_PAD_RO0		(PIN_MUX_G12_BASE + 0x54)
#define PIN_MUX_G12_VDET_RO0		(PIN_MUX_G12_BASE + 0x58)	//bit0

#define FLASH_SYS_GLB_BASE                  0x10030000
#define FLASH_SYS_GLB_CLK_MUX0              (FLASH_SYS_GLB_BASE + 0x0)
#define FLASH_SYS_GLB_CLK_MUX0_SET          (FLASH_SYS_GLB_BASE + 0x4000)
#define FLASH_SYS_GLB_CLK_MUX0_CLR          (FLASH_SYS_GLB_BASE + 0x8000)
#define FLASH_SYS_GLB_CLK_EB0_SET           (FLASH_SYS_GLB_BASE + 0x4004)
#define FLASH_SYS_GLB_CLK_EB0_CLR           (FLASH_SYS_GLB_BASE + 0x8004)
#define FLASH_SYS_GLB_CLK_EB1_SET           (FLASH_SYS_GLB_BASE + 0x4008)
#define FLASH_SYS_GLB_CLK_EB1_CLR           (FLASH_SYS_GLB_BASE + 0x8008)
#define FLASH_SYS_GLB_CLK_DIV0_SET          (FLASH_SYS_GLB_BASE + 0x400C)
#define FLASH_SYS_GLB_CLK_DIV0_CLR          (FLASH_SYS_GLB_BASE + 0x800C)
#define FLASH_SYS_GLB_SW_RST0_SET           (FLASH_SYS_GLB_BASE + 0x4014)
#define FLASH_SYS_GLB_SW_RST0_CLR           (FLASH_SYS_GLB_BASE + 0x8014)

#define NPU_SYS_GLB_BASE                    0x3890000
#define NPU_SYS_GLB_CLK_MUX0_SET            (NPU_SYS_GLB_BASE + 0x5C)
#define NPU_SYS_GLB_CLK_MUX0_CLR            (NPU_SYS_GLB_BASE + 0x60)

#define PLLC_GLB_REG           0x02210000
#define CPUPLL_CFG0_ADDR       (PLLC_GLB_REG + 0xC4)
#define CPUPLL_CFG1_ADDR       (PLLC_GLB_REG + 0xD0)
#define GRP_PLL_RDY_STS_ADDR   (PLLC_GLB_REG + 0x1C0)
#define CPUPLL_ON_CFG_SET_ADDR (PLLC_GLB_REG + 0x1EC)
#define CPUPLL_ON_CFG_CLR_ADDR (PLLC_GLB_REG + 0x1F0)
#endif
