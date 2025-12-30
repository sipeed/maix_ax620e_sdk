/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include "cmn.h"
#include "boot.h"
#include "ax_gzipd_api.h"
#include "crc32_mpeg2.h"
#include "spinor.h"
#include "ax_log.h"
#include "ax_common.h"
#include "dma.h"
#include "spinand.h"
#include "load_rootfs.h"

static int flash_clk_array[] = {200000000, 50000000, 25000000, 6000000};

#define FLASH_BOOT_MASK	        (0x7 << 1)

#if defined(AX620E_SUPPORT_SD)
static FATFS fs;
static FIL fil;
static u8 fatfs_mounted = 0;
#endif

extern u8 curr_cs;
u32 current_bus_width = 1;
u32 current_clock = 0;
extern struct pci_msg_reg *reg;

static int flash_read(u32 flash_type, char *buf, u32 flash_addr, int size)
{
    int ret;

	switch (flash_type) {
	case FLASH_NOR:
		ret = spinor_read(flash_addr, size, (u8 *)buf);
		if (ret < 0) {
			AX_LOG_ERROR("flash_read nor fail ret: %d", ret);
			return ret;
		}
		break;
	case FLASH_NAND_2K:
	case FLASH_NAND_4K:
		ret = spinand_read(flash_addr, size, buf);
		if (ret < 0) {
			AX_LOG_ERROR("flash_read nand fail ret: %d", ret);
			return ret;
		}
		break;
	default:
		return -1;
	}
    return size;
}

// u32 calc_word_chksum(int *data, int size)
// {
//     int count = size / 4;
//     int i;
//     u32 sum = 0;
//     if(axi_dma_word_checksum(&sum, (u64)data, size))
//     {
//         /* if dma calulate failed, use cpu calu. */
//         sum = 0;
//         for(i = 0; i < count; i++) {
//             sum += data[i];
//         }
//     }
//     return sum;
// }

#ifdef SUPPPORT_GZIPD
#if defined(AX620E_EMMC)
#define IMAGE_COMPRESSED_PADDR		0x58000000
#define SAMPLE_TILE_SIZE    (4 * 16 * 1024)
#endif
#if defined(AX620E_NAND)
#define IMAGE_COMPRESSED_PADDR		ROOTFS_COMPRESSED_ADDR
#define SAMPLE_TILE_SIZE    (64 * 1024 * 1024)
#endif
#if defined(AX620E_NOR)
#define IMAGE_COMPRESSED_PADDR		ROOTFS_COMPRESSED_ADDR
#define SAMPLE_TILE_SIZE    (22 * 1024 * 1024)
#endif
#if defined(AX620E_SUPPORT_SD)
#define IMAGE_COMPRESSED_PADDR		0x58000000
#define SAMPLE_TILE_SIZE    (4 * 16 * 1024)
#endif
#define GZIPD_LARGE_BUF
#define AX_GZIPD_FALSE 0
#define AX_GZIPD_TRUE  1
#define FIFO_DEPTH  1
//  #define GZIP_DATA_CRC_CHECK

