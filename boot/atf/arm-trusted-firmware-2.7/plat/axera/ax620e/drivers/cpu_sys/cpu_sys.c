#include <cpu_sys.h>
#include <platform_def.h>
#include "wakeup_source.h"
#include "ax620e_common_sys_glb.h"
#include "pll.h"
#include "ddr_sys.h"
#include <sleep_stage.h>

int cpu_sys_sleep(void)
{
	int ret = 0;
	unsigned int val = 0;
	writel(AX_SLEEP_STAGE_0A, SLEEP_STAGE_STORE_ADDR);

	/* set cpu clk to cpll_208M */
	val = readl(CPU_SYS_CLK_MUX_0_ADDR);
	val &= ~(0x7 << 2);
	val |= (0x1 << 2);
	writel(val, CPU_SYS_CLK_MUX_0_ADDR);

	/* set aclk cpu top to cpll_208M */
	val = readl(COMMON_SYS_CLK_MUX_0_ADDR);
	val &= ~(0x7 << 24);
	val |= (0x1 << 24);
	writel(val, COMMON_SYS_CLK_MUX_0_ADDR);

	/* set pclk top to cpll_208M */
	writel((0x3 << 0), COMMON_SYS_CLK_MUX_2_ADDR);

	writel(BIT_CPU_SYS_CA53_CLUSTER_INT_DISABLE_SET | BIT_CPU_SYS_CA53_CPU_INT_DISABLE_SET, CPU_SYS_INT_MSK_SET_ADDR);

	writel(BIT_CPU_SYS_PCLK_CPU_DBGMNR_EB_CLR, CPU_SYS_CLK_EB_1_CLR_ADDR);
	writel(BIT_CPU_SYS_ACLK_CPU_DBGMNR_EB_CLR, CPU_SYS_CLK_EB_1_CLR_ADDR);
	writel(BIT_CPU_SYS_PCLK_CPU_PERFMNR_EB_CLR, CPU_SYS_CLK_EB_1_CLR_ADDR);
	writel(BIT_CPU_SYS_ACLK_CPU_PERFMNR_EB_CLR, CPU_SYS_CLK_EB_1_CLR_ADDR);
	writel(BIT_CPU_SYS_CLK_PERFMNR_24M_EB_CLR, CPU_SYS_CLK_EB_1_CLR_ADDR);
	writel(BIT_CPU_SYS_HCLK_SPI_EB_CLR, CPU_SYS_CLK_EB_1_CLR_ADDR);
	writel(BIT_CPU_SYS_CLK_EMMC_EB_CLR, CPU_SYS_CLK_EB_1_CLR_ADDR);
	writel(BIT_CPU_SYS_CLK_ROSC_EB_CLR, CPU_SYS_CLK_EB_1_CLR_ADDR);
	writel(BIT_CPU_SYS_CLK_BROM_EB_CLR, CPU_SYS_CLK_EB_0_CLR_ADDR);
	writel(BIT_CPU_SYS_CLK_EMMC_CARD_EB_CLR, CPU_SYS_CLK_EB_0_CLR_ADDR);
	writel(BIT_CPU_SYS_CLK_H_SSI_EB_CLR, CPU_SYS_CLK_EB_0_CLR_ADDR);

	writel(BIT_CPU_SYS_DATA_IDLE_EN_SET, CPU_SYS_LPC_CFG_2_SET_ADDR);
	writel(BIT_CPU_SYS_DATA_BUS_IDLE_EN_SET, CPU_SYS_LPC_CFG_1_SET_ADDR);
	writel(BIT_CPU_SYS_DATA_NOC_TIMEOUT_EN_SET, CPU_SYS_LPC_CFG_1_SET_ADDR);

	writel(BIT_CPU_SYS_CFG_BUS_IDLE_EN_SET, CPU_SYS_LPC_CFG_0_SET_ADDR);
	writel(BIT_CPU_SYS_CFG_NOC_TIMEOUT_EN_SET, CPU_SYS_LPC_CFG_0_SET_ADDR);

	/* clear all interrupt */
	writel(BITS_CPU_ERR_INT_CLEAR_SET, CPU_SYS_RSP2INT_CTRL_SET_ADDR);

	writel(BIT_CPU_SYS_SLP_EN_SET, CPU_SYS_LPC_CFG_2_SET_ADDR);

	ret = pmu_module_sleep_en(MODULE_CPU, SLP_EN_SET);

	writel(AX_SLEEP_STAGE_0B, SLEEP_STAGE_STORE_ADDR);

	/* clear pll clock gating eb , except cpll */
	writel(0x7e, PLL_GRP_PLL_RE_OPEN_CLR_ADDR);
	writel(0x0, DPLL_PLL_RE_OPEN_ADDR);

	return ret;
}

int cpu_sys_helper(void)
{
	return 0;
}