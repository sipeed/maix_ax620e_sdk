/*
 * This driver supports the Synopsys Designware Ethernet QOS (Quality Of Service) IP block. 
 *
 */

//#define DEBUG Y

#include <common.h>
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <phy.h>
#include <reset.h>
#include <wait_bit.h>
#include <asm/gpio.h>
#include <asm/io.h>

#include "axera_emac.h"

// support dma rx desc cached memory
// #define CONFIG_AXERA_EMAC_USE_CACHED_DESC Y
#ifdef CONFIG_AXERA_EMAC_USE_CACHED_DESC
#undef CONFIG_SYS_NONCACHED_MEMORY
#endif

#define FLASH_SYS_GLB_BASE_ADDR	0x10030000

#define EMAC_FLASH_CLK_MUX_0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x0)
#define EMAC_RGMII_TX_SEL       4

#define EMAC_FLASH_SW_RST_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x14)
#define EMAC_SW_RST             8
#define EMAC_EPHY_SW_RST		9

#define EMAC_FLASH_EMAC0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x28)
#define EMAC_RX_CLK_DLY_SEL     5
#define EMAC_PHY_IF_SEL         9
#define EMAC_EXT_PAD_SEL        10
#define EMAC_PHY_LOOPBACK_EN	11

#define EMAC_FLASH_CLK_EB0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x4)
#define EMAC_RMII_PHY_EB	4
#define EMAC_RGMII_TX_EB 	3
#define EMAC_PTP_REF_EB 	2
#define EMAC_EPHY_CLK_EB 	13

#define EMAC_FLASH_CLK_EB1_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x8)
#define EMAC_BW_24M			8
#define EMAC_ACLK			2

#define EMAC_FLASH_SYS_GLB0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x144)
#define EMAC_RMII_RX_DIVN		0
#define EMAC_RMII_RX_DIVN_UP	4
#define EMAC_RMII_RX_DIV_EN		5

#define EMAC_FLASH_EPHY_0_ADDR (FLASH_SYS_GLB_BASE_ADDR + 0x20)
#define EMAC_EPHY_SHUTDOWN		0
#define EMAC_EPHY_CLK_SEL		5
#define EMAC_EPHY_LED_POL		13
#define EMAC_EFUSE_2_EPHY_OTP_BG	24

#ifdef CONFIG_AXERA_EMAC_HAPS
void delay_us(long usec)
{
	udelay(usec);

}

void delay_ms(long msec)
{
	mdelay(msec);
}
#else
//for debug in none-timer env
void delay_us(long usec)
{
	while(usec--)
		printf("\r\r\r");
}

void delay_ms(long msec)
{
	while(msec--)
		delay_us(10);
}
#endif

void ax_shutdown_ephy(void)
{
	u32 value;
	void *addr;

	addr = (void *)(EMAC_FLASH_EPHY_0_ADDR);
	value = readl(addr);

	if ((value & (0x1<<EMAC_EPHY_SHUTDOWN)) == 0x0) {
		value |= (0x1<<EMAC_EPHY_SHUTDOWN);
		writel(value, addr);
		printf("%s:0x%x\n", __func__, value);
	}
}

static void emac_rgmii_set_tx_speed(int bus_id, int speed)
{
	u32 value;
	void *addr;
	u8 pos = EMAC_RGMII_TX_SEL;

	addr = (void *)(EMAC_FLASH_CLK_MUX_0_ADDR);
	value = readl(addr);
	value &= ~(0x3 << pos);

	switch (speed)
	{
	case SPEED_10:
		value |= (0x0 << pos);
		break;
	case SPEED_100:
		value |= (0x1 << pos);
		break;
	case SPEED_1000:
		value |= (0x2 << pos);
		break;
	default:
		printf("EMAC:un-supported emac rgmii phy speed\n");
		return;
	}

	writel(value, addr);
}

static void emac_rmii_set_speed(int bus_id, int speed)
{
	u32 value;
	void *addr;
	u8 divn_pos = EMAC_RMII_RX_DIVN;
	u8 divn_update = EMAC_RMII_RX_DIVN_UP;

	//clk set
	addr = (void *)(EMAC_FLASH_SYS_GLB0_ADDR);
	value = readl(addr);
	value &= ~(0xF << divn_pos);

	switch (speed)
	{
	case SPEED_10:
		value |= (0xA << divn_pos);	//2.5M
		break;
	case SPEED_100:
		value |= (0x1 << divn_pos);	//25M
		break;
	default:
		printk("EMAC:un-supported emac0 rmii phy speed\n");
		return;
	}

	//clk update
	value |= (0x1 << divn_update);

	writel(value, addr);
}

static void emac_sw_rst(int bus_id)
{
	u32 value;
	void *addr;
	unsigned char emac_rst;


	addr = (void *)(EMAC_FLASH_SW_RST_ADDR);
	emac_rst = EMAC_SW_RST;
	value = readl(addr);
	value |= (0x1 << emac_rst);
	writel(value, addr);

#ifdef CONFIG_AXERA_EMAC_HAPS
	delay_ms(50);
#else
	mdelay(50); //20
#endif

	value = readl(addr);
	value &= (~(0x1 << emac_rst));
	writel(value, addr);

#ifdef CONFIG_AXERA_EMAC_HAPS
	delay_ms(50);
#else
	mdelay(50);
#endif
}

static void emac_clk_init(void)
{
	u32 value;
	void *addr;

	debug("configure EMAC clk...\n");

	/* enable aclk & pclk */
	addr = (void *)(EMAC_FLASH_CLK_EB1_ADDR);
	value = readl(addr);
	value |= (0x1 << EMAC_ACLK);
	writel(value, addr);

	/* enable rgmii tx clk */
	addr =  (void *)(EMAC_FLASH_CLK_EB0_ADDR);
	//RMII
	value = readl(addr);
	value |= (0x1 << EMAC_RMII_PHY_EB);
	writel(value, addr);

	//RGMII
	value = readl(addr);
	value |= (0x1 << EMAC_RGMII_TX_EB);
	writel(value, addr);

	//EPHY CLK
	value = readl(addr);
	value |= (0x1 << EMAC_EPHY_CLK_EB);
	writel(value, addr);

	//PTP
	value = readl(addr);
	value |= (0x1 << EMAC_PTP_REF_EB);
	writel(value, addr);

	/* enable rmii clock */
	addr =  (void *)(EMAC_FLASH_SYS_GLB0_ADDR);
	value = readl(addr);
	value |= (0x1 << EMAC_RMII_RX_DIV_EN);
	writel(value, addr);
}

