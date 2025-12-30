#include "cmn.h"
#include "dw_spi.h"
#include "spinand.h"
#include "dma.h"
#include "chip_reg.h"
#include "trace.h"
#include "timer.h"

u8 curr_cs = 0;
u32 ssi_rx_sample_delay = 0;
u32 ssi_phy_setting = 0;
#if defined(AX620E_NAND)
u8 spi_cmd_addr_dummy_cs_bypass = 0;		//0: disable, 1: 13 & 0f opcode, 2: 6b opcode, 3: continuous data only
#endif
static u32 ssi_version = SPI_VERSION_102A;

static inline u32 dw_read(u32 offset)
{
	return readl((DW_SPI_BASE + offset));
}

static inline void dw_write(u32 offset, u32 val)
{
	writel(val, (DW_SPI_BASE + offset));
}

static void spi_pin_config(void)
{
	u32 val;
	int cs;

#ifndef SPI_HAPS_DEBUG
	//pull-up/pull-down and driver strength config
	writel(0x10017, (PIN_MUX_G11_BASE + SFC_CLK));
	writel(0x10097, (PIN_MUX_G11_BASE + SFC_MOSI_IO0));
	writel(0x10097, (PIN_MUX_G11_BASE + SFC_MISO_IO1));
	writel(0x10097, (PIN_MUX_G11_BASE + SFC_WP_IO2));
	writel(0x10097, (PIN_MUX_G11_BASE + SFC_HOLD_IO3));
	writel(0x60093, (PIN_MUX_G11_BASE + SFC_CSN0)); //GPIO2 A27
	writel(0x60093, (PIN_MUX_G11_BASE + SFC_CSN1)); //GPIO2 A25
#endif

	for (cs = 0; cs < SPI_CS_NUM; cs++) {
		//software mode
		val = readl((GPIO2_BASE + 0x4 + SPI_CS_GPIO(cs) * 0x4));
		val &= ~(1 << 2);
		//direction
		val |= (1 << 1);
		//default high
		val |= (1 << 0);
		writel(val, (GPIO2_BASE + 0x4 + SPI_CS_GPIO(cs) * 0x4));
	}
}

static void spi_enable_chip(int enable)
{
	dw_write(DW_SPI_SSIENR, (enable ? 1 : 0));
}

static void dw_writer(struct dw_poll_priv *priv)
{
	/* get the max entries we can fill into tx fifo */
	u32 dw = priv->fifowidth >> 3;
	u32 tx_left = priv->entries;
	u32 tx_avail = DW_SPI_FIFO_LEN/dw - dw_read(DW_SPI_TXFLR);
	u32 tx_max = tx_left > tx_avail ? tx_avail : tx_left;
	priv->entries = tx_left - tx_max;

	while (tx_max--) {
		if (priv->fifowidth == DFS_8_BIT)
			dw_write(DW_SPI_DR, (*(u8 *)(priv->tx)));
		else
			dw_write(DW_SPI_DR, (*(u32 *)(priv->tx)));
		priv->tx += dw;
	}
}

static void dw_reader(struct dw_poll_priv *priv)
{
	/* get the max entries we should read out of rx fifo */
	u32 dw = priv->fifowidth >> 3;
	u32 rx_left = priv->entries;
	u32 rx_avail = dw_read(DW_SPI_RXFLR);
	u32 rx_max = rx_left > rx_avail ? rx_avail : rx_left;
	priv->entries = rx_left - rx_max;

	while (rx_max--) {
		if (priv->fifowidth == DFS_8_BIT)
			*(u8 *)(priv->rx) = dw_read(DW_SPI_DR);
		else
			*(u32 *)(priv->rx) = dw_read(DW_SPI_DR);
		priv->rx += dw;
	}
}

