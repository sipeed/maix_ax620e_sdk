/*
 * (C) Copyright 2020 AXERA
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <mmc.h>
#include <mtd.h>
#include <blk.h>
#include <memalign.h>
#include <linux/sizes.h>
#include <linux/string.h>
#include <fdl_engine.h>
#include <fdl_frame.h>
#include <fdl_channel.h>
#include <image-sparse.h>
#include <asm/arch/boot_mode.h>
#include <configs/ax620e_common.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <jffs2/load_kernel.h>
#include <linux/mtd/spi-nor.h>
#include "../../legacy-mtd-utils.h"
#include "../secureboot/secureboot.h"
#include "../cipher/ax_cipher_api.h"
#include <axera_update.h>

#define AXERA_DISK_GUID_STR      "12345678-1234-1234-1234-000000000000"
#define SIZE_SPL_IMG_SIZE				(111 * 1024)

extern FDL_ChannelHandler_T *g_CurrChannel;
extern int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t * info);
extern void reboot(void);
typedef int (*cmd_handler_t) (fdl_frame_t * pframe, void *arg);

struct fdl_cmdproc {
	//CMD_TYPE cmd;
	cmd_handler_t handler;
	void *arg;
};

#ifdef SPI_DUAL_CS
extern struct sf1_part_info g_sf1_part_info[SF1_MAX_PART_NUM];
extern char sf1_parts[256];
extern int get_sf1_part_info(char *part_name);
#endif
extern struct boot_mode_info boot_info_data;

#define CMD_HANDLER(cmd, pframe) cmdproc_tab[cmd - FDL_CMD_TYPE_MIN].handler\
						(pframe, cmdproc_tab[cmd - FDL_CMD_TYPE_MIN].arg)
#define FILE_START_ADDR_SIZE             (sizeof(u64))
#define FILE_TARGET_LEN_SIZE             (sizeof(u64))

extern char lastXferPart[32];
extern char lastLoadPart[32];

extern u32 xferPartCnt;
extern loff_t partOff;
extern u_long dl_buf_addr;
extern u_long dl_buf_size;
static int sparse_download_enable = 0;

struct sparse_storage sparse;
struct fdl_cmdproc cmdproc_tab[FDL_CMD_TYPE_MAX - FDL_CMD_TYPE_MIN];
struct fdl_file_info g_file_info;
struct fdl_read_info g_read_info;
extern int common_get_part_info(char * part_name, u64 * addr, u64 * len);
extern int common_raw_read(char *part_name, u64 addr, u64 len, char *data);
extern int common_raw_write(char * part_name, u64 addr, u64 len, char * data);
extern int common_raw_erase(char * part_name, u64 addr, u64 len);
extern int sparse_info_init(struct sparse_storage *info, const char *name);
extern int write_sparse_img(struct sparse_storage *info, const char *part_name, void *data, ulong * response);
extern void dump_buffer(u64 addr, u64 len);
extern int update_verify_image(const char *part_name, const char *pfile);
#ifdef CONFIG_MTD_SPI_NAND
extern int spi_nand_protect_disable(void);
#endif

u32 fdl_checksum32(u32 chksum, u8 const *pdata, u32 len)
{
	u32 i;

	for (i = 0; i < len; i++) {
		chksum += (u8) (*(pdata + i));
	}

	return chksum;
}

static u32 calc_image_checkSum(u8 * buf, u32 len)
{
	u32 chkSum = 0;
	u32 aligned = (len & ~0x3);
	u32 remaining = (len & 0x3);

	for (int i = 0; i < aligned / sizeof(u32); i++) {
		chkSum += ((u32 *) buf)[i];
	}

	for (int i = 0; i < remaining; i++) {
		chkSum += *(buf + aligned + i);
	}

	return chkSum;
}

//#define DDR_PARAM_DEBUG
#ifdef DDR_PARAM_DEBUG
static void ddr_param_dump(u32 * buf, u32 size)
{
	int i;

	for (i = 0; i < size / 16; i++) {
		printf("%08llX ++ %04X: %08X %08X %08X %08X\n",
			(u64)buf, i * 16,
			*(buf + i * 4), *(buf + i * 4 + 1),
			*(buf + i * 4 + 2), *(buf + i * 4 + 3));
	}
}
#endif

int ddr_vref_param_save_rom(u64 iram_addr, u64 image_addr, u32 image_len)
{
	u8 * buf = NULL;
	struct spl_header *header = NULL;
	u32 header_size, data_size;
	u64 part_addr, part_size;

	header_size = sizeof(struct spl_header);
	data_size = sizeof(struct ddr_info);
	buf = malloc(header_size + data_size);
	if (!buf || (image_len > header_size + data_size)) {
		printf("ddr param buf = 0x%llX, total size = 0x%X, image_addr = 0x%llX, image_len=0x%X\n",
			(u64)buf, header_size + data_size, image_addr, image_len);
		return -1;
	}
#ifdef DDR_PARAM_DEBUG
	ddr_param_dump((u32 *)iram_addr, data_size);
#endif
	memcpy((void *)buf, (void *)image_addr, image_len);
	memcpy((void *)(buf + header_size), (void *)iram_addr, data_size);
	header = (struct spl_header *)buf;
	header->magic_data = 0x55543322;
	header->img_size = data_size;
	header->img_check_sum = calc_image_checkSum((u8 *)(buf+ header_size), data_size);
	header->check_sum = calc_image_checkSum((u8 *)&header->capability, sizeof(struct spl_header) - 8);
#ifdef DDR_PARAM_DEBUG
	ddr_param_dump((u32 *)buf, header_size + data_size);
#endif
	if (0 != common_get_part_info("ddrinit", &part_addr, &part_size)) {
		return -1;
	}

	return ((header_size + data_size) == common_raw_write("ddrinit", part_addr, header_size + data_size, (char *)buf)) ? 0 : -1;
}

#ifdef CONFIG_SUPPORT_EMMC_BOOT
int repatition_emmc_handle(fdl_partition_t * fdl_part_table, u16 part_count)
{
	/*
	 * add code distinguish emmc/spi nand/nor
	 * if emmc used, dispath to different function, refact later
	 */
	u16 i, j, temp;
	u32 part_table_len;
	ulong blksz;
	disk_partition_t *partitions = NULL;
	struct blk_desc *blk_dev_desc = NULL;
	u64 disk_part_start_lba = 0;
	u64 disk_part_size_lb, disk_part_gap_lb;
	int ret;

	part_table_len = part_count * sizeof(disk_partition_t);
	partitions = (disk_partition_t *) malloc(part_table_len);
	if (partitions == NULL) {
		pr_err("fdl_repartition_handle alloc disk_partition_t error\n");
		return -1;
	}
	memset(partitions, 0, part_table_len);

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		pr_err("get mmc dev fail\n");
		ret = -1;
		goto RET;
	}
	blksz = blk_dev_desc->blksz;

	for (i = 0; i < part_count; i++) {
		partitions[i].blksz = blksz;
		partitions[i].bootable = 0;
		for (j = 0; j < 31; j++) {
			temp = *(u16 *) (fdl_part_table[i].part_id + j);
			if (!temp)
				break;
			partitions[i].name[j] = (u8) temp;
		}

		if (fdl_part_table[i].sizeof_bytes != 0)
			disk_part_size_lb = PAD_COUNT(fdl_part_table[i].sizeof_bytes, blksz);
		else
			disk_part_size_lb = 0;

		if (fdl_part_table[i].gapof_bytes != 0)
			disk_part_gap_lb = PAD_COUNT(fdl_part_table[i].gapof_bytes, blksz);
		else
			disk_part_gap_lb = 0;

		/* if first partition we start from fixed 1MB of disk */
		if (0 == i)
			disk_part_gap_lb = SZ_1M / blksz;
		/* only the last partition can use the all rest blks */
		if (fdl_part_table[i].part_sizeof_unit == 0xffffffff) {
			if (i == (part_count - 1)) {
				disk_part_size_lb = 0;
			} else {
				pr_err("not the last partition cannot use all rest blks\n");
				ret = -1;
				goto RET;
			}
		}
		disk_part_start_lba += disk_part_gap_lb;
		partitions[i].start = disk_part_start_lba;
		partitions[i].size = disk_part_size_lb;
		disk_part_start_lba += disk_part_size_lb;
#if CONFIG_IS_ENABLED(PARTITION_UUIDS)
#ifdef CONFIG_RANDOM_UUID
		gen_rand_uuid_str(partitions[i].uuid, UUID_STR_FORMAT_STD);
#else
		pr_err("plese set uuid\n");
#endif
#endif
		/* not must need */
#if 0
		partitions[i].sys_ind =;
		partitions[i].type =;
#endif

	}

	ret = gpt_restore(blk_dev_desc, AXERA_DISK_GUID_STR, partitions, part_count);

RET:
	free(partitions);
	return ret;
}
#endif

#if defined (CONFIG_MTD_SPI_NAND) && defined (FDL_BAD_BLKS_SCAN)
static void spi_nand_scan_bad_blks(void)
{
	int ret = -1;
	u32 busnum = 0;
	loff_t off;
	struct udevice *dev = NULL;
	struct mtd_info *mtd = NULL;

	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		pr_err("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return;
	}

	mtd = dev_get_uclass_priv(dev);
	if (!mtd) {
		pr_err("\nno devices available\n");
		return;
	}

	printf("\nbad blocks:\n");
	for (off = 0; off < mtd->size; off += mtd->erasesize) {
		if (mtd_block_isbad(mtd, off))
			printf("%lld: 0x%llx\n", (unsigned long long)off / mtd->erasesize, (unsigned long long)off);
	}
}
#endif