#ifdef GZIPD_LARGE_BUF
static int gzip_pipeline_flash_read(u32 flash_type, char *buf, u32 flash_addr, int size)
{
	int ret;
	u32 rdlen;
	gzipd_header_info_t header_info;
	u32 tile_cnt;
	u32 last_tile_size = 0;
	void *tiles_addr_start;
	void *tiles_addr_end;

	u32 flash_read_pos;
	u32 flash_read_len;
	void *flash_buf_ptr;

	u32 maybe_queued_num;
	u32 queued_num;
	u32 expected_queue_num;

	bool read_fisrt_tile = false;
#ifdef GZIP_DATA_CRC_CHECK
	u32 crc_val;
#endif

	flash_read_pos = flash_addr;
#if defined(AX620E_NAND)
	flash_buf_ptr = (void *)(IMAGE_COMPRESSED_PADDR + sizeof(struct img_header));
#else
	flash_buf_ptr = (void *)IMAGE_COMPRESSED_PADDR;
#endif
	if (size < SAMPLE_TILE_SIZE + sizeof(header_info))
		flash_read_len = size;
	else
		flash_read_len = SAMPLE_TILE_SIZE + sizeof(header_info);

#if defined(AX620E_NAND)
	rdlen = flash_read_len;
#else
	rdlen = flash_read(flash_type, flash_buf_ptr, flash_read_pos, flash_read_len);
#endif
	if (rdlen < 0 || rdlen < flash_read_len ) {
		debug_trace(GZIP_FLASH_READ_HEAD_FAILED);
		AX_LOG_ERROR("read data from emmc fail, len = %d\r\n", rdlen);
		return -1;
	}

	read_fisrt_tile = true;
	flash_read_pos += rdlen;

	ret = gzipd_dev_get_header_info(flash_buf_ptr, &header_info);
	if (ret) {
		debug_trace(GZIP_GET_HEAD_INFO_ERROR);
		AX_LOG_ERROR("get header info error\n");
		return -1;
	}

	AX_LOG_INFO("riscv,header_info.blk_num = %d, isize = %d, osize = %d \r\n",
			header_info.blk_num, header_info.isize, header_info.osize);

	gzipd_dev_cfg(SAMPLE_TILE_SIZE, (void *)buf,
					header_info.isize, header_info.osize, header_info.blk_num, &tile_cnt, &last_tile_size);
	AX_LOG_INFO("riscv, tile_cnt = %d, last_tile_size = 0x%x\r\n", tile_cnt, last_tile_size);

	tiles_addr_start = flash_buf_ptr + sizeof(header_info);
	flash_buf_ptr += rdlen;

last_tile_run:
	if (tile_cnt == 0) {
		AX_LOG_INFO("gzip [0x%08x, 0x%08x]", tiles_addr_start, last_tile_size);
		if (gzipd_dev_run_last_tile((void *)tiles_addr_start, last_tile_size)) {
			debug_trace(GZIP_RUN_LAST_TILE_ERROR);
			AX_LOG_ERROR(" run last tile error\r\n");
			return -1;
		}
		goto complete_finish;
	}

	tile_cnt -= 1;
run:
	if (tile_cnt == 0) {
		goto last_tile_run;
	}
	maybe_queued_num = FIFO_DEPTH;
	if (tile_cnt < FIFO_DEPTH) {
		maybe_queued_num = tile_cnt;
	}
	tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * maybe_queued_num ;
	flash_read_len = tiles_addr_end - tiles_addr_start;
	if (read_fisrt_tile) {
		flash_read_len -= SAMPLE_TILE_SIZE;
		read_fisrt_tile = false;
	}
	if (tile_cnt == 0) {
		flash_read_len = last_tile_size;
	}

	if (check_gzipd_status() == gzipd_error) {
		debug_trace(GZIP_CHECK_GZIP_ST_ERROR);
		return -1;
	}

	if (flash_read_len == 0) {
		goto run;
	}

#if defined(AX620E_NAND)
	ret = flash_read_len;
#else
	ret = flash_read(flash_type, flash_buf_ptr, flash_read_pos, flash_read_len);
#endif
	if (ret < 0 || ret < flash_read_len ) {
		debug_trace(GZIP_FLASH_READ_IMAGE_FAILED);
		AX_LOG_ERROR("read len[%d] data from flash[0x%x] fail, rdlen = %d\r\n", flash_read_len, flash_read_pos, ret);
		return -1;
	}
	flash_buf_ptr  += flash_read_len;
	flash_read_pos += flash_read_len;

	expected_queue_num = maybe_queued_num;
	queued_num = 0;
	do {
		if (queued_num < expected_queue_num ) {
			expected_queue_num -= queued_num;
		}
		AX_LOG_INFO("gzip [0x%08x, 0x%08x]", tiles_addr_start, tiles_addr_end);
		ret = gzipd_dev_run(tiles_addr_start, tiles_addr_end, &queued_num);
		if (ret) {
			debug_trace(GZIP_DEV_RUN_ERROR);
			return -1;
		}

		tiles_addr_start += queued_num * SAMPLE_TILE_SIZE;
	} while (queued_num != expected_queue_num);

	tile_cnt -= maybe_queued_num;
	goto run;

complete_finish:
	if (gzipd_dev_wait_complete_finish()) {
		AX_LOG_ERROR("fail de-compress the gzip file\r\n");
	#ifdef GZIP_DATA_CRC_CHECK
		crc_val = do_crc_table(flash_buf_ptr, flash_read_len);
		if (crc_val != header_info.icrc32) {
			AX_LOG_ERROR("gzip data CRC[0x%x] check error, original CRC = 0x%x\n", crc_val, header_info.icrc32);
		}
	#endif
		debug_trace(GZIP_WATI_FINISH_ERROR);
		return -1;
	}
	debug_trace(GZIP_DECOMPRESS_SUCCESS);
	AX_LOG_INFO("finish de-compress the whole gzip file success\r\n");
	return 0;
}
#else
struct ping_pong_buf {
	void 	*buf_addr;
	u32		length;
	u32 	status;
} gzip_pingpong_buf[2];
#define NOT_READY 	0
#define READDY		1
#define AX_ALIGN_UP(x, a)            ( ( ((x) + ((a) - 1) ) / a ) * a )

