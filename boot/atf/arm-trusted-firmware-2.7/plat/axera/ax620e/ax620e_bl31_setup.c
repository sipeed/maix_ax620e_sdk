/*
 * Copyright (c) 2015-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <lib/debugfs.h>
#include <lib/extensions/ras.h>
#if ENABLE_RME
#include <lib/gpt_rme/gpt_rme.h>
#endif
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_compat.h>
#include <ax620e_def.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <drivers/ti/uart/uart_16550.h>
#include <plat_ax620e.h>
#include <ax620e_pwrc.h>
#include <firewall.h>
#include <sema.h>
#include <sec_glb.h>
#include <wakeup_source.h>
#include <pll.h>
#include "ax620e_common_sys_glb.h"

extern int ddr_sys_helper(void);
extern int ddr_sys_sleep(void);
extern int ddr_sys_wakeup(void);
extern int cpu_sys_helper(void);
extern int cpu_sys_sleep(void);
extern void core_wakeup_from_ram(void);
extern void core_run_on_ram(void);
extern void jump_to_ram(unsigned long addr);
extern void suspend_on_ram(void);
extern void second_core_wakeup_from_ram(void);

/*
 * Placeholder variables for copying the arguments that have been passed to
 * BL31 from BL2.
 */
static entry_point_info_t bl32_image_ep_info;
static entry_point_info_t bl33_image_ep_info;
#if ENABLE_RME
static entry_point_info_t rmm_image_ep_info;
#endif
console_t console;

void plat_ax620e_secure_config(void)
{
	firewall_config();
	sema_config();

	/* efuse secure protect */
	mmio_write_32(EFUSE_CTRL, 0);
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for the
 * security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type. A NULL pointer is returned
 * if the image does not exist.
 ******************************************************************************/
struct entry_point_info *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	assert(sec_state_is_valid(type));
	if (type == NON_SECURE) {
		next_image_info = &bl33_image_ep_info;
	}
#if ENABLE_RME
	else if (type == REALM) {
		next_image_info = &rmm_image_ep_info;
	}
#endif
	else {
		next_image_info = &bl32_image_ep_info;
	}

	/*
	 * None of the images on the ARM development platforms can have 0x0
	 * as the entrypoint
	 */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

/*******************************************************************************
 * Perform any BL31 early platform setup common to ARM standard platforms.
 * Here is an opportunity to copy parameters passed by the calling EL (S-EL1
 * in BL2 & EL3 in BL1) before they are lost (potentially). This needs to be
 * done before the MMU is initialized so that the memory layout can be used
 * while creating page tables. BL2 has flushed this information to memory, so
 * we are guaranteed to pick up good data.
 ******************************************************************************/
void __init ax620e_bl31_early_platform_setup(void *from_bl2, uintptr_t soc_fw_config,
				uintptr_t hw_config, void *plat_params_from_bl2)
{
	/* Initialize the console to provide early debug support */
	console_16550_register(UART0_BASE, AX620E_UART_CLOCK,
			       AX620E_BAUDRATE, &console);

#if RESET_TO_BL31
	/* There are no parameters from BL2 if BL31 is a reset vector */
	assert(from_bl2 == NULL);
	assert(plat_params_from_bl2 == NULL);

# ifdef BL32_BASE
	/* Populate entry point information for BL32 */
	SET_PARAM_HEAD(&bl32_image_ep_info,
				PARAM_EP,
				VERSION_1,
				0);
	SET_SECURITY_STATE(bl32_image_ep_info.h.attr, SECURE);
	bl32_image_ep_info.pc = BL32_BASE;
	bl32_image_ep_info.spsr = 0;

#if defined(SPD_spmd)
	/* SPM (hafnium in secure world) expects SPM Core manifest base address
	 * in x0, which in !RESET_TO_BL31 case loaded after base of non shared
	 * SRAM(after 4KB offset of SRAM). But in RESET_TO_BL31 case all non
	 * shared SRAM is allocated to BL31, so to avoid overwriting of manifest
	 * keep it in the last page.
	 */
	bl32_image_ep_info.args.arg0 = ARM_TRUSTED_SRAM_BASE +
				PLAT_ARM_TRUSTED_SRAM_SIZE - PAGE_SIZE;
#endif

# endif /* BL32_BASE */

	/* Populate entry point information for BL33 */
	SET_PARAM_HEAD(&bl33_image_ep_info,
				PARAM_EP,
				VERSION_1,
				0);
	/*
	 * Tell BL31 where the non-trusted software image
	 * is located and the entry state information
	 */
	bl33_image_ep_info.pc = ax650_get_ns_image_entrypoint();

	bl33_image_ep_info.spsr = arm_get_spsr_for_bl33_entry();
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);

#else /* RESET_TO_BL31 */

	/*
	 * In debug builds, we pass a special value in 'plat_params_from_bl2'
	 * to verify platform parameters from BL2 to BL31.
	 * In release builds, it's not used.
	 */
	assert(((unsigned long long)plat_params_from_bl2) ==
		ARM_BL31_PLAT_PARAM_VAL);

	/*
	 * Check params passed from BL2 should not be NULL,
	 */
	bl_params_t *params_from_bl2 = (bl_params_t *)from_bl2;
	assert(params_from_bl2 != NULL);
	assert(params_from_bl2->h.type == PARAM_BL_PARAMS);
	assert(params_from_bl2->h.version >= VERSION_2);

	bl_params_node_t *bl_params = params_from_bl2->head;

	/*
	 * Copy BL33, BL32 and RMM (if present), entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	while (bl_params != NULL) {
		if (bl_params->image_id == BL32_IMAGE_ID) {
			bl32_image_ep_info = *bl_params->ep_info;
		}
#if ENABLE_RME
		else if (bl_params->image_id == RMM_IMAGE_ID) {
			rmm_image_ep_info = *bl_params->ep_info;
		}
#endif
		else if (bl_params->image_id == BL33_IMAGE_ID) {
			bl33_image_ep_info = *bl_params->ep_info;
		}

		bl_params = bl_params->next_params_info;
	}

	if (bl33_image_ep_info.pc == 0U)
		panic();
#if ENABLE_RME
	if (rmm_image_ep_info.pc == 0U)
		panic();
#endif
#endif /* RESET_TO_BL31 */

# if ARM_LINUX_KERNEL_AS_BL33
	/*
	 * According to the file ``Documentation/arm64/booting.txt`` of the
	 * Linux kernel tree, Linux expects the physical address of the device
	 * tree blob (DTB) in x0, while x1-x3 are reserved for future use and
	 * must be 0.
	 * Repurpose the option to load Hafnium hypervisor in the normal world.
	 * It expects its manifest address in x0. This is essentially the linux
	 * dts (passed to the primary VM) by adding 'hypervisor' and chosen
	 * nodes specifying the Hypervisor configuration.
	 */
#if RESET_TO_BL31
	bl33_image_ep_info.args.arg0 = (u_register_t)ARM_PRELOADED_DTB_BASE;
#else
	bl33_image_ep_info.args.arg0 = (u_register_t)hw_config;
#endif
	bl33_image_ep_info.args.arg1 = 0U;
	bl33_image_ep_info.args.arg2 = 0U;
	bl33_image_ep_info.args.arg3 = 0U;
# endif
}

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
		u_register_t arg2, u_register_t arg3)
{
	ax620e_bl31_early_platform_setup((void *)arg0, arg1, arg2, (void *)arg3);
}

