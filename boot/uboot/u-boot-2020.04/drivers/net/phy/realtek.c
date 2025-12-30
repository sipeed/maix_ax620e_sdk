// SPDX-License-Identifier: GPL-2.0+
/*
 * RealTek PHY drivers
 *
 * Copyright 2010-2011, 2015 Freescale Semiconductor, Inc.
 * author Andy Fleming
 * Copyright 2016 Karsten Merker <merker@debian.org>
 */
#include <common.h>
#include <linux/bitops.h>
#include <phy.h>

#define PHY_RTL8211x_FORCE_MASTER BIT(1)
#define PHY_RTL8211E_PINE64_GIGABIT_FIX BIT(2)
#define PHY_RTL8211F_FORCE_EEE_RXC_ON BIT(3)

#define PHY_AUTONEGOTIATE_TIMEOUT 5000

/* RTL8211x 1000BASE-T Control Register */
#define MIIM_RTL8211x_CTRL1000T_MSCE BIT(12);
#define MIIM_RTL8211x_CTRL1000T_MASTER BIT(11);

/* RTL8211x PHY Status Register */
#define MIIM_RTL8211x_PHY_STATUS       0x11
#define MIIM_RTL8211x_PHYSTAT_SPEED    0xc000
#define MIIM_RTL8211x_PHYSTAT_GBIT     0x8000
#define MIIM_RTL8211x_PHYSTAT_100      0x4000
#define MIIM_RTL8211x_PHYSTAT_DUPLEX   0x2000
#define MIIM_RTL8211x_PHYSTAT_SPDDONE  0x0800
#define MIIM_RTL8211x_PHYSTAT_LINK     0x0400

/* RTL8211x PHY Interrupt Enable Register */
#define MIIM_RTL8211x_PHY_INER         0x12
#define MIIM_RTL8211x_PHY_INTR_ENA     0x9f01
#define MIIM_RTL8211x_PHY_INTR_DIS     0x0000

/* RTL8211x PHY Interrupt Status Register */
#define MIIM_RTL8211x_PHY_INSR         0x13

/* RTL8211F PHY Status Register */
#define MIIM_RTL8211F_PHY_STATUS       0x1a
#define MIIM_RTL8211F_AUTONEG_ENABLE   0x1000
#define MIIM_RTL8211F_PHYSTAT_SPEED    0x0030
#define MIIM_RTL8211F_PHYSTAT_GBIT     0x0020
#define MIIM_RTL8211F_PHYSTAT_100      0x0010
#define MIIM_RTL8211F_PHYSTAT_DUPLEX   0x0008
#define MIIM_RTL8211F_PHYSTAT_SPDDONE  0x0800
#define MIIM_RTL8211F_PHYSTAT_LINK     0x0004

#define MIIM_RTL8211E_CONFREG           0x1c
#define MIIM_RTL8211E_CONFREG_TXD		0x0002
#define MIIM_RTL8211E_CONFREG_RXD		0x0004
#define MIIM_RTL8211E_CONFREG_MAGIC		0xb400	/* Undocumented */

#define MIIM_RTL8211E_EXT_PAGE_SELECT  0x1e

#define MIIM_RTL8211F_PAGE_SELECT      0x1f
#define MIIM_RTL8211F_TX_DELAY		0x100
#define MIIM_RTL8211F_LCR		0x10

static int rtl8211f_phy_extread(struct phy_device *phydev, int addr,
				int devaddr, int regnum)
{
	int oldpage = phy_read(phydev, MDIO_DEVAD_NONE,
			       MIIM_RTL8211F_PAGE_SELECT);
	int val;

	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PAGE_SELECT, devaddr);
	val = phy_read(phydev, MDIO_DEVAD_NONE, regnum);
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PAGE_SELECT, oldpage);

	return val;
}

static int rtl8211f_phy_extwrite(struct phy_device *phydev, int addr,
				 int devaddr, int regnum, u16 val)
{
	int oldpage = phy_read(phydev, MDIO_DEVAD_NONE,
			       MIIM_RTL8211F_PAGE_SELECT);

	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PAGE_SELECT, devaddr);
	phy_write(phydev, MDIO_DEVAD_NONE, regnum, val);
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PAGE_SELECT, oldpage);

	return 0;
}

static int rtl8211b_probe(struct phy_device *phydev)
{
#ifdef CONFIG_RTL8211X_PHY_FORCE_MASTER
	phydev->flags |= PHY_RTL8211x_FORCE_MASTER;
#endif

	return 0;
}

static int rtl8211e_probe(struct phy_device *phydev)
{
#ifdef CONFIG_RTL8211E_PINE64_GIGABIT_FIX
	phydev->flags |= PHY_RTL8211E_PINE64_GIGABIT_FIX;
#endif

	return 0;
}

