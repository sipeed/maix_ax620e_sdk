// SPDX-License-Identifier: GPL-2.0
/*
 * AXERA master SPI core controller driver
 *
 * Copyright (c) 2020, AIXIN-Chip Corporation.
 */

#include <common.h>
#include <asm-generic/gpio.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <spi.h>
#include <fdtdec.h>
#include <reset.h>
#include <dm/device_compat.h>
#include <linux/compat.h>
#include <linux/iopoll.h>
#include <asm/io.h>
#include <asm/arch-axera/dma.h>
#include <asm/arch/ax620e.h>

/* Register offsets */
#define DW_SPI_CTRL0			0x00
#define DW_SPI_CTRL1			0x04
#define DW_SPI_SSIENR			0x08
#define DW_SPI_MWCR			0x0c
#define DW_SPI_SER			0x10
#define DW_SPI_BAUDR			0x14
#define DW_SPI_TXFLTR			0x18
#define DW_SPI_RXFLTR			0x1c
#define DW_SPI_TXFLR			0x20
#define DW_SPI_RXFLR			0x24
#define DW_SPI_SR			0x28
#define DW_SPI_IMR			0x2c
#define DW_SPI_ISR			0x30
#define DW_SPI_RISR			0x34
#define DW_SPI_TXOICR			0x38
#define DW_SPI_RXOICR			0x3c
#define DW_SPI_RXUICR			0x40
#define DW_SPI_MSTICR			0x44
#define DW_SPI_ICR			0x48
#define DW_SPI_DMACR			0x4c
#define DW_SPI_DMATDLR			0x50
#define DW_SPI_DMARDLR			0x54
#define DW_SPI_IDR			0x58
#define DW_SPI_VERSION			0x5c
#define DW_SPI_DR			0x60
#define DW_SPI_RX_SAMPLE_DLY		0xf0
#define DW_SPI_SPI_CTRL0		0xf4

/* Bit fields in CTRLR0 */
#define SPI_DFS_OFFSET			0

#define SPI_FRF_OFFSET			6
#define SPI_FRF_SPI			0x0
#define SPI_FRF_SSP			0x1
#define SPI_FRF_MICROWIRE		0x2
#define SPI_FRF_RESV			0x3

#define SPI_MODE_OFFSET			8
#define SPI_SCPH_OFFSET			8
#define SPI_SCOL_OFFSET			9

#define SPI_TMOD_OFFSET			10
#define SPI_TMOD_MASK			(0x3 << SPI_TMOD_OFFSET)
#define	SPI_TMOD_TR			0x0		/* xmit & recv */
#define SPI_TMOD_TO			0x1		/* xmit only */
#define SPI_TMOD_RO			0x2		/* recv only */
#define SPI_TMOD_EPROMREAD		0x3		/* eeprom read mode */

#define SPI_SLVOE_OFFSET		12
#define SPI_SRL_OFFSET			13
#define SPI_CFS_OFFSET			16

#define SPI_SPI_FRF_OFFSET		22
#define SPI_SPI_FRF_MASK		(3 << SPI_FRF_OFFSET)
#define SPI_FRF_SPI_STANDARD		0 /* 1 line */
#define SPI_FRF_SPI_DUAL		1 /* 2 line */
#define SPI_FRF_SPI_QUAD		2 /* 4 line */
#define SPI_FRF_SPI_OCTAL		3 /* 8 line */

/* Bit fields in SPI_CTRLR0 */
#define SPI_TRANS_TYPE_OFFSET		0
#define SPI_TRANS_TYPE_TT0		0 /* Instruction and Address will be sent in Standard SPI Mode */
#define SPI_TRANS_TYPE_TT1		1 /* Instruction will be sent in Standard SPI Mode and Address will be sent in the mode specified by CTRLR0.SPI_FRF */
#define SPI_TRANS_TYPE_TT2		2 /* Both Instruction and Address will be sent in the mode specified by SPI_FRF */
#define SPI_TRANS_TYPE_TT3		3 /* Reserved */

