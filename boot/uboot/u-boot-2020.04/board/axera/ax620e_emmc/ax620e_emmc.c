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
#include <spi.h>

extern int chip_init(enum platform_type type);

DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_ARM64
void dcache_enable(void);
void icache_enable(void);
#endif

U_BOOT_DEVICE(sysreset) = {
	.name = "axera_sysreset",
};
#if 0
struct ugpiodev {
	int num;
	int act;
	int inact;
	bool init;
};

struct uspidev {
	int SPI_BUS;
	int speed;
	struct udevice* bus;
	struct udevice* dev;
	struct spi_slave* slave;
	struct ugpiodev cs;
	bool init;
};

struct ulcddev {
	struct uspidev spi;
	struct ugpiodev dc;
	struct ugpiodev rs;
	bool init;
};

void ugpiodev_preinit(struct ugpiodev* pgpiodev)
{
	pgpiodev->num = -1;
	pgpiodev->act = -1;
	pgpiodev->inact = -1;
	pgpiodev->init = false;
}

struct ulcddev ulcddev_new(void)
{
	struct ulcddev lcddev;

	lcddev.init = false;
	lcddev.spi.SPI_BUS = 1;
	lcddev.spi.speed = 0;
	lcddev.spi.bus = NULL;
	lcddev.spi.dev = NULL;
	lcddev.spi.slave = NULL;
	ugpiodev_preinit(&lcddev.spi.cs);

	ugpiodev_preinit(&lcddev.dc);
	ugpiodev_preinit(&lcddev.rs);

	return lcddev;
}

bool ugpiodev_init(struct ugpiodev* pgpiodev)
{
#define UGPIODEV_INIT_BUF_SIZE 8
	int ret = -1;
	char buf[UGPIODEV_INIT_BUF_SIZE];

	if (pgpiodev->num < 0 || pgpiodev->act < 0) {
		return false;
	}

	if (pgpiodev->init) {
		return true;
	}

	pgpiodev->act = (pgpiodev->act == 0) ? 0 : 1;
	pgpiodev->inact = (pgpiodev->act == 0) ? 1 : 0;

	memset(buf, 0x00, sizeof(buf));
	snprintf(buf, sizeof(buf), "gpio%d", pgpiodev->num);

	ret = gpio_request(pgpiodev->num, buf);
	if (ret < 0) {
		printf("[%s] gpio_request failed: %d", __PRETTY_FUNCTION__, ret);
		return false;
	}

	ret = gpio_direction_output(pgpiodev->num, pgpiodev->inact);
	if (ret < 0) {
		printf("[%s] gpio_direction_output failed: %d", __PRETTY_FUNCTION__, ret);
		return false;
	}

	gpio_set_value(pgpiodev->num, pgpiodev->inact);
	pgpiodev->init = true;

	return true;
}

bool ulcddev_init(struct ulcddev* pdev)
{
	int ret = -1;
	static const char txbuf[] = {0xFC, 0xC0, 0xFC, 0xC0};

	pdev->spi.SPI_BUS = 2;
	pdev->spi.speed = 12500000;
	pdev->spi.cs.num = 27;
	pdev->spi.cs.act = 0;

	ret = uclass_get_device_by_seq(UCLASS_SPI, pdev->spi.SPI_BUS, &pdev->spi.bus);
	if (ret) {
		printf("[%s] Failed to probe SPI bus %d: %d\n", __PRETTY_FUNCTION__, pdev->spi.SPI_BUS, ret);
		goto fail;
	}

	ret = spi_get_bus_and_cs(pdev->spi.SPI_BUS, 0, pdev->spi.speed, SPI_CPHA | SPI_CPOL,
				 "spi_generic_drv", "lcd@0", &pdev->spi.dev, &pdev->spi.slave);
	if (ret) {
		printf("[%s] Failed to get SPI slave: %d\n", __PRETTY_FUNCTION__, ret);
		goto fail;
	}

	ret = spi_claim_bus(pdev->spi.slave);
	if (ret) {
		printf("[%s] spi_claim_bus failed: %d\n", __PRETTY_FUNCTION__, ret);
		goto fail_free;
	}

	ret = spi_xfer(pdev->spi.slave, sizeof(txbuf) * 8, txbuf, NULL,
			  SPI_XFER_ONCE);
	if (ret) {
		printf("[%s] SPI xfer failed: %d\n", __PRETTY_FUNCTION__, ret);
		goto fail_release;
	}
	printf("SPI%d initialized\n", pdev->spi.SPI_BUS);

	if (!ugpiodev_init(&pdev->spi.cs)) {
		printf("[%s] SPI.CS init failed", __PRETTY_FUNCTION__);
		goto fail_release;
	}

	pdev->spi.init = true;
	pdev->init = true;
	return true;

fail_release:
	spi_release_bus(pdev->spi.slave);
fail_free:
	spi_free_slave(pdev->spi.slave);
fail:
	return false;
}

int uspi_write(struct uspidev* pspidev, const char* buf, int size)
{
	int ret = -1;

	gpio_set_value(pspidev->cs.num, pspidev->cs.act);
	ret = spi_xfer(pspidev->slave, size*8, buf, NULL,
			  SPI_XFER_ONCE);
	gpio_set_value(pspidev->cs.num, pspidev->cs.inact);

	return ret;
}

static void lcd_logo_init(void)
{
	struct ulcddev lcddev;
	char txbuf[32];
	int i = 0;
	int ret = -1;

	printf("-----buf[%ld]----\n", sizeof(txbuf));
	for (i = 0; i < sizeof(txbuf); ++i){
		txbuf[i] = (char)i;
		printf(" 0x%02x", txbuf[i]);
	}
	printf("\n-----buf----\n");

	lcddev = ulcddev_new();
	if (!ulcddev_init(&lcddev)) {
		return;
	}

	printf("#1 send %ld bytes start...\n", sizeof(txbuf)-1);
	ret = uspi_write(&lcddev.spi, txbuf, sizeof(txbuf)-1);
	if (ret) {
		printf("[%s] uspi_write failed: %d\n", __PRETTY_FUNCTION__, ret);
		return;
	}
	printf("#1 send %ld bytes finish\n", sizeof(txbuf)-1);

	printf("#2 send %ld bytes start...\n", sizeof(txbuf));
	ret = uspi_write(&lcddev.spi, txbuf, sizeof(txbuf));
	if (ret) {
		printf("[%s] uspi_write failed: %d\n", __PRETTY_FUNCTION__, ret);
		return;
	}
	printf("#2 send %ld bytes finish\n", sizeof(txbuf));
}
#endif

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

	// printf("+++==========user init start ===========+++\n");
	// lcd_logo_init();
	// printf("+++==========user init end ===========+++\n");

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

#if CONFIG_AXERA_AX630C_DDR4_RETRAIN
int dram_init(void)
{
	gd->ram_size = 0x7FFFF000;
	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = 0x40001000;
	gd->bd->bi_dram[0].size = 0x7FFFF000;
	return 0;
}
#else
#ifdef CONFIG_AX620Q_EMMC
int dram_init(void)
{
	gd->ram_size = 0xf000000;
	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = 0x40000000;
	gd->bd->bi_dram[0].size = 0xf000000;
	return 0;
}
#else
int dram_init(void)
{
	gd->ram_size = 0x80000000;
	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = 0x40000000;
	gd->bd->bi_dram[0].size = 0x80000000;
	return 0;
}
#endif
#endif


int board_early_init_f(void)
{
	chip_init(AX620E_HAPS);
	return 0;
}
