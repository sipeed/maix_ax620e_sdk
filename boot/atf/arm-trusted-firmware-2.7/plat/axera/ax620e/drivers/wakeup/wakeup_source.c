#include <wakeup_source.h>
#include <ax620e_common_sys_glb.h>
#include <ax620e_def.h>
#include <platform_def.h>

#define do_div(n, base) ({				\
	unsigned int __base = (base);			\
	unsigned int __rem;				\
	__rem = ((unsigned long long)(n)) % __base;	\
	(n) = ((unsigned long long)(n)) / __base;	\
	__rem;						\
})

void timer32_wakeup_config(unsigned int wait_count)
{
	unsigned long val = 0;
	unsigned long delta_stamp = 0;
	unsigned int delta = 0;
	unsigned long wakeup_start = 0;
	unsigned long sleep_end = 0;

	/* if user set it to 0, then timer32 will be closed */
	if(readl(AX620E_TIMER32_EB_ADDR) != 0x1) {
		return ;
	}

	/* !=0x0, means timer32 count value has been set in user mode */
	if ((val = readl(AX620E_TIMER32_WAIT_COUNT_ADDR)) != 0x0) {
		val *=1000;
		val *= TIMER32_COUNT_PER_SEC;	//wait_count = ms * 1000 * 32K / 1000000.
		do_div(val, 1000000);
		wait_count = val;
	}

flag1:
	writel(0, TIMER32_CMR_START);//stop compare

	val = readl(TIMER32_INTR_CTRL);
	val |= BIT_TIMER32_INTR_EOI;// set int eoi
	val |= BIT_TIMER32_INTR_MASK;// set int mask
	val &= ~BIT_TIMER32_INTR_EN; //disable int
	writel(val, TIMER32_INTR_CTRL);

	/* need clk */
	writel(1, COMMON_SYS_XTAL_SLEEP_BYP_ADDR);
	//writel(PINMUX_G6_CTRL_CLR_CLR, PINMUX_G6_MISC_CLR_ADDR);

	/* eic enable timer32 for wakeup */
	val = readl(COMMON_SYS_EIC_EN_SET_ADDR);
	val |= BIT_COMMON_TMR32_EIC_EN_SET;
	writel(val, COMMON_SYS_EIC_EN_SET_ADDR);

	/*prst and rst */
	val = readl(COMMON_SYS_SW_RST_0_ADDR);
	val |= (BIT_COMMON_SYS_TIMER32_SW_RST|BIT_COMMON_SYS_TIMER32_SW_PRST);
	writel(val, COMMON_SYS_SW_RST_0_ADDR);

	val = readl(COMMON_SYS_SW_RST_0_ADDR);
	val &= ~(BIT_COMMON_SYS_TIMER32_SW_RST|BIT_COMMON_SYS_TIMER32_SW_PRST);
	writel(val, COMMON_SYS_SW_RST_0_ADDR);

	/* select 32K */
	val = readl(COMMON_SYS_CLK_MUX1_ADDR);
	val &= ~BIT_COMMON_SYS_CLK_TIMER32_SEL_24M;
	writel(val, COMMON_SYS_CLK_MUX1_ADDR);

	/*clk channel enable */
	val = readl(COMMON_SYS_CLK_EB_0_ADDR);
	val |= BIT_COMMON_SYS_CLK_TIMER32_EB;
	writel(val, COMMON_SYS_CLK_EB_0_ADDR);

	/* global clk enable */
	val = readl(COMMON_SYS_CLK_EB_1_ADDR);
	val |= BIT_COMMON_SYS_PCLK_TMR32_EB;
	writel(val, COMMON_SYS_CLK_EB_1_ADDR);


	/* sleep end timer64 value */
	sleep_end = *((volatile unsigned long*)TIMER64_CNT_LOW_ADDR);

	wakeup_start = *((volatile unsigned long*)WAKEUP_START_TIMESTAMP_ADDR);

	delta_stamp = sleep_end - wakeup_start;
	if (delta_stamp > 0) {
		do_div(delta_stamp, 24);
		delta_stamp *= TIMER32_COUNT_PER_SEC;
		do_div(delta_stamp, 1000000);
		delta = delta_stamp;
		if (wait_count > delta)
			wait_count = wait_count - delta;
	}

	/* when the timer32 wait count < 10ms, we directly set it to 10ms in case of the timer32 interrupt block sleep. */
	if (wait_count < TIMER32_COUNT_PER_SEC / 100)
		wait_count = TIMER32_COUNT_PER_SEC / 100;

	val = readl(TIMER32_CNT_CCVR);
	val += wait_count;
	if (val >= 0xfffffff0) {
		goto flag1;
	} else {
		writel(val, TIMER32_CMR);
	}

	//compare start
	writel(BIT_TIMER32_CMR_START, TIMER32_CMR_START);

	//set int_en
	writel(BIT_TIMER32_INTR_EN, TIMER32_INTR_CTRL);
}

void timer32_init(void)
{
	unsigned long val = 0;
	/*prst and rst */

	val = readl(COMMON_SYS_SW_RST_0_ADDR);
	val |= (BIT_COMMON_SYS_TIMER32_SW_RST|BIT_COMMON_SYS_TIMER32_SW_PRST);
	writel(val, COMMON_SYS_SW_RST_0_ADDR);

	val = readl(COMMON_SYS_SW_RST_0_ADDR);
	val &= ~(BIT_COMMON_SYS_TIMER32_SW_RST|BIT_COMMON_SYS_TIMER32_SW_PRST);
	writel(val, COMMON_SYS_SW_RST_0_ADDR);

	/* select 24M */
	val = readl(COMMON_SYS_CLK_MUX1_ADDR);
	val |= BIT_COMMON_SYS_CLK_TIMER32_SEL_24M;
	writel(val, COMMON_SYS_CLK_MUX1_ADDR);

	/*clk channel enable */
	val = readl(COMMON_SYS_CLK_EB_0_ADDR);
	val |= BIT_COMMON_SYS_CLK_TIMER32_EB;
	writel(val, COMMON_SYS_CLK_EB_0_ADDR);

	/* global clk enable */
	val = readl(COMMON_SYS_CLK_EB_1_ADDR);
	val |= BIT_COMMON_SYS_PCLK_TMR32_EB;
	writel(val, COMMON_SYS_CLK_EB_1_ADDR);
}

void delayus(unsigned int us)
{
	unsigned int old, new;

	old = readl(TIMER32_CNT_CCVR);
	new = old + us * 24;
	writel(new, TIMER32_CMR);
	writel(0x1, TIMER32_CMR_START);

	while(old < new) {
		old = readl(TIMER32_CNT_CCVR);
	}
	writel(0, TIMER32_CMR_START);
	timer32_init();
}

void delayms(unsigned int ms)
{
	unsigned int i = 0;
	for(i = 0; i < ms; i++) {
			delayus(1000);
	}
}

void delays(unsigned int s)
{
	unsigned int i = 0;
	for(i = 0;i < s; i++) {
			delayms(1000);
	}
}

