/*
 * AXERA AX620E Host Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <lzma/LzmaTools.h>
#include <common.h>
#include <blk.h>
#include <cpu_func.h>
#include <asm/arch/boot_mode.h>
#include <malloc.h>
#include <mtd.h>
#include <image.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <jffs2/load_kernel.h>
#include <linux/mtd/spi-nor.h>
#include "../../legacy-mtd-utils.h"
#include <common.h>
#include <asm/io.h>
#include <asm/arch/ax620e.h>
#include <mmc.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <linux/compat.h>
#include <asm/io.h>
#include "stdlib.h"
#include <mapmem.h>
#include <part.h>
#include <fat.h>
#include <fs.h>
#include <rtc.h>
#include <linux/time.h>
#include "../secureboot/secureboot.h"
#include "axera_boot.h"
#include <asm/arch/boot_mode.h>
#include <asm/mach-types.h>
#ifdef CONFIG_CMD_AXERA_GZIPD
#include "../gzipd/ax_gzipd_api.h"
#endif
#include "../cipher/ax_cipher_api.h"

DECLARE_GLOBAL_DATA_PTR;

static loff_t partOff = 0;
static loff_t partSize = 0;

extern struct boot_mode_info boot_info_data;
extern misc_info_t *misc_info;
extern int pinmux_init(void);
extern int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t *info);
extern void announce_and_cleanup(int fake);
extern void ax_shutdown_ephy(void);
extern int fdt_chosen(void *fdt);

#ifdef CONFIG_CMD_AXERA_KERNEL_LZMA

int lzma_decompress_image(void *src, void *dest, u32 size)
{
	int ret;
	char lzma_cmd[128];
	memset(lzma_cmd,0,sizeof(lzma_cmd));
	sprintf(lzma_cmd, "lzmadec  0x%lx  0x%lx  0x%x", (unsigned long)src, (unsigned long)dest, size);
	printf("decompress cmd %s\n",lzma_cmd);
	ret = run_command(lzma_cmd, 0);
	if(ret)
		printf("decompress failed\n");

	flush_dcache_all();
	return ret;
}

#endif

#define BOOTIMG_HEADER_SIZE     64
#define BOOT_MAGIC_SIZE         6

/* key_n_header + rsa_key_n + key_e_header + rsa_key_e */
#define PUB_KEY_ARRAY_MAX_SZ  (396)   //sizeof(struct rsa_key)

#ifdef CONFIG_CMD_AXERA_GZIPD
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define TILE_BASE_SIZE  (8 * 1024)

int gzip_decompress_image(void *src, void *dest, u32 size)
{
	int ret;
	gzipd_header_info_t header_info;
	u32 tile_cnt, last_tile_size;
	u32 run_num;
	u64 tile_size = 0;
	u64 total_size = 0;
	void *tiles_addr_start;
	void *tiles_addr_end;
	void *img_compressed_addr;

	total_size = size;
	tile_size = (DIV_ROUND_UP(total_size, TILE_BASE_SIZE) / 12 + 1) * TILE_BASE_SIZE;
	img_compressed_addr = src;

	gzipd_dev_init();

	ret = gzipd_dev_get_header_info(img_compressed_addr, &header_info);
	if (ret) {
		printf("get header info error\n");
		return -1;
	}

	gzipd_dev_cfg(tile_size, dest,
			header_info.isize, header_info.osize, header_info.blk_num, &tile_cnt, &last_tile_size);

	printf("header_info.blk_num=%d, isize=%d, osize=%d, tile_cnt=%d, last_tile_size=%d\n",
			header_info.blk_num, header_info.isize, header_info.osize, tile_cnt, last_tile_size);

	tiles_addr_start = img_compressed_addr + sizeof(header_info);
	tiles_addr_end = tiles_addr_start + tile_size * (tile_cnt - 1) ;

	if (tile_cnt == 0) {
		if (gzipd_dev_run_last_tile(tiles_addr_start, last_tile_size)) {
			printf(" run last tile error\n");
			return -1;
		}
		goto complete_finish;
	}

	ret = gzipd_dev_run(tiles_addr_start, tiles_addr_end, &run_num);
	if (ret) {
		printf("gzipd run decompress error\n");
		return -1;
	}

	if (gzipd_dev_run_last_tile(tiles_addr_end, last_tile_size)) {
		printf("lastly gzipd run last tile error\n");
		return -1;
	}

complete_finish:
	gzipd_dev_wait_complete_finish();

	return 0;
}
#endif

