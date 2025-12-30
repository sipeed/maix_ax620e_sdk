/*
 * Copyright (c) 2013-2018, AX650 Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ax620e_pwrc.h>
#include <lib/bakery_lock.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <ax620e_def.h>
#include <plat/common/platform.h>
#include <bl31/bl31.h>
#include <ax620e_common_sys_glb.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/arm/gicv2.h>
#include <drivers/console.h>
#include <drivers/ti/uart/uart_16550.h>
#include <cpu_sys.h>
#include <debug_sys.h>
#include "timer.h"
#include "chip_top.h"

AX620E_INSTANTIATE_LOCK;
extern void cntkctl_enable(void);
extern void cntkctl_disable(void);

unsigned int ax620e_pwrc_read_psysr(u_register_t mpidr)
{
	unsigned int state;

	pmu_get_module_state(MODULE_CPU, &state);

	return state;
}

void ax620e_pwrc_write_pponr(u_register_t mpidr)
{
	unsigned int core_id = plat_core_pos_by_mpidr(mpidr);
	uint32_t addr_lo = ((uintptr_t)bl31_warm_entrypoint) & 0xFFFFFFFF;
	uint32_t addr_high = ((uintptr_t)bl31_warm_entrypoint) >> 32;

	if (core_id == 0) {
		mmio_write_32(COMM_SYS_DUMMY_SW2_CORE0_LOW, addr_lo);
		mmio_write_32(COMM_SYS_DUMMY_SW3_CORE0_HIGH, addr_high);
	} else if (core_id == 1) {
		mmio_write_32(COMM_SYS_DUMMY_SW0_CORE1_LOW, addr_lo);
		mmio_write_32(COMM_SYS_DUMMY_SW1_CORE1_HIGH, addr_high);
	}

	/*wakeup core1 from wfe */
	sev();
}

void ax620e_pwrc_write_ppoffr(u_register_t mpidr)
{
	uint32_t addr_high = 0;
	uint32_t addr_lo = 0;
	unsigned int core_id = 0;
	core_id = plat_core_pos_by_mpidr(mpidr);

	if (core_id == 0) {
		mmio_write_32(COMM_SYS_DUMMY_SW2_CORE0_LOW, 0x0);
		mmio_write_32(COMM_SYS_DUMMY_SW3_CORE0_HIGH, 0x0);
	} else if (core_id == 1) {
		mmio_write_32(COMM_SYS_DUMMY_SW0_CORE1_LOW, 0x0);
		mmio_write_32(COMM_SYS_DUMMY_SW1_CORE1_HIGH, 0x0);
	}

	cntkctl_disable();

	do {

		__asm__ __volatile__("dsb sy");
		isb();
		wfe();
		if (core_id == 0) {
			addr_high = mmio_read_32(COMM_SYS_DUMMY_SW3_CORE0_HIGH);
			addr_lo = mmio_read_32(COMM_SYS_DUMMY_SW2_CORE0_LOW);

		} else if (core_id == 1) {
			addr_high = mmio_read_32(COMM_SYS_DUMMY_SW1_CORE1_HIGH);
			addr_lo = mmio_read_32(COMM_SYS_DUMMY_SW0_CORE1_LOW);
		}

	} while((((uint64_t)addr_high << 32) | ((uint64_t)addr_lo)) == 0x0);

	cntkctl_enable();
}

void ax620e_sys_pwrdwn()
{
}

/* Nothing else to do here apart from initializing the lock */
void plat_ax620e_pwrc_setup(void)
{

	ax620e_lock_init();

	/* pmu config */
	pmu_init();

	/* mask all wakeup source here , will be enabled in wakeup source drivers */
	mmio_write_32(EIC_EN_CLR, EIC_EN_MASK_ALL);

	chip_top_set();

	/* clear usb wakeup interrupt here or usb int will affect flash sys sleep */
	mmio_write_32(COMMON_SYS_USB_INT_CTRL_SET_ADDR, BIT_COMMON_SYS_USB_WAKE_UP_INT_CLR_SET);
	mmio_write_32(COMMON_SYS_USB_INT_CTRL_SET_ADDR, BIT_COMMON_SYS_USB_WAKE_UP_INT_MASK_SET);

	/* settings for auto gate */
	mmio_write_32(MISC_CTRL, MISC_CTRL_VAL);
	mmio_write_32(CPU_SYS_FAB_GT_CTRL0_ADDR, CPU_SYS_FAB_GT_CTRL0_VAL);
	mmio_write_32(DEBUG_SYS_AUTO_GT_EN, DEBUG_SYS_AUTO_GT_EN_VAL);
}