static void emac_phy_select(struct eqos_priv *eqos, phy_interface_t interface)
{
	u32 value;
	void *addr;

	addr = (void *)(EMAC_FLASH_EMAC0_ADDR);
	value = readl(addr);
	value &= (~(0x3<<EMAC_PHY_IF_SEL));

	if (interface == PHY_INTERFACE_MODE_RGMII) {
		value |= (0x1 << EMAC_PHY_IF_SEL) | (0x1 << EMAC_EXT_PAD_SEL);
		debug("EMAC: select RGMII interface\n");
	} else if (interface == PHY_INTERFACE_MODE_RMII) {
		if (eqos->out_rmii_mode) {
			value |= (0x0 << EMAC_PHY_IF_SEL) | (0x1 << EMAC_EXT_PAD_SEL);
			debug("EMAC: select Out RMII interface\n");
		} else {
			value |= (0x0 << EMAC_PHY_IF_SEL) | (0x0 << EMAC_EXT_PAD_SEL);
			debug("EMAC: select Inner RMII interface, value:0x%08x\n", value);
		}
	} else if (interface == PHY_INTERFACE_MODE_GMII) {
		value |= (0x1 << EMAC_PHY_IF_SEL) | (0x1 << EMAC_EXT_PAD_SEL);
		debug("EMAC: select GMII interface\n");
	} else {
		printf("EMAC: don't support this phy interface\n");
		return;
	}

	writel(value, addr);
}

static int eqos_start_clks_ax620e(struct udevice *dev)
{
	emac_clk_init();
	return 0;
}

static void select_phy_interface(struct udevice *dev, phy_interface_t interface)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	emac_phy_select(eqos, interface);
}

#ifdef USE_CLK_FROM_DTS
static void eqos_stop_clks_ax620e(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	clk_disable(&eqos->clk_master_bus);
}
#else
static void eqos_stop_clks_ax620e(struct udevice *dev)
{
	return;
}
#endif

static void emac_ephy_sw_reset(struct eqos_priv *eqos)
{
	u32 value_rst;
	u32 value_shutdown;
	void  *addr_rst;
	void  *addr_shutdown;

	debug("reset ephy ...\n");

	addr_rst = (void *)(EMAC_FLASH_SW_RST_ADDR);
	addr_shutdown = (void *)(EMAC_FLASH_EPHY_0_ADDR);

	//ephy_rst=1;
	value_rst = readl(addr_rst);
	value_rst |= (0x1 << EMAC_EPHY_SW_RST);
	writel(value_rst, addr_rst);

	//ephy shutdown=0;
	value_shutdown = readl(addr_shutdown);
	value_shutdown &= (~(0x1 << EMAC_EPHY_SHUTDOWN));
	writel(value_shutdown, addr_shutdown);

	mdelay(15);

	//ephy_rst=0
	value_rst = readl(addr_rst);
	value_rst &= (~(0x1 << EMAC_EPHY_SW_RST));
	writel(value_rst, addr_rst);

	udelay(15);
}

static void emac_phy_gpio_reset(struct eqos_priv *eqos)
{
	debug("reset phy gpio...\n");

	if (!dm_gpio_is_valid(&eqos->phy_reset_gpio))
		return;

	dm_gpio_set_value(&eqos->phy_reset_gpio, 0);
	mdelay(100);
	dm_gpio_set_value(&eqos->phy_reset_gpio, 1);
	mdelay(200);
}

static void emac_ephy_set_bgs(struct eqos_priv *eqos)
{
	misc_info_t *misc_info;
	//void  *regs;
	void  *addr;
	u32 value;

	misc_info = (misc_info_t *) MISC_INFO_ADDR;

	addr = (void *)(EMAC_FLASH_EPHY_0_ADDR);

	value = readl(addr);

	//efuse2ephy_otp_bg;
	value &= (~(0xf<<EMAC_EFUSE_2_EPHY_OTP_BG));
	if ( misc_info->trim == 0x7 ) {
		value |= ((misc_info->bgs & 0xf)<<EMAC_EFUSE_2_EPHY_OTP_BG);
	} else {
		if (misc_info->bgs != 0xc &&
			misc_info->bgs != 0xd
		) {
			value |= ((misc_info->bgs & 0xf)<<EMAC_EFUSE_2_EPHY_OTP_BG);
		} else {
			debug("ephy bgs=0x%x, correct to bgs=0x0\n", misc_info->bgs);
		}
	}
	writel(value, addr);

	//printf("ephy bgs=0x%x, ephy0_reg=0x%08x\n", misc_info->bgs, value);
}

static void emac_ephy_set_clk(void)
{
	void  *addr;
	u32 value;

	//printf("-----%s-----\n",__func__);

	addr = (void *)(EMAC_FLASH_EPHY_0_ADDR);

	value = readl(addr);
	value &= (~(0x7<<EMAC_EPHY_CLK_SEL));
	value |= (0x4<<EMAC_EPHY_CLK_SEL);
	writel(value, addr);

	mdelay(10);

	value = readl(addr);
	value &= (~(0x7<<EMAC_EPHY_CLK_SEL));
	value |= (0x6<<EMAC_EPHY_CLK_SEL);
	writel(value, addr);

	mdelay(10);
}

static void emac_phy_reset(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("reset %s phy...\n", dev->name);

	if (eqos->interface == PHY_INTERFACE_MODE_RMII &&
		!eqos->out_rmii_mode )
	{
		emac_ephy_set_clk();
		emac_ephy_set_bgs(eqos);
		emac_ephy_sw_reset(eqos);
	} else {
		emac_phy_gpio_reset(eqos);
	}
}

static int eqos_start_resets_ax620e(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	emac_sw_rst(eqos->id);
	emac_phy_reset(dev);  //reset emac phy

	return 0;
}

static int eqos_stop_resets_ax620e(struct udevice *dev)
{
	return 0;
}

#ifdef USE_CLK_FROM_DTS
static ulong eqos_get_tick_clk_rate_ax620e(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	return clk_get_rate(&eqos->clk_master_bus);
}
#else
static ulong eqos_get_tick_clk_rate_ax620e(struct udevice *dev)
{
	int value;

	value = dev_read_u32_default(dev, "emac-bus-clock", 1);
	return value;
}
#endif

/*
 * TX and RX descriptors are 16 bytes. This causes problems with the cache
 * maintenance on CPUs where the cache-line size exceeds the size of these
 * descriptors. What will happen is that when the driver receives a packet
 * it will be immediately requeued for the hardware to reuse. The CPU will
 * therefore need to flush the cache-line containing the descriptor, which
 * will cause all other descriptors in the same cache-line to be flushed
 * along with it. If one of those descriptors had been written to by the
 * device those changes (and the associated packet) will be lost.
 *
 * To work around this, we make use of non-cached memory if available. If
 * descriptors are mapped uncached there's no need to manually flush them
 * or invalidate them.
 *
 * Note that this only applies to descriptors. The packet data buffers do
 * not have the same constraints since they are 1536 bytes large, so they
 * are unlikely to share cache-lines.
 */