static int rtl8211f_probe(struct phy_device *phydev)
{
#ifdef CONFIG_RTL8211F_PHY_FORCE_EEE_RXC_ON
	phydev->flags |= PHY_RTL8211F_FORCE_EEE_RXC_ON;
#endif

	return 0;
}

/* RealTek RTL8211x */
static int rtl8211x_config(struct phy_device *phydev)
{
	phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);

	/* mask interrupt at init; if the interrupt is
	 * needed indeed, it should be explicitly enabled
	 */
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211x_PHY_INER,
		  MIIM_RTL8211x_PHY_INTR_DIS);

	if (phydev->flags & PHY_RTL8211x_FORCE_MASTER) {
		unsigned int reg;

		reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_CTRL1000);
		/* force manual master/slave configuration */
		reg |= MIIM_RTL8211x_CTRL1000T_MSCE;
		/* force master mode */
		reg |= MIIM_RTL8211x_CTRL1000T_MASTER;
		phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, reg);
	}
	if (phydev->flags & PHY_RTL8211E_PINE64_GIGABIT_FIX) {
		unsigned int reg;

		phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PAGE_SELECT,
			  7);
		phy_write(phydev, MDIO_DEVAD_NONE,
			  MIIM_RTL8211E_EXT_PAGE_SELECT, 0xa4);
		reg = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211E_CONFREG);
		/* Ensure both internal delays are turned off */
		reg &= ~(MIIM_RTL8211E_CONFREG_TXD | MIIM_RTL8211E_CONFREG_RXD);
		/* Flip the magic undocumented bits */
		reg |= MIIM_RTL8211E_CONFREG_MAGIC;
		phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211E_CONFREG, reg);
		phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PAGE_SELECT,
			  0);
	}
	/* read interrupt status just to clear it */
	phy_read(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211x_PHY_INER);

	genphy_config_aneg(phydev);

	return 0;
}

static int rtl8211f_config(struct phy_device *phydev)
{
	u16 reg;

	if (phydev->flags & PHY_RTL8211F_FORCE_EEE_RXC_ON) {
		unsigned int reg;

		reg = phy_read_mmd(phydev, MDIO_MMD_PCS, MDIO_CTRL1);
		reg &= ~MDIO_PCS_CTRL1_CLKSTOP_EN;
		phy_write_mmd(phydev, MDIO_MMD_PCS, MDIO_CTRL1, reg);
	}

	phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);

	phy_write(phydev, MDIO_DEVAD_NONE,
		  MIIM_RTL8211F_PAGE_SELECT, 0xd08);
	reg = phy_read(phydev, MDIO_DEVAD_NONE, 0x11);

	/* enable TX-delay for rgmii-id and rgmii-txid, otherwise disable it */
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
#ifdef CONFIG_AXERA_EMAC
        phydev->interface == PHY_INTERFACE_MODE_RGMII ||
#endif
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)
		reg |= MIIM_RTL8211F_TX_DELAY;
	else
		reg &= ~MIIM_RTL8211F_TX_DELAY;

	phy_write(phydev, MDIO_DEVAD_NONE, 0x11, reg);
	/* restore to default page 0 */
	phy_write(phydev, MDIO_DEVAD_NONE,
		  MIIM_RTL8211F_PAGE_SELECT, 0x0);

	/* Set green LED for Link, yellow LED for Active */
	phy_write(phydev, MDIO_DEVAD_NONE,
		  MIIM_RTL8211F_PAGE_SELECT, 0xd04);
#ifdef CONFIG_AXERA_EMAC
	/*LED2(green) will light at 1000M and blink at transmitting.
	LED1(yellow) will light at 100M/10M and blink at transmitting.*/
	phy_write(phydev, MDIO_DEVAD_NONE, 0x10, 0x6271);
#else
	phy_write(phydev, MDIO_DEVAD_NONE, 0x10, 0x617f);
#endif	
	phy_write(phydev, MDIO_DEVAD_NONE,
		  MIIM_RTL8211F_PAGE_SELECT, 0x0);

	genphy_config_aneg(phydev);

	return 0;
}

