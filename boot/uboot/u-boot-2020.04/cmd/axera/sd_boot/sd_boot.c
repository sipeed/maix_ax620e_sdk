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
#include "../secureboot/secureboot.h"
#include <dm/uclass.h>
#include <dm/device.h>
#include <mtd.h>
#include <cpu_func.h>
#include "../../legacy-mtd-utils.h"
#include "axera_update.h"
#include "../boot/axera_boot.h"
#ifdef CONFIG_CMD_AXERA_GZIPD
#include "../gzipd/ax_gzipd_api.h"

extern int gzip_decompress_image(void *src, void *dest, u32 size);
#endif

extern struct boot_mode_info boot_info_data;

#define READ_IMG_SIZE (5 * 1024 *1024)

// ### SIPEED EDIT ###
#define SD_BOOT_CMD_LEN 192
#define SD_BOOT_MAX_IMAGE_SIZE (64U * 1024 * 1024)
#define SD_BOOT_RECOVERY_PART 1
#define SD_BOOT_NORMAL_PART 2
#define SD_BOOT_RECOVERY_KERNEL "kernel.img"
#define SD_BOOT_RECOVERY_DTB "dtb.img"
#define SD_BOOT_NORMAL_KERNEL "/boot/kernel.img"
#define SD_BOOT_NORMAL_DTB "/boot/dtb.img"
#define SD_BOOT_NORMAL_CONFIGS "/boot/configs"
#define SD_BOOT_MAX_CONFIG_SIZE (1024 * 1024)

enum sd_boot_config_source {
	SD_BOOT_CONFIG_P1_FAT,
	SD_BOOT_CONFIG_P2_EXT4,
	SD_BOOT_CONFIG_DEFAULT,
};
// ### SIPEED EDIT END ###

#ifndef CONFIG_ARM64
void ax_boot_kernel(char *img_addr,char *dtb_addr);
#endif

#ifdef CONFIG_CMD_AXERA_KERNEL_LZMA
int lzma_decompress_image(void *src, void *dest, u32 size);
#endif

// ### SIPEED EDIT ###
static enum sd_boot_config_source sd_boot_config_source = SD_BOOT_CONFIG_P1_FAT;

static int __atoi(char *str) {
    int result = 0;
    int sign = 1;

    while (*str == ' ' || *str == '\t') {
        str++;
    }

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}

static bool check_cmm_size_is_valid(int cmm_size)
{
	int board_id = get_board_id();
	int mmc_max_size = 0;
	switch (board_id) {
	case PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G:
		mmc_max_size = 512;
		break;
	case PHY_AX630C_AX631_MAIXCAM2_SOM_1G:
		mmc_max_size = 1024;
		break;
	case PHY_AX630C_AX631_MAIXCAM2_SOM_2G:
		mmc_max_size = 2048;
		break;
	case PHY_AX630C_AX631_MAIXCAM2_SOM_4G:
		mmc_max_size = 4096;
		break;
	case PHY_AX620QE_LP4_NANOAGENT_512M:
		mmc_max_size = 512;
		break;
	case PHY_AX620QF_LP4_NANOAGENT_256M:
		mmc_max_size = 256;
		break;
	default:
		mmc_max_size = 512;
		break;
	break;
	}

	if (cmm_size < 0) {
		return false;
	} else if (mmc_max_size - cmm_size < 128) {	// os min size is 128MiB
		return false;
	} else {
		return true;
	}
}

