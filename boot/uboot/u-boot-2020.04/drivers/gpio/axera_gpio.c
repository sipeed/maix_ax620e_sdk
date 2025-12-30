// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2015 Marek Vasut <marex@denx.de>
 *
 * DesignWare APB GPIO driver
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <errno.h>
#include <reset.h>

#define GPIO_SWPORTA_DR		BIT(0)
#define GPIO_SWPORTA_DDR	BIT(1)
#define GPIO_SOFT_HAR_MODE 	BIT(2)
#define GPIO_INTEN		BIT(3)
#define GPIO_INTMASK		BIT(4)
#define GPIO_INTTYPE_LEVEL	BIT(5)
#define GPIO_INT_POLARITY	BIT(6)
#define GPIO_PORTA_DEBOUNCE	BIT(7)
#define GPIO_PORTA_EOI		BIT(8)
#define GPIO_INT_BOTHEDGE	BIT(9)

#define GPIO_PORTA0_FUNC			0x4
#define GPIO_INTSTATUS_SECURE			0x84
#define GPIO_RAW_INTSTATUS_SECURE		0x88
#define GPIO_EXT_PORTA				0x8c
#define GPIO_ID_CODE				0x90
#define GPIO_LS_SYNC				0x94
#define GPIO_VER_ID_CODE			0x98
#define GPIO_CONFIG_REG2			0x9c
#define GPIO_CONFIG_REG1			0xa0
#define GPIO_INTSTATUS_NSECURE			0xa4
#define GPIO_RAW_INTSTATUS_NSECURE		0xa8
#define GPIO_NSECURE_MODE_VALUE			0x0
struct gpio_ax_priv {
	struct reset_ctl_bulk resets;
};

struct gpio_ax_platdata {
	const char *name;
	int bank;
	int pins;
	fdt_addr_t base;
	fdt_addr_t clk_rst_base;
};

static int ax_gpio_direction_input(struct udevice *dev, unsigned pin)
{
	struct gpio_ax_platdata *plat = dev_get_platdata(dev);

	clrbits_le32(plat->base + (pin + 1) * GPIO_PORTA0_FUNC, GPIO_SWPORTA_DDR);
	return 0;
}

static int ax_gpio_direction_output(struct udevice *dev, unsigned pin, int val)
{
	struct gpio_ax_platdata *plat = dev_get_platdata(dev);

	setbits_le32(plat->base + (pin + 1) * GPIO_PORTA0_FUNC, GPIO_SWPORTA_DDR);

	if (val)
		setbits_le32(plat->base + (pin + 1) * GPIO_PORTA0_FUNC, GPIO_SWPORTA_DR);
	else
		clrbits_le32(plat->base + (pin + 1) * GPIO_PORTA0_FUNC, GPIO_SWPORTA_DR);

	return 0;
}

static int ax_gpio_get_value(struct udevice *dev, unsigned pin)
{
	struct gpio_ax_platdata *plat = dev_get_platdata(dev);
	u32 val;
	if ((readl(plat->base + (pin + 1) * GPIO_PORTA0_FUNC) & GPIO_SWPORTA_DDR))
		return readl(plat->base + (pin + 1) * GPIO_PORTA0_FUNC) & GPIO_SWPORTA_DR;
	else {
		val = readl(plat->base + GPIO_EXT_PORTA);
		val = (val >> pin);
		val &= 1;
		return val;
	}
}

static int ax_gpio_set_value(struct udevice *dev, unsigned pin, int val)
{
	struct gpio_ax_platdata *plat = dev_get_platdata(dev);

	if(val == 0){
		val = readl(plat->base + (pin + 1) * GPIO_PORTA0_FUNC);
		val |= GPIO_SWPORTA_DDR;
		val &= (~GPIO_SWPORTA_DR);
		writel(val,plat->base + (pin + 1) * GPIO_PORTA0_FUNC);
		return 0;
	} else {
		val = readl(plat->base + (pin + 1) * GPIO_PORTA0_FUNC);
		val |= GPIO_SWPORTA_DDR;
		val |= GPIO_SWPORTA_DR;
		writel(val,plat->base + (pin + 1) * GPIO_PORTA0_FUNC);
		return 0;
	}

}

static int ax_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct gpio_ax_platdata *plat = dev_get_platdata(dev);
	u32 gpio;

	gpio = readl(plat->base + (offset + 1) * GPIO_PORTA0_FUNC);

	if (gpio & GPIO_SWPORTA_DDR)
		return GPIOF_OUTPUT;
	else
		return GPIOF_INPUT;
}

static const struct dm_gpio_ops gpio_ax_ops = {
	.direction_input = ax_gpio_direction_input,
	.direction_output = ax_gpio_direction_output,
	.get_value = ax_gpio_get_value,
	.set_value = ax_gpio_set_value,
	.get_function = ax_gpio_get_function,
};

static int gpio_ax_probe(struct udevice *dev)
{
	struct gpio_dev_priv *priv = dev_get_uclass_priv(dev);
	struct gpio_ax_platdata *plat = dev->platdata;
	if (!plat) {
		/* Reset on parent device only */
		return 0;
	}
	priv->gpio_count = plat->pins;
	priv->bank_name = plat->name;

	return 0;
}

static int gpio_ax_bind(struct udevice *dev)
{
	struct gpio_ax_platdata *plat = dev_get_platdata(dev);
	struct udevice *subdev;
	fdt_addr_t base;
	int ret, bank = 0;
	ofnode node;

	/* If this is a child device, there is nothing to do here */
	if (plat)
		return 0;

	base = dev_read_addr(dev);
	if (base == FDT_ADDR_T_NONE) {
		debug("Can't get the GPIO register base address\n");
		return -ENXIO;
	}

	for (node = dev_read_first_subnode(dev); ofnode_valid(node);
	     node = dev_read_next_subnode(node)) {
		if (!ofnode_read_bool(node, "gpio-controller"))
			continue;

		plat = devm_kcalloc(dev, 1, sizeof(*plat), GFP_KERNEL);
		if (!plat)
			return -ENOMEM;

		plat->base = base;
		plat->bank = bank;

		plat->pins = ofnode_read_u32_default(node, "ax,gpios", 0);

		if (ofnode_read_string_index(node, "bank-name", 0, &plat->name)) {
			/*
			 * Fall back to node name. This means accessing pins
			 * via bank name won't work.
			 */
			plat->name = ofnode_get_name(node);
		}

		ret = device_bind_ofnode(dev, dev->driver, plat->name,
					 plat, node, &subdev);
		if (ret)
			return ret;

		bank++;
	}

	return 0;
}

static const struct udevice_id gpio_ax_ids[] = {
	{.compatible = "axera,ax-apb-gpio"},
	{}
};

U_BOOT_DRIVER(gpio_ax) = {
	.name = "gpio-ax",
	.id = UCLASS_GPIO,
	.of_match = gpio_ax_ids,
	.ops = &gpio_ax_ops,
	.bind = gpio_ax_bind,
	.probe = gpio_ax_probe,
	.priv_auto_alloc_size = sizeof(struct gpio_ax_priv),
};