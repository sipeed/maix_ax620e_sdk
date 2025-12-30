/*
 * Copyright (c) 2013-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <lib/mmio.h>
#include <sec_glb.h>

#define IRAM0_SEC_MASK			0xF		/* 4bit & 4KB per bit & 16KB total */
#define IRAM1_SEC_MASK			0x1		/* 1bit & 4KB total */
/* first 4K not need protect in iram 0 per internal discussion */
#define IRAM0_PROT_4K_BITS		0xE
/* the only 4K in iram1 need protect per internal discussion */
#define IRAM1_PROT_4K_BITS		0x1

void sema_config(void)
{
#if 0
	/* clr PROT_SW: iram enable secure check */
	mmio_write_32(AXI2IRAM0_PROT_SW_CLR, IRAM0_SEC_MASK);
	mmio_write_32(AXI2IRAM1_PROT_SW_CLR, IRAM1_SEC_MASK);

	/* clr & set AXI2IRAM0_ADDR_4K: which 4KB need secure check */
	mmio_write_32(AXI2IRAM0_ADDR_4K_CLR, IRAM0_SEC_MASK);
	mmio_write_32(AXI2IRAM1_ADDR_4K_CLR, IRAM1_SEC_MASK);
	mmio_write_32(AXI2IRAM0_ADDR_4K_SET, IRAM0_PROT_4K_BITS);
	mmio_write_32(AXI2IRAM1_ADDR_4K_SET, IRAM1_PROT_4K_BITS);
#endif
}