int fdl_save_to_ddr(struct fdl_file_info *pfile)
{
	static long dtb_cur = DTB_IMAGE_ADDR;
	static long kernel_cur = KERNEL_IMAGE_ADDR;

	if (!strcmp(pfile->part_name, "dtb")) {
		//printf("save dtb\n");
		memcpy((void *)dtb_cur, (void *)pfile->start_addr, pfile->unsave_recv_len);
		dtb_cur += pfile->unsave_recv_len;
	} else if (!strcmp(pfile->part_name, "kernel")) {
		//printf("save kernel\n");
		memcpy((void *)kernel_cur, (void *)pfile->start_addr, pfile->unsave_recv_len);
		kernel_cur += pfile->unsave_recv_len;
	} else {
		printf("don't support saving %s\n", pfile->part_name);
	}
	return 0;
}

#ifdef CONFIG_SUPPORT_EMMC_BOOT
int fdl_save_to_emmc(struct fdl_file_info *pfile)
{
	lbaint_t start_lba, blkcnt_lb, base_lba;
	lbaint_t fdl_buf_written_lb;
	lbaint_t wr_blkcnt_lb;
	struct blk_desc *blk_dev_desc = NULL;
	disk_partition_t part_info;
	ulong blksz;
	u32 ret = 0;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		pr_err("get mmc dev fail\n");
		return -1;
	}
	blksz = blk_dev_desc->blksz;

	blkcnt_lb = PAD_COUNT(pfile->unsave_recv_len, blksz);
	if (pfile->recv_len > pfile->unsave_recv_len)
		fdl_buf_written_lb = PAD_COUNT(pfile->recv_len - pfile->unsave_recv_len, blksz);
	else
		fdl_buf_written_lb = 0;

	ret = get_part_info(blk_dev_desc, pfile->part_name, &part_info);
	if (ret == -1) {
		printf("%s: get %s partition info fail\n", __FUNCTION__, pfile->part_name);
	}

	if (pfile->recv_len > (u64) part_info.blksz * part_info.size) {
		printf
		    ("%s: part: %s: part_info.start = 0x%lx block count = 0x%lx, blksz: %lu\n",
		     __FUNCTION__, pfile->part_name, part_info.start, part_info.size, part_info.blksz);
		pr_err("received file size 0x%llx bytes bigger than partion size 0x%lx bytes\n", pfile->recv_len, part_info.blksz * part_info.size);
		return -1;
	}
	/*
	 * spl and eip bin have backup and store in boot_parttion1
	 * uboot store in boot_partition2
	 * others store in user_partition with gpt table
	 */

	if (!strcmp(pfile->part_name, "spl")) {
		ret = is_emmc_switch_boot_part1(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
		base_lba = part_info.start;
		start_lba = base_lba + fdl_buf_written_lb;

#if CONFIG_IS_ENABLED(BLK)
		wr_blkcnt_lb = blk_dwrite(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->start_addr);
#else
		wr_blkcnt_lb = blk_dev_desc->block_write(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->start_addr);
#endif
		if (wr_blkcnt_lb != blkcnt_lb) {
			pr_err("some error happend while write to disk\n");
			return -1;
		}
	} else if (!strcmp(pfile->part_name, "uboot")
			|| !strcmp(pfile->part_name, "uboot_b")
			|| !strcmp(pfile->part_name, "atf")
			|| !strcmp(pfile->part_name, "atf_b")
			|| !strcmp(pfile->part_name, "optee")
			|| !strcmp(pfile->part_name, "optee_b")
			|| !strcmp(pfile->part_name, "dtb")
			|| !strcmp(pfile->part_name, "dtb_b")
			|| !strcmp(pfile->part_name, "kernel")
			|| !strcmp(pfile->part_name, "kernel_b")) {
		ret = emmc_switch_to_uda_part(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
		base_lba = part_info.start;
		start_lba = base_lba + fdl_buf_written_lb;
#if CONFIG_IS_ENABLED(BLK)
		wr_blkcnt_lb = blk_dwrite(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->start_addr);
#else
		wr_blkcnt_lb = blk_dev_desc->block_write(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->start_addr);
#endif
		if (wr_blkcnt_lb != blkcnt_lb) {
			pr_err("some error happend while write to disk\n");
			return -1;
		}
	} else {
		ret = emmc_switch_to_uda_part(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
		base_lba = part_info.start;
		start_lba = base_lba + fdl_buf_written_lb;
#if CONFIG_IS_ENABLED(BLK)
		wr_blkcnt_lb = blk_dwrite(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->start_addr);
#else
		wr_blkcnt_lb = blk_dev_desc->block_write(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->start_addr);
#endif

		if (wr_blkcnt_lb != blkcnt_lb) {
			pr_err("some error happend while write to disk\n");
			return -1;
		}
	}
	is_emmc_switch_boot_part1(blk_dev_desc);
	return 0;
}
#endif

#ifdef CONFIG_MTD_SPI_NAND
int fdl_save_to_spinand(struct fdl_file_info *pfile)
{

	/* 1. mtd spi nand device init */
	u32 ret, dl_buf_off;
	u32 busnum = 0;
	u64 remaining;
	u64 off = 0;
	int idx;
	loff_t maxsize;
	size_t retlen;
	loff_t write_size;
	static int mtdpart_skip_blk_cnt;
	static loff_t size;

	struct udevice *dev = NULL;
	struct mtd_info *mtd = NULL;
	struct erase_info erase_op = { };

	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}

	mtd = dev_get_uclass_priv(dev);

	/* 2. first erase partition */
	if (strcmp(lastXferPart, pfile->part_name)) {
		erase_op.mtd = mtd;
		erase_op.len = mtd->erasesize;
		erase_op.scrub = 0;

		ret = mtd_arg_off(pfile->part_name, &idx, &partOff, &size, &maxsize, MTD_DEV_TYPE_NAND, mtd->size);
		if (ret)
			return ret;
		printf("%s: partiton %s off = 0x%llx size = 0x%llx\n", __func__, pfile->part_name, partOff, size);

		remaining = round_up(size, mtd->erasesize);
		erase_op.addr = partOff;

		while (remaining) {
			if (erase_op.addr + mtd->erasesize > partOff + size) {
				pr_err
				    ("%s: erase addr 0x%llX len 0x%X over %s part addr 0x%llX size 0x%llX\n",
				     __func__, erase_op.addr, mtd->erasesize, pfile->part_name, partOff, size);
				return -1;
			}
			//printf("Erase partition:%s",pfile->part_name);
			ret = mtd_erase(mtd, &erase_op);
			if (ret) {
				if (ret != -EIO) {
					printf("Failure while erasing at offset 0x%llx\n", erase_op.fail_addr);
					return -1;
				}
				pr_err("erase skip bad block @off = 0x%llX size = 0x%X\n", erase_op.addr, mtd->erasesize);
			}
			remaining -= mtd->erasesize;
			erase_op.addr += mtd->erasesize;
		}
		printf("partiton %s erased @off = 0x%llx size = 0x%llx\n", pfile->part_name, partOff, size);
	}

	/* 3. Loop over to do the actual read/write */
	if (strcmp(lastXferPart, pfile->part_name)) {
		xferPartCnt = 0;
		mtdpart_skip_blk_cnt = 0;
		printf("partiton %s write start, mtdpart_skip_blk_cnt=%d\n", pfile->part_name, mtdpart_skip_blk_cnt);
	}
#if 0
	off = partOff + xferPartCnt * dl_buf_size;	/* FDL_BUF_LEN = 2M */
	xferPartCnt++;

	mtd_write(mtd, off, pfile->unsave_recv_len, &retlen, (u_char *) pfile->start_addr);
	if (retlen != pfile->unsave_recv_len) {
		printf("write partition fail!.\n");
		return -1;
	}
	printf("partiton %s write @off = 0x%llx size = 0x%x\n", pfile->part_name, off, pfile->unsave_recv_len);
#else
	off = partOff + xferPartCnt * dl_buf_size + mtdpart_skip_blk_cnt * mtd->erasesize;	/* FDL_BUF_LEN = 2M */
	if (off % mtd->erasesize) {
		pr_err("nand addr 0x%llX is not block size 0x%X aligned!\n", off, mtd->erasesize);
		return -1;
	}
	xferPartCnt++;

	remaining = pfile->unsave_recv_len;
	dl_buf_off = 0;
	printf("buf: unsave_recv_len=0x%llX, maxsize=0x%lX\n", pfile->unsave_recv_len, dl_buf_size);
	while (remaining) {
		if (mtd_block_isbad(mtd, off)) {
			mtdpart_skip_blk_cnt++;
			printf("nand addr 0x%llX, blk %lld is bad, mtdpart_skip_blk_cnt=%d\n", off, off / mtd->erasesize, mtdpart_skip_blk_cnt);
			off += mtd->erasesize;
			continue;
		}

		write_size = (remaining > mtd->erasesize) ? mtd->erasesize : remaining;
		if (off + write_size > partOff + size) {
			pr_err
			    ("%s: write addr 0x%llX len 0x%llX over %s part addr 0x%llX size 0x%llX\n",
			     __func__, off, write_size, pfile->part_name, partOff, size);
			return -1;
		}
		if (dl_buf_off + write_size > pfile->unsave_recv_len) {
			pr_err
			    ("%s: dl_buf_off 0x%X len 0x%llX over unsave_recv_len 0x%llX\n",
			     __func__, dl_buf_off, write_size, pfile->unsave_recv_len);
			return -1;
		}

		mtd_write(mtd, off, write_size, &retlen, (u_char *) (pfile->start_addr + dl_buf_off));
		if (retlen != write_size) {
			pr_err("write partition fail!.\n");
			return -1;
		}
		printf("partiton %s wrote 0x%lX bytes, dl_buf_off 0x%X ==> nand addr 0x%llX\n", pfile->part_name, retlen, dl_buf_off, off);
		remaining -= write_size;
		dl_buf_off += write_size;
		off += write_size;
	}
#endif

	strcpy(lastXferPart, pfile->part_name);
	return 0;

}
#endif

#ifdef CONFIG_SPI_FLASH
int fdl_save_to_spinor(struct fdl_file_info *pfile)
{
	/* 1. mtd spi nand device init */
	u32 ret;
	u32 busnum = 0;
	u64 remaining, off = 0;
	int idx;
	loff_t size, maxsize;
	size_t retlen;
	struct udevice *dev;
	struct mtd_info *mtd;
	struct erase_info erase_op = { };
	struct spi_flash *flash;

	ret = uclass_get_device(UCLASS_SPI_FLASH, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}
	mtd = get_mtd_device_nm("nor0");
	flash = dev_get_uclass_priv(dev);

	/* 2. first erase partition */
	if (strcmp(lastXferPart, pfile->part_name)) {
		erase_op.mtd = mtd;
		erase_op.len = mtd->erasesize;
		erase_op.scrub = 0;

		ret = mtd_arg_off(pfile->part_name, &idx, &partOff, &size, &maxsize, MTD_DEV_TYPE_NOR, flash->size);
		if (ret)
			return ret;

		remaining = round_up(size, mtd->erasesize);
		erase_op.addr = partOff;

		while (remaining) {
			ret = mtd_erase(mtd, &erase_op);
			if (ret) {
				if (ret != -EIO) {
					printf("Failure while erasing at offset 0x%llx\n", erase_op.fail_addr);
					return -1;
				}
			} else {
				remaining -= mtd->erasesize;
			}
			erase_op.addr += mtd->erasesize;
		}
		printf("partiton %s erased @off = 0x%llx size = 0x%llx\n", pfile->part_name, partOff, size);
	}

	/* 3. Loop over to do the actual read/write */
	if (strcmp(lastXferPart, pfile->part_name))
		xferPartCnt = 0;

	off = partOff + xferPartCnt * dl_buf_size;
	xferPartCnt++;

	mtd_write(mtd, off, pfile->unsave_recv_len, &retlen, (u_char *) pfile->start_addr);
	if (retlen != pfile->unsave_recv_len) {
		printf("write partition fail!.\n");
		return -1;
	}
	printf("partiton %s write @off = 0x%llx size = 0x%llx\n", pfile->part_name, off, pfile->unsave_recv_len);

	strcpy(lastXferPart, pfile->part_name);
	return 0;
}

#ifdef SPI_DUAL_CS
int fdl_save_to_spinor1(struct fdl_file_info *pfile)
{
	/* 1. mtd spi nand device init */
	u32 ret;
	u64 off = 0;
	char cmd[256];
	int part_index = get_sf1_part_info(pfile->part_name);
	loff_t flash_addr = g_sf1_part_info[part_index].part_offset;
	loff_t size = g_sf1_part_info[part_index].part_size;

	/* 2. first erase partition */
	if (strcmp(lastXferPart, pfile->part_name)) {
		sprintf(cmd, "sf erase 0x%llX 0x%llX", flash_addr, size);
		printf("%s\n", cmd);
		ret = run_command(cmd, 0);
		if (ret) {
			printf("Failure while erasing at offset = 0x%llx size = 0x%llX\n", flash_addr, size);
			return -1;
		}
		printf("partiton %s erased @off = 0x%llx size = 0x%llx\n", pfile->part_name, flash_addr, size);
	}

	/* 3. Loop over to do the actual read/write */
	if (strcmp(lastXferPart, pfile->part_name))
		xferPartCnt = 0;

	off = flash_addr + xferPartCnt * dl_buf_size;
	xferPartCnt++;

	sprintf(cmd, "sf write 0x%lX 0x%llX 0x%llX", pfile->start_addr, off, pfile->unsave_recv_len);
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret) {
		printf("write partition fail!.\n");
		return -1;
	}
	printf("partiton %s write @off = 0x%llx size = 0x%llx\n", pfile->part_name, off, pfile->unsave_recv_len);

	strcpy(lastXferPart, pfile->part_name);
	return 0;
}
#endif
#endif

