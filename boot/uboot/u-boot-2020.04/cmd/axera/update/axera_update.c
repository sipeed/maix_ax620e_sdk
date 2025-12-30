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
#include <mmc.h>
#include "../../legacy-mtd-utils.h"
#include <jffs2/load_kernel.h>

#include "axera_update.h"

#define INFO_PART_START_STR    "<Partition"
#define PART_DATA_UNIT_STR     "unit"
#define PART_DATA_ID_STR       "id"
#define PART_DATA_SIZE_STR     "size"
#define INFO_PART_END_STR      "</Partitions>"
#define PARTITION_BOOT1            1

#ifdef SPI_DUAL_CS
struct sf1_part_info g_sf1_part_info[SF1_MAX_PART_NUM] = {0};
char sf1_parts[256] = {0};
#endif

extern struct boot_mode_info boot_info_data;
extern int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t * info);

char lastXferPart[32] = { '@' };
char lastLoadPart[32] = { '@' };

u32 xferPartCnt = 0;
loff_t partOff = 0;
u_long dl_buf_addr = FDL_BUF_ADDR;
u_long dl_buf_size = FDL_BUF_LEN;

#ifdef CONFIG_MTD_SPI_NAND
static bool write_protect_disable = false;
#endif

#ifdef CONFIG_SUPPORT_EMMC_BOOT
#define PARTITION_BOOT1		1
#define PARTITION_BOOT2		2

int get_emmc_part_info(char *part_name, u64 * addr, u64 * len)
{
	int ret = 0;
	struct blk_desc *blk_dev_desc = NULL;
	disk_partition_t part_info;
#if CONFIG_IS_ENABLED(BLK)
	struct mmc_uclass_priv *upriv;
	struct mmc *mmc;
	(void)mmc;
	(void)upriv;
#endif

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		printf("get mmc dev fail\n");
		return -1;
	}
	/* use bootargs */
	ret = get_part_info(blk_dev_desc, part_name, &part_info);

	if (ret == -1) {
		printf("%s: get %s partition info fail\n", __FUNCTION__, part_name);
	}
	*addr = (u64) part_info.start * blk_dev_desc->blksz;
	*len = (u64) part_info.size * blk_dev_desc->blksz;
	ret = 0;

	return ret;
}
#endif

#ifdef CONFIG_MTD_SPI_NAND
int get_spinand_part_info(char *part_name, u64 * addr, u64 * len)
{
	int ret = -1;
	int idx;
	u32 busnum = 0;
	loff_t off = 0;
	loff_t size, maxsize;
	struct udevice *dev = NULL;
	struct mtd_info *mtd = NULL;

	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}

	mtd = dev_get_uclass_priv(dev);

	ret = mtd_arg_off(part_name, &idx, &off, &size, &maxsize, MTD_DEV_TYPE_NAND, mtd->size);
	if (ret)
		return ret;

	*addr = off;
	*len = size;

	return ret;
}
#endif

#ifdef CONFIG_SPI_FLASH
int get_spinor_part_info(char *part_name, u64 * addr, u64 * len)
{
	int ret = -1;
	int idx;
	u32 busnum = 0;
	loff_t off = 0;
	loff_t size, maxsize;
	struct udevice *dev = NULL;
	struct mtd_info *mtd = NULL;

	ret = uclass_get_device(UCLASS_SPI_FLASH, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}
	mtd = get_mtd_device_nm("nor0");
	mtd = dev_get_uclass_priv(dev);

	ret = mtd_arg_off(part_name, &idx, &off, &size, &maxsize, MTD_DEV_TYPE_NOR, mtd->size);
	if (ret)
		return ret;

	*addr = off;
	*len = size;

	return ret;
}
#endif

#ifdef SPI_DUAL_CS
int get_sf1_part_info(char *part_name)
{
	int i;
	int part_index = -1;

	for (i = 0; i < SF1_MAX_PART_NUM; i++) {
		if (g_sf1_part_info[i].part_name && !strcmp(part_name, g_sf1_part_info[i].part_name)) {
			part_index = g_sf1_part_info[i].part_index;
			break;
		}
	}
	return part_index;
}
#endif

int common_get_part_info(char *part_name, u64 * addr, u64 * len)
{
	int ret = -1;
#if defined CONFIG_SPI_FLASH || CONFIG_MTD_SPI_NAND
	char *bootargs = NULL;
	char *mtdparts = NULL;
#ifdef SPI_DUAL_CS
	char cmd[256];
	static u8 spinor_probe_done[2] = {0};
	int sf1_part_index;
#endif
#endif
	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
#ifdef CONFIG_SUPPORT_EMMC_BOOT
		ret = get_emmc_part_info(part_name, addr, len);
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
		printf("mtdpart: %s\n",env_get("mtdparts"));
		ret = get_spinand_part_info(part_name, addr, len);
#endif
		break;
	case STORAGE_TYPE_NOR:
#ifdef CONFIG_SPI_FLASH
#ifdef SPI_DUAL_CS
		sf1_part_index = get_sf1_part_info(part_name);
		if (!spinor_probe_done[1] && ((sf1_part_index >= 0) && (sf1_part_index < 20))) {
			sprintf(cmd, "sf probe 1");
			printf("%s\n", cmd);
			ret = run_command(cmd, 0);
			if (0 != ret) {
				printf("ret=%d\n", ret);
				return ret;
			}

			spinor_probe_done[1] = 1;
			*addr = g_sf1_part_info[sf1_part_index].part_offset;
			*len = g_sf1_part_info[sf1_part_index].part_size;
			break;
		}

		if (!spinor_probe_done[0]) {
			sprintf(cmd, "sf probe 0");
			printf("%s\n", cmd);
			ret = run_command(cmd, 0);
			if (0 != ret) {
				printf("ret=%d\n", ret);
				return ret;
			}
			spinor_probe_done[0] = 1;
		}
#endif

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
		printf("mtdpart: %s\n",env_get("mtdparts"));
		ret = get_spinor_part_info(part_name, addr, len);
#endif
		break;
	default:
		break;
	}
	if (0 == ret) {
		printf("%s: part %s, base addr 0x%llX, part size 0x%llX\n", __FUNCTION__, part_name, *addr, *len);
	} else {
		printf("[ERROR]%s: part %s not found\n", __FUNCTION__, part_name);
	}

	return ret;
}

