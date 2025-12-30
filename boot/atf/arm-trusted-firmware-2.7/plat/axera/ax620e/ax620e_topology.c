/*
 * Copyright (c) 2013-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>
#include <arch.h>
#include <lib/psci/psci.h>

/* The RockChip power domain tree descriptor */
const unsigned char ax620e_power_domain_tree_desc[] = {
	/* No of root nodes */
	PLATFORM_SYSTEM_COUNT,
	/* No of children for the root node */
	PLATFORM_CLUSTER_COUNT,
	/* No of children for the first cluster node */
	PLATFORM_CORE_COUNT,
};

/*******************************************************************************
 * This function returns the axera default topology tree information.
 ******************************************************************************/
const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return ax620e_power_domain_tree_desc;
}

int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	unsigned int cpu_id;

	cpu_id = MPIDR_AFFLVL0_VAL(mpidr);

	return cpu_id;
}
