#ifndef __DPLL_CTRL_GLB__
#define __DPLL_CTRL_GLB__

//dpll_ctrl_glb
#define DDR_SYS_DPLL_BASE_ADDR                      0x265000
#define DPLL_CTRL_GLB_DPLL_STS_ADDR                 0x0
#define DPLL_CTRL_GLB_DPLL_CFG0_ADDR                0x4
#define DPLL_CTRL_GLB_DPLL_CFG1_ADDR                0x10
#define DPLL_CTRL_GLB_PLL_RDY_STS_ADDR              0x40
#define DPLL_CTRL_GLB_DPLL_ON_CFG_ADDR              0x44
#define DPLL_CTRL_GLB_PLL_RE_OPEN_ADDR              0x68

#define DDR_SYS_GLB_BASE_ADDR                       0x210000
#define DDR_SYS_GLB_CLK_MUX_0_ADDR                  0x0
#define DDR_SYS_GLB_CLK_DIV_0_ADDR                  0x18

#define DDR_SYS_GLB_CLK_EB_1_SET_ADDR               0xE0
#define EB_1_SET_CLK_DDRPHY_4X_IN_EB_SET_LSB        (20)
#define EB_1_SET_CLK_DDRPHY_1X_IN_EB_SET_LSB        (19)
#define CLK_EB_1_CLK_DDRMC_EB_SET_LSB               (17)
#define CLK_EB_1_CLK_DDRMC_D2_EB_SET_LSB            (16)

#define DDR_SYS_GLB_CLK_EB_1_CLR_ADDR               0xE4
#define EB_1_CLR_CLK_DDRPHY_4X_IN_EB_CLR_LSB        (20)
#define EB_1_CLR_CLK_DDRPHY_1X_IN_EB_CLR_LSB        (19)
#define EB_1_CLK_DDRMC_EB_CLR_LSB                   (17)
#define EB_1_CLK_DDRMC_D2_EB_CLR_LSB                (16)

#define DDR_SYS_GLB_SW_RST_0_SET_ADDR               0xF0
#define SW_RST_0_SET_DDRPHY_SW_RST_SET_LSB          (14)
#define SW_RST_0_SET_DDRMC_SW_RST_SET_LSB           (13)

#define DDR_SYS_GLB_SW_RST_0_CLR_ADDR               0xF4
#define SW_RST_0_CLR_DDRPHY_SW_RST_CLR_LSB          (14)
#define SW_RST_0_CLR_DDRMC_SW_RST_CLR_LSB           (13)

#endif