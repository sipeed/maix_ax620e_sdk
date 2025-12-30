/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "cmn.h"
#include "../usb/printf.h"
#include "ax_gzipd_api.h"

#define AX_GZIP_LOG_ERR info
#define AX_GZIP_LOG_DBG info

#define AX_GZIPD_FALSE 0
#define AX_GZIPD_TRUE  1

#define FIFO_DEPTH          (16)
#define SAMPLE_TILE_SIZE    (4 * 16 * 1024)

#define IMAGE_COMPRESSED_PADDR 	0x50000000
#define IMAGE_DECCOM_PADDR 		0x40F00000

#ifndef GZIPD_PIPELINE_MODE


/* call ax_gzipd_test(0x40820000) to run this testcase */
int ax_gzipd_test(const char *pZIPData)
{
	int ret;
	gzipd_header_info_t header_info;
	void *img_compressed_addr;
	u32 tile_cnt, idx;
	u32 last_tile_size = 0;
	u64 tile_addr;
	void *tiles_addr_start;
	void *tiles_addr_end;

	gzipd_dev_init();

	// ret = gzipd_get_kernel_image_from_emmc((char **)&img_compressed_addr);
	// if (ret) {
	// 	AX_GZIP_LOG_ERR("get kernel_image from emmc error\n");
	// 	return -1;
	// }

	img_compressed_addr = (void *)pZIPData;
	ret = gzipd_dev_get_header_info(img_compressed_addr, &header_info);
	if (ret) {
		AX_GZIP_LOG_ERR("get header info error\n");
		return -1;
	}

#ifdef AX_GZIPD_BYPASS_EN
	header_info.osize = header_info.isize;
#endif

	info("header_info.blk_num = %d, isize = %d, osize = %d \n",
			header_info.blk_num, header_info.isize, header_info.osize);

	gzipd_dev_cfg(64 * 1024, (void *)IMAGE_DECCOM_PADDR,
					header_info.isize, header_info.osize, header_info.blk_num, &tile_cnt, &last_tile_size);

	tile_addr = (u64)img_compressed_addr + sizeof(header_info);

	info("tile_cnt = %d, last_tile_size = %d\n", tile_cnt, last_tile_size);
	if (tile_cnt == 0) {
		if (gzipd_dev_run_last_tile((void *)tile_addr, last_tile_size)) {
			AX_GZIP_LOG_ERR(" run last tile error\n");
			return -1;
		}
		goto complete_finish;
	}

	tiles_addr_start = (void *)tile_addr;
	tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * ((tile_cnt < 16? tile_cnt : 16) - 1) ;
	idx = 0;
	while (idx < tile_cnt - 1) {
		u32 run_num;
		ret = gzipd_dev_run(tiles_addr_start, tiles_addr_end, &run_num);
		if (ret) {
			return -1;
		}
		idx += run_num;

		u32 wait_loop = 100;
		while (wait_loop--) {
			int level = gzipd_dev_get_fifo_level();
			if (level < 16) {
				AX_GZIP_LOG_DBG("here read data from emmc ,level = %d\n", level);
				break;
			} else if(level == 16 && wait_loop % 10 == 0) {
				AX_GZIP_LOG_DBG("gzipd fifo is still full, please re-try[%d]\n", wait_loop);
			}
		}

		tiles_addr_start += run_num * SAMPLE_TILE_SIZE;
		if (tile_cnt - idx > 16 ) {
			tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * 16;
		} else {
			AX_GZIP_LOG_DBG("not full cmdq, tile_cnt = %d, idx = %d\n", tile_cnt, idx);
			tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * (tile_cnt - idx - 1);
		}
		AX_GZIP_LOG_DBG(" sampled gzipd test : idx = %d\n", idx);
	}

	if (gzipd_dev_run_last_tile(tiles_addr_end, last_tile_size)) {
		AX_GZIP_LOG_ERR("lastly gzipd run last tile error\n");
		return -1;
	}

complete_finish:
	gzipd_dev_wait_complete_finish();
	info("finish de-compress the whole gzip file success\n");

	return 0;
}