static int gzip_pipeline_flash_read_with_p2pbuf(u32 flash_type, char *buf, u32 flash_addr, int size)
{
    int ret;
	gzipd_header_info_t header_info;
	void *gzipd_base_addr;
	u32 left_tile_num = 0;
	u32 last_tile_size = 0;
	u32 buf_idx = 0;
	void *tile_buf_addr;
	u32 flash_readlen;
	u32 flash_read_cur;
	u32	rdlen;
#ifdef GZIP_DATA_CRC_CHECK
	u32 tile_crc = 0xFFFFFFFF;
	u32 file_crc;
#endif
	/* offset with header_info of gzip file */
	gzipd_base_addr = (void *)IMAGE_COMPRESSED_PADDR;
	gzip_pingpong_buf[0].buf_addr = (void *)AX_ALIGN_UP((unsigned long)gzipd_base_addr + sizeof(header_info), 8);
	gzip_pingpong_buf[0].length = AX_ALIGN_UP(SAMPLE_TILE_SIZE, 8);
	gzip_pingpong_buf[0].status = NOT_READY;
	gzip_pingpong_buf[1].buf_addr = gzip_pingpong_buf[0].buf_addr + gzip_pingpong_buf[0].length;
	gzip_pingpong_buf[1].length = SAMPLE_TILE_SIZE;
	gzip_pingpong_buf[1].status = NOT_READY;

	flash_readlen = SAMPLE_TILE_SIZE + sizeof(header_info);
	if (size < flash_readlen) {
		flash_readlen = size;
	}
	flash_read_cur = flash_addr;

	do {
		struct ping_pong_buf *cur_tile_buf;
		cur_tile_buf = &gzip_pingpong_buf[buf_idx & 0x1];
		tile_buf_addr = cur_tile_buf->buf_addr;
		if (buf_idx == 0) {
			tile_buf_addr = tile_buf_addr - sizeof(header_info);
		}
		AX_LOG_INFO("read flash idx[%d] pos = 0x%x, readlen = 0x%x", buf_idx, flash_read_cur, flash_readlen);
		rdlen = flash_read(flash_type, tile_buf_addr, flash_read_cur, flash_readlen);
		if (rdlen < 0 || rdlen < flash_readlen) {
			AX_LOG_ERROR("flash read fail, expected readlen =%d, but rdlen = %d", flash_readlen, rdlen);
			return -1;
		}
#ifdef GZIP_DATA_CRC_CHECK
		tile_crc = do_crc_table_block(tile_buf_addr, flash_readlen, &tile_crc);
#endif
		cur_tile_buf->status = READDY;
		flash_read_cur += rdlen;
		if (buf_idx == 0 && cur_tile_buf->status == READDY) {
			ret = gzipd_dev_get_header_info(tile_buf_addr, &header_info);
			if (ret) {
				AX_LOG_ERROR("get header info error\n");
				return -1;
			}

			AX_LOG_INFO("riscv,header_info.blk_num = %d, isize = %d, osize = %d \r\n",
					header_info.blk_num, header_info.isize, header_info.osize);

			gzipd_dev_cfg(SAMPLE_TILE_SIZE, (void *)buf,
							header_info.isize, header_info.osize, header_info.blk_num, &left_tile_num, &last_tile_size);

			AX_LOG_INFO("riscv, left_tile_num = %d, last_tile_size = 0x%x\r\n", left_tile_num, last_tile_size);

		}
		buf_idx++;

		if (gzipd_is_bus_busy()) {
			AX_LOG_ERROR("gzipd still busying  , left_tile_num = %d", left_tile_num);
			return -1;
		}

		if (left_tile_num == 1) {
			AX_LOG_INFO("gzip [0x%08x, 0x%08x]", cur_tile_buf->buf_addr, last_tile_size);
			if (gzipd_dev_run_last_tile((void *)cur_tile_buf->buf_addr, last_tile_size)) {
				AX_LOG_ERROR(" run last tile error\r\n");
				return -1;
			} else {
				break;
			}
		} else {
			u32 num;
			if (gzipd_dev_run(cur_tile_buf->buf_addr, cur_tile_buf->buf_addr + SAMPLE_TILE_SIZE, &num)) {
				AX_LOG_ERROR("gzipd_dev_run error\r\n");
				return -1;
			}
		}

		if (left_tile_num > 1) {
			flash_readlen = SAMPLE_TILE_SIZE;
			if (left_tile_num == 2) {
				flash_readlen = last_tile_size;
			}
			left_tile_num--;
		}

	} while (left_tile_num);

	if (gzipd_dev_wait_complete_finish()) {
		AX_LOG_ERROR("fail de-compress the gzip file\r\n");
	#ifdef GZIP_DATA_CRC_CHECK
		file_crc = do_crc_table_block(tile_buf_addr, flash_readlen, &tile_crc);
		if (file_crc != header_info.icrc32) {
			AX_LOG_ERROR("gzip data CRC[0x%x] check error, original CRC = 0x%x\n", file_crc, header_info.icrc32);
		}
	#endif
		return -1;
	}
	AX_LOG_INFO("finish de-compress the whole gzip file success\r\n");
	return 0;
}
#endif
#endif

