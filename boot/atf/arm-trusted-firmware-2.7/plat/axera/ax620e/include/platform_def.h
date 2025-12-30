/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <arch.h>
#include <plat/common/common_def.h>

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64

/*******************************************************************************
 * Generic platform constants
 ******************************************************************************/

/* Size of cacheable stacks */

#define PLATFORM_STACK_SIZE 0x800

#define	SYS_COUNTER_FREQ  24000000

#define FIRMWARE_WELCOME_STR		"Booting Trusted Firmware\n"

#define PLATFORM_SYSTEM_COUNT		1
#define PLATFORM_CLUSTER_COUNT		1
#define PLATFORM_CORE_COUNT             2
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL2

#define ARM_BL31_PLAT_PARAM_VAL		ULL(0x0f1e2d3c4b5a6978)

#define ARM_PWR_LVL0			MPIDR_AFFLVL0
#define ARM_PWR_LVL1			MPIDR_AFFLVL1
#define ARM_PWR_LVL2			MPIDR_AFFLVL2

/* Local power state for power domains in Run state. */
#define ARM_LOCAL_STATE_RUN	U(0)
/* Local power state for retention. Valid only for CPU power domains */
#define ARM_LOCAL_STATE_RET	U(1)
/* Local power state for OFF/power-down. Valid for CPU and cluster power
   domains */
#define ARM_LOCAL_STATE_OFF	U(2)

/*
 * This macro defines the deepest retention state possible. A higher state
 * id will represent an invalid or a power down state.
 */
#define PLAT_MAX_RET_STATE		U(1)

/*
 * This macro defines the deepest power down states possible. Any state ID
 * higher than this is invalid.
 */
#define PLAT_MAX_OFF_STATE		U(2)

/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/
#define TZDDR_BASE		ATF_IMG_ADDR
#define TZDDR_SIZE		ATF_IMG_PKG_SIZE

/*******************************************************************************
 * BL31 specific defines.
 ******************************************************************************/
#define BL31_BASE		(ATF_IMG_ADDR)
#define BL31_LIMIT		(BL31_BASE + ATF_IMG_PKG_SIZE)

#define CPU_SYS_SLEEP_RAM_ADDR		0x1100
#define DDR_SYS_SLEEP_RAM_ADDR		0x1800
#define DDR_SYS_WAKEUP_RAM_ADDR		0x1C00
#define CORE_WAKEUP_FROM_RAM_ADDR	0x2800
#define CORE_RUN_ON_RAM_ADDR		0x2C00
#define SECOND_CORE_WAKEUP_FROM_RAM_ADDR 0x3000
#define SUSPEND_ON_RAM_ADDR		0x3400
#define CPU_CLK_MUX_0_ADDR		0x4000
#define CPU_CLK_EB_0_ADDR		0x4004
#define CPU_CLK_EB_1_ADDR		0x4008
#define CPU_CLK_DIV_0_ADDR		0x400C
#define COMMON_CLK_MUX_0_ADDR 		0x4010
#define COMMON_CLK_MUX_2_ADDR 		0x4014
#define WAKEUP_START_TIMESTAMP_ADDR		0x402C
#define TIMESTAMP_START_STORE_IRAM_ADDR		0x4100
#define TIMESTAMP_STORE_IRAM_MAX_SIZE		0x64	/* 100 bytes used to store atf timestamp info. */
#define TIMESTAMP_START_STORE_IRAM_TMP_ADDR	0x4180
#define SLEEP_STAGE_STORE_ADDR	0x4200


/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define PLAT_VIRT_ADDR_SPACE_SIZE   (1ULL << 36)
#define PLAT_PHY_ADDR_SPACE_SIZE    (1ULL << 36)
#define MAX_XLAT_TABLES		9
#define MAX_MMAP_REGIONS	33

/*******************************************************************************
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT	6
#define CACHE_WRITEBACK_GRANULE	(1 << CACHE_WRITEBACK_SHIFT)

#endif /* PLATFORM_DEF_H */
