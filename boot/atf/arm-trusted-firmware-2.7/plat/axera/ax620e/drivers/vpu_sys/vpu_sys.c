#include <vpu_sys.h>

unsigned int vpu_sys_clk_mux_0_reserve = 0;
unsigned int vpu_sys_clk_eb_0_reserve = 0;
unsigned int vpu_sys_clk_eb_1_reserve = 0;


int vpu_sys_sleep(void)
{
	int ret;
	unsigned int state;

	vpu_sys_clk_mux_0_reserve = readl(VPU_SYS_CLK_MUX_0_ADDR);
	vpu_sys_clk_eb_0_reserve = readl(VPU_SYS_CLK_EB_0_ADDR);
	vpu_sys_clk_eb_1_reserve = readl(VPU_SYS_CLK_EB_1_ADDR);

	/* lpc_cfg_bus_idle_en & lpc_data_bus_idle_en */
	writel(BIT_VPU_LPC_CFG_BUS_IDLE_EN_SET, VPU_SYS_VENC1_LPC1_SET_ADDR);
	writel(BIT_VPU_LPC_DATA_BUS_IDLE_EN_SET, VPU_SYS_VENC1_LPC2_SET_ADDR);
	writel(BIT_VPU_DATA_IDLE_EN_SET, VPU_SYS_VENC1_LPC0_SET_ADDR);
	writel(BIT_VPU_SYS_LPC_SW_RST_CLR, VPU_SYS_SW_RST_0_CLR_ADDR);

	/* clear clk_eb */
	writel(BITS_VPU_SYS_CLK_EB_1, VPU_SYS_CLK_EB_1_ADDR);
	writel(BITS_VPU_SYS_CLK_EB_0, VPU_SYS_CLK_EB_0_ADDR);

	writel(BIT_VPU_LPC_SLP_EN_SET, VPU_SYS_VENC1_LPC0_SET_ADDR);
	pmu_module_sleep_en(MODULE_VPU, SLP_EN_SET);

	/* check sys power state till off */
	while (!(ret = pmu_get_module_state(MODULE_VPU, &state)) && (state != PWR_STATE_OFF));

	NOTICE("vpu sys sleep state is 0x%x\r\n", state);

	return 0;
}

int vpu_sys_wakeup(void)
{
	int ret;
	unsigned int state;

	pmu_module_sleep_en(MODULE_VPU, SLP_EN_CLR);
	ret = pmu_module_wakeup(MODULE_VPU);

	writel(BIT_VPU_LPC_SLP_EN_CLR, VPU_SYS_VENC1_LPC0_CLR_ADDR);
	writel(BIT_VPU_LPC_CFG_BUS_IDLE_EN_CLR, VPU_SYS_VENC1_LPC1_CLR_ADDR);
	writel(BIT_VPU_DATA_IDLE_EN_CLR, VPU_SYS_VENC1_LPC0_CLR_ADDR);
	writel(BIT_VPU_LPC_DATA_BUS_IDLE_EN_CLR, VPU_SYS_VENC1_LPC2_CLR_ADDR);

	ret = pmu_get_module_state(MODULE_VPU, &state);

	NOTICE("vpu sys wakeup state is 0x%x\r\n", state);

	writel(vpu_sys_clk_mux_0_reserve, VPU_SYS_CLK_MUX_0_ADDR);
	writel(vpu_sys_clk_eb_0_reserve, VPU_SYS_CLK_EB_0_ADDR);
	writel(vpu_sys_clk_eb_1_reserve, VPU_SYS_CLK_EB_1_ADDR);

	return ret;
}