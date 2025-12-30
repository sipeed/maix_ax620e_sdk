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
#include <linux/delay.h>
#include <asm/arch/pll_config.h>
#include <asm/arch/boot_mode.h>

extern void wdt0_enable(bool enable);
extern int pinmux_init(void);
extern void dphyrx_pin_reg_config(void);

static misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;

void store_chip_type(void)
{
	writel(misc_info->chip_type, COMM_SYS_DUMMY_SW9);
}

static int thm_reset_enable(void)
{
	u32 temp;
	temp = readl(COMM_ABORT_CFG);
	temp |= ABORT_THM_EN;
	writel(temp, (u32)COMM_ABORT_CFG);
	return 0;
}
int chip_init(enum platform_type type)
{
	generic_timer_init();
	//pmu_init();
	wdt0_enable(0);
	store_chip_type();
#ifndef CONFIG_AXERA_ENV_BOARD_ID
	/* pinmux init */
	pinmux_init();
#endif
	thm_reset_enable();
#ifdef CDPHY_PIN_FUNC_MULTIPLEX
	//dphyrx_pin_reg_config();
#endif
	return 0;
}
