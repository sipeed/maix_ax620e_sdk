#include <common.h>
#include <asm/io.h>
#include <asm/arch/pll_config.h>
#define AON_SYS_PLL_BASE_ADDR 	(0x4E10000)
#define GRP0_PLL_RE_OPEN  	(0xDC)
#define GRP0_PLL_RE_OPEN_SET  	(0xE0)
#define GRP0_PLL_RE_OPEN_CLR  	(0xE4)
#define DPLL0_ON_CFG		(0xB8)
#define DPLL0_ON_CFG_SET	(0xBC)
#define DPLL0_ON_CFG_CLR	(0xC0)
#define DPLL0_CFG1		(0x78)
#define DPLL0_STS		(0x68)
#define GRP0_PLL_RDY_STS	(0x9c)
#define GRP1_PLL_RE_OPEN	(0x1B8)
#define GRP1_PLL_RE_OPEN_SET	(0x1BC)
#define GRP1_PLL_RE_OPEN_CLR	(0x1C0)
#define DPLL1_ON_CFG		(0x188)
#define DPLL1_ON_CFG_SET	(0x18C)
#define DPLL1_ON_CFG_CLR	(0x190)
#define DPLL1_CFG1		(0x12C)
#define DPLL1_STS		(0x184)
#define GRP1_PLL_RDY_STS	(0x184)
#define CPLL0_ON_CFG 		(0xA0)
#define CPLL1_ON_CFG 		(0xAC)
#define CPLL0_CFG1 		(0x10)
#define CPLL1_CFG1 		(0x44)
#define CPLL0_STS 		(0)
#define CPLL1_STS 		(0x34)
#define PLL_WAIT_TIMEOUT	(1000)
struct ax_pll_cfg_reg {
	AX_PLL_ID_E id;
	ulong re_open_reg;
	ulong on_reg;
	ulong cfg1_reg;
	ulong lock_sts_reg;
	ulong rdy_sts_reg;
	u8 re_open_bit;
	u8 on_bit;
	u8 lock_sts_bit;
	u8 rdy_sts_bit;
};
struct ax_pll_cfg_reg pll_cfg_array[] = {
	{AX_CPLL0, GRP0_PLL_RE_OPEN, CPLL0_ON_CFG, CPLL0_CFG1, CPLL0_STS,
	 GRP0_PLL_RDY_STS, 0, 0, 0, 0},
	{AX_CPLL1, GRP0_PLL_RE_OPEN, CPLL1_ON_CFG, CPLL1_CFG1, CPLL1_STS,
	 GRP0_PLL_RDY_STS, 1, 0, 0, 1},
	{AX_DPLL0, GRP0_PLL_RE_OPEN, DPLL0_ON_CFG, DPLL0_CFG1, DPLL0_STS,
	 GRP0_PLL_RDY_STS, 2, 0, 0, 2},
	{AX_DPLL1, GRP1_PLL_RE_OPEN, DPLL1_ON_CFG, DPLL1_CFG1, DPLL1_STS,
	 GRP1_PLL_RDY_STS, 0, 0, 0, 0},
};

void pll_set(AX_PLL_ID_E id, u32 clk)
{
	u32 val;
	int i;
	struct ax_pll_cfg_reg *pll_cfg = 0;
	for (i = 0; i < sizeof(pll_cfg_array) / sizeof(pll_cfg_array[0]); i++) {
		if (pll_cfg_array[i].id == id) {
			pll_cfg = &pll_cfg_array[i];
			break;
		}
	}
	if (i == sizeof(pll_cfg_array) / sizeof(pll_cfg_array[0])) {
		return;
	}
	/*re_open set to 0 */
	clrbits_le32(AON_SYS_PLL_BASE_ADDR + pll_cfg->re_open_reg,
		     1 << pll_cfg->re_open_bit);
	/*on set to 0 */
	clrbits_le32(AON_SYS_PLL_BASE_ADDR + pll_cfg->on_reg,
		     1 << pll_cfg->on_bit);
	/*
	 *POST_DIV set to 1
	 *LDO_STB_X2_EN set to 1
	 *FBK_INT set to int val
	 *don't support fraction config
	 */
	/* clr POST_DIV, PRE_DIV and FBK_INT */
	val = readl(AON_SYS_PLL_BASE_ADDR + pll_cfg->cfg1_reg);
	val &= ~(BIT(23) | BIT(24) | BIT(17) | BIT(18) | GENMASK(8, 0));
	writel(val, AON_SYS_PLL_BASE_ADDR + pll_cfg->cfg1_reg);

	val = (1 << 23) | (1 << 14) | (clk / 12);
	writel(val, AON_SYS_PLL_BASE_ADDR + pll_cfg->cfg1_reg);
	/*dpll0_on_set_set set to 1 */
	setbits_le32(AON_SYS_PLL_BASE_ADDR + pll_cfg->on_reg,
		     1 << pll_cfg->on_bit);
	/*wait LOCKED */
	while (1) {
		if (readl(AON_SYS_PLL_BASE_ADDR + pll_cfg->lock_sts_reg) &
		    (1 << pll_cfg->lock_sts_bit)) {
			break;
		}
	}
	/*wait rdy */
	while (1) {
		if (readl(AON_SYS_PLL_BASE_ADDR + pll_cfg->rdy_sts_reg) &
		    (1 << pll_cfg->rdy_sts_bit)) {
			break;
		}
	}
	/*re_open set to 1 */
	setbits_le32(AON_SYS_PLL_BASE_ADDR + pll_cfg->re_open_reg,
		     1 << pll_cfg->re_open_bit);
}
