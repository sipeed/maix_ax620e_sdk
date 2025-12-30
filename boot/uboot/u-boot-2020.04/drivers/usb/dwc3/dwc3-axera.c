
#include <common.h>
#include <asm-generic/io.h>
#include <malloc.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/sizes.h>

#define FLASH_CLK_RST_BASE_ADDR 0x10030000
#define FLASH_CLK_MUX_0	(FLASH_CLK_RST_BASE_ADDR + 0x0)
#define FLASH_CLK_EB_0	(FLASH_CLK_RST_BASE_ADDR + 0x4)
#define FLASH_CLK_EB_1	(FLASH_CLK_RST_BASE_ADDR + 0x8)
#define FLASH_SW_RST0	(FLASH_CLK_RST_BASE_ADDR + 0x14)
#define USB2_CTRL		(FLASH_CLK_RST_BASE_ADDR + 0x40)

#define USB2_PHY_SW_RST 24
#define USB2_VCC_SW_RST 25
#define CLK_FLASH_GLB_SEL (0x7 << 6)	/* bit6..8 */
#define CLK_USB2_REF_EB 12
#define CLK_USB2_REF_ALT_CLK_EB 14
#define BUS_CLK_USB2_EB 5
#define VBUSVALID 6

#define DEVICE_MODE 1
#define HOST_MODE   0

struct dwc3_axera {
	struct udevice *dev;
};

void usb_clk_init(void)
{
	u32 value;
	void *addr;

	/* FLASH_SYS USB */
	addr = (void *)(FLASH_CLK_EB_0);
	value = readl(addr);
	value |= (0x1 << CLK_USB2_REF_EB);	//ref_clk
	value |= (0x1 << CLK_USB2_REF_ALT_CLK_EB);	//phy ref_clk
	writel(value, addr);

	addr = (void *)(FLASH_CLK_EB_1);
	value = readl(addr);
	value |= (0x1 << BUS_CLK_USB2_EB);	//usb bus_clk
	writel(value, addr);

	addr = (void *)(FLASH_CLK_MUX_0);
	value = readl(addr);
	value &= ~CLK_FLASH_GLB_SEL;
	value |= (0x1 << 6);	//bus_clk freq: 0:24M, 1:100M, 2:156M 3:208M, 4:250M, 5:312M
	writel(value, addr);
}

void usb_sw_rst(void)
{
	u32 value;
	void  *addr;

	addr = (void *)(FLASH_SW_RST0);
	value = readl(addr);
	value |= ((0x1 << USB2_VCC_SW_RST) |
			(0x01 << USB2_PHY_SW_RST));
	writel(value, addr);

	mdelay(1);

	value = readl(addr);
	value &= (~((0x1 << USB2_VCC_SW_RST) |
			(0x01 << USB2_PHY_SW_RST)));
	writel(value, addr);

	mdelay(1);
}

void usb_vbus_init(char usb_mode)
{
	u32 value;
	void *addr;

	addr = (void *)(USB2_CTRL);
	value = readl(addr);
	if (usb_mode == DEVICE_MODE)
		value |= (0x1 << VBUSVALID);
	else
		value &= (~(0x1 << VBUSVALID));
	writel(value, addr);
}

static int dwc3_axera_probe(struct udevice *dev)
{
	struct dwc3_axera *data = dev_get_platdata(dev);
	data->dev = dev;

	printf("\ndwc3 axera probe...\n");
	usb_clk_init();
	usb_sw_rst();
	usb_vbus_init(HOST_MODE);

	return 0;
}

static int dwc3_axera_remove(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id dwc3_axera_of_match[] = {
	{ .compatible = "axera,ax620e-dwc3", },
	{},
};

U_BOOT_DRIVER(dwc3_axera) = {
	.name = "dwc3-axera",
	.id = UCLASS_SIMPLE_BUS,
	.of_match = dwc3_axera_of_match,
	.probe = dwc3_axera_probe,
	.remove = dwc3_axera_remove,
	.platdata_auto_alloc_size = sizeof(struct dwc3_axera),
	.flags = DM_FLAG_OS_PREPARE,
};