static void *eqos_alloc_descs(unsigned int num)
{
#ifdef CONFIG_SYS_NONCACHED_MEMORY
	debug("\nalloc noncached memory for desc\n");
	return (void *)noncached_alloc(EQOS_DESCRIPTORS_SIZE,
				      EQOS_DESCRIPTOR_ALIGN);
#else
	debug("\nalloc cached memory for desc\n");
	return memalign(EQOS_DESCRIPTOR_ALIGN, EQOS_DESCRIPTORS_SIZE);
#endif
}

static void eqos_free_descs(void *descs)
{
#ifdef CONFIG_SYS_NONCACHED_MEMORY
	/* FIXME: noncached_alloc() has no opposite */
#else
	free(descs);
#endif
}


static void eqos_inval_desc_ax620e(void *desc)
{
#ifndef CONFIG_SYS_NONCACHED_MEMORY
	unsigned long start = rounddown((unsigned long)desc, ARCH_DMA_MINALIGN);
	unsigned long end = roundup((unsigned long)desc + EQOS_DESCRIPTOR_SIZE,
				    ARCH_DMA_MINALIGN);

	invalidate_dcache_range(start, end);
#endif
}


static void eqos_flush_desc_ax620e(void *desc)
{
#ifndef CONFIG_SYS_NONCACHED_MEMORY
	unsigned long start = rounddown((unsigned long)desc, ARCH_DMA_MINALIGN);
	unsigned long end = roundup((unsigned long)desc + EQOS_DESCRIPTOR_SIZE,
				    ARCH_DMA_MINALIGN);

	flush_dcache_range(start, end);
#endif
}


static void eqos_inval_buffer_ax620e(void *buf, size_t size)
{
	unsigned long start = rounddown((unsigned long)buf, ARCH_DMA_MINALIGN);
	unsigned long end = roundup((unsigned long)buf + size,
				    ARCH_DMA_MINALIGN);

	invalidate_dcache_range(start, end);
}


static void eqos_flush_buffer_ax620e(void *buf, size_t size)
{
	unsigned long start = rounddown((unsigned long)buf, ARCH_DMA_MINALIGN);
	unsigned long end = roundup((unsigned long)buf + size,
				    ARCH_DMA_MINALIGN);

	flush_dcache_range(start, end);
}


static int eqos_mdio_wait_idle(struct eqos_priv *eqos)
{
	return wait_for_bit_le32(&eqos->mac_regs->mdio_address,
				 EQOS_MAC_MDIO_ADDRESS_GB, false,
				 1000000, true);
}


static int eqos_mdio_read(struct mii_dev *bus, int mdio_addr, int mdio_devad,
			  int mdio_reg)
{
	struct eqos_priv *eqos = bus->priv;
	u32 val;
	int ret;

	debug("%s(dev=%p, addr=%x, reg=%d):\n", __func__, eqos->dev, mdio_addr,
	      mdio_reg);

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		pr_err("MDIO not idle at entry");
		return ret;
	}

	val = readl(&eqos->mac_regs->mdio_address);
	val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
		EQOS_MAC_MDIO_ADDRESS_C45E;
	val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
		(mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
		(eqos->config->config_mac_mdio <<
		 EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
		(EQOS_MAC_MDIO_ADDRESS_GOC_READ <<
		 EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
		EQOS_MAC_MDIO_ADDRESS_GB;
	writel(val, &eqos->mac_regs->mdio_address);

#ifdef CONFIG_AXERA_EMAC_HAPS
	delay_us(eqos->config->mdio_wait);
#else
	udelay(eqos->config->mdio_wait);
#endif

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		pr_err("MDIO read didn't complete");
		return ret;
	}

	val = readl(&eqos->mac_regs->mdio_data);
	val &= EQOS_MAC_MDIO_DATA_GD_MASK;

	debug("%s: val=%x\n", __func__, val);

	return val;
}

static int eqos_mdio_write(struct mii_dev *bus, int mdio_addr, int mdio_devad,
			   int mdio_reg, u16 mdio_val)
{
	struct eqos_priv *eqos = bus->priv;
	u32 val;
	int ret;

	debug("%s(dev=%p, addr=%x, reg=%d, val=%x):\n", __func__, eqos->dev,
	      mdio_addr, mdio_reg, mdio_val);

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		pr_err("MDIO not idle at entry");
		return ret;
	}

	writel(mdio_val, &eqos->mac_regs->mdio_data);

	val = readl(&eqos->mac_regs->mdio_address);
	val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
		EQOS_MAC_MDIO_ADDRESS_C45E;
	val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
		(mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
		(eqos->config->config_mac_mdio <<
		 EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
		(EQOS_MAC_MDIO_ADDRESS_GOC_WRITE <<
		 EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
		EQOS_MAC_MDIO_ADDRESS_GB;
	writel(val, &eqos->mac_regs->mdio_address);

#ifdef CONFIG_AXERA_EMAC_HAPS
	delay_us(eqos->config->mdio_wait);
#else
	udelay(eqos->config->mdio_wait);
#endif

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		pr_err("MDIO read didn't complete");
		return ret;
	}

	return 0;
}

static int eqos_set_full_duplex(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s: dev=%s\n", __func__, dev->name);

	setbits_le32(&eqos->mac_regs->configuration, EQOS_MAC_CONFIGURATION_DM);

	return 0;
}


static int eqos_set_half_duplex(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s: dev=%s\n", __func__, dev->name);

	clrbits_le32(&eqos->mac_regs->configuration, EQOS_MAC_CONFIGURATION_DM);

	/* WAR: Flush TX queue when switching to half-duplex */
	setbits_le32(&eqos->mtl_regs->txq0_operation_mode,
		     EQOS_MTL_TXQ0_OPERATION_MODE_FTQ);

	return 0;
}


static int eqos_set_gmii_speed(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s: dev=%s\n", __func__, dev->name);

	clrbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);

	return 0;
}

static int eqos_set_mii_speed_100(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s: dev=%s\n", __func__, dev->name);

	setbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);

	return 0;
}

static int eqos_set_mii_speed_10(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s: dev=%s\n", __func__, dev->name);

	clrsetbits_le32(&eqos->mac_regs->configuration,
			EQOS_MAC_CONFIGURATION_FES, EQOS_MAC_CONFIGURATION_PS);

	return 0;
}


static int eqos_set_clk_speed_ax620e(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	if(eqos->interface == PHY_INTERFACE_MODE_RGMII){
		debug("adjust RGMII tx clock\n");
		emac_rgmii_set_tx_speed(eqos->id, eqos->phy->speed);
	} else if (eqos->interface == PHY_INTERFACE_MODE_RMII){
		debug("adjust RMII rx clock\n");
		emac_rmii_set_speed(eqos->id, eqos->phy->speed);
    }

	return 0;
}


