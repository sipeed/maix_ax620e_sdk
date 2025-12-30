#include <isp_sys.h>

unsigned int isp_sys_clk_mux_0_reserve = 0;
unsigned int isp_sys_clk_eb_0_reserve = 0;
unsigned int isp_sys_clk_eb_1_reserve = 0;

int isp_sys_sleep(void)
{
	unsigned int state = 0;
	int ret;

	isp_sys_clk_mux_0_reserve = readl(ISP_SYS_CLK_MUX_0_ADDR);
	isp_sys_clk_eb_0_reserve = readl(ISP_SYS_CLK_EB_0_ADDR);
	isp_sys_clk_eb_1_reserve = readl(ISP_SYS_CLK_EB_1_ADDR);

	/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
	writel(BIT_ISP_LPC_DATA_BUS_IDLE_EN_SET, ISP_SYS_LPC_DATA_SET_ADDR);
	writel(BIT_ISP_LPC_CFG_BUS_IDLE_EN_SET, ISP_SYS_LPC_CFG_SET_ADDR);
	writel(BIT_ISP_LPC_DATA_IDLE_EN_SET, ISP_SYS_LPC_DATA_SET_ADDR);
	writel(BIT_ISP_SYS_LPC_SW_RST_CLR, ISP_SYS_SW_RST_1_CLR_ADDR);

	/* clear clk_eb */
	writel(BITS_ISP_CLK_EB_0, ISP_SYS_CLK_EB_0_ADDR); /*close clk except 24M */
	writel(BITS_ISP_CLK_EB_1, ISP_SYS_CLK_EB_1_ADDR); /*close clk except lpc */

	/* enable sys sleep enable and pmu sys sleep en */
	writel(BIT_ISP_SLEEP_EN_SET, ISP_SYS_SLEEP_EN_SET_ADDR);
	pmu_module_sleep_en(MODULE_ISP, SLP_EN_SET);

	/* check sys power state till off */
	while (!(ret = pmu_get_module_state(MODULE_ISP, &state)) && (state != PWR_STATE_OFF));

	NOTICE("isp sleep state is 0x%x\r\n", state);

	return ret;
}

int isp_sys_wakeup(void)
{
	int ret;
	unsigned int state = 0;

	pmu_module_sleep_en(MODULE_ISP, SLP_EN_CLR);

	ret = pmu_module_wakeup(MODULE_ISP);

	writel(BIT_ISP_LPC_DATA_BUS_IDLE_EN_CLR, ISP_SYS_LPC_DATA_CLR_ADDR);
	writel(BIT_ISP_LPC_CFG_BUS_IDLE_EN_CLR, ISP_SYS_LPC_CFG_CLR_ADDR);
	writel(BIT_ISP_LPC_DATA_IDLE_EN_CLR, ISP_SYS_LPC_DATA_CLR_ADDR);
	writel(BIT_ISP_SLEEP_EN_CLR, ISP_SYS_SLEEP_EN_CLR_ADDR);

	ret = pmu_get_module_state(MODULE_ISP, &state);

	NOTICE("isp wakeup state is 0x%x\r\n", state);

	writel(isp_sys_clk_mux_0_reserve, ISP_SYS_CLK_MUX_0_ADDR);
	writel(isp_sys_clk_eb_0_reserve, ISP_SYS_CLK_EB_0_ADDR);
	writel(isp_sys_clk_eb_1_reserve, ISP_SYS_CLK_EB_1_ADDR);

	return ret;
}