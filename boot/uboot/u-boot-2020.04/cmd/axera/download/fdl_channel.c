/*
 * (C) Copyright 2020 AXERA
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <fdl_channel.h>
#include <serial.h>
#include <asm/arch/ax620e.h>
#include <asm/arch/boot_mode.h>
#include <fdl_frame.h>

#define HOST_RDY             0x3C
#define UART_RECV_CHAR_SIZE  3

FDL_ChannelHandler_T *g_CurrChannel = NULL;
extern struct FDL_ChannelHandler gUartChannel;
extern struct FDL_ChannelHandler gUsbChannel;	//define in \cmd\axera_download\fdl_channel_usb.c
extern struct boot_mode_info boot_info_data;

int usb_handshake(u32 timeout_ms)
{
	u8 buf[64] __attribute__ ((aligned(64)));
	u32 recv_len;

	recv_len = g_CurrChannel->read(g_CurrChannel, buf, 64);

	if (recv_len > 0 && buf[0] == HOST_RDY && buf[1] == HOST_RDY && buf[2] == HOST_RDY) {
		printf("usb 3C handshake success\n");
		return 0;
	}

	printf("usb 3C handshake failed\n");
	g_CurrChannel = NULL;

	return -1;
}

int uart_handshake(u32 timeout_ms)
{
	u8 buf[3] = { 0 };
	int k;

	for (k = 0; k < UART_RECV_CHAR_SIZE; k++) {
		buf[k] = g_CurrChannel->getchar(g_CurrChannel);
	}
	if (buf[0] == HOST_RDY && buf[1] == HOST_RDY && buf[2] == HOST_RDY) {
		printf("uart 3C handshake success\n");
		return 0;
	}

	printf("uart 3C handshake failed\n");
	g_CurrChannel = NULL;
	return -1;
}

int fdl_channel_init(void)
{
	int ret;

	//get boot_mode
	switch (boot_info_data.dl_channel) {
	case DL_CHAN_USB:
		g_CurrChannel = &gUsbChannel;
		break;
	case DL_CHAN_UART0:
		gUartChannel.channel = DL_CHAN_UART0;
		gUartChannel.priv = (void *)&eserial1_device;
		g_CurrChannel = &gUartChannel;
		break;
	case DL_CHAN_UART1:
		gUartChannel.channel = DL_CHAN_UART1;
		gUartChannel.priv = (void *)&eserial2_device;
		g_CurrChannel = &gUartChannel;
		break;
	default:
		printf("get fdl channel error\n");
		break;
	}

	ret = (g_CurrChannel == NULL) ? 1 : 0;

	return ret;
}