static int eqos_adjust_link(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;
	static int old_speed = -1;

	debug("%s: dev=%s\n", __func__, dev->name);

	if (eqos->phy->duplex)
		ret = eqos_set_full_duplex(dev);
	else
		ret = eqos_set_half_duplex(dev);
	if (ret < 0) {
		pr_err("eqos_set_*_duplex() failed: %d\n", ret);
		return ret;
	}

#ifdef CONFIG_AXERA_EMAC_HAPS
/*in snps s80 haps platform, we need set controller 1000MHz and PHY 100MHz*/
	if(eqos->interface == PHY_INTERFACE_MODE_GMII)
		eqos->phy->speed = SPEED_1000;
#endif

	if(old_speed != eqos->phy->speed){
		printf("link speed:%dMbps, duplex:%s\n", eqos->phy->speed,
	    	eqos->phy->duplex ? "full" : "half");
		old_speed = eqos->phy->speed;
	}

	switch (eqos->phy->speed) {
	case SPEED_1000:
		ret = eqos_set_gmii_speed(dev);
		break;
	case SPEED_100:
		ret = eqos_set_mii_speed_100(dev);
		break;
	case SPEED_10:
		ret = eqos_set_mii_speed_10(dev);
		break;
	default:
		pr_err("invalid speed %d\n", eqos->phy->speed);
		return -EINVAL;
	}
	if (ret < 0) {
		pr_err("eqos_set_*mii_speed*() failed: %d\n", ret);
		return ret;
	}

	ret = eqos->config->ops->eqos_set_clk_speed(dev);
	if (ret < 0) {
		pr_err("eqos_set_clk_speed() failed: %d\n", ret);
		return ret;
	}

	return 0;
}


static int eqos_write_hwaddr(struct udevice *dev)
{
	struct eth_pdata *plat = dev_get_platdata(dev);
	struct eqos_priv *eqos = dev_get_priv(dev);
	uint32_t val;

	/*
	 * This function may be called before start() or after stop(). At that
	 * time, on at least some configurations of the EQoS HW, all clocks to
	 * the EQoS HW block will be stopped, and a reset signal applied. If
	 * any register access is attempted in this state, bus timeouts or CPU
	 * hangs may occur. This check prevents that.
	 *
	 * A simple solution to this problem would be to not implement
	 * write_hwaddr(), since start() always writes the MAC address into HW
	 * anyway. However, it is desirable to implement write_hwaddr() to
	 * support the case of SW that runs subsequent to U-Boot which expects
	 * the MAC address to already be programmed into the EQoS registers,
	 * which must happen irrespective of whether the U-Boot user (or
	 * scripts) actually made use of the EQoS device, and hence
	 * irrespective of whether start() was ever called.
	 *
	 * Note that this requirement by subsequent SW is not valid for
	 * Tegra186, and is likely not valid for any non-PCI instantiation of
	 * the EQoS HW block. This function is implemented solely as
	 * future-proofing with the expectation the driver will eventually be
	 * ported to some system where the expectation above is true.
	 */
	if (!eqos->config->reg_access_always_ok && !eqos->reg_access_ok)
		return 0;

	/* Update the MAC address */
	val = (plat->enetaddr[5] << 8) |
		(plat->enetaddr[4]);
	writel(val, &eqos->mac_regs->address0_high);
	val = (plat->enetaddr[3] << 24) |
		(plat->enetaddr[2] << 16) |
		(plat->enetaddr[1] << 8) |
		(plat->enetaddr[0]);
	writel(val, &eqos->mac_regs->address0_low);

	return 0;
}


