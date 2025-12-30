#ifndef __GPIO_H_
#define __GPIO_H_

#define AX_GPIO_BASE0       0x4800000
#define AX_GPIO_BASE1       0x4801000
#define AX_GPIO_BASE2       0x6000000
#define AX_GPIO_BASE3       0x6001000

#define GPIO_SWPORTA_DR		(1 << 0)
#define GPIO_SWPORTA_DDR	(1 << 1)
#define GPIO_SOFT_HAR_MODE 	(1 << 2)
#define GPIO_INTEN			(1 << 3)
#define GPIO_INTMASK		(1 << 4)
#define GPIO_INTTYPE_LEVEL	(1 << 5)
#define GPIO_INT_POLARITY	(1 << 6)
#define GPIO_PORTA_DEBOUNCE	(1 << 7)
#define GPIO_PORTA_EOI		(1 << 8)
#define GPIO_INT_BOTHEDGE	(1 << 9)

#define GPIO_PORTA0_FUNC				0x4
#define GPIO_INTSTATUS_SECURE			0x84
#define GPIO_RAW_INTSTATUS_SECURE		0x88
#define GPIO_EXT_PORTA					0x8c
#define GPIO_ID_CODE					0x90
#define GPIO_LS_SYNC					0x94
#define GPIO_VER_ID_CODE				0x98
#define GPIO_CONFIG_REG2				0x9c
#define GPIO_CONFIG_REG1				0xa0
#define GPIO_INTSTATUS_NSECURE			0xa4
#define GPIO_RAW_INTSTATUS_NSECURE		0xa8
#define GPIO_NSECURE_MODE_VALUE			0x0

int ax_gpio_direction_input(unsigned pin);
int ax_gpio_direction_output(unsigned pin, int val);
int ax_gpio_get_value(unsigned pin);
int ax_gpio_set_value(unsigned pin, int val);
#endif