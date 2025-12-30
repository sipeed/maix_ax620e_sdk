#include "pwm.h"
#include "printf.h"
#include "timer.h"

static struct device_info ax620e_device_info[] = {
	{PWM0_SET_REG_BASE, PWM_CHANNEL_0, PINMUX_PWM00_OFFSET,
	PWM00_CLK_RET_VAL,
	PWM00_CLK_EB_VAL},
	{PWM0_SET_REG_BASE, PWM_CHANNEL_1, PINMUX_PWM01_OFFSET,
	PWM01_CLK_RET_VAL,
	PWM01_CLK_EB_VAL},
	{PWM0_SET_REG_BASE, PWM_CHANNEL_2, PINMUX_PWM02_OFFSET,
	PWM02_CLK_RET_VAL,
	PWM02_CLK_EB_VAL},
	{PWM0_SET_REG_BASE, PWM_CHANNEL_3, PINMUX_PWM03_OFFSET,
	PWM03_CLK_RET_VAL,
	PWM03_CLK_EB_VAL},
	{PWM1_SET_REG_BASE, PWM_CHANNEL_0, PINMUX_PWM04_OFFSET,
	PWM04_CLK_RET_VAL,
	PWM04_CLK_EB_VAL},
	{PWM1_SET_REG_BASE, PWM_CHANNEL_1,  PINMUX_PWM05_OFFSET,
	PWM05_CLK_RET_VAL,
	PWM05_CLK_EB_VAL},
	{PWM1_SET_REG_BASE, PWM_CHANNEL_2, PINMUX_PWM06_OFFSET,
	PWM06_CLK_RET_VAL,
	PWM06_CLK_EB_VAL},
	{PWM1_SET_REG_BASE, PWM_CHANNEL_3, PINMUX_PWM07_OFFSET,
	PWM07_CLK_RET_VAL,
	PWM07_CLK_EB_VAL},
	{PWM2_SET_REG_BASE, PWM_CHANNEL_0, PINMUX_PWM08_OFFSET,
	PWM08_CLK_RET_VAL,
	PWM08_CLK_EB_VAL},
	{PWM2_SET_REG_BASE, PWM_CHANNEL_1, PINMUX_PWM09_OFFSET,
	PWM09_CLK_RET_VAL,
	PWM09_CLK_EB_VAL},
	{PWM2_SET_REG_BASE, PWM_CHANNEL_2, PINMUX_PWM10_OFFSET,
	PWM10_CLK_RET_VAL,
	PWM10_CLK_EB_VAL},
	{PWM2_SET_REG_BASE, PWM_CHANNEL_3, PINMUX_PWM11_OFFSET,
	PWM11_CLK_RET_VAL,
	PWM11_CLK_EB_VAL},
	{},
};

static struct volt_duty_transfer ax620e_evb_transfer[] = {
	{540000, 600, 1200}, /* 5.4, 6, 1.2 */
	{}
};


static void pwm_reg_set_val(u64 base, u64 reg_offset, u64 value, u64 val_offset, bool set)
{
	u64 reg_val = readl(base + reg_offset);

	if (set) {
		reg_val |= (value << val_offset);
	} else {
		reg_val &= ~(value << val_offset);
	}

	writel(reg_val, base + reg_offset);
}

static void pwm_reg_set_bool(u64 base, u64 offset, u64 value, bool set)
{
	u64 reg_val = readl(base + offset);

	if (set) {
		reg_val |= (1 << value);
	} else {
		reg_val &= ~(1 << value);
	}

	writel(reg_val, base + offset);
}

static u32 volt_to_duty(struct volt_duty_transfer *transfer_array, u32 vout)
{
	u32 duty = 0;
	struct volt_duty_transfer *transfer;

	transfer = &transfer_array[0];

	duty = (u32)((transfer->sub
		- (transfer->scale * vout)) /
		(transfer->div));

	debug("duty is %u\n", duty);

	return duty;
}