static int eqos_start(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret = -1, i;
	ulong rate;
	u32 val, tx_fifo_sz, rx_fifo_sz, tqs, rqs, pbl;
	ulong last_rx_desc;

	debug("%s: dev=%s\n", __func__, dev->name);

#ifndef CONFIG_SYS_NONCACHED_MEMORY
	debug("emac dma use noncached memory\n");
#ifndef CONFIG_AXERA_EMAC_USE_CACHED_DESC
	// disable cache for dma desc
	debug("disable d-cache for emac\n");
	dcache_disable();
#else
	// driver support cache operation for dma desc now!
	debug("don't disable d-cache for transfer\n");
#endif
#endif

	eqos->tx_desc_idx = 0;
	eqos->rx_desc_idx = 0;


	rate = eqos->config->ops->eqos_get_tick_clk_rate(dev);
	eqos->bus_clock = rate;
	debug("emac bus clock: %ldMhz\n", rate/1000000);
	val = (rate / 1000000) - 1;
	writel(val, &eqos->mac_regs->us_tic_counter);

	/*
	 * if PHY was already connected and configured,
	 * don't need to reconnect/reconfigure again
	 */
	if (!eqos->phy) {
		eqos->phy = phy_connect(eqos->mii, -1, dev,
					eqos->config->interface(dev));
		if (!eqos->phy) {
			pr_err("phy_connect() failed\n");
			goto err_stop_clks;
		}

		ret = phy_config(eqos->phy);
		if (ret < 0) {
			pr_err("phy_config() failed: %d\n", ret);
			goto err_shutdown_phy;
		}
	}

	ret = phy_startup(eqos->phy);
	if (ret < 0) {
		pr_err("phy_startup() failed: %d\n", ret);
		goto err_shutdown_phy;
	}

	if (!eqos->phy->link) {
		pr_err("No link\n");
		goto err_shutdown_phy;
	}
	eqos->reg_access_ok = true;
	ret = wait_for_bit_le32(&eqos->dma_regs->mode,
				EQOS_DMA_MODE_SWR, false,
				eqos->config->swr_wait, false);
	if (ret) {
		pr_err("EQOS_DMA_MODE_SWR stuck\n");
		goto err_stop_clks;
	}

	ret = eqos_adjust_link(dev);
	if (ret < 0) {
		pr_err("eqos_adjust_link() failed: %d\n", ret);
		goto err_shutdown_phy;
	}

	/* Configure MTL */

	/* Enable Store and Forward mode for TX */
	/* Program Tx operating mode */
	setbits_le32(&eqos->mtl_regs->txq0_operation_mode,
		     EQOS_MTL_TXQ0_OPERATION_MODE_TSF |
		     (EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED <<
		      EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT));

	/* Transmit Queue weight */
	writel(0x10, &eqos->mtl_regs->txq0_quantum_weight);

	/* Enable Store and Forward mode for RX, since no jumbo frame */
	setbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
		     EQOS_MTL_RXQ0_OPERATION_MODE_RSF);

	/* Transmit/Receive queue fifo size; use all RAM for 1 queue */
	val = readl(&eqos->mac_regs->hw_feature1);
	tx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT) &
		EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK;
	rx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT) &
		EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK;

	/*
	 * r/tx_fifo_sz is encoded as log2(n / 128). Undo that by shifting.
	 * r/tqs is encoded as (n / 256) - 1.
	 */
	tqs = (128 << tx_fifo_sz) / 256 - 1;
	rqs = (128 << rx_fifo_sz) / 256 - 1;

	clrsetbits_le32(&eqos->mtl_regs->txq0_operation_mode,
			EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK <<
			EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT,
			tqs << EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT);
	clrsetbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
			EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK <<
			EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT,
			rqs << EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT);

	/* Flow control used only if each channel gets 4KB or more FIFO */
	if (rqs >= ((4096 / 256) - 1)) {
		u32 rfd, rfa;

		setbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
			     EQOS_MTL_RXQ0_OPERATION_MODE_EHFC);

		/*
		 * Set Threshold for Activating Flow Contol space for min 2
		 * frames ie, (1500 * 1) = 1500 bytes.
		 *
		 * Set Threshold for Deactivating Flow Contol for space of
		 * min 1 frame (frame size 1500bytes) in receive fifo
		 */
		if (rqs == ((4096 / 256) - 1)) {
			/*
			 * This violates the above formula because of FIFO size
			 * limit therefore overflow may occur inspite of this.
			 */
			rfd = 0x3;	/* Full-3K */
			rfa = 0x1;	/* Full-1.5K */
		} else if (rqs == ((8192 / 256) - 1)) {
			rfd = 0x6;	/* Full-4K */
			rfa = 0xa;	/* Full-6K */
		} else if (rqs == ((16384 / 256) - 1)) {
			rfd = 0x6;	/* Full-4K */
			rfa = 0x12;	/* Full-10K */
		} else {
			rfd = 0x6;	/* Full-4K */
			rfa = 0x1E;	/* Full-16K */
		}

		clrsetbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
				(EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
				(EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT),
				(rfd <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
				(rfa <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT));
	}

	/* Configure MAC */
	clrsetbits_le32(&eqos->mac_regs->rxq_ctrl0,
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_MASK <<
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT,
			eqos->config->config_mac <<
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT);

	/* Set TX flow control parameters */
	/* Set Pause Time */
	setbits_le32(&eqos->mac_regs->q0_tx_flow_ctrl,
		     0xffff << EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);
	/* Assign priority for TX flow control */
	clrbits_le32(&eqos->mac_regs->txq_prty_map0,
		     EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK <<
		     EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT);
	/* Assign priority for RX flow control */
	clrbits_le32(&eqos->mac_regs->rxq_ctrl2,
		     EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK <<
		     EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT);
	/* Enable flow control */
	setbits_le32(&eqos->mac_regs->q0_tx_flow_ctrl,
		     EQOS_MAC_Q0_TX_FLOW_CTRL_TFE);
	setbits_le32(&eqos->mac_regs->rx_flow_ctrl,
		     EQOS_MAC_RX_FLOW_CTRL_RFE);

	clrsetbits_le32(&eqos->mac_regs->configuration,
			EQOS_MAC_CONFIGURATION_GPSLCE |
			EQOS_MAC_CONFIGURATION_WD |
			EQOS_MAC_CONFIGURATION_JD |
			EQOS_MAC_CONFIGURATION_JE,
			EQOS_MAC_CONFIGURATION_CST |
			EQOS_MAC_CONFIGURATION_ACS);

	eqos_write_hwaddr(dev);

	/* Configure DMA */
	/* Enable OSP mode */
	setbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_OSP);

	/* RX buffer size. Must be a multiple of bus width */
	clrsetbits_le32(&eqos->dma_regs->ch0_rx_control,
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_MASK <<
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT,
			EQOS_MAX_PACKET_SIZE <<
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT);

	setbits_le32(&eqos->dma_regs->ch0_control,
		     EQOS_DMA_CH0_CONTROL_PBLX8);

	/*
	 * Burst length must be < 1/2 FIFO size.
	 * FIFO size in tqs is encoded as (n / 256) - 1.
	 * Each burst is n * 8 (PBLX8) * 16 (AXI width) == 128 bytes.
	 * Half of n * 256 is n * 128, so pbl == tqs, modulo the -1.
	 */
	pbl = tqs + 1;
	if (pbl > 32)
		pbl = 32;
	clrsetbits_le32(&eqos->dma_regs->ch0_tx_control,
			EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK <<
			EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT,
			pbl << EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT);

	clrsetbits_le32(&eqos->dma_regs->ch0_rx_control,
			EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK <<
			EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT,
			8 << EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);

	/* DMA performance configuration */
	val = (2 << EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
		EQOS_DMA_SYSBUS_MODE_EAME | EQOS_DMA_SYSBUS_MODE_BLEN16 |
		EQOS_DMA_SYSBUS_MODE_BLEN8 | EQOS_DMA_SYSBUS_MODE_BLEN4;
	writel(val, &eqos->dma_regs->sysbus_mode);

	/* Set up descriptors */
	memset(eqos->descs, 0, EQOS_DESCRIPTORS_SIZE);
	for (i = 0; i < EQOS_DESCRIPTORS_RX; i++) {
		struct eqos_desc *rx_desc = &(eqos->rx_descs[i]);
#ifdef CONFIG_AXERA_EMAC
		rx_desc->des0 = (u32)(ulong)(eqos->rx_dma_buf +
					     (i * EQOS_MAX_PACKET_SIZE));
		rx_desc->des1 = ((ulong)(eqos->rx_dma_buf) >> 32) & 0xffff;
		debug("rx_desc[%d]->des0: %x\n", i, rx_desc->des0);
		debug("rx_desc[%d]->des1: %x\n", i, rx_desc->des1);
#else
		rx_desc->des0 = (u32)(ulong)(eqos->rx_dma_buf +
					     (i * EQOS_MAX_PACKET_SIZE));
#endif
		rx_desc->des3 |= EQOS_DESC3_OWN | EQOS_DESC3_BUF1V;
		eqos->config->ops->eqos_flush_desc(rx_desc);
	}

#ifdef CONFIG_AXERA_EMAC
	writel((ulong)eqos->tx_descs >> 32, &eqos->dma_regs->ch0_txdesc_list_haddress);
	writel((u32)(ulong)eqos->tx_descs, &eqos->dma_regs->ch0_txdesc_list_address);
	debug("tx_desc_h_addr_reg: %p, value: %x\n",  &eqos->dma_regs->ch0_txdesc_list_haddress, (u32)((ulong)eqos->tx_descs >> 32));
	debug("tx_desc_addr_reg: %p, value: %x\n",  &eqos->dma_regs->ch0_txdesc_list_address, (u32)(ulong)eqos->tx_descs);
#else
	writel(0, &eqos->dma_regs->ch0_txdesc_list_haddress);
	writel((ulong)eqos->tx_descs, &eqos->dma_regs->ch0_txdesc_list_address);
#endif
	writel(EQOS_DESCRIPTORS_TX - 1,
	       &eqos->dma_regs->ch0_txdesc_ring_length);

#ifdef CONFIG_AXERA_EMAC
	writel((ulong)eqos->rx_descs >> 32, &eqos->dma_regs->ch0_rxdesc_list_haddress);
	writel((u32)(ulong)eqos->rx_descs, &eqos->dma_regs->ch0_rxdesc_list_address);
	debug("rx_desc_h_addr_reg: %p, value: %x\n",  &eqos->dma_regs->ch0_rxdesc_list_haddress, (u32)((ulong)eqos->rx_descs >> 32));
	debug("rx_desc_addr_reg: %p, value: %x\n",  &eqos->dma_regs->ch0_rxdesc_list_address, (u32)(ulong)eqos->rx_descs);
#else
	writel(0, &eqos->dma_regs->ch0_rxdesc_list_haddress);
	writel((ulong)eqos->rx_descs, &eqos->dma_regs->ch0_rxdesc_list_address);
#endif
	writel(EQOS_DESCRIPTORS_RX - 1,
	       &eqos->dma_regs->ch0_rxdesc_ring_length);

	/* Enable everything */
	setbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_TE | EQOS_MAC_CONFIGURATION_RE);

	setbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_ST);
	setbits_le32(&eqos->dma_regs->ch0_rx_control,
		     EQOS_DMA_CH0_RX_CONTROL_SR);

	/* TX tail pointer not written until we need to TX a packet */
	/*
	 * Point RX tail pointer at last descriptor. Ideally, we'd point at the
	 * first descriptor, implying all descriptors were available. However,
	 * that's not distinguishable from none of the descriptors being
	 * available.
	 */
