#ifndef __TIMER_H_
#define __TIMER_H_

#include "sys_all_have.h"

#define TIMER_BASE      0x2240000
#define TIMER_CNT_CCVR	0x0
#define TIMER_CMR	0x4
#define TIMER_CMR_START	0x10
#define CLK_BASE        0x2340000
#define CLK_MUX1_OFF    0xc
#define CLK_EB_0_OFF    0x24
#define CLK_EB_1_OFF    0x30
#define SW_RST_0_OFF    0x54
#define TIME_FREQ       24      /* 10MHZ */
typedef unsigned long u64;
typedef unsigned int u32;

static inline void reg_cfg(u64 addr, u32 shift, int enabled)
{
	u32 value = readl(addr);
	if (enabled) {
		value |= shift;
	} else {
		value &= ~shift;
	}
	writel(value, addr);
}

static inline void clk_init(void)
{
	// clk global reset
	reg_cfg(CLK_BASE + SW_RST_0_OFF, (1 << 21), false);
	// freq selection
	reg_cfg(CLK_BASE + CLK_MUX1_OFF, (1 << 26), true);
	// channel reset
	reg_cfg(CLK_BASE + SW_RST_0_OFF, (1 << 22), false);
	// channel clock enable
	reg_cfg(CLK_BASE + CLK_EB_0_OFF, (1 << 14), true);
	//pclk enable
	reg_cfg(CLK_BASE + CLK_EB_1_OFF, (1 << 15), true);
}

/* timer total count : period us */
static inline void timer_init(void)
{
	clk_init();
}

static inline void delayus(u32 us)
{
	u32 old, new;

	old = readl(TIMER_BASE + TIMER_CNT_CCVR);
	new = old + us * TIME_FREQ;
	writel(new, TIMER_BASE + TIMER_CMR);
	writel(0x1, TIMER_BASE + TIMER_CMR_START);

	while(old < new) {
		old = readl(TIMER_BASE + TIMER_CNT_CCVR);
	}
	writel(0, TIMER_BASE + TIMER_CMR_START);
	reg_cfg(CLK_BASE + CLK_EB_1_OFF, (1 << 15), false);
	clk_init();
}

static inline void delayms(u32 ms)
{
	u32 i = 0;
	for(i = 0; i < ms; i++) {
		delayus(1000);
	}
}

static inline void delays(u32 s)
{
	u32 i = 0;
	for(i = 0;i < s; i++) {
		delayms(1000);
	}
}
#endif
