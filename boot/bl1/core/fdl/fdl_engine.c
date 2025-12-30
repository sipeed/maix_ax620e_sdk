/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "uart.h"
#include "cmn.h"
#include "fdl_engine.h"
#include "fdl_channel.h"
#include "secure.h"
#include "board.h"
#include "boot.h"
#include "dw_spi.h"
#include "trace.h"
#include "printf.h"

static u32 secure_done = 0;

extern int axi_dma_word_checksum(u32 *out, unsigned long sar, int size);
extern FDL_ChannelHandler_T *g_CurrChannel;
extern struct FDL_ChannelHandler gUartChannel;
struct fdl_cmdproc cmdproc_tab[FDL_CMD_TYPE_MAX - FDL_CMD_TYPE_MIN];
struct fdl_file_info g_file_info = {
	.start_addr = -1,
	.curr_addr = -1,
	.target_len = -1,
	.recv_len = 0,
	.image_id = 0,
	.secure_en = 0,
};

#define CMD_HANDLER(cmd, pframe) cmdproc_tab[cmd - FDL_CMD_TYPE_MIN].handler\
						(pframe, cmdproc_tab[cmd - FDL_CMD_TYPE_MIN].arg)

int verify_img_header(struct img_header *header)
{
	if(header->magic_data != IMG_HEADER_MAGIC_DATA)
		return -1;
	if(calc_word_chksum((int *)&header->capability, sizeof(struct img_header) - 8) != header->check_sum)
		return -1;

	return 0;
}

u32 calc_word_chksum(int *data, int size)
{
	int count = size / 4;
	int i;
	u32 sum = 0;
	if(axi_dma_word_checksum(&sum, (unsigned long)data, size))
	{
		/* if dma calulate failed, use cpu calu. */
		sum = 0;
		for(i = 0; i < count; i++) {
			sum += data[i];
		}
	}
	return sum;
}

static u32 calc_image_checkSum(u8 *buf, u32 len)
{
	u32 chkSum = 0;
	u32 aligned = (len & ~0x3);
	u32 remaining = (len & 0x3);

	chkSum = calc_word_chksum((int *)buf, aligned);

	for (int i = 0; i< remaining; i++) {
		chkSum += *(buf + aligned + i);
	}
	return chkSum;
}
#if CONFIG_ARM64
void jump_to_execute(u32 start_addr)
{
	/* make sure wtd enabled before jump */
	wtd_enable(1);
	__asm__ __volatile__ (
		"mov x0,%0\n\t"
		"br x0\n\t"
		:
		: "r" (start_addr));
}
#else
void jump_to_execute(u32 start_addr)
{
	/* make sure wtd enabled before jump */
	wtd_enable(1);
	__asm__ __volatile__ (
		"mov r0,%0\n\t"
		"bx r0\n\t"
		:
		: "r" (start_addr)
	);
}
#endif
int cmd_connect(fdl_frame_t *pframe, void *arg)
{
	int ret;

	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	return 0;
}

int cmd_start_transfer(fdl_frame_t *pframe, void *arg)
{
	struct fdl_file_info *pfile = (struct fdl_file_info *)arg;
	int ret;

	pfile->start_addr = *((u32 *)pframe->data);
	pfile->target_len = *((u64 *)(pframe->data + sizeof(u64)));
	pfile->curr_addr = pfile->start_addr;
	pfile->recv_len = 0;
	pfile->image_id++;
	pfile->secure_en = is_secure_enable();

	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	return 0;
}

int cmd_transfer_data(fdl_frame_t *pframe, void *arg)
{
	struct fdl_file_info *pfile = (struct fdl_file_info *)arg;
	u32 img_size, chksum, chksum_en, retlen;
	int ret;

	img_size = ((u32 *)pframe->data)[0];
	chksum_en = ((u32 *)pframe->data)[1];
	chksum = ((u32 *)pframe->data)[2];
	if (pfile->recv_len + img_size > pfile->target_len) {
		ret = frame_send_respone(FDL_RESP_SIZE_ERR);
		if (ret)
			return ret;
		return 0;
	}

	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	retlen = g_CurrChannel->read(g_CurrChannel, (u8 *)(unsigned long)pfile->curr_addr, img_size, 5000);//5s timeout
	if (retlen != img_size)
		return -1;

	if (chksum_en && chksum != calc_image_checkSum((u8 *)(unsigned long)pfile->curr_addr, img_size)) {
		ret = frame_send_respone(FDL_RESP_VERIFY_ERROR);
		if (ret)
			return ret;
		return 0;
	} else {
		pfile->curr_addr += img_size;
		pfile->recv_len += img_size;
	}
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	return 0;
}

