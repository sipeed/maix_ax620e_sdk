/*
 * (C) Copyright 2020 AXERA Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <dm/uclass-internal.h>
#include <power/regulator.h>
#include <asm/arch/ax620e.h>
#include <dm/device-internal.h>
#include <nand.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <pwm.h>

extern int chip_init(enum platform_type type);

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_ARM64
void dcache_enable(void);
void icache_enable(void);
#endif

U_BOOT_DEVICE(sysreset) = {
	.name = "axera_sysreset",
};

int adc_init(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_first_device_err(UCLASS_ADC, &dev);
	if (ret) {
		printf("No available ADC device\n");
		return CMD_RET_FAILURE;
	}

	return 0;
}

int thermal_init(void)
{
	struct udevice *dev;
	int ret;

	ret = uclass_first_device_err(UCLASS_THERMAL, &dev);
	if (ret) {
		printf("No available THERMAL device\n");
		return CMD_RET_FAILURE;
	}

	return 0;
}

int board_init(void)
{
#ifdef CONFIG_TARGET_AX620E_HAPS
	/* periph_ck_rst_cfg
	 * 0x2002000[0:1]: clk_periph_glb_sel
	 * 0x2002000[2]: clk_periph_gpio_sel
	 * 0x2002014[5]: hclk_spi_s_eb
	 * 0x2002014[6:12]: pclk_gpio0~6_eb
	 * 0x2002024[0:6]: gpio0~6_sw_rst
	 * 0x2002024[7:8]: hspi_sw_hrst/hspi_sw_rst
	 * 0x200202c[8:9]: spi_m0_sw_prst/spi_m0_sw_rst
	 * 0x200202c[10:11]: spi_m1_sw_prst/spi_m1_sw_rst
	 * 0x200202c[12:13]: spi_m2_sw_prst/spi_m2_sw_rst
	 */
	*(volatile int *)0x2002000 |= (0x4 | 0x3); /*clk_periph_gpio_sel gpll_24m/clk_periph_glb_sel mpll_350m*/
	*(volatile int *)0x2002024 &= ~(0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x20 | 0x40 | 0x80 | 0x100);
	*(volatile int *)0x200202c &= ~(0x100 | 0x200 | 0x400 | 0x800 | 0x1000 | 0x2000);
	*(volatile int *)0x2002014 |= (0x20 | 0x40 | 0x80 | 0x100 | 0x200 | 0x400 | 0x800 | 0x1000);
	/* flash_ck_rst_cfg
	 * 0x110000[5:6]: clk_flash_glb_sel
	 * 0x110000[7:8]: clk_flash_ser_sel
	 * 0x110004[5]: clk_flash_ser_eb
	 * 0x110008[4:5]: clk_h_spi_eb/clk_p_spi_eb
	 * 0x110008[10:11]: hclk_spi_eb/pclk_spi_eb
	 * 0x110010[3:4]: hspi_sw_hrst/hspi_sw_rst
	 * 0x110010[5:6]: pspi_sw_prst/pspi_sw_rst
	 */
	*(volatile int *)0x110010 &= ~(0x8 | 0x10 | 0x20 | 0x40);
	*(volatile int *)0x110008 |= (0x10 | 0x20 | 0x400 | 0x800);
	*(volatile int *)0x110004 |= 0x20;
	*(volatile int *)0x110000 &= ~(0x60 | 0x180);
	*(volatile int *)0x110000 |= (0x2 << 5) | (0x2 << 7); /*clk_flash_glb_sel mpll_350m/clk_flash_ser_sel npll_200m*/
#endif
	adc_init();
	thermal_init();
	return 0;
}

void board_nand_init(void)
{
	struct udevice *dev;
	struct mtd_info *mtd;
	int ret;
	int busnum = 0;

	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return;
	}
	ret = device_probe(dev);
	if (ret) {
		printf("device_probe error, (error=%d)\n", ret);
		return;
	}

	mtd = dev_get_uclass_priv(dev);
	ret = nand_register(0, mtd);
	if (ret) {
		printf("nand_register error, (error=%d)\n", ret);
		return;
	}
}

#ifndef CONFIG_ARM64
void enable_caches(void)
{
	icache_enable();
	dcache_enable();
}
#endif

int dram_init(void)
{
	gd->ram_size = 0x10000000;
	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = 0x40000000;
	gd->bd->bi_dram[0].size = 0x10000000;
	return 0;
}


int board_early_init_f(void)
{
	chip_init(AX620E_HAPS);
	return 0;
}

