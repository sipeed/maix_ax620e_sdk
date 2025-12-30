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
#include "fdl_usb.h"
#include "dma.h"


static int usb_dl_open(FDL_ChannelHandler_T *channel)
{
	return axera_usb_init();
}

static int usb_dl_read(FDL_ChannelHandler_T *channel, u8 *buf, u32 len, u32 timeout_ms)
{
	return usb_recv(buf, len, timeout_ms);
}

static int usb_dl_write(FDL_ChannelHandler_T *channel, const u8 *buf, u32 len, u32 timeout_ms)
{
	return usb_send(buf, len, timeout_ms);
}


struct FDL_ChannelHandler gUSBChannel = {
	.channel = DL_CHAN_USB,
	.open = usb_dl_open,
	.read = usb_dl_read,
	.write = usb_dl_write,
};