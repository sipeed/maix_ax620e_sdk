/*
 * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>
#include <drivers/arm/gicv2.h>
#include <plat/common/platform.h>
#include <ax620e_def.h>

static const gicv2_driver_data_t arm_gic_data = {
	.gicd_base = AX620E_GICD_BASE,
	.gicc_base = AX620E_GICC_BASE,
};

/******************************************************************************
 * ARM common helper to initialize the GICv2 only driver.
 *****************************************************************************/
void plat_ax620e_gic_driver_init(void)
{
	gicv2_driver_init(&arm_gic_data);
}

void plat_ax620e_gic_init(void)
{
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();
}

/******************************************************************************
 * ARM common helper to enable the GICv2 CPU interface
 *****************************************************************************/
void plat_ax620e_gic_cpuif_enable(void)
{
	gicv2_cpuif_enable();
}

/******************************************************************************
 * ARM common helper to disable the GICv2 CPU interface
 *****************************************************************************/
void plat_ax620e_gic_cpuif_disable(void)
{
	gicv2_cpuif_disable();
}

/******************************************************************************
 * ARM common helper to initialize the per cpu distributor interface in GICv2
 *****************************************************************************/
void plat_ax620e_gic_pcpu_init(void)
{
	gicv2_pcpu_distif_init();
}

/******************************************************************************
 * Stubs for Redistributor power management. Although GICv2 doesn't have
 * Redistributor interface, these are provided for the sake of uniform GIC API
 *****************************************************************************/
void plat_ax620e_gic_redistif_on(void)
{
	return;
}

void plat_ax620e_gic_redistif_off(void)
{
	return;
}


/******************************************************************************
 * ARM common helper to save & restore the GICv3 on resume from system suspend.
 * The normal world currently takes care of saving and restoring the GICv2
 * registers due to legacy reasons. Hence we just initialize the Distributor
 * on resume from system suspend.
 *****************************************************************************/
void plat_ax620e_gic_save(void)
{
	return;
}

void plat_ax620e_gic_resume(void)
{
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
}
