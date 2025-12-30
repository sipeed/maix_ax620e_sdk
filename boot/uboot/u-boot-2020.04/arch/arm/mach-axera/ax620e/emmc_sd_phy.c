#include <common.h>
#include <asm/arch/ax620e.h>
#include <asm/io.h>

/* PHY_CNFG */
#define BITS_PHY_CNFG_PAD_SN(x)       (((x) & 0xF) << 20)
#define BITS_PHY_CNFG_PAD_SP(x)       (((x) & 0xF) << 16)
#define BIT_PHY_CNFG_PHY_PWRGOOD       BIT(1)
#define BIT_PHY_CNFG_PHY_RSTN          BIT(0)

#define BITS_TXSLEW_CTRL_N(x)       (((x) & 0xF) << 9)
#define BITS_TXSLEW_CTRL_P(x)       (((x) & 0xF) << 5)
#define BITS_WEAKPULL_EN(x)         (((x) & 0x3) << 3)
#define BITS_RXSEL_EN(x)            (((x) & 0x7) << 0)

#define PHY_BASE           0x300
#define PHY_CNFG           (PHY_BASE + 0x0)     //32bits
#define PHY_CMDPAD_CNFG    (PHY_BASE + 0x4)     //16bits
#define PHY_DATPAD_CNFG    (PHY_BASE + 0x6)     //16bits
#define PHY_CLKPAD_CNFG    (PHY_BASE + 0x8)     //16bits
#define PHY_STBPAD_CNFG    (PHY_BASE + 0xA)     //16bits
#define PHY_RSTNPAD_CNFG   (PHY_BASE + 0xC)     //16bits
#define PHY_COMMDL_CNFG    (PHY_BASE + 0x1C)    //8bits
#define PHY_SDCLKDL_CNFG   (PHY_BASE + 0x1D)    //8bits
#define PHY_SDCLKDL_DC     (PHY_BASE + 0x1E)    //8bits
#define PHY_SMPLDL_CNFG    (PHY_BASE + 0x20)    //8bits
#define PHY_ATDL_CNFG      (PHY_BASE + 0x21)    //8bits

#define EMMC_PINMUX_ADDR                      0x4251E00

/* pinmux set phy_func_en, when fun is not zero(It is not emmc/sd/sdio function). */
int phy_func_en(u32 pinmux_addr)
{
	writel(BIT(16), (u64)(pinmux_addr + 0x4));
	if (EMMC_PINMUX_ADDR == pinmux_addr)
		writel(BIT(17), (u64)(pinmux_addr + 0x4));
	return 0;
}

/*
* set pad voltage, pad_val = 1 is 1.8V ,pad_val = 0 default 3.3V
*/
int phy_setting(u32 ip_addr, int pad_vol)
{
	u32 temp_u32;
	u16 temp_u16;
	unsigned long timeout;
	/* Wait max 500 ms */
	timeout = 5000;

	/* CNFG_PHY_PWRGOOD */
	while(!(temp_u32 = readl((u64)(ip_addr + PHY_CNFG)) & BIT(1))) {
		if (timeout == 0) {
			return -1;
		}
		timeout--;
		udelay(100);
	}
	/* set pad voltage 1.8V or 3.3V(default) */
	if(pad_vol) {
		int val = readl((u64)(ip_addr + 0x3C));
		val |= BIT(19);
		writel(val, (u64)ip_addr + 0x3C);

		/* PHY_CNFG to set driver strength 40ohm for a whole group, eg: sd (clk,cmd,data0-3) */
		temp_u32 = readl((u64)(ip_addr + PHY_CNFG));
		temp_u32 = temp_u32 |
		           BITS_PHY_CNFG_PAD_SN(0xC) |
		           BITS_PHY_CNFG_PAD_SP(0xC);
		writel(temp_u32, (u64)(ip_addr + PHY_CNFG));

		/* CMDPAD_CNFG to set cmd pad pu and pd */
		//pad setting
		temp_u16 = BITS_TXSLEW_CTRL_N(2) |
		           BITS_TXSLEW_CTRL_P(2) |
		           BITS_WEAKPULL_EN(0) | //disable上下拉
		           BITS_RXSEL_EN(1); //1.8V施密特触发器
		writew(temp_u16, (u64)(ip_addr + PHY_CMDPAD_CNFG));
		writew(temp_u16, (u64)(ip_addr + PHY_DATPAD_CNFG));
		writew(temp_u16, (u64)(ip_addr + PHY_RSTNPAD_CNFG));
		writew(temp_u16, (u64)(ip_addr + PHY_CLKPAD_CNFG));
		writew(temp_u16, (u64)(ip_addr + PHY_STBPAD_CNFG));
	} else {
		/* PHY_CNFG to set driver strength 40ohm for a whole group, eg: sd (clk,cmd,data0-3) */
		temp_u32 = readl((u64)(ip_addr + PHY_CNFG));
		temp_u32 = temp_u32 |
		           BITS_PHY_CNFG_PAD_SN(0xC) |
		           BITS_PHY_CNFG_PAD_SP(0xD);
		writel(temp_u32, (u64)(ip_addr + PHY_CNFG));

		/* CMDPAD_CNFG to set cmd pad pu and pd */
		//pad setting
		temp_u16 = BITS_TXSLEW_CTRL_N(2) |
		           BITS_TXSLEW_CTRL_P(2) |
		           BITS_WEAKPULL_EN(0) | //disable上下拉
		           BITS_RXSEL_EN(2); //3.3V施密特触发器
		writew(temp_u16, (u64)(ip_addr + PHY_CMDPAD_CNFG));
		writew(temp_u16, (u64)(ip_addr + PHY_DATPAD_CNFG));
		writew(temp_u16, (u64)(ip_addr + PHY_RSTNPAD_CNFG));
		writew(temp_u16, (u64)(ip_addr + PHY_CLKPAD_CNFG));
		writew(temp_u16, (u64)(ip_addr + PHY_STBPAD_CNFG));
	}
	//deassert reset
	temp_u32 |= BIT(0);
	writel(temp_u32, (u64)(ip_addr + PHY_CNFG));

	return 0;
}
