
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
#include <rtc.h>
#include <asm/io.h>
#include <asm/arch-axera/boot_mode.h>
#include "memory_dump.h"

unsigned long s_dump_info_paddr;
unsigned long s_dump_info_sz;
struct axera_dump_info axera_dump_info;

static void convert_dumpinfo2uboot_mode(struct axera_dump_info *in, struct axera_dump_info *out)
{
	if (!in || !out)
		return;
	out->info_paddr       = in->info_paddr;
	out->info_size        = in->info_size;
	out->info_vaddr       = in->info_vaddr;
	out->kernel_paddr     = in->kernel_paddr;
	out->kernel_vaddr     = in->kernel_vaddr;
	out->kmem_size        = in->kmem_size;
	out->magic_addr       = in->info_paddr + (in->magic_addr - in->info_vaddr);
	out->time_addr        = in->info_paddr + (in->time_addr - in->info_vaddr);
	out->dump_ranges_addr = in->info_paddr + (in->dump_ranges_addr - in->info_vaddr);
	out->mmu_addr         = in->info_paddr + (in->mmu_addr - in->info_vaddr);
	out->pt_note_addr     = in->info_paddr + (in->pt_note_addr - in->info_vaddr);
	out->cpu_id           = in->cpu_id;
	out->dump_cnt         = in->dump_cnt;
}

static int get_meminfo_from_fdt(const char*fdt, unsigned long *info_paddr, unsigned long *info_sz)
{
	unsigned long addr, size;
	int offset, len;
	const u32 *val;

	if (fdt_check_header(fdt)) {
		printf("[error] Invalid device tree header\n");
		return -1;
	}

	offset = fdt_path_offset(fdt, "/reserved-memory/axera_memory_dump@0");
	if (offset < 0){
		printf("[error] reserved_mem error \n");
	}

	val = fdt_getprop(fdt, offset, "reg", &len);
	if (val == NULL) {
		printf("[error] get prop val failed!\n");
		return -1;
	}
#ifdef CONFIG_ARM64
	addr = fdt32_to_cpu(val[0]);
	addr = addr << 32;
	addr |= fdt32_to_cpu(val[1]);

	size = fdt32_to_cpu(val[2]);
	size = size << 32;
	size |= fdt32_to_cpu(val[3]);
#else
	addr = fdt32_to_cpu(val[1]);
	size = fdt32_to_cpu(val[3]);
#endif
	*info_paddr = addr;
	*info_sz    = size;

	return 0;
}

static int get_dumpinfo(const char* fdt, struct axera_dump_info *dump_info)
{
	int offset, len;
	u32 reason_mask;
	const u32 *val;
	int cnt, i;
	u32 *tmp;
	reason_mask = readl(dump_info->magic_addr);
	if (reason_mask != AXERA_REASON_MASK)
		return -1;
	if (reason_mask != AXERA_REASON_MASK){
		if (fdt_check_header(fdt)) {
			printf("[error] Invalid device tree header\n");
			return -1;
		}
		offset = fdt_path_offset(fdt, "/reserved-memory/axera_memory_dump@0");
		if (offset < 0)
			printf("[error] reserved_mem error \n");
		val = fdt_getprop(fdt, offset, "dump_ranges", &len);
		if (val == NULL) {
			printf("[error] get dump_ranges failed! len=%d\n", len);
			return -ENODEV;
		}
		cnt = len / 4*sizeof(int);
		dump_info->dump_cnt = cnt;
		tmp = (u32*)dump_info->dump_ranges_addr;
		for (i=0; i < cnt; i++){
			*tmp++ = fdt32_to_cpu(val[i]);
			*tmp++ = fdt32_to_cpu(val[i+1]);
			*tmp++ = fdt32_to_cpu(val[i+2]);
			*tmp++ = fdt32_to_cpu(val[i+3]);
		}
	}

	return 0;
}

static int  get_dump_reason_mask(struct axera_dump_info *axera_dump_info)
{
	int reason_mask;
	reason_mask = readl(axera_dump_info->magic_addr);
	return reason_mask;
}

static void  clear_boot_reason_mask(struct axera_dump_info  *axera_dump_info)
{
	writel(0x0, (axera_dump_info->magic_addr));
}

static char* get_crash_time(struct axera_dump_info *axera_dump_info, char *timeptr, const int len)
{
	struct rtc_time tm;
	memcpy((void *)&tm,(void *)(axera_dump_info->time_addr),sizeof(tm));
	snprintf(timeptr, len, "%4d%02d%02d%02d%02d%02d",tm.tm_year+1900,tm.tm_mon, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
	return timeptr;
}

int ax_get_boot_reason_mask(void)
{
	int ret;
	u32 reason_mask;
	struct axera_dump_info *dumpInfoPtr = NULL;

	ret = get_meminfo_from_fdt((char *)DTB_IMAGE_ADDR, &s_dump_info_paddr, &s_dump_info_sz);

	if(ret != 0){
		printf("[error] get_meminfo_from_fdt failed: %d\n", ret);
		return -1;
	}

	dumpInfoPtr = (struct axera_dump_info *)ALIGN_UP_16(s_dump_info_paddr + DUMPINFO_OFFSET);
	convert_dumpinfo2uboot_mode(dumpInfoPtr, &axera_dump_info);
	reason_mask = get_dump_reason_mask(&axera_dump_info);

	return reason_mask;
}

int ax_dump_memory(SaveMemToFileFunc saveFunc)
{
	int ret;
	char crash_time[64] = {0};
	char dump_file_name[DUMP_FILE_NAME_SIZE] = {0};
	int cnt, i;
	u32 *tmp;
	unsigned long start_addr, end_addr;

	printf("[info] memory dumping ...\n");
	ret = get_dumpinfo((char *)DTB_IMAGE_ADDR, &axera_dump_info);
	if (ret < 0) {
		printf("[error] get_dumpinfo failed: %d\n", ret);
		return -1;
	}
	get_crash_time(&axera_dump_info, (char*)&crash_time, sizeof(crash_time)/sizeof(char));
	cnt = axera_dump_info.dump_cnt;
	tmp = (u32*)axera_dump_info.dump_ranges_addr;
	memset(dump_file_name, 0, sizeof(dump_file_name));
	snprintf(dump_file_name, DUMP_FILE_NAME_SIZE, "/vmcore.info.%s.bin", crash_time);
	saveFunc(axera_dump_info.info_paddr, axera_dump_info.info_size, dump_file_name);
	for(i = 0; i < cnt; i++){
#ifdef CONFIG_ARM64
		start_addr = *tmp++;
		start_addr = start_addr << 32;
		start_addr |= *tmp++;
		end_addr   = *tmp++;
		end_addr   = end_addr << 32;
		end_addr  |= *tmp++;
#else
		tmp++;
		start_addr = *tmp++;
		tmp++;
		end_addr   = *tmp++;
#endif
		memset(dump_file_name, 0, sizeof(dump_file_name));
		snprintf(dump_file_name, DUMP_FILE_NAME_SIZE, "/vmcore.dump.%s_%lx_%lx.bin", crash_time, start_addr, end_addr);
		saveFunc(start_addr,end_addr - start_addr,dump_file_name);
	}
	printf("[info] memory dump Done!!!\n");
	clear_boot_reason_mask(&axera_dump_info);

	return 0;
}
