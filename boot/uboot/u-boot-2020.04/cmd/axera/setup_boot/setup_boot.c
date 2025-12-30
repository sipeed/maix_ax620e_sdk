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
#include "../../legacy-mtd-utils.h"
#include "axera_update.h"
#include <mmc.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <linux/compat.h>
#include <asm/io.h>
#include <mapmem.h>
#include <part.h>
#include <fat.h>
#include <fs.h>
#include <rtc.h>
#include <linux/time.h>
#include <elf.h>
#ifdef CONFIG_AXERA_SUPPORT_MEMORY_DUMP
#include "memory_dump/memory_dump.h"
#endif

extern struct boot_mode_info boot_info_data;
extern boot_mode_info_t *get_dl_and_boot_info(void);
extern void set_wdt0_timeout(u32 time);
extern u32 dump_reason;

#ifdef CONFIG_CMD_AXERA_SDUPDATE
static boot_mode_t sd_update_mode(void)
{
	struct blk_desc *sd_desc = NULL;
	char *update_status = env_get("sdupdate");

	sd_desc = blk_get_dev("mmc", SD_DEV_ID);
	if (NULL == sd_desc) {
		printf("no sd card\n");
		return CMD_UNDEFINED_MODE;
	}

	if ((boot_info_data.mode == NORMAL_BOOT_MODE) || (boot_info_data.mode == SD_BOOT_MODE)) {
		if ((update_status == NULL) || !strcmp(update_status, "retry")) {
			env_set("bootcmd", "sd_update");
			printf("env sdupdate is %s, enter sd update mode\n", update_status);
			boot_info_data.mode = SD_UPDATE_MODE;
			wdt0_enable(0);
			return SD_UPDATE_MODE;
		} else if (!strcmp(update_status, "fail")) {
			printf("sd update failed twice, need check\n");
		}
	}

	return CMD_UNDEFINED_MODE;
}
#endif

static boot_mode_t usb_update_mode(void)
{
	if (boot_info_data.mode == USB_UPDATE_MODE) {
		env_set("bootdelay", "0");
		env_set("bootcmd", "download");
		printf("enter usb download mode\n");
		wdt0_enable(0);
		return USB_UPDATE_MODE;
	}
	return CMD_UNDEFINED_MODE;
}

static boot_mode_t uart_update_mode(void)
{
	if (boot_info_data.mode == UART_UPDATE_MODE) {
		env_set("bootdelay", "0");
		env_set("bootcmd", "download");
		printf("enter uart download mode\n");
		wdt0_enable(0);
		return UART_UPDATE_MODE;
	}
	return CMD_UNDEFINED_MODE;
}

#ifdef CONFIG_CMD_AXERA_TFTP_OTA
static boot_mode_t tftp_update_mode(void)
{
	if ((boot_info_data.mode == NORMAL_BOOT_MODE) && (NULL != env_get("ota_ready"))) {
		printf("env ota_ready=%s\n", env_get("ota_ready"));
		if (!strcmp(env_get("ota_ready"), "true")) {
			env_set("bootcmd", "axera_ota");
			printf("enter tftp ota update\n");
			wdt0_enable(0);
			boot_info_data.mode = TFTP_UPDATE_MODE;
			return TFTP_UPDATE_MODE;
		} else if (!strcmp(env_get("ota_ready"), "retry")) {
			env_set("bootcmd", "axera_ota");
			printf("retry tftp ota update\n");
			wdt0_enable(0);
			boot_info_data.mode = TFTP_UPDATE_MODE;
			return TFTP_UPDATE_MODE;
		}
	}
	return CMD_UNDEFINED_MODE;
}
#endif

#ifdef CONFIG_CMD_AXERA_SDBOOT
static boot_mode_t sd_boot_mode(void)
{
	if (boot_info_data.mode == SD_BOOT_MODE) {
		env_set("bootcmd", "sd_boot");
		printf("enter sd boot mode\n");
		return SD_BOOT_MODE;
	}
	return CMD_UNDEFINED_MODE;
}
#endif