void pwm_clk_config(int pwm_id)
{
	struct device_info *info;

	info = &ax620e_device_info[pwm_id];
	if (!info) {
		return;
	}

	//global reset
	if (0 <= pwm_id && pwm_id <= 3) {
		pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM_GLB_CLK_RET_OFFSET, PWM0_GLB_CLK_RET_VAL, false);
	} else if (4 <= pwm_id && pwm_id <= 7) {
		pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM_GLB_CLK_RET_OFFSET, PWM1_GLB_CLK_RET_VAL, false);
	} else {
		pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM_GLB_CLK_RET_OFFSET, PWM2_GLB_CLK_RET_VAL, false);
	}

	//select 24M
	pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM_GLB_CLK_SEL_OFFSET, PWM_GLB_CLK_SEL_VAL, true);
	//reset
	pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM_CLK_RET_OFFSET, info->clk_rst_val, false);
	//clk gate eb
	pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM_GLB_CLK_GATE_OFFSET, PWM_GLB_CLK_GATE_VAL, true);
	//global clk eb
	if (0 <= pwm_id && pwm_id <= 3) {
		pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM0_GLB_CLK_EB_OFFSET, PWM0_GLB_CLK_EB_VAL, true);
	} else if (4 <= pwm_id && pwm_id <= 7) {
		pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM1_GLB_CLK_EB_OFFSET, PWM1_GLB_CLK_EB_VAL, true);
	} else {
		pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM2_GLB_CLK_EB_OFFSET, PWM2_GLB_CLK_EB_VAL, true);
	}

	//clk eb
	pwm_reg_set_bool(PERIPH_SYS_GLB_BASE_ADDR, PWM_CLK_EB_OFFSET, info->clk_eb_val, true);
}

void pwm_pinmux_config(int pwm_id)
{
	struct device_info *info;

	info = &ax620e_device_info[pwm_id];
	if (!info) {
		return;
	}
	if (0 <= pwm_id && pwm_id <= 7) {
		pwm_reg_set_val(PINMUXG6_BASE_ADDR, info->pinmux_shift, PINMUX_PWM_MASK_VAL0,PINMUX_PWM_MASK_OFFSET0, false);
		pwm_reg_set_val(PINMUXG6_BASE_ADDR, info->pinmux_shift, PINMUX_PWM_VAL0, PINMUX_PWM_VAL_OFFSET0, true);
	} else if (8 <= pwm_id && pwm_id <=9) {
		pwm_reg_set_val(PINMUXG12_BASE_ADDR, info->pinmux_shift, PINMUX_PWM_MASK_VAL1,PINMUX_PWM_MASK_OFFSET1, false);
		pwm_reg_set_val(PINMUXG12_BASE_ADDR, info->pinmux_shift, PINMUX_PWM_VAL1, PINMUX_PWM_VAL_OFFSET1, true);
	} else if (10 <= pwm_id && pwm_id <= 11) {
		pwm_reg_set_val(PINMUXG2_BASE_ADDR, info->pinmux_shift, PINMUX_PWM_MASK_VAL2,PINMUX_PWM_MASK_OFFSET2, false);
		pwm_reg_set_val(PINMUXG2_BASE_ADDR, info->pinmux_shift, PINMUX_PWM_VAL2, PINMUX_PWM_VAL_OFFSET2, true);
	}

}

void pwm_config(u32 pwm_id, u32 freq, u32 vout)
{
	u32 duty,temp;
	struct device_info *info;
	u32 real_vout;
	u32 real_freq;
	u32 real_period;	/* unit:ns */
	u32 real_duty;		/* unit:ns */
	u32 period_count;
	u32 duty_count;

	if ((vout < 700 || vout > 900)) {
		debug("VDDCORE volt out of range.\n");
		return;
	}

	info = &ax620e_device_info[pwm_id];
	if (!info) {
		return;
	}

	real_vout = vout;

	duty = volt_to_duty(ax620e_evb_transfer, real_vout);
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

void pwm_volt_config(int pwm_id, int voltage)
{
	pwm_pinmux_config(pwm_id);
	pwm_clk_config(pwm_id);
	pwm_config(pwm_id, 240, voltage);
	udelay(500);
}

