/*
 * AXERA AX620E Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/boot_mode.h>
#include <fs.h>
#include <blk.h>
#include <memalign.h>
#include <fat.h>
#include <linux/sizes.h>
#include <asm/io.h>
#include <asm/arch/ax620e.h>
#include <image-sparse.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <mtd.h>
#include <linux/mtd/spinand.h>
#include <linux/mtd/spi-nor.h>
#include "../../legacy-mtd-utils.h"
#include "axera_update.h"
#include "../cipher/ax_cipher_api.h"

#define VERSION_2_PARSE_XML

#define SD_UPDATE_BUF_SIZE     (10 * 1024 * 1024)
#define SD_UPDATE_CFG_NAME     "config"
#define SIZE_SPL_IMG_SIZE			(111 * 1024)
#define SPL_SD_ADDR            (0)
#define IMAGE_INFO_SD_ADDR     (768 * 1024)

/* mtd device types */
#define MTD_DEV_TYPE_NOR	0x0001
#define MTD_DEV_TYPE_NAND	0x0002
#define MTD_DEV_TYPE_ONENAND	0x0004
#define MTD_DEV_TYPE_SPINAND	0x0008

#ifdef CONFIG_MTD_SPI_NAND
static bool write_protect_disable = false;
#endif

static char lastXferPart[32] = { '@' };

static u32 xferPartCnt = 0;
static loff_t partOff = 0;

struct sd_update_file {
	char part_name[MAX_PART_NAME_LEN];
	u64 target_len; /* bin file size */
	u64 saved_len;
	u8 *pbuf;
};

extern struct boot_mode_info boot_info_data;
extern u_long dl_buf_addr;
extern u_long dl_buf_size;

extern void reboot(void);
extern void dump_buffer(u64 addr, u64 len);
extern int common_get_part_info(char *part_name, u64 * addr, u64 * len);
extern int common_raw_erase(char *part_name, u64 addr, u64 len);
extern int sparse_info_init(struct sparse_storage *info, const char *name);
extern int write_sparse_img(struct sparse_storage *info, char *part_name, void *data, u64 * response);

#define PARTITION_BOOT1		1
#define PARTITION_BOOT2		2

extern int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t * info);
extern int update_verify_image(const char *part_name, const char *pfile);

#ifdef CONFIG_SUPPORT_EMMC_BOOT
static int sd_update_to_emmc(struct sd_update_file *pfile, int wr_len)
{
	lbaint_t start_lba, blkcnt_lb, base_lba;
	lbaint_t written_lb;
	lbaint_t wr_blkcnt_lb;
	struct blk_desc *blk_dev_desc = NULL;
	int ret;
	ulong blksz;
	disk_partition_t part_info;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		pr_err("get mmc dev fail\n");
		return -1;
	}
	blksz = blk_dev_desc->blksz;

	blkcnt_lb = PAD_COUNT(wr_len, blksz);
	if (pfile->saved_len)
		written_lb = PAD_COUNT(pfile->saved_len, blksz);
	else
		written_lb = 0;

	ret = get_part_info(blk_dev_desc, pfile->part_name, &part_info);
	if (ret == -1) {
		printf("%s: get %s partition info fail\n", __FUNCTION__, pfile->part_name);
	}

	if (!strcmp(pfile->part_name, "spl")) {
		ret = is_emmc_switch_boot_part1(blk_dev_desc);
		if(ret != 0) {
			return -1;
		}
		base_lba = part_info.start;

		start_lba = base_lba + written_lb;

		wr_blkcnt_lb = blk_dwrite(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->pbuf);
		if (wr_blkcnt_lb != blkcnt_lb) {
			pr_err("some error happend while write to disk\n");
			return -1;
		}
	} else if (!strcmp(pfile->part_name, "uboot")) {
		ret = emmc_switch_to_uda_part(blk_dev_desc);
		if(ret != 0) {
			return -1;
		}
		base_lba = part_info.start;
		start_lba = base_lba + written_lb;

		wr_blkcnt_lb = blk_dwrite(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->pbuf);
		if (wr_blkcnt_lb != blkcnt_lb) {
			pr_err("some error happend while write to disk\n");
			return -1;
		}
	} else {
		ret = emmc_switch_to_uda_part(blk_dev_desc);
		if(ret != 0) {
			return -1;
		}
		base_lba = part_info.start;
		start_lba = base_lba + written_lb;
		if ((written_lb + blkcnt_lb) > part_info.size) {
			pr_err("exceed partition size\n");
			return -1;
		}

		wr_blkcnt_lb = blk_dwrite(blk_dev_desc, start_lba, blkcnt_lb, (void *)pfile->pbuf);
		if (wr_blkcnt_lb != blkcnt_lb) {
			pr_err("some error happend while write to disk\n");
			return -1;
		}
	}

	pfile->saved_len += wr_len;
	return 0;
}
#endif

