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
#include "board.h"
#include "sdio_slave.h"
#include "dma.h"

struct FDL_ChannelHandler gSdioslvChannel;

static int sdio_slv_dl_open(FDL_ChannelHandler_T *channel)
{
	int ret;
	axi_dma_hw_init();
	ret = sdio_slave_func_init();
	return ret;
}
static int sdio_slv_dl_read(FDL_ChannelHandler_T *channel, u8 *buf, u32 len, u32 timeout_ms)
{
	return sdio_slv_read(len, buf, timeout_ms);
}
static int sdio_slv_dl_write(FDL_ChannelHandler_T *channel, const u8 *buf, u32 len, u32 timeout_ms)
{
	return sdio_slv_write(len, (void *)buf, timeout_ms);
}


struct FDL_ChannelHandler gSdioslvChannel = {
	.channel = DL_CHAN_SDIO,
	.open = sdio_slv_dl_open,
	.read = sdio_slv_dl_read,
	.write = sdio_slv_dl_write,
};


