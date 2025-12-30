/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "fdl_frame.h"
#include "fdl_channel.h"
#include "secure.h"

#define ROM_VER           "fdl1 v1.0;raw"
#define ROM_VER_SEC       "fdl1 v1.0;secureboot;raw"
#define FRMAE_HEADER      0x5C6D8E9F
#define HEADER_1ST_BYTE   0x9F

struct fdl_frame frame_buf;
extern FDL_ChannelHandler_T *g_CurrChannel;

static u16 frame_checksum(fdl_frame_t * pframe)
{
	u32 sum = 0;
	int cal_len = pframe->data_len + FDL_DATALEN_SIZE + FDL_CMDIDX_SIZE;
	u8 *pSrc = (unsigned char *)pframe + FDL_MAGIC_SIZE;

	while (cal_len > 1) {
		sum += *(unsigned short *)(pSrc);
		pSrc += 2;
		cal_len -= 2;
	}

	if (cal_len == 1) {
		sum += *pSrc;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return (u16) (~sum);
}

int frame_send(CMD_TYPE cmd, u8 * txbuf, u32 txlen, u32 timeout_ms)
{
	u16 checksum;
	u32 count;
	u32 write_count;

	frame_buf.magic_num = FRMAE_HEADER;
	frame_buf.cmd_index = cmd;
	frame_buf.data_len = txlen;

	for (int i = 0; i < txlen; i++) {
		frame_buf.data[i] = txbuf[i];
	}

	checksum = frame_checksum(&frame_buf);
	//add checksum to the end of data
	frame_buf.data[txlen] = checksum & 0xFF;
	frame_buf.data[txlen + 1] = (checksum >> 8) & 0xFF;

	count = FRAME_EXCEPT_DATA_SIZE + txlen;

	if (g_CurrChannel->channel == DL_CHAN_SPI)
		count = FDL_FRAME_MAX_SIZE;

	write_count = g_CurrChannel->write(g_CurrChannel, (u8 *)&frame_buf, count, timeout_ms);
	if (write_count != count)
		return -1;

	return 0;
}

fdl_frame_t *frame_get(u32 timeout_ms)
{
	u8 ch;
	int frame_size;
	u16 data_len = 0;
	u32 recv_count;
	u16 frame_chksum;
	u8 *pframe = (u8 *)&frame_buf;

	ax_memset(pframe, 0, FRAME_EXCEPT_DATA_SIZE - FDL_CHECKSUM_SIZE);

	if (g_CurrChannel->channel == DL_CHAN_SPI) {
		frame_size = g_CurrChannel->read(g_CurrChannel, pframe, FDL_FRAME_MAX_SIZE, timeout_ms);
		if (frame_size != FDL_FRAME_MAX_SIZE)
			return NULL;
	} else if (g_CurrChannel->channel == DL_CHAN_USB) {
		frame_size = g_CurrChannel->read(g_CurrChannel, pframe, FDL_FRAME_MAX_SIZE, timeout_ms);
		// filter 0x3c
		if (0 == frame_size || pframe[0] != HEADER_1ST_BYTE)
			return NULL;
	} else if (g_CurrChannel->channel == DL_CHAN_UART) {
		//abandon the redundant 0x3c before header
		do {
			ch = g_CurrChannel->getchar(g_CurrChannel, timeout_ms);
		} while (ch != HEADER_1ST_BYTE);

		*pframe++ = ch;
		recv_count = 1;

		while (1) {
			*pframe++ = g_CurrChannel->getchar(g_CurrChannel, timeout_ms);
			recv_count++;
			if (recv_count == (FDL_MAGIC_SIZE + FDL_DATALEN_SIZE))
				data_len = frame_buf.data_len;
			if ((recv_count - FRAME_EXCEPT_DATA_SIZE) == data_len)
				break;
		}
	}

	data_len = frame_buf.data_len;
	frame_chksum = frame_buf.data[data_len] | (frame_buf.data[data_len + 1] << 8);
	if ((frame_chksum != frame_checksum(&frame_buf)) || frame_buf.magic_num != FRMAE_HEADER) {
		frame_send_respone(FDL_RESP_VERIFY_ERROR);
		return NULL;
	}

	return &frame_buf;
}

int frame_send_respone(CMD_TYPE resp)
{
	return frame_send(resp, NULL, 0, 3000);
}

int frame_send_version()
{
	const char *version;
	u32 len;

	if (is_secure_enable()) {
		version = ROM_VER_SEC;
		len = ax_strlen(ROM_VER_SEC);
	} else {
		version = ROM_VER;
		len = ax_strlen(ROM_VER);
	}

	return frame_send(FDL_RESP_VERSION, (u8 *) version, len, 3000);
}
