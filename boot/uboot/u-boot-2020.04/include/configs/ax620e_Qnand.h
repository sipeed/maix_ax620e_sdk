/*
 * (C) Copyright 2020 AXERA Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __AX620E_QNAND_H
#define __AX620E_QNAND_H

#include <configs/ax620e_common.h>

#define COUNTER_FREQUENCY    24000000
#define MEM_REGION_DDR_SIZE (0x100000000UL)

#define CONFIG_SYS_MALLOC_LEN		(3 << 20)	//3M

#define SDRAM_BANK_SIZE			(2UL << 30)

#define SDHCI_ALIGN_BUF_ADDR	0x40000000
#define SDHCI_ALIGN_BUF_BLKS	0x1000
#define FDL_BUF_ADDR        	(SDHCI_ALIGN_BUF_ADDR + 0x1000)
#define FDL_BUF_LEN         	0x2000000
#define SD_UPDATE_BUF       	(SDHCI_ALIGN_BUF_ADDR + 0x2001000)
#define USB_UPDATE_BUF      	(SDHCI_ALIGN_BUF_ADDR + 0x2001000)
#define SPARSE_IMAGE_BUF_ADDR	(SDHCI_ALIGN_BUF_ADDR + 15 * 1024 * 1024)
#define SPARSE_IMAGE_BUF_LEN	0x9000000
#define OTA_BUF_ADDR        	(SDHCI_ALIGN_BUF_ADDR + 15 * 1024 * 1024)
#define OTA_BUF_LEN         	0x9000000

#define CONFIG_SYS_WHITE_ON_BLACK
#define FDL_BAD_BLKS_SCAN
//#define SPI_RX_SAMPLE_DLY_SCAN
#ifdef SPI_RX_SAMPLE_DELAY
#undef SPI_RX_SAMPLE_DELAY
#define SPI_RX_SAMPLE_DELAY		(3)
#endif

#define EMMC_DEV_ID          0
#define SD_DEV_ID            1

#endif
