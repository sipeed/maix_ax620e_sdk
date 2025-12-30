/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "fdl_channel.h"
#include "cmn.h"
#include "dma.h"
#include "uart.h"
#include "printf.h"

//defult set uart0
struct uart_info gUartInfo = {
	.reg_base = UART0_BASE,
	.baudrate = 115200,
};

static int uart_dl_open(FDL_ChannelHandler_T *channel)
{
	axi_dma_hw_init();
	return 0;
}

static char uart_dl_getchar(FDL_ChannelHandler_T *channel, u32 timeout_ms)
{
	struct uart_info *p_info = (struct uart_info *)channel->priv;
	int base = p_info->reg_base;

	return uart_getc(base, timeout_ms * 1000);
}

static int uart_dl_read(FDL_ChannelHandler_T *channel, u8 *buf, u32 len, u32 timeout_ms)
{
	struct uart_info *p_info = (struct uart_info *)channel->priv;
	int base = p_info->reg_base;

	return uart_read(base, buf, len);
	//return uart_read_pio(base, buf, len);
}

static int uart_dl_write(FDL_ChannelHandler_T *channel, const u8 *buf, u32 len, u32 timeout_ms)
{
	struct uart_info *p_info = (struct uart_info *)channel->priv;
	int base = p_info->reg_base;
	return uart_write(base, (u8 *)buf, len);
	//return uart_write_pio(base, (u8 *)buf, len);
}

static int uart_dl_set_baudrate(FDL_ChannelHandler_T *channel, u32 baudrate)
{
	struct uart_info *p_info = (struct uart_info *)channel->priv;
	return uart_set_baudrate(p_info->reg_base, baudrate);
}

struct FDL_ChannelHandler gUartChannel = {
	.channel = DL_CHAN_UART,
	.open = uart_dl_open,
	.getchar = uart_dl_getchar,
	.read = uart_dl_read,
	.write = uart_dl_write,
	.set_baudrate = uart_dl_set_baudrate,
	.priv = &gUartInfo,
};