#ifdef CONFIG_SUPPORT_EMMC_BOOT
u32 calc_word_chksum(int *data, int size)
{
	int count = size / 4;
	int i;
	u32 sum = 0;

	/* It needs to be calculated by dma, temporarily using the cpu. */
	for(i = 0; i < count; i++) {
		sum += data[i];
	}
	return sum;
}
int flash_read_from_emmc(const char *part_name, void *dest)
{
	int ret = 0;
	disk_partition_t part_info;
	struct blk_desc *blk_dev_desc = NULL;
	uint64_t rd_blkcnt_lb;
	void *image_load_addr = (void *)(dest - sizeof(struct img_header));
	struct img_header *image_header =
						(struct img_header *)image_load_addr;

	blk_dev_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!blk_dev_desc) {
		printf("get mmc dev fail\n");
		return -1;
	}

	ret = get_part_info(blk_dev_desc, part_name, &part_info);
	if (ret == -1) {
		printf("%s: get %s partition info fail\n", __FUNCTION__, part_name);
		return -1;
	}
	rd_blkcnt_lb = blk_dread(blk_dev_desc, part_info.start,
				(sizeof(struct img_header) + 511) / 512, image_load_addr);
	if (rd_blkcnt_lb != (sizeof(struct img_header) + 511) / 512) {
		printf("read %s image header failed\n", part_name);
		return -1;
	}

	printf("reading %s image ...\n", part_name);
	rd_blkcnt_lb = blk_dread(blk_dev_desc, part_info.start,
						(sizeof(struct img_header) + image_header->img_size + 511) / 512, image_load_addr);
	if (rd_blkcnt_lb != ((sizeof(struct img_header) + image_header->img_size + 511) / 512)) {
		printf("%s get %s image fail\n", __func__, part_name);
		return -1;
	}

	if (image_header->capability & IMG_CHECK_ENABLE) {
		if (calc_word_chksum((int *)(image_load_addr + sizeof(struct img_header)), image_header->img_size) != image_header->img_check_sum) {
			printf("calc_word_chksum %s image failed\n", part_name);
			return -1;
		}
	}

	return image_header->img_size;
}
#endif

