/*
 * (C) Copyright 2020 AXERA
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <pwm.h>
#include <asm/io.h>

#define CHANNELS_PER_PWM                  4
#define PWM_TIMERN_LOADCOUNT_OFF(N)       (0x0 + (N) * 0x14)
#define PWM_TIMERN_CONTROLREG_OFF(N)      (0x8 + (N) * 0x14)
#define PWM_TIMERN_LOADCOUNT2_OFF(N)      (0xB0 + (N) * 0x4)
#define PWM_TIMERN_MODE                   0x1E /* PWM mode but not enable */
#define PWM_TIMERN_EN                     0x1  /* PWM enable bit */
#define true                              1
#define false                             0

#define CHANNEL_CLK_SEL_FREQ      24000000        /* 24MHz */


/* PWM channel frequence selection */
enum E_Freq_Select {
	FREQ_SELECT_32K = 0,
	FREQ_SELECT_24M,
	/*add support when you have more choice */
	FREQ_SELECT_NUMBER
};

#define PWM_REG_CFG_SZ (sizeof(struct pwm_reg_cfg) / sizeof(uint))
#define PWM_FREQ_REG_CFG_SZ (sizeof(struct pwm_freq_reg_cfg) / sizeof(uint))

typedef struct pwm_reg_cfg {
	uint off;
	uint shift;
}S_pwm_reg_cfg;

typedef struct pwm_freq_reg_cfg {
	uint off;
	uint shift;
	uint width;
}S_pwm_freq_reg_cfg;

typedef struct axera_pwm_priv {
	ulong pwm_timer_base;
	ulong pwm_clk_base;
	struct pwm_freq_reg_cfg freq_sel_cfg;                /* Pwm channel frequence select */
	struct pwm_reg_cfg gate_en_cfg;                      /* Pwm gate clock enable */
	struct pwm_reg_cfg chan_glb_rst_cfg;                 /* Pwm channel global reset */
	struct pwm_reg_cfg chan_glb_en_cfg;                  /* Pwm channel global enable */
	struct pwm_reg_cfg chan_en_cfg[CHANNELS_PER_PWM];    /* Pwm channel enable */
	struct pwm_reg_cfg chan_rst_cfg[CHANNELS_PER_PWM];   /* Pwm channel reset */
	bool pwm_chan_enabled[CHANNELS_PER_PWM];
}S_axera_pwm_priv;

static void pwm_clk_reg_set(struct axera_pwm_priv *priv,
				uint off, uint shift, int enabled)
{
	ulong pwm_reg_addr = 0;
	u32  pwm_reg_data = 0;

	pwm_reg_addr = priv->pwm_clk_base + off;
	pwm_reg_data = readl(pwm_reg_addr);

	if (enabled) {
		pwm_reg_data |= (1 << shift);
	} else {
		pwm_reg_data &= ~(1 << shift);
	}
	writel(pwm_reg_data, pwm_reg_addr);
}

static void pwm_freq_reg_set(struct axera_pwm_priv *priv,
				uint off, uint shift, uint width, u32 select)
{
	ulong pwm_reg_addr = 0;
	u32   pwm_reg_data = 0;
	u32   pwm_reg_mask = 0;

	pwm_reg_addr = priv->pwm_clk_base + off;
	pwm_reg_data = readl(pwm_reg_addr);
	pwm_reg_mask = ((1 << width) - 1) << shift;
	pwm_reg_data &= ~pwm_reg_mask;
	pwm_reg_data |= (select << shift);
	writel(pwm_reg_data, pwm_reg_addr);
}

static void axera_pwm_channel_clk_enable(struct udevice *dev,
				uint channel)
{
	struct axera_pwm_priv *priv = dev_get_priv(dev);

	/* Pwm freq selction */
	pwm_freq_reg_set(priv, priv->freq_sel_cfg.off,
			priv->freq_sel_cfg.shift,
			priv->freq_sel_cfg.width,
			FREQ_SELECT_24M);

	/* Pwm channel reset */
	pwm_clk_reg_set(priv, priv->chan_rst_cfg[channel].off,
	 		priv->chan_rst_cfg[channel].shift, false);

	/* Pwm gate enable */
	pwm_clk_reg_set(priv, priv->gate_en_cfg.off,
			priv->gate_en_cfg.shift, true);

	/* Pwm global clock enable */
	pwm_clk_reg_set(priv, priv->chan_glb_en_cfg.off,
			priv->chan_glb_en_cfg.shift, true);

	/* Pwm channel clock enable */
	pwm_clk_reg_set(priv, priv->chan_en_cfg[channel].off,
			priv->chan_en_cfg[channel].shift, true);
}