#ifdef CONFIG_SUPPORT_EMMC_BOOT
int fdl_read_from_emmc(char *part_name, u64 addr, u64 len, char *buffer)
{
	lbaint_t start_lba, blkcnt_lb;
	lbaint_t rd_blkcnt_lb;
	struct blk_desc *blk_dev_desc = NULL;
	ulong blksz;
	ulong last_len;
	int ret;

	//disk_partition_t part_info;
	char *temp_data = NULL;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		pr_err("get mmc dev fail\n");
		return -1;
	}
	blksz = blk_dev_desc->blksz;
	if (blksz != 512) {
		printf("%s: blksz 0x%lX is error\n", __FUNCTION__, blksz);
		return -1;
	}
	if (addr % blksz) {
		printf("%s: addr 0x%llX not align\n", __FUNCTION__, addr);
		return -1;
	}
	start_lba = addr / blksz;
	if (len % blksz) {
		blkcnt_lb = len / blksz;
		last_len = len - blkcnt_lb * blksz;
	} else {
		blkcnt_lb = PAD_COUNT(len, blksz);
		last_len = 0;
	}
	printf
	    ("[%s]addr 0x%llX, len 0x%llX, start lba 0x%lX, blkcnt 0x%lX, last bytes 0x%lX\n",
	     __FUNCTION__, addr, len, start_lba, blkcnt_lb, last_len);

	if (!strcmp(part_name, "spl")) {
		ret = is_emmc_switch_boot_part1(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
	} else {
		ret = emmc_switch_to_uda_part(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
	}
#if CONFIG_IS_ENABLED(BLK)
	rd_blkcnt_lb = blk_dread(blk_dev_desc, start_lba, blkcnt_lb, (void *)buffer);
	if (rd_blkcnt_lb != blkcnt_lb) {
		printf("some error happend while read from disk\n");
		return -1;
	}
	if (last_len) {
		temp_data = malloc(blksz);
		if (temp_data == NULL) {
			printf("%s malloc %lX bytes fail\n", __FUNCTION__, blksz);
			return -1;
		}
		if (blk_dread(blk_dev_desc, (start_lba + blkcnt_lb), 1, (void *)temp_data) == 1) {
			memcpy((void *)(buffer + blkcnt_lb * blksz), (void *)temp_data, last_len);
		} else {
			printf("some error happend while read from disk\n");
			return -1;
		}
	}
#else
	rd_blkcnt_lb = blk_dev_desc->block_read(blk_dev_desc, start_lba, blkcnt_lb, (void *)buffer);
	if (rd_blkcnt_lb != blkcnt_lb) {
		printf("some error happend while read from disk\n");
		return -1;
	}
	if (last_len) {
		temp_data = malloc(blksz);
		if (temp_data == NULL) {
			printf("%s malloc 0x%lX bytes fail\n", __FUNCTION__, blksz);
			return -1;
		}
		if (blk_dev_desc->block_read(blk_dev_desc, (start_lba + blkcnt_lb), 1, (void *)temp_data) == 1) {
			memcpy((void *)(buffer + blkcnt_lb * blksz), (void *)temp_data, last_len);
		} else {
			printf("some error happend while read from disk\n");
			return -1;
		}
	}
#endif

	return (rd_blkcnt_lb * blksz + last_len);
}

//#define DEBUG_SPARSE_DL_DATA_CHECK
#ifdef DEBUG_SPARSE_DL_DATA_CHECK
#define READ_BACK_CHECK_ADDR		0x100000000
#define READ_BACK_CHECK_SIZE_LIMIT	0x6400000
int emmc_read_back_check(struct blk_desc *blk_dev_desc, lbaint_t start_lba, lbaint_t blkcnt_lb, const void *buffer)
{
	lbaint_t rd_blkcnt_lb;
	ulong blksz;
	char *read_data = NULL;

	blksz = blk_dev_desc->blksz;
	if (blksz != 512) {
		printf("[ERROR]%s: blksz 0x%lX is error\n", __FUNCTION__, blksz);
		return -1;
	}
	if (blkcnt_lb * blksz <= READ_BACK_CHECK_SIZE_LIMIT) {
		rd_blkcnt_lb = blk_dread(blk_dev_desc, start_lba, blkcnt_lb, (void *)READ_BACK_CHECK_ADDR);
		if (rd_blkcnt_lb != blkcnt_lb) {
			printf("%s: some error happend while reading disk\n", __FUNCTION__);
			return -1;
		}
		read_data = (char *)0x430000000;
		if (memcmp((void *)buffer, (void *)read_data, (blkcnt_lb * blksz))) {
			if (blkcnt_lb < 0x20) {
				dump_buffer((u64) buffer, (u64) (blkcnt_lb * blksz));
				printf("\r\n[ERROR]blk_dread: start_lba 0x%lX, blkcnt_lb 0x%lX\n", start_lba, blkcnt_lb);
				dump_buffer((u64) read_data, (u64) (blkcnt_lb * blksz));
			} else {
				printf
				    ("\r\n[ERROR]blk_dread: start_lba 0x%lX, blkcnt_lb 0x%lX, buffer addr 0x%llX\n",
				     start_lba, blkcnt_lb, (u64) buffer);
			}
			return -1;
		}
	} else {
		printf("\r\n[ERROR]read back check request size 0x%lX over limit\n", blkcnt_lb * blksz);
	}
	return 0;
}
#endif

int emmc_write(char *part_name, u64 addr, u64 len, char *buffer)
{
	lbaint_t start_lba, blkcnt_lb;
	lbaint_t wr_blkcnt_lb, grp_wr_blkcnt_lb;
	struct blk_desc *blk_dev_desc = NULL;
	ulong blksz;
	ulong last_len;
	ulong grp_blks, unsaved_blks;
	char *temp_data = NULL;
	char *temp_buffer = NULL;
	int ret;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		pr_err("get mmc dev fail\n");
		return -1;
	}
	blksz = blk_dev_desc->blksz;
	if (blksz != 512) {
		printf("%s: blksz 0x%lX is error\n", __FUNCTION__, blksz);
		return -1;
	}
	if (addr % blksz) {
		printf("%s: addr 0x%llX not align\n", __FUNCTION__, addr);
		return -1;
	}
	start_lba = (addr / blksz) & 0xffffffff;
	blkcnt_lb = (len / blksz) & 0xffffffff;
	if (len % blksz) {
		last_len = len - blkcnt_lb * blksz;
	} else {
		last_len = 0;
	}
	wr_blkcnt_lb = 0;
	/*
	 * spl and eip bin have backup and store in boot_parttion1
	 * uboot store in boot_partition2
	 * others store in user_partition with gpt table
	 */

	if (!strcmp(part_name, "spl")) {
		ret = is_emmc_switch_boot_part1(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
	} else {
		ret = emmc_switch_to_uda_part(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
	}

#if CONFIG_IS_ENABLED(BLK)
	if ((u64) buffer % 0x1000 == 0) {
		wr_blkcnt_lb = blk_dwrite(blk_dev_desc, start_lba, blkcnt_lb, (void *)buffer);
		if (wr_blkcnt_lb != blkcnt_lb) {
			printf("%s: some error happend while writing disk\n", __FUNCTION__);
			return -1;
		}
	} else {
		temp_data = (char *)SDHCI_ALIGN_BUF_ADDR;
		unsaved_blks = blkcnt_lb;
		temp_buffer = buffer;
		while (unsaved_blks != 0) {
			grp_blks = ((unsaved_blks > SDHCI_ALIGN_BUF_BLKS) ? SDHCI_ALIGN_BUF_BLKS : unsaved_blks);
			memcpy((void *)temp_data, (void *)temp_buffer, (grp_blks * blksz));
			grp_wr_blkcnt_lb = blk_dwrite(blk_dev_desc, (start_lba + wr_blkcnt_lb), grp_blks, (void *)temp_data);
			if (grp_wr_blkcnt_lb != grp_blks) {
				printf("%s: some error happend while writing disk\n", __FUNCTION__);
				return -1;
			}
			unsaved_blks -= grp_blks;
			wr_blkcnt_lb += grp_blks;
			temp_buffer += grp_blks * blksz;
		}
	}
#ifdef DEBUG_SPARSE_DL_DATA_CHECK
	if (emmc_read_back_check(blk_dev_desc, start_lba, blkcnt_lb, (void *)buffer)) {
		printf("[ERROR]%s: emmc_read_back_check fail\n", __FUNCTION__);
		return -1;
	}
#endif
	if (last_len) {
		printf("%s: last %lX bytes\n", __FUNCTION__, last_len);
		temp_data = (char *)SDHCI_ALIGN_BUF_ADDR;	//malloc(blksz);
		/*if (temp_data == NULL) {
		   printf("%s: malloc %lX bytes fail\n", __FUNCTION__, blksz);
		   return -1;
		   } */

		if (blk_dread(blk_dev_desc, (start_lba + blkcnt_lb), 1, (void *)temp_data) != 1) {
			printf("%s: some error happend while reading disk\n", __FUNCTION__);
			return -1;
		}
		memcpy((void *)temp_data, (void *)(buffer + blkcnt_lb * blksz), last_len);

		if (blk_dwrite(blk_dev_desc, (start_lba + blkcnt_lb), 1, (void *)temp_data) != 1) {
			printf("%s: some error happend while writing disk\n", __FUNCTION__);
			return -1;
		}
	}
#else
	if ((u64) buffer % 0x1000 == 0) {
		wr_blkcnt_lb = blk_dev_desc->block_write(blk_dev_desc, start_lba, blkcnt_lb, (void *)buffer);
		if (wr_blkcnt_lb != blkcnt_lb) {
			printf("%s: some error happend while writing disk\n", __FUNCTION__);
			return -1;
		}
	} else {
		temp_data = (char *)SDHCI_ALIGN_BUF_ADDR;
		unsaved_blks = blkcnt_lb;
		while (unsaved_blks != 0) {
			grp_blks = ((unsaved_blks > SDHCI_ALIGN_BUF_BLKS) ? SDHCI_ALIGN_BUF_BLKS : unsaved_blks);
			memcpy((void *)temp_data, (void *)buffer, (grp_blks * blksz));
			grp_wr_blkcnt_lb = blk_dev_desc->block_write(blk_dev_desc, (start_lba + wr_blkcnt_lb), grp_blks, (void *)temp_data);
			if (grp_wr_blkcnt_lb != grp_blks) {
				printf("%s: some error happend while writing disk\n", __FUNCTION__);
				return -1;
			}
			unsaved_blks -= grp_blks;
			wr_blkcnt_lb += grp_blks;
			buffer += grp_blks * blksz;
		}
	}

	if (last_len) {
		temp_data = (char *)SDHCI_ALIGN_BUF_ADDR;	//malloc(blksz);
		/*if (temp_data == NULL) {
		   printf("%s: malloc 0x%lX bytes fail\n", __FUNCTION__, blksz);
		   return -1;
		   } */

		if (blk_dev_desc->block_read(blk_dev_desc, (start_lba + blkcnt_lb), 1, (void *)temp_data) != 1) {
			printf("%s: some error happend while reading disk\n", __FUNCTION__);
			return -1;
		}
		memcpy((void *)temp_data, (void *)(buffer + blkcnt_lb * blksz), last_len);

		if (blk_dev_desc->block_write(blk_dev_desc, (start_lba + blkcnt_lb), 1, (void *)temp_data) != 1) {
			printf("%s: some error happend while writing disk\n", __FUNCTION__);
			return -1;
		}
	}
#endif

	return (wr_blkcnt_lb * blksz + last_len);
}

int emmc_erase(char *part_name, u64 addr, u64 len)
{
	lbaint_t start_lba, blkcnt_lb;
	lbaint_t erase_blkcnt_lb;
	struct blk_desc *blk_dev_desc = NULL;
	ulong blksz;
	int ret;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		pr_err("get mmc dev fail\n");
		return -1;
	}
	blksz = blk_dev_desc->blksz;
	if (blksz != 512) {
		printf("%s: blksz 0x%lX is error\n", __FUNCTION__, blksz);
		return -1;
	}
	if (addr % blksz) {
		printf("%s: addr 0x%llX not align\n", __FUNCTION__, addr);
		return -1;
	}
	if (len % blksz) {
		printf("%s: len 0x%llX not align\n", __FUNCTION__, len);
		return -1;
	}
	start_lba = addr / blksz;
	blkcnt_lb = len / blksz;
	printf("[%s]addr 0x%llX, len 0x%llX, start lba 0x%lX, blkcnt 0x%lX\n", __FUNCTION__, addr, len, start_lba, blkcnt_lb);

	if (!strcmp(part_name, "spl")) {
		ret = is_emmc_switch_boot_part1(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
	} else {
		ret = emmc_switch_to_uda_part(blk_dev_desc);
		if (ret != 0) {
			return -1;
		}
	}
	/*
	 * all image store in user_partition with bootargs
	 */
	if (!strcmp(part_name, "eraseall")) {
		ret = blk_dselect_hwpart(blk_dev_desc, 0);
		if (ret) {
			printf("%s: Failed to select user area\n", __FUNCTION__);
			return -1;
		}
		start_lba = 0;
		blkcnt_lb = blk_dev_desc->lba;
		printf("%s: start eraseall, start lba 0x%lX, lbacnt 0x%lX\n", __FUNCTION__, start_lba, blkcnt_lb);
	}
#if CONFIG_IS_ENABLED(BLK)
	erase_blkcnt_lb = blk_derase(blk_dev_desc, start_lba, blkcnt_lb);
	if (erase_blkcnt_lb != blkcnt_lb) {
		printf("%s: some error happend while erasing disk\n", __FUNCTION__);
		return -1;
	}
#else
	erase_blkcnt_lb = blk_dev_desc->block_erase(blk_dev_desc, start_lba, blkcnt_lb);
	if (erase_blkcnt_lb != blkcnt_lb) {
		printf("%s: some error happend while erasing disk\n", __FUNCTION__);
		return -1;
	}
#endif

	if (!strcmp(part_name, "eraseall") || !strcmp(part_name, "env")) {
		env_load();
		env_save();
	}

	return 0;
}
#endif

#ifdef CONFIG_MTD_SPI_NAND
int spi_nand_protect_disable(void)
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

static int spi_nand_write(char *part_name, u64 addr, u64 len, char *buffer)
{

	/* 1. mtd spi nand device init */
	u32 ret, dl_buf_off;
	u32 busnum = 0;
	u64 remaining;
	u64 off = 0;
	int idx;
	loff_t maxsize;
	size_t retlen, write_size;
	static int mtdpart_skip_blk_cnt = 0;
	static loff_t size = 0;
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
	if (strcmp(lastXferPart, part_name)) {
		erase_op.mtd = mtd;
		erase_op.len = mtd->erasesize;
		erase_op.scrub = 0;

		ret = mtd_arg_off(part_name, &idx, &partOff, &size, &maxsize, MTD_DEV_TYPE_NAND, mtd->size);
		if (ret)
			return ret;
		printf("%s: partiton %s off = 0x%llx size = 0x%llx\n", __func__, part_name, partOff, size);

		remaining = round_up(size, mtd->erasesize);
		erase_op.addr = partOff;

		while (remaining) {
			if (erase_op.addr + mtd->erasesize > partOff + size) {
				pr_err
				    ("%s: erase addr 0x%llX len 0x%X over %s part addr 0x%llX size 0x%llX\n",
				     __func__, erase_op.addr, mtd->erasesize, part_name, partOff, size);
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
		printf("partiton %s erased @off = 0x%llx size = 0x%llx\n", part_name, partOff, size);
	}

	/* 3. Loop over to do the actual read/write */
	if (strcmp(lastXferPart, part_name)) {
		xferPartCnt = 0;
		mtdpart_skip_blk_cnt = 0;
		printf("partiton %s write start, mtdpart_skip_blk_cnt=%d\n", part_name, mtdpart_skip_blk_cnt);
	}

	off = partOff + xferPartCnt * dl_buf_size + mtdpart_skip_blk_cnt * mtd->erasesize;	/* FDL_BUF_LEN = 2M */
	if (off % mtd->erasesize) {
		pr_err("nand addr 0x%llX is not block size 0x%X aligned!\n", off, mtd->erasesize);
		return -1;
	}
	xferPartCnt++;

	remaining = len;
	dl_buf_off = 0;
	printf("buf: unsave_recv_len=0x%llX, dl_buf_size=0x%lX\n", len, dl_buf_size);
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
			    ("%s: write addr 0x%llX len 0x%lX over %s part addr 0x%llX size 0x%llX\n",
			     __func__, off, write_size, part_name, partOff, size);
			return -1;
		}
		if (dl_buf_off + write_size > len) {
			pr_err("%s: dl_buf_off 0x%X len 0x%lX over unsave_recv_len 0x%llX\n", __func__, dl_buf_off, write_size, len);
			return -1;
		}

		mtd_write(mtd, off, write_size, &retlen, (u_char *) (buffer + dl_buf_off));
		if (retlen != write_size) {
			pr_err("write partition fail!.\n");
			return -1;
		}
		printf("partiton %s wrote 0x%lX bytes, dl_buf_off 0x%X ==> nand addr 0x%llX\n", part_name, retlen, dl_buf_off, off);
		remaining -= write_size;
		dl_buf_off += write_size;
		off += write_size;
	}

	strcpy(lastXferPart, part_name);
	return dl_buf_off;

}

static int spi_nand_erase(char *part_name, u64 addr, u64 len)
{
	int ret = -1;
	int idx;
	u32 busnum = 0;
	u64 remaining;
	loff_t off = 0;
	loff_t size, maxsize;
	struct udevice *dev = NULL;
	struct mtd_info *mtd = NULL;
	struct erase_info erase_op = { };

	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}

	mtd = dev_get_uclass_priv(dev);

	erase_op.mtd = mtd;
	erase_op.len = mtd->erasesize;
	erase_op.scrub = 0;

	if (!strcmp(part_name, "eraseall")) {
		printf("nand eraseall\n");
		erase_op.addr = 0;
		remaining = mtd->size;
	} else {
		printf("nand erase %s part\n", part_name);
		ret = mtd_arg_off(part_name, &idx, &off, &size, &maxsize, MTD_DEV_TYPE_NAND, mtd->size);
		if (ret)
			return ret;

		if ((addr < off) || (addr + len > off + size)) {
			printf("erase region [0x%llX, 0x%llX] over part region [0x%llX, 0x%llX]\n", addr, (addr + len), off, (off + size));
			return -1;
		}
		remaining = round_up(len, mtd->erasesize);
		erase_op.addr = addr;
	}

	while (remaining) {
		printf("erase phy addr 0x%llX, size 0x%llX\n", erase_op.addr, erase_op.len);
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

	if (!strcmp(part_name, "eraseall") || !strcmp(part_name, "env")) {
		env_load();
		env_save();
	}

	return ret;
}

int fdl_read_from_spinand(char *part_name, u64 addr, u64 len, char *buffer)
{
	/* 1. mtd spi nand device init */
	int ret;
	u32 load_buf_off;
	u32 busnum = 0;
	loff_t off = 0;
	int idx;
	loff_t maxsize;
	size_t retlen, read_size, remaining;
	static loff_t part_off = 0;
	static loff_t part_size = 0;
	static int mtdpart_skip_blk_cnt = 0;
	struct udevice *dev;
	struct mtd_info *mtd;

	printf("%s: %s, addr 0x%llX, len 0x%llX\n", __func__, part_name, addr, len);
	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}
	mtd = dev_get_uclass_priv(dev);

	/* 2. Loop over to do the actual read/write */
	if (strcmp(lastLoadPart, part_name)) {
		if (addr % mtd->erasesize) {
			pr_err("part addr 0x%llX is not block size 0x%X aligned!\n", addr, mtd->erasesize);
			return -1;
		}
		ret = mtd_arg_off(part_name, &idx, &part_off, &part_size, &maxsize, MTD_DEV_TYPE_NAND, mtd->size);
		if (ret) {
			printf("%s, %d, ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}
		printf("%s: partiton %s off = 0x%llX size = 0x%llX\n", __func__, part_name, part_off, part_size);
		mtdpart_skip_blk_cnt = 0;
	}

	load_buf_off = 0;
	remaining = len;
	if (addr % mtd->erasesize) {
		read_size = ((addr + mtd->erasesize - 1) & ~(mtd->erasesize - 1)) - addr;
		read_size = (remaining > read_size) ? read_size : remaining;
		off = addr + mtdpart_skip_blk_cnt * mtd->erasesize;
		printf("%s: read phy_addr=0x%llX, last len=0x%lX\n", __func__, off, read_size);
		if (off + read_size > part_off + part_size) {
			pr_err
			    ("%s: read addr 0x%llX len 0x%lX over part %s addr 0x%llX size 0x%llX\n",
			     __func__, off, read_size, part_name, part_off, part_size);
			return -1;
		}
		if (load_buf_off + read_size > dl_buf_size) {
			pr_err("%s: load_buf_off 0x%X len 0x%lX over dl_buf_size 0x%lX\n", __func__, load_buf_off, read_size, dl_buf_size);
			return -1;
		}

		mtd_read(mtd, off, read_size, &retlen, (u_char *) (buffer + load_buf_off));
		if (retlen != read_size) {
			pr_err("%s image read fail!\n", part_name);
			return -1;
		}
		printf("partiton %s read 0x%lX bytes, nand addr 0x%llX ==> buf addr 0x%llX\n", part_name, retlen, off, (u64) (buffer + load_buf_off));
		remaining -= read_size;
		load_buf_off += read_size;
		off += read_size;
	} else {
		off = (loff_t) (addr + mtdpart_skip_blk_cnt * mtd->erasesize);	/* FDL_BUF_LEN = 2M */
	}

	printf("%s: read phy_addr=0x%llX, remaining=0x%lX\n", __func__, off, remaining);
	while (remaining) {
		if (mtd_block_isbad(mtd, off)) {
			mtdpart_skip_blk_cnt++;
			printf("nand addr 0x%llX, blk %lld is bad, mtdpart_skip_blk_cnt=%d\n", off, (u64) off / mtd->erasesize, mtdpart_skip_blk_cnt);
			off += mtd->erasesize;
			continue;
		}

		read_size = (remaining > mtd->erasesize) ? mtd->erasesize : remaining;
		if (off + read_size > part_off + part_size) {
			pr_err
			    ("%s: read addr 0x%llX len 0x%lX over part %s addr 0x%llX size 0x%llX\n",
			     __func__, off, read_size, part_name, part_off, part_size);
			return -1;
		}
		if (load_buf_off + read_size > dl_buf_size) {
			pr_err("%s: load_buf_off 0x%X len 0x%lX over dl_buf_size 0x%lX\n", __func__, load_buf_off, read_size, dl_buf_size);
			return -1;
		}

		mtd_read(mtd, off, read_size, &retlen, (u_char *) (buffer + load_buf_off));
		if (retlen != read_size) {
			pr_err("%s image read fail!\n", part_name);
			return -1;
		}
		printf("partiton %s read 0x%lX bytes, nand addr 0x%llX ==> buf addr 0x%llX\n", part_name, retlen, off, (u64) (buffer + load_buf_off));

		remaining -= read_size;
		load_buf_off += read_size;
		off += read_size;
	}

	strcpy(lastLoadPart, part_name);
	return load_buf_off;
}
#endif

#ifdef CONFIG_SPI_FLASH
int fdl_read_from_spinor(char *part_name, u64 addr, u64 len, char *buffer)
{
	/* 1. mtd spi nor device init */
	int ret;
	u32 load_buf_off;
	u32 busnum = 0;
	loff_t off = 0;
	int idx;
	loff_t maxsize;
	size_t retlen, read_size, remaining;
	static loff_t part_off = 0;
	static loff_t part_size = 0;
	static int mtdpart_skip_blk_cnt = 0;
	struct udevice *dev;
	struct mtd_info *mtd;
	struct spi_flash *flash;

	printf("%s: %s, addr 0x%llX, len 0x%llX\n", __func__, part_name, addr, len);
	ret = uclass_get_device(UCLASS_SPI_FLASH, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}
	mtd = get_mtd_device_nm("nor0");
	flash = dev_get_uclass_priv(dev);

	/* 2. Loop over to do the actual read/write */
	if (strcmp(lastLoadPart, part_name)) {
		if (addr % mtd->erasesize) {
			pr_err("part addr 0x%llX is not block size 0x%X aligned!\n", addr, mtd->erasesize);
			return -1;
		}
		ret = mtd_arg_off(part_name, &idx, &part_off, &part_size, &maxsize, MTD_DEV_TYPE_NOR, flash->size);
		if (ret) {
			printf("%s, %d, ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}
		printf("%s: partiton %s off = 0x%llX size = 0x%llX\n", __func__, part_name, part_off, part_size);
		mtdpart_skip_blk_cnt = 0;
	}

	load_buf_off = 0;
	remaining = len;
	if (addr % mtd->erasesize) {
		read_size = ((addr + mtd->erasesize - 1) & ~(mtd->erasesize - 1)) - addr;
		read_size = (remaining > read_size) ? read_size : remaining;
		off = addr + mtdpart_skip_blk_cnt * mtd->erasesize;
		printf("%s: read phy_addr=0x%llX, last len=0x%lX\n", __func__, off, read_size);
		if (off + read_size > part_off + part_size) {
			pr_err
			    ("%s: read addr 0x%llX len 0x%lX over part %s addr 0x%llX size 0x%llX\n",
			     __func__, off, read_size, part_name, part_off, part_size);
			return -1;
		}
		if (load_buf_off + read_size > dl_buf_size) {
			pr_err("%s: load_buf_off 0x%X len 0x%lX over dl_buf_size 0x%lX\n", __func__, load_buf_off, read_size, dl_buf_size);
			return -1;
		}

		mtd_read(mtd, off, read_size, &retlen, (u_char *) (buffer + load_buf_off));
		if (retlen != read_size) {
			pr_err("%s image read fail!\n", part_name);
			return -1;
		}
		printf("partiton %s read 0x%lX bytes, nor addr 0x%llX ==> buf addr 0x%llX\n", part_name, retlen, off, (u64) (buffer + load_buf_off));
		remaining -= read_size;
		load_buf_off += read_size;
		off += read_size;
	} else {
		off = (loff_t) (addr + mtdpart_skip_blk_cnt * mtd->erasesize);	/* FDL_BUF_LEN = 2M */
	}

	printf("%s: read phy_addr=0x%llX, remaining=0x%lX\n", __func__, off, remaining);
	while (remaining) {
		if (mtd_block_isbad(mtd, off)) {
			mtdpart_skip_blk_cnt++;
			printf("nor addr 0x%llX, blk %lld is bad, mtdpart_skip_blk_cnt=%d\n", off, (u64) off / mtd->erasesize, mtdpart_skip_blk_cnt);
			off += mtd->erasesize;
			continue;
		}

		read_size = (remaining > mtd->erasesize) ? mtd->erasesize : remaining;
		if (off + read_size > part_off + part_size) {
			pr_err
			    ("%s: read addr 0x%llX len 0x%lX over part %s addr 0x%llX size 0x%llX\n",
			     __func__, off, read_size, part_name, part_off, part_size);
			return -1;
		}
		if (load_buf_off + read_size > dl_buf_size) {
			pr_err("%s: load_buf_off 0x%X len 0x%lX over dl_buf_size 0x%lX\n", __func__, load_buf_off, read_size, dl_buf_size);
			return -1;
		}

		mtd_read(mtd, off, read_size, &retlen, (u_char *) (buffer + load_buf_off));
		if (retlen != read_size) {
			pr_err("%s image read fail!\n", part_name);
			return -1;
		}
		printf("partiton %s read 0x%lX bytes, nand addr 0x%llX ==> buf addr 0x%llX\n", part_name, retlen, off, (u64) (buffer + load_buf_off));

		remaining -= read_size;
		load_buf_off += read_size;
		off += read_size;
	}

	strcpy(lastLoadPart, part_name);
	return load_buf_off;
}

static int spi_norflash_write(char *part_name, u64 addr, u64 len, char *buffer)
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
	if (strcmp(lastXferPart, part_name)) {
		erase_op.mtd = mtd;
		erase_op.len = mtd->erasesize;
		erase_op.scrub = 0;

		ret = mtd_arg_off(part_name, &idx, &partOff, &size, &maxsize, MTD_DEV_TYPE_NOR, flash->size);
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
		printf("partiton %s erased @off = 0x%llx size = 0x%llx\n", part_name, partOff, size);
	}

	/* 3. Loop over to do the actual read/write */
	if (strcmp(lastXferPart, part_name))
		xferPartCnt = 0;

	off = partOff + xferPartCnt * dl_buf_size;
	xferPartCnt++;

	mtd_write(mtd, off, len, &retlen, (u_char *)buffer);
	if (retlen != len) {
		printf("write partition fail!.\n");
		return -1;
	}
	printf("partiton %s write @off = 0x%llx size = 0x%llx\n", part_name, off, len);

	strcpy(lastXferPart, part_name);
	return len;

}

static int spi_nor_erase(char *part_name, u64 addr, u64 len)
{
	int ret = -1;
	int idx;
	u32 busnum = 0;
	u64 remaining;
	loff_t off = 0;
	loff_t size, maxsize;
	struct udevice *dev = NULL;
	struct mtd_info *mtd = NULL;
	struct erase_info erase_op = { };
	struct spi_flash *flash;

	ret = uclass_get_device(UCLASS_SPI_FLASH, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}

	mtd = get_mtd_device_nm("nor0");
	flash = dev_get_uclass_priv(dev);

	erase_op.mtd = mtd;
	erase_op.len = mtd->erasesize;
	erase_op.scrub = 0;

	if (!strcmp(part_name, "eraseall")) {
		printf("nor eraseall\n");
		erase_op.addr = 0;
		remaining = flash->size;
		printf("erase size: 0x%llx\n", remaining);
	} else {
		printf("nor erase %s part\n", part_name);
		ret = mtd_arg_off(part_name, &idx, &off, &size, &maxsize, MTD_DEV_TYPE_NOR, flash->size);
		if (ret)
			return ret;

		if ((addr < off) || (addr + len > off + size)) {
			printf("erase region [0x%llX, 0x%llX] over part region [0x%llX, 0x%llX]\n", addr, (addr + len), off, (off + size));
			return -1;
		}
		remaining = round_up(len, mtd->erasesize);
		erase_op.addr = addr;
		printf("erase size: 0x%llx\n", remaining);
	}

	while (remaining) {
		printf("erase phy addr 0x%llX, size 0x%llX\n", erase_op.addr, erase_op.len);
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

	if (!strcmp(part_name, "eraseall") || !strcmp(part_name, "env")) {
		env_load();
		env_save();
	}

	return ret;
}
#endif

int common_raw_read(char *part_name, u64 addr, u64 len, char *data)
{
	u64 read_len = 0;

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
#ifdef CONFIG_SUPPORT_EMMC_BOOT
		read_len = fdl_read_from_emmc(part_name, addr, len, data);
#endif
		break;
	case STORAGE_TYPE_NAND:
#ifdef CONFIG_MTD_SPI_NAND
		read_len = fdl_read_from_spinand(part_name, addr, len, data);
#endif
		break;
	case STORAGE_TYPE_NOR:
#ifdef CONFIG_SPI_FLASH
		env_set("mtdids", MTDIDS_SPINOR);
		env_set("mtdparts", MTDPARTS_SPINOR);
		read_len = fdl_read_from_spinor(part_name, addr, len, data);
#endif
		break;
	default:
		break;
	}

	return read_len;
}

int common_raw_write(char *part_name, u64 addr, u64 len, char *data)
{
	u64 write_len = 0;
	int ret;

	ret = update_verify_image(part_name, data);
	if (ret)
		return -1;

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
#ifdef CONFIG_SUPPORT_EMMC_BOOT
		write_len = emmc_write(part_name, addr, len, data);
#endif
		break;
	case STORAGE_TYPE_NAND:
#ifdef CONFIG_MTD_SPI_NAND
		spi_nand_protect_disable();
		write_len = spi_nand_write(part_name, addr, len, data);
#endif
		break;
	case STORAGE_TYPE_NOR:
#ifdef CONFIG_SPI_FLASH
		write_len = spi_norflash_write(part_name, addr, len, data);
#endif
		break;
	default:
		break;
	}

	return write_len;
}

int common_raw_erase(char *part_name, u64 addr, u64 len)
{
	int ret = -1;

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
#ifdef CONFIG_SUPPORT_EMMC_BOOT
		ret = emmc_erase(part_name, addr, len);
#endif
		break;
	case STORAGE_TYPE_NAND:
#ifdef CONFIG_MTD_SPI_NAND
		spi_nand_protect_disable();
		ret = spi_nand_erase(part_name, addr, len);
#endif
		break;
	case STORAGE_TYPE_NOR:
#ifdef CONFIG_SPI_FLASH
		env_set("mtdids", MTDIDS_SPINOR);
		env_set("mtdparts", MTDPARTS_SPINOR);
		ret = spi_nor_erase(part_name, addr, len);
#endif
		break;
	default:
		break;
	}

	return ret;
}

#ifdef SPI_DUAL_CS
void ddr_set(ulong addr, ulong size)
{
	ulong i;

	for (i = 0; i < size; i += 4) {
		*((unsigned int *)(addr+i)) = i;
	}
}

int ddr_check(ulong addr, ulong size)
{
	ulong i;

	for (i = 0; i < size; i += 4) {
		if (i != *((unsigned int *)(addr+i))) {
			printf("ddr check error\n");
			return -1;
		}
	}
	printf("ddr check pass\n");
	return 0;
}
#endif

static char *extract_val_space(const char *str, const char *key)
{
	char *v, *k;
	char *s, *strcopy;
	char *new = NULL;

	strcopy = strdup(str);
	if (strcopy == NULL)
		return NULL;

	s = strcopy;
	while (s) {
		v = strsep(&s, " ");
		if (!v)
			break;
		k = strsep(&v, "=");
		if (!k)
			break;

		while (*k != 0) {
			if  (strcmp(k, key) == 0) {
				new = strdup(v);
				goto free_ret;
			}
			k++;
		}
	}

free_ret:
	free(strcopy);
	return new;
}

extern struct boot_mode_info boot_info_data;
u64 get_capacity_user(void)
{
	if(boot_info_data.storage_sel == STORAGE_TYPE_EMMC) {
		#if CONFIG_IS_ENABLED(BLK)
		struct mmc_uclass_priv *upriv = NULL;
		struct blk_desc *blk_dev_desc = NULL;
		struct mmc *mmc = NULL;

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
		printf("mmc capacity_user is 0x%llx\n",mmc->capacity_user);
		return mmc->capacity_user;
		#endif
	}
#ifdef CONFIG_MTD_SPI_NAND
	if(boot_info_data.storage_sel == STORAGE_TYPE_NAND) {
		int ret = -1;
		u32 busnum = 0;
		struct udevice *dev = NULL;
		struct mtd_info *mtd = NULL;

		ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
		if (ret) {
			printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
			return -1;
		}

		mtd = dev_get_uclass_priv(dev);
		printf("nand mtd->size is 0x%llx\n",mtd->size);
		return mtd->size;
	}
#endif
#ifdef CONFIG_SPI_FLASH
	if(boot_info_data.storage_sel == STORAGE_TYPE_NOR) {
		int ret = -1;
		u32 busnum = 0;
		struct udevice *dev = NULL;
		struct spi_flash *flash;

		ret = uclass_get_device(UCLASS_SPI_FLASH, busnum, &dev);
		if (ret) {
			printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
			return -1;
		}

		flash = dev_get_uclass_priv(dev);
		printf("nor flash->size is 0x%x\n", flash->size);
		return flash->size;
	}
#endif
	return 0;
}

int get_part_info_rawdata(struct update_part_info **part_list, char * src, int len)
{
	int ret = -1;
	int count = 0;
	int step = 0;
	int autoresize_flag = 0;
	uint part_size_unit;
	struct update_part_info *pheader = NULL;
	struct update_part_info *curr = NULL;
	struct update_part_info *pbin_info;
	char *tok, *val, *str, *p, *tmpstr;
	u64 capacity_user;
	u64 use_part_size = 0;
	str = src;
	capacity_user = get_capacity_user();
	do {
		if ((0 == step) && !memcmp(str, INFO_PART_START_STR, strlen(INFO_PART_START_STR))) {
			tok = strsep(&str, ">");

			val = extract_val_space(tok, PART_DATA_UNIT_STR);
			if (!val) {
				return -1;
			}
			printf("partinfo start unit val %s\n", val);
			if (strlen(val) <= 2) {
				free(val);
				return -1;
			}
			part_size_unit = *(val+1);
			if ('0' == part_size_unit) {
				part_size_unit = 1024 * 1024;
			}
			else if ('1' == part_size_unit) {
				part_size_unit = 1024 * 512;
			}
			else if ('2' == part_size_unit) {
				part_size_unit = 1024;
			}
			else {
				printf("part size unit not support\n");
				free(val);
				return -1;
			}
			free(val);
			step += 1;
		}

		if (1 == step) {
			if (!memcmp(str, INFO_PART_END_STR, strlen(INFO_PART_END_STR))) {
				if (curr)
					curr->next = NULL;
				step += 1;
				break;
			}

			if (!memcmp(str, INFO_PART_START_STR, strlen(INFO_PART_START_STR))) {
				if (autoresize_flag) {
					printf("[%s]err: auto resize part must be last part\n", __FUNCTION__);
					if (curr)
						curr->next = NULL;
					ret = -1;
					goto free_pbin_node;
				}
				tok = strsep(&str, ">");

				val = extract_val_space(tok, PART_DATA_ID_STR);
				if (!val) {
					ret = -1;
					goto free_pbin_node;
				}
				if (strlen(val) <= 2) {
					free(val);
					ret = -1;
					goto free_pbin_node;
				}
				pbin_info = malloc(sizeof(struct update_part_info));
				if (!pbin_info) {
					free(val);
					ret = -1;
					goto free_pbin_node;
				}
				memset(pbin_info, 0, sizeof(struct update_part_info));
				strncpy(pbin_info->part_name, (val+1), (strlen(val)-2));
				free(val);

				tmpstr = extract_val_space(tok, PART_DATA_SIZE_STR);
				if (!tmpstr) {
					ret = -1;
					goto free_pbin_node;
				}
				if (strlen(tmpstr) <= 2) {
					free(tmpstr);
					ret = -1;
					goto free_pbin_node;
				}
				val = malloc(strlen(tmpstr));
				if (!val) {
					free(tmpstr);
					ret = -1;
					goto free_pbin_node;
				}
				strcpy(val, tmpstr+1);
				free(tmpstr);
				p = strchr(val, '\"');
				if (p)
					*p = '\0'; // ignore last "
				p = val;
				if (!strcmp(val, "0xFFFFFFFF") || !strcmp(val, "0xffffffff") || !strcmp(val, "0XFFFFFFFF") || !strcmp(val, "0Xffffffff")) {
					if(capacity_user > 0 && capacity_user != -1)
						pbin_info->part_size = capacity_user - use_part_size;
					else
						printf("mmc capacity error, please check!\n");
					autoresize_flag = 1;
				}
				else {
					pbin_info->part_size = ustrtoull(p, &p, 0);
					pbin_info->part_size *= part_size_unit;
				}
				free(val);

				if (!pheader) {
					pheader = pbin_info;
					curr = pbin_info;
				} else {
					curr->next = pbin_info;
					curr = pbin_info;
				}
				count += 1;
				use_part_size += pbin_info->part_size;
				printf("curr :%d part %s, size %llX, use_part_size 0x%llX\n", count, curr->part_name, curr->part_size, use_part_size);
			}
		}
	} while (++str < src + len);

	if (step < 2) {
		printf("%s: part info error, end at step %d\n", __FUNCTION__, step);
		ret = -1;
		goto free_pbin_node;
	}

	ret = count;
	*part_list = pheader;
	printf("successfully parsed part info\n");
	return ret;

free_pbin_node:
	while (pheader) {
		pbin_info = pheader;
		pheader = pheader->next;
		free(pbin_info);
	}

	return ret;
}

int update_parts_info(struct update_part_info *pheader)
{
	int i, part_size_KB, part_size_MB;
	int part_count = 0;
	int len = 0;
	struct update_part_info *pcur = pheader;
	char buf[512] = {0};
	char *mmc_parts = NULL;

	while (pcur) {
		part_count++;
		pcur = pcur->next;
	}
	if (0 == part_count) {
		printf("%s: no valid part\n", __func__);
		return -1;
	}
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
			printf("%s: %d, storage_sel %d error\n", __FUNCTION__, __LINE__, boot_info_data.storage_sel);
			break;
	}
	pcur = pheader;
	for (i = 0; i < part_count; i++) {
		if (0 != (pcur->part_size % 1024)) {
			printf("%s: part size 0x%llX must be 1KB aligned.\n", pcur->part_name, pcur->part_size);
			return -1;
		}

		if (0 == (pcur->part_size % (1024 * 1024))) {
			part_size_MB = pcur->part_size / (1024 * 1024);
			printf("%s: %d MB\n", pcur->part_name, part_size_MB);
			len += snprintf(buf + len, PAGE_SIZE-len, "%dM(%s),", part_size_MB, pcur->part_name);
		}
		else {
			part_size_KB = pcur->part_size / 1024;
			printf("%s: %d KB.\n", pcur->part_name, part_size_KB);
			len += snprintf(buf + len, PAGE_SIZE-len, "%dK(%s),", part_size_KB, pcur->part_name);
		}

		pcur = pcur->next;
	}

	buf[strlen(buf)-1] = '\0';

	printf("buf = %s\n",buf);
	env_set("bootargs",buf);
	env_save();
	return 0;
}

#ifdef VERSION_2_PARSE_XML
static int update_parse_xml(struct update_part_info **bin_info)
{
	int ret = 0;
	int is_spl_found = 0;
	int is_uboot_found = 0;
	loff_t read_len, size;
	struct update_part_info *pheader = NULL;
	struct update_part_info *pcurr = NULL;
	struct update_part_info *pbin_info;
	char *part_data = NULL;
	char file_name[MAX_FILE_NAME_LEN] = {0};

	if (!fat_exists(XML_NAME)) {
		printf("update config file is not exist, exit update\n");
		return -1;
	}

	printf("\nstart parse %s...\n", XML_NAME);
	if (!fat_size(XML_NAME, &size)) {
		part_data = (char *)malloc(size + 1);
		if (!part_data) {
			printf("update malloc part_data space fail, exit update\n");
			return -1;
		}
		memset(part_data, 0, size + 1);
	} else {
			printf("get xml file size fail, exit update\n");
			return -1;
	}

	ret = fat_read_file(XML_NAME, part_data, 0, size, &read_len);
	if (ret || size != read_len) {
		printf("read config file fail, exit update\n");
		goto free_data;
	}

	if (get_part_info_rawdata(&pheader, part_data, size) < 0) {
		printf("%s: part info error\n", __FUNCTION__);
		ret = -1;
		goto free_part_list;
	}

	pcurr = pheader;
	while (pcurr) {
		if (!strcmp(pcurr->part_name, "spl")) {
			is_spl_found = 1;
		}

		if (!strcmp(pcurr->part_name, "uboot")) {
			is_uboot_found = 1;
		}

		pcurr = pcurr->next;
	}

	if (0 == is_spl_found) {
		pbin_info = malloc(sizeof(struct update_part_info));
		if (!pbin_info) {
			ret = -1;
			goto free_part_list;
		}
		memset(pbin_info, 0, sizeof(struct update_part_info));
		strcpy(pbin_info->part_name, "spl");
		pbin_info->part_size = SPL_MAX_SIZE;
		pbin_info->next = pheader;
		pheader = pbin_info;
		pbin_info = NULL;
	}

	if (0 == is_uboot_found) {
		pbin_info = malloc(sizeof(struct update_part_info));
		if (!pbin_info) {
			ret = -1;
			goto free_part_list;
		}
		memset(pbin_info, 0, sizeof(struct update_part_info));
		strcpy(pbin_info->part_name, "uboot");
		pbin_info->part_size = UBOOT_MAX_SIZE;
		pbin_info->next = pheader;
		pheader = pbin_info;
		pbin_info = NULL;
	}

	pcurr = pheader;
	while (pcurr) {
		strcpy(file_name, pcurr->part_name);
		if (!strcmp(pcurr->part_name, "spl") || !strcmp(pcurr->part_name, "uboot") ||
				!strcmp(pcurr->part_name, "uboot_b")) {
			strcat(file_name, ".bin");
		}
		else {
			strcat(file_name, ".img");
		}

		if (fat_exists(file_name)) {
			strcpy(pcurr->file_name, file_name);
		}
		else {
			strcpy(pcurr->file_name, "none");
		}
		printf("part:%s, updating file:%s\n", pcurr->part_name, pcurr->file_name);
		memset(file_name, 0, sizeof(file_name));
		pcurr = pcurr->next;
	}

	*bin_info = pheader;
	printf("successfully parsed image file\n\n");

	goto free_data;

free_part_list:
	while (pheader) {
		pbin_info = pheader;
		pheader = pheader->next;
		free(pbin_info);
	}

free_data:
	free(part_data);

	return ret;
}

int update_parse_part_info(struct update_part_info **bin_info)
{
	int ret = 0;

	if (SD_UPDATE_MODE == boot_info_data.mode  ||
		boot_info_data.mode == USB_UPDATE_MODE ||
		boot_info_data.mode == NORMAL_BOOT_MODE
		) {
		ret = update_parse_xml(bin_info);
		printf("%s: fs loop ret %d\n", __FUNCTION__, ret);
	}

	return ret;
}

#ifdef CONFIG_SUPPORT_EMMC_BOOT
int set_emmc_boot_mode_after_dl(void)
{
	u8 width, reset, mode;
	u8 ack, part_num;
	boot_mode_info_t *boot_mode = (boot_mode_info_t *) BOOT_MODE_INFO_ADDR;
	printf("%s: storage_sel:%d , boot_type: %d\n", __FUNCTION__, boot_mode->storage_sel, boot_mode->boot_type);
	if(boot_mode->storage_sel == STORAGE_TYPE_EMMC) {
		#if CONFIG_IS_ENABLED(BLK)
		struct mmc_uclass_priv *upriv = NULL;
		struct blk_desc *blk_dev_desc = NULL;
		struct mmc *mmc = NULL;

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

		switch (boot_mode->boot_type) {
			case EMMC_BOOT_8BIT_50M_768K:
				width = 0x2; //0x2 : x8 (sdr/ddr) bus width in boot operation mode
				reset = 0x0; //Reset bus width to x1, single data rate and backward compatible timings after boot operation (default)
				mode = 0x1; //0x1 : Use single data rate + High Speed timings in boot operation mode
				ack = 0x1; //0x1 : Boot acknowledge sent during boot operation Bit
				part_num = 0x1; //0x1 : Boot partition 1 enabled for boot
				break;
			case EMMC_BOOT_4BIT_25M_768K:
			case EMMC_BOOT_4BIT_25M_128K:
				width = 0x1; //0x1 : x4 (sdr/ddr) bus width in boot operation mode
				reset = 0x0; //Reset bus width to x1, single data rate and backward compatible timings after boot operation (default)
				mode = 0x1; //0x1 : Use single data rate + High Speed timings in boot operation mode
				ack = 0x1; //0x1 : Boot acknowledge sent during boot operation Bit
				part_num = 0x1; //0x1 : Boot partition 1 enabled for boot
				break;
			default:
				printf("%s: %d, Not in emmc boot mode, do not need to set ext_csd regs\n", __FUNCTION__, __LINE__);
				return 0;
		}
		mmc_set_boot_bus_width(mmc, width, reset, mode);
		mmc_set_part_conf(mmc, ack, part_num, 0);
		mmc_set_rst_n_function(mmc, 1);
		#endif
	}
	return 0;
}
#endif

int set_reboot_mode_after_dl(void)
{
	printf("%s: storage_sel:%d , boot_type: %d\n", __FUNCTION__, boot_info_data.storage_sel, boot_info_data.boot_type);
	/* Turn on write protection */
	writel(1, TOP_CHIPMODE_GLB_SW_PORT_SET);
	writel(0xf, TOP_CHIPMODE_GLB_SW_CLR);
	switch (boot_info_data.boot_type) {
		case EMMC_BOOT_UDA:
			writel(0x1, TOP_CHIPMODE_GLB_SW_SET);
			break;
		case EMMC_BOOT_8BIT_50M_768K:
			writel(0x3, TOP_CHIPMODE_GLB_SW_SET);
			break;
		case EMMC_BOOT_4BIT_25M_768K:
			writel(0xD, TOP_CHIPMODE_GLB_SW_SET);
			break;
		case EMMC_BOOT_4BIT_25M_128K:
			writel(0x9, TOP_CHIPMODE_GLB_SW_SET);
			break;
		case NAND_2K:
			writel(0x7, TOP_CHIPMODE_GLB_SW_SET);
			break;
		case NAND_4K:
			writel(0xB, TOP_CHIPMODE_GLB_SW_SET);
			break;
		case NOR:
			writel(0xF, TOP_CHIPMODE_GLB_SW_SET);
			break;
		default:
			printf("%s: %d, BOOT_TYPE_UNKNOWN\n", __FUNCTION__, __LINE__);
			break;
	}
	/* Turn off write protection */
	writel(1, TOP_CHIPMODE_GLB_SW_PORT_CLR);
	return 0;
}

#ifdef CONFIG_SUPPORT_EMMC_BOOT
int is_emmc_switch_boot_part1(struct blk_desc *blk_dev_desc)
{
	int ret;
	switch (boot_info_data.boot_type) {
		case EMMC_BOOT_8BIT_50M_768K:
		case EMMC_BOOT_4BIT_25M_768K:
		case EMMC_BOOT_4BIT_25M_128K:
			ret = blk_dselect_hwpart(blk_dev_desc, PARTITION_BOOT1);
			if (ret) {
				pr_err("Failed to select h/w partition 1\n");
				return -1;
			}
			break;
		case EMMC_BOOT_UDA:
			/* printf("%s: %d, Do not need swtich boot partiton1\n", __FUNCTION__, __LINE__); */
			break;
		default:
			break;
	}
	return 0;
}
int emmc_switch_to_uda_part(struct blk_desc *blk_dev_desc)
{
	int ret;
	ret = blk_dselect_hwpart(blk_dev_desc, 0);
	if (ret) {
		pr_err("Failed to select h/w partition 1\n");
		return -1;
	}
	return 0;
}
#endif

#endif
