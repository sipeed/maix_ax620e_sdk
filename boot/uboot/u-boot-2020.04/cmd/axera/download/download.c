/*
 * AXERA AX620E Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/boot_mode.h>
#include <fdl_channel.h>
#include <fdl_engine.h>
#include <fdl_frame.h>

extern FDL_ChannelHandler_T *g_CurrChannel;

int do_download(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	printf("do downlaod\n");

	ret = fdl_channel_init();
	if (ret) {
		printf("download channel init fail\n");
		return ret;
	}

	fdl_dl_init();

	//init current channel
	ret = g_CurrChannel->open(g_CurrChannel);
	if (ret) {
		printf("open channel failed\n");
		return ret;
	}

	/* ACK for romcode excute cmd */
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret) {
		printf("error while send fdl2 first ack\n");
		return ret;
	}

	fdl_dl_entry();

	return 0;
}

U_BOOT_CMD(
	download,	1,	0,	do_download,
	"download mode",
	"choose to enter download mode\n"
	"it is used for downloading image to storage\n"
);

