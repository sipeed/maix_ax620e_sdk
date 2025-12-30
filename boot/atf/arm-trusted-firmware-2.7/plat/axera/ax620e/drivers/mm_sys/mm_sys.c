#include <mm_sys.h>

unsigned int mm_sys_clk_mux_0_reserve = 0;
unsigned int mm_sys_clk_eb_0_reserve = 0;
unsigned int mm_sys_clk_eb_1_reserve = 0;
unsigned int mm_sys_clk_div_0_reserve = 0;

int mm_sys_sleep(void)
{
	unsigned int state;
	int ret;

	mm_sys_clk_mux_0_reserve = mmio_read_32(MM_SYS_CLK_MUX_0_ADDR);
	mm_sys_clk_eb_0_reserve = mmio_read_32(MM_SYS_CLK_EB_0_ADDR);
	mm_sys_clk_eb_1_reserve = mmio_read_32(MM_SYS_CLK_EB_1_ADDR);
	mm_sys_clk_div_0_reserve = mmio_read_32(MM_SYS_CLK_DIV_0_ADDR);

	/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
	writel(BIT_MM_LPC_CFG_BUS_IDLE_EN_SET, MM_SYS_LPC1_SET_ADDR);
	writel(BIT_MM_LPC_DATA_BUS_IDLE_EN_SET, MM_SYS_LPC2_SET_ADDR);
	writel(BIT_MM_DATA_IDLE_EN_SET, MM_SYS_LPC0_SET_ADDR);
	writel(BIT_MM_SYS_LPC_SW_RST_CLR, MM_SYS_SW_RST_0_CLR_ADDR);

	/* clear clk_eb */
	//writel(BITS_MM_CLK_EB_0_CLR, MM_SYS_CLK_EB_0_CLR_ADDR);
	//writel(BITS_MM_CLK_EB_1_CLR, MM_SYS_CLK_EB_1_CLR_ADDR);
	writel(BITS_MM_SYS_CLK_EB_1, MM_SYS_CLK_EB_1_ADDR);
	writel(BITS_MM_SYS_CLK_EB_0, MM_SYS_CLK_EB_0_ADDR);

	/* enable sys sleep enable and pmu sys sleep en */
	writel(BIT_MM_LPC_SLP_EN_SET, MM_SYS_LPC0_SET_ADDR);
	pmu_module_sleep_en(MODULE_MM, SLP_EN_SET);

	/* check sys power state till off */
	while (!(ret = pmu_get_module_state(MODULE_MM, &state)) && (state != PWR_STATE_OFF));

	NOTICE("mm sleep state is 0x%x\r\n", state);

	return ret;

}

int mm_sys_wakeup(void)
{
	int ret;
	unsigned int state;

	pmu_module_sleep_en(MODULE_MM, SLP_EN_CLR);

	ret = pmu_module_wakeup(MODULE_MM);

	writel(BIT_MM_LPC_CFG_BUS_IDLE_EN_CLR, MM_SYS_LPC1_CLR_ADDR);
	writel(BIT_MM_LPC_DATA_BUS_IDLE_EN_CLR, MM_SYS_LPC2_CLR_ADDR);
	writel(BIT_MM_DATA_IDLE_EN_CLR, MM_SYS_LPC0_CLR_ADDR);
	writel(BIT_MM_LPC_SLP_EN_CLR, MM_SYS_LPC0_CLR_ADDR);

	ret = pmu_get_module_state(MODULE_MM, &state);
	NOTICE("mm sys wakeup state is 0x%x\r\n", state);

	writel(mm_sys_clk_mux_0_reserve, MM_SYS_CLK_MUX_0_ADDR);
	writel(mm_sys_clk_eb_0_reserve, MM_SYS_CLK_EB_0_ADDR);
	writel(mm_sys_clk_eb_1_reserve, MM_SYS_CLK_EB_1_ADDR);
	writel(mm_sys_clk_div_0_reserve, MM_SYS_CLK_DIV_0_ADDR);

	return ret;
}