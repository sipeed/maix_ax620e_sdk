// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * DWC3 controller driver
 *
 * Author: Ramneek Mehresh<ramneek.mehresh@freescale.com>
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <generic-phy.h>
#include <usb.h>
#include <dwc3-uboot.h>

#include <usb/xhci.h>
#include <asm/io.h>
#include <linux/usb/dwc3.h>
#include <linux/usb/otg.h>

struct xhci_dwc3_platdata {
	struct phy *usb_phys;
	int num_phys;
};

void dwc3_set_mode(struct dwc3 *dwc3_reg, u32 mode)
{
	clrsetbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG),
			DWC3_GCTL_PRTCAPDIR(mode));
}

static void dwc3_phy_reset(struct dwc3 *dwc3_reg)
{
	/* Assert USB3 PHY reset */
	setbits_le32(&dwc3_reg->g_usb3pipectl[0], DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Assert USB2 PHY reset */
	setbits_le32(&dwc3_reg->g_usb2phycfg, DWC3_GUSB2PHYCFG_PHYSOFTRST);

	mdelay(100);

	/* Clear USB3 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb3pipectl[0], DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Clear USB2 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb2phycfg, DWC3_GUSB2PHYCFG_PHYSOFTRST);
}

void dwc3_core_soft_reset(struct dwc3 *dwc3_reg)
{
	/* Before Resetting PHY, put Core in Reset */
	setbits_le32(&dwc3_reg->g_ctl, DWC3_GCTL_CORESOFTRESET);

	/* reset USB3 phy - if required */
	dwc3_phy_reset(dwc3_reg);

	mdelay(100);

	/* After PHYs are stable we can take Core out of reset state */
	clrbits_le32(&dwc3_reg->g_ctl, DWC3_GCTL_CORESOFTRESET);
}

int dwc3_core_init(struct dwc3 *dwc3_reg)
{
	u32 reg;
	u32 revision;
	unsigned int dwc3_hwparams1;

	revision = readl(&dwc3_reg->g_snpsid);
	/* This should read as U3 followed by revision number */
	if ((revision & DWC3_GSNPSID_MASK) != 0x55330000) {
		puts("this is not a DesignWare USB3 DRD Core\n");
		return -1;
	}

	dwc3_core_soft_reset(dwc3_reg);

	dwc3_hwparams1 = readl(&dwc3_reg->g_hwparams1);

	reg = readl(&dwc3_reg->g_ctl);
	reg &= ~DWC3_GCTL_SCALEDOWN_MASK;
	reg &= ~DWC3_GCTL_DISSCRAMBLE;
	switch (DWC3_GHWPARAMS1_EN_PWROPT(dwc3_hwparams1)) {
	case DWC3_GHWPARAMS1_EN_PWROPT_CLK:
		reg &= ~DWC3_GCTL_DSBLCLKGTNG;
		break;
	default:
		debug("No power optimization available\n");
	}

	/*
	 * WORKAROUND: DWC3 revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if ((revision & DWC3_REVISION_MASK) < 0x190a)
		reg |= DWC3_GCTL_U2RSTECN;

	writel(reg, &dwc3_reg->g_ctl);

	return 0;
}

void dwc3_set_fladj(struct dwc3 *dwc3_reg, u32 val)
{
	setbits_le32(&dwc3_reg->g_fladj, GFLADJ_30MHZ_REG_SEL |
			GFLADJ_30MHZ(val));
}

#ifdef CONFIG_ARCH_AXERA
/* ref clk adj value */
#define GFLADJ_REFCLK_240MHZ_DECR	0xa
#define GFLADJ_REFCLK_FLADJ			0x7f0
#define GUCTL_REFCLKPER				0x29
#define DWC3_GUCTL_REFCLKPER_MASK	0x3ff
#define DWC3_GFLADJ_REFCLK_LPM_SEL			BIT(23)
#define DWC3_GFLADJ_REFCLK_FLADJ_MASK		0x3fff
#define DWC3_GFLADJ_REFCLK_240MHZ_DECR_MASK	0x7f

void dwc3_axera_usb_set(uintptr_t base)
{
	u32 value;
	void *addr;

	/* DWC3_GFLADJ: 0xc630 */
	addr = (void *)(base+0xc630);
	value = readl(addr);
	value &= ~(DWC3_GFLADJ_REFCLK_FLADJ_MASK << 8);
	value &= ~(DWC3_GFLADJ_REFCLK_240MHZ_DECR_MASK << 24);
	value |= GFLADJ_REFCLK_240MHZ_DECR << 24 |
			 DWC3_GFLADJ_REFCLK_LPM_SEL 	|
			 GFLADJ_REFCLK_FLADJ << 8;
	writel(value, addr);

	/* DWC3_GUCTL: 0xc12c */
	addr = (void *)(base+0xc12c);
	value = readl(addr);
	value &= ~(DWC3_GUCTL_REFCLKPER_MASK << 22);
	value |= GUCTL_REFCLKPER << 22;
	writel(value, addr);

	/* DWC3_GUCTL1: 0xc11c */
	addr = (void *)(base+0xc11c);
	value = readl(addr);
	value |= (0x1 << 26);
	writel(value, addr);
}
#endif

#if CONFIG_IS_ENABLED(DM_USB)

static int xhci_dwc3_probe(struct udevice *dev)
{
	struct xhci_hcor *hcor;
	struct xhci_hccr *hccr;
	struct dwc3 *dwc3_reg;
	const char *phy;
	u32 reg;
#ifndef CONFIG_ARCH_AXERA
	enum usb_dr_mode dr_mode;
	struct xhci_dwc3_platdata *plat = dev_get_platdata(dev);
	int ret;
#endif

	hccr = (struct xhci_hccr *)((uintptr_t)dev_read_addr(dev));
	hcor = (struct xhci_hcor *)((uintptr_t)hccr +
			HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

#ifndef CONFIG_ARCH_AXERA
	ret = dwc3_setup_phy(dev, &plat->usb_phys, &plat->num_phys);
	if (ret && (ret != -ENOTSUPP))
		return ret;
#endif

	dwc3_reg = (struct dwc3 *)((char *)(hccr) + DWC3_REG_OFFSET);

	dwc3_core_init(dwc3_reg);

#ifdef CONFIG_ARCH_AXERA
	dwc3_axera_usb_set((uintptr_t) hccr);
#endif

	/* Set dwc3 usb2 phy config */
	reg = readl(&dwc3_reg->g_usb2phycfg[0]);

	phy = dev_read_string(dev, "phy_type");
	if (phy && strcmp(phy, "utmi_wide") == 0) {
		reg |= DWC3_GUSB2PHYCFG_PHYIF;
		reg &= ~DWC3_GUSB2PHYCFG_USBTRDTIM_MASK;
		reg |= DWC3_GUSB2PHYCFG_USBTRDTIM_16BIT;
	}

	if (dev_read_bool(dev, "snps,dis_enblslpm-quirk"))
		reg &= ~DWC3_GUSB2PHYCFG_ENBLSLPM;

	if (dev_read_bool(dev, "snps,dis-u2-freeclk-exists-quirk"))
		reg &= ~DWC3_GUSB2PHYCFG_U2_FREECLK_EXISTS;

	if (dev_read_bool(dev, "snps,dis_u2_susphy_quirk"))
		reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;

	writel(reg, &dwc3_reg->g_usb2phycfg[0]);

#ifdef CONFIG_ARCH_AXERA
	dwc3_set_mode(dwc3_reg, USB_DR_MODE_HOST);
#else
	dr_mode = usb_get_dr_mode(dev_of_offset(dev));
	if (dr_mode == USB_DR_MODE_UNKNOWN)
		/* by default set dual role mode to HOST */
		dr_mode = USB_DR_MODE_HOST;
#endif

	return xhci_register(dev, hccr, hcor);
}

static int xhci_dwc3_remove(struct udevice *dev)
{
	struct xhci_dwc3_platdata *plat = dev_get_platdata(dev);

	dwc3_shutdown_phy(dev, plat->usb_phys, plat->num_phys);

	return xhci_deregister(dev);
}

static const struct udevice_id xhci_dwc3_ids[] = {
	{ .compatible = "snps,dwc3" },
	{ }
};

U_BOOT_DRIVER(xhci_dwc3) = {
	.name = "xhci-dwc3",
	.id = UCLASS_USB,
	.of_match = xhci_dwc3_ids,
	.probe = xhci_dwc3_probe,
	.remove = xhci_dwc3_remove,
	.ops = &xhci_usb_ops,
	.priv_auto_alloc_size = sizeof(struct xhci_ctrl),
	.platdata_auto_alloc_size = sizeof(struct xhci_dwc3_platdata),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
#endif