static void axera_pwm_channel_clk_disable(struct udevice *dev,
				uint channel)
{
	struct axera_pwm_priv *priv = dev_get_priv(dev);

	/* Pwm channel clock disable */
	pwm_clk_reg_set(priv, priv->chan_en_cfg[channel].off,
			priv->chan_en_cfg[channel].shift, false);
}

static int axera_pwm_set_config(struct udevice *dev, uint channel,
				uint period_ns, uint duty_ns)
{
	struct axera_pwm_priv *priv = dev_get_priv(dev);
	u64 temp;
	u32 period_count;
	u32 duty_count;

	if (channel >= CHANNELS_PER_PWM) {
		pr_err("%s channel:%d is not avaiable\n", __func__, channel);
		return -EINVAL;
	}

	if (period_ns < (1000000000 / CHANNEL_CLK_SEL_FREQ)) {
		pr_err("period is to smaller, even smaller than input clock\n");
		return -EINVAL;
	}

	debug("%s: Config %s channel %u period:%u duty:%u\n", __func__,
		dev->name, channel, period_ns, duty_ns);

	/* Config pwm clock and enable */
	axera_pwm_channel_clk_enable(dev, channel);

	/* disable pwm timer and config pwm mode */
	writel(PWM_TIMERN_MODE,
		priv->pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(channel));

	temp = (u64)period_ns * CHANNEL_CLK_SEL_FREQ / 1000000000;
	period_count = (u32)temp;
	temp = (u64)duty_ns * CHANNEL_CLK_SEL_FREQ / 1000000000;
	duty_count = (u32)temp;

	debug("%s: config %s channel %u period_count:%u duty_count:%u\n", __func__,
		dev->name, channel, period_count, duty_count);

	writel(duty_count,
		priv->pwm_timer_base + PWM_TIMERN_LOADCOUNT2_OFF(channel));
	writel(period_count - duty_count,
		priv->pwm_timer_base + PWM_TIMERN_LOADCOUNT_OFF(channel));

	/*if channel is enabled current, keep enable state*/
	if (priv->pwm_chan_enabled[channel])
		writel((PWM_TIMERN_MODE | PWM_TIMERN_EN),
			priv->pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(channel));

	/* pwm clock disable */
	//axera_pwm_channel_clk_disable(dev, channel);

	return 0;
}

static int axera_pwm_set_enable(struct udevice *dev, uint channel,
				bool enable)
{
	struct axera_pwm_priv *priv = dev_get_priv(dev);
	u32 reg;

	if (channel >= CHANNELS_PER_PWM) {
		pr_err("%s channel:%d is not avaiable\n", __func__, channel);
		return -EINVAL;
	}

	debug("%s: enable '%s' channel %u\n", __func__, dev->name, channel);

	if (enable) {
		axera_pwm_channel_clk_enable(dev, channel);
		reg = readl(priv->pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(channel));
		reg |= PWM_TIMERN_EN;
		writel(reg, priv->pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(channel));
	} else {
		reg = readl(priv->pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(channel));
		reg &= ~PWM_TIMERN_EN;
		writel(reg, priv->pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(channel));
		axera_pwm_channel_clk_disable(dev, channel);
	}

	priv->pwm_chan_enabled[channel] = enable;

	return 0;
}

