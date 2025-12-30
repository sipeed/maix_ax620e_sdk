/*
 * (C) Copyright 2020 AXERA Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __PWM_COMMON_H_
#define __PWM_COMMON_H_

#include <common.h>
#include <dm.h>
#include <pwm.h>
#include <asm/arch/ax620e.h>
#include <asm/io.h>

#define CHANNEL_CLK_SEL_FREQ		24000000	/* 24MHZ */
#define PWM_CLK_SELECT_REG_OFFSET	0x0
#define PWM_CLK_SELECT_REG_BASE		0x2002000
#define PWM_CLK_SELECT_REG_DATA		(1 << 6)	/* PWM01-PWM08, PWM11-PWM18 use 24MHZ */
#define PWM_CLK_GLB_ENB_REG_OFFSET	0X4
#define PWM_CLK_GLB_ENB_REG_BASE	0x2002000
#define PWM_CLK_GLB_ENB_REG_DATA	(1 << 2)	/* PWM01-PWM08,PWM11-PWM18 clock enable */
#define PWM_CLK_ENB_REG_OFFSET		0x8
#define PWM_CLK_ENB_REG_BASE		0x2002000
#define PWM01_CLK_ENB_REG_DATA		(1 << 22)	/* Enable pwm01 channel */
#define PWM02_CLK_ENB_REG_DATA		(1 << 23)	/* Enable pwm02 channel */
#define PWM05_CLK_ENB_REG_DATA		(1 << 26)	/* Enable pwm05 channel */
#define PWM_CLK_GATE_REG_OFFSET		0x14
#define PWM_CLK_GATE_REG_BASE		0x2002000
#define PWM_CLK_GATE_REG_DATA		(1 << 24)	/* PWM01-PWM08 clk gate enable */
#define PWM_CLK_RESET_REG_OFFSET	0x24
#define PWM_CLK_RESET_REG_BASE		0x2002000
#define PWM01_CLK_RESET_REG_DATA	(1 << 22)	/* PWM01 clk software reset */
#define PWM02_CLK_RESET_REG_DATA	(1 << 23)	/* PWM02 clk software reset */
#define PWM05_CLK_RESET_REG_DATA	(1 << 26)	/* PWM05 clk software reset */
#define PWM_CLK_RESET_REG_GLB_DATA	(1 << 30)	/* PWM01-PWM08 software reset */

#define PWM_SET_REG_BASE		0x2010000
#define PWM_TIMERN_LOADCOUNT_OFF(N)	(0x0 + (N) * 0x14)
#define PWM_TIMERN_CONTROLREG_OFF(N)	(0x8 + (N) * 0x14)
#define PWM_TIMERN_LOADCOUNT2_OFF(N)	(0xB0 + (N) * 0x4)
#define PWM_TIMERN_MODE			0x1E		/* PWM mode but not enable */
#define PWM_TIMERN_EN			0x1		/* PWM enable bit */
#define PWM_CHANNEL_1			0
#define PWM_CHANNEL_2			1
#define PWM_CHANNEL_3			2
#define PWM_CHANNEL_4			3
#define PWM_CHANNEL_5			4
#define PWM_CHANNEL_6			5
#define PWM_CHANNEL_7			6
#define PWM_CHANNEL_8			7

enum volt_type {
	VDDCORE = 0,
	VDDCPU,
	NNVDD,
	VOLT_TYPE_NUM,
};

/*
	sub = real sub * 10000
	scale = real scale * 100
	Vout = real Vout * 100
	div = real div * 100
	duty = (sub - sacle * Vout) / div = (real_duty * 100)
*/
struct volt_duty_transfer
{
	u32 sub;
	u32 scale;
	u32 div;
};

struct device_info
{
	u64 pwm_base;	/* pwm register base address */
	u32 channel_id;	/* pwm channel id, start with 0 */
};

/*
	freq unit: KHZ, vout unit: V * 100
	for example, if frequence is 240KHZ,volt is 0.8V,
	so you should input 240, 80.
*/
void pwm_config(enum platform_type plat, enum volt_type type,
		u32 freq, u32 vout);

void pwm_clk_config(void);

#endif
