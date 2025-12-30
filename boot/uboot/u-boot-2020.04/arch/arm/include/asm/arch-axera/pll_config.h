#ifndef _PLL_CONFIG_H_
#define _PLL_CONFIG_H_
typedef enum {
	AX_CPLL0 = 0,
	AX_CPLL1,
	AX_DPLL0,
	AX_DPLL1,
	AX_VPLL0,
	AX_VPLL1,
	AX_APLL0,
	AX_APLL1,
	AX_NPLL,
	AX_MPLL,
	AX_EPLL,
	AX_GPLL,
	AX_DSPLL,
	AX_NFPLL,
} AX_PLL_ID_E;
void pll_set(AX_PLL_ID_E id, u32 clk);
#endif
