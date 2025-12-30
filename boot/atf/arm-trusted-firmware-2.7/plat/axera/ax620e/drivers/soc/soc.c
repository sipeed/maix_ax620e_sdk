/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

unsigned int plat_get_syscnt_freq2(void)
{
	return SYS_COUNTER_FREQ;
}
