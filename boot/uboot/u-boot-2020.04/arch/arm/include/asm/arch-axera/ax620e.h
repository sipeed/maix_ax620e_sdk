/*
 * AXERA AX620E
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __AX620E_H__
#define __AX620E_H__

#define GENERIC_TIMER_BASE         0x01B30000

#define BOOT_MODE_INFO_ADDR        0X700	//env from last stage
#define SD_UPDATE_STATUS_ADDR      0x44000040	//sd update status
#define SD_UPDATE_FINISH           0xAA55BB66
#define SD_UPDATE_FAIL             0xCCDDEEFF

#define USB_UPDATE_STATUS_ADDR      0x44000048	//usb update status
#define USB_UPDATE_FINISH           0xAA55BB66
#define USB_UPDATE_FAIL             0xCCDDEEFF

#define PMU_BASE            0x2100000
#define PMU_WAKEUP          0x8
#define PMU_WAKEUP1         0xC
#define PMU_WAKEUP_SET      0xEC
#define PMU_WAKEUP_CLR      0xF0
#define PMU_WAKEUP1_SET     0xF4
#define PMU_WAKEUP1_CLR     0xF8

#define PIN_MUX_G11_BASE		0x02309000
#define PIN_MUX_G11_PINCTRL		(PIN_MUX_G11_BASE + 0x0)	//bit[8:7]
#define PIN_MUX_G11_PINCTRL_SET 	(PIN_MUX_G11_BASE + 0x4)
#define PIN_MUX_G11_PINCTRL_CLR 	(PIN_MUX_G11_BASE + 0x8)
#define PIN_MUX_G11_PAD_RO0		(PIN_MUX_G11_BASE + 0x9C)
#define PIN_MUX_G11_VDET_RO0		(PIN_MUX_G11_BASE + 0xA0)	//bit0

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

#define INIT_CLK_400K				(400000)
#define INIT_CLK_300K				(300000)
#define INIT_CLK_200K				(200000)
#define INIT_CLK_100K				(100000)
#define DEFAULT_SD_CLK				(12000000)
#define DEFAULT_EMMC_CLK 			(12000000)
#define LEGACY_EMMC_CLK 			(25000000)
#define HS_EMMC_CLK 			 	(50000000)
#define HS_SD_CLK 			 	 	(50000000)

#define BIT_DWC_MSHC_CLK_CTRL_CLK_GEN_SELECT          BIT(5)
#define BIT_DWC_MSHC_CLK_CTRL_PLL_ENABLE              BIT(3)
#define BIT_DWC_MSHC_CLK_CTRL_SD_CLK_EN               BIT(2)
#define BIT_DWC_MSHC_CLK_CTRL_INTERNAL_CLK_STABLE     BIT(1)
#define BIT_DWC_MSHC_CLK_CTRL_INTERNAL_CLK_EN         BIT(0)

#define COMM_SYS_GLB	                0x02340000
#define COMM_SYS_GLB_CLK_MUX2_SET       (COMM_SYS_GLB + 0x1C)
#define COMM_ABORT_CFG                  (COMM_SYS_GLB + 0xA8)
#define COMM_ABORT_STATUS               (COMM_SYS_GLB + 0xB4)
#define COMM_SYS_DMA_FLASH_DW_SEL0      (COMM_SYS_GLB + 0x3d4)
#define COMM_SYS_DMA_FLASH_DW_SEL1      (COMM_SYS_GLB + 0x3d8)
#define CHIP_RST_SW                     BIT(0)

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
#define PERI_SYS_GLB_DMA_SEL0_SET       (PERI_SYS_GLB + 0x130)
#define PERI_SYS_GLB_DMA_SEL0_CLR       (PERI_SYS_GLB + 0x134)
#define PERI_SYS_GLB_DMA_SEL1_SET       (PERI_SYS_GLB + 0x138)
#define PERI_SYS_GLB_DMA_SEL1_CLR       (PERI_SYS_GLB + 0x13C)
#define PERI_SYS_GLB_DMA_HS_SEL3_SET    (PERI_SYS_GLB + 0x158)
#define PERI_SYS_GLB_DMA_HS_SEL3_CLR    (PERI_SYS_GLB + 0x15C)

#define ABORT_WDT2_ALARM BIT(4)
#define ABORT_WDT0_ALARM BIT(2)
#define ABORT_THM_ALARM BIT(1)
#define ABORT_SWRST_ALARM BIT(0)

#define ABORT_WDT2_EN BIT(9)
#define ABORT_WDT0_EN BIT(7)
#define ABORT_THM_EN BIT(6)

#define ABORT_WDT2_CLR BIT(5)
#define ABORT_WDT0_CLR BIT(3)
#define ABORT_THM_CLR BIT(2)
#define ABORT_SWRST_CLR BIT(1)

#define WDT0_BASE                  0x4840000
#define WDT0_TORR_ADDR             (WDT0_BASE + 0xC)
#define WDT0_TORR_START_ADDR       (WDT0_BASE + 0x18)
#define WDT0_CLK_FREQ              (24000000)

enum platform_type {
	AX620E_HAPS = 0,
	AX620E_EMMC,
	AX620E_NAND,
	PLATFORM_TYPE_NUM,
};

int get_board_id(void);
unsigned char get_chip_type_id(void);
int setup_boot_mode(void);
void store_board_id(void);
void print_board_id(void);
void print_plate_id(void);
void print_chip_type(void);
void store_chip_type(void);
void wdt0_enable(bool enable);
void set_ephy_led_pol(void);
#endif
