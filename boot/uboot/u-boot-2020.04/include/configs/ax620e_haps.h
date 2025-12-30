/*
 * (C) Copyright 2020 AXERA Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __AX620E_HPAS_H
#define __AX620E_HPAS_H

#include <configs/ax620e_common.h>

#define COUNTER_FREQUENCY    24000000
#define MEM_REGION_DDR_SIZE (0x100000000UL)

/* boot-logo related buffer address */
#define LOGO_IMAGE_LOAD_ADDR 0x70000000U
#define LOGO_SHOW_BUFFER 0x7C800000U
#define LOGO_SHOW_BUF_SIZE 0x800000U

#define CONFIG_SYS_MALLOC_LEN		(3 << 20)	//3M

#define SDHCI_ALIGN_BUF_ADDR	0x40000000
#define SDHCI_ALIGN_BUF_BLKS	0x1000
#define FDL_BUF_ADDR        	(SDHCI_ALIGN_BUF_ADDR + 0x1000)
#define FDL_BUF_LEN         	0x200000
#define SD_UPDATE_BUF       	(SDHCI_ALIGN_BUF_ADDR + 0x201000)
#define USB_UPDATE_BUF      	(SDHCI_ALIGN_BUF_ADDR + 0x201000)
#define SPARSE_IMAGE_BUF_ADDR	(UBOOT_IMG_HEADER_BASE + 4 * 1024 * 1024)
#define SPARSE_IMAGE_BUF_LEN	0x10000000
#define OTA_BUF_ADDR        	(UBOOT_IMG_HEADER_BASE + 4 * 1024 * 1024)
#define OTA_BUF_LEN         	0x10000000

#define SDRAM_BANK_SIZE			(2UL << 30)

#define CONFIG_SYS_WHITE_ON_BLACK

#define EMMC_DEV_ID          0
#define SD_DEV_ID            1

#endif
