
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

static int fat_prepare(void)
{
	struct blk_desc *sd_desc = NULL;
	char *mmc_type = NULL;
	struct udevice *dev;

	printf("[info] sd  memory dumping ...\n");
	/* check sd is present */
	sd_desc = blk_get_dev("mmc", SD_DEV_ID);
	if (NULL == sd_desc) {
		printf("[error] memory dump sd is not present, exit sd update\n");
		return -1;
	}

	/* we register fat device */
	if (fat_register_device(sd_desc, 1)) {
		printf("[error] sd: no part1 found, check sd card!\n");
		return -1;
	}

	for (uclass_first_device(UCLASS_MMC, &dev); dev; uclass_next_device(&dev)) {
		struct mmc *m = mmc_get_mmc_dev(dev);
		if (m->has_init) {
			mmc_type = IS_SD(m) ? "SD" : "eMMC";
		} else {
			mmc_type = "NONE";
		}

		if(!strcmp(mmc_type,"SD")) {
			break;
		}

	}

	if(strcmp(mmc_type, "SD")) {
		pr_err("[error] SD card no found, memory dump failed\n");
		return -1;
	}

	return 0;
}

static int sd_save_mem2file(unsigned long int addr,
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
		ret = fat_prepare();
		if (ret == 0) {
			ret = ax_dump_memory(sd_save_mem2file);
			if (ret < 0)
				printf("[error] axera memoey dump emmc failed\n");
			fat_close();
		}
	}
	wdt0_enable(1);

	return CMD_UNDEFINED_MODE;
}
