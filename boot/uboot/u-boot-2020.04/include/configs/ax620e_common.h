/*
 * (C) Copyright 2023 AXERA
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_AX620E_COMMON_H
#define __CONFIG_AX620E_COMMON_H
#include "ax_common.h"
/*noncached memory size: 1MB*/

#define CONFIG_CMDLINE_TAG

#define CONFIG_SYS_MAXARGS		16

#define CONFIG_SYS_CBSIZE		1024 /* Console I/O Buffer Size */
#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_SYS_INIT_SP_ADDR		0x41000000
#define CONFIG_SYS_LOAD_ADDR		0x41100000

#define CONFIG_SYS_BOOTM_LEN	(64 << 20)	/* 64M */

#define DTB_IMAGE_ADDR                  CONFIG_AXERA_DTB_IMG_ADDR
#define KERNEL_IMAGE_ADDR               CONFIG_AXERA_KERNEL_IMG_ADDR

#define DTB_IMAGE_COMPRESSED_ADDR   	(UBOOT_IMG_HEADER_BASE + 4 * 1024 * 1024)
#define KERNEL_IMAGE_COMPRESSED_ADDR 	(UBOOT_IMG_HEADER_BASE + 5 * 1024 * 1024)
#define SD_BOOT_IMAGE_ADDR          	(UBOOT_IMG_HEADER_BASE + 45 * 1024 * 1024)

#define VO_NR 2
#define AX_VO_DPU0_BASE_ADDR 0x4407000
#define AX_VO_DPU1_BASE_ADDR 0x4408000

#define MM_SYS_GLB_BASE_ADDR 0x4430000
#define COMMON_SYS_GLB_BASE_ADDR 0x2340000
#define FLASH_SYS_GLB_BASE_ADDR 0x10030000
#define DISPLAY_SYS_GLB_BASE_ADDR 0x4600000
#define COMMMON_DPHYTX_BASE_ADDR 0x23F1000
#define DISPLAY_DSITX_BASE_ADDR 0x4620000

/* FAT sd card locations. */
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	1
#ifdef CONFIG_AXERA_AX630C_DDR4_RETRAIN
#define CONFIG_SYS_SDRAM_BASE		0x40001000
#else
#define CONFIG_SYS_SDRAM_BASE		0x40000000
#endif
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_NS16550_SERIAL

/* spi NAND Flash */
#define CONFIG_SYS_MAX_NAND_DEVICE 		1
#define CONFIG_SYS_NAND_SELF_INIT

#define SPI_RX_SAMPLE_DELAY			(3)

#define CONFIG_SYS_NS16550_REG_SIZE (-4)
#define CONFIG_SYS_NS16550_MEM32
#define CONFIG_SYS_NS16550_CLK (208000000)
#define CONFIG_SYS_NS16550_COM1 (0x4880000)
#define CONFIG_SYS_NS16550_COM2 (0x4881000)
#define CONFIG_SYS_NS16550_COM3 (0x4882000)

/* axera xhci host driver */
/* #define CONFIG_SYS_USB_XHCI_MAX_ROOT_PORTS	2 */
#define CONFIG_SYS_MMC_MAX_BLK_COUNT    1024
#define CONFIG_IMAGE_SPARSE_FILLBUF_SIZE 0x80000

#define CONFIG_SYS_MMC_ENV_DEV	0
#define CONFIG_SYS_MMC_ENV_PART	0

#define CONFIG_GICV2
#define GIC_BASE        0x1850000
#define GICD_BASE       (GIC_BASE + 0x1000)
#define GICC_BASE       (GIC_BASE + 0x2000)

#define PWM_TIMER_BASE          0x205000 //TBC
#define TIMER2LOADCOUNT_OFFSET  0x14
#define TIMER2CONTROLREG_OFFSET 0x1C
#define TIMER2LOADCOUNT2_OFFSET 0xB4

#ifdef CONFIG_DWC_AHSATA_AXERA
#define CONFIG_LBA48
#define SYS_MEM_CTB_START           (0x40280000)
#define SYS_MEM_DATA_START          (0x402C0000)
#define AX_SATA_BIST_SUPPORT
#endif

#define AX_DEBUG	1
#ifndef pr_fmt
#define pr_fmt(fmt)	fmt
#endif

#define ax_debug_cond(cond, fmt, args...)		\
	do {						\
		if (cond)				\
			printf(pr_fmt(fmt), ##args);	\
	} while (0)

#define ax_debug(fmt, args...)			\
	ax_debug_cond(AX_DEBUG, fmt, ##args)

#ifdef  AXERA_DEBUG_KCONFIG
#define DEBUG_BOOT_ARGS "kasan_multi_shot "
#else
#define DEBUG_BOOT_ARGS ""
#endif

#define NOR_MTDPARTS  "mtdparts=spi4.0:" FLASH_PARTITIONS
#define NAND_MTDPARTS "mtdparts=spi4.0:" FLASH_PARTITIONS

/* bootargs for eMMC */
#define BOOTARGS_EMMC DEBUG_BOOT_ARGS KERNEL_BOOTARGS
/* bootargs for SD */
#define BOOTARGS_SD OS_MEM_ARGS " console=ttyS0,115200n8 earlycon=uart8250,mmio32,0x4880000 init=/sbin/init root=/dev/mmcblk1p2 rw rootdelay=3 rootfstype=ext4"

/* bootargs for SPI NOR Flash */
#define MTDIDS_SPINOR		"nor0=spi4.0"
#define MTDPARTS_SPINOR		NOR_MTDPARTS
#define BOOTARGS_SPINOR		KERNEL_BOOTARGS

/* bootargs for SPI NAND Flash */
#define MTDIDS_DEFAULT		"nand0=spi4.0"
#define MTDPARTS_DEFAULT	NAND_MTDPARTS
#define BOOTARGS_SPINAND	KERNEL_BOOTARGS

//#define CONFIG_USE_BOOTARGS
#ifndef CONFIG_BOOTARGS
#define CONFIG_BOOTARGS OS_MEM_ARGS " console=ttyS0,115200n8 noinitrd earlycon=uart8250,mmio32 \
root=/dev/mtdblock7 rw rootfstype=ubifs ubi.mtd=7,2048 root=ubi0:rootfs init=/linuxrc \
mtdparts=spi5.0:4M(uboot),768K(env),1M(atf),1M(dtb),32M(kernel),512K(param),192M(rootfs)"
#endif

#endif
