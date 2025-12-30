
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
#include "memory_dump.h"

extern u32 dump_reason;

static int usb_storage_prepare(void)
{
	struct blk_desc *usb_stor_desc = NULL;

	if (run_command("usb start", 0)) {
		printf("[error] memory dump usb start error");
		return -1;
	}

	/* step1 check usb-storage is present */
	usb_stor_desc = blk_get_dev("usb", 0);
	if (NULL == usb_stor_desc) {
		printf("usb-storage is not present, exit dump\n");
		return -1;
	}

	/* we register usb to fatfs */
	if (fat_register_device(usb_stor_desc, 1)) {
		printf("[warn] memory dump usb-storage register part1 fat fail, try part0\n");

		if (fat_register_device(usb_stor_desc, 0)) { /* in normal condition, part0 is MBR */
			printf("[error] usb-storage register part0 fat fail, exit usb-storage memory dump\n");
			return -1;
		}
	}

	return 0;
}

static int usb_storage_save_mem2file(unsigned long int addr,
                            unsigned long int size,
                            char *filename)
{
	void *buf;
	int ret;
	loff_t offset = 0;
	loff_t actwrite;

	printf("[info] saving %s ...\n", filename);
	buf = map_sysmem(addr, size);
	ret = file_fat_write(filename, buf, offset, size, &actwrite);
	if (ret < 0 && (size != actwrite)) {
		printf("[error] Unable to write file %s ret:%d, size:%ld, actwrite:%lld\n", filename, ret, size, actwrite);
		return -1;
	}
	unmap_sysmem(buf);
	printf("[info] dump %s success!!!\n", filename);

	return 0;
}

boot_mode_t sysdump_mode(void)
{
	int ret;
	int reason_mask;

	wdt0_enable(0);
	if ((dump_reason & 0x1C) && (AXERA_REASON_MASK == (reason_mask = ax_get_boot_reason_mask()))) {
		printf("[info] Sysdump started, dump_reason: %d reason_mask = 0x%x\n", dump_reason, reason_mask);
		ret = usb_storage_prepare();
		if (ret == 0) {
			ret = ax_dump_memory(usb_storage_save_mem2file);
			if (ret < 0)
				printf("[error] axera memoey dump emmc failed\n");
			fat_close();
		}
	}
	wdt0_enable(1);

	return CMD_UNDEFINED_MODE;
}