#else
/* int flash_read(u32 flash_type, char *buf, u32 flash_addr, int size)*/
int ax_gzipd_pipeline_test(u32 type, char *pDecData, u32 pEncData, u32 EncSize)
{
	int ret;
	gzipd_header_info_t header_info;
	void *img_compressed_addr;
	u32 tile_cnt, idx;
	u32 last_tile_size = 0;
	u64 tile_addr;
	u32 read_emmc_data_pos = 0;
	u32 blkscnt = 0;
	u32 start_pos;
	u32 rd_len;
	void *img_buf_pos;
	void *tiles_addr_start;
	void *tiles_addr_end;
	int need_read_data;

	gzipd_dev_init();

	start_pos = pEncData;
	img_buf_pos = (void *)IMAGE_COMPRESSED_PADDR;
	// ret = gzipd_read_kernel_from_emmc(blk_dev_desc, start_pos, blkscnt, img_buf_pos);
	rd_len = SAMPLE_TILE_SIZE * 2;
	ret = flash_read(type, img_buf_pos, pEncData, SAMPLE_TILE_SIZE * 2);
	if (ret < 0) {
		AX_GZIP_LOG_ERR("read data from emmc fail\n");
		return -1;
	}
	read_emmc_data_pos += rd_len;

	img_compressed_addr = img_buf_pos;
	start_pos += rd_len;
	img_buf_pos += rd_len;
	ret = gzipd_dev_get_header_info(img_compressed_addr, &header_info);
	if (ret) {
		AX_GZIP_LOG_ERR("get header info error\n");
		return -1;
	}

#ifdef AX_GZIPD_BYPASS_EN
	header_info.osize = header_info.isize;
#endif
	AX_GZIP_LOG_DBG("header_info.blk_num = %d, isize = %d, osize = %d \n",
			header_info.blk_num, header_info.isize, header_info.osize);

	gzipd_dev_cfg(SAMPLE_TILE_SIZE, (void *)IMAGE_DECCOM_PADDR,
					header_info.isize, header_info.osize, header_info.blk_num, &tile_cnt, &last_tile_size);

	tile_addr = (u64)img_compressed_addr + sizeof(header_info);

	info("tile_cnt = %d, last_tile_size = %d\n", tile_cnt, last_tile_size);
	if (tile_cnt == 0) {
		if (gzipd_dev_run_last_tile((void *)tile_addr, last_tile_size)) {
			AX_GZIP_LOG_ERR(" run last tile error\n");
			return -1;
		}
		goto complete_finish;
	}

	tiles_addr_start = (void *)tile_addr;
	tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * ((tile_cnt < 16? tile_cnt : 16) - 1) ;
	need_read_data = AX_GZIPD_FALSE;
	idx = 0;
	while (idx < tile_cnt - 1) {
		u32 run_num = 0;
		if ((u64)tiles_addr_end - tile_addr <= read_emmc_data_pos) {
			ret = gzipd_dev_run(tiles_addr_start, tiles_addr_end, &run_num);
			if (ret) {
				return -1;
			}
			idx += run_num;
		} else {
			AX_GZIP_LOG_DBG("tiles_addr_end = %lld, tile_addr = %lld, diff = %p, read_emmc_data_pos = %d \n",
					(u64)tiles_addr_end, tile_addr, tiles_addr_end - tile_addr, read_emmc_data_pos);
		}
		u32 wait_loop = 100;
		while (wait_loop--) {
			int level = gzipd_dev_get_fifo_level();
			if (level < 16 && need_read_data == AX_GZIPD_FALSE) {
				AX_GZIP_LOG_DBG("here input data to gzipd ,level = %d\n", level);
				if (read_emmc_data_pos < (idx + FIFO_DEPTH) * SAMPLE_TILE_SIZE) {
					need_read_data = AX_GZIPD_TRUE;
					continue;
				}

				tiles_addr_start += run_num * SAMPLE_TILE_SIZE;
				if (tile_cnt - idx > 16 ) {
					tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * 16;
				} else {
					AX_GZIP_LOG_DBG("not full cmdq, tile_cnt = %d, idx = %d\n", tile_cnt, idx);
					tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * (tile_cnt - idx - 1);
				}
				break;
			} else if((level == 16) || need_read_data == AX_GZIPD_TRUE) {
				AX_GZIP_LOG_DBG("read next data in emmc when gzipd cmdq full or need more data = %d\n", need_read_data);
				if (read_emmc_data_pos > EncSize) {
					AX_GZIP_LOG_ERR(" read the end of the whole z20e file's data\n");
					break;
				}

				rd_len = SAMPLE_TILE_SIZE * 2;
				if (EncSize - read_emmc_data_pos < rd_len) {
					rd_len = EncSize - read_emmc_data_pos;
				}
				AX_GZIP_LOG_DBG("read emmc data, start = %ld, blkscnt = %ld \n", start_pos, rd_len);
				// ret = gzipd_read_kernel_from_emmc(blk_dev_desc, start_pos, blkscnt, img_buf_pos);
				ret = flash_read(type, img_buf_pos, start_pos, rd_len);
				if (ret < 0) {
					AX_GZIP_LOG_ERR("read len[%d] data from emmc[0x%x] fail\n", rd_len, start_pos);
					return -1;
				}
				start_pos += rd_len;
				img_buf_pos += rd_len;
				read_emmc_data_pos += rd_len;
				need_read_data = AX_GZIPD_FALSE;
			}
		}

		AX_GZIP_LOG_DBG(" sampled gzipd test : idx = %d\n", idx);
	}

	if (gzipd_dev_run_last_tile(tiles_addr_end, last_tile_size)) {
		AX_GZIP_LOG_ERR("lastly gzipd run last tile error\n");
		return -1;
	}

complete_finish:
	gzipd_dev_wait_complete_finish();
	info("finish de-compress the whole gzip file success\n");

	return 0;
}

#endif