static int rtl8211x_parse_status(struct phy_device *phydev)
{
	unsigned int speed;
	unsigned int mii_reg;

	mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211x_PHY_STATUS);

	if (!(mii_reg & MIIM_RTL8211x_PHYSTAT_SPDDONE)) {
		int i = 0;

		/* in case of timeout ->link is cleared */
		phydev->link = 1;
		puts("Waiting for PHY realtime link");
		while (!(mii_reg & MIIM_RTL8211x_PHYSTAT_SPDDONE)) {
			/* Timeout reached ? */
			if (i > PHY_AUTONEGOTIATE_TIMEOUT) {
				puts(" TIMEOUT !\n");
				phydev->link = 0;
				break;
			}

			if ((i++ % 1000) == 0)
				putc('.');
			udelay(1000);	/* 1 ms */
			mii_reg = phy_read(phydev, MDIO_DEVAD_NONE,
					MIIM_RTL8211x_PHY_STATUS);
		}
		puts(" done\n");
		udelay(500000);	/* another 500 ms (results in faster booting) */
	} else {
		if (mii_reg & MIIM_RTL8211x_PHYSTAT_LINK)
			phydev->link = 1;
		else
			phydev->link = 0;
	}

	if (mii_reg & MIIM_RTL8211x_PHYSTAT_DUPLEX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	speed = (mii_reg & MIIM_RTL8211x_PHYSTAT_SPEED);

	switch (speed) {
	case MIIM_RTL8211x_PHYSTAT_GBIT:
		phydev->speed = SPEED_1000;
		break;
	case MIIM_RTL8211x_PHYSTAT_100:
		phydev->speed = SPEED_100;
		break;
	default:
		phydev->speed = SPEED_10;
	}

	return 0;
}

static int rtl8211f_parse_status(struct phy_device *phydev)
{
	unsigned int speed;
	unsigned int mii_reg;
	int i = 0;

	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PAGE_SELECT, 0xa43);
	mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PHY_STATUS);

	phydev->link = 1;
	while (!(mii_reg & MIIM_RTL8211F_PHYSTAT_LINK)) {
		if (i > PHY_AUTONEGOTIATE_TIMEOUT) {
			puts(" TIMEOUT !\n");
			phydev->link = 0;
			break;
		}

		if ((i++ % 1000) == 0)
			putc('.');
		udelay(1000);
		mii_reg = phy_read(phydev, MDIO_DEVAD_NONE,
				   MIIM_RTL8211F_PHY_STATUS);
	}

	if (mii_reg & MIIM_RTL8211F_PHYSTAT_DUPLEX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	speed = (mii_reg & MIIM_RTL8211F_PHYSTAT_SPEED);

	switch (speed) {
	case MIIM_RTL8211F_PHYSTAT_GBIT:
		phydev->speed = SPEED_1000;
		break;
	case MIIM_RTL8211F_PHYSTAT_100:
		phydev->speed = SPEED_100;
		break;
	default:
		phydev->speed = SPEED_10;
	}

	return 0;
}

static int rtl8211x_startup(struct phy_device *phydev)
{
	int ret;

	/* Read the Status (2x to make sure link is right) */
	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	return rtl8211x_parse_status(phydev);
}

static int rtl8211e_startup(struct phy_device *phydev)
{
	int ret;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	return genphy_parse_link(phydev);
}

static int rtl8211f_startup(struct phy_device *phydev)
{
	int ret;

	/* Read the Status (2x to make sure link is right) */
	ret = genphy_update_link(phydev);
	if (ret)
		return ret;
	/* Read the Status (2x to make sure link is right) */

	return rtl8211f_parse_status(phydev);
}

#ifdef CONFIG_AXERA_EMAC
static void ax_ephy_802_3az_eee_disable(struct phy_device *phydev)
{
	unsigned int value;

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0000);	/* Switch to Page 0 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x7);
	phy_write(phydev, MDIO_DEVAD_NONE, 0xe, 0x3c);
	phy_write(phydev, MDIO_DEVAD_NONE, 0xd, BIT(14) | 0x7);
	value = phy_read(phydev, MDIO_DEVAD_NONE, 0xe);
	value &= ~BIT(1);
	phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x7);
	phy_write(phydev, MDIO_DEVAD_NONE, 0xe, 0x3c);
	phy_write(phydev, MDIO_DEVAD_NONE, 0xd, BIT(14) | 0x7);
	phy_write(phydev, MDIO_DEVAD_NONE, 0xe, value);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0200);	/* switch to page 2 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x18, 0x0000);
}

static int ax_ephy_afe_rx_set(struct phy_device *phydev)
{
	unsigned int value;

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0600);	/* Switch to Page 6 */
	value = phy_read(phydev, MDIO_DEVAD_NONE, 0x10);
	value &= ~0x7;
	value |= 0x6;
	value &= ~(0x7<<3);
	value |= (0x5<<3);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x10, value);	/* Adc gain optimization */

	value = phy_read(phydev, MDIO_DEVAD_NONE, 0x14);
	value &= ~(0x3<<13);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, value);	/* Adc gain optimization */

	return 0;
}