int fdl_save_to_storage(struct fdl_file_info *pfile)
{
	int ret = -1;

	ret = update_verify_image(pfile->part_name, pfile->start_addr);
	if (ret != 0) {
		frame_send_respone(FDL_RESP_SECURE_SIGNATURE_ERR);
		return -1;
	}

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
#ifdef CONFIG_SUPPORT_EMMC_BOOT
		ret = fdl_save_to_emmc(pfile);
#endif
		break;
	case STORAGE_TYPE_NAND:
#ifdef CONFIG_MTD_SPI_NAND
		ret = fdl_save_to_spinand(pfile);
#endif
		break;
	case STORAGE_TYPE_NOR:
#ifdef CONFIG_SPI_FLASH
#ifdef SPI_DUAL_CS
	if (get_sf1_part_info(pfile->part_name) < 0)
#endif
		ret = fdl_save_to_spinor(pfile);
#ifdef SPI_DUAL_CS
	else
		ret = fdl_save_to_spinor1(pfile);
#endif
#endif
		break;
	case STORAGE_TYPE_UNKNOWN:
		ret = fdl_save_to_ddr(pfile);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

int fdl_repartition_handle(fdl_partition_t * fdl_part_table, u16 part_count)
{
	int ret = -1;

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
#ifdef CONFIG_SUPPORT_EMMC_BOOT
		ret = repatition_emmc_handle(fdl_part_table, part_count);
#endif
		break;
	case STORAGE_TYPE_NAND:
		//ret = repartion_nand_handle(fdl_part_table, part_count);
		ret = 0;	//temp
		break;
	case STORAGE_TYPE_NOR:
		//ret = repartion_nor_handle(fdl_part_table, part_count);
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

#define MAX_PATTERN_NUM           (6)
#if defined (CONFIG_AXERA_AX620E)
#define SPI_BASE_ADDR             (0x1A00000)
#else
@error
#endif
#define SPI_SSIENR_ADDR           (SPI_BASE_ADDR + 0x8)
#define SPI_BAURD_ADDR            (SPI_BASE_ADDR + 0x14)
#define SPI_RX_SAMPLE_DELAY_ADDR  (SPI_BASE_ADDR + 0xf0)
#ifdef SPI_RX_SAMPLE_DLY_SCAN
#define SPI_RX_SAMPLE_SCAN_FIRST_WRITE
static int spi_pattern_data_set(u32 * pbuf, u32 pattern_size, u32 pattern_val)
{
	int i;

	if (NULL == pbuf) {
		pr_err("%s: data buf is null\n", __func__);
		return -1;
	}
	if (pattern_size % 4) {
		pr_err("%s: pattern_size 0x%x not 4 bytes aligned\n", __func__, pattern_size);
		return -1;
	}

	for (i = 0; i < pattern_size / 4; i++) {
		pbuf[i] = pattern_val;
	}
	return 0;
}

static int spi_pattern_data_check(u32 * pbuf, u32 pattern_size, u32 pattern_val)
{
	int i;

	if (NULL == pbuf) {
		pr_err("%s: data buf is null\n", __func__);
		return -1;
	}
	if (pattern_size % 4) {
		pr_err("%s: pattern_size 0x%x not 4 bytes aligned\n", __func__, pattern_size);
		return -1;
	}

	for (i = 0; i < pattern_size / 4; i++) {
		if (pattern_val != pbuf[i]) {
			pr_err("addr 0x%lx, 0x%x != 0x%x\n", (unsigned long)&pbuf[i], pbuf[i], pattern_val);
			return -1;
		}
	}
	//printf("pattern 0x%x pass\n", pattern_val);
	return 0;
}

#ifdef CONFIG_SPI_FLASH
static u32 spinor_rx_sample_delay_scan(int baurd)
{
	int ret = -1;
	u32 i, delay;
	u32 busnum = 0;
	size_t retlen;
	u32 spi_pattern[MAX_PATTERN_NUM] = { 0, 0xffffffff, 0x5a5a5a5a, 0x0f0f0f0f, 0x12345678, 0x98765432 };
	u_char *pbuf = NULL;
#ifdef SPI_RX_SAMPLE_SCAN_FIRST_WRITE
	struct erase_info erase_op;
#endif
	struct udevice *dev = NULL;
	struct mtd_info *mtd = NULL;
	struct spi_nor *nor;

	printf("======== %s start ===========\n", __func__);
#if defined (CONFIG_AXERA_AX620E)
	printf("0x%x=0x%x\n", CPU_SYS_GLB_CLK_MUX0, *((u32 *)CPU_SYS_GLB_CLK_MUX0));
#endif
	ret = uclass_get_device(UCLASS_SPI_FLASH, busnum, &dev);
	if (ret) {
		pr_err("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return 0;
	}

	mtd = dev_get_uclass_priv(dev);
	if (!mtd) {
		pr_err("no devices available\n");
		return 0;
	}
	nor = mtd->priv;

	if (MAX_PATTERN_NUM * nor->page_size > mtd->erasesize) {
		pr_err("scan size over nand block0\n");
		return 0;
	}

	pbuf = memalign(0x100000, MAX_PATTERN_NUM * nor->page_size);
	if (NULL == pbuf) {
		pr_err("no mem space\n");
		return 0;
	}
	printf("malloc 0x%x bytes at addr 0x%lx for scan\n", MAX_PATTERN_NUM * nor->page_size, (unsigned long)pbuf);
	for (i = 0; i < MAX_PATTERN_NUM; i++) {
		if (spi_pattern_data_set((u32 *) (pbuf + i * nor->page_size), nor->page_size, spi_pattern[i])) {
			pr_err("pattern set fail\n");
			return 0;
		}
	}
#ifdef SPI_RX_SAMPLE_SCAN_FIRST_WRITE
	erase_op.mtd = mtd;
	erase_op.len = mtd->erasesize;
	erase_op.scrub = 0;
	erase_op.addr = 0;
	ret = mtd_erase(mtd, &erase_op);
	if (ret) {
		if (ret != -EIO) {
			pr_err("Failure while erasing at offset 0x%llx\n", erase_op.fail_addr);
			return 0;
		}
		pr_err("erase @off = 0x%llX size = 0x%X return %d\n", erase_op.addr, mtd->erasesize, -EIO);
	}

	mtd_write(mtd, 0, MAX_PATTERN_NUM * nor->page_size, &retlen, pbuf);
	if (retlen != MAX_PATTERN_NUM * nor->page_size) {
		pr_err("write partition fail!.\n");
		return 0;
	}
#endif
	// pose edge rx sample delay scan
	for (delay = 0; delay < 2 * baurd; delay++) {
		*((u32 *) SPI_SSIENR_ADDR) = 0;
		udelay(100);
		*((u32 *) SPI_RX_SAMPLE_DELAY_ADDR) = delay;
		*((u32 *) SPI_SSIENR_ADDR) = 1;
		udelay(100);
		printf("0x%x=0x%x, 0x%x=0x%x, 0x%x=0x%x:\n",
		       SPI_SSIENR_ADDR, *((u32 *) SPI_SSIENR_ADDR),
		       SPI_BAURD_ADDR, *((u32 *) SPI_BAURD_ADDR), SPI_RX_SAMPLE_DELAY_ADDR, *((u32 *) SPI_RX_SAMPLE_DELAY_ADDR));
		memset((void *)pbuf, 0xee, MAX_PATTERN_NUM * nor->page_size);
		mtd_read(mtd, 0, MAX_PATTERN_NUM * nor->page_size, &retlen, pbuf);
		if (retlen != MAX_PATTERN_NUM * nor->page_size) {
			pr_err("read fail!\n");
			return 0;
		}
		for (i = 0; i < MAX_PATTERN_NUM; i++) {
			if (spi_pattern_data_check((u32 *) (pbuf + i * nor->page_size), nor->page_size, spi_pattern[i])) {
				pr_err("pattern check fail\n");
				break;
			}
		}
		if (MAX_PATTERN_NUM == i)
			printf("pos edge: delay=%d pass\n", delay);
	}
	// neg edge rx sample delay scan
	for (delay = 0; delay < 2 * baurd; delay++) {
		*((u32 *) SPI_SSIENR_ADDR) = 0;
		udelay(100);
		*((u32 *) SPI_RX_SAMPLE_DELAY_ADDR) = 0x10000 | delay;
		*((u32 *) SPI_SSIENR_ADDR) = 1;
		udelay(100);
		printf("0x%x=0x%x, 0x%x=0x%x, 0x%x=0x%x:\n",
		       SPI_SSIENR_ADDR, *((u32 *) SPI_SSIENR_ADDR),
		       SPI_BAURD_ADDR, *((u32 *) SPI_BAURD_ADDR), SPI_RX_SAMPLE_DELAY_ADDR, *((u32 *) SPI_RX_SAMPLE_DELAY_ADDR));
		memset((void *)pbuf, 0xee, MAX_PATTERN_NUM * nor->page_size);
		mtd_read(mtd, 0, MAX_PATTERN_NUM * nor->page_size, &retlen, pbuf);
		if (retlen != MAX_PATTERN_NUM * nor->page_size) {
			pr_err("read fail!\n");
			return 0;
		}
		for (i = 0; i < MAX_PATTERN_NUM; i++) {
			if (spi_pattern_data_check((u32 *) (pbuf + i * nor->page_size), nor->page_size, spi_pattern[i])) {
				pr_err("pattern check fail\n");
				break;
			}
		}
		if (MAX_PATTERN_NUM == i)
			printf("neg edge: delay=%d pass\n", delay);
	}

	free(pbuf);
	pbuf = NULL;
	printf("======== %s complete ========\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_MTD_SPI_NAND
static u32 spinand_rx_sample_delay_scan(int baurd)
{
	int ret = -1;
	u32 i, delay;
	u32 busnum = 0;
	size_t retlen;
	u32 spi_pattern[MAX_PATTERN_NUM] = { 0, 0xffffffff, 0x5a5a5a5a, 0x0f0f0f0f, 0x12345678, 0x98765432 };
	u_char *pbuf = NULL;
#ifdef SPI_RX_SAMPLE_SCAN_FIRST_WRITE
	struct erase_info erase_op;
#endif
	struct udevice *dev = NULL;
	struct mtd_info *mtd = NULL;

	printf("======== %s start ===========\n", __func__);
#if defined (CONFIG_AXERA_AX620E)
	printf("0x%x=0x%x\n", CPU_SYS_GLB_CLK_MUX0, *((u32 *)CPU_SYS_GLB_CLK_MUX0));
#endif
	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		pr_err("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return 0;
	}

	mtd = dev_get_uclass_priv(dev);
	if (!mtd) {
		pr_err("no devices available\n");
		return 0;
	}

	if (MAX_PATTERN_NUM * mtd->writesize > mtd->erasesize) {
		pr_err("scan size over nand block0\n");
		return 0;
	}

	pbuf = memalign(0x100000, MAX_PATTERN_NUM * mtd->writesize);
	if (NULL == pbuf) {
		pr_err("no mem space\n");
		return 0;
	}
	printf("malloc 0x%x bytes at addr 0x%lx for scan\n", MAX_PATTERN_NUM * mtd->writesize, (unsigned long)pbuf);
	for (i = 0; i < MAX_PATTERN_NUM; i++) {
		if (spi_pattern_data_set((u32 *) (pbuf + i * mtd->writesize), mtd->writesize, spi_pattern[i])) {
			pr_err("pattern set fail\n");
			return 0;
		}
	}
#ifdef SPI_RX_SAMPLE_SCAN_FIRST_WRITE
	erase_op.mtd = mtd;
	erase_op.len = mtd->erasesize;
	erase_op.scrub = 0;
	erase_op.addr = 0;
	ret = mtd_erase(mtd, &erase_op);
	if (ret) {
		if (ret != -EIO) {
			pr_err("Failure while erasing at offset 0x%llx\n", erase_op.fail_addr);
			return 0;
		}
		pr_err("erase @off = 0x%llX size = 0x%X return %d\n", erase_op.addr, mtd->erasesize, -EIO);
	}

	mtd_write(mtd, 0, MAX_PATTERN_NUM * mtd->writesize, &retlen, pbuf);
	if (retlen != MAX_PATTERN_NUM * mtd->writesize) {
		pr_err("write partition fail!.\n");
		return 0;
	}
#endif
	// pose edge rx sample delay scan
	for (delay = 0; delay < 2 * baurd; delay++) {
		*((u32 *) SPI_SSIENR_ADDR) = 0;
		udelay(100);
		*((u32 *) SPI_RX_SAMPLE_DELAY_ADDR) = delay;
		*((u32 *) SPI_SSIENR_ADDR) = 1;
		udelay(100);
		printf("0x%x=0x%x, 0x%x=0x%x, 0x%x=0x%x:\n",
		       SPI_SSIENR_ADDR, *((u32 *) SPI_SSIENR_ADDR),
		       SPI_BAURD_ADDR, *((u32 *) SPI_BAURD_ADDR), SPI_RX_SAMPLE_DELAY_ADDR, *((u32 *) SPI_RX_SAMPLE_DELAY_ADDR));
		memset((void *)pbuf, 0xee, MAX_PATTERN_NUM * mtd->writesize);
		mtd_read(mtd, 0, MAX_PATTERN_NUM * mtd->writesize, &retlen, pbuf);
		if (retlen != MAX_PATTERN_NUM * mtd->writesize) {
			pr_err("read fail!\n");
			return 0;
		}
		for (i = 0; i < MAX_PATTERN_NUM; i++) {
			if (spi_pattern_data_check((u32 *) (pbuf + i * mtd->writesize), mtd->writesize, spi_pattern[i])) {
				pr_err("pattern check fail\n");
				break;
			}
		}
		if (MAX_PATTERN_NUM == i)
			printf("pos edge: delay=%d pass\n", delay);
	}
	// neg edge rx sample delay scan
	for (delay = 0; delay < 2 * baurd; delay++) {
		*((u32 *) SPI_SSIENR_ADDR) = 0;
		udelay(100);
		*((u32 *) SPI_RX_SAMPLE_DELAY_ADDR) = 0x10000 | delay;
		*((u32 *) SPI_SSIENR_ADDR) = 1;
		udelay(100);
		printf("0x%x=0x%x, 0x%x=0x%x, 0x%x=0x%x:\n",
		       SPI_SSIENR_ADDR, *((u32 *) SPI_SSIENR_ADDR),
		       SPI_BAURD_ADDR, *((u32 *) SPI_BAURD_ADDR), SPI_RX_SAMPLE_DELAY_ADDR, *((u32 *) SPI_RX_SAMPLE_DELAY_ADDR));
		memset((void *)pbuf, 0xee, MAX_PATTERN_NUM * mtd->writesize);
		mtd_read(mtd, 0, MAX_PATTERN_NUM * mtd->writesize, &retlen, pbuf);
		if (retlen != MAX_PATTERN_NUM * mtd->writesize) {
			pr_err("read fail!\n");
			return 0;
		}
		for (i = 0; i < MAX_PATTERN_NUM; i++) {
			if (spi_pattern_data_check((u32 *) (pbuf + i * mtd->writesize), mtd->writesize, spi_pattern[i])) {
				pr_err("pattern check fail\n");
				break;
			}
		}
		if (MAX_PATTERN_NUM == i)
			printf("neg edge: delay=%d pass\n", delay);
	}

	free(pbuf);
	pbuf = NULL;
	printf("======== %s complete ========\n", __func__);
	return 0;
}
#endif

static void spi_rx_sample_delay_scan(int baurd)
{
	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_NAND:
#ifdef CONFIG_MTD_SPI_NAND
		spinand_rx_sample_delay_scan(baurd);
#endif
		break;
	case STORAGE_TYPE_NOR:
#ifdef CONFIG_SPI_FLASH
		spinor_rx_sample_delay_scan(baurd);
#endif
		break;
	default:
		printf("%s: storage_sel %d error\n", __func__, boot_info_data.storage_sel);
		break;
	}
}
#endif

#define CMD_START_DATALEN_WITH_CHKSUM     0x5C
#define CMD_START_DATALEN_NO_CHKSUM       0x58
#define CMD_START_IMG_SIZE_OFF            72
#define CMD_START_IMG_CHKSUM_OFF          88
int cmd_start_transfer(fdl_frame_t * pframe, void *arg)
{
	int i;
	u64 temp;
	u16 ch16;
	struct fdl_file_info *pfile = (struct fdl_file_info *)arg;

	dl_buf_addr = FDL_BUF_ADDR;
	dl_buf_size = FDL_BUF_LEN;

	if (dl_buf_size == 0) {
		//error("fdl buf len is 0 bytes, cannot receive data\n");
		return -1;
	}
	memset((void *)pfile, 0, sizeof(struct fdl_file_info));

	/*
	 *                           cmd_start_trnafer format
	 *      | header | data_len | cmd | data area | checksum |
	 *      | 4B | 4B | 4B | par_id 72B + size 8B + rsv 8 + chksum 4B(op) | 2bytes |
	 */
	if (pframe->data_len == CMD_START_DATALEN_WITH_CHKSUM) {
		pfile->chksum_flag = 1;
		pfile->target_chksum = *(u32 *) (pframe->data + CMD_START_IMG_CHKSUM_OFF);
	} else if (pframe->data_len == CMD_START_DATALEN_NO_CHKSUM) {
		pfile->chksum_flag = 0;
		pfile->target_chksum = 0;
	} else {
		//error("cmd_start_tranfer format error\n");
		frame_send_respone(FDL_RESP_INVLID_CMD);
		return -1;
	}

	pfile->start_addr = dl_buf_addr;
	pfile->curr_addr = dl_buf_addr;

	for (i = 0; i < 31; i++) {
		ch16 = *(u16 *) (pframe->data + 2 * i);
		if (!ch16)
			break;
		pfile->part_name[i] = (char)ch16;
	}

	temp = *(u32 *) (pframe->data + CMD_START_IMG_SIZE_OFF + 4);
	temp = temp << 32;
	pfile->target_len = *(u32 *) (pframe->data + CMD_START_IMG_SIZE_OFF) | temp;	//combined to 64 bit data

	pfile->recv_len = 0;
	pfile->unsave_recv_len = 0;
	pfile->recv_chksum = 0;
	pfile->full_img_size = pfile->target_len;	// full pkg img size
	u64 part_addr, part_size = 0;
	if (0 == common_get_part_info(pfile->part_name, &part_addr, &part_size)) {
		if (pfile->full_img_size <= part_size) {
			frame_send_respone(FDL_RESP_ACK);
		} else {
			printf
			    ("part name: %s, full_img_size: 0x%llx bigger than part size: 0x%llx, update failed!\n",
			     pfile->part_name, pfile->full_img_size, part_size);
			frame_send_respone(FDL_RESP_SIZE_ERR);
			return -1;
		}
	} else {
		frame_send_respone(FDL_RESP_ACK);
	}
	return 0;
}

#define DEBUG_SPARSE_DL_ERASE_PART
int cmd_transfer_data(fdl_frame_t * pframe, void *arg)
{
	ulong free_buf_len;
	int ret, retlen;
	u32 checksum_en = 0;
	u32 chksum_flag = 0;
	u32 checksum = 0;
	struct fdl_file_info *pfile = (struct fdl_file_info *)arg;
#ifdef DEBUG_SPARSE_DL_ERASE_PART
	ulong sparse_buf_point, sparse_buf_last;
	u64 part_addr, part_size;
#endif

	pfile->target_len = *(u32 *) (pframe->data);	//1 pkg img size
	/*
	 * if the FDL buf size is too small
	 */
	if (pfile->target_len > dl_buf_size) {
		pr_err("cmd_transfer error, frame size is big than buf size\n");
		return -1;
	}

	if (pfile->recv_len + pfile->target_len > pfile->full_img_size) {
		pr_err("cmd_transfer error, recive data size mismatch\n");
		frame_send_respone(FDL_RESP_SIZE_ERR);
		return -1;
	}
	/* fisrt pkg
	 * 0x5C6D8E9F (pframe->data_len n) (pframe->cmd_index 0x00000002) (pframe->data size(4) + chksum_en(4) + chksum(4))     (checksum XXXX)
	 */
	checksum_en = *(u32 *) (pframe->data + 4);
	if (checksum_en == 1) {
		checksum = *(u32 *) (pframe->data + 8);	//1 pkg img chksum
		chksum_flag = 1;
	} else if (checksum_en == 0) {
		checksum = 0;
		chksum_flag = 0;
	}
	/* two resp pkg
	 * Header(4)    Payload Data Len(4)     CMD(4)  Payload Data(0) Check Sum(2)
	 */
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	/* three resp pkg
	 * IMAGE RAW DATA
	 */
	retlen = g_CurrChannel->read(g_CurrChannel, (u8 *) pframe->data, pfile->target_len);	//read data to pframe->data (max 64K)
	if (retlen != pfile->target_len) {
		printf("read retlen error, retlen = %d,pfile->target_len = %lld\n", retlen, pfile->target_len);
		return -1;
	}
	/* Verify a package of img */
	if (chksum_flag && checksum != calc_image_checkSum((u8 *) pframe->data, pfile->target_len)) {
		ret = frame_send_respone(FDL_RESP_VERIFY_CHEKSUM_ERROR);
		if (ret)
			return ret;
	}

	if ((0 == pfile->recv_len) && is_sparse_image((void *)pframe->data)) {
		dl_buf_addr = SPARSE_IMAGE_BUF_ADDR;
		dl_buf_size = SPARSE_IMAGE_BUF_LEN;
		pfile->start_addr = dl_buf_addr;
		pfile->curr_addr = dl_buf_addr;
		sparse_download_enable = 1;
		if (sparse_info_init(&sparse, pfile->part_name)) {
			frame_send_respone(FDL_RESP_DEST_ERR);
			return -1;
		}
		printf("%s: part %s is sparse format, buf 0x%lX, size 0x%lX\n", __FUNCTION__, pfile->part_name, dl_buf_addr, dl_buf_size);
#ifdef DEBUG_SPARSE_DL_ERASE_PART
		if (!common_get_part_info(pfile->part_name, &part_addr, &part_size)) {
			printf("%s: erase part %s, addr 0x%llX, size 0x%llX\n", __FUNCTION__, pfile->part_name, part_addr, part_size);
			common_raw_erase(pfile->part_name, part_addr, part_size);
		}
#endif
	}
	if (pfile->unsave_recv_len + pfile->target_len > dl_buf_size) {
		/*
		 * the buf is 4KB size bundary
		 * fill some frame data to make buf full and then save to storage
		 */
		free_buf_len = dl_buf_size - pfile->unsave_recv_len;
		memcpy((void *)pfile->curr_addr, (void *)pframe->data, free_buf_len);
		pfile->unsave_recv_len = dl_buf_size;
		pfile->recv_len += free_buf_len;

		if (sparse_download_enable) {
			ret = write_sparse_img(&sparse, pfile->part_name, (void *)pfile->start_addr, &sparse_buf_point);
			if (ret || (sparse_buf_point < dl_buf_addr)
			    || (sparse_buf_point > (dl_buf_addr + dl_buf_size))) {
				printf("cmd_transfer_data write sparse image fail, ret %d, end point addr 0x%lX\n", ret, sparse_buf_point);
				frame_send_respone(FDL_RESP_DEST_ERR);
				return -2;
			}
			sparse_buf_last = dl_buf_addr + dl_buf_size - sparse_buf_point;
			if ((sparse_buf_last + pfile->target_len - free_buf_len) > dl_buf_size) {
				printf("%s: write sparse error, chunk size over 0x%llX\n", __FUNCTION__, (dl_buf_size - pfile->target_len));
				frame_send_respone(FDL_RESP_SIZE_ERR);
				return -2;
			}
			if (sparse_buf_last) {
				printf
				    ("%s: move last buffer 0x%lX, size 0x%lX to 0x%lX\n",
				     __FUNCTION__, sparse_buf_point, sparse_buf_last, dl_buf_addr);
				memcpy((void *)dl_buf_addr, (void *)(ulong) sparse_buf_point, (ulong) sparse_buf_last);
			}
			/* reset the curr_addr unsave_recv_len */
			pfile->curr_addr = dl_buf_addr + sparse_buf_last;
			pfile->unsave_recv_len = sparse_buf_last;
			memcpy((void *)pfile->curr_addr, (void *)(pframe->data + free_buf_len), (pfile->target_len - free_buf_len));
			pfile->unsave_recv_len += pfile->target_len - free_buf_len;
			pfile->curr_addr += pfile->target_len - free_buf_len;
			pfile->recv_len += pfile->target_len - free_buf_len;
		} else {
			ret = fdl_save_to_storage(pfile);

			if (ret) {
				pr_err("cmd_transfer_data save data to storage fail\n");
				frame_send_respone(FDL_RESP_DEST_ERR);
				return -2;
			}
			/* reset the curr_addr unsave_recv_len */
			pfile->curr_addr = dl_buf_addr;
			memcpy((void *)pfile->curr_addr, (void *)(pframe->data + free_buf_len), (pfile->target_len - free_buf_len));
			pfile->unsave_recv_len = pfile->target_len - free_buf_len;
			pfile->curr_addr += pfile->target_len - free_buf_len;
			pfile->recv_len += pfile->target_len - free_buf_len;
		}

	} else {
		memcpy((void *)pfile->curr_addr, (void *)pframe->data, pfile->target_len);
		pfile->curr_addr += pfile->target_len;
		pfile->unsave_recv_len += pfile->target_len;
		pfile->recv_len += pfile->target_len;
	}

	if (pfile->chksum_flag)
		pfile->recv_chksum = fdl_checksum32(pfile->recv_chksum, pframe->data, pfile->target_len);

	/* four resp pkg
	 * Header(4)    Payload Data Len(4)     CMD(4)  Payload Data(0) Check Sum(2)
	 */
	ret = frame_send_respone(FDL_RESP_ACK);
	if (ret)
		return ret;

	return 0;
}

int cmd_transfer_end(fdl_frame_t * pframe, void *arg)
{
	int ret;
	//u32 img_chksum;
	struct fdl_file_info *pfile = (struct fdl_file_info *)arg;
	if (pfile->chksum_flag && (pfile->recv_chksum != pfile->target_chksum)) {
		//error("transfer_end img checksum error\n");
		frame_send_respone(FDL_RESP_VERIFY_CHEKSUM_ERROR);
		return -1;
	}

	if (pfile->recv_len != pfile->full_img_size) {
		//error("transfer_end img size mismatch error\n");
		frame_send_respone(FDL_RESP_SIZE_ERR);
		return -1;
	}

	printf("download image ok\n");

	if (pfile->unsave_recv_len) {
		if (sparse_download_enable) {
			ret = write_sparse_img(&sparse, pfile->part_name, (void *)pfile->start_addr, NULL);
			if (ret) {
				printf("transfer_end write sparse image fail\n");
				frame_send_respone(FDL_RESP_DEST_ERR);
				return -2;
			}
			sparse_download_enable = 0;
		} else {
			if (!strcmp(pfile->part_name, "ddrinit")) {
				ret = ddr_vref_param_save_rom(DDR_INFO_ADDR, dl_buf_addr, pfile->unsave_recv_len);
			}
			else
				ret = fdl_save_to_storage(pfile);
			if (ret) {
				//error("transfer_end save data to storage fail\n");
				frame_send_respone(FDL_RESP_DEST_ERR);
				return -2;
			}
		}
	}

	frame_send_respone(FDL_RESP_ACK);
	return 0;
}

#define REPARTITION_PART_ENTRY_LEN      88
#define REPARTITION_PART_SIZE_OFF       72
#define REPARTITION_PART_GAP_OFF        80
#define FDL_PART_HEADER_LEN             8

extern struct boot_mode_info boot_info_data;
int cmd_repartition(fdl_frame_t * pframe, void *arg)
{
	u16 i, j;
	u32 part_table_len;
	u64 temp;
	int len = 0, ret = 0;
	fdl_partition_header_t fdl_part_header;
	fdl_partition_t *fdl_part_table;
	char *mmc_parts = NULL;
	u64 capacity_user;
	u64 use_part_size = 0;
#ifdef CONFIG_SUPPORT_EMMC_BOOT
	struct mmc_uclass_priv *upriv;
	struct blk_desc *blk_dev_desc = NULL;
	struct mmc *mmc;
	(void)mmc;
	(void)upriv;
#endif
#if !defined(CONFIG_SUPPORT_EMMC_BOOT) && (defined (CONFIG_SPI_FLASH) || defined (CONFIG_MTD_SPI_NAND))
	u32 busnum = 0;
	struct udevice *dev = NULL;
#endif
#if !defined(CONFIG_MTD_SPI_NAND) && defined(CONFIG_SPI_FLASH)
	struct spi_flash *flash;
#endif
#if !defined(CONFIG_SUPPORT_EMMC_BOOT) && defined(CONFIG_MTD_SPI_NAND)
	struct mtd_info *mtd = NULL;
#endif
	void *fdl_part_cur;
	char buf[512] = { 0 };
	char partition_name[CMD_PART_ID_SIZE] = { 0 };
#ifdef SPI_DUAL_CS
	int len_cs1 = 0;
	int part_index_cs1 = 0;
	u64 use_part_size_cs1 = 0;
	int last_part_cs0 = 0;
	int flash_cs = 0;
#endif

	memcpy((void *)&fdl_part_header, (void *)pframe->data, FDL_PART_HEADER_LEN);

#ifdef CONFIG_SUPPORT_EMMC_BOOT
	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		printf("get mmc dev fail\n");
		return -1;
	}
	upriv = blk_dev_desc->bdev->parent->uclass_priv;
	if (!upriv || !upriv->mmc) {
		printf("[%s]ERROR: parent->uclass_priv or uclass_priv->mmc is null\n", __FUNCTION__);
		return -1;
	}
	mmc = upriv->mmc;
	capacity_user = mmc->capacity_user;
	printf("mmc capacity_user is 0x%llx\n", capacity_user);
#endif
#if !defined(CONFIG_SUPPORT_EMMC_BOOT) && defined(CONFIG_MTD_SPI_NAND)
	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}

	mtd = dev_get_uclass_priv(dev);
	capacity_user = mtd->size;
	printf("nand capacity_user is 0x%llx\n", capacity_user);
#endif
#if !defined(CONFIG_MTD_SPI_NAND) && defined(CONFIG_SPI_FLASH)
	ret = uclass_get_device(UCLASS_SPI_FLASH, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}

	flash = dev_get_uclass_priv(dev);
	capacity_user = flash->size;
	printf("nor capacity_user is 0x%llx\n", capacity_user);
#endif

	if (fdl_part_header.dwMagic != PARTITION_HEADER_MAGIC) {
		pr_err("cmd_repartition partition header magic error\n");
		return -1;
	}

	part_table_len = fdl_part_header.part_count * sizeof(struct fdl_partition);
	fdl_part_table = (fdl_partition_t *) malloc(part_table_len);
	if (fdl_part_table == NULL) {
		return -1;
	}
	memset(fdl_part_table, 0, part_table_len);

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
		mmc_parts = strstr(BOOTARGS_EMMC, "mmcblk0:");
		len = (mmc_parts - BOOTARGS_EMMC) + strlen("mmcblk0:");
		memcpy(buf, BOOTARGS_EMMC, len);
		break;
	case STORAGE_TYPE_NAND:
		mmc_parts = strstr(BOOTARGS_SPINAND, "spi4.0:");
		len = (mmc_parts - BOOTARGS_SPINAND) + strlen("spi4.0:");
		memcpy(buf, BOOTARGS_SPINAND, len);
		break;
	case STORAGE_TYPE_NOR:
		mmc_parts = strstr(BOOTARGS_SPINOR, "spi4.0:");
		len = (mmc_parts - BOOTARGS_SPINOR) + strlen("spi4.0:");
		memcpy(buf, BOOTARGS_SPINOR, len);
		break;
	default:
		ret = -1;
		printf("%s: %d, storage_sel %d error\n", __FUNCTION__, __LINE__, boot_info_data.storage_sel);
		break;
	}
	if (ret)
		frame_send_respone(FDL_RESP_INVALID_PARTITION);

	for (i = 0; i < fdl_part_header.part_count; i++) {
		fdl_part_cur = (void *)(pframe->data + FDL_PART_HEADER_LEN + i * REPARTITION_PART_ENTRY_LEN);

		memcpy((void *)fdl_part_table[i].part_id, (void *)fdl_part_cur, CMD_PART_ID_SIZE);
		for (j =0; j < CMD_PART_ID_SIZE; j++) {
			partition_name[j] = fdl_part_table[i].part_id[j];
		}
		partition_name[CMD_PART_ID_SIZE -1] = '\0';

		temp = *(u32 *) (fdl_part_cur + REPARTITION_PART_SIZE_OFF + 4);
		temp = temp << 32;
		temp = *(u32 *) (fdl_part_cur + REPARTITION_PART_SIZE_OFF) | temp;
		fdl_part_table[i].part_sizeof_unit = temp;

		temp = *(u32 *) (fdl_part_cur + REPARTITION_PART_GAP_OFF + 4);
		temp = temp << 32;
		temp = *(u32 *) (fdl_part_cur + REPARTITION_PART_GAP_OFF) | temp;
		fdl_part_table[i].part_gapof_unit = temp;

		switch (fdl_part_header.size_unit) {
		case 0:
			fdl_part_table[i].sizeof_bytes = fdl_part_table[i].part_sizeof_unit * SZ_1M;
			fdl_part_table[i].gapof_bytes = fdl_part_table[i].part_gapof_unit * SZ_1M;
			break;
		case 1:
			fdl_part_table[i].sizeof_bytes = fdl_part_table[i].part_sizeof_unit * SZ_512K;
			fdl_part_table[i].gapof_bytes = fdl_part_table[i].part_gapof_unit * SZ_512K;
			break;
		case 2:
			fdl_part_table[i].sizeof_bytes = fdl_part_table[i].part_sizeof_unit * SZ_1K;
			fdl_part_table[i].gapof_bytes = fdl_part_table[i].part_gapof_unit * SZ_1K;
			break;
		case 3:
			fdl_part_table[i].sizeof_bytes = fdl_part_table[i].part_sizeof_unit * SZ_1;
			fdl_part_table[i].gapof_bytes = fdl_part_table[i].part_gapof_unit * SZ_1;
			break;
		default:
			fdl_part_table[i].sizeof_bytes = fdl_part_table[i].part_sizeof_unit * SZ_1M;
			fdl_part_table[i].gapof_bytes = fdl_part_table[i].part_gapof_unit * SZ_1M;
			break;
		}
		/* only the last partition can use the all rest blks */
		if (fdl_part_table[i].part_sizeof_unit == 0xffffffff) {
#ifndef SPI_DUAL_CS
			if (i == (fdl_part_header.part_count - 1)) {
#endif
				if (fdl_part_header.size_unit == 3) {
					pr_err("The minimum partition unit is KByte, and bytes are not supported.\n");
					return -1;
				}
				if (fdl_part_header.size_unit == 0) {
					fdl_part_table[i].part_sizeof_unit = (capacity_user - use_part_size) / 1024 / 1024;	//M
				}
				if (fdl_part_header.size_unit == 1) {
					fdl_part_table[i].part_sizeof_unit = (capacity_user - use_part_size) / 1024 / 512;
				}
				if (fdl_part_header.size_unit == 2) {
					fdl_part_table[i].part_sizeof_unit = (capacity_user - use_part_size) / 1024;	//K
				}

#ifdef SPI_DUAL_CS
				if (!flash_cs)
					last_part_cs0 = 1;
#else
			} else {
				pr_err("not the last partition cannot use all rest blks\n");
				return -1;
			}
#endif
		} else {
#ifdef SPI_DUAL_CS
		if (!flash_cs)
#endif
			use_part_size += fdl_part_table[i].sizeof_bytes;
		}

		if (fdl_part_header.size_unit == 0)
			len += snprintf(buf + len, PAGE_SIZE - len, "%lldM(%s),", fdl_part_table[i].part_sizeof_unit, partition_name);
		if (fdl_part_header.size_unit == 1)
			len +=
			    snprintf(buf + len, PAGE_SIZE - len, "%lldK(%s),",
				     fdl_part_table[i].part_sizeof_unit * SZ_512, partition_name);
		if (fdl_part_header.size_unit == 2)
#ifdef SPI_DUAL_CS
		if (flash_cs) {
			len_cs1 += snprintf(sf1_parts + len_cs1, PAGE_SIZE - len_cs1, "%lldK(%s),", fdl_part_table[i].part_sizeof_unit, partition_name);
			strcpy(g_sf1_part_info[part_index_cs1].part_name, partition_name);
			g_sf1_part_info[part_index_cs1].part_offset = use_part_size_cs1;
			g_sf1_part_info[part_index_cs1].part_size = fdl_part_table[i].sizeof_bytes;
			g_sf1_part_info[part_index_cs1].part_index = part_index_cs1;
			use_part_size_cs1 += fdl_part_table[i].sizeof_bytes;
			printf("sf1 part %d: name %s, addr 0x%llX, size 0x%llX\n",
				g_sf1_part_info[part_index_cs1].part_index, g_sf1_part_info[part_index_cs1].part_name,
				g_sf1_part_info[part_index_cs1].part_offset, g_sf1_part_info[part_index_cs1].part_size);
			part_index_cs1++;
		}
		else
#endif
			len += snprintf(buf + len, PAGE_SIZE - len, "%lldK(%s),", fdl_part_table[i].part_sizeof_unit, partition_name);
#ifdef SPI_DUAL_CS
		if (last_part_cs0) {
			flash_cs++;
			last_part_cs0 = 0;
			strcpy(sf1_parts, "spi4.1:");
			len_cs1 = strlen(sf1_parts);
		}
#endif
	}
	/* remove , */
	buf[strlen(buf) - 1] = '\0';
	printf("buf: %s\n", buf);
	env_set("bootargs", buf);
#ifdef SPI_DUAL_CS
	sf1_parts[strlen(sf1_parts) - 1] = '\0';
	printf("sf1_parts: %s\n", sf1_parts);
	env_set("sf1_parts", sf1_parts);
#endif
	env_save();		//for set bootargs to env
	free(fdl_part_table);
	frame_send_respone(FDL_RESP_ACK);
	return 0;
}

//#define SPINOR1_TEST
#ifdef SPINOR1_TEST
extern void ddr_set(ulong addr, ulong size);
extern int ddr_check(ulong addr, ulong size);
static int spinor1_test(void)
{
	int ret = -1;
	int cs = 0;
	ulong ddr_addr = 0x40000000;
	ulong flash_addr = 0;
	ulong len = 0x10000;
	char cmd[256];

	printf("==================== spinor1 flash test ====================\n");
	sprintf(cmd, "sf probe %d", cs++);
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);

	sprintf(cmd, "sf probe %d", cs);
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);

	sprintf(cmd, "sf erase 0x%lX 0x%lX", flash_addr, len);
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);

	ddr_set(ddr_addr, len);

	sprintf(cmd, "sf write 0x%lX 0x%lX 0x%lX", ddr_addr, flash_addr, len);
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);

	memset((void *)ddr_addr, 0xff, len);
	ret = ddr_check(ddr_addr, len);

	sprintf(cmd, "sf read 0x%lX 0x%lX 0x%lX", ddr_addr, flash_addr, len);
	printf("%s\n", cmd);
	ret = run_command(cmd, 0);

	ret = ddr_check(ddr_addr, len);
	printf("=============================================================\n");

	return ret;
}
#endif

