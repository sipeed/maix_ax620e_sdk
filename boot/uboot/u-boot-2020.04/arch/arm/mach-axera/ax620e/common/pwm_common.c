/*
 * (C) Copyright 2020 AXERA Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include "pwm_common.h"

static struct volt_duty_transfer ax650_evb_transfer[] = {
	/* VDDCORE */
	[VDDCORE] = {
		54000, 600, 120		/* 5.4, 6, 1.2 */
	},
	/* VDDCPU */
	[VDDCPU] = {
		22000, 200, 120		/* 2.2, 2, 1.2 */
	},
	/* NPU */
	[NNVDD] = {
		30000, 400, 120		/* 3, 4, 1.2 */
	}
};

static struct volt_duty_transfer *ax650_transfer_choose[] = {
	[AX620E_HAPS] = ax650_evb_transfer,
	[AX620E_EMMC] = ax650_evb_transfer,
	[AX620E_NAND] = ax650_evb_transfer,
};

static struct device_info ax650_evb_device_info[] = {
	/* VDDCORE */
	[VDDCORE] = {
		PWM_SET_REG_BASE, PWM_CHANNEL_5,	/* PWM05 */
	},
	/* VDDCPU */
	[VDDCPU] = {
		PWM_SET_REG_BASE, PWM_CHANNEL_2,	/* PWM02 */
	},
	/* NPU */
	[NNVDD] = {
		PWM_SET_REG_BASE, PWM_CHANNEL_1,	/* PWM01 */
	}
};

static struct device_info *ax650_device_info_choose[] = {
	[AX620E_HAPS] = ax650_evb_device_info,
	[AX620E_EMMC] = ax650_evb_device_info,
	[AX620E_NAND] = ax650_evb_device_info,
};

static u32 volt_to_duty(struct volt_duty_transfer *transfer_array, u32 index, u32 vout)
{
	u32 duty = 0;
	struct volt_duty_transfer *transfer;

	transfer = &transfer_array[index];

	duty = (u32)((transfer->sub
		- (transfer->scale * vout)) /
		(transfer->div));

	debug("duty is %u\n", duty);

	return duty;
}

static void pwm_reg_set(u64 base, u64 offset, u64 value, bool set)
{
	u64 reg_val = readl(base + offset);

	if (set) {
		reg_val |= value;
	} else {
		reg_val &= ~value;
	}

	writel(reg_val, base + offset);
}

void pwm_clk_config(void)
{
	//global reset PWM01-PWM08
	pwm_reg_set(PWM_CLK_RESET_REG_BASE,
		PWM_CLK_RESET_REG_OFFSET, PWM_CLK_RESET_REG_GLB_DATA, false);
	//PWM01-PWM08, PWM11-PWM18 use 24MHz
	pwm_reg_set(PWM_CLK_SELECT_REG_BASE, PWM_CLK_SELECT_REG_OFFSET,
		PWM_CLK_SELECT_REG_DATA, true);
	//PWM01 clk reset
	pwm_reg_set(PWM_CLK_RESET_REG_BASE,
		PWM_CLK_RESET_REG_OFFSET, PWM01_CLK_RESET_REG_DATA, false);
	//PWM02 clk reset
	pwm_reg_set(PWM_CLK_RESET_REG_BASE,
		PWM_CLK_RESET_REG_OFFSET, PWM02_CLK_RESET_REG_DATA, false);
	//PWM05 clk reset
	pwm_reg_set(PWM_CLK_RESET_REG_BASE,
		PWM_CLK_RESET_REG_OFFSET, PWM05_CLK_RESET_REG_DATA, false);
	//PWM01-PWM08 clk gate enable
	pwm_reg_set(PWM_CLK_GATE_REG_BASE, PWM_CLK_GATE_REG_OFFSET,
		PWM_CLK_GATE_REG_DATA, true);
	//PWM01-PWM08, PWM11-PWM18 clk enable
	pwm_reg_set(PWM_CLK_GLB_ENB_REG_BASE, PWM_CLK_GLB_ENB_REG_OFFSET,
		PWM_CLK_GLB_ENB_REG_DATA, true);
	//PWM01 clk enable
	pwm_reg_set(PWM_CLK_ENB_REG_BASE, PWM_CLK_ENB_REG_OFFSET,
		PWM01_CLK_ENB_REG_DATA, true);
	//PWM02 clk enable
	pwm_reg_set(PWM_CLK_ENB_REG_BASE, PWM_CLK_ENB_REG_OFFSET,
		PWM02_CLK_ENB_REG_DATA, true);
	//PWM05 clk enable
	pwm_reg_set(PWM_CLK_ENB_REG_BASE, PWM_CLK_ENB_REG_OFFSET,
		PWM05_CLK_ENB_REG_DATA, true);
}

