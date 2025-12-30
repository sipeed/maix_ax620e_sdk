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
#include "fdl_frame.h"
#include "uart.h"
#include "timer.h"
#include "dma.h"
#include "dw_spi.h"
#include "board.h"
#include "chip_reg.h"
#include "printf.h"

FDL_ChannelHandler_T *g_CurrChannel = NULL;
extern struct FDL_ChannelHandler gUSBChannel;
extern struct FDL_ChannelHandler gSpiChannel;
extern struct FDL_ChannelHandler gUartChannel;

static int spi_slv_handshake(u32 timeout_ms)
{
	int ret;
	fdl_frame_t *pframe;

	g_CurrChannel = &gSpiChannel;
	ret = g_CurrChannel->open(g_CurrChannel);
	if(ret)
		return -1;

	//ack romcode cmd_execute
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	info("ack romcode cmd_execute\r\n");
	ret = frame_send(FDL_CMD_HANDSHAKE, NULL, 0, timeout_ms);
	if (ret < 0)
		return ret;

	pframe = frame_get(200);
	if (!pframe)
		return -1;

	if (pframe->cmd_index != FDL_RESP_ACK)
		return -1;

	ret = frame_send(FDL_RESP_ACK, NULL, 0, 200);
	if (ret < 0)
		return ret;

	return 0;
}

static int usb_handshake(u32 timeout_ms)
{
	u8 buf[64];
	u32 recv_len;
	int ret;

	g_CurrChannel = &gUSBChannel;
	ret = g_CurrChannel->open(g_CurrChannel);
	if(ret)
		return -1;

	//ack romcode cmd_execute
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	recv_len = g_CurrChannel->read(g_CurrChannel, buf, 64, timeout_ms);

	if (recv_len > 0 && buf[0] == HOST_RDY && buf[1] == HOST_RDY && buf[2] == HOST_RDY) {
		info("usb handshake success\r\n");;
		return 0;
	}

	g_CurrChannel = NULL;
	return -1;
}

static int uart_handshake(u32 timeout_ms)
{
	struct uart_info *p_info = (struct uart_info *)gUartChannel.priv;
	int base = UART0_BASE;
	u8 buf[3];
	int ret;
	u32 dl_channel;

	g_CurrChannel = &gUartChannel;
	ret = g_CurrChannel->open(g_CurrChannel);
	if(ret)
		return -1;

	dl_channel = readl(COMM_SYS_DUMMY_SW5);
	if (dl_channel == DL_CHAN_UART1)
		p_info->reg_base = UART1_BASE;
	//ack romcode cmd_execute
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;
	info("ack romcode cmd_execute\r\n");
	/* delay 20ms to wait for host's 0x3c arriving */
	mdelay(20);

	for (int i = 0; i < 2; i++) {
		for (int k = 0; k < 3; k++) {
			buf[k] = uart_getc((base + i * 0x1000), 1);
		}
		if (buf[0] == HOST_RDY && buf[1] == HOST_RDY \
			&& buf[2] == HOST_RDY) {
			p_info->reg_base = (base + i * 0x1000);
			return 0;
		}
	}
	info("uart_handshake failed\r\n");
	return -1;
}

int fdl_channel_init(dl_channel_e chan, u32 timeout_ms)
{
	int ret = 0;
	g_CurrChannel = NULL;

	/* disable wtd during downloading */
	wtd_enable(0);

	switch (chan) {
	case DL_CHAN_SPI:
		ret = spi_slv_handshake(timeout_ms);
		break;
	case DL_CHAN_USB:
		ret = usb_handshake(timeout_ms);
		break;
	case DL_CHAN_UART:
		ret = uart_handshake(timeout_ms);
		break;
	default:
		;
	}

	if (ret < 0)
		return ret;

	return ret;
}