#ifdef CONFIG_AXERA_EMAC_USE_CACHED_DESC
	last_rx_desc = (ulong)&(eqos->rx_descs[(1)]);
	writel(last_rx_desc, &eqos->dma_regs->ch0_rxdesc_tail_pointer);	//only enable one rx transfer
#else
	last_rx_desc = (ulong)&(eqos->rx_descs[(EQOS_DESCRIPTORS_RX - 1)]);
	writel(last_rx_desc, &eqos->dma_regs->ch0_rxdesc_tail_pointer);
#endif

	eqos->started = true;

	debug("%s: OK\n", __func__);
	return 0;

err_shutdown_phy:
	phy_shutdown(eqos->phy);
err_stop_clks:
	eqos->config->ops->eqos_stop_clks(dev);
	pr_err("FAILED: %d\n", ret);
	return ret;
}


static void eqos_stop(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int i;

	debug("%s: dev=%s\n", __func__, dev->name);

	if (!eqos->started)
		return;

	eqos->started = false;
	eqos->reg_access_ok = false;

	/* Disable TX DMA */
	clrbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_ST);

	/* Wait for TX all packets to drain out of MTL */
	for (i = 0; i < 1000000; i++) {
		u32 val = readl(&eqos->mtl_regs->txq0_debug);
		u32 trcsts = (val >> EQOS_MTL_TXQ0_DEBUG_TRCSTS_SHIFT) &
			EQOS_MTL_TXQ0_DEBUG_TRCSTS_MASK;
		u32 txqsts = val & EQOS_MTL_TXQ0_DEBUG_TXQSTS;
		if ((trcsts != 1) && (!txqsts))
			break;
	}

	/* Turn off MAC TX and RX */
	clrbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_TE | EQOS_MAC_CONFIGURATION_RE);

	/* Wait for all RX packets to drain out of MTL */
	for (i = 0; i < 1000000; i++) {
		u32 val = readl(&eqos->mtl_regs->rxq0_debug);
		u32 prxq = (val >> EQOS_MTL_RXQ0_DEBUG_PRXQ_SHIFT) &
			EQOS_MTL_RXQ0_DEBUG_PRXQ_MASK;
		u32 rxqsts = (val >> EQOS_MTL_RXQ0_DEBUG_RXQSTS_SHIFT) &
			EQOS_MTL_RXQ0_DEBUG_RXQSTS_MASK;
		if ((!prxq) && (!rxqsts))
			break;
	}

	/* Turn off RX DMA */
	clrbits_le32(&eqos->dma_regs->ch0_rx_control,
		     EQOS_DMA_CH0_RX_CONTROL_SR);

	if (eqos->phy) {
		phy_shutdown(eqos->phy);
	}

	eqos->config->ops->eqos_stop_clks(dev);

#ifndef CONFIG_SYS_NONCACHED_MEMORY
#ifndef CONFIG_AXERA_EMAC_USE_CACHED_DESC
	debug("enable dcache for emac\n");
	dcache_enable();
#endif
#endif

	debug("%s: OK\n", __func__);
}


static int eqos_send(struct udevice *dev, void *packet, int length)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	struct eqos_desc *tx_desc;
	int i;

	debug("%s(dev=%s, packet=%p, length=%d):\n", __func__, dev->name, packet,
	      length);

	memcpy(eqos->tx_dma_buf, packet, length);
	eqos->config->ops->eqos_flush_buffer(eqos->tx_dma_buf, length);

	tx_desc = &(eqos->tx_descs[eqos->tx_desc_idx]);
	eqos->tx_desc_idx++;
	eqos->tx_desc_idx %= EQOS_DESCRIPTORS_TX;

#ifdef CONFIG_AXERA_EMAC
	tx_desc->des0 = (u32)(u64)eqos->tx_dma_buf & 0xffffffff;
	tx_desc->des1 = ((u64)eqos->tx_dma_buf >> 32) & 0xffff;
#else
	tx_desc->des0 = (ulong)eqos->tx_dma_buf;
	tx_desc->des1 = 0;
#endif
	tx_desc->des2 = length;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	mb();
	tx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_FD | EQOS_DESC3_LD | length;
	eqos->config->ops->eqos_flush_desc(tx_desc);

	writel((ulong)(tx_desc + 1), &eqos->dma_regs->ch0_txdesc_tail_pointer);

	for (i = 0; i < 1000000; i++) {
		eqos->config->ops->eqos_inval_desc(tx_desc);
		if (!(readl(&tx_desc->des3) & EQOS_DESC3_OWN))
			return 0;
#ifdef CONFIG_AXERA_EMAC_HAPS
		delay_us(1);
#else
		udelay(1);
#endif
	}

	printf("%s: TX timeout\n", __func__);

	return -ETIMEDOUT;
}


