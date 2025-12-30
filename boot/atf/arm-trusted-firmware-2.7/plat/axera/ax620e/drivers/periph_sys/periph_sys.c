#include <periph_sys.h>

unsigned int peri_sys_clk_mux_0_reserve = 0;
unsigned int peri_sys_clk_eb_0_reserve = 0;
unsigned int peri_sys_clk_eb_1_reserve = 0;
unsigned int peri_sys_clk_eb_2_reserve = 0;
unsigned int peri_sys_clk_eb_3_reserve = 0;
unsigned int peri_sys_clk_div_0_reserve = 0;

int peri_sys_sleep(void)
{
	int ret;
	unsigned int state;

	/* in order to enable wdt2 during deep sleep, we set periph sys clk frc sw here. */
	writel(BIT_PMU_GLB_PERIPH_SYS_CLK_FRC_EN, PMU_GLB_CLK_FRC_SW_SET_ADDR);

	/* in order to enable wdt2 during deep sleep, we set periph sys clk frc en here. */
	writel(BIT_PMU_GLB_PERIPH_SYS_CLK_FRC_EN, PMU_GLB_CLK_FRC_EN_SET_ADDR);

	peri_sys_clk_mux_0_reserve = readl(PERIPH_SYS_CLK_MUX_0_ADDR);
	peri_sys_clk_eb_0_reserve = readl(PERIPH_SYS_CLK_EB_0_ADDR);
	peri_sys_clk_eb_1_reserve = readl(PERIPH_SYS_CLK_EB_1_ADDR);
	peri_sys_clk_eb_2_reserve = readl(PERIPH_SYS_CLK_EB_2_ADDR);
	peri_sys_clk_eb_3_reserve = readl(PERIPH_SYS_CLK_EB_3_ADDR);
	peri_sys_clk_div_0_reserve = readl(PERIPH_SYS_CLK_DIV_0_ADDR);

	/* in order to enable wdt2 during deep sleep, set lpc bypass here */
	writel(BIT_PERIPH_SYS_LPC_BP_CLK_EB_SET, PERIPH_SYS_LPC_SET_ADDR);

	writel(BIT_PERIPH_SYS_LPC_DATA_BUS_FRC_WORK_CLR ,PERIPH_SYS_LPC_DATA_CLR_ADDR);

	/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
	writel(BIT_PERIPH_SYS_LPC_CFG_BUS_IDLE_EN_SET, PERIPH_SYS_LPC_CFG_SET_ADDR);
	writel(BIT_PERIPH_SYS_LPC_DATA_BUS_IDLE_EN_SET, PERIPH_SYS_LPC_DATA_SET_ADDR);
	writel(BIT_PERIPH_SYS_LPC_DATA_IDLE_EN_SET, PERIPH_SYS_LPC_SET_ADDR);
	writel(BIT_PERIPH_SYS_LPC_SW_RST_CLR, PERIPH_SYS_SW_RST_1_CLR_ADDR);

	/* dont't clr clk_periph_24m and clk_wdt2_eb */
	writel(BITS_PERIPH_SYS_CLK_EB_0, PERIPH_SYS_CLK_EB_0_CLR_ADDR);
	writel(BITS_PERIPH_SYS_CLK_EB_1, PERIPH_SYS_CLK_EB_1_ADDR);
	writel(BITS_PERIPH_SYS_CLK_EB_2, PERIPH_SYS_CLK_EB_2_ADDR);
	writel(BITS_PERIPH_SYS_CLK_EB_3, PERIPH_SYS_CLK_EB_3_ADDR);

	writel(BIT_PERIPH_SYS_LPC_SLP_EN_SET, PERIPH_SYS_LPC_SET_ADDR);

	pmu_module_sleep_en(MODULE_PERIPH, SLP_EN_SET);

	/* check sys power state till off */
	while (!(ret = pmu_get_module_state(MODULE_PERIPH, &state)) && (state != PWR_STATE_OFF));

	return 0;
}

int  peri_sys_wakeup(void)
{
	int ret;
	unsigned int state;
	pmu_module_sleep_en(MODULE_PERIPH, SLP_EN_CLR);
	ret = pmu_module_wakeup(MODULE_PERIPH);

	writel(BIT_PERIPH_SYS_LPC_SLP_EN_CLR, PERIPH_SYS_LPC_CLR_ADDR);
	writel(BIT_PERIPH_SYS_LPC_CFG_BUS_IDLE_EN_CLR, PERIPH_SYS_LPC_CFG_CLR_ADDR);
	writel(BIT_PERIPH_SYS_LPC_DATA_BUS_IDLE_EN_CLR, PERIPH_SYS_LPC_DATA_CLR_ADDR);
	writel(BIT_PERIPH_SYS_LPC_DATA_IDLE_EN_CLR, PERIPH_SYS_LPC_CLR_ADDR);
	writel(BIT_PERIPH_SYS_LPC_DATA_BUS_FRC_WORK_SET ,PERIPH_SYS_LPC_DATA_SET_ADDR);

	writel(peri_sys_clk_mux_0_reserve, PERIPH_SYS_CLK_MUX_0_ADDR);
	writel(peri_sys_clk_eb_0_reserve, PERIPH_SYS_CLK_EB_0_ADDR);
	writel(peri_sys_clk_eb_1_reserve, PERIPH_SYS_CLK_EB_1_ADDR);
	writel(peri_sys_clk_eb_2_reserve, PERIPH_SYS_CLK_EB_2_ADDR);
	writel(peri_sys_clk_eb_3_reserve, PERIPH_SYS_CLK_EB_3_ADDR);
	writel(peri_sys_clk_div_0_reserve, PERIPH_SYS_CLK_DIV_0_ADDR);

	ret = pmu_get_module_state(MODULE_PERIPH, &state);

	return ret;
}