/*
	freq unit: KHZ, vout unit: V * 100
	for example, if frequence is 240KHZ,volt is 0.8V,
	so you should input 240, 80.
*/
void pwm_config(enum platform_type plat, enum volt_type type,
		u32 freq, u32 vout)
{
	u32 duty,temp;
	struct device_info *info;
	u32 real_vout;
	u32 real_freq;
	u32 real_period;	/* unit:ns */
	u32 real_duty;		/* unit:ns */
	u32 period_count;
	u32 duty_count;

	if ((u32)plat >= (u32)PLATFORM_TYPE_NUM) {
		debug("%s: platform type error\n", __func__);
		return;
	}

	if ((u32)type >= (u32)VOLT_TYPE_NUM) {
		debug("%s: type error\n", __func__);
		return;
	}

	if ((type == VDDCORE) && (vout < 70 || vout > 90)) {
		debug("VDDCORE volt out of range.\n");
		return;
	}

	if ((type == VDDCPU) && (vout < 50 || vout > 110)) {
		debug("VDDCPU volt out of range.\n");
		return;
	}

	if ((type == NNVDD) && (vout < 45 || vout > 75)) {
		debug("NNVDD volt out of range.\n");
		return;
	}

	info = &ax650_device_info_choose[(u32)plat][(u32)type];

	if (!info) {
		return;
	}

	real_vout = vout;

	duty = volt_to_duty(ax650_transfer_choose[(u32)plat], (u32)type, real_vout);
	if (duty < 0) {
		debug("%s: get duty failed\n", __func__);
		return;
	}

	real_freq = freq;

	if (real_freq <= 0) {
		debug("%s: pwm frequence can't set to zero\n", __func__);
		return;
	}

	real_period = 1000000 / real_freq;

	real_duty = real_period * duty / 100;

	debug("real_period is %u, real_duty is %u\n", real_period, real_duty);

	if ((real_period <= 0)
		|| (real_duty < 0)) {
		debug("%s: get real_period error\n", __func__);
		return;
	}
	temp = (u64)real_period * CHANNEL_CLK_SEL_FREQ / 1000000000;
	period_count = (u32)temp;
	temp = (u64)real_duty * CHANNEL_CLK_SEL_FREQ / 1000000000;
	duty_count = (u32)temp;

	debug("period_count = %u, duty_count=%u\n", period_count, duty_count);

	writel(duty_count,
		info->pwm_base +PWM_TIMERN_LOADCOUNT2_OFF(info->channel_id));
	writel(period_count - duty_count,
		info->pwm_base + PWM_TIMERN_LOADCOUNT_OFF(info->channel_id));
	writel((PWM_TIMERN_MODE | PWM_TIMERN_EN),
		info->pwm_base + PWM_TIMERN_CONTROLREG_OFF(info->channel_id));

	debug("read duty is:%u,read period is %u\n",
		readl(info->pwm_base +PWM_TIMERN_LOADCOUNT2_OFF(info->channel_id)),
		readl(info->pwm_base + PWM_TIMERN_LOADCOUNT_OFF(info->channel_id)));
}