static int poll_transfer(struct dw_poll_priv *priv)
{
	if (priv->rx)
		/* dummy word to trigger RXONLY transfer */
		dw_write(DW_SPI_DR, 0xffffffff);
	do {
		if (priv->tx) {
			dw_writer(priv);
		} else if (priv->rx){
			dw_reader(priv);
		} else {
			return -1;
		}
	} while (priv->entries);

	return 0;
}

void external_cs_manage(u32 on)
{
	u32 val;

	//value
	if (on) {//high
		val = readl((GPIO2_BASE + 0x4 + SPI_CS_GPIO(curr_cs) * 0x4));
		val |= (1 << 0);
		writel(val, (GPIO2_BASE + 0x4 + SPI_CS_GPIO(curr_cs) * 0x4));
	} else {//low
		val = readl((GPIO2_BASE + 0x4 + SPI_CS_GPIO(curr_cs) * 0x4));
		val &= ~ (1 << 0);
		writel(val, (GPIO2_BASE + 0x4 + SPI_CS_GPIO(curr_cs) * 0x4));
	}
}

static int dw_spi_xfer(unsigned int bitlen, const void *dout, void *din,
				unsigned int flags, unsigned int fifowidth, unsigned int qmode)
{
	const u8 *tx = dout;
	u8 *rx = din;
	int ret = 0;
	u32 cr0 = 0;
	u32 spicr0 = 0;
	u32 txftlr = 0;
	u32 rxftlr = 0;
	u32 status, txEmpty, busy;
	int timeout = RX_TIMEOUT;
	u32 tmode = SPI_TMOD_TO;
	u32 nbytes;
	u32 entries;
	struct dw_poll_priv priv;
	/* txrx_dlr and burst_len must keep identical,
	 currently configured as half fifo depth */
	u32 txrx_dlr = SPI_MST_DMA_TXRX_DLR;
	u32 burst_len = DMAC_BURST_TRANS_LEN_32;
	u32 dma_width = DMAC_TRANS_WIDTH_8;

	if (fifowidth == DFS_32_BIT)
		dma_width = DMAC_TRANS_WIDTH_32;

	/* spi core configured to do 8 bit transfers */
	if (bitlen % 8 || fifowidth % 8 || bitlen % fifowidth) {
		return -1;
	}
	nbytes = bitlen >> 3;
	entries = nbytes / (fifowidth >> 3);

	/* Start the transaction if necessary. */
	if (flags & SPI_XFER_BEGIN)
		external_cs_manage(0);

#if 0//(TEST_PLATFORM == HAPS)
	if (tx) {
		switch (nbytes) {
		case 1:
			info("\r\n ==> %dB: %02x\n", nbytes, tx[0]);
			break;

		case 2:
			info("\r\n ==> %dB: %02x %02x\n", nbytes, tx[0], tx[1]);
			break;

		case 3:
			info("\r\n ==> %dB: %02x %02x %02x\n", nbytes, tx[0], tx[1], tx[2]);
			break;

		default:
			info("\r\n ==> %dB: %02x %02x %02x %02x\n", nbytes, tx[0], tx[1], tx[2], tx[3]);
			break;
		}
	}
#endif

	cr0 = ((fifowidth - 1) << SPI_DFS_OFFSET) | (SPI_FRF_SPI << SPI_FRF_OFFSET);
	if (qmode) {
		cr0 |= SPI_QAUD_MODE << SPI_SPI_FRF_OFFSET;
		spicr0 = SPI_CLK_STRETCH_EN << SPI_CLK_STRETCH_EN_OFFSET;
		rxftlr = SPI_MST_DMA_TXRX_DLR + 1;
		if(tx) {
			spicr0 |= SPI_BOTH_IN_SPIFRF_MODE << SPI_TRANS_TYPE_OFFSET;
			if (fifowidth == DFS_32_BIT)
				spicr0 |= SPI_ADDR_L_32_BITS << SPI_ADDR_L_OFFSET;
			else
				spicr0 |= SPI_ADDR_L_8_BITS << SPI_ADDR_L_OFFSET;

			txftlr = 1 << SPI_TXFTHR_OFFSET;//bug fix
		}
	}

	if (tx)
		tmode = SPI_TMOD_TO;
	else if (rx)
		tmode = SPI_TMOD_RO;

	cr0 &= ~SPI_TMOD_MASK;
	cr0 |= (tmode << SPI_TMOD_OFFSET);

	/* Disable controller before writing control registers */
	spi_enable_chip(0);
	/* Reprogram cr0 only if changed */
	if (dw_read(DW_SPI_CTRL0) != cr0)
		dw_write(DW_SPI_CTRL0, cr0);

	if (qmode && dw_read(DW_SPI_SPI_CTRL0) != spicr0)
		dw_write(DW_SPI_SPI_CTRL0, spicr0);

	dw_write(DW_SPI_TXFLTR, txftlr);
	dw_write(DW_SPI_RXFLTR, rxftlr);

	if (rx)
		dw_write(DW_SPI_CTRL1, entries-1);

	if (SPI_VERSION_103A != ssi_version) {
		/* Enable controller after writing control registers */
		spi_enable_chip(1);
	}

#ifdef SPI_MST_USE_DMA
	if (entries >= txrx_dlr) {
		if (SPI_VERSION_103A == ssi_version) {
			if (tx) {
				dw_write(DW_SPI_DMATDLR, txrx_dlr);
				dw_write(DW_SPI_DMACR, 2);
				spi_enable_chip(1);
				axi_dma_xfer_start(DMAC_CHAN0, (unsigned long)tx, (DW_SPI_BASE + DW_SPI_DR), nbytes,
					dma_width, dma_width, burst_len, (fifowidth == DFS_32_BIT) ? DMA_ENDIAN_32BIT : DMA_ENDIAN_NONE, DMA_MEM_TO_DEV, SSI_DMA_TX_REQ);
			}
			else {
				axi_dma_xfer_start(DMAC_CHAN0, (DW_SPI_BASE + DW_SPI_DR), (unsigned long)rx, nbytes,
					dma_width, dma_width, burst_len, (fifowidth == DFS_32_BIT) ? DMA_ENDIAN_32BIT : DMA_ENDIAN_NONE, DMA_DEV_TO_MEM, SSI_DMA_RX_REQ);
				dw_write(DW_SPI_DMARDLR, txrx_dlr-1);
				dw_write(DW_SPI_DMACR, 1);
				spi_enable_chip(1);
				/* dummy word to trigger RXONLY transfer */
				dw_write(DW_SPI_DR, 0xffffffff);
			}

			ret = axi_dma_wait_xfer_done(DMAC_CHAN0);
		} else {
			if (tx) {
				axi_dma_xfer_start(DMAC_CHAN0, (unsigned long)tx, (DW_SPI_BASE + DW_SPI_DR), nbytes,
				dma_width, dma_width, burst_len, (fifowidth == DFS_32_BIT) ? DMA_ENDIAN_32BIT : DMA_ENDIAN_NONE, DMA_MEM_TO_DEV, SSI_DMA_TX_REQ);
				dw_write(DW_SPI_DMATDLR, txrx_dlr);
				dw_write(DW_SPI_DMACR, 2);
			}
			else {
				axi_dma_xfer_start(DMAC_CHAN0, (DW_SPI_BASE + DW_SPI_DR), (unsigned long)rx, nbytes,
				dma_width, dma_width, burst_len, (fifowidth == DFS_32_BIT) ? DMA_ENDIAN_32BIT : DMA_ENDIAN_NONE, DMA_DEV_TO_MEM, SSI_DMA_RX_REQ);
				dw_write(DW_SPI_DMARDLR, txrx_dlr-1);
				dw_write(DW_SPI_DMACR, 1);
				/* dummy word to trigger RXONLY transfer */
				dw_write(DW_SPI_DR, 0xffffffff);
			}

			ret = axi_dma_wait_xfer_done(DMAC_CHAN0);
			dw_write(DW_SPI_DMACR, 0);
		}
	} else
#endif
	{
		UNUSED(txrx_dlr);
		UNUSED(burst_len);
		UNUSED(dma_width);
		if (SPI_VERSION_103A == ssi_version) {
			/* Enable controller after writing control registers */
			spi_enable_chip(1);
		}
		priv.entries = entries;
		priv.fifowidth = fifowidth;
		priv.tx = (void *)tx;
		priv.rx = rx;
		ret = poll_transfer(&priv);
	}

	/* Wait for current transmit operation to complete. */
	do {
		status = dw_read(DW_SPI_SR);
		txEmpty = status & SR_TF_EMPT;
		busy = status & SR_BUSY;
	} while (!txEmpty || (busy && timeout--));

	if (timeout < 0)
		ret = -1;
#ifdef SPI_MST_USE_DMA
	else if ((SPI_VERSION_103A == ssi_version) && (entries >= txrx_dlr)) {
		spi_enable_chip(0);
		dw_write(DW_SPI_DMACR, 0);
		spi_enable_chip(1);
	}
#endif

#if 0//(TEST_PLATFORM == HAPS)
	if (rx) {
		switch (nbytes) {
		case 1:
			info("\r\n <== %dB: %02x\n", nbytes, rx[0]);
			break;

		case 2:
			info("\r\n <== %dB: %02x %02x\n", nbytes, rx[0], rx[1]);
			break;

		case 3:
			info("\r\n <== %dB: %02x %02x %02x\n", nbytes, rx[0], rx[1], rx[2]);
			break;

		default:
			info("\r\n <== %dB: %02x %02x %02x %02x\n", nbytes, rx[0], rx[1], rx[2], rx[3]);
			break;
		}
	}
#endif

	/* Stop the transaction if necessary */
	if (flags & SPI_XFER_END)
		external_cs_manage(1);

	return ret;
}