#ifdef CONFIG_MTD_SPI_NAND
int sd_spi_nand_protect_disable(void)
{
       int ret;
       u32 busnum = 0;
       u8 sr1;
       struct udevice *dev = NULL;
       struct mtd_info *mtd = NULL;
       struct spinand_device *spinand = NULL;
       struct spi_mem_op op = SPINAND_SET_FEATURE_OP(REG_BLOCK_LOCK, &sr1);

       if (write_protect_disable)
               return 0;

       ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
       if (ret) {
               printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
               return ret;
       }

       mtd = dev_get_uclass_priv(dev);
       if (NULL == mtd) {
               printf("dev_get_uclass_priv: fail\n");
               return ret;
       }

       spinand = mtd_to_spinand(mtd);
       if (NULL == spinand) {
               printf("mtd_to_spinand: fail\n");
               return ret;
       }

       sr1 = 0;
       ret = spi_mem_exec_op(spinand->slave, &op);
       if (ret) {
               printf("spi_mem_exec_op: write sr1 (err=%d)\n", ret);
               return ret;
       }
       write_protect_disable = true;
       return 0;
}

int sd_update_to_spinand(struct sd_update_file *pfile, int wr_len)
{

/* 1. mtd spi nand device init */
	u32 ret, dl_buf_off;
	u32 busnum = 0;
	u64 remaining;
	loff_t off = 0;
	int idx;
	loff_t size, maxsize;
	size_t retlen;
	int write_size;
	static int mtdpart_skip_blk_cnt = 0;
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
				pr_err("%s: erase addr 0x%llX len 0x%X over %s part addr 0x%llX size 0x%llX\n",
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
				pr_err("erase skip bad block @off = 0x%llX size = 0x%X\n", erase_op.addr,
				       mtd->erasesize);
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
	off = partOff + xferPartCnt * SD_UPDATE_BUF_SIZE + mtdpart_skip_blk_cnt * mtd->erasesize;	/* SD_UPDATE_BUF_SIZE = 10M */

	if (off & (mtd->erasesize - 1)) {
		pr_err("nand addr 0x%llX is not block size 0x%X aligned!\n", off, mtd->erasesize);
		return -1;
	}
	xferPartCnt++;

	remaining = wr_len;
	dl_buf_off = 0;
	printf("buf: wr_len=0x%X, maxsize=0x%X\n", wr_len, SD_UPDATE_BUF_SIZE);
	while (remaining) {
		if (mtd_block_isbad(mtd, off)) {
			mtdpart_skip_blk_cnt++;
			ret = off;
			do_div(ret, mtd->erasesize);
			printf("nand addr 0x%llX, blk %lld is bad, mtdpart_skip_blk_cnt=%d\n", off,
			       ret, mtdpart_skip_blk_cnt);
			off += mtd->erasesize;
			continue;
		}

		write_size = (remaining > mtd->erasesize) ? mtd->erasesize : remaining;
		if (off + write_size > partOff + size) {
			pr_err("%s: write addr 0x%llX len 0x%X over %s part addr 0x%llX size 0x%llX\n",
			       __func__, off, write_size, pfile->part_name, partOff, size);
			return -1;
		}
		if (dl_buf_off + write_size > wr_len) {
			pr_err("%s: dl_buf_off 0x%X len 0x%X over unsave_recv_len 0x%X\n",
			       __func__, dl_buf_off, write_size, wr_len);
			return -1;
		}

		mtd_write(mtd, off, write_size, &retlen, (u_char *) (pfile->pbuf + dl_buf_off));
		if (retlen != write_size) {
			pr_err("write partition fail!.\n");
			return -1;
		}
		printf("partiton %s wrote 0x%lX bytes, dl_buf_off 0x%X ==> nand addr 0x%llX\n", pfile->part_name, retlen,
		       dl_buf_off, off);
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
static int sd_update_to_spinor(struct sd_update_file *pfile, int wr_len)
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

	mtd_write(mtd, off, wr_len, &retlen, (u_char *)pfile->pbuf);
	if (retlen != wr_len) {
		printf("write partition fail!.\n");
		return -1;
	}
	printf("partiton %s write @off = 0x%llx size = 0x%x\n", pfile->part_name, off, wr_len);

	strcpy(lastXferPart, pfile->part_name);
	return 0;

}
#endif

static int sd_update_to_storage(struct sd_update_file *pfile, int len)
{
	int ret = 0;
#if defined CONFIG_SPI_FLASH || CONFIG_MTD_SPI_NAND
	char *bootargs = NULL;
	char *mtdparts = NULL;
#endif

	ret = update_verify_image(pfile->part_name, pfile->pbuf);
	if (ret)
		return -1;

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
#ifdef CONFIG_SUPPORT_EMMC_BOOT
		ret = sd_update_to_emmc(pfile, len);
#endif
		break;
	case STORAGE_TYPE_NAND:
#ifdef CONFIG_MTD_SPI_NAND
		bootargs = env_get("bootargs");
		if (NULL != bootargs) {
			mtdparts = strstr(bootargs , "mtdparts");
			if (NULL != mtdparts) {
				mtdparts = strdup(mtdparts);
				strtok(mtdparts, " ");
				env_set("mtdparts", mtdparts);
				free(mtdparts);
			}
		}
		sd_spi_nand_protect_disable();
		ret = sd_update_to_spinand(pfile, len);
#endif
		break;
	case STORAGE_TYPE_NOR:
#ifdef CONFIG_SPI_FLASH
		env_set("mtdids", MTDIDS_SPINOR);
		env_set("mtdparts", MTDPARTS_SPINOR);

		bootargs = env_get("bootargs");
		if (NULL != bootargs) {
			mtdparts = strstr(bootargs , "mtdparts");
			if (NULL != mtdparts) {
				mtdparts = strdup(mtdparts);
				strtok(mtdparts, " ");
				env_set("mtdparts", mtdparts);
				free(mtdparts);
			}
		}
		ret = sd_update_to_spinor(pfile, len);
#endif
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

#ifndef VERSION_2_PARSE_XML
static int sd_update_parse(struct update_part_info **bin_info)
{
	int ret = 0;
	int p_count = 0;
	int i;
	loff_t size, actread;
	struct update_part_info *pheader = NULL;
	struct update_part_info *pbin_info;
	struct update_part_info *pcurr = NULL;
	char *pconfig = NULL;
	char *tok, *val, *str, *p;

	if (!fat_exists(SD_UPDATE_CFG_NAME)) {
		printf("sd update config file is not exist, exit sd update\n");
		return -1;
	}

	if (!fat_size(SD_UPDATE_CFG_NAME, &size)) {
		pconfig = (char *)malloc(size + 1);
		if (!pconfig) {
			printf("sd update malloc config space fail, exit sd update\n");
			return -1;
		}
		memset(pconfig, 0, size + 1);
	} else {
		printf("get config file size fail, exit sd update\n");
		return -1;
	}

	ret = fat_read_file(SD_UPDATE_CFG_NAME, pconfig, 0, size, &actread);
	if (ret || size != actread) {
		printf("read sd_update config file fail, exit sd update\n");
		goto free_config;
	}

	str = pconfig;
	while (*str) {
		if (*str++ == ';')
			p_count += 1;
	}

	str = pconfig;
	for (i = 0; i < p_count; i++) {
		tok = strsep(&str, ";");

		pbin_info = malloc(sizeof(struct update_part_info));
		if (!pbin_info) {
			ret = -1;
			goto free_node;
		}
		memset(pbin_info, 0, sizeof(struct update_part_info));

		/* get part name */
		val = extract_val(tok, "name");
		if (!val) {
			ret = -1;
			goto free_node;
		}
		strncpy(pbin_info->part_name, val, MAX_PART_NAME_LEN - 1);
		free(val);

		/* get part size */
		val = extract_val(tok, "size");
		if (!val) {
			ret = -1;
			goto free_node;
		}
		p = val;
		if (!strcmp(val, "0xFFFFFFFF") || !strcmp(val, "0xffffffff") ||
		    !strcmp(val, "0XFFFFFFFF") || !strcmp(val, "0Xffffffff"))
			pbin_info->part_size = -1;
		else
			pbin_info->part_size = ustrtoull(p, &p, 0);
		free(val);

		/* get file name */
		val = extract_val(tok, "file");
		if (!val) {
			ret = -1;
			goto free_node;
		}
		strncpy(pbin_info->file_name, val, MAX_FILE_NAME_LEN - 1);
		free(val);

		if (!pheader) {
			pheader = pbin_info;
			pcurr = pbin_info;
		} else {
			pcurr->next = pbin_info;
			pcurr = pbin_info;
		}

	}

	if (pcurr)
		pcurr->next = NULL;
	*bin_info = pheader;
	printf("successfully parsed sd config file\n");

	goto free_config;

free_node:
	while (pheader) {
		pbin_info = pheader;
		pheader = pheader->next;
		free(pbin_info);
	}

free_config:
	free(pconfig);

	return ret;
}
#endif

static int sd_update_bin_check(struct update_part_info *pheader)
{
	loff_t size;
	struct update_part_info *pcur = pheader;

	if (!pcur) {
		printf("no sd update bin information, exit sd update\n");
		return -1;
	}

	while (pcur) {
		/* check file is none not update */
		if (!strcmp(pcur->file_name, "none")) {
			pcur = pcur->next;
			continue;
		}

		/* check file is exist */
		if (!fat_exists(pcur->file_name)) {
			printf("%s is not exist, exit sd update\n", pcur->file_name);
			return -1;
		}

		/* check file size must low than part size */
		if (!fat_size(pcur->file_name, &size)) {
			if (size > pcur->part_size) {
				printf("%s size:%llu, but part size:%llu, exit sd update\n",
				       pcur->file_name, size, pcur->part_size);
				return -1;
			}
		} else {
			printf("get %s size fail, exit sd update\n", pcur->file_name);
			return -1;
		}

		pcur = pcur->next;
	}

	printf("sd update bin check success\n");

	return 0;
}

#ifdef USE_GPT_PARTITON
#define AXERA_DISK_GUID_STR    "12345678-1234-1234-1234-000000000000"
static int sd_update_repatition_emmc(struct update_part_info *pheader)
{
	u16 i, ret;
	int part_count = 0;
	struct update_part_info *pcur = pheader;
	struct update_part_info *ptemp = NULL;
	u32 part_table_len;
	ulong blksz;
	disk_partition_t *partitions = NULL;
	struct blk_desc *blk_dev_desc = NULL;
	u64 disk_part_start_lba = 0;
	u64 disk_part_size_lb, disk_part_gap_lb;

	/* spl & uboot store in boot partition, not create gpt partition */
	while (pcur) {
		if (strcmp(pcur->part_name, "spl") && strcmp(pcur->part_name, "uboot"))
			part_count++;

		pcur = pcur->next;
	}

	part_table_len = part_count * sizeof(disk_partition_t);
	partitions = (disk_partition_t *) malloc(part_table_len);
	if (partitions == NULL) {
		pr_err("sd_update_repatition_emmc alloc disk_partition_t error\n");
		return -1;
	}
	memset(partitions, 0, part_table_len);

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		pr_err("get mmc dev fail\n");
		ret = -1;
		goto free_ret;
	}
	blksz = blk_dev_desc->blksz;

	ptemp = pheader;
	for (i = 0; i < part_count; i++) {
		while (ptemp) {
			pcur = ptemp;
			if (strcmp(pcur->part_name, "spl") && strcmp(pcur->part_name, "uboot")) {
				ptemp = ptemp->next;
				break;
			}
			ptemp = ptemp->next;
		}

		partitions[i].blksz = blksz;
		partitions[i].bootable = 0;

		strcpy((char *)partitions[i].name, pcur->part_name);

		if (pcur->part_size != 0)
			disk_part_size_lb = PAD_COUNT(pcur->part_size, blksz);
		else {
			printf("partition size is 0, exit sd update\n");
			ret = -1;
			goto free_ret;
		}

		/* if first partition we start from fixed 1MB of disk */
		if (0 == i)
			disk_part_gap_lb = SZ_1M / blksz;
		else
			disk_part_gap_lb = 0;

#if 1
		/* only the last partition can use the all rest blks */
		if (pcur->part_size == -1) {
			if (i == (part_count - 1)) {
				disk_part_size_lb = 0;
			} else {
				pr_err("not the last partition cannot use all rest blks\n");
				ret = -1;
				goto free_ret;
			}
		}
#endif
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

free_ret:
	free(partitions);
	return ret;
}

static int sd_update_repartition(struct update_part_info *pheader)
{
	int ret;

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
		ret = sd_update_repatition_emmc(pheader);
		break;
	case STORAGE_TYPE_NAND:
		//ret = sd_update_repatition_nand(pheader);
		ret = -1;	//temp
		break;
	case STORAGE_TYPE_NOR:
		//ret = sd_update_repatition_nor(pheader);
		ret = -1;	//temp
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}
#endif

static int sd_update_save_storage(struct update_part_info *pheader)
{
	u8 *pbuf = NULL;
	loff_t actread, remain_read;
	u32 len_read;
	int ret = 0;
	int read_cnt;
	loff_t size;
	struct sd_update_file update_file_info;
	struct update_part_info *pcur = pheader;
	struct sparse_storage sd_sparse;
	int sparse_download_enable = 0;
	u32 free_buf_len;
	u64 sparse_buf_last, sparse_buf_point;
	u64 unsave_recv_len = 0;
	u64 curr_addr = SPARSE_IMAGE_BUF_ADDR;
	u64 part_addr, part_size;
	dl_buf_addr = SPARSE_IMAGE_BUF_ADDR;
	dl_buf_size = SPARSE_IMAGE_BUF_LEN;

#ifdef USE_GPT_PARTITON
	/* step1 repartition */
	ret = sd_update_repartition(pheader);
	if (ret) {
		printf("sd update repartition fail\n");
		return -1;
	}
#endif

	ret = update_parts_info(pheader);
	if (ret) {
		printf("update_parts_info fail\n");
		return ret;
	}
	/* step2 read file and save to storage */
	// pbuf = (u8 *)memalign(0x100000, SD_UPDATE_BUF_SIZE);
	/* Specify the address because OCM space is not large enough */
	pbuf = (u8 *) SD_UPDATE_BUF;
	if (!pbuf) {
		printf("sd update malloc %dMB fail\n", SD_UPDATE_BUF_SIZE / 1024 / 1024);
		return -1;
	}

	while (pcur) {
		/* check file is none not update */
		if (!strcmp(pcur->file_name, "none")) {
			pcur = pcur->next;
			continue;
		}

		if (boot_info_data.mode == NORMAL_BOOT_MODE || SD_UPDATE_MODE == boot_info_data.mode) {
			if (!fat_size(pcur->file_name, &size)) {
				printf("%s size is:%llu\n", pcur->file_name, size);
				remain_read = size;
				read_cnt = 0;
			} else {
				printf("sd_update_save_storage get %s size fail, exit sd update\n", pcur->file_name);
				ret = -1;
				goto free_buf;
			}
		} else {
			printf("%s size is:%llu\n", pcur->file_name, pcur->image_size);
			remain_read = pcur->image_size;
			read_cnt = 0;
		}

		memset(&update_file_info, 0, sizeof(update_file_info));
		strcpy(update_file_info.part_name, pcur->part_name);
		update_file_info.target_len = size;
		update_file_info.pbuf = pbuf;

		while (remain_read > 0) {
			if (remain_read > SD_UPDATE_BUF_SIZE)
				len_read = SD_UPDATE_BUF_SIZE;
			else
				len_read = remain_read;

			ret = fat_read_file(pcur->file_name, pbuf, read_cnt * SD_UPDATE_BUF_SIZE, len_read, &actread);
			if (ret || len_read != actread) {
				printf("read %s fail\n", pcur->file_name);
				ret = -1;
				goto free_buf;
			}
			if (!read_cnt && is_sparse_image((void *)pbuf)) {
				printf("%s: file %s is sparse format\n", __FUNCTION__, pcur->file_name);
				if (dl_buf_size < len_read) {
					printf("sd update sparse buf size 0x%lX is less than sd first_read size 0x%X\n",
					       dl_buf_size, len_read);
					ret = -1;
					goto free_buf;
				}
				curr_addr = SPARSE_IMAGE_BUF_ADDR;
				unsave_recv_len = 0;
				sparse_download_enable = 1;
				if (sparse_info_init(&sd_sparse, pcur->part_name)) {
					pr_err("sd update part:%s, sparse info init fail\n", pcur->part_name);
					ret = -1;
					goto free_buf;
				}
				printf("%s: part %s is sparse format, buf 0x%lX, size 0x%lX\n", __FUNCTION__,
				       pcur->part_name, dl_buf_addr, dl_buf_size);
				if (!common_get_part_info(pcur->part_name, &part_addr, &part_size)) {
					printf("%s: erase part %s, addr 0x%llX, size 0x%llX\n", __FUNCTION__,
					       pcur->part_name, part_addr, part_size);
					common_raw_erase(pcur->part_name, part_addr, part_size);
				}
			}

			if (sparse_download_enable) {
				if (unsave_recv_len + len_read > dl_buf_size) {
					free_buf_len = dl_buf_size - unsave_recv_len;
					memcpy((void *)curr_addr, (void *)pbuf, free_buf_len);
					unsave_recv_len = dl_buf_size;
					ret =
					    write_sparse_img(&sd_sparse, pcur->part_name, (void *)dl_buf_addr,
							     &sparse_buf_point);
					if (ret || (sparse_buf_point < dl_buf_addr)
					    || (sparse_buf_point > (dl_buf_addr + dl_buf_size))) {
						printf("%s: write sparse image fail, ret %d, end point addr 0x%llX\n",
						       __FUNCTION__, ret, sparse_buf_point);
						ret = -1;
						goto free_buf;
					}
					sparse_buf_last = dl_buf_addr + dl_buf_size - sparse_buf_point;
					if ((sparse_buf_last + len_read - free_buf_len) > dl_buf_size) {
						printf("%s: write sparse error, chunk size over 0x%lX\n", __FUNCTION__,
						       (dl_buf_size - len_read));
						ret = -1;
						goto free_buf;
					}
					if (sparse_buf_last) {
						printf("%s: move last buffer 0x%llX, size 0x%llX to 0x%lX\n",
						       __FUNCTION__, sparse_buf_point, sparse_buf_last, dl_buf_addr);
						memcpy((void *)dl_buf_addr, (void *)sparse_buf_point,
						       (u32) sparse_buf_last);
					}
					/* reset the curr_addr unsave_recv_len */
					curr_addr = dl_buf_addr + sparse_buf_last;
					unsave_recv_len = sparse_buf_last;
					memcpy((void *)curr_addr, (void *)(pbuf + free_buf_len),
					       (len_read - free_buf_len));
					unsave_recv_len += (len_read - free_buf_len);
					curr_addr += (len_read - free_buf_len);
				} else {
					memcpy((void *)curr_addr, (void *)pbuf, len_read);
					curr_addr += len_read;
					unsave_recv_len += len_read;
				}
			} else {
				if (sd_update_to_storage(&update_file_info, len_read)) {
					printf("write %s fail\n", pcur->file_name);
					ret = -2;
					goto free_buf;
				}
			}

			remain_read -= len_read;
			read_cnt++;
		}

		if (sparse_download_enable && unsave_recv_len) {
			ret = write_sparse_img(&sd_sparse, pcur->part_name, (void *)dl_buf_addr, NULL);
			if (ret) {
				printf("%s write sparse image fail\n", __FUNCTION__);
				ret = -1;
				goto free_buf;
			}
			sparse_download_enable = 0;
		}
		printf("%s finished to update\n", pcur->part_name);

		pcur = pcur->next;
	}
free_buf:
	return ret;
}

/* 	1. check sd is present
 *	2. check image list and size
 *	3. write image to storage
 */
int do_sd_update(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	struct blk_desc *sd_desc = NULL;
	struct update_part_info *pbin_info = NULL;
	struct update_part_info *ptemp;
	int ret;

	printf("enter sd update process\n");

#if defined(CONFIG_AXERA_SECURE_BOOT) && defined(CONFIG_CMD_AXERA_CIPHER)
	AX_CIPHER_Init();
#endif

	/* step1 check sd is present */
	sd_desc = blk_get_dev("mmc", SD_DEV_ID);
	if (NULL == sd_desc) {
		printf("sd is not present, exit sd update\n");
		goto normal_boot;
	}

	/* we register fat device */
	if (fat_register_device(sd_desc, 1)) {
		printf("sd register part1 fat fail, exit sd update\n");
		goto normal_boot;
	}

	/* step 2 check image list */
#ifdef VERSION_2_PARSE_XML
	ret = update_parse_part_info(&pbin_info);
#else
	if (boot_info_data.mode == NORMAL_BOOT_MODE ||  boot_info_data.mode == SD_UPDATE_MODE) {
		ret = sd_update_parse(&pbin_info);
	}
#endif

	if (ret) {
		printf("sd parse config file fail, exit sd update\n");
		goto normal_boot;
	} else if (sd_update_bin_check(pbin_info)) {
		printf("sd update file check fail, exit sd update\n");
		goto normal_boot;
	}
	wdt0_enable(0);
	/* step 3 write image to storage */
	if (sd_update_save_storage(pbin_info))
		goto free_node;

	printf("all bins updated successfully\n");

	env_set("sdupdate", "finish");
	env_save();
	printf("set env sdupdate to %s\n", env_get("sdupdate"));
#ifdef CONFIG_SUPPORT_EMMC_BOOT
	set_emmc_boot_mode_after_dl();
#endif
	set_reboot_mode_after_dl();
	reboot();

	/* if update suceed, it will reboot, won't come here */
normal_boot:
	if(boot_info_data.is_sd_boot == true) {
		boot_info_data.mode = SD_BOOT_MODE;
		printf("enter sd boot\n");
		run_command_list("sd_boot", -1, 0);
	} else {
#if !defined CONFIG_BOOT_OPTIMIZATION_SUPPORT
		printf("enter axera boot\n");
		run_command_list("axera_boot", -1, 0);
#else
		run_command_list("help", -1, 0);
#endif
	}
	return 0;

free_node:
	ptemp = pbin_info;
	while (ptemp) {
		pbin_info = pbin_info->next;
		free(ptemp);
		ptemp = pbin_info;
	}

	printf("env sdupdate is %s", env_get("sdupdate"));
	if (!strcmp(env_get("sdupdate"), "retry")) {
		env_set("sdupdate", "fail");
		printf("retry sd update failed, set sdupdate env to %s", env_get("sdupdate"));
	} else {
		env_set("sdupdate", "retry");
		printf("sd update failed, set sdupdate env to %s", env_get("sdupdate"));
	}
	env_save();

	return -1;
}

U_BOOT_CMD(sd_update, 1, 0, do_sd_update,
	   "download mode", "choose to enter sd update mode\n" "it is used for sd update image to storage\n");