static int read_cmm_size_from_boot(int *cmm_size) {
	int ret = 0;
	struct blk_desc *mmc_desc;
	char *filename = "configs";

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

	if (fat_exists(filename)) {
		int buffer_size = 15 * 1024 * 1024;
		char *buffer = malloc(buffer_size);
		if (buffer == NULL) {
			printf("malloc buffer failed\n");
			return -1;
		}
		if (file_fat_read(filename, buffer, buffer_size) <= 0) {
			printf("file_fat_read failed, ret:%d\n", ret);
			free(buffer);
			return -1;
		}

		int value = -1;
		char *key = "maix_memory_cmm";
		char *line = strtok(buffer, "\n");
		while (NULL != line) {
			char *key_value_str = strstr(line, key);
			if (key_value_str != NULL) {
				char *p = line;
				bool key_is_valid = true;
				while (p != key_value_str) {
					if (*p == '#') {
						key_is_valid = false;
						printf("maix_memory_cmm is invalid\r\n");
						break;
					}
					p ++;
				}

				if (key_is_valid) {
					char *value_str = strstr(key_value_str, "=") + 1;
					value = __atoi(value_str);
				}
				break;
			}
			line = strtok(NULL, "\n");
		}

		if (cmm_size) {
			*cmm_size = value;
		}
		free(buffer);
	} else {
		printf("%s file is not exist\n", filename);
		return -1;
	}
	return ret;
}

static int check_and_config_upgrade(void)
{
	uint32_t val = 0;
	writel(0x00000003, 0x2302024);
	val = readl(0x2302024);
	val = readl(0x600100c) & (~0x2);
	writel(val, 0x600100c);
	val = readl(0x600100c);
	udelay(1000);

	val = readl(0x600108c);
	val = readl(0x600108c);
	int boot_key = (val >> 2) & 0x01;

	char *_bootargs = NULL;
	_bootargs = env_get("bootargs");

	// delete boot_key=xxx
	char *pos = strstr(_bootargs, "boot_key=");
	if (pos) {
		char *pos2 = strstr(pos + strlen("boot_key="), " ");
		if (pos2) {
			strcpy(pos, pos2 + 1);
		} else {
			*pos = '\0';
		}
	}

	if (boot_key == 0) //  boot key pressed
	{
		char new_bootargs[512] = {0};
		printf("boot key pressed\n");

		memcpy(new_bootargs, _bootargs, strlen(_bootargs));
		char *boot_key_arg = " boot_key=1";
		memcpy(new_bootargs + strlen(new_bootargs), boot_key_arg, strlen(boot_key_arg));
		printf("new_bootargs[%d]: %s\n", strlen(new_bootargs), new_bootargs);
		env_set("bootargs", new_bootargs);

		return 1;
	} else {
		return 0;
	}
}

