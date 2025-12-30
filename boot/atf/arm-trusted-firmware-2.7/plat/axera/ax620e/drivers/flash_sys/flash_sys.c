#include <flash_sys.h>
#include "ax620e_common_sys_glb.h"
#include "pmu.h"
#include "arch_helpers.h"

unsigned int flash_sys_clk_eb_0_reserve = 0;
unsigned int flash_sys_clk_eb_1_reserve = 0;
unsigned int flash_sys_clk_mux_0_reserve = 0;
unsigned int flash_sys_clk_div_0_reserve = 0;
unsigned int flash_sys_clk_div_1_reserve = 0;

int flash_sys_sleep(void)
{
	int ret = -1;
	unsigned int state;

	flash_sys_clk_mux_0_reserve = mmio_read_32(FLASH_SYS_CLK_MUX_0_ADDR);
	flash_sys_clk_eb_0_reserve = mmio_read_32(FLASH_SYS_CLK_EB_0_ADDR);
	flash_sys_clk_eb_1_reserve = mmio_read_32(FLASH_SYS_CLK_EB_1_ADDR);
	flash_sys_clk_div_0_reserve = mmio_read_32(FLASH_SYS_CLK_DIV_0_ADDR);
	flash_sys_clk_div_1_reserve = mmio_read_32(FLASH_SYS_CLK_DIV_1_ADDR);

	mmio_write_32(COMMON_SYS_USB_INT_CTRL_SET_ADDR, BIT_COMMON_SYS_USB_WAKE_UP_INT_MASK_SET);
	mmio_write_32(COMMON_SYS_USB_INT_CTRL_SET_ADDR, BIT_COMMON_SYS_USB_WAKE_UP_INT_CLR_SET);

	mmio_write_32(FLASH_SYS_LPC_CFG_SET_ADDR, BIT_FLASH_LPC_CFG_BUS_IDLE_EN_SET);
	mmio_write_32(FLASH_SYS_LPC_DATA_SET_ADDR, BIT_FLASH_LPC_DATA_BUS_IDLE_EN_SET);
	mmio_write_32(FLASH_SYS_LPC_SET_ADDR, BIT_FLASH_LPC_DATA_IDLE_EN_SET);
	mmio_write_32(FLASH_SYS_LPC_SET_ADDR, BIT_FLASH_LPC_BP_CLK_EB_SET);

	while(mmio_read_32(FLASH_SYS_FAB_BUSY_ADDR) != 0x0);

	mmio_write_32(FLASH_SYS_CLK_EB_0_ADDR, BITS_FLASH_SYS_CLK_EB_0);
	mmio_write_32(FLASH_SYS_CLK_EB_1_ADDR, BITS_FLASH_SYS_CLK_EB_1);

	mmio_clrbits_32(FLASH_SYS_LPC_DATA_ADDR, BIT_FLASH_LPC_DATA_BUS_FRC_WORK);

	mmio_write_32(FLASH_SYS_LPC_SET_ADDR, BIT_FLASH_LPC_SLP_EN_SET);

	pmu_module_sleep_en(MODULE_FLASH, SLP_EN_SET);

	NOTICE("flash sys sleep config finished\r\n");

	while (!(ret = pmu_get_module_state(MODULE_FLASH, &state)) && (state != PWR_STATE_OFF));

	NOTICE("flash sys sleep state is 0x%x\r\n", state);

	return 0;
}

int flash_sys_wakeup(void)
{
	int ret;
	unsigned int state;

	pmu_module_sleep_en(MODULE_FLASH, SLP_EN_CLR);
	ret = pmu_module_wakeup(MODULE_FLASH);

	/* special module not power down so sleep en should clear */
	writel(BIT_FLASH_LPC_SLP_EN_CLR, FLASH_SYS_LPC_CLR_ADDR);
	/* special module not power down so idle en should clear */
	writel(BIT_FLASH_LPC_CFG_BUS_IDLE_EN_CLR, FLASH_SYS_LPC_CFG_CLR_ADDR);
	writel(BIT_FLASH_LPC_DATA_BUS_IDLE_EN_CLR, FLASH_SYS_LPC_DATA_CLR_ADDR);
	writel(BIT_FLASH_LPC_DATA_IDLE_EN_CLR, FLASH_SYS_LPC_CLR_ADDR);

	mmio_setbits_32(FLASH_SYS_LPC_DATA_ADDR, BIT_FLASH_LPC_DATA_BUS_FRC_WORK);

	ret = pmu_get_module_state(MODULE_FLASH, &state);

	NOTICE("flash sys wakeup state is 0x%x\r\n", state);

	writel(flash_sys_clk_mux_0_reserve, FLASH_SYS_CLK_MUX_0_ADDR);
	writel(flash_sys_clk_eb_0_reserve, FLASH_SYS_CLK_EB_0_ADDR);
	writel(flash_sys_clk_eb_1_reserve, FLASH_SYS_CLK_EB_1_ADDR);
	writel(flash_sys_clk_div_0_reserve, FLASH_SYS_CLK_DIV_0_ADDR);
	writel(flash_sys_clk_div_1_reserve, FLASH_SYS_CLK_DIV_1_ADDR);

	return ret;
}
