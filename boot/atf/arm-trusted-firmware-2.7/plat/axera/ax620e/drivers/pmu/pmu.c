/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <pmu.h>

void pmu_init(void)
{
	mmio_write_32(PMU_GLB_PWR_WAIT0_ADDR, 0);
	mmio_write_32(PMU_GLB_PWR_WAIT1_ADDR, 0);
	mmio_write_32(PMU_GLB_PWR_WAITON0_ADDR, 0x0);
	mmio_write_32(PMU_GLB_PWR_WAITON1_ADDR, 0x0);

	mmio_write_32(PMU_GLB_INT_CLR_SET_ADDR, 0xFFFF);
}


