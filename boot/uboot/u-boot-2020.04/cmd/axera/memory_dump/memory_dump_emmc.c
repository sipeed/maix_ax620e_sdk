
/*
 * AXERA AX620E Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fs.h>
#include <asm/arch/ax620e.h>
#include <mapmem.h>
#include <elf.h>
#include <asm/arch-axera/boot_mode.h>
#include "memory_dump.h"

extern u32 dump_reason;

static disk_partition_t s_emmc_partition;

int get_part_info(struct blk_desc *dev_desc, const char *name, disk_partition_t * info);
int ext4fs_probe(struct blk_desc *fs_dev_desc, disk_partition_t *fs_partition);
int ext4_write_file(const char *filename, void *buf, loff_t offset, loff_t len, loff_t *actwrite);
void ext4fs_close(void);

static int ext4_prepare(const char* parttiton)
{
	int ret = 0;
	struct blk_desc *emmc_desc;

	emmc_desc = blk_get_dev("mmc", EMMC_DEV_ID);
	if (NULL == emmc_desc) {
		printf("[error] memory dump: emmc is not present, exit dump!\n");
		return -1;
	}
	ret = get_part_info(emmc_desc, parttiton, &s_emmc_partition);
	if(ret < 0) {
		printf("[error] memory dump get %s partition error, ret:%d\n", parttiton, ret);
		return ret;
	}
	ret = ext4fs_probe(emmc_desc, &s_emmc_partition);

	if (ret) {
		printf("[error] memory dump ext4fs_probe failed ret: %d\n", ret);
	}

	return ret;
}

static int ext4_save_mem2file(unsigned long int addr,
                              unsigned long int size,
                              char *filename)
{
	int ret = 0;
	void *buf;
	loff_t offset = 0;
	loff_t actwrite;

	printf("[info] saving %s ...\n", filename);
	buf = map_sysmem(addr, size);
	ret = ext4_write_file(filename, buf, offset, size, &actwrite);
	unmap_sysmem(buf);
	if (ret < 0 && (size != actwrite)) {
		printf("[error] Unable to write file %s ret:%d, size:%ld, actwrite:%lld\n", filename, ret, size, actwrite);
	} else {
		printf("[info] dump %s success!!!\n", filename);
	}

	return ret;
}

boot_mode_t sysdump_mode(void)
{
	int ret;
	int reason_mask;

	wdt0_enable(0);
	if ((dump_reason & 0x1C) && (AXERA_REASON_MASK == (reason_mask = ax_get_boot_reason_mask()))) {
		printf("[info] Sysdump started, dump_reason: %d reason_mask = 0x%x\n", dump_reason, reason_mask);
		ret = ext4_prepare("opt");
		if (ret == 0) {
			ret = ax_dump_memory(ext4_save_mem2file);
			if (ret < 0)
				printf("[error] axera memoey dump emmc failed\n");
			ext4fs_close();
		}
	}
	wdt0_enable(1);

	return CMD_UNDEFINED_MODE;
}
