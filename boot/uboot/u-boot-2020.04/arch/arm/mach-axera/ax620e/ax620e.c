/*
 * Copyright (c) 2023 AXERA in AX620E project.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <sdhci.h>
#include <malloc.h>
#include <asm/arch/ax620e.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch-axera/dma.h>
#include <fat.h>
#include <string.h>
#include <ubi_uboot.h>
#include <ubifs_uboot.h>

#ifdef CONFIG_ARM64
static struct mm_region ax620e_mem_map[] = {
	{
#ifdef CONFIG_AXERA_AX630C_DDR4_RETRAIN
		.virt = 0x40001000UL,
		.phys = 0x40001000UL,
		.size = MEM_REGION_DDR_SIZE - 0x1000,
#else
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = MEM_REGION_DDR_SIZE,
#endif
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
                        PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x00000000UL,
		.phys = 0x00000000UL,
		.size = 0x10500000UL,//寄存器空间
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	},{
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = ax620e_mem_map;
#endif

int arch_cpu_init(void)
{
	/* We do some SoC one time setting here. */
	axi_dma_hw_init();
	return 0;
}

struct boot_mode_info boot_info_data;

u32 get_boot_voltage(void)
{
	u32 val;
	val = readl(PIN_MUX_G11_VDET_RO0);
	/* 0: 3.3V   1: 1.8V */
	if (((val >> 0) & BIT(0))) { //3.3v
		return 0;
	} else { //1.8v
		return 1;
	}
}

boot_mode_info_t *get_dl_and_boot_info(void)
{
	boot_mode_info_t *boot_mode = (boot_mode_info_t *) BOOT_MODE_INFO_ADDR;
	printf("boot_mode->magic = 0x%x\n", boot_mode->magic);
	printf("boot_mode->dl_channel = %d\n", boot_mode->dl_channel);
	printf("boot_mode->storage_sel = %d\n", boot_mode->storage_sel);
	printf("boot_mode->boot_type = %d\n", boot_mode->boot_type);

	boot_mode->mode = NORMAL_BOOT_MODE;

	if (boot_mode->dl_channel == DL_CHAN_UART1
	    || boot_mode->dl_channel == DL_CHAN_UART0) {
		boot_mode->mode = UART_UPDATE_MODE;
	}
	if (boot_mode->dl_channel == DL_CHAN_USB) {
		boot_mode->mode = USB_UPDATE_MODE;
	}
	if (boot_mode->is_sd_boot == true) {
		boot_mode->mode = SD_BOOT_MODE;
	}

	return boot_mode;
}

// console ,download, coredump to disable wtd0
void wdt0_enable(bool enable)
{
	/* set wdt timeout clk to 30S */
	writel((30 * WDT0_CLK_FREQ) >> 16, (void *)(WDT0_TORR_ADDR));
	writel(0x1, (void *)(WDT0_TORR_START_ADDR));
	writel(0x0, (void *)(WDT0_TORR_START_ADDR));

	/* set wdt0 clk source to 24MHz */
	writel(BIT(19), (void *)(PERI_SYS_GLB_CLK_MUX0_SET));

	if (enable) {
		writel(1, WDT0_BASE);
	} else {
		writel(0, WDT0_BASE);
	}
}

void set_wdt0_timeout(u32 time)
{
	u32 time_temp = time / 2;
	/* set wdt timeout clk to time_temp */
	writel((time_temp * WDT0_CLK_FREQ) >> 16, (void *)(WDT0_TORR_ADDR));
	writel(0x1, (void *)(WDT0_TORR_START_ADDR));
	writel(0x0, (void *)(WDT0_TORR_START_ADDR));

	/* set wdt0 clk source to 24MHz */
	writel(BIT(19), (void *)(PERI_SYS_GLB_CLK_MUX0_SET));

	writel(1, WDT0_BASE);
}

#if 0
void reboot(void)
{
	wdt0_enable(true);
	printf("trigger watchdog, reboot now ...\n");
	while (1) ;
}
#else
static void chip_rst_sw(void)
{
	u32 tmp;

	tmp = readl(TOP_CHIPMODE_GLB_BACKUP0);
	tmp &= ~(BOOT_DOWNLOAD | BOOT_KERNEL_FAIL);
	writel(tmp, (void *)TOP_CHIPMODE_GLB_BACKUP0);

	tmp = readl((void *)COMM_ABORT_CFG);
	tmp |= CHIP_RST_SW;
	printf("Set REG 0x%X, value 0x%X\n", COMM_ABORT_CFG, tmp);
	writel(tmp, (void *)COMM_ABORT_CFG);
}

void reboot(void)
{
	chip_rst_sw();
	printf("trigger chip_rst_sw ...\n");
	while (1) ;
}
#endif