static int flash_init(u32 flash_type, u32 clk, u32 bus_width)
{
    int ret = 0;

	switch (flash_type) {
	case FLASH_NOR:
		if (0 == curr_cs) {
			AX_LOG_DGB("nor CS0 BOOT");
		} else {
			AX_LOG_DGB("nor CS1 BOOT");
		}
		ret = spinor_init(clk, bus_width);
		if (ret < 0)
			return ret;
		break;
	case FLASH_NAND_4K:
		if (0 == curr_cs) {
			AX_LOG_DGB("nand 4k CS0 BOOT");
		} else {
			AX_LOG_DGB("nand 4k CS1 BOOT");
		}
		ret = spinand_init(clk, bus_width);
		if (ret < 0)
			return ret;
		break;
	case FLASH_NAND_2K:
		if (0 == curr_cs) {
			AX_LOG_DGB("nand 2k CS0 BOOT");
		} else {
			AX_LOG_DGB("nand 2k CS1 BOOT");
		}
		ret = spinand_init(clk, bus_width);
		if (ret < 0)
			return ret;
		break;
	default:
		return -1;
	}
    return ret;
}

int read_img_header(u32 flash_type, struct img_header *header, struct img_header *boot_header, struct boot_image_info *img_info)
{
    u32 flash_addr, flash_addr_bk;
    u32 bus_width = BUS_WIDTH_8;
    int sel_clk;
    /* read image header, try different bus frequency if failed
     * emmc/sd use 12Mhz
     */
    if (!boot_header || !header || !img_info)
        return -1;
    flash_addr = img_info->boot_header_flash_addr;
    flash_addr_bk = img_info->boot_header_flash_bk_addr;
    AX_LOG_DGB("read_img_header: flash_addr = 0x%x flash_addr_bk = 0x%x", flash_addr, flash_addr_bk);

    switch(flash_type) {
        case FLASH_NAND_4K:
        case FLASH_NOR:
        case FLASH_NAND_2K:
            if (img_info->boot_index >= 2)
                curr_cs = 1;
            else
                curr_cs = 0;
        default:
            bus_width = BUS_WIDTH_4; // use 4bit
            sel_clk = 1;
            break;
    }
#if defined(AX620E_NAND)
	/* decode nand org info */
	if (flash_type  == FLASH_NAND_2K || flash_type  == FLASH_NAND_4K) {
		spinand_page_size_shift = (header->nand_nor_cfg & GENMASK(1, 0)) + 11;
		spinand_blk_size_shift =  ((header->nand_nor_cfg & GENMASK(4, 2)) >> 2) + 16;
		spinand_oob_size_shift = ((header->nand_nor_cfg & GENMASK(6, 5)) >> 5) + 6;
		spinand_oob_len = (header->nand_nor_cfg & BIT(7)) ? 2 : 1;
		spinand_planes = (header->nand_ext_cfg & GENMASK(1, 0));
		spinand_rx_sample_delay[0] = header->nand_rx_sample_delay[0];
		spinand_rx_sample_delay[1] = header->nand_rx_sample_delay[1];
		spinand_rx_sample_delay[2] = header->nand_rx_sample_delay[2];
		spinand_phy_setting[0] = header->nand_phy_setting[0];
		spinand_phy_setting[1] = header->nand_phy_setting[1];
		spinand_phy_setting[2] = header->nand_phy_setting[2];
	}
#endif

#if defined(AX620E_NOR)
	if (flash_type == FLASH_NOR) {
    	spinor_qe_rdsr = (header->nand_nor_cfg & GENMASK(15, 8)) >> 8;
    	spinor_qe_wrsr = (header->nand_nor_cfg & GENMASK(23, 16)) >> 16;
    	spinor_qe_bit = (header->nand_nor_cfg & GENMASK(31, 24)) >> 24;
    	spinor_qe_cfg_parsred = 1;
    	spinor_rx_sample_delay[0] = header->nor_rx_sample_delay[0];
    	spinor_rx_sample_delay[1] = header->nor_rx_sample_delay[1];
    	spinor_rx_sample_delay[2] = header->nor_rx_sample_delay[2];
    	spinor_phy_setting[0] = header->nor_phy_setting[0];
    	spinor_phy_setting[1] = header->nor_phy_setting[1];
    	spinor_phy_setting[2] = header->nor_phy_setting[2];
	}
#endif

//retry_read_bk:
#if 1
    if (flash_init(flash_type, flash_clk_array[sel_clk], bus_width) < 0) {
        debug_trace(FLASH_INIT_FAILED);
        AX_LOG_ERROR("spl flash init failed");
        return BOOT_FLASH_INIT_FAIL;
    }
#endif
    if(flash_read(flash_type, (char *)boot_header, flash_addr, sizeof(struct img_header)) < 0) {
        debug_trace(FLASH_READ_HEAD_FAILED);
        AX_LOG_ERROR("spl flash read failed");
        return BOOT_FLASH_READ_FAIL;
    }

    // if(verify_img_header((struct img_header *)boot_header) < 0) {
    //     AX_LOG_INFO("spl verify header failed\r\n");

    //     if(flash_addr == flash_addr_bk) {
    //         return BOOT_READ_HEADER_FAIL;
    //     } else {
    //         flash_addr = flash_addr_bk;
    //         goto retry_read_bk;
    //     }
    // }
    debug_trace(READ_HEAD_SUCCESS);
    return 0;
}