static int load_file_from_boot_partition(char *filename, char **file_data, size_t *file_size)
{
	int ret = 0;
	struct blk_desc *mmc_desc = NULL;
	loff_t file_len = 0;

	mmc_desc = blk_get_dev("mmc", SD_DEV_ID);
	if (NULL == mmc_desc) {
		printf("[error] memory dump: emmc is not present, exit dump!\n");
		return -1;
	}

	ret = fat_register_device(mmc_desc, 1);
	if (ret != 0) {
		printf("[error] fat_register_device failed\n");
		return -1;
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

static int load_configs_from_normal_partition(char **file_data, size_t *file_size)
{
	char boot_cmd[SD_BOOT_CMD_LEN] = {0};
	char *filesize = NULL;
	ulong file_len = 0;
	int ret = 0;

	ret = snprintf(boot_cmd, sizeof(boot_cmd), "ext4load mmc 1:%d 0x%x %s",
		       SD_BOOT_NORMAL_PART, (unsigned int)SD_BOOT_IMAGE_ADDR,
		       SD_BOOT_NORMAL_CONFIGS);
	if (ret < 0 || (size_t)ret >= sizeof(boot_cmd)) {
		printf("sd boot: configs load command too long\n");
		return -1;
	}

	printf("sd boot: %s\n", boot_cmd);
	ret = run_command_list(boot_cmd, -1, 0);
	if (ret != 0) {
		printf("sd boot: %s is not exist on ext4 mmc 1:%d, use defaults\n",
		       SD_BOOT_NORMAL_CONFIGS, SD_BOOT_NORMAL_PART);
		return -1;
	}

	filesize = env_get("filesize");
	if (!filesize)
		return -1;

	file_len = simple_strtoul(filesize, NULL, 16);
	if (file_len == 0 || file_len > SD_BOOT_MAX_CONFIG_SIZE) {
		printf("sd boot: invalid %s size %lu bytes, use defaults\n",
		       SD_BOOT_NORMAL_CONFIGS, file_len);
		return -1;
	}

	*file_size = file_len;
	*file_data = malloc(file_len + 1);
	if (!*file_data) {
		printf("malloc %s failed\n", SD_BOOT_NORMAL_CONFIGS);
		return -1;
	}
	memset(*file_data, 0, file_len + 1);
	memcpy(*file_data, (void *)SD_BOOT_IMAGE_ADDR, file_len);

	printf("sd boot: load %s success\n", SD_BOOT_NORMAL_CONFIGS);
	return 0;
}

static int load_configs_file(char **file_data, size_t *file_size)
{
	if (sd_boot_config_source == SD_BOOT_CONFIG_DEFAULT)
		return -1;

	if (sd_boot_config_source == SD_BOOT_CONFIG_P2_EXT4)
		return load_configs_from_normal_partition(file_data, file_size);

	return load_file_from_boot_partition("configs", file_data, file_size);
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

static int read_cmm_size_from_configs(int *cmm_size)
{
	char *buffer = NULL;
	char *tmp_buffer = NULL;
	char *value = NULL;
	size_t buffer_size = 0;

	if (sd_boot_config_source == SD_BOOT_CONFIG_DEFAULT)
		return -1;

	if (sd_boot_config_source == SD_BOOT_CONFIG_P1_FAT)
		return read_cmm_size_from_boot(cmm_size);

	if (load_configs_file(&buffer, &buffer_size) != 0)
		return -1;

	tmp_buffer = strdup(buffer);
	if (tmp_buffer) {
		value = find_key_value_from_string(tmp_buffer, "maix_memory_cmm");
		if (value) {
			printf("found maix_memory_cmm: %s\r\n", value);
			if (cmm_size)
				*cmm_size = __atoi(value);
			free(value);
		}
		free(tmp_buffer);
	}
	free(buffer);

	return 0;
}

static void config_system_console(void) {
	// config system console
	char *buffer = NULL;
	size_t buffer_size = 0;
	if (load_configs_file(&buffer, &buffer_size) == 0) {
		bool disable_system_console = false;
		char console_config[128] = "console=null ";
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
		}

		if (kernel_loglevel >= 0) {
			char new_bootargs[1024] = {0};
			char *_bootargs = env_get("bootargs");
			char loglevel_config[32] = {0};
			char *loglevel = strstr(_bootargs, "loglevel=");
			int ret;

			if (kernel_loglevel == 0)
				sprintf(loglevel_config, "loglevel=%d quiet", kernel_loglevel);
			else
				sprintf(loglevel_config, "loglevel=%d", kernel_loglevel);
			if (loglevel) {
				char *end = loglevel;
				while ((char)*end != '\0' && (char)*end != ' ') end ++;
				if ((char)*end != '\0') end ++;
				ret = snprintf(new_bootargs, sizeof(new_bootargs), "%.*s%s%s%s",
					(int)(loglevel - _bootargs), _bootargs, loglevel_config,
					*end ? " " : "", end);
			} else {
				ret = snprintf(new_bootargs, sizeof(new_bootargs), "%s %s",
					_bootargs, loglevel_config);
			}
			if (ret < 0 || ret >= sizeof(new_bootargs)) {
				printf("set kernel loglevel failed\n");
				free(buffer);
				return;
			}
			env_set("bootargs", new_bootargs);
		}
		free(buffer);
	}
}

static void config_cmm_size(void) {
	int cmm_size = -1;
	if (0 != read_cmm_size_from_configs(&cmm_size)) {
		cmm_size = -1;	// if failed, use default cmm_size
	}
	printf("user config cmm from sd boot: %d\n", cmm_size);

	const char *oldargs = env_get("bootargs");
	char newargs[1024], new_mem_cfg[128];
	int board_id = get_board_id();
	bool need_config_bootargs = false;

	if (!check_cmm_size_is_valid(cmm_size)) {
		switch (board_id) {
		case PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G:
			strcpy(new_mem_cfg, BOARD_0_5G_OS_MEM);
			need_config_bootargs = true;
			break;
		case PHY_AX630C_AX631_MAIXCAM2_SOM_1G:
			strcpy(new_mem_cfg, BOARD_1G_OS_MEM);
			need_config_bootargs = true;
			break;
		case PHY_AX630C_AX631_MAIXCAM2_SOM_2G:
			strcpy(new_mem_cfg, BOARD_2G_OS_MEM);
			need_config_bootargs = true;
			break;
		case PHY_AX630C_AX631_MAIXCAM2_SOM_4G:
			strcpy(new_mem_cfg, BOARD_4G_OS_MEM);
			need_config_bootargs = true;
			break;
		case PHY_AX620QE_LP4_NANOAGENT_512M:
			strcpy(new_mem_cfg, BOARD_512M_OS_MEM);
			need_config_bootargs = true;
			break;
		case PHY_AX620QF_LP4_NANOAGENT_256M:
			strcpy(new_mem_cfg, BOARD_256M_OS_MEM);
			need_config_bootargs = true;
			break;
		default:
			strcpy(new_mem_cfg, BOARD_1G_OS_MEM);
			need_config_bootargs = true;
			break;
		break;
		}
	} else {
		int os_mem_size = 0;
		switch (board_id) {
		case PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G:
			os_mem_size = 512 - cmm_size;
			break;
		case PHY_AX630C_AX631_MAIXCAM2_SOM_1G:
			os_mem_size = 1024 - cmm_size;
			break;
		case PHY_AX630C_AX631_MAIXCAM2_SOM_2G:
			os_mem_size = 2048 - cmm_size;
			break;
		case PHY_AX630C_AX631_MAIXCAM2_SOM_4G:
			os_mem_size = 4096 - cmm_size;
			break;
		case PHY_AX620QE_LP4_NANOAGENT_512M:
			os_mem_size = 512 - cmm_size;
			break;
		case PHY_AX620QF_LP4_NANOAGENT_256M:
			os_mem_size = 256 - cmm_size;
			break;
		default:
			os_mem_size = 512 - cmm_size;
			break;
		break;
		}

		snprintf(new_mem_cfg, sizeof(new_mem_cfg), "mem=%dM", os_mem_size);
		need_config_bootargs = true;
	}

	if (need_config_bootargs) {
		char *mem_start = strstr(oldargs, "mem");
		char *mem_end = strchr(mem_start, ' ');
		int mem_start_offset = mem_start - oldargs;
		memcpy(newargs, oldargs, mem_start_offset);
		memcpy(newargs + mem_start_offset, new_mem_cfg, strlen(new_mem_cfg));
		strcpy(newargs + mem_start_offset + strlen(new_mem_cfg), mem_end);
		env_set("bootargs", newargs);
	}
}

static void config_console_for_nanoagent(void)
{
	const char *old_console = "console=ttyS0,115200n8 earlycon=uart8250,mmio32,0x4880000";
	const char *new_console = "console=ttyS3,115200n8 earlycon=uart8250,mmio32,0x6080000";
	char new_bootargs[1024] = {0};
	char *bootargs = NULL;
	char *console_pos = NULL;
	int board_id = get_board_id();
	int ret;

	if (board_id != PHY_AX620QE_LP4_NANOAGENT_512M && board_id != PHY_AX620QF_LP4_NANOAGENT_256M)
		return;

	bootargs = env_get("bootargs");
	if (!bootargs)
		return;

	console_pos = strstr(bootargs, old_console);
	if (!console_pos)
		return;

	ret = snprintf(new_bootargs, sizeof(new_bootargs), "%.*s%s%s",
		(int)(console_pos - bootargs), bootargs,
		new_console, console_pos + strlen(old_console));
	if (ret < 0 || ret >= sizeof(new_bootargs)) {
		printf("set nanoagent console failed\n");
		return;
	}

	env_set("bootargs", new_bootargs);
	printf("set nanoagent console to ttyS3\n");
}

static bool is_nanokvm_go_board(void)
{
	int board_id = get_board_id();

	return board_id == PHY_AX620QE_LP4_NANOAGENT_512M ||
	       board_id == PHY_AX620QF_LP4_NANOAGENT_256M;
}

static void remove_boot_key_arg(char *bootargs)
{
	char *pos = NULL;
	char *pos2 = NULL;

	if (!bootargs)
		return;

	while ((pos = strstr(bootargs, "boot_key=")) != NULL) {
		pos2 = strstr(pos + strlen("boot_key="), " ");
		if (pos2) {
			strcpy(pos, pos2 + 1);
		} else {
			*pos = '\0';
		}
	}
}

static int set_recovery_bootargs(void)
{
	char new_bootargs[1024] = {0};
	char *bootargs = env_get("bootargs");
	size_t bootargs_len = 0;
	size_t remain = 0;
	int ret = 0;

	if (!bootargs)
		bootargs = "";

	ret = snprintf(new_bootargs, sizeof(new_bootargs), "%s", bootargs);
	if (ret < 0 || (size_t)ret >= sizeof(new_bootargs)) {
		printf("set recovery bootargs failed\n");
		return -1;
	}

	remove_boot_key_arg(new_bootargs);

	bootargs_len = strlen(new_bootargs);
	remain = sizeof(new_bootargs) - bootargs_len;
	ret = snprintf(new_bootargs + bootargs_len, remain, "%sboot_key=1",
		       bootargs_len ? " " : "");
	if (ret < 0 || (size_t)ret >= remain) {
		printf("append recovery bootargs failed\n");
		return -1;
	}

	printf("new_bootargs[%d]: %s\n", (int)strlen(new_bootargs), new_bootargs);
	env_set("bootargs", new_bootargs);

	return 0;
}

static int run_sd_load_cmd(const char *fs_cmd, int part, const char *filename,
			   u32 size, u32 offset)
{
	char boot_cmd[SD_BOOT_CMD_LEN] = {0};
	int ret = 0;

	ret = snprintf(boot_cmd, sizeof(boot_cmd),
		       "%s mmc 1:%d 0x%x %s 0x%x 0x%x",
		       fs_cmd, part, (unsigned int)SD_BOOT_IMAGE_ADDR,
		       filename, size, offset);
	if (ret < 0 || (size_t)ret >= sizeof(boot_cmd)) {
		printf("sd boot: load command too long for %s\n", filename);
		return -1;
	}

	printf("sd boot: %s\n", boot_cmd);
	ret = run_command_list(boot_cmd, -1, 0);
	if (ret != 0) {
		printf("sd boot: load %s from mmc 1:%d failed, ret=%d\n",
		       filename, part, ret);
		return -1;
	}

	return 0;
}

static int load_sd_boot_image(const char *fs_cmd, int part,
			      const char *filename, char *image_addr,
			      u64 *image_size)
{
	struct img_header *boot_img_header = NULL;
	u32 img_size = 0;
	u32 read_size = 0;
	u32 offset = 0;
	int cnt = 0;
	int last_size = 0;
	int j = 0;

	memset((void *)SD_BOOT_IMAGE_ADDR, 0, SECBOOT_HEADER_SIZE);
	if (run_sd_load_cmd(fs_cmd, part, filename, SECBOOT_HEADER_SIZE, 0) != 0)
		return -1;

	boot_img_header = (struct img_header *)SD_BOOT_IMAGE_ADDR;
	img_size = boot_img_header->img_size;
	if (img_size == 0 || img_size > SD_BOOT_MAX_IMAGE_SIZE) {
		printf("sd boot: invalid %s image size %u bytes\n",
		       filename, img_size);
		return -1;
	}

	if (image_size)
		*image_size = img_size;

	printf("sd boot: %s image size is %u bytes\n", filename, img_size);
	memset((void *)image_addr, 0, img_size);

	cnt = img_size / READ_IMG_SIZE;
	last_size = img_size % READ_IMG_SIZE;

	for (j = 0; j < cnt; j++) {
		read_size = READ_IMG_SIZE;
		offset = SECBOOT_HEADER_SIZE + READ_IMG_SIZE * j;
		memset((void *)SD_BOOT_IMAGE_ADDR, 0, read_size);
		if (run_sd_load_cmd(fs_cmd, part, filename, read_size, offset) != 0)
			return -1;

		memmove((void *)(image_addr + READ_IMG_SIZE * j),
			(void *)SD_BOOT_IMAGE_ADDR, read_size);
	}

	if (last_size) {
		read_size = last_size;
		offset = SECBOOT_HEADER_SIZE + READ_IMG_SIZE * j;
		memset((void *)SD_BOOT_IMAGE_ADDR, 0, read_size);
		if (run_sd_load_cmd(fs_cmd, part, filename, read_size, offset) != 0)
			return -1;

		memmove((void *)(image_addr + READ_IMG_SIZE * j),
			(void *)SD_BOOT_IMAGE_ADDR, read_size);
	}

	printf("sd boot: %s read %u finish from %s mmc 1:%d\n",
	       filename, img_size, fs_cmd, part);

	return 0;
}

static int load_nanokvm_go_boot_images(int recovery_boot,
						       char *img_addr, char *dtb_addr,
						       u64 *kernel_image_size,
						       u64 *dtb_image_size,
						       int *final_recovery_boot)
{
	int ret = 0;

	if (final_recovery_boot)
		*final_recovery_boot = recovery_boot ? 1 : 0;

	if (!recovery_boot) {
		printf("sd boot: try normal boot from ext4 mmc 1:%d\n",
		       SD_BOOT_NORMAL_PART);
		ret = load_sd_boot_image("ext4load", SD_BOOT_NORMAL_PART,
					 SD_BOOT_NORMAL_KERNEL, img_addr,
					 kernel_image_size);
		if (ret == 0) {
			ret = load_sd_boot_image("ext4load", SD_BOOT_NORMAL_PART,
						 SD_BOOT_NORMAL_DTB, dtb_addr,
						 dtb_image_size);
		}

		if (ret != 0) {
			printf("sd boot: normal boot failed, fallback to recovery boot\n");
			if (set_recovery_bootargs() != 0)
				return -1;
			recovery_boot = 1;
			if (final_recovery_boot)
				*final_recovery_boot = 1;
		}
	}

	if (recovery_boot) {
		printf("sd boot: try recovery boot from fat mmc 1:%d\n",
		       SD_BOOT_RECOVERY_PART);
		ret = load_sd_boot_image("fatload", SD_BOOT_RECOVERY_PART,
					 SD_BOOT_RECOVERY_KERNEL, img_addr,
					 kernel_image_size);
		if (ret != 0) {
			printf("sd boot: recovery kernel load failed\n");
			return -1;
		}

		ret = load_sd_boot_image("fatload", SD_BOOT_RECOVERY_PART,
					 SD_BOOT_RECOVERY_DTB, dtb_addr,
					 dtb_image_size);
		if (ret != 0) {
			printf("sd boot: recovery dtb load failed\n");
			return -1;
		}
	}

	if (final_recovery_boot)
		*final_recovery_boot = recovery_boot ? 1 : 0;

	return 0;
}
// ### SIPEED EDIT END ###

int do_sd_boot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char boot_cmd[50];
	int cnt, last_size;
	int j;
	struct img_header *boot_img_header = NULL;
	// ### SIPEED EDIT ###
	int recovery_boot = 0;
	int final_recovery_boot = 0;
	// ### SIPEED EDIT END ###
#if defined(CONFIG_CMD_AXERA_GZIPD) || defined(CONFIG_CMD_AXERA_KERNEL_LZMA)
	char *img_addr = (char *)KERNEL_IMAGE_COMPRESSED_ADDR;
	char *dtb_addr = (char *)DTB_IMAGE_COMPRESSED_ADDR;
	u64 kernel_image_size;
	u64 dtb_image_size;
#else
	char *img_addr = (char *)KERNEL_IMAGE_ADDR;
	char *dtb_addr = (char *)DTB_IMAGE_ADDR;
#endif
	int dtb_size;

	if (boot_info_data.mode != SD_BOOT_MODE)
		return 0;

	printf("now enter sd boot\n");

	env_set("bootargs", BOOTARGS_SD);
	// ### SIPEED EDIT ###
	config_console_for_nanoagent();
	recovery_boot = check_and_config_upgrade();

	if (is_nanokvm_go_board()) {
#if defined(CONFIG_CMD_AXERA_GZIPD) || defined(CONFIG_CMD_AXERA_KERNEL_LZMA)
		if (load_nanokvm_go_boot_images(recovery_boot, img_addr, dtb_addr,
							&kernel_image_size,
							&dtb_image_size,
							&final_recovery_boot) != 0) {
			return -1;
		}
#else
		if (load_nanokvm_go_boot_images(recovery_boot, img_addr, dtb_addr,
							NULL, NULL,
							&final_recovery_boot) != 0) {
			return -1;
		}
#endif
		sd_boot_config_source = final_recovery_boot ?
			SD_BOOT_CONFIG_DEFAULT : SD_BOOT_CONFIG_P2_EXT4;
		if (final_recovery_boot)
			printf("sd boot: recovery boot uses default configs\n");
		config_system_console();
		config_cmm_size();
		goto sd_boot_images_loaded;
	}

	sd_boot_config_source = SD_BOOT_CONFIG_P1_FAT;
	config_system_console();
	config_cmm_size();
	// ### SIPEED EDIT END ###

	sprintf(boot_cmd, "fatload mmc 1:1 0x%x kernel.img 0x%x", SD_BOOT_IMAGE_ADDR, SECBOOT_HEADER_SIZE);
	run_command_list(boot_cmd, -1, 0);

	boot_img_header = (struct img_header *)SD_BOOT_IMAGE_ADDR;
	cnt = boot_img_header->img_size / READ_IMG_SIZE;
	last_size = boot_img_header->img_size % READ_IMG_SIZE;
	j = 0;
#if defined(CONFIG_CMD_AXERA_GZIPD) || defined(CONFIG_CMD_AXERA_KERNEL_LZMA)
	kernel_image_size = boot_img_header->img_size;
#endif
	printf("kernel size is %d bytes\n", boot_img_header->img_size);

	memset( (void *)img_addr, 0, boot_img_header->img_size);

	while (cnt > 0 && cnt--) {
		memset(boot_cmd, 0, sizeof(boot_cmd));
		memset( (void *)SD_BOOT_IMAGE_ADDR, 0, READ_IMG_SIZE);
		sprintf(boot_cmd, "fatload mmc 1:1 0x%x kernel.img 0x%x 0x%x", SD_BOOT_IMAGE_ADDR, READ_IMG_SIZE, SECBOOT_HEADER_SIZE + READ_IMG_SIZE * j);
		run_command_list(boot_cmd, -1, 0);

		memmove((void *)img_addr + READ_IMG_SIZE * j, (void *)SD_BOOT_IMAGE_ADDR, READ_IMG_SIZE);
		j++;
	}

	if (last_size) {
		memset( (void *)SD_BOOT_IMAGE_ADDR, 0, last_size);
		sprintf(boot_cmd, "fatload mmc 1:1 0x%x kernel.img 0x%x 0x%x", SD_BOOT_IMAGE_ADDR, last_size, SECBOOT_HEADER_SIZE + READ_IMG_SIZE * j);
		run_command_list(boot_cmd, -1, 0);
		memmove((void *)img_addr + READ_IMG_SIZE * j, (void *)SD_BOOT_IMAGE_ADDR, last_size);
	}
	printf("sd boot: kernel img read %d finish\n", j * READ_IMG_SIZE + last_size);

	memset(boot_cmd, 0, sizeof(boot_cmd));
	sprintf(boot_cmd, "fatload mmc 1:1 0x%x dtb.img 0x%x", SD_BOOT_IMAGE_ADDR, SECBOOT_HEADER_SIZE);
	run_command_list(boot_cmd, -1, 0);
	boot_img_header = (struct img_header *)SD_BOOT_IMAGE_ADDR;
	dtb_size = boot_img_header->img_size;
#if defined(CONFIG_CMD_AXERA_GZIPD) || defined(CONFIG_CMD_AXERA_KERNEL_LZMA)
	dtb_image_size = dtb_size;
#endif

	memset(boot_cmd, 0, sizeof(boot_cmd));
	memset( (void *)dtb_addr, 0, dtb_size);
	memset( (void *)SD_BOOT_IMAGE_ADDR, 0, dtb_size);
	sprintf(boot_cmd, "fatload mmc 1:1 0x%x dtb.img ", SD_BOOT_IMAGE_ADDR);
	run_command_list(boot_cmd, -1, 0);
	memmove((void *)dtb_addr, (void *)(SD_BOOT_IMAGE_ADDR + SECBOOT_HEADER_SIZE), dtb_size);
	printf("sd boot: dtb img read %d finish\n", dtb_size);

	// ### SIPEED EDIT ###
sd_boot_images_loaded:
	// ### SIPEED EDIT END ###
#ifdef CONFIG_CMD_AXERA_GZIPD
	flush_dcache_all();
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

#ifdef CONFIG_CMD_AXERA_KERNEL_LZMA
	printf("unzip kernel...\n");
	flush_dcache_all();
	if (lzma_decompress_image((void *)KERNEL_IMAGE_COMPRESSED_ADDR, (void *)KERNEL_IMAGE_ADDR, kernel_image_size)) {
		pr_err("kernel image decompress failed\n");
		return -1;
	}

	if (gzip_decompress_image((void *)DTB_IMAGE_COMPRESSED_ADDR,  (void *)DTB_IMAGE_ADDR, dtb_image_size)) {
		pr_err("dtb image decompress failed\n");
		return -1;
	}
	invalidate_dcache_all();
#endif

	memset(boot_cmd, 0, sizeof(boot_cmd));
#ifdef CONFIG_ARM64
	printf("boot arm64 Image kernel\n");
	sprintf(boot_cmd, "booti 0x%lx - 0x%lx", (unsigned long)KERNEL_IMAGE_ADDR, (unsigned long)DTB_IMAGE_ADDR);
#else
	printf("boot arm32 Image kernel\n");
	ax_boot_kernel((void *)(unsigned long)KERNEL_IMAGE_ADDR, (void *)(unsigned long)DTB_IMAGE_ADDR);
#endif
	printf("boot cmd is: %s\n", boot_cmd);
	run_command_list(boot_cmd, -1, 0);

	return 0;
}

U_BOOT_CMD(sd_boot, 1, 0, do_sd_boot,
	   "sd boot", "axera enter sd boot mode\n" "it is used for sd boot to kernel\n");