#ifndef CONFIG_SYSRESET
void reset_cpu(unsigned long ignored)
{
	reboot();
}
#endif

u32 dump_reason;
u32 axera_get_boot_reason(void)
{
	u32 abort_cfg;
	u32 abort_status;
	abort_status = readl(COMM_ABORT_STATUS);
	dump_reason = abort_status;
	/*clear abort alarm status */
	abort_cfg =
	    ABORT_WDT0_CLR | ABORT_WDT2_CLR | ABORT_THM_CLR | ABORT_SWRST_CLR;
	writel(abort_cfg, COMM_ABORT_CFG);
	/*enable watchdog, thermal abort function */
	abort_cfg = ABORT_WDT2_EN | ABORT_WDT0_EN | ABORT_THM_EN;
	writel(abort_cfg, COMM_ABORT_CFG);
	return abort_status;
}

static void print_boot_reason(void)
{
	u32 tmp;
	tmp = axera_get_boot_reason();
	printf("boot_reason:0x%x\n",tmp);
	if (tmp & (1 << 4)) {
		printf("wdt2 reset\n");
	}
	if (tmp & (1 << 2)) {
		printf("wdt0 reset\n");
	}
	if (tmp & (1 << 1)) {
		printf("thm reset\n");
	}
	if (tmp & (1 << 0)) {
		printf("swrst reset\n");
	}
}

static void print_board_info(void)
{
	print_chip_type();
	print_board_id();
}
#ifdef CONFIG_SUPPORT_AB
static int set_slot_ab(void)
{
	u32 slottype = 0;

	slottype = readl(TOP_CHIPMODE_GLB_BACKUP0);

	if (slottype & SLOTA) {
		env_set("bootsystem", "A");
		printf("From slota boot\n");
	}
	if (slottype & SLOTB) {
		env_set("bootsystem", "B");
		printf("From slotb boot\n");
	}
	env_save();
	return 0;
}
#endif

