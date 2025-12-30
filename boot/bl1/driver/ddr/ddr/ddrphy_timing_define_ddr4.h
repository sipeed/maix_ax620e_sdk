#ifndef __DDRPHY_TIMING_DEFINE_DDR4__
#define __DDRPHY_TIMING_DEFINE_DDR4__
#include "ddr_init_config.h"

const unsigned int ddr4phy_timing[][31] = //[6][31]
{
#if DDR4_CONFIG_1600
    {// 0 800
        0x00000001,
        0x00010408,
        0x0000003C,
        0x0000003C,
        0x000000FE,
        0x000000FF,
        0x90400008,
        0x02100020,
        0x000A00FE,
        0x04104066,//dqs preamble
        0x00585858,
        0x0000000B,
        0x00000020,
        0x00010000,
        0x00272213,
        0x00008000,
        0x00008000,
        0x00272213,
        0x02008080,
        0x00008000,
        0x00008100,
        0x00008100,
        0x00008000,
        0x00000018,
        0x00272213,
        0x02008080,
        0x00008000,
        0x00008100,
        0x00008100,
        0x00008000,
        0x00000018,
    },
#endif
#if DDR4_CONFIG_1866
    {// 1 933
        0x00000001,
        0x00010408,
        0x0000001E,
        0x0000001E,
        0x0000003E,
        0x0000003F,
        0x70400004,
        0x02100008,
        0x00000000,
        0x00018033,
        0x00000000,
        0x0000000B,
        0x00000020,
        0x00010002,
        0x00272213,
        0x00008000,
        0x00008000,
        0x00272213,
        0x00008080,
        0x00008000,
        0x00008100,
        0x00008100,
        0x00008000,
        0x00000006,
        0x00272213,
        0x00008080,
        0x00008000,
        0x00008100,
        0x00008100,
        0x00008000,
        0x00000006,
    },
#endif
#if DDR4_CONFIG_2133
    {// 2 1066
        0x00000001,
        0x00010408,
        0x0000003C,
        0x0000003C,
        0x000003FE,
        0x000003FE,
        0x70400040,
        0x02100100,
        0x000003FE,
        0x00018033,
        0x00000000,
        0x0000000B,
        0x00000020,
        0x00010002,
        0x00272213,
        0x00008000,
        0x00008000,
        0x00272213,
        0x02008080,
        0x00008000,
        0x00008100,
        0x00008100,
        0x00008000,
        0x0000000C,
        0x00272213,
        0x02008080,
        0x00008000,
        0x00008100,
        0x00008100,
        0x00008000,
        0x0000000C,
    },
#endif
#if DDR4_CONFIG_2400
    {// 3 1200
        0x00000001,
        0x00010408,
        0x0000003C,
        0x0000003C,
        0x000003FE,
        0x000003FE,
        0x70400040,//DDRPHY_GEN_TMG6_F0[16:0] Adjust value of DQS gate timing
        0x02100100,
        0x000003FE,
        0x00018033,
        0x00000000,
        0x0000000B,
        0x00000020,
        0x00010002,
        0x00272213,
        0x00008000,
        0x00008000,
        0x00272213,
        0x00008080,
        0x00008000,
        0x00008080,
        0x00008080,
        0x00008000,
        0x00000006,
        0x00272213,
        0x00008080,
        0x00008000,
        0x00008080,
        0x00008080,
        0x00008000,
        0x00000006,
    },
#endif
#if DDR4_CONFIG_2666
    {// 4 1333
        0x00000001, ////DLL-0
        0x00010408, ////DLL-1
        0x0000003C, ////DATA OE
        0x0000003C, ////DQS  OE
        0x000003FE, ////DATA IE
        0x000003FE, ////DQS  IE
        0x90400040, ////DQS  GATE
        0x02100100, ////RD EN
        0x000A03FE, ////RD ODT
        0x04106066, ////IO -0
        0x00585858,////IO -1
        0x0000000B,////PLL-0
        0x00000020,////PLL-1
        0x00010000,////MISC
        0x00272213,////DLL-2 AC
        0x00008000,////AC-ADDR DL
        0x00008000,////AC-CMD DL
        0x00372213,////DLL-3 DS0
        0x00008080,////DS0-CLKWR DL
        0x00008000,////DS0-CLKWR DIFF DL
        0x00008080,////DS0-DQSPOS DL
        0x00008080,////DS0-DQSNEG DL
        0x00008000,////DS0-WRLVL DL
        0x00000018,////DQS  OUT
        0x00372213,////DLL-4 DS1
        0x00008080,////DS1-CLKWR DL
        0x00008000,////DS1-CLKWR DIFF DL
        0x00008080,////DS1-DQSPOS DL
        0x00008080,////DS1-DQSNEG DL
        0x00008000,////DS1-WRLVL DL
        0x00000018,////DQS  OUT
    },
#endif
#if DDR4_CONFIG_3200
    {// 5 1600
        0x00000001,//D_DDRPHY_GEN_TMG0_F0_ADDR
        0x00010408,//D_DDRPHY_GEN_TMG1_F0_ADDR
        0x0000003C,//D_DDRPHY_GEN_TMG2_F0_ADDR
        0x0000007C,//D_DDRPHY_GEN_TMG3_F0_ADDR
        0x000003FE,//D_DDRPHY_GEN_TMG4_F0_ADDR
        0x000003FE,//D_DDRPHY_GEN_TMG5_F0_ADDR
        0x10400040,//D_DDRPHY_GEN_TMG6_F0_ADDR
        0x02100100,//D_DDRPHY_GEN_TMG7_F0_ADDR
        0x000A03FE,//D_DDRPHY_GEN_TMG8_F0_ADDR
        0x04106066,//D_DDRPHY_GEN_TMG9_F0_ADDR
        0x00585858,//D_DDRPHY_GEN_TMG10_F0_ADDR
        0x0000000B,//D_DDRPHY_GEN_TMG11_F0_ADDR
        0x00000020,//D_DDRPHY_GEN_TMG12_F0_ADDR
        0x00010000,//D_DDRPHY_GEN_TMG13_F0_ADDR
        0x00372213,//D_DDRPHY_AC_TMG0_F0_ADDR
        0x00008000,//D_DDRPHY_AC_TMG1_F0_ADDR
        0x00008000,//D_DDRPHY_AC_TMG2_F0_ADDR
        0x00372213,//D_DDRPHY_DS0_TMG0_F0_ADDR
        0x00008080,//D_DDRPHY_DS0_TMG1_F0_ADDR
        0x00008000,//D_DDRPHY_DS0_TMG2_F0_ADDR
        0x00008080,//D_DDRPHY_DS0_TMG3_F0_ADDR
        0x00008080,//D_DDRPHY_DS0_TMG4_F0_ADDR
        0x00000036,//D_DDRPHY_DS0_TMG5_F0_ADDR
        0x0000007f,//D_DDRPHY_DS0_TMG6_F0_ADDR
        0x00372213,//D_DDRPHY_DS1_TMG0_F0_ADDR
        0x00008080,//D_DDRPHY_DS1_TMG1_F0_ADDR
        0x00008000,//D_DDRPHY_DS1_TMG2_F0_ADDR
        0x00008080,//D_DDRPHY_DS1_TMG3_F0_ADDR
        0x00008080,//D_DDRPHY_DS1_TMG4_F0_ADDR
        0x00000036,//D_DDRPHY_DS1_TMG5_F0_ADDR
        0x0000007f,//D_DDRPHY_DS1_TMG6_F0_ADDR
    },
#endif
};
#endif
