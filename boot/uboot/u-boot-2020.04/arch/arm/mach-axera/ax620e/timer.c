/*
 * Copyright (C) AXERA
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/ax620e.h>

DECLARE_GLOBAL_DATA_PTR;

static int init_done __attribute__((section(".data"))) = 0;

/*
 * Timer initialization
 */
int generic_timer_init(void)
{
	/* Only init the timer once */
	if (init_done)
		return 0;
	init_done = 1;

	writel(0x0, GENERIC_TIMER_BASE + 0x8);
	writel(0x0, GENERIC_TIMER_BASE + 0xC);
	writel(0x16E3600, GENERIC_TIMER_BASE + 0x20); //24mhz
	writel(0x11, GENERIC_TIMER_BASE);   //enable generic timer

	return 0;
}