static int axera_pwm_ofdata_to_platdata(struct udevice *dev)
{
	u32 freq_sel[PWM_FREQ_REG_CFG_SZ];
	u32 gate_en[PWM_REG_CFG_SZ];
	u32 chan_glb_rst[PWM_REG_CFG_SZ];
	u32 chan_glb_en[PWM_REG_CFG_SZ];
	u32 chan_en[PWM_REG_CFG_SZ * CHANNELS_PER_PWM];
	u32 chan_rst[PWM_REG_CFG_SZ * CHANNELS_PER_PWM];
	int i, ret;
	struct axera_pwm_priv *priv;

	memset(freq_sel, 0, sizeof(freq_sel));
	memset(gate_en, 0, sizeof(gate_en));
	memset(chan_en, 0, sizeof(chan_en));
	memset(chan_rst, 0, sizeof(chan_rst));
	memset(chan_glb_en, 0, sizeof(chan_glb_en));
	memset(chan_glb_rst, 0, sizeof(chan_glb_rst));

	priv = dev_get_priv(dev);

	priv->pwm_timer_base = devfdt_get_addr_index(dev, 0);
	priv->pwm_clk_base = devfdt_get_addr_index(dev, 1);

	if ((priv->pwm_clk_base == FDT_ADDR_T_NONE) ||
		(priv->pwm_timer_base == FDT_ADDR_T_NONE)) {
		pr_err("axera pwm get reg fail\n");
		return -1;
	}

	ret = fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev), "chan-en",
					chan_en, (PWM_REG_CFG_SZ * CHANNELS_PER_PWM));
	if (ret) {
		pr_err("%s get chan_en fail\n", __func__);
		return -1;
	}

	ret = fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev), "freq-sel",
					freq_sel, PWM_FREQ_REG_CFG_SZ);
	if (ret) {
		pr_err("%s get freq_sel fail\n", __func__);
		return -1;
	}

	ret = fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev), "gate-en",
					gate_en, PWM_REG_CFG_SZ);
	if (ret) {
		pr_err("%s get gate_en fail\n", __func__);
		return -1;
	}

	ret = fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev), "chan-glb-en",
					chan_glb_en, PWM_REG_CFG_SZ);
	if (ret) {
		pr_err("%s get chan_glb_en fail\n", __func__);
		return -1;
	}

	ret = fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev), "chan-glb-rst",
					chan_glb_rst, PWM_REG_CFG_SZ);
	if (ret) {
		pr_err("%s get chan_glb_rst fail\n", __func__);
		return -1;
	}

	ret = fdtdec_get_int_array(gd->fdt_blob, dev_of_offset(dev), "chan-rst",
					chan_rst, (PWM_REG_CFG_SZ * CHANNELS_PER_PWM));
	if (ret) {
		pr_err("%s get chan_rst fail\n", __func__);
		return -1;
	}

	for (i = 0; i < CHANNELS_PER_PWM; i++) {
		priv->chan_en_cfg[i].off    = chan_en[i * PWM_REG_CFG_SZ];
		priv->chan_en_cfg[i].shift  = chan_en[i * PWM_REG_CFG_SZ + 1];
		priv->chan_rst_cfg[i].off   = chan_rst[i * PWM_REG_CFG_SZ];
		priv->chan_rst_cfg[i].shift = chan_rst[i * PWM_REG_CFG_SZ + 1];
	}

	priv->freq_sel_cfg.off       = freq_sel[0];
	priv->freq_sel_cfg.shift     = freq_sel[1];
	priv->freq_sel_cfg.width     = freq_sel[2];

	priv->gate_en_cfg.off        = gate_en[0];
	priv->gate_en_cfg.shift      = gate_en[1];

	priv->chan_glb_rst_cfg.off   = chan_glb_rst[0];
	priv->chan_glb_rst_cfg.shift = chan_glb_rst[1];

	priv->chan_glb_en_cfg.off    = chan_glb_en[0];
	priv->chan_glb_en_cfg.shift  = chan_glb_en[1];

	/* Pwm channel global reset */
	/* if global rest bit has been configed during using
		other pwm channels, here we can't reset again as
		it will affect the register settings before.
	*/
	if (readl(priv->pwm_clk_base + priv->chan_glb_rst_cfg.off) &
		(1 << priv->chan_glb_rst_cfg.shift))
		pwm_clk_reg_set(priv, priv->chan_glb_rst_cfg.off,
				priv->chan_glb_rst_cfg.shift, false);

	return 0;
}

static const struct pwm_ops axera_pwm_ops = {
	.set_config	= axera_pwm_set_config,
	.set_enable	= axera_pwm_set_enable,
};

static const struct udevice_id axera_pwm_ids[] = {
	{ .compatible = "axera,ax620e-pwm" },
	{ }
};

U_BOOT_DRIVER(axera_pwm) = {
	.name	= "axera_pwm",
	.id	= UCLASS_PWM,
	.of_match = axera_pwm_ids,
	.ops	= &axera_pwm_ops,
	.ofdata_to_platdata	= axera_pwm_ofdata_to_platdata,
	.priv_auto_alloc_size	= sizeof(struct axera_pwm_priv),
};