static int eqos_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	struct eqos_desc *rx_desc;
	int length;

	debug("%s(dev=%p, flags=%x):\n", __func__, dev, flags);

	rx_desc = &(eqos->rx_descs[eqos->rx_desc_idx]);
	eqos->config->ops->eqos_inval_desc(rx_desc);	//invalidate operation is necessary!!
	if (rx_desc->des3 & EQOS_DESC3_OWN) {
		debug("%s: RX packet not available\n", __func__);
		return -EAGAIN;
	}

	*packetp = eqos->rx_dma_buf +
		(eqos->rx_desc_idx * EQOS_MAX_PACKET_SIZE);
	length = rx_desc->des3 & 0x7fff;
	debug("%s: *packetp=%p, length=%d\n", __func__, *packetp, length);

	eqos->config->ops->eqos_inval_buffer(*packetp, EQOS_MAX_PACKET_SIZE);
	return length;
}


static int eqos_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	uchar *packet_expected;
	struct eqos_desc *rx_desc;
#ifdef CONFIG_AXERA_EMAC_USE_CACHED_DESC
	uint32_t rxdesc_tail, current_rx_desc;
#endif

	debug("%s(packet=%p, length=%d)\n", __func__, packet, length);

	packet_expected = eqos->rx_dma_buf +
		(eqos->rx_desc_idx * EQOS_MAX_PACKET_SIZE);
	if (packet != packet_expected) {
		debug("%s: Unexpected packet (expected %p)\n", __func__,
		      packet_expected);
		return -EINVAL;
	}

	rx_desc = &(eqos->rx_descs[eqos->rx_desc_idx]);
#ifdef CONFIG_AXERA_EMAC
	rx_desc->des0 = (u32)(u64)packet & 0xffffffff;
	rx_desc->des1 = ((u64)packet >> 32) & 0xffff;
#else
	rx_desc->des0 = (u32)(ulong)packet;
	rx_desc->des1 = 0;
#endif
	rx_desc->des2 = 0;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	mb();
	rx_desc->des3 |= EQOS_DESC3_OWN | EQOS_DESC3_BUF1V;
	eqos->config->ops->eqos_flush_desc(rx_desc);

#ifdef CONFIG_AXERA_EMAC_USE_CACHED_DESC
	// to support rx desc cached operation, we enable one rx desc to transfer.
	// if we enable more than one rx desc, flush operation will efffect them.
	current_rx_desc = readl(&eqos->dma_regs->ch9_current_app_rx_desc);
	if(current_rx_desc == ((ulong)&(eqos->rx_descs[3]) & 0xffffffff) )
		rxdesc_tail = (ulong)&(eqos->rx_descs[0]);
	else
		rxdesc_tail = current_rx_desc + EQOS_DESCRIPTOR_SIZE;

	writel(rxdesc_tail, &eqos->dma_regs->ch0_rxdesc_tail_pointer);
#else
	writel((ulong)rx_desc, &eqos->dma_regs->ch0_rxdesc_tail_pointer);
#endif

	eqos->rx_desc_idx++;
	eqos->rx_desc_idx %= EQOS_DESCRIPTORS_RX;

	return 0;
}


static int eqos_probe_resources_core(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;

	debug("%s: dev=%s\n", __func__, dev->name);

	eqos->descs = eqos_alloc_descs(EQOS_DESCRIPTORS_TX +
				       EQOS_DESCRIPTORS_RX);
	if (!eqos->descs) {
		debug("%s: eqos_alloc_descs() failed\n", __func__);
		ret = -ENOMEM;
		goto err;
	}
	eqos->tx_descs = (struct eqos_desc *)eqos->descs;
	eqos->rx_descs = (eqos->tx_descs + EQOS_DESCRIPTORS_TX);
	debug("%s: tx_descs=%p, rx_descs=%p\n", __func__, eqos->tx_descs,
	      eqos->rx_descs);

	eqos->tx_dma_buf = memalign(EQOS_BUFFER_ALIGN, EQOS_MAX_PACKET_SIZE);
	if (!eqos->tx_dma_buf) {
		debug("%s: memalign(tx_dma_buf) failed\n", __func__);
		ret = -ENOMEM;
		goto err_free_descs;
	}
	debug("%s: tx_dma_buf=%p\n", __func__, eqos->tx_dma_buf);

	eqos->rx_dma_buf = memalign(EQOS_BUFFER_ALIGN, EQOS_RX_BUFFER_SIZE);
	if (!eqos->rx_dma_buf) {
		debug("%s: memalign(rx_dma_buf) failed\n", __func__);
		ret = -ENOMEM;
		goto err_free_tx_dma_buf;
	}
	debug("%s: rx_dma_buf=%p\n", __func__, eqos->rx_dma_buf);

	eqos->rx_pkt = malloc(EQOS_MAX_PACKET_SIZE);
	if (!eqos->rx_pkt) {
		debug("%s: malloc(rx_pkt) failed\n", __func__);
		ret = -ENOMEM;
		goto err_free_rx_dma_buf;
	}
	debug("%s: rx_pkt=%p\n", __func__, eqos->rx_pkt);

	debug("%s: OK\n", __func__);
	return 0;

err_free_rx_dma_buf:
	free(eqos->rx_dma_buf);
err_free_tx_dma_buf:
	free(eqos->tx_dma_buf);
err_free_descs:
	eqos_free_descs(eqos->descs);
err:

	debug("%s: returns %d\n", __func__, ret);
	return ret;
}


static int eqos_remove_resources_core(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s: dev=%s\n", __func__, dev->name);

	free(eqos->rx_pkt);
	free(eqos->rx_dma_buf);
	free(eqos->tx_dma_buf);
	eqos_free_descs(eqos->descs);

	debug("%s: OK\n", __func__);
	return 0;
}


/* board-specific Ethernet Interface initializations. */
__weak int board_interface_eth_init(struct udevice *dev,
				    phy_interface_t interface_type)
{
	select_phy_interface(dev, interface_type);
	return 0;
}