#if !defined(CONFIG_MTD_SPI_NAND) && defined(CONFIG_SPI_FLASH)
int flash_read_from_nor(const char *part_name, void *dest)
{
	u32 ret;
	u32 busnum = 0;
	loff_t off = 0;
	int idx;
	loff_t size, maxsize;
	size_t retlen;
	struct udevice *dev;
	struct mtd_info *mtd;
	struct spi_flash *flash;
	void *image_load_addr = (void *)(dest - sizeof(struct img_header));
	struct img_header *image_header =
						(struct img_header *)image_load_addr;

	ret = uclass_get_device(UCLASS_SPI_FLASH, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}
	mtd = get_mtd_device_nm("nor0");
	flash = dev_get_uclass_priv(dev);

	ret = mtd_arg_off(part_name, &idx, &off, &size, &maxsize, MTD_DEV_TYPE_NOR, flash->size);
	if (ret) {
		printf("%s, %d, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	size = sizeof(struct img_header);
	mtd_read(mtd, off, size, &retlen, (u_char *) image_load_addr);
	if (retlen != size) {
		printf("%s image read fail!\n", part_name);
		return -1;
	}

	size = sizeof(struct img_header) + image_header->img_size + 1023;
	printf("reading %s image size 0x%llX from spi nor flash ...\n", part_name, size);
	mtd_read(mtd, off, size, &retlen, (u_char *) image_load_addr);
	if (retlen != size) {
		printf("%s image read fail!\n", part_name);
		return -1;
	}

	return image_header->img_size;
}
#endif

#ifdef CONFIG_MTD_SPI_NAND
static int raw_read_nand(struct mtd_info *mtd, loff_t start, loff_t size, void *buff)
{
	loff_t read_addr;
	loff_t read_size, remaining;
	size_t retlen;
	u32 load_buf_off = 0;

	read_addr = start;
	remaining = size;

	while (remaining) {
		if (mtd_block_isbad(mtd, read_addr)) {
			printf("nand addr 0x%llX, blk %lld is bad\n", read_addr, (u64) read_addr / mtd->erasesize);
			read_addr += mtd->erasesize;
			continue;
		}

		read_size = (remaining > mtd->erasesize) ? mtd->erasesize : remaining;
		if (read_addr + read_size > partOff + partSize) {
			printf("%s: read addr 0x%llX len 0x%llX over kernel-dtb part addr 0x%llX size 0x%llX\n",
			       __func__, read_addr, read_size, partOff, partSize);
			return -1;
		}

		mtd_read(mtd, read_addr, read_size, &retlen, (u_char *)(buff + load_buf_off));
		if (retlen != read_size) {
			printf("image read fail!\n");
			return -1;
		}
		remaining -= read_size;
		load_buf_off += read_size;
		read_addr += read_size;
	}

    return 0;
}
int flash_read_from_nand(const char *part_name, void *dest)
{
	u32 ret;
	u32 busnum = 0;
	int idx;
	loff_t size, maxsize;
	struct udevice *dev;
	struct mtd_info *mtd;
	void *image_load_addr = (void *)(dest - sizeof(struct img_header));
	struct img_header *image_header =
						(struct img_header *)image_load_addr;

	ret = uclass_get_device(UCLASS_MTD, busnum, &dev);
	if (ret) {
		printf("uclass_get_device: Invalid bus %d (err=%d)\n", busnum, ret);
		return ret;
	}
	mtd = dev_get_uclass_priv(dev);
	partOff = 0;
	partSize = 0;
	ret = mtd_arg_off(part_name, &idx, &partOff, &partSize, &maxsize, MTD_DEV_TYPE_NAND, mtd->size);
	if (ret) {
		printf("%s, %d, ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}
	printf("%s: part %s, flash addr 0x%llX, size 0x%llX\n", __func__, part_name, partOff, partSize);

	if ((u64) partOff % mtd->erasesize) {
		pr_err("nand addr 0x%llX is not block size 0x%X aligned!\n", partOff, mtd->erasesize);
		return -1;
	}

	size = sizeof(struct img_header);
	ret = raw_read_nand(mtd, partOff, sizeof(struct img_header), (void *) image_load_addr);
	if (ret != 0) {
		printf("header image read fail!\n");
		return -1;
	}

	size = sizeof(struct img_header) + image_header->img_size + 1023;
	printf("reading %s image size 0x%llX from spi nand flash ...\n", part_name, size);
	raw_read_nand(mtd, partOff, size, (void *) image_load_addr);
	if (ret != 0) {
		printf("%s image read fail!\n", part_name);
		return -1;
	}

	return image_header->img_size;
}
#endif

int flash_raw_read(const char *part_name, void *dest)
{
	u64 read_len = 0;
#if defined CONFIG_SPI_FLASH || CONFIG_MTD_SPI_NAND
	char *bootargs = NULL;
	char *mtdparts = NULL;
#endif
#if !defined(CONFIG_MTD_SPI_NAND) && defined(CONFIG_SPI_FLASH)
	char *mtdids = NULL;
#endif

	switch (boot_info_data.storage_sel) {
	case STORAGE_TYPE_EMMC:
#ifdef CONFIG_SUPPORT_EMMC_BOOT
		read_len = flash_read_from_emmc(part_name, dest);
#endif
		break;
	case STORAGE_TYPE_NAND:
#ifdef CONFIG_MTD_SPI_NAND
		bootargs = env_get("bootargs");
		if(NULL == bootargs) {
			printf("bootargs not found in env, will use default\n");
			bootargs = BOOTARGS_SPINAND;
			env_set("bootargs", bootargs);
			env_save();
		} else {
			mtdparts = strstr(bootargs , "mtdparts");
			if (NULL != mtdparts) {
				mtdparts = strdup(mtdparts);
				strtok(mtdparts, " ");
				env_set("mtdparts", mtdparts);
				free(mtdparts);
			}
		}
		read_len = flash_read_from_nand(part_name, dest);
#endif
		break;
	case STORAGE_TYPE_NOR:
#if !defined(CONFIG_MTD_SPI_NAND) && defined(CONFIG_SPI_FLASH)
		bootargs = env_get("bootargs");
		if(NULL == bootargs) {
			printf("bootargs not found in env, will use default\n");
			bootargs = BOOTARGS_SPINOR;
			mtdparts = MTDPARTS_SPINOR;
			env_set("bootargs", bootargs);
			env_set("mtdparts", mtdparts);
			env_save();
		} else {
			mtdparts = strstr(bootargs , "mtdparts");
			if (NULL != mtdparts) {
				mtdparts = strdup(mtdparts);
				strtok(mtdparts, " ");
				env_set("mtdparts", mtdparts);
				free(mtdparts);
			} else {
				mtdparts = MTDPARTS_SPINOR;
				env_set("mtdparts", mtdparts);
			}
		}
		mtdids = env_get("mtdids");
		if (NULL == mtdids) {
			env_set("mtdids", MTDIDS_SPINOR);
		}

		read_len = flash_read_from_nor(part_name, dest);
#endif
		break;
	default:
		break;
	}

	return read_len;
}

int axera_secboot_image_check(const char *img_name, void *img_addr)
{
	int is_sec_enable;
#if defined(CONFIG_AXERA_SECURE_BOOT) && defined(CONFIG_CMD_AXERA_CIPHER)
	int key_bits, ret;
	int is_encrypted = 0;
	int aes_key[8];
	char public_key[PUB_KEY_ARRAY_MAX_SZ] = {0};
	struct img_header *image_header =
		(struct img_header *)(img_addr - sizeof(struct img_header));
#endif
#ifdef SECUREBOOT_ENABLE
	int secboot_verify = 1;
#else
	int secboot_verify = env_get_ulong("secureboot_test", 10, 0);
	if (secboot_verify) {
		int secboot_rsa2048_hash[8] = {
			0x78ddfe9e, 0x698a9288, 0xccad1548, 0x5ca04574,
			0x9f1aa945, 0xf57eb822, 0xb807db67, 0x29140398
		};
		for (int i = 0; i < 8; i++) {
			misc_info->pub_key_hash[i] = secboot_rsa2048_hash[i];
		}
		printf("sw secureboot is enabled\n");
	}
#endif

	is_sec_enable = is_secure_enable();
	if (is_sec_enable)
		secboot_verify = 1;

#if defined(CONFIG_AXERA_SECURE_BOOT) && defined(CONFIG_CMD_AXERA_CIPHER)
	if (secboot_verify != 0) {
		is_encrypted = (image_header->capability & IMG_CIPHER_ENABLE) ? 1 : 0;
		key_bits = (image_header->capability & RSA_3072_MODE) ? 3072 : 2048;

		printf("bondopt secureboot bit is enable:%d, key_bits:%d\n", is_sec_enable, key_bits);

		/* copy key_key_n_header and key(2048 or 3072) in bytes */
		memcpy((void *)public_key, (void *)&image_header->pub_key.key_n_header, (4 + key_bits / 8));
		/* copy key_e_header & rsa_key_e*/
		memcpy((void *)public_key + 4 + key_bits / 8, (void *)&image_header->pub_key.key_e_header, 8);
		flush_cache((unsigned long)public_key, sizeof(struct rsa_key));
		if (public_key_verify(public_key, sizeof(struct rsa_key)) < 0) {
			printf("public key verify failed\n");
			return -1;
		}

		printf("total %s image size = %d\n", img_name, image_header->img_size);

		ret = rsa_img_verify(&image_header->pub_key, (char *)img_addr,
							(char *)image_header->signature.signature, image_header->img_size, key_bits);
		if(ret < 0) {
			printf("%s image verify failed\n", img_name);
			return -1;
		}
		printf(">>> * %s image verify success! * <<< \n", img_name);
	}

	if (is_encrypted) {
		ret = cipher_aes_ecb_decrypto((int *)misc_info->aes_key,
			(u64)image_header->aes_key, (u64)aes_key, sizeof(image_header->aes_key));
		if (ret < 0) {
			printf("aes key cipher_aes_ecb_decrypto faield\r\n");
			return -1;
		}
		ret = cipher_aes_ecb_decrypto(aes_key, (unsigned long)img_addr,
										(unsigned long)img_addr, image_header->img_size);
		if (ret < 0) {
			printf("decrypto %s image failed\n", img_name);
			return -1;
		}
		printf(">>> * %s image decrypt success! * <<< \n", img_name);
	}
#else
	if (secboot_verify != 0) {
		printf("secboot verify is enabled, CONFIG option is not match\n");
		return -1;
	}
#endif

	return 0;
}

#ifndef CONFIG_ARM64
void ax_boot_kernel(char *img_addr,char *dtb_addr)
{
	unsigned long machid = MACH_TYPE_VEXPRESS;//MACRO MACH_TYPE_VEXPRESS 2272
	unsigned long r2;
	void (*kernel_entry)(int zero, int arch, uint params);
	kernel_entry = (void (*)(int, int, uint))(ulong)img_addr;
	fdt_chosen(dtb_addr);
	fdt_fixup_ethernet(dtb_addr);
	r2 = (ulong)dtb_addr;

	printf("## Transferring control to Linux (img at address %08lx) (dtb at address %08lx)" \
		"...\n", (ulong) kernel_entry,(ulong)dtb_addr);
	announce_and_cleanup(0);
	kernel_entry(0, machid, r2);
}
#endif

// ### SIPEED EDIT ###
static int load_file_from_boot_partition(char *filename, char **file_data, size_t *file_size)
{
	int ret = 0;
	struct blk_desc *mmc_desc = NULL;
	disk_partition_t fs_partition;
	char *parttiton = "boot";
	loff_t file_len = 0;

	mmc_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (NULL == mmc_desc) {
		printf("[error] memory dump: emmc is not present, exit dump!\n");
		return -1;
	}

	ret = get_part_info(mmc_desc, parttiton, &fs_partition);
	if(ret < 0) {
		printf("[error] memory dump get %s partition error, ret:%d\n", parttiton, ret);
		return ret;
	}

	if (fat_set_blk_dev(mmc_desc, &fs_partition) != 0) {
		mmc_desc = blk_get_dev("mmc", SD_DEV_ID);
		if (NULL == mmc_desc) {
			printf("[error] memory dump: emmc/sd is not present, exit dump!\n");
			return -1;
		}

		ret = fat_register_device(mmc_desc, 1);
		if (ret != 0) {
			printf("[error] fat_register_device failed\n");
			return -1;
		}
	}

	if (!fat_exists(filename)) {
		printf("%s file is not exist\n", filename);
		return -1;
	}

	if (fat_size(filename, &file_len) != 0) {
		printf("[error] get %s file size error\n", filename);
		return -1;
	}

	*file_size = file_len;
	*file_data = malloc(file_len);
	if (file_fat_read(filename, *file_data, file_len) <= 0) {
		printf("file_fat_read failed, ret:%d\n", ret);
		free(*file_data);
		return -1;
	}

	printf("load /boot/%s success\n", filename);
	return 0;
}

static char *find_key_value_from_string(char *str, char *key) {
	char new_key[128] = {0};
	char *line = strtok(str, "\n");
	char *value_str = NULL;
	snprintf(new_key, sizeof(new_key), "%s=", key);
	while (line != NULL) {
		while ((char)*line == ' ') line ++;

		if (strlen(line) == 0 || line[0] == '#') {
			line = strtok(NULL, "\n");
			continue;
		}

		char *found = strstr(line, new_key);
		if (found) {
			char *value_start = found + strlen(new_key);
			while ((char)*value_start == ' ') value_start ++;
			if (strlen(value_start) > 0) {
				value_str = strdup(value_start);
				break;
			}
		}

		line = strtok(NULL, "\n");
	}

	return value_str;
}

static void config_system_console(void) {
	// config system console
	char *buffer = NULL;
	size_t buffer_size = 0;
	if (load_file_from_boot_partition("configs", &buffer, &buffer_size) == 0) {
		bool disable_system_console = false;
		char console_config[128] = {0};
		char *tmp_buffer = strdup(buffer);
		char *value = find_key_value_from_string(tmp_buffer, "maix_system_console");
		if (value) {
			printf("found maix_system_console: %s\r\n", value);
			disable_system_console = strcmp(value, "0") ? false : true;
		}
		free(tmp_buffer);

		int kernel_loglevel = -1;
		tmp_buffer = strdup(buffer);
		value = find_key_value_from_string(tmp_buffer, "maix_kernel_loglevel");
		if (value) {
			printf("found maix_kernel_loglevel: %s\r\n", value);
			kernel_loglevel = simple_strtoul(value, NULL, 10);
		}
		free(tmp_buffer);

		char *_maix_console = env_get("maix_console");
		if (_maix_console) {
			printf("found maix_console: %s\r\n", _maix_console);
			disable_system_console = strstr(_maix_console, "1") ? false : true;
		}

		printf("try %s system console\r\n", disable_system_console ? "disable" : "enable");

		if (disable_system_console) {
			char new_bootargs[1024] = {0};
			char *_bootargs = env_get("bootargs");
			memcpy(new_bootargs, _bootargs, strlen(_bootargs));

			char *console = strstr(new_bootargs, "console=");
			char *end = console;
			while ((char)*end != ' ') end ++;
			if ((char)*end != '\0') end ++;
			strcpy(console, console_config);
			strcat(new_bootargs, end);
			env_set("bootargs", new_bootargs);
			free(buffer);
		}

		if (kernel_loglevel >= 0) {
			char new_bootargs[1024] = {0};
			char *_bootargs = env_get("bootargs");
			memcpy(new_bootargs, _bootargs, strlen(_bootargs));

			char *loglevel = strstr(new_bootargs, "loglevel=");
			char *end = loglevel;
			while ((char)*end != ' ') end ++;
			if ((char)*end != '\0') end ++;
			char loglevel_config[32] = {0};
			sprintf(loglevel_config, "loglevel=%d", kernel_loglevel);
			strcpy(loglevel, loglevel_config);
			strcat(new_bootargs, end);
			env_set("bootargs", new_bootargs);
			if (buffer != NULL)
				free(buffer);
		}
	}
}
// ### SIPEED EDIT END ###

int do_axera_boot(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int ret = -1;
	char *img_addr = NULL;
	char *dtb_addr = NULL;
	char boot_cmd[30];
	int kernel_image_size;
	int dtb_image_size;
#ifdef CONFIG_SUPPORT_RECOVERY
	char *bootable = NULL;
	int recovery_image_size;
#endif
#ifdef SPI_DUAL_CS
	char *sf1_parts = NULL;
	char *default_bootargs = NULL;
	char dualflash_bootargs[512] = {0};
#endif

	const char *x_kernel = "kernel";
	const char *x_dtb = "dtb";

	// ### SIPEED EDIT ###
	config_system_console();
	// ### SIPEED EDIT END ###
#ifdef CONFIG_AXERA_EMAC
	ax_shutdown_ephy();
#endif

#ifdef CONFIG_SUPPORT_AB
	char *bootsystem = env_get("bootsystem");
	if (bootsystem == NULL) {
		x_kernel = "kernel";
		x_dtb = "dtb";
	} else {
		if (strcmp(bootsystem, "B") == 0) {
			x_kernel = "kernel_b";
			x_dtb = "dtb_b";
		} else {
			x_kernel = "kernel";
			x_dtb = "dtb";
		}
	}
#endif

#if defined CONFIG_CMD_AXERA_GZIPD || CONFIG_CMD_AXERA_KERNEL_LZMA
	img_addr = (char *)KERNEL_IMAGE_COMPRESSED_ADDR;
	dtb_addr = (char *)DTB_IMAGE_COMPRESSED_ADDR;
#else
	img_addr = (char *)KERNEL_IMAGE_ADDR;
	dtb_addr = (char *)DTB_IMAGE_ADDR;
#endif

	pr_err("enter do_axera_boot\n");
	/* Turn on watchdog before booting to kernel. */
	wdt0_enable(1);
#if defined(CONFIG_AXERA_SECURE_BOOT) && defined(CONFIG_CMD_AXERA_CIPHER)
	AX_CIPHER_Init();
#endif

#ifdef CONFIG_SUPPORT_RECOVERY
	bootable = env_get("bootable");
	if (!strcmp(bootable, "recovery")) {
		printf("From recovery boot\n");

		env_set("bootargs", RECOVERY_BOOTARGS);

		recovery_image_size = flash_raw_read("recovery", (void *)KERNEL_IMAGE_ADDR);
		if (recovery_image_size < 0) {
			pr_err("recovery image read failed\n");
			return -1;
		}
		flush_dcache_all();

		if (axera_secboot_image_check("recovery", (void *)KERNEL_IMAGE_ADDR)) {
			pr_err("recovery image secureboot checked failed\n");
			return -1;
		}

		sprintf(boot_cmd, "bootz 0x%lx", (unsigned long)KERNEL_IMAGE_ADDR);

		printf("recovery boot cmd is :%s\n", boot_cmd);
		run_command_list(boot_cmd, -1, 0);
	}
#endif

	/* step 1: read raw image data which is with image_header */
	kernel_image_size = flash_raw_read(x_kernel, (void *)img_addr);
	if (kernel_image_size < 0) {
		pr_err("kernel image read failed\n");
		return -1;
	}

	dtb_image_size = flash_raw_read(x_dtb, (void *)dtb_addr);
	if (dtb_image_size < 0) {
		pr_err("dtb image read failedi\n");
		return -1;
	}
	flush_dcache_all();
#ifdef SPI_DUAL_CS
	sf1_parts = env_get("sf1_parts");
	default_bootargs = env_get("bootargs");
	if (sf1_parts && !strstr(default_bootargs, sf1_parts)) {
		memcpy(dualflash_bootargs, default_bootargs, strlen(default_bootargs));
		dualflash_bootargs[strlen(default_bootargs)] = ';';
		memcpy(dualflash_bootargs + strlen(dualflash_bootargs), sf1_parts, strlen(sf1_parts));
		printf("dualflash_bootargs[%d]: %s\n", strlen(dualflash_bootargs), dualflash_bootargs);
		env_set("bootargs", dualflash_bootargs);
	}
#endif

	/* step 2: secure verify check */
	if (axera_secboot_image_check(x_kernel, (void *)img_addr)) {
		pr_err("kernel image secureboot checked failed\n");
		goto failed;
	} else if (axera_secboot_image_check(x_dtb, (void *)dtb_addr)) {
		pr_err("dtb image secureboot checked failed\n");
		goto failed;
	}

	/* step 3: axgzip to dest address */

#ifdef CONFIG_CMD_AXERA_KERNEL_LZMA
	if (lzma_decompress_image((void *)KERNEL_IMAGE_COMPRESSED_ADDR, (void *)KERNEL_IMAGE_ADDR, kernel_image_size)) {
		pr_err("kernel image decompress failed\n");
		return -1;
	}

	if (gzip_decompress_image((void *)DTB_IMAGE_COMPRESSED_ADDR,  (void *)DTB_IMAGE_ADDR, dtb_image_size)) {
		pr_err("dtb image decompress failed\n");
		return -1;
	}

	invalidate_dcache_all();
#else /*gzip*/
	if (gzip_decompress_image((void *)KERNEL_IMAGE_COMPRESSED_ADDR, (void *)KERNEL_IMAGE_ADDR, kernel_image_size)) {
		pr_err("kernel image decompress failed\n");
		return -1;
	}
	if (gzip_decompress_image((void *)DTB_IMAGE_COMPRESSED_ADDR,  (void *)DTB_IMAGE_ADDR, dtb_image_size)) {
		pr_err("dtb image decompress failed\n");
		return -1;
	}
	invalidate_dcache_all();
#endif

#ifdef CONFIG_VIDEO_AXERA
	extern void fdt_fixup_logo_info(void *fdt);

	fdt_fixup_logo_info((void *)(unsigned long)DTB_IMAGE_ADDR);
#endif

#ifdef CONFIG_ARM64
	sprintf(boot_cmd, "booti 0x%lx - 0x%lx", (unsigned long)KERNEL_IMAGE_ADDR, (unsigned long)DTB_IMAGE_ADDR);
#else
	ax_boot_kernel((void *)(unsigned long)KERNEL_IMAGE_ADDR, (void *)(unsigned long)DTB_IMAGE_ADDR);
#endif
	printf("boot cmd is :%s\n", boot_cmd);
	run_command_list(boot_cmd, -1, 0);

#ifdef SUPPORT_RECOVERY
failed:
	writel(0x800, TOP_CHIPMODE_GLB_BACKUP0_SET);
	reboot();
#else
failed:
	return -1;
#endif

	//in fact will not come here
	pr_err("axera boot kernel failed\n");
	while (1);

	return ret;
}

U_BOOT_CMD(axera_boot, 1, 0, do_axera_boot,
	   "axera boot", "axera enter normal boot mode\n" "it is used for axera boot to kernel\n");
