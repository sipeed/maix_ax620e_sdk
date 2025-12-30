#include <npu_sys.h>

unsigned int npu_sys_clk_mux_0_reserve = 0;
unsigned int npu_sys_clk_eb_0_reserve = 0;
unsigned int npu_sys_clk_eb_1_reserve = 0;

int npu_sys_sleep(void)
{
	unsigned int state;
	int ret;

	npu_sys_clk_mux_0_reserve = readl(NPU_SYS_CLK_MUX_0_ADDR);
	npu_sys_clk_eb_0_reserve = readl(NPU_SYS_CLK_EB_0_ADDR);
	npu_sys_clk_eb_1_reserve = readl(NPU_SYS_CLK_EB_1_ADDR);

	/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
	writel(BIT_NPU_SYS_CFG_BUS_IDLE_EN_SET, NPU_SYS_LPC_CFG_SETTING_SET_ADDR);
	writel(BIT_NPU_SYS_DATA_BUS_IDLE_EN_SET, NPU_SYS_LPC_DATA_SETTING_SET_ADDR);
	writel(BIT_NPU_SYS_DATA_IDLE_EB_SET, NPU_SYS_LPC_SETTING_SET_ADDR);
	writel(BIT_NPU_SYS_LPC_SW_RST_CLR, NPU_SYS_SW_RST_0_CLR_ADDR);

	/* wait until eu_idle_sts = 1 */
	while (readl(NPU_SYS_EU_IDLE_STS_ADDR) != 0x1);

	/* clear clk_eb except clk_lpc_eb */
	writel(BITS_NPU_SYS_CLK_EB_0, NPU_SYS_CLK_EB_0_ADDR);
	writel(BITS_NPU_SYS_CLK_EB_1, NPU_SYS_CLK_EB_1_ADDR);

	/* enable sys sleep enable and pmu sys sleep en */
	writel(BIT_NPU_SYS_SLP_EN_SET, NPU_SYS_LPC_SETTING_SET_ADDR);
	pmu_module_sleep_en(MODULE_NPU, SLP_EN_SET);

	/* check sys power state till off */
	while (!(ret = pmu_get_module_state(MODULE_NPU, &state)) && (state != PWR_STATE_OFF));

	NOTICE("npu sys sleep state is 0x%x\r\n", state);

	return ret;
}

int npu_sys_wakeup(void)
{
	int ret;
	unsigned int state;

	pmu_module_sleep_en(MODULE_NPU, SLP_EN_CLR);
	ret = pmu_module_wakeup(MODULE_NPU);

	writel(BIT_NPU_SYS_CFG_BUS_IDLE_EN_CLR, NPU_SYS_LPC_CFG_SETTING_CLR_ADDR);
	writel(BIT_NPU_SYS_DATA_BUS_IDLE_EN_CLR, NPU_SYS_LPC_DATA_SETTING_CLR_ADDR);
	writel(BIT_NPU_SYS_DATA_IDLE_EB_CLR, NPU_SYS_LPC_SETTING_CLR_ADDR);
	writel(BIT_NPU_SYS_SLP_EN_CLR, NPU_SYS_LPC_SETTING_CLR_ADDR);

	ret = pmu_get_module_state(MODULE_NPU, &state);

	NOTICE("npu sys wakeup state is 0x%x\r\n", state);

	writel(npu_sys_clk_mux_0_reserve, NPU_SYS_CLK_MUX_0_ADDR);
	writel(npu_sys_clk_eb_0_reserve, NPU_SYS_CLK_EB_0_ADDR);
	writel(npu_sys_clk_eb_1_reserve, NPU_SYS_CLK_EB_1_ADDR);

	return ret;
}