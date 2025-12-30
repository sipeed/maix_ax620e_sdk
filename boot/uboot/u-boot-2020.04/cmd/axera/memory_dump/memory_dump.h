
/*
 * AXERA AX620E Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef MEMORY_DUMP_H
#define MEMORY_DUMP_H

#include <asm/arch/boot_mode.h>

#define DUMPINFO_OFFSET     1024
#define AXERA_REASON_MASK   0xf98e7c6d
#define DUMP_FILE_NAME_SIZE 128

#define ALIGN_UP_16(value) ((value + 0xF) & (~0xF))

struct axera_dump_info {
	unsigned long info_paddr;       /*dump info phy address*/
	unsigned long info_size;        /*dump info size*/
	unsigned long info_vaddr;       /*dump info virtual address*/
	unsigned long kernel_paddr;     /*kernel start phy address*/
	unsigned long kernel_vaddr;     /*kernel start virtual address*/
	unsigned long kmem_size;        /*kernel mem size*/
	unsigned long dump_info_addr;   /*dump info start address*/
	unsigned long magic_addr;       /*sysdump magic addr*/
	unsigned long time_addr;        /*sysdump time addr*/
	unsigned long dump_ranges_addr; /*dump ranges start address*/
	unsigned long pt_note_addr;     /*pt note address*/
	unsigned long mmu_addr;         /*mmu regs save addr*/
	unsigned int  cpu_id;           /*panic cpu id*/
	unsigned int  dump_cnt;         /*sysdump range count*/
};


typedef int (*SaveMemToFileFunc)(unsigned long int addr,
                                 unsigned long int size,
                                 char *filename);

boot_mode_t sysdump_mode(void);
int ax_get_boot_reason_mask(void);
int ax_dump_memory(SaveMemToFileFunc saveFunc);

#endif /*MEMORY_DUMP_H*/
