/*
 * Copyright (c) 2013-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <plat_ax620e.h>
#include <ax620e_pwrc.h>
#include <platform_def.h>
#include <ax620e_common_sys_glb.h>
#include <cpu_sys.h>
#include <ddr_sys.h>
#include <flash_sys.h>
#include <isp_sys.h>
#include <mm_sys.h>
#include <npu_sys.h>
#include <periph_sys.h>
#include <vpu_sys.h>
#include <chip_top.h>
#include <wakeup.h>
#include <wakeup_source.h>
#include <ax620e_def.h>
#include <timestamp.h>
#include <sleep_stage.h>

extern void ret_to_addr(uint32_t addr_high, uint32_t addr_lo);
extern uint8_t plat_my_core_pos(void);
extern void core_wakeup_from_ram(void);
extern void core_run_on_ram(void);
extern void jump_to_ram(unsigned long addr);
extern void suspend_on_ram(void);
extern void second_core_wakeup_from_ram(void);

uintptr_t ax620e_sec_entrypoint;

#ifdef SUSPEND_EB
#ifdef CONFIG_CLK_AUX_SUSPEND_DISABLE
static void clk_aux_disabled(void)
{
	writel(0x10, COMMON_SYS_AUX_CFG1_CLR_ADDR);
}

static void clk_aux_enabled(void)
{
	writel(0x10, COMMON_SYS_AUX_CFG1_SET_ADDR);
}
#endif
#endif

/*******************************************************************************
 * Function which implements the common ax620e specific operations to power down a
 * cluster in response to a CPU_OFF or CPU_SUSPEND request.
 ******************************************************************************/
static void ax620e_cluster_pwrdwn_common(void)
{
}

/******************************************************************************
 * Helper function to resume the platform from system suspend. Reinitialize
 * the system components which are not in the Always ON power domain.
 * TODO: Unify the platform setup when waking up from cold boot and system
 * resume in arm_bl31_platform_setup().
 *****************************************************************************/
void ax620e_system_pwr_domain_resume(void)
{
	/* Assert system power domain is available on the platform */
	assert(PLAT_MAX_PWR_LVL >= ARM_PWR_LVL2);
	writel(AX_WAKEUP_STAGE_14, SLEEP_STAGE_STORE_ADDR);

	plat_ax620e_gic_resume();

	writel(AX_WAKEUP_STAGE_15, SLEEP_STAGE_STORE_ADDR);

#ifdef SUSPEND_EB
#ifdef CONFIG_CLK_AUX_SUSPEND_DISABLE
	clk_aux_enabled();
#endif
	wakeup_sys();
#endif
}

static void ax620e_power_domain_on_finish_common(const psci_power_state_t *target_state)
{

	assert(target_state->pwr_domain_state[ARM_PWR_LVL0] ==
					ARM_LOCAL_STATE_OFF);

	/* Perform the common cluster specific operations */
	if (target_state->pwr_domain_state[ARM_PWR_LVL1] ==
					ARM_LOCAL_STATE_OFF) {
	}

	/* Perform the common system specific operations */
	if (target_state->pwr_domain_state[ARM_PWR_LVL2] ==
						ARM_LOCAL_STATE_OFF)
		ax620e_system_pwr_domain_resume();
}

/*******************************************************************************
 * ax620e handler called when a CPU is about to enter standby.
 ******************************************************************************/
static void ax620e_cpu_standby(plat_local_state_t cpu_state)
{
}

/*******************************************************************************
 * ax620e handler called when a power domain is about to be turned on. The
 * mpidr determines the CPU to be turned on.
 ******************************************************************************/
static int ax620e_pwr_domain_on(u_register_t mpidr)
{
	int rc = PSCI_E_SUCCESS;
	unsigned int state;

	/*
	 * Ensure that we do not cancel an inflight power off request for the
	 * target cpu. That would leave it in a zombie wfi. Wait for it to power
	 * off and then program the power controller to turn that CPU on.
	 */
	do {
		state = ax620e_pwrc_read_psysr(mpidr);
	} while ((state != PWR_STATE_ON && state != PWR_STATE_OFF) != 0U);

	ax620e_pwrc_write_pponr(mpidr);
	return rc;
}

