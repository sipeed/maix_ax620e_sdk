/*
 * (C) Copyright 2020 AXERA
 *
 * SPDX-License-Identifier:     GPL-2.0+
 *
 */

#include <common.h>
#include <serial.h>
#include <fdl_channel.h>


DECLARE_GLOBAL_DATA_PTR;

static u8 uart_getchar(FDL_ChannelHandler_T *channel);


static int uart_open(FDL_ChannelHandler_T *channel)
{
	int ret;
	struct serial_device *fdl_uart_dev = (struct serial_device *)channel->priv;

	ret = fdl_uart_dev->start();

	return ret;
}

static int uart_read(FDL_ChannelHandler_T *channel,  u8 *buf, u32 len)
{
	u8 ch;
	u32 recv_count = 0;
	u8 *pframe = buf;

	while (1) {
		ch = uart_getchar(channel);
		recv_count++;
		*pframe++ = ch;
		if (recv_count == len)
			break;
	}
	return recv_count;
}

static u8 uart_getchar(FDL_ChannelHandler_T *channel)
{
	struct serial_device *fdl_uart_dev = (struct serial_device *)channel->priv;

	while (!fdl_uart_dev->tstc());

	return fdl_uart_dev->getc();
}

static u32 uart_write(FDL_ChannelHandler_T *channel, const u8 *buf, u32 len)
{
	struct serial_device *fdl_uart_dev = (struct serial_device *)channel->priv;
	u8 ch;
	u32 wr_count = 0;

	while (wr_count < len) {
		ch = *buf++;
		fdl_uart_dev->putc(ch);
		wr_count++;
	}

	return wr_count;
}

static int uart_putchar(FDL_ChannelHandler_T *channel, const u8 ch)
{
	struct serial_device *fdl_uart_dev = (struct serial_device *)channel->priv;

	fdl_uart_dev->putc(ch);

	return 1;
}

static int uart_setbaudrate(FDL_ChannelHandler_T *channel, u32 baudrate)
{
	struct serial_device *fdl_uart_dev = (struct serial_device *)channel->priv;
	u32 baudrate_temp = gd->baudrate;
	gd->baudrate = baudrate;
	printf("download switch baud rate to  %d\n",baudrate);
	fdl_uart_dev->setbrg();
	gd->baudrate = baudrate_temp;
	return 0;
}

struct FDL_ChannelHandler gUartChannel = {
	.channel = DL_CHAN_UART0,
	.open = uart_open,
	.read = uart_read,
	.getchar = uart_getchar,
	.write = uart_write,
	.putchar = uart_putchar,
	.setbaudrate = uart_setbaudrate,
	.priv = NULL,
};