int cmd_reboot(fdl_frame_t * pframe, void *arg)
{
	frame_send_respone(FDL_RESP_ACK);
	set_reboot_mode_after_dl();
#ifdef SPINOR1_TEST
	spinor1_test();
#endif
	reboot();
	return 0;
}

int cmd_read_start(fdl_frame_t * pframe, void *arg)
{
	int i;
	u64 temp;
	u16 ch16;
	struct fdl_read_info *pfile = (struct fdl_read_info *)arg;

	printf("%s magic_num 0x%X, data_len 0x%X, cmd_index 0x%X\n", __FUNCTION__, pframe->magic_num, pframe->data_len, pframe->cmd_index);

	dl_buf_addr = FDL_BUF_ADDR;
	dl_buf_size = FDL_BUF_LEN;
	if (dl_buf_size == 0) {
		//error("fdl buf len is 0 bytes, cannot receive data\n");
		return -1;
	}
	memset((void *)pfile, 0, sizeof(struct fdl_read_info));

	/*
	 *                           cmd_start_trnafer format
	 *      | header | data_len | cmd | data area | checksum |
	 *      | 4B | 4B | 4B | par_id 72B + size 8B + rsv 8 + chksum 4B(op) | 2bytes |
	 */
	for (i = 0; i < 31; i++) {
		ch16 = *(u16 *) (pframe->data + 2 * i);
		if (!ch16)
			break;
		pfile->part_name[i] = (char)ch16;
	}

	if (!pfile->part_name) {
		printf("%s: part name is null\n", __FUNCTION__);
		return -1;
	}

	if (0 != common_get_part_info(pfile->part_name, &pfile->part_base_addr, &pfile->part_size)) {
		frame_send_respone(FDL_RESP_INVALID_PARTITION);
		return -1;
	}
	printf("%s: part name %s, base addr 0x%llX, part size 0x%llX\n", __FUNCTION__, pfile->part_name, pfile->part_base_addr, pfile->part_size);

	if (pframe->data_len == CMD_START_DATALEN_WITH_CHKSUM) {
		pfile->chksum_flag = 1;
	} else if (pframe->data_len == CMD_START_DATALEN_NO_CHKSUM) {
		pfile->chksum_flag = 0;
		pfile->send_chksum = 0;
	} else {
		//error("cmd_start_tranfer format error\n");
		frame_send_respone(FDL_RESP_INVLID_CMD);
		return -1;
	}

	temp = *(u32 *) (pframe->data + CMD_START_IMG_SIZE_OFF + 4);
	temp = temp << 32;
	pfile->request_len = *(u32 *) (pframe->data + CMD_START_IMG_SIZE_OFF) | temp;	//combined to 64 bit data
	printf("%s: request len 0x%llX, chksum_flag 0x%X\n", __FUNCTION__, pfile->request_len, pfile->chksum_flag);
	if (pfile->request_len > pfile->part_size) {
		printf("exceed partition size\n");
		frame_send_respone(FDL_RESP_SIZE_ERR);
		return -1;
	}

	pfile->buf_addr = dl_buf_addr;
	pfile->buf_point = dl_buf_addr;
	pfile->buf_len = 0;
	pfile->buf_send_len = 0;
	pfile->total_send_len = 0;
	pfile->part_read_size = 0;

	frame_send_respone(FDL_RESP_ACK);

	return 0;
}

