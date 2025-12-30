#include <ax620e_common_sys_glb.h>
#include <sys_all_have.h>
#include <deb_gpio.h>
#include <pll.h>
#include <ddr_sys.h>

int chip_top_set(void)
{
	int ret = 0;

	writel(0, COMMON_SYS_PAD_SLEEP_BYP_ADDR);
	writel(0, COMMON_SYS_PLL_SLEEP_BYP_ADDR);
	writel(1, COMMON_SYS_XTAL_SLEEP_BYP_ADDR);

	/* cpll force on */
	writel(BIT_PLL_GRP_PLL_FRC_EN_SW_SET, PLL_GRP_PLL_FRC_EN_SW_SET_ADDR);
	writel(BIT_PLL_GRP_PLL_FRC_EN_SET, PLL_GRP_PLL_FRC_EN_SET_ADDR);

	/* set cpll wait cycle to minimum */
	writel(BITS_PLL_CPLL_ON_CFG_CLR, PLL_CPLL_ON_CFG_CLR_ADDR);

	/* set cpupll wait ref clock to minimum */
	writel(BITS_PLL_CPUPLL_ON_CFG_CLR, PLL_CPUPLL_ON_CFG_CLR_ADDR);

	/* set epll wait ref clock to minimum */
	writel(BITS_PLL_EPLL_ON_CFG_CLR, PLL_EPLL_ON_CFG_CLR_ADDR);

	/* set hpll wait ref clock to minimum */
	writel(BITS_PLL_HPLL_ON_CFG_CLR, PLL_HPLL_ON_CFG_CLR_ADDR);

	/* set npll wait ref clock to minimum */
	writel(BITS_PLL_NPLL_ON_CFG_CLR, PLL_NPLL_ON_CFG_CLR_ADDR);

	/* set vpll0 wait ref clock to minimum */
	writel(BITS_PLL_VPLL0_ON_CFG_CLR, PLL_VPLL0_ON_CFG_CLR_ADDR);

	/* set vpll1 wait ref clock to minimum */
	writel(BITS_PLL_VPLL1_ON_CFG_CLR, PLL_VPLL1_ON_CFG_CLR_ADDR);

	/* set dpll wait ref clock to minimum */
	writel(BITS_DPLL_ON_CFG_CLR, DPLL_ON_CFG_CLR_ADDR);

	//writel(PINMUX_G6_CTRL_SET_SET, PINMUX_G6_MISC_SET_ADDR);

	//writel(BIT_COMMON_SYS_EIC_MASK_ENABLE_SET, COMMON_SYS_EIC_MASK_ENABLE_SET_ADDR);
	//writel(BIT_COMMON_SYS_EIC_RISCV_MASK_ENABLE_SET, COMMON_SYS_EIC_RISCV_MASK_ENABLE_SET_ADDR);

	/* mask all deb gpio int */
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, COMMON_SYS_DEB_GPIO_31_0_INT_CLR_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, COMMON_SYS_DEB_GPIO_31_0_INT_MASK_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, COMMON_SYS_DEB_GPIO_63_32_INT_CLR_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, COMMON_SYS_DEB_GPIO_63_32_INT_MASK_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, COMMON_SYS_DEB_GPIO_89_64_INT_CLR_SET_ADDR);
	writel(BIT_COMMON_SYS_DEB_GPIO_MASK_ALL, COMMON_SYS_DEB_GPIO_89_64_INT_MASK_SET_ADDR);

	return ret;
}