int spi_mem_adjust_op_size(struct spi_mem_op *op)
{
	if (op->data.dir == SPI_MEM_DATA_IN) {
		op->data.nbytes = op->data.nbytes <= MAX_READ_SIZE ? op->data.nbytes : MAX_READ_SIZE;
	} else {
		op->data.nbytes = op->data.nbytes <= MAX_WRITE_SIZE ? op->data.nbytes : MAX_WRITE_SIZE;
	}

	if (!op->data.nbytes)
		return -1;

	return 0;
}

int spi_mem_exec_op(const struct spi_mem_op *op)
{
	unsigned int pos = 0;
	const u8 *tx_buf = NULL;
	u8 *rx_buf = NULL;
	u32 fifowidth = 8;
	u32 qmode = 0;
	int op_len;
	u32 flag;
	int ret;
	int i;

	if (op->data.nbytes) {
		if (op->data.dir == SPI_MEM_DATA_IN)
			rx_buf = op->data.buf.in;
		else
			tx_buf = op->data.buf.out;
	}

	op_len = sizeof(op->cmd.opcode) + op->addr.nbytes + op->dummy.nbytes;
	u8 op_buf[op_len];

#if defined(AX620E_NAND)
	if (spi_cmd_addr_dummy_cs_bypass <= 2)
#endif
	{
	op_buf[pos++] = op->cmd.opcode;

	if (op->addr.nbytes) {
		for (i = 0; i < op->addr.nbytes; i++)
			op_buf[pos + i] = op->addr.val >>
				(8 * (op->addr.nbytes - i - 1));
		pos += op->addr.nbytes;
	}

	if (op->dummy.nbytes) {
		for (i = 0; i < op->dummy.nbytes; i++)
			op_buf[pos + i] = 0xff;
		//pos += op->dummy.nbytes;
	}

	/* 1st transfer: opcode + address + dummy cycles */
	flag = SPI_XFER_BEGIN;
	/* Make sure to set END bit if no tx or rx data follow */
	if (!tx_buf && !rx_buf)
		flag |= SPI_XFER_END;

	ret = dw_spi_xfer(op_len * 8, op_buf, NULL, flag, fifowidth, qmode);
	if (ret)
		return ret;
	}

	/* 2nd transfer: rx or tx data path */
	if (tx_buf || rx_buf) {
		if (op->data.buswidth == 4)
			qmode = 1;
#ifdef SPI_MST_USE_DMA
		if ((0 == (op->data.nbytes % sizeof(u32))) && ((op->data.nbytes / sizeof(u32)) >= SPI_MST_DMA_TXRX_DLR))
			fifowidth = 32;
#endif
#if defined(AX620E_NAND)
		ret = dw_spi_xfer(op->data.nbytes * 8, tx_buf, rx_buf, (spi_cmd_addr_dummy_cs_bypass > 1) ? 0 : SPI_XFER_END, fifowidth, qmode);
#else
		ret = dw_spi_xfer(op->data.nbytes * 8, tx_buf, rx_buf, SPI_XFER_END, fifowidth, qmode);
#endif
		if (ret)
			return ret;
	}

	return 0;
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


#if defined(AX620E_NOR)
#define SPI_RX_SAMPLE_DELAY	(0x3)
#elif defined(AX620E_NAND)
#define SPI_RX_SAMPLE_DELAY	(0x3)
#endif
/* Reset the controller, disable all interrupts, set baudrate */
void spi_hw_init(u32 bus_clk)
{
	u32 clk_div;
	static int initialized = 0;

	if (initialized)
		return;

	if (bus_clk <= 6000000) {
		//clk_h_ssi_sel(CLK_H_SSI_SEL_CPLL_24M);
		/* clk_div doesn't support odd number */
		clk_div = SPI_MAX_BUS_CLK / bus_clk;
		clk_div = (clk_div + 1) & 0xfffe;
	}
	else if (bus_clk == 50000000) {
		bus_clk = 104000000;
		clk_h_ssi_sel(CLK_H_SSI_SEL_CPLL_416M);
		/* clk_div doesn't support odd number */
		clk_div = SPI_CPLL_416M_MAX_BUS_CLK / bus_clk;
		clk_div = (clk_div + 1) & 0xfffe;
	}
	else {
		clk_h_ssi_sel(CLK_H_SSI_SEL_CPLL_208M);
		/* clk_div doesn't support odd number */
		clk_div = SPI_NPLL_200M_MAX_BUS_CLK / bus_clk;
		clk_div = (clk_div + 1) & 0xfffe;
	}

	spi_pin_config();
	spi_enable_chip(0);
	dw_write(DW_SPI_IMR, 0);
	dw_write(DW_SPI_BAUDR, clk_div);
	ssi_version = dw_read(DW_SPI_VERSION);
	if (SPI_VERSION_103A == ssi_version) {
		dw_write(DW_SPI_SPI_CTRL0, 0);
		if (ssi_rx_sample_delay && (0 == (ssi_rx_sample_delay & 0xfffeff00)))
			dw_write(DW_SPI_RX_SAMPLE_DELAY, ssi_rx_sample_delay);
		else if (bus_clk > 50000000)
			dw_write(DW_SPI_RX_SAMPLE_DELAY, SPI_RX_SAMPLE_DELAY);
		else
			dw_write(DW_SPI_RX_SAMPLE_DELAY, clk_div / 2 - 1);
	}
	spi_enable_chip(1);
	ax_print_str("\r\nssi clk=");
	ax_print_num(bus_clk, 10);
	ax_print_str(", CPU_SYS_GLB_CLK_MUX0=0x");
	ax_print_num(readl(CPU_SYS_GLB_CLK_MUX0), 16);
	ax_print_str(", DW_SPI_BAUDR=");
	ax_print_num(dw_read(DW_SPI_BAUDR), 16);
	ax_print_str(", DW_SPI_RX_SAMPLE_DELAY=");
	ax_print_num(dw_read(DW_SPI_RX_SAMPLE_DELAY), 16);
	initialized = 1;
}