int cmd_read_data(fdl_frame_t * pframe, void *arg)
{
	u64 buf_last_len, buf_free_len;
	u64 part_last_len, read_len;
	u64 temp;
	//u64 pkg_offset;
	u32 pkg_len;
	struct fdl_read_info *pfile = (struct fdl_read_info *)arg;
	pkg_len = *(u32 *) (pframe->data + 8);
	temp = *(u32 *) (pframe->data + 4);
	temp = temp << 32;

	if (pfile->buf_send_len + pkg_len > pfile->buf_len) {
		if (pfile->buf_point != pfile->buf_addr) {
			if (pfile->buf_len > pfile->buf_send_len) {
				buf_last_len = pfile->buf_len - pfile->buf_send_len;
			} else {
				printf("ERROR: buf_send_len 0x%lX over buf_len 0x%lX\n", pfile->buf_send_len, pfile->buf_len);
				return -1;
			}
			memcpy((void *)pfile->buf_addr, (void *)pfile->buf_point, buf_last_len);
			pfile->buf_len = buf_last_len;
			pfile->buf_send_len = 0;
			printf
			    ("[Move tail buf_point 0x%lX to buf_addr 0x%lX] buf_len 0x%lX, buf_send_len 0x%lX, total_send_len 0x%llX, part_read_size 0x%llX\n",
			     pfile->buf_point, pfile->buf_addr, pfile->buf_len, pfile->buf_send_len, pfile->total_send_len, pfile->part_read_size);
			pfile->buf_point = pfile->buf_addr;
		}

		if (pfile->buf_point == pfile->buf_addr) {
			if (pfile->buf_len >= dl_buf_size) {
				printf("ERROR: unsend buf len 0x%lX over buf limit size 0x%lX\n", pfile->buf_len, dl_buf_size);
			} else {
				buf_free_len = dl_buf_size - pfile->buf_len;

				if (pfile->part_read_size >= pfile->request_len) {
					printf("ERROR: read size 0x%llX over request len 0x%llX\n", pfile->part_read_size, pfile->request_len);
					return -1;
				}
				part_last_len = pfile->request_len - pfile->part_read_size;

				temp = (part_last_len < buf_free_len) ? part_last_len : buf_free_len;
				read_len =
				    common_raw_read(pfile->part_name,
						    (pfile->part_base_addr +
						     pfile->part_read_size), temp, (char *)(pfile->buf_point + pfile->buf_len));
				if (read_len != temp) {
					printf("ERROR: result read len 0x%llX, read size 0x%llx\n", read_len, temp);
					return -1;
				}
				printf("[Read 0x%llX bytes to addr 0x%lX] ", temp, (pfile->buf_point + pfile->buf_len));
				pfile->part_read_size += temp;
				pfile->buf_len += temp;
				printf
				    ("buf_point 0x%lX, buf_len 0x%lX, buf_send_len 0x%lX, total_send_len 0x%llX, part_read_size 0x%llX\n",
				     pfile->buf_point, pfile->buf_len, pfile->buf_send_len, pfile->total_send_len, pfile->part_read_size);
			}
		}
	}

	if (pfile->buf_send_len + pkg_len <= pfile->buf_len) {
		frame_send_data(FDL_RESP_FLASH_DATA, (char *)pfile->buf_point, pkg_len);
		pfile->buf_send_len += pkg_len;
		pfile->buf_point += pkg_len;
		pfile->total_send_len += pkg_len;
		if (pfile->buf_len == pfile->buf_send_len) {
			pfile->buf_point = pfile->buf_addr;
			pfile->buf_len = 0;
			pfile->buf_send_len = 0;
		}
		if (pfile->total_send_len == pfile->request_len)
			printf
			    ("after send frame buf_addr 0x%lX, buf_point 0x%lX, buf_len 0x%lX, buf_send_len 0x%lX, total_send_len 0x%llX, part_read_size 0x%llX\n",
			     pfile->buf_addr, pfile->buf_point, pfile->buf_len, pfile->buf_send_len, pfile->total_send_len, pfile->part_read_size);
	}

	return 0;
}