void bl31_platform_setup(void)
{

	// /* Initialize the GIC driver, cpu and distributor interfaces */
	plat_ax620e_gic_driver_init();
	plat_ax620e_gic_init();

	// /* Initialize power controller before setting up topology */
	plat_ax620e_pwrc_setup();

	plat_ax620e_secure_config();

	writel(0x0, AX620E_TIMER32_WAIT_COUNT_ADDR);
	writel(0x1, AX620E_TIMER32_EB_ADDR);
	*((volatile unsigned long*)WAKEUP_START_TIMESTAMP_ADDR) = 0x0;

	//timer32_init();
}

void bl31_plat_runtime_setup(void)
{
}

const mmap_region_t plat_ax620e_mmap[] = {

	MAP_REGION_FLAT(COMMON_CPU_GLB_BASE, COMMON_CPU_GLB_SIZE,
	 	MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(COMMON_SYS_GLB_BASE, COMMON_SYS_GLB_SIZE,
	 	MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(GIC400_BASE, GIC400_SIZE,
		MT_DEVICE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(DDR_SYS_BASE, DDR_SYS_SIZE,
		MT_DEVICE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(CPU_SYS_GLB_BASE, CPU_SYS_GLB_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(GTMER_BASER, GTMER_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
#ifdef SUSPEND_EB
	MAP_REGION_FLAT(ISP_SYS_BASE, ISP_SYS_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(NPU_SYS_BASE, NPU_SYS_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(VPU_SYS_BASE, VPU_SYS_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(MM_SYS_BASE, MM_SYS_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
#endif
	MAP_REGION_FLAT(PMU_BASE, PMU_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(PERIPH_SYS_BASE, PERIPH_SYS_SIZE,
		MT_DEVICE | MT_RW | MT_SECURE),
#ifdef SUSPEND_EB
	MAP_REGION_FLAT(FLASH_SYS_BASE, FLASH_SYS_SIZE,
	 	MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(PINMUX_G6_BASE, PINMUX_G6_SIZE,
	 	MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(TIMER32_BASE, TIMER32_SIZE,
	 	MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(TIMER64_BASE, TIMER64_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
#endif
	MAP_REGION_FLAT(DEBUG_SYS_BASE, DEBUG_SYS_SIZE,
		MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(PLL_GLB_REG_BASE, SIZE_K(64),
		MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(COMMON_SYS_FAB_TOP_GLB_BASE, SIZE_K(64),
		MT_DEVICE | MT_RW | MT_NS),
	{0},
};

/*******************************************************************************
 * Perform the very early platform specific architectural setup shared between
 * ARM standard platforms. This only does basic initialization. Later
 * architectural setup (bl31_arch_setup()) does not do anything platform
 * specific.
 ******************************************************************************/
static void ax620e_bl31_plat_arch_setup(void)
{
#ifdef SUSPEND_EB
	unsigned long cpu_sys_sleep_ram_addr = 0;
	unsigned long ddr_sys_sleep_ram_addr = 0;
	unsigned long ddr_sys_wakeup_ram_addr = 0;
	unsigned long core_wakeup_from_ram_addr = 0;
	unsigned long core_run_on_ram_addr = 0;
	unsigned long second_core_run_on_ram_addr = 0;
	unsigned long suspend_on_ram_addr = 0;

	cpu_sys_sleep_ram_addr = CPU_SYS_SLEEP_RAM_ADDR;
	ddr_sys_sleep_ram_addr = DDR_SYS_SLEEP_RAM_ADDR;
	ddr_sys_wakeup_ram_addr = DDR_SYS_WAKEUP_RAM_ADDR;
	core_wakeup_from_ram_addr = CORE_WAKEUP_FROM_RAM_ADDR;
	core_run_on_ram_addr = CORE_RUN_ON_RAM_ADDR;
	second_core_run_on_ram_addr = SECOND_CORE_WAKEUP_FROM_RAM_ADDR;
	suspend_on_ram_addr = SUSPEND_ON_RAM_ADDR;

	memcpy((void*)cpu_sys_sleep_ram_addr, (void*)cpu_sys_sleep,
		(unsigned long)cpu_sys_helper - (unsigned long)cpu_sys_sleep );
	memcpy((void*)ddr_sys_sleep_ram_addr, (void*)ddr_sys_sleep,
		(unsigned long)ddr_sys_wakeup - (unsigned long)ddr_sys_sleep );
	memcpy((void*)ddr_sys_wakeup_ram_addr, (void*)ddr_sys_wakeup,
		(unsigned long)ddr_sys_helper - (unsigned long)ddr_sys_wakeup );
	memcpy((void*)core_wakeup_from_ram_addr, (void*)core_wakeup_from_ram,
		(unsigned long)core_run_on_ram - (unsigned long)core_wakeup_from_ram);
	memcpy((void*)core_run_on_ram_addr, (void*)core_run_on_ram,
		(unsigned long)second_core_wakeup_from_ram - (unsigned long)core_run_on_ram);
	memcpy((void*)second_core_run_on_ram_addr, (void*)second_core_wakeup_from_ram,
		(unsigned long)suspend_on_ram - (unsigned long)second_core_wakeup_from_ram);
	memcpy((void*)suspend_on_ram_addr, (void*)suspend_on_ram,
		(unsigned long)jump_to_ram - (unsigned long)suspend_on_ram);
#endif

	const mmap_region_t bl_regions[] = {
		MAP_REGION_FLAT(RUN_ON_DDR_BASE, RUN_ON_DDR_SIZE, MT_MEMORY | MT_RW | MT_NS),
#ifdef SUSPEND_EB
		MAP_REGION_FLAT(IRAM0_BASE,  IRAM0_SIZE, MT_MEMORY | MT_RW | MT_SECURE),
		MAP_REGION_FLAT(IRAM1_BASE,  IRAM1_SIZE, MT_MEMORY | MT_RW | MT_SECURE),
		MAP_REGION_FLAT(IROM_BASE, IROM_SIZE, MT_MEMORY | MT_RW | MT_NS),
		MAP_REGION_FLAT(OCM_BASE, OCM_SIZE, MT_MEMORY | MT_RW | MT_NS),
#endif
		MAP_REGION_FLAT(BL_CODE_BASE, BL_CODE_END - BL_CODE_BASE, MT_CODE | MT_RO | MT_SECURE),
		MAP_REGION_FLAT(BL_RO_DATA_BASE, BL_RO_DATA_END - BL_RO_DATA_BASE, MT_RO_DATA | MT_RO | MT_SECURE),
		{0}
	};

	setup_page_tables(bl_regions, plat_ax620e_mmap);
	enable_mmu_el3(0);
}

void bl31_plat_arch_setup(void)
{
	ax620e_bl31_plat_arch_setup();
}
