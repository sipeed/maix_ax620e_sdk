#ifndef __UART_H__
#define __UART_H__
#include "cmn.h"

#define UART0_BASE		0x4880000
#define UART1_BASE		0x4881000
#define UART2_BASE		0x4882000

#define USE_UART		UART0_BASE

#ifndef HAPS_DEBUG
#define UART_BAUDATE      115200
#define UART_BASE_CLK     208000000
#else
#define UART_BAUDATE      38400
#define UART_BASE_CLK     10000000
#endif

//pin config reg(pin G6)
#define UART0_TXD		0x0C
#define UART0_RXD		0x18
#define UART1_TXD		0x24
#define UART1_RXD		0x30

#define DLL_R			0x0
#define RBR_R			0x0
#define THR_R			0x0
#define DLH_R			0x4
#define IER_R			0x4
#define FCR_R			0x8
#define LCR_R			0xC
#define MCR_R			0x10
#define LSR_R			0x14
#define MSR_R			0x18
#define DMASA_R			0xa8

/*
 * These are the definitions for the FIFO Control Register
 */
#define UART_FCR_FIFO_EN	0x01 /* Fifo enable */
#define UART_FCR_CLEAR_RCVR	0x02 /* Clear the RCVR FIFO */
#define UART_FCR_CLEAR_XMIT	0x04 /* Clear the XMIT FIFO */
#define UART_FCR_DMA_SELECT	0x08 /* For DMA applications */

#define UART_FCR_RT_MASK	0xC0 /* Mask for the RX FIFO trigger range */
#define UART_FCR_RT_CHAR_1	0x00 /* Mask for trigger set at 1 */
#define UART_FCR_RT_QUARTER	0x40 /* Mask for trigger set at 4 */
#define UART_FCR_RT_HALF	0x80 /* Mask for trigger set at 8 */
#define UART_FCR_RT_FULL_2	0xC0 /* Mask for trigger set at 14 */

#define UART_FCR_TET_MASK	0x30 /* Mask for the TX FIFO trigger range */
#define UART_FCR_TET_EMEPTY	0x00 /* Mask for trigger set at 0 */
#define UART_FCR_TET_CHAR_2	0x10 /* Mask for trigger set at 2 */
#define UART_FCR_TET_QUARTER	0x20 /* Mask for trigger set at 4 */
#define UART_FCR_TET_HALF	0x30 /* Mask for trigger set at 8 */

#define UART_FCR_RXSR		0x02 /* Receiver soft reset */
#define UART_FCR_TXSR		0x04 /* Transmitter soft reset */

/* Ingenic JZ47xx specific UART-enable bit. */
#define UART_FCR_UME		0x10

/*
 * These are the definitions for the Modem Control Register
 */
#define UART_MCR_DTR	0x01		/* DTR   */
#define UART_MCR_RTS	0x02		/* RTS   */
#define UART_MCR_OUT1	0x04		/* Out 1 */
#define UART_MCR_OUT2	0x08		/* Out 2 */
#define UART_MCR_LOOP	0x10		/* Enable loopback test mode */
#define UART_MCR_AFE	0x20		/* Enable auto-RTS/CTS */

#define UART_MCR_DMA_EN	0x04
#define UART_MCR_TX_DFR	0x08

/*
 * These are the definitions for the Line Control Register
 *
 * Note: if the word length is 5 bits (UART_LCR_WLEN5), then setting
 * UART_LCR_STOP will select 1.5 stop bits, not 2 stop bits.
 */
#define UART_LCR_WLS_MSK 0x03		/* character length select mask */
#define UART_LCR_WLS_5	0x00		/* 5 bit character length */
#define UART_LCR_WLS_6	0x01		/* 6 bit character length */
#define UART_LCR_WLS_7	0x02		/* 7 bit character length */
#define UART_LCR_WLS_8	0x03		/* 8 bit character length */
#define UART_LCR_STB	0x04		/* # stop Bits, off=1, on=1.5 or 2) */
#define UART_LCR_PEN	0x08		/* Parity eneble */
#define UART_LCR_EPS	0x10		/* Even Parity Select */
#define UART_LCR_STKP	0x20		/* Stick Parity */
#define UART_LCR_SBRK	0x40		/* Set Break */
#define UART_LCR_BKSE	0x80		/* Bank select enable */
#define UART_LCR_DLAB	0x80		/* Divisor latch access bit */

/*
 * These are the definitions for the Line Status Register
 */
#define UART_LSR_DR	0x01		/* Data ready */
#define UART_LSR_OE	0x02		/* Overrun */
#define UART_LSR_PE	0x04		/* Parity error */
#define UART_LSR_FE	0x08		/* Framing error */
#define UART_LSR_BI	0x10		/* Break */
#define UART_LSR_THRE	0x20		/* Xmit holding register empty */
#define UART_LSR_TEMT	0x40		/* Xmitter empty */
#define UART_LSR_ERR	0x80		/* Error */

#define UART_MSR_DCD	0x80		/* Data Carrier Detect */
#define UART_MSR_RI	    0x40		/* Ring Indicator */
#define UART_MSR_DSR	0x20		/* Data Set Ready */
#define UART_MSR_CTS	0x10		/* Clear to Send */
#define UART_MSR_DDCD	0x08		/* Delta DCD */
#define UART_MSR_TERI	0x04		/* Trailing edge ring indicator */
#define UART_MSR_DDSR	0x02		/* Delta DSR */
#define UART_MSR_DCTS	0x01		/* Delta CTS */

/*
 * These are the definitions for the Interrupt Identification Register
 */
#define UART_IIR_NO_INT	0x01	/* No interrupts pending */
#define UART_IIR_ID	    0x06	/* Mask for the interrupt ID */

#define UART_IIR_MSI	0x00	/* Modem status interrupt */
#define UART_IIR_THRI	0x02	/* Transmitter holding register empty */
#define UART_IIR_RDI	0x04	/* Receiver data interrupt */
#define UART_IIR_RLSI	0x06	/* Receiver line status interrupt */

/*
 * These are the definitions for the Interrupt Enable Register
 */
#define UART_IER_MSI	0x08	/* Enable Modem status interrupt */
#define UART_IER_RLSI	0x04	/* Enable receiver line status interrupt */
#define UART_IER_THRI	0x02	/* Enable Transmitter holding register int. */
#define UART_IER_RDI	0x01	/* Enable receiver data interrupt */

/* useful defaults for LCR */
#define UART_LCR_8N1	0x03

#define UART_MCRVAL (UART_MCR_DTR | UART_MCR_RTS)
/* Clear & enable FIFOs */
#define UART_FCR_DEFVAL (UART_FCR_FIFO_EN | \
							UART_FCR_RXSR |	\
							UART_FCR_TXSR)
#define UART_LCRVAL UART_LCR_8N1	/* 8 data, 1 stop, no parity */
#define	EAGAIN		11	/* Try again */

void uart_init(int base);
void uart_putc(int    base, const char ch, u32 timeout);
int uart_getc(int base, u32 timeout);
int uart_read(int base, unsigned char *buf, int len);
int uart_write(int base, unsigned char *buf, int len);
u32 uart_write_pio(int base, u8 *buf, int len);
int uart_read_pio(int base, unsigned char *buf, int len);
int uart_set_baudrate(u32 base, u32 baudrate);

#endif