#define SPI_ADDR_L_OFFSET		2
#define SPI_ADDR_L8			2

#define SPI_CLK_STRETCH_EN_OFFSET	30
#define SPI_CLK_STRETCH_EN		1

#define SPI_TXFTHR_OFFSET		16
/* Bit fields in SR, 7 bits */
#define SR_MASK				GENMASK(6, 0)	/* cover 7 bits */
#define SR_BUSY				BIT(0)
#define SR_TF_NOT_FULL			BIT(1)
#define SR_TF_EMPT			BIT(2)
#define SR_RF_NOT_EMPT			BIT(3)
#define SR_RF_FULL			BIT(4)
#define SR_TX_ERR			BIT(5)
#define SR_DCOL				BIT(6)

#define RX_TIMEOUT			1000		/* timeout in ms */
#define DW_SPI_FIFO_LEN			64
#define SPI_MST_DMA_TXRX_DLR	(DW_SPI_FIFO_LEN >> 1) //half fifo depth
#define SPI_MST_USE_DMA

//#define AX_SPI_MSG_PRINT
#ifdef SPI_DUAL_CS
#define MAX_CS_COUNT		2
#ifdef AX_SPI_MSG_PRINT
#define GPIO1_BASE		(0x4801000)
#define GPIO2_BASE		(0x6000000)
#define GPIO1_A12_ADDR	(GPIO1_BASE + (12 + 1) * 4)
#define GPIO2_A29_ADDR	(GPIO2_BASE + (29 + 1) * 4)
#endif
#endif

struct dw_spi_platdata {
	s32 frequency;		/* Default clock frequency, -1 for none */
	void __iomem *regs;
};

struct dw_spi_priv {
	void __iomem *regs;
	unsigned int freq;		/* Default frequency */
	unsigned int mode;
	struct clk clk;
	unsigned long bus_clk_rate;

#ifdef SPI_DUAL_CS
	struct gpio_desc cs_gpio[MAX_CS_COUNT];	/* External chip-select gpio */
#else
	struct gpio_desc cs_gpio;	/* External chip-select gpio */
#endif

	int bits_per_word;
	u8 cs;			/* chip select pin */
	u8 tmode;		/* TR/TO/RO/EEPROM */
	u8 type;		/* SPI/SSP/MicroWire */
	int len;

	u32 fifo_len;		/* depth of the FIFO buffer */
	void *tx;
	void *tx_end;
	void *rx;
	void *rx_end;

	struct reset_ctl_bulk	resets;
};

static inline u32 dw_read(struct dw_spi_priv *priv, u32 offset)
{
	return __raw_readl(priv->regs + offset);
}

static inline void dw_write(struct dw_spi_priv *priv, u32 offset, u32 val)
{
	__raw_writel(val, priv->regs + offset);
}