int cmd_read_end(fdl_frame_t * pframe, void *arg)
{
	printf("%s magic_num 0x%X, data_len 0x%X, cmd_index 0x%X\n", __FUNCTION__, pframe->magic_num, pframe->data_len, pframe->cmd_index);

	frame_send_respone(FDL_RESP_ACK);
	return 0;
}

int cmd_erase(fdl_frame_t * pframe, void *arg)
{
	int i;
	u64 temp, flag, size, part_addr, part_size;
	u16 ch16;
	char name[36] = { 0 };
	printf("%s: magic_num 0x%X, data_len 0x%X, cmd_index 0x%X\n", __FUNCTION__, pframe->magic_num, pframe->data_len, pframe->cmd_index);

	temp = *(u32 *) (pframe->data + 4);
	temp = temp << 32;
	flag = *(u32 *) (pframe->data) | temp;	//combined to 64 bit data

	if (flag == 1) {
		printf("%s: flag 0x%llX\n", __FUNCTION__, flag);
		if (common_raw_erase("eraseall", 0, 0)) {
			frame_send_respone(FDL_RESP_OPERATION_FAIL);
			return -1;
		}
	} else {
		for (i = 0; i < 35; i++) {
			ch16 = *(u16 *) (pframe->data + sizeof(flag) + 2 * i);
			if (!ch16)
				break;
			name[i] = (char)ch16;
		}

		temp = *(u32 *) (pframe->data + sizeof(flag) + sizeof(name) * sizeof(ch16) + 4);
		temp = temp << 32;
		size = *(u32 *) (pframe->data + sizeof(flag) + sizeof(name) * sizeof(ch16)) | temp;	//combined to 64 bit data
		printf("%s: flag 0x%llX, part %s, erase size 0x%llX\n", __FUNCTION__, flag, name, size);

		if (0 != common_get_part_info(name, &part_addr, &part_size)) {
			frame_send_respone(FDL_RESP_DEST_ERR);
			return -1;
		}

		if (size > part_size) {
			frame_send_respone(FDL_RESP_SIZE_ERR);
			return -1;
		}
		if (size == 0) {
			size = part_size;
		}
		if (common_raw_erase(name, part_addr, size)) {
			frame_send_respone(FDL_RESP_OPERATION_FAIL);
			return -1;
		}
	}

	frame_send_respone(FDL_RESP_ACK);
	return 0;
}

