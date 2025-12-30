#include "gpio.h"
#include "cmn.h"

unsigned int ax_gpio_base(unsigned pin)
{
	unsigned int base;
	base = pin / 32;
	switch (base) {
	case 0:
		base = AX_GPIO_BASE0;
		break;
	case 1:
		base = AX_GPIO_BASE1;
		break;
	case 2:
		base = AX_GPIO_BASE2;
		break;
	case 3:
		base = AX_GPIO_BASE3;
		break;
	default:
		return -1;
	}
	return base;
}

int ax_gpio_direction_input(unsigned pin)
{
	unsigned int base, val;
	base = ax_gpio_base(pin);
	val = readl((u64)(base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC));
	val &= (~GPIO_SWPORTA_DDR);
	writel(val, (u64)(base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC));
	return 0;
}

int ax_gpio_direction_output(unsigned pin, int val)
{
	unsigned int value, base;
	base = ax_gpio_base(pin);
	if (val) {
		value = readl(base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
		value |= GPIO_SWPORTA_DDR;
		writel(value, base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
		value |= GPIO_SWPORTA_DR;
		writel(value, base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
	} else {
		value = readl(base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
		value |= GPIO_SWPORTA_DDR;
		writel(value, base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
		value &= (~GPIO_SWPORTA_DR);
		writel(value, base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
	}

	return 0;
}

int ax_gpio_get_value(unsigned pin)
{
	unsigned int base, val;
	base = ax_gpio_base(pin);
	val = readl(base + GPIO_EXT_PORTA);
	val = (val >> (pin % 32));
	val &= 1;
	return val;
}

int ax_gpio_set_value(unsigned pin, int val)
{
	unsigned int value, base;
	base = ax_gpio_base(pin);
	if (val) {
		value = readl(base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
		value |= GPIO_SWPORTA_DR;
		writel(val, base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
	} else {
		value = readl(base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
		value &= (~GPIO_SWPORTA_DR);
		writel(value, base + ((pin % 32) + 1) * GPIO_PORTA0_FUNC);
	}
	return 0;
}