#ifdef CONFIG_CMD_AXERA_USB_STOR_UPDATE
static boot_mode_t usb_stor_mode(void)
{
	char * update_status = env_get("usbupdate");

	if ((boot_info_data.mode == NORMAL_BOOT_MODE) && (NULL != update_status)) {
		if (!strcmp(update_status, "ready") || !strcmp(update_status, "retry")) {
			struct blk_desc *usb_stor_desc = NULL;
			int ret;

			ret = run_command("usb start", 0);
			if (ret) {
				printf("usb start error %d\n", ret);
				return CMD_UNDEFINED_MODE;
			}

			usb_stor_desc = blk_get_dev("usb", 0);
			if (NULL == usb_stor_desc) {
				printf("usb-storage is not present\n");
				return CMD_UNDEFINED_MODE;
			}

			printf("usb-storage is present\n");
			env_set("bootcmd", "usb_storage_update");
			wdt0_enable(0);
			return USB_STOR_MODE;
		}
	}
    return CMD_UNDEFINED_MODE;
}
#endif

static s_boot_func_array boot_func_array[BOOTMODE_FUN_NUM] = {
#if defined CONFIG_AXERA_SUPPORT_MEMORY_DUMP
	sysdump_mode,
#endif
#ifdef CONFIG_CMD_AXERA_SDUPDATE
	sd_update_mode,
#endif
	usb_update_mode,
	uart_update_mode,
#ifdef CONFIG_CMD_AXERA_TFTP_OTA
	tftp_update_mode,
#endif
#ifdef CONFIG_CMD_AXERA_USB_STOR_UPDATE
	usb_stor_mode,
#endif
#ifdef CONFIG_CMD_AXERA_SDBOOT
	sd_boot_mode,
#endif
	0,
};

static void update_cmdline(void)
{
	char *boot_reason_cmd = NULL;
	char *board_id_cmd = NULL;
	char * bootargs;
	misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;
	int board_id = misc_info->phy_board_id;
	u32 boot_reason = dump_reason;

	bootargs = env_get("bootargs");
	if(NULL == bootargs)
		return;

	boot_reason_cmd = strstr(bootargs, "boot_reason");
	if(!boot_reason_cmd) {
		pr_err("boot_reason not find bootargs\n");
		return;
	}
	if (boot_reason >= 0 && boot_reason <= 9) {
		boot_reason_cmd[15] = boot_reason + '0';
	}
	if (boot_reason == 0x10) {
		boot_reason_cmd[14] = 1 + '0';
		boot_reason_cmd[15] = '0';
	}

	board_id_cmd = strstr(bootargs, "board_id");
	if(!board_id_cmd) {
		pr_err("board_id not find bootargs\n");
		return;
	}
	if (board_id >= 0 && board_id <= 9) {
		board_id_cmd[11] = board_id + '0';
	}
	if (board_id >= 10 && board_id <= 15) {
		board_id_cmd[11] = board_id + 'W';
	}
	env_set("bootargs", bootargs);
	env_save();
}

int setup_boot_mode(void)
{
	int i = 0;
	boot_mode_t boot_mode;
	struct boot_mode_info *const boot_info = get_dl_and_boot_info();
	if (boot_info->magic != BOOT_MODE_ENV_MAGIC) {
		printf("boot_mode magic error\n");
		return -1;
	}
	memcpy(&boot_info_data, boot_info, sizeof(boot_mode_info_t));

	for (i = 0; i < BOOTMODE_FUN_NUM - 1; i++) {
		if (0 == boot_func_array[i]) {
			#if defined CONFIG_SUPPORT_RECOVERY || !defined CONFIG_BOOT_OPTIMIZATION_SUPPORT
				env_set("bootcmd", "axera_boot");
				printf("enter normal boot mode\n");
			#else
				/* #define BOOT_KERNEL_FAIL  BIT(7)
				* #define BOOT_DOWNLOAD     BIT(8)
				* #define BOOT_RECOVERY     BIT(11)
				* Check whether bit7 and bit8 are 1 */
				writel(BOOT_KERNEL_FAIL | BOOT_DOWNLOAD, TOP_CHIPMODE_GLB_BACKUP0_CLR);
				set_wdt0_timeout(180);
				printf("===============================###############=========================================\n");
				printf(">>>>>> Go to the command line and wait for the upgrade (TF card or TFTP) ....... <<<<<<\n");
				printf("===============================###############==========================================\n");
				env_set("bootcmd", "help");
			#endif
			break;
		}
		boot_mode = boot_func_array[i] ();
		if (CMD_UNDEFINED_MODE == boot_mode) {
			continue;
		} else {
			printf("get boot mode in boot func array[%d]\n", i);
			break;
		}
	}
	update_cmdline();
	printf("boot_info_data.mode = %d\n", boot_info_data.mode);

	return 0;
}