int read_image_data(u32 flash_type, struct img_header *boot_header, struct boot_image_info *img_info)
{
    if (!boot_header || !img_info)
        return -1;
    u32 flash_addr = img_info->boot_flash_addr;
    char *img_addr = (char *)((u32)boot_header + sizeof(struct img_header));
    // int check_enable = (boot_header->capability & IMG_CHECK_ENABLE) ? 1 : 0;

    debug_trace(READ_IMAGE_START);
//retry_read_bk:
#ifdef SUPPPORT_GZIPD
#ifdef GZIPD_LARGE_BUF
    if(gzip_pipeline_flash_read(flash_type, img_addr, flash_addr, boot_header->img_size) < 0) {
#else
    if(gzip_pipeline_flash_read_with_p2pbuf(flash_type, img_addr, flash_addr, boot_header->img_size) < 0) {
#endif
        AX_LOG_ERROR("spl flash read failed");
        return BOOT_FLASH_READ_FAIL;
    }
#else
    /* read image data */
    if (flash_read(flash_type, img_addr, flash_addr, boot_header->img_size) < 0) {
        AX_LOG_ERROR("read img data flash_read faield");
        return BOOT_FLASH_READ_FAIL;
    }
#endif
    // if (check_enable) {
    //         /* read backup if checksum failed */
    //         if (calc_word_chksum((int *)(img_addr), boot_header->img_size) != boot_header->img_check_sum) {
    //                 AX_LOG_INFO("read img data calc_word_chksum faield\r\n");
    //                 if(flash_addr == img_info->boot_flash_bk_addr) {
    //                         return BOOT_IMG_CHECK_FAIL;
    //                 } else {
    //                         flash_addr = img_info->boot_flash_bk_addr;
    //                         goto retry_read_bk;
    //                 }
    //         }
    // }
    debug_trace(READ_IMAGE_SUCCESS);
    return (int)(u64)(uintptr_t)img_addr;
}

static void get_image_info(u32 flash_type, int boot_index, boot_image_info_t *image_info, u32 flash_ops, u32 flash_bk_ops)
{
    // image_info->img_type = img_type;
    image_info->boot_index = boot_index;
    image_info->boot_header_flash_addr = flash_ops;
    image_info->boot_header_flash_bk_addr = flash_bk_ops;
    image_info->boot_flash_addr = flash_ops + sizeof(struct img_header);
    image_info->boot_flash_bk_addr = flash_bk_ops + sizeof(struct img_header);
}

int boot_process(u32 flash_type, int boot_index, u32 flash_ops, u32 flash_bk_ops, long ram_ops)
{
    struct img_header *header = (struct img_header *)SPL_IMG_HEADER_BASE;
    boot_image_info_t image_info;
    struct img_header *boot_header = (struct img_header *)ram_ops;
    int ret;

    debug_trace(BOOT_PROCESS);
    AX_LOG_DGB("boot_index = %x", boot_index);
    get_image_info(flash_type, boot_index, &image_info, flash_ops, flash_bk_ops);

    ret = read_img_header(flash_type, header, boot_header, &image_info);
    if(ret < 0) {
        AX_LOG_ERROR("read img header failed");
        return ret;
    }

#if defined(AX620E_NAND) && defined (SUPPPORT_GZIPD)
    AX_LOG_DGB("flash_type=%d, flash_ops=0x%x, flash_bk_ops=0x%x, img_size=0x%x, ram_ops=0x%lx",
		flash_type, flash_ops, flash_bk_ops, boot_header->img_size, ram_ops);
    if ((FLASH_NAND_2K == flash_type) || (FLASH_NAND_4K == flash_type)) {
        spinand_read(flash_ops, boot_header->img_size + sizeof(struct img_header), (char *)IMAGE_COMPRESSED_PADDR);
    }
#endif
    ret = read_image_data(flash_type, boot_header, &image_info);
    if(ret < 0) {
        AX_LOG_ERROR("read img data failed");
        return ret;
    }

    return ret;
}

u32 get_boot_index(void)
{
    /* The valid value is 0, 1, 2, 3 */
    u32 chipmode;
    chipmode = ax_readl(TOP_CHIPMODE_GLB_BACKUP0);
    return chipmode & 0x3;
}

u32 get_boot_mode(u32 chip_mode)
{
	/* EMMC UDA:0, EMMC BOOT 8BIT_50M_768K:1,  NAND 2K:3, EMMC BOOT 4BIT_25M_768K:4, NAND 4K:5, EMMC BOOT 4BIT_25M_128K:6, NOR:7 */
	return  (chip_mode & FLASH_BOOT_MASK) >> 1;
}

int flash_boot(u32 flash_ops, u32 flash_bk_ops, long ram_ops)
{
    int ret = 0;
    u32 boot_index = 0;
    char chip_mode, flash_type;

#if 1
    axi_dma_hw_init();
#ifdef SUPPPORT_GZIPD
    gzipd_dev_init();
#endif
#endif
    chip_mode = ax_readl(TOP_CHIPMODE_GLB_SW);
    flash_type = get_boot_mode(chip_mode);

    boot_index = get_boot_index();//bit31 flag,  bit[1:0] index
    ret = boot_process(flash_type, boot_index, flash_ops, flash_bk_ops, ram_ops);
    if(ret < 0) {
        AX_LOG_ERROR("boot_process rootfs failed\r\n");
        return -1;
    }
    return 0;
}