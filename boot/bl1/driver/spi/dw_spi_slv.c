#include "cmn.h"
#include "timer.h"
#include "dw_spi.h"
#include "dma.h"
#include "chip_reg.h"

static u32 ssi_s_version = SPI_VERSION_102A;

static 	u32 fifowidth = DFS_8_BIT;

static inline u32 dw_slv_read(u32 offset)
{
	return readl((DW_SPI_SLAVE_BASE + offset));
}

static inline void dw_slv_write(u32 offset, u32 val)
{
	writel(val, (DW_SPI_SLAVE_BASE + offset));
}

static void spi_slv_pin_config(void)
{
	u32 val;
	/* SPI slave pins */
	//pull-up/pull-down and driver strength config
	writel(0x20013, (PIN_MUX_G1_BASE + SPI_CLK_S));
	writel(0x20093, (PIN_MUX_G1_BASE + SPI_D0_S));
	writel(0x20093, (PIN_MUX_G1_BASE + SPI_D1_S));
	writel(0x20093, (PIN_MUX_G1_BASE + SPI_D2_S));
	writel(0x20093, (PIN_MUX_G1_BASE + SPI_D3_S));
	writel(0x20093, (PIN_MUX_G1_BASE + SPI_CS_S));
	/* GPIO pin */
	writel(0x60003, (PIN_MUX_G1_BASE + FLASH_EN));
	writel(BIT(SPI_S_DUPLEX_OFFS), PIN_MUX_G1_MISC_SET);

#ifndef SPI_HAPS_DEBUG
	//software mode
	val = readl((GPIO0_BASE + 0x4 + 3 * 0x4));
	val &= ~(1 << 2);
	//direction
	val |= (1 << 1);
	//default low & RXONLY
	val &= ~(1 << 0);//GPIO0_A3
	writel(val, (GPIO0_BASE + 0x4 + 3 * 0x4));
#else
	//software mode
	val = readl((GPIO0_BASE + 0x4 + 17 * 0x4));
	val &= ~(1 << 2);
	//direction
	val |= (1 << 1);
	//default low & RXONLY
	val &= ~(1 << 0);//GPIO0_C1
	writel(val, (GPIO0_BASE + 0x4 + 17 * 0x4));

#ifdef TAG_0207
	writel(BIT(6), 0x10038144);
#endif
#endif
}

static void spi_slv_enable_chip(int enable)
{
	dw_slv_write(DW_SPI_SSIENR, (enable ? 1 : 0));
}

static void dw_slv_writer(struct dw_poll_priv *priv)
{
	/* get the max entries we can fill into tx fifo */
	u32 dw = priv->fifowidth >> 3;
	u32 tx_left = priv->entries;
	u32 tx_avail = DW_SPI_SLV_FIFO_LEN/dw - dw_slv_read(DW_SPI_TXFLR);
	u32 tx_max = tx_left > tx_avail ? tx_avail : tx_left;
	priv->entries = tx_left - tx_max;

	while (tx_max--) {
		if (priv->fifowidth == DFS_8_BIT)
			dw_slv_write(DW_SPI_DR, (*(u8 *)(priv->tx)));
		else
			dw_slv_write(DW_SPI_DR, (*(u32 *)(priv->tx)));
		priv->tx += dw;
	}
}

static void dw_slv_reader(struct dw_poll_priv *priv)
{
	/* get the max entries we should read out of rx fifo */
	u32 dw = priv->fifowidth >> 3;
	u32 rx_left = priv->entries;
	u32 rx_avail = dw_slv_read(DW_SPI_RXFLR);
	u32 rx_max = rx_left > rx_avail ? rx_avail : rx_left;
	priv->entries = rx_left - rx_max;

	while (rx_max--) {
		if (priv->fifowidth == DFS_8_BIT)
			*(u8 *)(priv->rx) = dw_slv_read(DW_SPI_DR);
		else
			*(u32 *)(priv->rx) = dw_slv_read(DW_SPI_DR);
		priv->rx += dw;
	}
}

static int slv_poll_transfer(struct dw_poll_priv *priv)
{
	do {
		if (priv->tx) {
			dw_slv_writer(priv);
		} else if (priv->rx){
			dw_slv_reader(priv);
		} else {
			return -1;
		}
	} while (priv->entries);

	return 0;
}