static int request_gpio_cs(struct udevice *bus)
{
#if CONFIG_IS_ENABLED(DM_GPIO) && !defined(CONFIG_SPL_BUILD)
	struct dw_spi_priv *priv = dev_get_priv(bus);
	int ret;
#ifdef SPI_DUAL_CS
	int i;

	/* External chip select gpio line is optional */
	ret = gpio_request_list_by_name(bus, "cs-gpio", priv->cs_gpio, ARRAY_SIZE(priv->cs_gpio), 0);
	if (ret < 0) {
		pr_err("Can't get %s gpios! Error: %d", bus->name, ret);
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(priv->cs_gpio); i++) {
		if (!dm_gpio_is_valid(&priv->cs_gpio[i]))
			continue;

		dm_gpio_set_dir_flags(&priv->cs_gpio[i],
				      GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	}
#else
	/* External chip select gpio line is optional */
	ret = gpio_request_by_name(bus, "cs-gpio", 0, &priv->cs_gpio, 0);
	if (ret == -ENOENT)
		return 0;

	if (ret < 0) {
		printf("Error: %d: Can't get %s gpio!\n", ret, bus->name);
		return ret;
	}

	if (dm_gpio_is_valid(&priv->cs_gpio)) {
		dm_gpio_set_dir_flags(&priv->cs_gpio,
				      GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	}

	debug("%s: used external gpio for CS management\n", __func__);
#endif
#endif
	return 0;
}

static int dw_spi_ofdata_to_platdata(struct udevice *bus)
{
	struct dw_spi_platdata *plat = bus->platdata;

	plat->regs = (struct dw_spi *)devfdt_get_addr(bus);

	/* Use 500KHz as a suitable default */
	plat->frequency = dev_read_u32_default(bus, "spi-max-frequency",
					       500000);
	debug("%s: regs=%p max-frequency=%d\n", __func__, plat->regs,
	      plat->frequency);

	return request_gpio_cs(bus);
}

static inline void spi_enable_chip(struct dw_spi_priv *priv, int enable)
{
	dw_write(priv, DW_SPI_SSIENR, (enable ? 1 : 0));
}

#define CLK_H_SSI_SEL_OFFSET    (7)
#define CLK_H_SSI_SEL_MASK      (0x7 << CLK_H_SSI_SEL_OFFSET)
#define CLK_H_SSI_SEL_CPLL_24M  (0x0)
#define CLK_H_SSI_SEL_EPLL_125M (0x1)
#define CLK_H_SSI_SEL_CPLL_208M (0x2)
#define CLK_H_SSI_SEL_CPLL_312M (0x3)
#define CLK_H_SSI_SEL_NPLL_400M (0x4)
#define CLK_H_SSI_SEL_CPLL_416M (0x5)
#define CLK_H_SSI_SEL_MAX       (0x6)
static void clk_h_ssi_sel(u8 sel)
{
	u8 val = ((sel >= CLK_H_SSI_SEL_MAX) ? CLK_H_SSI_SEL_CPLL_24M : sel);

	if (val == ((readl(CPU_SYS_GLB_CLK_MUX0) & CLK_H_SSI_SEL_MASK) >> CLK_H_SSI_SEL_OFFSET))
		return;

	// clk_h_ssi_eb clr
	writel(BIT(3), CPU_SYS_GLB_CLK_EB0_CLR);
	udelay(1);
	writel(CLK_H_SSI_SEL_MASK, CPU_SYS_GLB_CLK_MUX0_CLR);
	writel((val << CLK_H_SSI_SEL_OFFSET), CPU_SYS_GLB_CLK_MUX0_SET);

	// clk_h_ssi_eb set
	writel(BIT(3), CPU_SYS_GLB_CLK_EB0_SET);
}

/* Restart the controller, disable all interrupts, clean rx fifo */
static void spi_hw_init(struct dw_spi_priv *priv)
{
	clk_h_ssi_sel(CLK_H_SSI_SEL_CPLL_416M);
	spi_enable_chip(priv, 0);
	dw_write(priv, DW_SPI_IMR, 0x00);
#ifdef CONFIG_TARGET_AX620E_HAPS
	dw_write(priv, DW_SPI_SPI_CTRL0, 0x00);
#endif
	spi_enable_chip(priv, 1);

	/*
	 * Try to detect the FIFO depth if not set by interface driver,
	 * the depth could be from 2 to 256 from HW spec
	 */
	if (!priv->fifo_len) {
		u32 fifo;

		for (fifo = 1; fifo < 256; fifo++) {
			dw_write(priv, DW_SPI_TXFLTR, fifo);
			if (fifo != dw_read(priv, DW_SPI_TXFLTR))
				break;
		}

		priv->fifo_len = (fifo == 1) ? 0 : fifo;
		dw_write(priv, DW_SPI_TXFLTR, 0);
	}
	debug("%s: fifo_len=%d\n", __func__, priv->fifo_len);
}

/*
 * We define dw_spi_get_clk function as 'weak' as some targets
 * (like SOCFPGA_GEN5 and SOCFPGA_ARRIA10) don't use standard clock API
 * and implement dw_spi_get_clk their own way in their clock manager.
 */
__weak int dw_spi_get_clk(struct udevice *bus, ulong *rate)
{
	struct dw_spi_priv *priv = dev_get_priv(bus);
	int ret;

	ret = clk_get_by_index(bus, 0, &priv->clk);
	if (ret)
		return ret;

	ret = clk_enable(&priv->clk);
	if (ret && ret != -ENOSYS && ret != -ENOTSUPP)
		return ret;

	*rate = clk_get_rate(&priv->clk);
	if (!*rate)
		goto err_rate;

	debug("%s: get spi controller clk via device tree: %lu Hz\n",
	      __func__, *rate);

	return 0;

err_rate:
	clk_disable(&priv->clk);
	clk_free(&priv->clk);

	return -EINVAL;
}

static int dw_spi_reset(struct udevice *bus)
{
	int ret;
	struct dw_spi_priv *priv = dev_get_priv(bus);

	ret = reset_get_bulk(bus, &priv->resets);
	if (ret) {
		/*
		 * Return 0 if error due to !CONFIG_DM_RESET and reset
		 * DT property is not present.
		 */
		if (ret == -ENOENT || ret == -ENOTSUPP)
			return 0;

		dev_warn(bus, "Can't get reset: %d\n", ret);
		return ret;
	}

	ret = reset_deassert_bulk(&priv->resets);
	if (ret) {
		reset_release_bulk(&priv->resets);
		dev_err(bus, "Failed to reset: %d\n", ret);
		return ret;
	}

	return 0;
}

static int dw_spi_child_pre_probe(struct udevice *dev)
{
	struct spi_slave *slave = dev_get_parent_priv(dev);

	/* max read/write sizes */
#ifdef SPI_MST_USE_DMA
	slave->max_read_size = 32 << 10;
	slave->max_write_size = 32 << 10;
#else
	slave->max_read_size = 64;
	slave->max_write_size = 64;
#endif

	return 0;
}

static int dw_spi_probe(struct udevice *bus)
{
	struct dw_spi_platdata *plat = dev_get_platdata(bus);
	struct dw_spi_priv *priv = dev_get_priv(bus);
	int ret;

	priv->regs = plat->regs;
	priv->freq = plat->frequency;

	ret = dw_spi_get_clk(bus, &priv->bus_clk_rate);
	if (ret)
		return ret;

	ret = dw_spi_reset(bus);
	if (ret)
		return ret;

	/* Currently only bits_per_word == 8 supported */
	priv->bits_per_word = 8;

	priv->tmode = 0; /* Tx & Rx */

	/* Basic HW init */
	spi_hw_init(priv);

	return 0;
}

/* Return the max entries we can fill into tx fifo */
static inline u32 tx_max(struct dw_spi_priv *priv)
{
#if 1 /* ENABLE_QPI */
	u32 tx_left, tx_room;
	tx_left = (priv->tx_end - priv->tx) / (priv->bits_per_word >> 3);
	tx_room = priv->fifo_len - dw_read(priv, DW_SPI_TXFLR);

	return min_t(u32, tx_left, tx_room);
#else
	u32 tx_left, tx_room, rxtx_gap;

	tx_left = (priv->tx_end - priv->tx) / (priv->bits_per_word >> 3);
	tx_room = priv->fifo_len - dw_read(priv, DW_SPI_TXFLR);

	/*
	 * Another concern is about the tx/rx mismatch, we
	 * thought about using (priv->fifo_len - rxflr - txflr) as
	 * one maximum value for tx, but it doesn't cover the
	 * data which is out of tx/rx fifo and inside the
	 * shift registers. So a control from sw point of
	 * view is taken.
	 */
	rxtx_gap = ((priv->rx_end - priv->rx) - (priv->tx_end - priv->tx)) /
		(priv->bits_per_word >> 3);

	return min3(tx_left, tx_room, (u32)(priv->fifo_len - rxtx_gap));
#endif
}

/* Return the max entries we should read out of rx fifo */
static inline u32 rx_max(struct dw_spi_priv *priv)
{
	u32 rx_left = (priv->rx_end - priv->rx) / (priv->bits_per_word >> 3);

	return min_t(u32, rx_left, dw_read(priv, DW_SPI_RXFLR));
}

static void dw_writer(struct dw_spi_priv *priv)
{
	u32 max = tx_max(priv);
	u16 txw = 0;

	while (max--) {
		/* Set the tx word if the transfer's original "tx" is not null */
		if (priv->tx_end - priv->len) {
			if (priv->bits_per_word == 8)
				txw = *(u8 *)(priv->tx);
			else
				txw = *(u16 *)(priv->tx);
		}
		dw_write(priv, DW_SPI_DR, txw);
		debug("%s: tx=0x%02x\n", __func__, txw);
		priv->tx += priv->bits_per_word >> 3;
	}
}

static void dw_reader(struct dw_spi_priv *priv)
{
	u32 max = rx_max(priv);
	u16 rxw;

	while (max--) {
		rxw = dw_read(priv, DW_SPI_DR);
		debug("%s: rx=0x%02x\n", __func__, rxw);

		/* Care about rx if the transfer's original "rx" is not null */
		if (priv->rx_end - priv->len) {
			if (priv->bits_per_word == 8)
				*(u8 *)(priv->rx) = rxw;
			else
				*(u16 *)(priv->rx) = rxw;
		}
		priv->rx += priv->bits_per_word >> 3;
	}
}

static int poll_transfer(struct dw_spi_priv *priv)
{
#if 1 /* ENABLE_QPI */
	if (priv->rx) {
		dw_write(priv, DW_SPI_DR, 0xffffffff);
	}
	do {
		if (priv->tx) {
			dw_writer(priv);
		} else if (priv->rx) {
			dw_reader(priv);
		} else {
			return -1;
		}
	} while (priv->rx_end > priv->rx || priv->tx_end > priv->tx);

	return 0;

#else
	do {
		dw_writer(priv);
		dw_reader(priv);
	} while (priv->rx_end > priv->rx);

	return 0;
#endif
}

/*
 * We define external_cs_manage function as 'weak' as some targets
 * (like MSCC Ocelot) don't control the external CS pin using a GPIO
 * controller. These SoCs use specific registers to control by
 * software the SPI pins (and especially the CS).
 */
__weak void external_cs_manage(struct udevice *dev, bool on)
{
#if CONFIG_IS_ENABLED(DM_GPIO) && !defined(CONFIG_SPL_BUILD)
	struct dw_spi_priv *priv = dev_get_priv(dev->parent);
#ifdef SPI_DUAL_CS
	struct dm_spi_slave_platdata *platdata = dev_get_parent_platdata(dev);

	if (!dm_gpio_is_valid(&priv->cs_gpio[platdata->cs]))
#else
	if (!dm_gpio_is_valid(&priv->cs_gpio))
#endif
		return;

#ifdef SPI_DUAL_CS
	dm_gpio_set_value(&priv->cs_gpio[platdata->cs], on ? 1 : 0);
#ifdef AX_SPI_MSG_PRINT
	printf("0x%X: 0x%X\n", GPIO1_BASE, *((volatile u32 *)GPIO1_BASE));
	printf("0x%X: 0x%X\n", GPIO1_A12_ADDR, *((volatile u32 *)GPIO1_A12_ADDR));
	printf("0x%X: 0x%X\n", GPIO2_BASE, *((volatile u32 *)GPIO2_BASE));
	printf("0x%X: 0x%X\n", GPIO2_A29_ADDR, *((volatile u32 *)GPIO2_A29_ADDR));
#endif
#else
	dm_gpio_set_value(&priv->cs_gpio, on ? 1 : 0);
#endif
#endif
}

static int dw_spi_xfer(struct udevice *dev, unsigned int bitlen,
		       const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev->parent;
	struct dw_spi_priv *priv = dev_get_priv(bus);
	const u8 *tx = dout;
	u8 *rx = din;
	int ret = 0;
	u32 cr0 = 0;
	u32 spicr0 = 0;
	u32 txftlr = 0;
	u32 rxftlr = 0;
	u32 val;
	u32 cs;
#ifdef AX_SPI_MSG_PRINT
	int i;
#endif
#ifdef SPI_MST_USE_DMA
	/* txrx_dlr and burst_len must keep identical,
	 currently configured as half fifo depth */
	u32 txrx_dlr = SPI_MST_DMA_TXRX_DLR;
	u32 burst_len = DMAC_BURST_TRANS_LEN_32;
	u32 dma_width = DMAC_TRANS_WIDTH_8;
#endif
#if !defined (SPI_MST_USE_DMA) && defined (CONFIG_TARGET_AX620E_HAPS)
	int temp_len;
#endif

	/* spi core configured to do 8 bit transfers */
	if (bitlen % 8) {
		debug("Non byte aligned SPI transfer.\n");
		return -1;
	}

	/* Start the transaction if necessary. */
	if (flags & SPI_XFER_BEGIN)
		external_cs_manage(dev, false);

	cr0 = (priv->bits_per_word - 1) | (priv->type << SPI_FRF_OFFSET) |
		(priv->mode << SPI_MODE_OFFSET) |
		(priv->tmode << SPI_TMOD_OFFSET);

	priv->len = bitlen >> 3;
#ifdef AX_SPI_MSG_PRINT
	printf("[%dB %s] ", priv->len, (tx ? "out" : "in"));
	if (tx) {
		for (i = 0; i < (priv->len > 8 ? 8 : priv->len); i++)
			printf("%02x ", tx[i]);
		printf("\n");
	}
#endif

	if (flags & SPI_XFER_QUAD) {
		cr0 |= SPI_FRF_SPI_QUAD << SPI_SPI_FRF_OFFSET;
		if (rx || (priv->len >= 2))
			spicr0 = SPI_CLK_STRETCH_EN << SPI_CLK_STRETCH_EN_OFFSET;
		rxftlr = SPI_MST_DMA_TXRX_DLR + 1;
		if (tx) {
			spicr0 |= SPI_TRANS_TYPE_TT2 << SPI_TRANS_TYPE_OFFSET;
			spicr0 |= SPI_ADDR_L8 << SPI_ADDR_L_OFFSET;
#if 0
			txftlr = 1 << SPI_TXFTHR_OFFSET;//bug fix
#endif
#if defined (SPI_MST_USE_DMA)
			txftlr = (((priv->len > 0x1f) ? 0x1f : (priv->len - 1)) << SPI_TXFTHR_OFFSET |
			       ((priv->len > 0x4) ? 0x4 : (priv->len - 1)));
#else
			if ((priv->len > 0) && (priv->len <= priv->fifo_len))
				txftlr = (priv->len - 1) << SPI_TXFTHR_OFFSET;
			else
				printf("%s: pio xfer len %d, bitlen %d error\n", __func__, priv->len, bitlen);
#endif
		}
	}
#if 1 /* ENABLE_QPI, support tx only & rx only */

	if (tx)
		priv->tmode = SPI_TMOD_TO; //cr0=707
	else if (rx)
		priv->tmode = SPI_TMOD_RO; //cr0=b07

#else
	if (rx && tx)
		priv->tmode = SPI_TMOD_TR;
	else if (rx)
		priv->tmode = SPI_TMOD_RO;
	else
		/*
		 * In transmit only mode (SPI_TMOD_TO) input FIFO never gets
		 * any data which breaks our logic in poll_transfer() above.
		 */
		priv->tmode = SPI_TMOD_TR;
#endif

	cr0 &= ~SPI_TMOD_MASK;
	cr0 |= (priv->tmode << SPI_TMOD_OFFSET);

	debug("%s: rx=%p tx=%p len=%d [bytes]\n", __func__, rx, tx, priv->len);

#if 1 /* ENABLE_QPI */

	if (tx) {
		priv->tx = (void *)tx;
		priv->tx_end = priv->tx + priv->len;
		priv->rx_end = priv->rx = 0;
	} else if (rx) {
		priv->rx = rx;
		priv->rx_end = priv->rx + priv->len;
		priv->tx_end = priv->tx = 0;
	}

#else
	priv->tx = (void *)tx;
	priv->tx_end = priv->tx + priv->len;
	priv->rx = rx;
	priv->rx_end = priv->rx + priv->len;
#endif

	/* Disable controller before writing control registers */
	spi_enable_chip(priv, 0);

	debug("%s: cr0=%08x\n", __func__, cr0);
	/* Reprogram cr0 only if changed */
	if (dw_read(priv, DW_SPI_CTRL0) != cr0)
		dw_write(priv, DW_SPI_CTRL0, cr0);

	if ((flags & SPI_XFER_QUAD) && dw_read(priv, DW_SPI_SPI_CTRL0) != spicr0)
		dw_write(priv, DW_SPI_SPI_CTRL0, spicr0);

	dw_write(priv, DW_SPI_TXFLTR, txftlr);
	dw_write(priv, DW_SPI_RXFLTR, rxftlr);

	if (rx)
		dw_write(priv, DW_SPI_CTRL1, priv->len - 1);
	else if (tx && (flags & SPI_XFER_QUAD)) {
		if ((priv->len < 2) && (spicr0 & (SPI_CLK_STRETCH_EN << SPI_CLK_STRETCH_EN_OFFSET)))
			printf("TXONLY xfer len = %d < 2\n", priv->len);
		dw_write(priv, DW_SPI_CTRL1, priv->len - 2);
	}

	/*
	 * Configure the desired SS (slave select 0...3) in the controller
	 * The DW SPI controller will activate and deactivate this CS
	 * automatically. So no cs_activate() etc is needed in this driver.
	 */
	cs = spi_chip_select(dev);
	dw_write(priv, DW_SPI_SER, 1 << cs);

#ifdef SPI_MST_USE_DMA
	if (priv->len >= txrx_dlr) {
		if (tx) {
			dw_write(priv, DW_SPI_DMATDLR, txrx_dlr);
			dw_write(priv, DW_SPI_DMACR, 2);

			/* Enable controller after writing control registers */
			spi_enable_chip(priv, 1);

			axi_dma_xfer_start(DMAC_CHAN0, (u64)tx, ((u64)priv->regs + DW_SPI_DR), priv->len,
			dma_width, dma_width, burst_len, DMA_ENDIAN_NONE, DMA_MEM_TO_DEV, SSI_DMA_TX_REQ);
		} else {
			axi_dma_xfer_start(DMAC_CHAN0, ((u64)priv->regs + DW_SPI_DR), (u64)rx, priv->len,
			dma_width, dma_width, burst_len, DMA_ENDIAN_NONE, DMA_DEV_TO_MEM, SSI_DMA_RX_REQ);
			dw_write(priv, DW_SPI_DMARDLR, txrx_dlr-1);
			dw_write(priv, DW_SPI_DMACR, 1);

			/* Enable controller after writing control registers */
			spi_enable_chip(priv, 1);

			/* dummy word to trigger RXONLY transfer */
			dw_write(priv, DW_SPI_DR, 0xffffffff);
		}

		ret = axi_dma_wait_xfer_done(DMAC_CHAN0);
	} else
#endif
	{
	/* Enable controller after writing control registers */
	spi_enable_chip(priv, 1);

	/* Start transfer in a polling loop */
	ret = poll_transfer(priv);
	}

	/*
	 * Wait for current transmit operation to complete.
	 * Otherwise if some data still exists in Tx FIFO it can be
	 * silently flushed, i.e. dropped on disabling of the controller,
	 * which happens when writing 0 to DW_SPI_SSIENR which happens
	 * in the beginning of new transfer.
	 */
	if (readl_poll_timeout(priv->regs + DW_SPI_SR, val,
			       (val & SR_TF_EMPT) && !(val & SR_BUSY),
			       RX_TIMEOUT * 1000)) {
		ret = -ETIMEDOUT;
	}

#ifdef SPI_MST_USE_DMA
	if (priv->len >= txrx_dlr) {
		spi_enable_chip(priv, 0);
		dw_write(priv, DW_SPI_DMACR, 0);
		spi_enable_chip(priv, 1);
	}
#endif

#ifdef AX_SPI_MSG_PRINT
	if (rx) {
		for (i = 0; i < (priv->len > 8 ? 8 : priv->len); i++)
			printf("%02x ", rx[i]);
		printf("\n");
	}
#endif
	/* Stop the transaction if necessary */
	if (flags & SPI_XFER_END)
		external_cs_manage(dev, true);

	return ret;
}

static int dw_spi_set_speed(struct udevice *bus, uint speed)
{
	struct dw_spi_platdata *plat = bus->platdata;
	struct dw_spi_priv *priv = dev_get_priv(bus);
	u16 clk_div;

	if (speed > plat->frequency)
		speed = plat->frequency;

	/* Disable controller before writing control registers */
	spi_enable_chip(priv, 0);

	/* clk_div doesn't support odd number */
	clk_div = priv->bus_clk_rate / speed;
	clk_div = (clk_div + 1) & 0xfffe;
	//dw_write(priv, DW_SPI_BAUDR, clk_div);
	dw_write(priv, DW_SPI_BAUDR, 4);//50=200/4
	dw_write(priv, DW_SPI_RX_SAMPLE_DLY, SPI_RX_SAMPLE_DELAY);//50=200/4
	/*printf("%s: reg [0x%x, 0x%x], [0x%llx, 0x%x], [0x%llx, 0x%x]\n", __func__,
		FLASH_SYS_CLK_RST_BASE, readl(FLASH_SYS_CLK_RST_BASE),
		(u64)priv->regs + DW_SPI_BAUDR, dw_read(priv, DW_SPI_BAUDR),
		(u64)priv->regs + DW_SPI_RX_SAMPLE_DLY, dw_read(priv, DW_SPI_RX_SAMPLE_DLY));*/

	/* Enable controller after writing control registers */
	spi_enable_chip(priv, 1);

	priv->freq = speed;
	debug("%s: regs=%p speed=%d clk_div=%d\n", __func__, priv->regs,
	      priv->freq, clk_div);

	return 0;
}

static int dw_spi_set_mode(struct udevice *bus, uint mode)
{
	struct dw_spi_priv *priv = dev_get_priv(bus);

	/*
	 * Can't set mode yet. Since this depends on if rx, tx, or
	 * rx & tx is requested. So we have to defer this to the
	 * real transfer function.
	 */
	priv->mode = mode;
	debug("%s: regs=%p, mode=%d\n", __func__, priv->regs, priv->mode);

	return 0;
}

static int dw_spi_remove(struct udevice *bus)
{
	struct dw_spi_priv *priv = dev_get_priv(bus);
	int ret;

	ret = reset_release_bulk(&priv->resets);
	if (ret)
		return ret;

#if CONFIG_IS_ENABLED(CLK)
	ret = clk_disable(&priv->clk);
	if (ret)
		return ret;

	ret = clk_free(&priv->clk);
	if (ret)
		return ret;
#endif
	return 0;
}

static const struct dm_spi_ops dw_spi_ops = {
	.xfer		= dw_spi_xfer,
	.set_speed	= dw_spi_set_speed,
	.set_mode	= dw_spi_set_mode,
	/*
	 * cs_info is not needed, since we require all chip selects to be
	 * in the device tree explicitly
	 */
};

static const struct udevice_id dw_spi_ids[] = {
	{ .compatible = "snps,dw-ssi" },
	{ }
};

U_BOOT_DRIVER(dw_spi) = {
	.name = "dw_spi",
	.id = UCLASS_SPI,
	.of_match = dw_spi_ids,
	.ops = &dw_spi_ops,
	.ofdata_to_platdata = dw_spi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct dw_spi_platdata),
	.priv_auto_alloc_size = sizeof(struct dw_spi_priv),
	.child_pre_probe = dw_spi_child_pre_probe,
	.probe = dw_spi_probe,
	.remove = dw_spi_remove,
};