#ifdef CONFIG_SUPPORT_RECOVERY
static int set_recovery(void)
{
	u32 bootable = 0;

	bootable = readl(TOP_CHIPMODE_GLB_BACKUP0);
	if ((bootable & BOOT_KERNEL_FAIL) || (bootable & BOOT_RECOVERY)) {
		env_set("bootable", "recovery");
	}
	env_save();
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
// ### SIPEED EDIT ###
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

static bool value_is_exact_zero(char *value)
{
	char *end;

	while (*value == ' ' || *value == '\t') {
		value++;
	}

	end = value + strlen(value);
	while (end > value && (*(end - 1) == ' ' || *(end - 1) == '\t' || *(end - 1) == '\r')) {
		end--;
	}

	return end == value + 1 && *value == '0';
}

static int parse_cmm_config(char *buffer, int *cmm_size) {
	const char *key = "maix_memory_cmm";
	char *line = strtok(buffer, "\n");

	while (line != NULL) {
		char *key_value_str = strstr(line, key);
		if (key_value_str != NULL) {
			bool key_is_valid = true;

			for (char *p = line; p < key_value_str; p++) {
				if (*p == '#') {
					key_is_valid = false;
					printf("[info] maix_memory_cmm is invalid\r\n");
					break;
				}
			}

			if (key_is_valid) {
				char *value_str = strstr(key_value_str, "=");
				if (value_str != NULL) {
					int value = __atoi(value_str + 1);
					if (cmm_size) {
						*cmm_size = value;
					}
					return 0;
				}
			}
		}
		line = strtok(NULL, "\n");
	}

	return -1;
}

static void parse_uboot_autoboot_key_config(char *buffer)
{
	const char *key = "maix_uboot_autoboot_key";
	char *line = strtok(buffer, "\n");

	while (line != NULL) {
		char *key_value_str = strstr(line, key);
		if (key_value_str != NULL) {
			bool key_is_valid = true;

			for (char *p = line; p < key_value_str; p++) {
				if (*p == '#') {
					key_is_valid = false;
					printf("[info] maix_uboot_autoboot_key is invalid\r\n");
					break;
				}
			}

			if (key_is_valid) {
				char *value_str = strstr(key_value_str, "=");
				if (value_str != NULL && value_is_exact_zero(value_str + 1)) {
					printf("disable U-Boot autoboot key check\r\n");
					env_set("bootdelay", "-2");
				}
			}
			break;
		}
		line = strtok(NULL, "\n");
	}
}

extern int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t *info);
static int read_cmm_size_from_boot(int *cmm_size) {
	struct blk_desc *mmc_desc = NULL;
	char *partition = "boot";
	char *filename = "configs";
	disk_partition_t fs_partition;

	bool mmc_ready = false, nand_ready = false;
	char *buffer = NULL;
	int ret = -1;

	mmc_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (mmc_desc == NULL) {
		mmc_desc = blk_get_dev("mmc", SD_DEV_ID);
	}

	if (mmc_desc != NULL && get_part_info(mmc_desc, partition, &fs_partition) == 0) {
		if (fat_set_blk_dev(mmc_desc, &fs_partition) == 0) {
			mmc_ready = true;
		} else {
			mmc_desc = blk_get_dev("mmc", SD_DEV_ID);
			if (mmc_desc != NULL && fat_register_device(mmc_desc, 1) == 0) {
				mmc_ready = true;
			}
		}
	}

	if (!mmc_ready) {
		printf("[info] mmc is not ready, trying to load %s from NAND UBIFS\n", filename);
		if (ubi_part("rootfs", NULL) == 0) {
			ubifs_init();
			if (uboot_ubifs_mount("ubi0:bootfs") == 0) {
				nand_ready = true;
			} else {
				printf("[error] mount ubi0:bootfs failed\n");
			}
		} else {
			printf("[error] ubi_part rootfs failed\n");
		}
	}

	if (mmc_ready) {
		if (!fat_exists(filename)) {
			printf("[info] file does not exist on FAT\n");
			return -1;
		}

		int buffer_size = 15 * 1024 * 1024;
		buffer = malloc(buffer_size);
		if (buffer == NULL) {
			printf("[error] malloc buffer failed\n");
			return -1;
		}

		if (file_fat_read(filename, buffer, buffer_size) <= 0) {
			printf("[error] file_fat_read failed\n");
			free(buffer);
			return -1;
		}

	} else if (nand_ready) {
		if (!ubifs_exists(filename)) {
			printf("[info] file does not exist on UBIFS\n");
			uboot_ubifs_umount();
			return -1;
		}

		loff_t file_size, actread;
		ubifs_size(filename, &file_size);

		buffer = malloc(file_size + 1);
		if (buffer == NULL) {
			printf("[error] malloc buffer failed\n");
			uboot_ubifs_umount();
			return -1;
		}

		if (ubifs_read(filename, buffer, 0, file_size, &actread) == 0) {
			buffer[actread] = '\0';
		} else {
			printf("[error] ubifs_read failed\n");
			free(buffer);
			uboot_ubifs_umount();
			return -1;
		}
		uboot_ubifs_umount();

	} else {
		printf("[error] memory dump: emmc/sd/nand is not present, exit dump!\n");
		return -1;
	}

	if (buffer != NULL) {
		ret = parse_cmm_config(buffer, cmm_size);
		if (ret != 0) {
			printf("[error] maix_memory_cmm parse failed\n");
		}
		free(buffer);
	}

	return ret;
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
	case PHY_AX620Q_LP4_NANOAGENT_256M:
		mmc_max_size = 256;
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

int read_file_from_boot(const char* filename, char* buffer, int bufsize)
{
	struct blk_desc* mmc_desc = NULL;
	const char* partition = "boot";
	disk_partition_t fs_partition;
	int ret;

	if (!filename || !buffer || bufsize <= 1) {
		printf("[error] invalid argument\n");
		return -EINVAL;
	}

	mmc_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (!mmc_desc) {
		printf("[warn] emmc not present, try sd mmc 1:1\n");
	} else {
		ret = get_part_info(mmc_desc, partition, &fs_partition);
		if (ret < 0) {
			printf("[warn] get %s partition failed, ret=%d, try sd mmc 1:1\n",
			       partition, ret);
		} else {
			ret = fat_set_blk_dev(mmc_desc, &fs_partition);
			if (ret != 0) {
				printf("[warn] fat_set_blk_dev failed, ret=%d, try sd mmc 1:1\n",
				       ret);
			} else {
				goto read_file;
			}
		}
	}

	mmc_desc = blk_get_dev("mmc", SD_DEV_ID);
	if (!mmc_desc) {
		printf("[error] emmc boot unavailable and sd mmc 1:1 unavailable\n");
		return -ENODEV;
	}

	ret = fat_register_device(mmc_desc, 1);
	if (ret != 0) {
		printf("[error] sd fat_register_device failed, ret=%d\n", ret);
		return ret;
	}

read_file:
	if (!fat_exists(filename)) {
		printf("[error] file %s not exist\n", filename);
		return -ENOENT;
	}

	int read_size = file_fat_read(filename, buffer, bufsize - 1);
	if (read_size <= 0) {
		printf("[error] file_fat_read failed, ret=%d\n", read_size);
		return -EIO;
	}
	buffer[min(read_size, bufsize - 1)] = '\0';

	return read_size;
}

int read_int_from_boot(const char* filename, int* ovalue)
{
	if (!ovalue) {
		printf("[error] invalid argument\n");
		return -EINVAL;
	}

	char tmp_buf[512];
	int ret = read_file_from_boot(filename, tmp_buf, sizeof(tmp_buf));
	if (ret < 0)
		return ret;

	char* p = tmp_buf;
	while (*p && ((*p <= 0x20) || (*p == 0x7F)))
		p++;

	if (*p == '-' || (*p >= '0' && *p <= '9'))
		*ovalue = simple_strtol(p, NULL, 10);
	else {
		printf("[warn] no valid number found in %s\n", filename);
		*ovalue = -1;
	}

	return 0;
}

int read_string_from_boot(const char *filename, char *buffer, int buffer_len)
{
	char tmp_buf[512];
	int ret = read_file_from_boot(filename, tmp_buf, sizeof(tmp_buf));
	if (ret < 0)
		return ret;

	char *p = tmp_buf;
	while (*p && ((*p <= 0x20) || (*p == 0x7F)))
		p++;

	memset(buffer, '\0', buffer_len);
	strncpy(buffer, p, buffer_len - 1);
	buffer[buffer_len - 1] = '\0';
	return 0;
}

static void config_uboot_autoboot_key_from_boot(void)
{
	int buffer_size = 15 * 1024 * 1024;
	char *buffer = malloc(buffer_size);

	if (buffer == NULL) {
		printf("[error] malloc configs buffer failed\n");
		return;
	}

	if (read_file_from_boot("configs", buffer, buffer_size) >= 0) {
		parse_uboot_autoboot_key_config(buffer);
	}

	free(buffer);
}

int read_nanokvm_logo_index_from_boot(int* logo_index)
{
	int ret = 0;
	struct blk_desc *mmc_desc = NULL;
	const char *parttiton = "boot";
	const char *filename = "bootmode";
	disk_partition_t fs_partition;

	mmc_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (NULL == mmc_desc) {
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

	int buffer_size = 4 * 1024; // 4KB
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
	const char *key = "logo";
	char *line = strtok(buffer, "\n");
	while (NULL != line) {
		char *key_value_str = strstr(line, key);
		if (key_value_str != NULL) {
			char *p = line;
			bool key_is_valid = true;
			while (p != key_value_str) {
				if (*p == '#') {
					key_is_valid = false;
					printf("%s is invalid\r\n", key);
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

	if (logo_index) {
		*logo_index = value;
	}
	free(buffer);
	return ret;
}
// ### SIPEED EDIT END ###
int board_late_init(void)
{
#ifdef CONFIG_SUPPORT_AB
	set_slot_ab();
#endif
#ifdef CONFIG_SUPPORT_RECOVERY
	set_recovery();
#endif
	print_board_info();
	wdt0_enable(1);
	print_boot_reason();
#if defined(CONFIG_CMD_AXERA_DOWNLOAD) || defined(CONFIG_CMD_AXERA_BOOT)
	setup_boot_mode();
#endif
	set_ephy_led_pol();

// ### SIPEED EDIT ###
	config_uboot_autoboot_key_from_boot();

	int cmm_size = -1;
	if (0 != read_cmm_size_from_boot(&cmm_size)) {
		cmm_size = -1;	// if failed, use default cmm_size
	}
	printf("user config cmm=%d\n", cmm_size);

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
		case PHY_AX620Q_LP4_NANOAGENT_256M:
			strcpy(new_mem_cfg, BOARD_256M_OS_MEM);
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
		case AX620Q_LP4_DEMO_V1_1:
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
		case PHY_AX620Q_LP4_NANOAGENT_256M:
			os_mem_size = 256 - cmm_size;
			break;
		case PHY_AX620QE_LP4_NANOAGENT_512M:
			os_mem_size = 512 - cmm_size;
			break;
		case PHY_AX620QF_LP4_NANOAGENT_256M:
			os_mem_size = 256 - cmm_size;
			break;
		case AX620Q_LP4_DEMO_V1_1:
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
		if (mem_start) {
			char *mem_end = strchr(mem_start, ' ');
			if (mem_end) {
				int mem_start_offset = mem_start - oldargs;
				memcpy(newargs, oldargs, mem_start_offset);
				memcpy(newargs + mem_start_offset, new_mem_cfg, strlen(new_mem_cfg));
				strcpy(newargs + mem_start_offset + strlen(new_mem_cfg), mem_end);
				env_set("bootargs", newargs);
			} else {
				printf("Get mem end failed, old args: %s\r\n", oldargs);
			}
		} else {
			printf("Get mem from bootargs failed, old args: %s\r\n", oldargs);
		}
	}
// ### SIPEED EDIT END ###
	return 0;
}
#endif