static void spi_slv_switch_mode(unsigned int xfer_mode)
{
	u32 cr0;

	switch (xfer_mode) {
	case SPI_TMOD_RO :
		cr0 = dw_slv_read(DW_SPI_CTRL0);
		cr0 &= ~(SPI_TMOD_MASK | SPI_SLVOE_MASK);
		cr0 |= (SPI_TMOD_RO << SPI_TMOD_OFFSET) | (SPI_SLVOE_DISABLE << SPI_SLVOE_OFFSET);
		break;
	case SPI_TMOD_TO :
		cr0 = dw_slv_read(DW_SPI_CTRL0);
		cr0 &= ~(SPI_TMOD_MASK | SPI_SLVOE_MASK);
		cr0 |= (SPI_TMOD_TO << SPI_TMOD_OFFSET) | (SPI_SLVOE_ENABLE << SPI_SLVOE_OFFSET);
		break;
	default:
		break;
	}

	spi_slv_enable_chip(0);
	dw_slv_write(DW_SPI_CTRL0, cr0);
	spi_slv_enable_chip(1);
}

static void spi_slv_txEnable(u8 enable)
{
	u32 val;
#ifdef SPI_HAPS_DEBUG
	//value
	if (enable) {//high = TXONLY
		val = readl((GPIO0_BASE + 0x4 + 17 * 0x4));
		val |= 1 << 0;
		writel(val, (GPIO0_BASE + 0x4 + 17 * 0x4));
	} else {//low = RXONLY
		val = readl((GPIO0_BASE + 0x4 + 17 * 0x4));
		val &= ~(1 << 0);
		writel(val, (GPIO0_BASE + 0x4 + 17 * 0x4));
	}
#else
	//value
	if (enable) {//high = TXONLY
		val = readl((GPIO0_BASE + 0x4 + 3 * 0x4));
		val |= 1 << 0;
		writel(val, (GPIO0_BASE + 0x4 + 3 * 0x4));
	} else {//low = RXONLY
		val = readl((GPIO0_BASE + 0x4 + 3 * 0x4));
		val &= ~(1 << 0);
		writel(val, (GPIO0_BASE + 0x4 + 3 * 0x4));
	}
#endif
}

static int dw_spi_slv_xfer(unsigned int nbytes, const void *dout, void *din, u32 timeout_ms)
{
	const u8 *tx = dout;
	u8 *rx = din;
	int ret = 0;
	u32 status, txEmpty, busy;
	struct dw_poll_priv priv;
	u32 wait_time, start = getCurrTime(MSEC);
	u32 txrx_dlr = SPI_SLV_DMA_TXRX_DLR;
	u32 burst_len = DMAC_BURST_TRANS_LEN_16;
	u32 dma_width = DMAC_TRANS_WIDTH_8;

	if (tx)
		spi_slv_switch_mode(SPI_TMOD_TO);
	else
		spi_slv_switch_mode(SPI_TMOD_RO);

#ifdef SPI_SLV_USE_DMA
	if (nbytes >= txrx_dlr) {
		if (SPI_VERSION_103A == ssi_s_version) {
			spi_slv_enable_chip(0);
			if (tx) {
				dw_slv_write(DW_SPI_DMATDLR, txrx_dlr);
				dw_slv_write(DW_SPI_DMACR, 2);
				spi_slv_enable_chip(1);
				axi_dma_xfer_start(DMAC_CHAN0, (unsigned long)tx, (DW_SPI_SLAVE_BASE + DW_SPI_DR), nbytes,
					dma_width, dma_width, burst_len, DMA_ENDIAN_NONE, DMA_MEM_TO_DEV, SSI_S_DMA_TX_REQ);

				while (dw_slv_read(DW_SPI_SR) & SR_TF_EMPT);
				spi_slv_txEnable(1);
			}
			else {
				axi_dma_xfer_start(DMAC_CHAN0, (DW_SPI_SLAVE_BASE + DW_SPI_DR), (unsigned long)rx, nbytes,
					dma_width, dma_width, burst_len, DMA_ENDIAN_NONE, DMA_DEV_TO_MEM, SSI_S_DMA_RX_REQ);
				dw_slv_write(DW_SPI_DMARDLR, txrx_dlr-1);
				dw_slv_write(DW_SPI_DMACR, 1);
				spi_slv_enable_chip(1);

				spi_slv_txEnable(0);
			}

			ret = axi_dma_wait_xfer_done(DMAC_CHAN0);
		} else {
			if (tx) {
				axi_dma_xfer_start(DMAC_CHAN0, (unsigned long)tx, (DW_SPI_SLAVE_BASE + DW_SPI_DR), nbytes,
				dma_width, dma_width, burst_len, DMA_ENDIAN_NONE, DMA_MEM_TO_DEV, SSI_S_DMA_TX_REQ);
				dw_slv_write(DW_SPI_DMATDLR, txrx_dlr);
				dw_slv_write(DW_SPI_DMACR, 2);

				while (dw_slv_read(DW_SPI_SR) & SR_TF_EMPT);
				spi_slv_txEnable(1);
			} else {
				axi_dma_xfer_start(DMAC_CHAN0, (DW_SPI_SLAVE_BASE + DW_SPI_DR), (unsigned long)rx, nbytes,
				dma_width, dma_width, burst_len, DMA_ENDIAN_NONE, DMA_DEV_TO_MEM, SSI_S_DMA_RX_REQ);
				dw_slv_write(DW_SPI_DMARDLR, txrx_dlr - 1);
				dw_slv_write(DW_SPI_DMACR, 1);

				spi_slv_txEnable(0);
			}

			ret = axi_dma_wait_xfer_done(DMAC_CHAN0);
			dw_slv_write(DW_SPI_DMACR, 0);
		}
	} else
#endif
	{
		UNUSED(txrx_dlr);
		UNUSED(burst_len);
		UNUSED(dma_width);
		priv.entries = nbytes/(fifowidth >> 3);
		priv.fifowidth = fifowidth;
		priv.tx = (void *)tx;
		priv.rx = rx;
		ret = slv_poll_transfer(&priv);
		if (tx)
			spi_slv_txEnable(1);
	}

	if (tx) {
		/* Wait for current transmit operation to complete. */
		do {
			status = dw_slv_read(DW_SPI_SR);
			txEmpty = status & SR_TF_EMPT;
			busy = status & SR_BUSY;
			wait_time = start - getCurrTime(MSEC);
			if (wait_time > timeout_ms) {
				ret = -1;
				break;
			}
		} while ((!txEmpty) || busy);
	}
	return ret;
}

