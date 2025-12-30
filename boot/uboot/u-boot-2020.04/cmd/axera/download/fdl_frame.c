/*
 * (C) Copyright 2020 AXERA
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <fdl_frame.h>
#include <fdl_channel.h>

#define ROM_VER           "fdl2 v1.0;raw"
#define FRMAE_HEADER      0x5C6D8E9F
#define HEADER_1ST_BYTE   0x9F

struct fdl_frame frame_buf;	//buf used to send & recv
extern FDL_ChannelHandler_T *g_CurrChannel;

static int frame_check_header(fdl_frame_t * pframe)
{
	if (pframe->magic_num != FRMAE_HEADER)
		return -1;
	else
		return 0;
}

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

	sum = (sum >> 16) + (sum & 0x0FFFF);
	sum += (sum >> 16);

	return (u16) (~sum);
}

int frame_send_respone(CMD_TYPE resp)
{
	u16 checksum;
	u32 count;
	u32 write_count;

	frame_buf.cmd_index = resp;
	frame_buf.magic_num = FRMAE_HEADER;
	frame_buf.data_len = 0;
	checksum = frame_checksum(&frame_buf);

	//add checksum to the end of data
	frame_buf.data[0] = checksum & 0xFF;
	frame_buf.data[1] = (checksum >> 8) & 0xFF;

	count = FRAME_EXCEPT_DATA_SIZE;

	write_count = g_CurrChannel->write(g_CurrChannel, (u8 *) & frame_buf, count);
	if (write_count == count)
		return 0;
	else
		return -1;
}

int frame_send_data(CMD_TYPE resp, char *buffer, u64 len)
{
	u16 checksum;
	u32 count;
	u32 write_count;

	if (len + FDL_CHECKSUM_SIZE > FRAME_DATA_MAXLEN)
		printf("%s: data len 0x%llX + checksum size over packet limit data len 0x%llX\n",
		       __FUNCTION__, len, (u64) FRAME_DATA_MAXLEN);
	frame_buf.cmd_index = resp;
	frame_buf.magic_num = FRMAE_HEADER;
	frame_buf.data_len = len;
	memset((void *)frame_buf.data, 0, FRAME_DATA_MAXLEN);
	memcpy((void *)frame_buf.data, (void *)buffer, len);
	checksum = frame_checksum(&frame_buf);

	//add checksum to the end of data
	frame_buf.data[len] = checksum & 0xFF;
	frame_buf.data[len + 1] = (checksum >> 8) & 0xFF;

	count = len + FRAME_EXCEPT_DATA_SIZE;

	write_count = g_CurrChannel->write(g_CurrChannel, (u8 *) & frame_buf, count);
	//printf("<<%s: magic_num 0x%X data_len 0x%X, cmd_index 0x%X, body&checksum len 0x%llX\n", __FUNCTION__,
	//frame_buf.magic_num, frame_buf.data_len, frame_buf.cmd_index, (len+2));
	if (write_count == count)
		return 0;
	else
		return -1;
}

fdl_frame_t *frame_get(void)
{
	u8 ch;
	int frame_size;
	u16 data_len = 0;
	u32 recv_count = 0;
	u16 frame_chksum;
	u8 *pframe = (u8 *) & frame_buf;

	//set the header data to 0
	memset((void *)pframe, 0, FRAME_EXCEPT_DATA_SIZE);
	if (g_CurrChannel->channel == DL_CHAN_USB) {
		frame_size = g_CurrChannel->read(g_CurrChannel, pframe, FRAME_MAX_SIZE);
		if (0 == frame_size)
			return NULL;

		data_len = frame_buf.data_len;
	} else {
		//filter usless char before header
		while ((ch = g_CurrChannel->getchar(g_CurrChannel))
		       != HEADER_1ST_BYTE) ;

		//has recevied first char
		recv_count++;
		*pframe++ = ch;

		while (1) {
			ch = g_CurrChannel->getchar(g_CurrChannel);
			recv_count++;

			*pframe++ = ch;
			if (recv_count == (FDL_MAGIC_SIZE + FDL_DATALEN_SIZE))
				data_len = frame_buf.data_len;

			if ((recv_count - FRAME_EXCEPT_DATA_SIZE) == data_len)
				break;
		}
	}

	frame_chksum = frame_buf.data[data_len] | (frame_buf.data[data_len + 1] << 8);

	if ((frame_chksum != frame_checksum(&frame_buf)) || (frame_check_header(&frame_buf))) {
		frame_send_respone(FDL_RESP_VERIFY_CHEKSUM_ERROR);
		return NULL;
	}

	return &frame_buf;
}

int frame_send_version(void)
{
	const char *version;
	u32 len;

	version = ROM_VER;
	len = strlen(ROM_VER);

	return frame_send_data(FDL_RESP_VERSION, (char *)version, len);
}
