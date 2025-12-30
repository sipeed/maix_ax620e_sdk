#include "timer.h"
#include "uart.h"
#include "dma.h"
#include "stdarg.h"
#include "chip_reg.h"
#include "printf.h"

static void IsThrEmpty(int base, u32 timeout_us)
{
	u32 start = getCurrTime(USEC);
	int status;
	u32 delay;

	do {
		status = readl(base + LSR_R) & UART_LSR_THRE;
		delay = start - getCurrTime(USEC);
	} while (!status && delay <= timeout_us);
}

#ifdef FDL_SUPPORT
static void IsTemtEmpty(int base, u32 timeout_us)
{
	u32 start = getCurrTime(USEC);
	int status;
	u32 delay;

	do {
		status = readl(base + LSR_R) & UART_LSR_TEMT;
		delay = start - getCurrTime(USEC);
	} while (!status && delay <= timeout_us);
}
#endif

void uart_putc(int base, const char ch, u32 timeout_us)
{
	IsThrEmpty(base, timeout_us);
	writel(ch, base + THR_R);
}

static void IsDataReady(int base, u32 timeout_us)
{
	u32 start = getCurrTime(USEC);
	int status;
	u32 delay;

	do {
		status = readl(base + LSR_R) & UART_LSR_DR;
		delay = start - getCurrTime(USEC);
	} while (!status && delay <= timeout_us);
}

int uart_getc(int base, u32 timeout_us)
{
	IsDataReady(base, timeout_us);
	return readl(base + RBR_R);
}

#ifdef FDL_SUPPORT
int uart_read(int base, u8 *buf, int len)
{
	int ret;
	u32 per_num = 0;
	u32 burst_len = DMAC_BURST_TRANS_LEN_1;
	u32 dma_width = DMAC_TRANS_WIDTH_8;

	switch (base) {
	case UART0_BASE:
		per_num = UART0_RX_REQ;
		break;
	case UART1_BASE:
		per_num = UART1_RX_REQ;
		break;
	}

	/* workaround: software ack dma req generated during pio mode */
	writel(1, (base + DMASA_R));
	ret = axi_dma_xfer_start(DMAC_CHAN0, (base + RBR_R), (unsigned long)buf, len,
	dma_width, dma_width, burst_len, DMA_ENDIAN_NONE, DMA_DEV_TO_MEM, per_num);
	if (ret < 0)
		return ret;
	ret = axi_dma_wait_xfer_done(DMAC_CHAN0);
	if (ret < 0)
		return ret;

	return len;
}

int uart_write(int base, u8 *buf, int len)
{
	int ret;
	u32 per_num = 0;
	u32 burst_len = DMAC_BURST_TRANS_LEN_8;
	u32 dma_width = DMAC_TRANS_WIDTH_8;

	switch (base) {
	case UART0_BASE:
		per_num = UART0_TX_REQ;
		break;
	case UART1_BASE:
		per_num = UART1_TX_REQ;
		break;
	}

	/* workaround: software ack dma req generated during pio mode */
	writel(1, (base + DMASA_R));
	axi_dma_xfer_start(DMAC_CHAN0, (unsigned long)buf, (base + THR_R), len,
	dma_width, dma_width, burst_len, DMA_ENDIAN_NONE, DMA_MEM_TO_DEV, per_num);
	ret = axi_dma_wait_xfer_done(DMAC_CHAN0);
	if (ret < 0)
		return ret;

	return len;
}
#endif

#if defined(UART_USE_PIO)
u32 uart_write_pio(int base, u8 *buf, int len)
{
	u8 ch;
	u32 wr_count = 0;

	while (wr_count < len) {
		ch = *buf++;
		uart_putc(base, ch, 500);
		wr_count++;
	}

	return wr_count;
}

int uart_read_pio(int base, u8 *buf, int len)
{
	u8 ch;

	u32 recv_count = 0;
	u8 *pframe = buf;

	while (1) {
		ch = uart_getc(base, 500);
		recv_count++;

		*pframe++ = ch;

		if (recv_count == len)
				break;
		}

	return recv_count;
}
#endif
#ifdef FDL_SUPPORT
int uart_set_baudrate(u32 base, u32 baudrate)
{
	u32 baud_inter = (UART_BASE_CLK + baudrate / 2) / 16 / baudrate;
	u32 baud_frac = ((UART_BASE_CLK + baudrate / 2) / baudrate) & 0xf;
	u32 temp;
	IsTemtEmpty(base, 100000);
	/* set baud*/
	temp = readl(base + LCR_R) & ~UART_LCR_BKSE;
	writel(temp | UART_LCR_BKSE, base + LCR_R);
	writel(baud_inter & 0xFF, base + DLL_R);
	writel((baud_inter >> 8) & 0xFF, base + DLH_R);
	writel(baud_frac, base + 0xc0);
	writel(temp, base + LCR_R);
	return 0;
}
#endif

void uart_init(int base)
{
	u32 baud = (UART_BASE_CLK + UART_BAUDATE/2) / 16 / UART_BAUDATE;
	u32 temp = 0;

	/* uart0/1 pin config */
	if (base == UART0_BASE) {
		writel(0x83, PIN_MUX_G6_UART0_TXD);
		writel(0x93, PIN_MUX_G6_UART0_RXD);
	}
	if (base == UART1_BASE) {
		writel(0x83, PIN_MUX_G6_UART1_TXD);
		writel(0x93, PIN_MUX_G6_UART1_RXD);
	}
	writel(0, base + IER_R);
	writel(UART_MCRVAL, base + MCR_R);
	writel(UART_FCR_DEFVAL, base + FCR_R);
	//*((volatile unsigned int *)0x0201700c) |= 1<<7;        //LCR
	writel(UART_LCRVAL, base + LCR_R);

	/* set baud*/
	temp = readl(base + LCR_R) & ~UART_LCR_BKSE;
	writel(temp | UART_LCR_BKSE, base + LCR_R);
	writel(baud & 0xFF, base + DLL_R);
	writel((baud >> 8) & 0xFF, base + DLH_R);
	writel(temp, base + LCR_R);
}

