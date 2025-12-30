#include <common.h>
#include <adc.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <dm/device_compat.h>
#include <linux/iopoll.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <linux/err.h>
#include <sdhci.h>
#include <malloc.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch-axera/dma.h>

#define AX_ADC_MA_EN		0x4
#define AX_ADC_MA_POR_EN	0x8
#define AX_ADC_MA_CTRL		0x10
#define AX_ADC_MA_POR_CTRL	0x14
#define AX_ADC_CTRL		0x18
#define AX_ADC_CLK_EN		0x1C	/*clk enable */
#define AX_ADC_CLK_SELECT	0x20	/*clk select */
#define AX_ADC_RSTN		0x24
#define AX_ADC_MON_EN		0xC8
#define AX_ADC_MON_CH		0xC4
#define AX_ADC_MON_INTERVAL	0xCC

#define AX_ADC_DATA_CHANNEL0	0xA0
#define AX_ADC_DATA_CHANNEL1	0xA4
#define AX_ADC_DATA_CHANNEL2	0xA8
#define AX_ADC_DATA_CHANNEL3	0xAC
#define AX_ADC_SEL(x)		(1 << (10 + x))
#define AX_ADC_SEL_EN		BIT(0)

struct ax_adc_data {
	int num_bits;
	int num_channels;
};

struct ax_adc_priv {
	void __iomem *regs;
	int active_channel;
	const struct ax_adc_data *data;
	struct udevice *dev;
	int max_id;
};

int ax_adc_start_channel(struct udevice *dev, int channel)
{
	struct ax_adc_priv *priv = dev_get_priv(dev);
	if ((channel < 0) || (channel > 3) || (priv == NULL)) {
		printf("The channel is invalid\n");
		return 0;
	}

	priv->active_channel = channel;
	return 0;
}

int ax_adc_channel_data(struct udevice *dev, int channel, unsigned int *data)
{

	struct ax_adc_priv *priv = dev_get_priv(dev);
	if (priv == NULL) {
		pr_err("get pirv failed!");
		return -EINVAL;
	} else if (channel != priv->active_channel) {
		pr_err("Requested channel is not active!");
		return -EINVAL;
	}

	switch (channel) {
	case 0:
		*data = 0x3ff & readl((priv->regs) + AX_ADC_DATA_CHANNEL0);
		break;
	case 1:
		*data = 0x3ff & readl((priv->regs) + AX_ADC_DATA_CHANNEL1);
		break;
	case 2:
		*data = 0x3ff & readl((priv->regs) + AX_ADC_DATA_CHANNEL2);
		break;
	case 3:
		*data = 0x3ff & readl((priv->regs) + AX_ADC_DATA_CHANNEL3);
		break;
	}
	return 0;

}

int ax_adc_stop(struct udevice *dev)
{
	struct ax_adc_priv *priv = dev_get_priv(dev);
	if (priv == NULL) {
		printf("Dev Stop Failed\n");
		return 0;
	}
	/* Power down adc */
	priv->active_channel = -1;

	return 0;
}

int ax_adc_probe(struct udevice *dev)
{
	struct ax_adc_priv *priv = dev_get_priv(dev);
	if (priv == NULL) {
		printf("Probe Failed\n");
		return 0;
	}
	printf("%s\n", __func__);
	priv->active_channel = -1;
	return 0;
}

int ax_adc_ofdata_to_platdata(struct udevice *dev)
{
	struct adc_uclass_platdata *uc_pdata = dev_get_uclass_platdata(dev);
	struct ax_adc_priv *priv = dev_get_priv(dev);
	struct ax_adc_data *data;

	data = (struct ax_adc_data *)dev_get_driver_data(dev);
	priv->regs = dev_read_addr_ptr(dev);;
	if (priv->regs == NULL) {
		pr_err("Dev: %s - can't get address!", dev->name);
		return -ENODATA;
	}
	priv->data = data;
	uc_pdata->data_format = ADC_DATA_FORMAT_BIN;
	uc_pdata->data_mask = GENMASK(9, 0);
	uc_pdata->channel_mask = GENMASK(3, 0);

	return 0;
}

static const struct adc_ops ax_adc_ops = {
	.start_channel = ax_adc_start_channel,
	.channel_data = ax_adc_channel_data,
	.stop = ax_adc_stop,
};

struct ax_adc_data adc_data = {
	.num_bits = 10,
	.num_channels = 4,
};

static const struct udevice_id ax_adc_ids[] = {
	{.compatible = "axera,ax620e-adc",},
	{},
};

U_BOOT_DRIVER(ax_adc) = {
	.name = "ax_adc",
	.id = UCLASS_ADC,
	.of_match = ax_adc_ids,
	.ops =&ax_adc_ops,
	.probe = ax_adc_probe,
	.ofdata_to_platdata =ax_adc_ofdata_to_platdata,
	.priv_auto_alloc_size =sizeof(struct ax_adc_priv),
};
