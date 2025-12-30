/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <common.h>
#include <command.h>
#include <time.h>
#include <blk.h>
#include <asm/io.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include "ax_gzipd_reg.h"
#include "ax_gzipd_api.h"

#define AX_GZIPD_FALSE 0
#define AX_GZIPD_TRUE  1

#define FIFO_DEPTH          (16)
#define SAMPLE_TILE_SIZE    (4 * 16 * 1024)

#define IMAGE_COMPRESSED_PADDR  0x40800000
#define IMAGE_COMPRESSED_SIZE   0x2000000
#define IMAGE_DECCOM_PADDR      (IMAGE_COMPRESSED_PADDR + IMAGE_COMPRESSED_SIZE)
#define IMAGE_DECCOM_SIZE       0x8000000

extern int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t *info);
static struct blk_desc *blk_dev_desc = NULL;
static disk_partition_t part_info;

int gzipd_get_kernel_image_from_emmc(char **image)
{
	u64 rd_blkcnt_lb_kernel;
	struct blk_desc *blk_dev_desc = NULL;
	disk_partition_t part_info;
	u32 ret = 0;
	char *kernel_load_addr = (char *)IMAGE_COMPRESSED_PADDR;

	blk_dev_desc = blk_get_dev("mmc", 0);
	if (!blk_dev_desc) {
		AX_GZIP_LOG_ERR("get mmc dev fail\n");
		return -1;
	}

	ret = get_part_info(blk_dev_desc, "kernel", &part_info);
	if (ret == -1) {
		AX_GZIP_LOG_ERR("%s: get kernel partition info fail\n", __FUNCTION__);
	}

	printf("mmc part_info.start = %ld, .size = %ld, .blksize = %ld\n", part_info.start, part_info.size, part_info.blksz);
	memset(kernel_load_addr, 0, part_info.blksz * part_info.size);

	rd_blkcnt_lb_kernel = blk_dread(blk_dev_desc, part_info.start, part_info.size, kernel_load_addr);
	if (rd_blkcnt_lb_kernel != part_info.size) {
		AX_GZIP_LOG_ERR("do_axera_boot get kernel image fail\n");
		return -1;
	}
	*image = kernel_load_addr;
	return 0;
}

static int gzipd_get_kernel_part_info(struct blk_desc **dev_desc, disk_partition_t *kernel_part_info)
{
	u32 ret = 0;
	struct blk_desc *l_dev_desc;

	l_dev_desc = blk_get_dev("mmc", 0);   //EMMC_DEV_ID = 0
	if (!(l_dev_desc)) {
		AX_GZIP_LOG_ERR("get mmc dev fail\n");
		return -1;
	}

	ret = get_part_info(l_dev_desc, "kernel", kernel_part_info);
	if (ret == -1) {
		AX_GZIP_LOG_ERR("%s: get kernel partition info fail\n", __FUNCTION__);
		return -1;
	}
	*dev_desc = l_dev_desc;
	printf("mmc part_info.start = %ld, .size = %ld , .blksize = %ld\n",
			kernel_part_info->start, kernel_part_info->size, kernel_part_info->blksz);
	return 0;
}

static int gzipd_read_kernel_from_emmc(struct blk_desc *dev_desc, long start, long size, void *buff)
{
	u64 read_done_cnt;

	read_done_cnt = blk_dread(dev_desc, start, size, buff);
	if (read_done_cnt != size) {
		AX_GZIP_LOG_ERR("do_axera_boot get kernel image read by step block fail, ret = %lld\n", read_done_cnt);
		return -1;
	}

	return read_done_cnt;
}

static int do_gzipd_test(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	gzipd_header_info_t header_info;
	void *img_compressed_addr;
	u32 tile_cnt, idx;
	u32 last_tile_size = 0;
	ulong tile_addr;
	void *tiles_addr_start;
	void *tiles_addr_end;

	gzipd_dev_init();

	ret = gzipd_get_kernel_image_from_emmc((char **)&img_compressed_addr);
	if (ret) {
		AX_GZIP_LOG_ERR("get kernel_image from emmc error\n");
		return -1;
	}

	ret = gzipd_dev_get_header_info(img_compressed_addr, &header_info);
	if (ret) {
		AX_GZIP_LOG_ERR("get header info error\n");
		return -1;
	}

#ifdef AX_GZIPD_BYPASS_EN
	header_info.osize = header_info.isize;
#endif

	printf("header_info.blk_num = %d, isize = %d, osize = %d \n",
			header_info.blk_num, header_info.isize, header_info.osize);

	gzipd_dev_cfg(64 * 1024, (void *)IMAGE_DECCOM_PADDR,
					header_info.isize, header_info.osize, header_info.blk_num, &tile_cnt, &last_tile_size);

	tile_addr = (long)img_compressed_addr + sizeof(header_info);

	printf("tile_cnt = %d, last_tile_size = %d\n", tile_cnt, last_tile_size);
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
	if (gzipd_dev_wait_complete_finish()) {
		printf("De-compress the whole gzip file Fail \n");
	} else {
		printf("finish de-compress the whole gzip file Success\n");
	}

	return 0;
}