static int ax_ephy_config(struct phy_device *phydev)
{
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0100);	/* Switch to Page 1 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x12, 0x4824);	/* Disable APS */

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0200);	/* Switch to Page 2 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x18, 0x0000);	/* 10 base-t filter selector */

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0300);	/* Switch to Page 3 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x11, 0x8010);	/* AGC to minimum */

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0600);	/* Switch to Page 6 */
	// phy_write(phydev, MDIO_DEVAD_NONE, 0x10, 0x5540);	/* Adc gain optimization */
	// phy_write(phydev, MDIO_DEVAD_NONE, 0x12, 0x8400);	/* Adc gain optimization */
	// phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x1088);	/* Adc gain optimization */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x15, 0x3333);	/* a_TX_level optimiztion */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x19, 0x004c);	/* PHYAFE TX optimization and invorce ADC clock */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1b, 0x888f);	/* TX_BIAS */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1c, 0x8880);	/* PHYAFE PDCW optimization */

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0800);	/* Switch to Page 8 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x0844);	/* PHYAFE Tx_auto */

	ax_ephy_802_3az_eee_disable(phydev);		/* Disable 802.3az IEEE */
	// ax_ephy_afe_rx_set(phydev);

	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x0000);	/* Switch to Page 0 */

	return 0;
}

static int ax_ephy_probe(struct phy_device *phydev)
{
	return 0;
}
#endif

/* Support for RTL8211B PHY */
static struct phy_driver RTL8211B_driver = {
	.name = "RealTek RTL8211B",
	.uid = 0x1cc912,
	.mask = 0xffffff,
	.features = PHY_GBIT_FEATURES,
	.probe = &rtl8211b_probe,
	.config = &rtl8211x_config,
	.startup = &rtl8211x_startup,
	.shutdown = &genphy_shutdown,
};

/* Support for RTL8211E-VB-CG, RTL8211E-VL-CG and RTL8211EG-VB-CG PHYs */
static struct phy_driver RTL8211E_driver = {
	.name = "RealTek RTL8211E",
	.uid = 0x1cc915,
	.mask = 0xffffff,
	.features = PHY_GBIT_FEATURES,
	.probe = &rtl8211e_probe,
	.config = &rtl8211x_config,
	.startup = &rtl8211e_startup,
	.shutdown = &genphy_shutdown,
};

/* Support for RTL8211DN PHY */
static struct phy_driver RTL8211DN_driver = {
	.name = "RealTek RTL8211DN",
	.uid = 0x1cc914,
	.mask = 0xffffff,
	.features = PHY_GBIT_FEATURES,
	.config = &rtl8211x_config,
	.startup = &rtl8211x_startup,
	.shutdown = &genphy_shutdown,
};

/* Support for RTL8211F PHY */
static struct phy_driver RTL8211F_driver = {
	.name = "RealTek RTL8211F",
	.uid = 0x1cc916,
	.mask = 0xffffff,
	.features = PHY_GBIT_FEATURES,
	.probe = &rtl8211f_probe,
	.config = &rtl8211f_config,
	.startup = &rtl8211f_startup,
	.shutdown = &genphy_shutdown,
	.readext = &rtl8211f_phy_extread,
	.writeext = &rtl8211f_phy_extwrite,
};

#ifdef CONFIG_AXERA_EMAC
static struct phy_driver JL2101_driver = {
	.name = "JL2101 1000M Ethernet",
	.uid = 0x937c4020,
	.mask = 0x1fffffff,
	.features = PHY_GBIT_FEATURES,
	.probe = &rtl8211f_probe,
	.config = &rtl8211f_config,
	.startup = &rtl8211f_startup,
	.shutdown = &genphy_shutdown,
	.readext = &rtl8211f_phy_extread,
	.writeext = &rtl8211f_phy_extwrite,
};

static struct phy_driver ax_ephy_driver = {
	.name = "AX620E Fast Ethernet",
	.uid = 0x00441400,
	.mask = 0x0ffffff0,
	.features = PHY_BASIC_FEATURES,
	.probe = &ax_ephy_probe,
	.config = &ax_ephy_config,
	.startup = &genphy_startup,
	.shutdown = &genphy_shutdown,
};
#endif

int phy_realtek_init(void)
{
	phy_register(&RTL8211B_driver);
	phy_register(&RTL8211E_driver);
	phy_register(&RTL8211F_driver);
	phy_register(&RTL8211DN_driver);
#ifdef CONFIG_AXERA_EMAC
	phy_register(&JL2101_driver);
	phy_register(&ax_ephy_driver);
#endif
	return 0;
}