#define PUB_KEY_ARRAY_MAX_SZ  (396)
int cmd_transfer_end(fdl_frame_t *pframe, void *arg)
{
	int ret;
	char public_key[PUB_KEY_ARRAY_MAX_SZ];
	struct fdl_file_info *pfile = (struct fdl_file_info *)arg;

	if (pfile->recv_len != pfile->target_len) {
		ret = frame_send_respone(FDL_RESP_SIZE_ERR);
		if (ret)
			return ret;
		return 0;
	}

	if (pfile->secure_en) {
			struct img_header *header = (struct img_header *)(unsigned long)pfile->start_addr;
			char *img_addr = (char*)header + IMG_HEADER_SIZE;
			int key_len = (header->capability & RSA_3072_MODE) ? 3072 : 2048;

			/* verify image header */
			if(verify_img_header(header) < 0) {
				goto secure_err;
			}
			/* hash verification if secure enabled */
			ax_memset(public_key, 0, PUB_KEY_ARRAY_MAX_SZ);
			ax_memcpy((void *)(public_key), (void *)&header->key_n_header, (4 + key_len / 8));
			ax_memcpy((void *)(public_key + (4 + key_len / 8)), (void *)&header->key_e_header, 8);
			if (public_key_verify((unsigned long)(public_key), sizeof(public_key)) < 0) {
				goto secure_err;
			}
			if(cipher_sha256(img_addr, (char *)hash_digest, header->img_size) < 0) {
				goto secure_err;
			}
			if(cipher_rsa_verify(public_key, (char *)&header->sig_header, hash_digest, key_len) < 0) {
				goto secure_err;
			}
			secure_done = 1;
		} else {
			if(pfile->image_id == 2) {
				ret = frame_send_respone(FDL_RESP_SIZE_ERR);
				if (ret)
					return ret;
				return -1;
			}
	}

	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;
	return 0;

secure_err:
	ret = frame_send_respone(FDL_RESP_SECURE_SIGNATURE_ERR);
	if (ret)
		return ret;
	return -1;
}

int cmd_execute(fdl_frame_t *pframe, void *arg)
{
	// int ret;
	struct fdl_file_info *pfile = (struct fdl_file_info *)arg;
	u32 start_addr = pfile->start_addr + IMG_HEADER_SIZE;

	if (pfile->recv_len != pfile->target_len)
		return -1;

	if (pfile->secure_en && !secure_done)
		return -1;

	/* response ACK in u-boot */
#if 0
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;
#endif

	/* no image header padded in pre-isp product */
	if (g_CurrChannel->channel == DL_CHAN_SPI)
		start_addr = pfile->start_addr;

	/* this is only for EDA simulation */
	//sim_trace(EDA_SIM_END);

	jump_to_execute(start_addr);

	return 0;
}

static int cmd_change_baudrate(fdl_frame_t *pframe, void *arg)
{
	u32 baudrate;
	int ret = 0;
	baudrate = *((u32 *)pframe->data);
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	if(g_CurrChannel->set_baudrate) {
		ret = g_CurrChannel->set_baudrate(g_CurrChannel, baudrate);
	}
	return ret;
}

int cmd_enable_quadmode(fdl_frame_t *pframe, void *arg)
{
	int ret;
	spi_slv_quad_mode(1);

	/* respond as quad mode */
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	return 0;
}

int cmd_slv_freq_boost(fdl_frame_t *pframe, void *arg)
{
	int ret;
	u32 tmp = 0;
	u8 sel = CLK_SPI_S_SEL_EPLL_125M;

	if (pframe->data_len) {
		tmp = ((u32 *)pframe->data)[0];
		sel = (tmp >= CLK_SPI_S_SEL_MAX) ? CLK_SPI_S_SEL_EPLL_125M : tmp;
	}

	spi_slv_freq_boost(sel);
	/* respond as quad mode */
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	return 0;
}

int cmd_update_cpu_freq(fdl_frame_t *pframe, void *arg)
{
	int ret;
	// cpu_clk_config();

	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	return 0;
}

static void cmdproc_register(CMD_TYPE cmd, cmd_handler_t handler, void *arg)
{
	//cmdproc_tab[cmd - FDL_CMD_TYPE_MIN].cmd = cmd;
	cmdproc_tab[cmd - FDL_CMD_TYPE_MIN].handler = handler;
	cmdproc_tab[cmd - FDL_CMD_TYPE_MIN].arg = arg;
}

void fdl_dl_init()
{
	ax_memset(cmdproc_tab, 0, sizeof(cmdproc_tab));

	cmdproc_register(FDL_CMD_CONNECT, cmd_connect, 0);
	cmdproc_register(FDL_CMD_START_TRANSFER, cmd_start_transfer, &g_file_info);
	cmdproc_register(FDL_CMD_START_TRANSFER_DATA, cmd_transfer_data, &g_file_info);
	cmdproc_register(FDL_CMD_START_TRANSFER_END, cmd_transfer_end, &g_file_info);
	cmdproc_register(FDL_CMD_EXECUTE, cmd_execute, &g_file_info);
	cmdproc_register(FDL_CMD_CHANGE_BAUD, cmd_change_baudrate, &g_file_info);
	cmdproc_register(FDL_CMD_UPDATE_FREQ, cmd_update_cpu_freq, 0);
	cmdproc_register(FDL_CMD_ENABLE_QUAD, cmd_enable_quadmode, 0);
	cmdproc_register(FDL_CMD_SLV_FREQ_BOOST, cmd_slv_freq_boost, 0);
}

int fdl_dl_entry()
{
	int ret;
	CMD_TYPE cmd;
	fdl_frame_t *pframe;

	fdl_dl_init();

	while (1) {
		pframe = frame_get(10000);
		if (!pframe) {
			ret = -1;
			break;
		}

		cmd = pframe->cmd_index;
		ret = CMD_HANDLER(cmd, pframe);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

void fw_load_and_exec()
{
	int ret;
	if (g_CurrChannel->channel == DL_CHAN_USB || g_CurrChannel->channel == DL_CHAN_UART) {
		//respond version
		ret = frame_send_version();
		if (ret < 0)
			goto boot_trap;
	}
	info("enter fw_load_and_exec\r\n");
	/* this function should never return in normal case */
	fdl_dl_entry();
boot_trap:
	while (1);
}