/*******************************************************************************
 * ax620e handler called when a power domain is about to be turned off. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
static void ax620e_pwr_domain_off(const psci_power_state_t *target_state)
{
	/* Prevent interrupts from spuriously waking up this cpu */
	plat_ax620e_gic_cpuif_disable();

	/* Turn redistributor off */
	plat_ax620e_gic_redistif_off();
}

void __dead2 ax620e_pwr_domain_pwr_down_wfi(const psci_power_state_t *target_state)
{

	uint32_t addr_high = 0, addr_lo = 0;
	uint8_t core_id;
	core_id = plat_my_core_pos();

	clean_dcache_range(ATF_IMG_ADDR, ATF_IMG_PKG_SIZE);

	disable_mmu_el3(); //close mmu before jump to ram

	if (target_state->pwr_domain_state[ARM_PWR_LVL2] == ARM_LOCAL_STATE_OFF) {
		ax_sys_sleeptimestamp(AX_ID_ATF, AX_SUB_ID_SUSPEND_END);
		/* code after that run on ram */
		jump_to_ram(SUSPEND_ON_RAM_ADDR);
	} else {
		ax620e_pwrc_write_ppoffr(read_mpidr_el1());

		if (core_id == 0) {
			addr_high = mmio_read_32(COMM_SYS_DUMMY_SW3_CORE0_HIGH);
			addr_lo = mmio_read_32(COMM_SYS_DUMMY_SW2_CORE0_LOW);
		} else if (core_id == 1){
			addr_high = mmio_read_32(COMM_SYS_DUMMY_SW1_CORE1_HIGH);
			addr_lo = mmio_read_32(COMM_SYS_DUMMY_SW0_CORE1_LOW);
		}

		if (core_id == 0) {
			mmio_write_32(COMM_SYS_DUMMY_SW2_CORE0_LOW, 0x0);
			mmio_write_32(COMM_SYS_DUMMY_SW3_CORE0_HIGH, 0x0);
		} else if (core_id == 1) {
			mmio_write_32(COMM_SYS_DUMMY_SW0_CORE1_LOW, 0x0);
			mmio_write_32(COMM_SYS_DUMMY_SW1_CORE1_HIGH, 0x0);
		}
		//disable_mmu_el3();
		ret_to_addr(addr_high, addr_lo);
	}
	while(1);
}

/******************************************************************************
 * Helper function to save the platform state before a system suspend. Save the
 * state of the system components which are not in the Always ON power domain.
 *****************************************************************************/