void spi_slv_quad_mode(u8 enable)
{
	u32 cr0;
	cr0 = dw_slv_read(DW_SPI_CTRL0);
	cr0 &= ~SPI_SPI_FRF_MASK;
	if (enable) {
		writel(BIT(SPI_S_DUPLEX_OFFS), PIN_MUX_G1_MISC_CLR);
		cr0 |= SPI_QAUD_MODE << SPI_SPI_FRF_OFFSET;
	} else {
		writel(BIT(SPI_S_DUPLEX_OFFS), PIN_MUX_G1_MISC_SET);
		cr0 |= SPI_STANDARD_MODE << SPI_SPI_FRF_OFFSET;
	}

	//set standard/quad mode
	spi_slv_enable_chip(0);
	dw_slv_write(DW_SPI_CTRL0, cr0);
	spi_slv_enable_chip(1);
}

void clk_spi_s_sel(u8 sel)
{
	u8 val = ((sel >= CLK_SPI_S_SEL_MAX) ? CLK_SPI_S_SEL_CPLL_24M : sel);

	if (val == ((readl(FLASH_SYS_GLB_CLK_MUX0) & CLK_SPI_S_SEL_MASK) >> CLK_SPI_S_SEL_OFFSET))
		return;

	// clk_spi_s_eb clr
	writel(BIT(12), FLASH_SYS_GLB_CLK_EB0_CLR);
	udelay(1);
	writel(CLK_SPI_S_SEL_MASK, FLASH_SYS_GLB_CLK_MUX0_CLR);
	writel((val << CLK_SPI_S_SEL_OFFSET), FLASH_SYS_GLB_CLK_MUX0_SET);

	// clk_spi_s_eb set
	writel(BIT(12), FLASH_SYS_GLB_CLK_EB0_SET);
}

void spi_slv_freq_boost(u8 sel)
{
	clk_spi_s_sel(sel);
}

/* Reset the controller, disable all interrupts */
void spi_slv_hw_init()
{
	u32 cr0;

	//clk_spi_s_sel(CLK_SPI_S_SEL_CPLL_208M);
	spi_slv_pin_config();
	spi_slv_enable_chip(0);
	dw_slv_write(DW_SPI_IMR, 0);
	cr0 = (fifowidth - 1) << SPI_DFS_OFFSET;
	cr0 |= SPI_FRF_SPI << SPI_FRF_OFFSET;
	cr0 |= SPI_TMOD_RO << SPI_TMOD_OFFSET;
	cr0 |= SPI_SLVOE_DISABLE << SPI_SLVOE_OFFSET;
	cr0 |= SPI_STANDARD_MODE << SPI_SPI_FRF_OFFSET;
	dw_slv_write(DW_SPI_CTRL0, cr0);
	ssi_s_version = dw_slv_read(DW_SPI_VERSION);
	spi_slv_enable_chip(1);
}

int spi_slv_read(unsigned int nbytes, void *din, u32 timeout_ms)
{
	int ret;
	ret = dw_spi_slv_xfer(nbytes, NULL, din, timeout_ms);
	if (ret < 0)
		return ret;
	return nbytes;
}

int spi_slv_write(unsigned int nbytes, void *dout, u32 timeout_ms)
{
	int ret;
	ret = dw_spi_slv_xfer(nbytes, dout, NULL, timeout_ms);
	if (ret < 0)
		return ret;
	return nbytes;
}