int do_gzipd_test_v2(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	gzipd_header_info_t header_info;
	void *img_compressed_addr;
	u32 tile_cnt, idx;
	u32 last_tile_size = 0;
	ulong tile_addr;
	u32 read_emmc_data_pos = 0;
	u32 blkscnt = 0;
	u32 start_pos;
	u32 total_blks;
	u32 blks;
	void *img_buf_pos;
	void *tiles_addr_start;
	void *tiles_addr_end;
	int need_read_data;

	gzipd_dev_init();

	if(gzipd_get_kernel_part_info(&blk_dev_desc, &part_info)) {
		AX_GZIP_LOG_ERR("get kernel partion info fail\n");
		return -1;
	}

	start_pos = part_info.start;
	total_blks = part_info.size;
	blks = SAMPLE_TILE_SIZE / part_info.blksz;
	img_buf_pos = (void *)IMAGE_COMPRESSED_PADDR;
	blkscnt = blks * FIFO_DEPTH *4;
	ret = gzipd_read_kernel_from_emmc(blk_dev_desc, start_pos, blkscnt, img_buf_pos);
	if (ret == -1) {
		AX_GZIP_LOG_ERR("read FIFO_DEPTH num of TILE of data from emmc fail\n");
		return -1;
	}
	read_emmc_data_pos += blkscnt * part_info.blksz;

	img_compressed_addr = img_buf_pos;
	start_pos += blkscnt;
	img_buf_pos += blkscnt * part_info.blksz;
	ret = gzipd_dev_get_header_info(img_compressed_addr, &header_info);
	if (ret) {
		AX_GZIP_LOG_ERR("get header info error\n");
		return -1;
	}

#ifdef AX_GZIPD_BYPASS_EN
	header_info.osize = header_info.isize;
#endif
	printf("header_info.blk_num = %d, isize = %d, osize = %d \n",
			header_info.blk_num, header_info.isize, header_info.osize);

	gzipd_dev_cfg(SAMPLE_TILE_SIZE, (void *)IMAGE_DECCOM_PADDR,
					header_info.isize, header_info.osize, header_info.blk_num, &tile_cnt, &last_tile_size);

	tile_addr = (long)img_compressed_addr + sizeof(header_info);

	printf("tile_cnt = %d, last_tile_size = %d\n", tile_cnt, last_tile_size);
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
		if ((long)tiles_addr_end - tile_addr <= read_emmc_data_pos) {
			ret = gzipd_dev_run(tiles_addr_start, tiles_addr_end, &run_num);
			if (ret) {
				return -1;
			}
			idx += run_num;
		} else {
			AX_GZIP_LOG_DBG("tiles_addr_end = %lld, tile_addr = %lld, diff = %p, read_emmc_data_pos = %d \n",
					(long)tiles_addr_end, tile_addr, tiles_addr_end - tile_addr, read_emmc_data_pos);
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
				if (read_emmc_data_pos > part_info.blksz * part_info.size) {
					AX_GZIP_LOG_ERR(" read the end of the whole z20e file's data\n");
					break;
				}
				blkscnt = blks * FIFO_DEPTH / 2;
				if (total_blks - start_pos < blkscnt) {
					blkscnt = total_blks - start_pos;
				}
				AX_GZIP_LOG_DBG("read emmc data, start = %ld, blkscnt = %ld \n", start_pos, blkscnt);
				ret = gzipd_read_kernel_from_emmc(blk_dev_desc, start_pos, blkscnt, img_buf_pos);
				if (ret == -1) {
					AX_GZIP_LOG_ERR("read blkscnt [%d] data from emmc fail\n", blkscnt);
					return -1;
				}
				start_pos += blkscnt;
				img_buf_pos += blkscnt * part_info.blksz;
				read_emmc_data_pos += blkscnt * part_info.blksz;
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
	if (gzipd_dev_wait_complete_finish()) {
		printf("De-compress the whole gzip file Fail \n");
	} else {
		printf("finish de-compress the whole gzip file success\n");
	}

	return 0;
}

U_BOOT_CMD(
	gzipd_test, 1, 0, do_gzipd_test,
	"ax620e gzipd test",
	"decompress kernel image inside EMMC"
);
U_BOOT_CMD(
	gzipd_test_v2, 1, 0, do_gzipd_test_v2,
	"ax620e gzipd test_v2",
	"decompress kernel image inside EMMC in mode of pipeline"
);