int cmd_set_baudrate(fdl_frame_t * pframe, void *arg)
{
	int ret;
	u32 baudrate_new;

	baudrate_new = *((u32 *) pframe->data);
	frame_send_respone(FDL_RESP_ACK);
	ret = g_CurrChannel->setbaudrate(g_CurrChannel, baudrate_new);
	if (ret)
		while (1) ;

	return 0;
}

int cmd_connect(fdl_frame_t * pframe, void *arg)
{
	int ret;

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

void fdl_dl_init(void)
{
#if defined(CONFIG_AXERA_SECURE_BOOT) && defined(CONFIG_CMD_AXERA_CIPHER)
	AX_CIPHER_Init();
#endif

	memset(cmdproc_tab, 0, sizeof(cmdproc_tab));

	cmdproc_register(FDL_CMD_CONNECT, cmd_connect, 0);
	cmdproc_register(FDL_CMD_START_TRANSFER, cmd_start_transfer, &g_file_info);
	cmdproc_register(FDL_CMD_TRANSFERRING_DATA, cmd_transfer_data, &g_file_info);
	cmdproc_register(FDL_CMD_START_TRANSFER_END, cmd_transfer_end, &g_file_info);
	cmdproc_register(FDL_CMD_REPATITION, cmd_repartition, 0);
	cmdproc_register(FDL_CMD_REBOOT, cmd_reboot, 0);
	cmdproc_register(FDL_CMD_START_READ_FLASH, cmd_read_start, &g_read_info);
	cmdproc_register(FDL_CMD_READING_FLASH, cmd_read_data, &g_read_info);
	cmdproc_register(FDL_CMD_END_READ_FLASH, cmd_read_end, &g_read_info);
	cmdproc_register(FDL_CMD_ERASE_FLASH, cmd_erase, 0);
	cmdproc_register(FDL_CMD_CHG_BAUDRATE, cmd_set_baudrate, 0);
	//others add later
#if defined (CONFIG_MTD_SPI_NAND)
	spi_nand_protect_disable();
#if defined (FDL_BAD_BLKS_SCAN)
	spi_nand_scan_bad_blks();
#endif
#endif
#ifdef SPI_RX_SAMPLE_DLY_SCAN
	spi_rx_sample_delay_scan(4);
#endif
}

void fdl_dl_entry(void)
{
	int ret;
	CMD_TYPE cmd;
	fdl_frame_t *pframe;
	while (1) {
		pframe = frame_get();
		if (!pframe)
			break;

		cmd = pframe->cmd_index;
		ret = CMD_HANDLER(cmd, pframe);
		if (ret)
			return;
	}
}