void ax620e_system_pwr_domain_save(void)
{
	uint8_t core_id;
	int val;
	core_id = plat_my_core_pos();
	unsigned int core_run_on_ram_low  = 0;
	unsigned int core_run_on_ram_high = 0;
	unsigned int core_wakeup_from_ram_low  = 0;
	unsigned int core_wakeup_from_ram_high = 0;
#ifdef SUSPEND_EB
	unsigned int cpu_clk_mux_0_reserve;
	unsigned int cpu_clk_eb_0_reserve;
	unsigned int cpu_clk_eb_1_reserve;
	unsigned int cpu_clk_div_0_reserve;
	unsigned int common_clk_mux_0_reserve;
	unsigned int common_clk_mux_2_reserve;
#endif

	core_run_on_ram_low  = ((uintptr_t)CORE_RUN_ON_RAM_ADDR) & 0xFFFFFFFF;
	core_run_on_ram_high = ((uintptr_t)CORE_RUN_ON_RAM_ADDR) >> 32;
	core_wakeup_from_ram_low  = ((uintptr_t)CORE_WAKEUP_FROM_RAM_ADDR) & 0xFFFFFFFF;
	core_wakeup_from_ram_high = ((uintptr_t)CORE_WAKEUP_FROM_RAM_ADDR) >> 32;

	if (core_id == 0) {
		mmio_write_32(COMM_SYS_DUMMY_SW1_CORE1_HIGH, core_run_on_ram_high);
		mmio_write_32(COMM_SYS_DUMMY_SW0_CORE1_LOW, core_run_on_ram_low);
		mmio_write_32(CA53_CFG_RVBARADDR0_H, core_wakeup_from_ram_high);
		mmio_write_32(CA53_CFG_RVBARADDR0_L, core_wakeup_from_ram_low);
	} else if (core_id == 1) {
		mmio_write_32(COMM_SYS_DUMMY_SW3_CORE0_HIGH, core_run_on_ram_high);
		mmio_write_32(COMM_SYS_DUMMY_SW2_CORE0_LOW, core_run_on_ram_low);
		mmio_write_32(CA53_CFG_RVBARADDR1_H, core_wakeup_from_ram_high);
		mmio_write_32(CA53_CFG_RVBARADDR1_L, core_wakeup_from_ram_low);
	}
	sev();
#ifdef SUSPEND_EB
	NOTICE("start sleep\r\n");

	writel(AX_SLEEP_STAGE_00, SLEEP_STAGE_STORE_ADDR);

	val = readl(DDRC_DDRMC_PORT0_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT0;
	writel(val, DDRC_DDRMC_PORT0_CFG0_ADDR);

	val = readl(DDRC_DDRMC_PORT1_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT1;
	writel(val, DDRC_DDRMC_PORT1_CFG0_ADDR);

	val = readl(DDRC_DDRMC_PORT2_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT2;
	writel(val, DDRC_DDRMC_PORT2_CFG0_ADDR);

	val = readl(DDRC_DDRMC_PORT3_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT3;
	writel(val, DDRC_DDRMC_PORT3_CFG0_ADDR);

	val = readl(DDRC_DDRMC_PORT4_CFG0_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN_PORT4;
	writel(val, DDRC_DDRMC_PORT4_CFG0_ADDR);

	val = readl(DDRC_DDRMC_CFG22_ADDR);
	val |= BIT_DDRC_RF_AUTO_SLP_EN;
	writel(val, DDRC_DDRMC_CFG22_ADDR);

	/* eic riscv en clr */
	writel(0x0, EIC_RISCV_EN);
	/* eic mask en set */
	writel(0x1, EIC_MASK_EN_SET);

#ifdef CONFIG_CLK_AUX_SUSPEND_DISABLE
	clk_aux_disabled();
#endif
	flash_sys_sleep();
	writel(AX_SLEEP_STAGE_01, SLEEP_STAGE_STORE_ADDR);
	npu_sys_sleep();
	writel(AX_SLEEP_STAGE_02, SLEEP_STAGE_STORE_ADDR);
	isp_sys_sleep();
	writel(AX_SLEEP_STAGE_03, SLEEP_STAGE_STORE_ADDR);
	mm_sys_sleep();
	writel(AX_SLEEP_STAGE_04, SLEEP_STAGE_STORE_ADDR);
	vpu_sys_sleep();
	writel(AX_SLEEP_STAGE_05, SLEEP_STAGE_STORE_ADDR);
	peri_sys_sleep();
	/* check flash isp mm npu vpu peri status */
	while(readl(PMU_GLB_PWR_STATE_ADDR) != 0x55555500);
	writel(AX_SLEEP_STAGE_06, SLEEP_STAGE_STORE_ADDR);
	cpu_clk_mux_0_reserve = readl(CPU_SYS_GLB_BASE_ADDR);
	cpu_clk_eb_0_reserve = readl(CPU_SYS_CLK_EB_0_ADDR);
	cpu_clk_eb_1_reserve = readl(CPU_SYS_CLK_EB_1_ADDR);
	cpu_clk_div_0_reserve = readl(CPU_SYS_CLK_DIV_0_ADDR);
	common_clk_mux_0_reserve = readl(COMMON_SYS_CLK_MUX_0_ADDR);
	common_clk_mux_2_reserve = readl(COMMON_SYS_CLK_MUX_2_ADDR);
	memcpy((void*)CPU_CLK_MUX_0_ADDR, (void*)&cpu_clk_mux_0_reserve, 4);
	memcpy((void*)CPU_CLK_EB_0_ADDR, (void*)&cpu_clk_eb_0_reserve, 4);
	memcpy((void*)CPU_CLK_EB_1_ADDR, (void*)&cpu_clk_eb_1_reserve, 4);
	memcpy((void*)CPU_CLK_DIV_0_ADDR, (void*)&cpu_clk_div_0_reserve, 4);
	memcpy((void*)COMMON_CLK_MUX_0_ADDR, (void*)&common_clk_mux_0_reserve, 4);
	memcpy((void*)COMMON_CLK_MUX_2_ADDR, (void*)&common_clk_mux_2_reserve, 4);
#ifdef TIMER32_WAKEUP_EB
	timer32_wakeup_config(TIMER32_COUNT_PER_SEC);  //default wakeup time 1s
#endif
	writel(AX_SLEEP_STAGE_07, SLEEP_STAGE_STORE_ADDR);
#endif
}

/*******************************************************************************
 * ax620e handler called when a power domain is about to be suspended. The
 * target_state encodes the power state that each level should transition to.
 ******************************************************************************/
static void ax620e_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	NOTICE("enter ax620e_pwr_domain_suspend\r\n");
	/*
	 * ax620e has retention only at cpu level. Just return
	 * as nothing is to be done for retention.
	 */
	ax_sys_sleeptimestamp(AX_ID_ATF, AX_SUB_ID_SUSPEND_START);
	if (target_state->pwr_domain_state[ARM_PWR_LVL0] ==
					ARM_LOCAL_STATE_RET)
		return;

	assert(target_state->pwr_domain_state[ARM_PWR_LVL0] ==
					ARM_LOCAL_STATE_OFF);

	/* Prevent interrupts from spuriously waking up this cpu */
	plat_ax620e_gic_cpuif_disable();

	/* Perform the common cluster specific operations */
	if (target_state->pwr_domain_state[ARM_PWR_LVL1] ==
					ARM_LOCAL_STATE_OFF)
		ax620e_cluster_pwrdwn_common();

	/* Perform the common system specific operations */
	if (target_state->pwr_domain_state[ARM_PWR_LVL2] ==
						ARM_LOCAL_STATE_OFF)
		ax620e_system_pwr_domain_save();
}

/*******************************************************************************
 * ax620e handler called when a power domain has just been powered on after
 * being turned off earlier. The target_state encodes the low power state that
 * each level has woken up from.
 ******************************************************************************/
static void ax620e_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	ax620e_power_domain_on_finish_common(target_state);
}

/*******************************************************************************
 * ax620e handler called when a power domain has just been powered on and the cpu
 * and its cluster are fully participating in coherent transaction on the
 * interconnect. Data cache must be enabled for CPU at this point.
 ******************************************************************************/
static void ax620e_pwr_domain_on_finish_late(const psci_power_state_t *target_state)
{
	/* Program GIC per-cpu distributor or re-distributor interface */
	plat_ax620e_gic_pcpu_init();

	/* Enable GIC CPU interface */
	plat_ax620e_gic_cpuif_enable();
}

/*******************************************************************************
 * ax620e handler called when a power domain has just been powered on after
 * having been suspended earlier. The target_state encodes the low power state
 * that each level has woken up from.
 * TODO: At the moment we reuse the on finisher and reinitialize the secure
 * context. Need to implement a separate suspend finisher.
 ******************************************************************************/
static void ax620e_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	unsigned long timer64_val = 0;

	if (target_state->pwr_domain_state[ARM_PWR_LVL0] ==
					ARM_LOCAL_STATE_RET)
		return;

	writel(AX_WAKEUP_STAGE_13, SLEEP_STAGE_STORE_ADDR);

	/* wakeup timer64 start */
	*((volatile unsigned long*)WAKEUP_START_TIMESTAMP_ADDR) = 0x0;
	timer64_val = *((volatile unsigned long*)TIMER64_CNT_LOW_ADDR);
	*((volatile unsigned long*)WAKEUP_START_TIMESTAMP_ADDR) = timer64_val;

	ax_sys_sleeptimestamp(AX_ID_ATF, AX_SUB_ID_RESUME_START);

	ax620e_power_domain_on_finish_common(target_state);

	/* Enable GIC CPU interface */
	plat_ax620e_gic_cpuif_enable();

	ax_sys_sleeptimestamp(AX_ID_ATF, AX_SUB_ID_RESUME_END);
}

static int ax620e_node_hw_state(u_register_t target_cpu,
			     unsigned int power_level)
{
	return 0;
}

/*
 * The ax620e doesn't truly support power management at SYSTEM power domain. The
 * SYSTEM_SUSPEND will be down-graded to the cluster level within the platform
 * layer. The `fake` SYSTEM_SUSPEND allows us to validate some of the driver
 * save and restore sequences on ax620e.
 */
#if !ARM_BL31_IN_DRAM
static void ax620e_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	unsigned int i;

	for (i = ARM_PWR_LVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = ARM_LOCAL_STATE_OFF;
}
#endif

/*******************************************************************************
 * ARM standard platform handler called to check the validity of the power state
 * parameter.
 ******************************************************************************/
int validate_power_state(unsigned int power_state,
			    psci_power_state_t *req_state)
{
	return PSCI_E_SUCCESS;
}


/*******************************************************************************
 * Handler to filter PSCI requests.
 ******************************************************************************/
/*
 * The system power domain suspend is only supported only via
 * PSCI SYSTEM_SUSPEND API. PSCI CPU_SUSPEND request to system power domain
 * will be downgraded to the lower level.
 */
static int ax620e_validate_power_state(unsigned int power_state,
			    psci_power_state_t *req_state)
{
	return 0;
}

/*
 * Custom `translate_power_state_by_mpidr` handler for ax620e. Unlike in the
 * `ax620e_validate_power_state`, we do not downgrade the system power
 * domain level request in `power_state` as it will be used to query the
 * PSCI_STAT_COUNT/RESIDENCY at the system power domain level.
 */
static int ax620e_translate_power_state_by_mpidr(u_register_t mpidr,
		unsigned int power_state,
		psci_power_state_t *output_state)
{
	// return validate_power_state(power_state, output_state);
	return 0;
}

/*******************************************************************************
 * ARM standard platform handler called to check the validity of the non secure
 * entrypoint. Returns 0 if the entrypoint is valid, or -1 otherwise.
 ******************************************************************************/
int ax620e_validate_ns_entrypoint(uintptr_t entrypoint)
{
	return 0;
}

int ax620e_validate_psci_entrypoint(uintptr_t entrypoint)
{
	return (ax620e_validate_ns_entrypoint(entrypoint) == 0) ? PSCI_E_SUCCESS :
		PSCI_E_INVALID_ADDRESS;
}


/*******************************************************************************
 * Export the platform handlers via plat_arm_psci_pm_ops. The ARM Standard
 * platform layer will take care of registering the handlers with PSCI.
 ******************************************************************************/
plat_psci_ops_t plat_ax620e_psci_pm_ops = {
	.cpu_standby = ax620e_cpu_standby,
	.pwr_domain_on = ax620e_pwr_domain_on,
	.pwr_domain_off = ax620e_pwr_domain_off,
	.pwr_domain_pwr_down_wfi = ax620e_pwr_domain_pwr_down_wfi,
	.pwr_domain_suspend = ax620e_pwr_domain_suspend,
	.pwr_domain_on_finish = ax620e_pwr_domain_on_finish,
	.pwr_domain_on_finish_late = ax620e_pwr_domain_on_finish_late,
	.pwr_domain_suspend_finish = ax620e_pwr_domain_suspend_finish,
	.validate_power_state = ax620e_validate_power_state,
	.validate_ns_entrypoint = ax620e_validate_psci_entrypoint,
	.translate_power_state_by_mpidr = ax620e_translate_power_state_by_mpidr,
	.get_node_hw_state = ax620e_node_hw_state,
#if !ARM_BL31_IN_DRAM
	/*
	 * The TrustZone Controller is set up during the warmboot sequence after
	 * resuming the CPU from a SYSTEM_SUSPEND. If BL31 is located in SRAM
	 * this is  not a problem but, if it is in TZC-secured DRAM, it tries to
	 * reconfigure the same memory it is running on, causing an exception.
	 */
	.get_sys_suspend_power_state = ax620e_get_sys_suspend_power_state,
#endif
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	ax620e_sec_entrypoint = sec_entrypoint;

	*psci_ops = &plat_ax620e_psci_pm_ops;

	return 0;
}