static int eqos_probe_resources_ax620e(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;
	phy_interface_t interface;

	eqos->id = dev_read_u32_default(dev, "id", -1);
	if(eqos->id == -1){
		pr_err("read id from dts failed");
		return -1;
	}

	eqos->rmii_loopback_mode = dev_read_bool(dev, "axera,rmii_loopback");
	eqos->out_rmii_mode = dev_read_bool(dev, "axera,out_rmii");

	ret = gpio_request_by_name(dev, "phy-rst-gpio", 0,
				   &eqos->phy_reset_gpio,
				   GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	if (ret) {
		pr_err("phy-rst-gpio not configure\n");
	}

	interface = eqos->config->interface(dev);
	eqos->interface = interface;
	if (interface == PHY_INTERFACE_MODE_NONE) {
		pr_err("Invalid PHY interface\n");
		return -EINVAL;
	}

	ret = board_interface_eth_init(dev, interface);
	if (ret)
		return -EINVAL;

#ifdef USE_CLK_FROM_DTS
	ret = clk_get_by_name(dev, "stmmaceth", &eqos->clk_master_bus);
	if (ret) {
		pr_err("clk_get_by_name(master_bus) failed: %d\n", ret);
		return ret;
	}
#endif

	return 0;
}

static phy_interface_t eqos_get_interface_ax620e(struct udevice *dev)
{
	const char *phy_mode;
	phy_interface_t interface = PHY_INTERFACE_MODE_NONE;

	debug("%s: dev=%s\n", __func__, dev->name);

	phy_mode = fdt_getprop(gd->fdt_blob, dev_of_offset(dev), "phy-mode",
			       NULL);
	if (phy_mode)
		interface = phy_get_interface_by_name(phy_mode);

	debug("\n%s phy-mode: %s, %d\n", dev->name, phy_mode, interface);
	return interface;
}


static int eqos_remove_resources_ax620e(struct udevice *dev)
{
#ifdef USE_CLK_FROM_DTS
	struct eqos_priv *eqos = dev_get_priv(dev);

	clk_free(&eqos->clk_master_bus);
#endif

	return 0;
}


static int eqos_probe(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);
	int ret;

	//printf("\n------------eqos_probe: %s----------------\n", dev->name);

	eqos->dev = dev;
	eqos->config = (void *)dev_get_driver_data(dev);

	eqos->regs = devfdt_get_addr(dev);
	if (eqos->regs == FDT_ADDR_T_NONE) {
		pr_err("devfdt_get_addr() failed");
		return -ENODEV;
	}

	debug("\n%s: reg base addr:%x \n", dev->name, (unsigned int)eqos->regs);

	eqos->mac_regs = (void *)(eqos->regs + EQOS_MAC_REGS_BASE);
	eqos->mtl_regs = (void *)(eqos->regs + EQOS_MTL_REGS_BASE);
	eqos->dma_regs = (void *)(eqos->regs + EQOS_DMA_REGS_BASE);

	ret = eqos_probe_resources_core(dev);
	if (ret < 0) {
		pr_err("eqos_probe_resources_core() failed: %d\n", ret);
		return ret;
	}

	ret = eqos->config->ops->eqos_probe_resources(dev);
	if (ret < 0) {
		pr_err("eqos_probe_resources() failed: %d\n", ret);
		goto err_remove_resources_core;
	}

	eqos->mii = mdio_alloc();
	if (!eqos->mii) {
		pr_err("mdio_alloc() failed");
		ret = -ENOMEM;
		goto err_remove_resources_tegra;
	}
	eqos->mii->read = eqos_mdio_read;
	eqos->mii->write = eqos_mdio_write;
	eqos->mii->priv = eqos;
	strcpy(eqos->mii->name, dev->name);

	ret = mdio_register(eqos->mii);
	if (ret < 0) {
		pr_err("mdio_register() failed: %d\n", ret);
		goto err_free_mdio;
	}

	ret = eqos->config->ops->eqos_start_clks(dev);
	if (ret < 0) {
		pr_err("eqos_start_clks() failed: %d\n", ret);
		goto err_free_mdio;
	}


#ifdef CONFIG_AXERA_EMAC_HAPS
	delay_us(10);
#else
	udelay(10);
#endif
	ret = eqos->config->ops->eqos_start_resets(dev);
	if (ret < 0) {
		pr_err("eqos_start_resets() failed: %d\n", ret);
		goto err_free_mdio;
	}

	debug("%s: OK\n", __func__);
	return 0;

err_free_mdio:
	mdio_free(eqos->mii);
err_remove_resources_tegra:
	eqos->config->ops->eqos_remove_resources(dev);
err_remove_resources_core:
	eqos_remove_resources_core(dev);

	debug("%s: returns %d\n", __func__, ret);
	return ret;
}


static int eqos_remove(struct udevice *dev)
{
	struct eqos_priv *eqos = dev_get_priv(dev);

	debug("%s: dev=%s\n", __func__, dev->name);

	mdio_unregister(eqos->mii);
	mdio_free(eqos->mii);
	eqos->config->ops->eqos_remove_resources(dev);

	eqos_probe_resources_core(dev);

	debug("%s: OK\n", __func__);
	return 0;
}


static const struct eth_ops eqos_ops = {
	.start = eqos_start,
	.stop = eqos_stop,
	.send = eqos_send,
	.recv = eqos_recv,
	.free_pkt = eqos_free_pkt,
	.write_hwaddr = eqos_write_hwaddr,
};


static struct eqos_ops eqos_ax620e_ops = {
	.eqos_inval_desc = eqos_inval_desc_ax620e,
	.eqos_flush_desc = eqos_flush_desc_ax620e,
	.eqos_inval_buffer = eqos_inval_buffer_ax620e,
	.eqos_flush_buffer = eqos_flush_buffer_ax620e,
	.eqos_probe_resources = eqos_probe_resources_ax620e,
	.eqos_remove_resources = eqos_remove_resources_ax620e,
	.eqos_stop_resets = eqos_stop_resets_ax620e,
	.eqos_start_resets = eqos_start_resets_ax620e,
	.eqos_stop_clks = eqos_stop_clks_ax620e,
	.eqos_start_clks = eqos_start_clks_ax620e,
	.eqos_set_clk_speed = eqos_set_clk_speed_ax620e,
	.eqos_get_tick_clk_rate = eqos_get_tick_clk_rate_ax620e
};


static const struct eqos_config eqos_ax620e_config = {
	.reg_access_always_ok = false,
	.mdio_wait = 100,
	.swr_wait = 50,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV,
#ifdef CONFIG_AXERA_EMAC_HAPS
	.config_mac_mdio = EQOS_MAC_MDIO_ADDRESS_CR_60_100,
#else
	.config_mac_mdio = EQOS_MAC_MDIO_ADDRESS_CR_300_500,
#endif
	.interface = eqos_get_interface_ax620e,
	.ops = &eqos_ax620e_ops
};


static const struct udevice_id eqos_ids[] = {
	{
		.compatible = "axera,ax620e-eqos",
		.data = (ulong)&eqos_ax620e_config
	},

	{ }
};


U_BOOT_DRIVER(eth_eqos) = {
	.name = "eth_eqos",
	.id = UCLASS_ETH,
	.of_match = eqos_ids,
	.probe = eqos_probe,
	.remove = eqos_remove,
	.ops = &eqos_ops,
	.priv_auto_alloc_size = sizeof(struct eqos_priv),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
};
