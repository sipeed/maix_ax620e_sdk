/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __BOOT_H__
#define __BOOT_H__

#include <stdbool.h>
#include "cmn.h"
#include "chip_reg.h"

#define SPL_IMG_HEADER_BASE		(SYS_OCM_BASE)

#define IMG_HEADER_MAGIC_DATA		(0x55543322)
#define MAX_BLK_SIZE			((2 * 1024 * 1024) - 1) //2M - 1

#define IMG_HANDLD_THRESHOLD		(MAX_BLK_SIZE >> 1)

#define FLASH_EMMC			0
#define FLASH_EMMC_BOOT_8BIT_50M_768K	1
#define FLASH_SPI_SLV			2
#define FLASH_NAND_2K			3
#define FLASH_EMMC_BOOT_4BIT_25M_128K	4
#define FLASH_NAND_4K			5
#define FLASH_EMMC_BOOT_4BIT_25M_768K	6
#define FLASH_NOR			7
#define FLASH_SD_OR_USB_DL		8
#define FLASH_SD			9

typedef enum {
	BOOT_MODE_UNKNOWN    = 0x0,
	BOOT_MODE_DOWNLOAD   = 0x1,
	BOOT_MODE_NORMALBOOT = 0x2,
	BOOT_MODE_SDBOOT     = 0x3,
}boot_mode_e;

typedef enum {
	STORAGE_TYPE_EMMC = 0x0,
	STORAGE_TYPE_NAND = 0x1,
	STORAGE_TYPE_NOR = 0x2,
	STORAGE_TYPE_SD = 0x3,
	STORAGE_TYPE_PCIE = 0x4,
	STORAGE_TYPE_UNKNOWN = 0x5,
} storage_sel_e;

typedef enum {
	BOOT_TYPE_UNKNOWN = 0x0,
	EMMC_BOOT_UDA = 0x1,
	EMMC_BOOT_8BIT_50M_768K = 0x2,
	EMMC_BOOT_4BIT_25M_768K = 0x3,
	EMMC_BOOT_4BIT_25M_128K = 0x4,
	NAND_2K = 0x5,
	NAND_4K = 0x6,
	NOR = 0x7,
} boot_type_e;

typedef enum {
	NONE_CHIP_TYPE = 0x0,
	AX620Q_CHIP = 0xB,
	AX620QX_CHIP = 0x15,
	AX630C_CHIP = 0x26
} chip_type_efuse_e;

#define BOOT_MODE_ENV_MAGIC	0x12345678
#define BOOT_MODE_INFO_ADDR 0x0 //iram0 addr
#define MISC_INFO_ADDR 0x40 //iram0 addr
#define DDR_INFO_ADDR 0xA0 //iram0 addr

struct img_header {
	u32 check_sum;//0x00
	u32 magic_data;//0x04, should be 0x55543322
	u32 capability;
	u32 img_size;
	u32 fw_size;
	u32 img_check_sum;
	u32 fw_check_sum;
	/*
	 * [1:0]: nand page_size, page_size = 2^(nand_nor_cfg[1:0] + 11), 2KB ~ 16KB
	 * [4:2]: nand block_size, block_size = 2^(nand_nor_cfg[4:2] + 16), 64KB ~ 8MB
	 * [6:5]: nand oob_size, oob_size = 2^(nand_nor_cfg[6:5] + 6), 64Bytes ~ 512Bytes
	 * [7]: nand bad block marker size, if nand_nor_cfg[7] is 0, the bad block marker
	 * is 1 byte, if nand_type[7] is 1, bad block marker is 2 bytes.
	 * [15: 8]: nor QE read status register cmd
	 * [23:16]: nor QE write status register cmd
	 * [31:24]: nor QE bit offset
	 */
	u32 nand_nor_cfg;
	u32 boot_bak_flash_addr;
	u32 fw_flash_addr;
	u32 fw_bak_flash_addr;
	u32 key_n_header;
	u32 rsa_key_n[96];
	u32 key_e_header;
	u32 rsa_key_e;
	u32 sig_header;
	u32 signature[96];
	u32 nand_ext_cfg;
	u32 nand_rx_sample_delay[3];
	u32 nor_rx_sample_delay[3];
	u32 ocm_start_addr;
	u32 riscv_flash_addr;
	u32 riscv_check_sum;
	u32 riscv_img_size;
	u32 nand_phy_setting[3];
	u32 nor_phy_setting[3];
	u32 emmc_phy_legacy_delayline;
	u32 emmc_phy_hs_delayline;
	u32 sd_phy_default_delayline;
	u32 aes_key[12];
	u32 reserved[17];
};

typedef enum {
	IMG_UNKNOWN = 0x0,
	DDRINIT = 0x1,
	ATF = 0x2,
	UBOOT = 0x3,
	OPTEE = 0x4,
	DTB = 0x5,
	KERNEL = 0x6,
	RAMDISK = 0x7,
} boot_img_e;

typedef struct boot_image_info {
	u32 boot_index;
	u32 boot_header_flash_addr;
	u32 boot_flash_addr;
	u32 boot_header_flash_bk_addr;
	u32 boot_flash_bk_addr;
	boot_img_e img_type;
} boot_image_info_t;

/* img_header capbility bit field */
#define MMC_BUSWIDTH_4				(1 << 0)
#define MMC_BUSWIDTH_8				(1 << 1)
#define SPI_NAND_BUSWIDTH_4			(1 << 2)
#define SPI_NOR_BUSWIDTH_4			(1 << 3)
#define SPI_SLV_BUSWIDTH_RX_TX_1 		(1 << 4)
#define SPI_SLV_BUSWIDTH_4			(1 << 5)
#define SDCARD_BUSWIDTH_4			(1 << 6)
#define SDIO_BUSWIDTH_4				(1 << 7)
#define IMG_CIPHER_ENABLE			(1 << 8)
#define CPU_CLK_CONFIG				(1 << 9)
#define RSA_3072_MODE				(1 << 10)
#define IMG_BAK_ENABLE				(1 << 11)
#define FW_BAK_ENABLE				(1 << 12)
#define IMG_CHECK_ENABLE			(1 << 13) //to enable image checksum verify, if check fail, read the backup from flash
#define FW_CHECK_ENABLE				(1 << 14) //to enable FW image checksum verify, if check fail, read the backup from flash
#define NPU_OCM_USE 				(1 << 15)
#define CE_FW_CP_DMA_USE 			(1 << 16)
#define MMC_BUSCLK_25M				(1 << 17)
#define MMC_BUSCLK_50M				(1 << 18)
#define SPI_NAND_BUSCLK_25M			(1 << 19)
#define SPI_NAND_BUSCLK_50M			(1 << 20)
#define SPI_NOR_BUSCLK_25M			(1 << 21)
#define SPI_NOR_BUSCLK_50M			(1 << 22)
#define RISCV_EXISTS				(1 << 23)


#define BUS_WIDTH_1 				1
#define BUS_WIDTH_4 				4
#define BUS_WIDTH_8 				8

enum {
	BOOT_READ_HEADER_FAIL = -1,
	BOOT_EMMC_READ_FAIL = -2,
	BOOT_FLASH_INIT_FAIL = -3,
	BOOT_FLASH_READ_FAIL = -4,
	BOOT_EIP_INIT_FAIL = -5,
	BOOT_SHA_INIT_FAIL = -6,
	BOOT_SHA_UPDATE_FAIL = -7,
	BOOT_SHA_FAINAL_FAIL = -8,
	BOOT_SHA_FAIL = -9,
	BOOT_AES_FAIL = -10,
	BOOT_RSA_FAIL = -11,
	BOOT_IMG_CHECK_FAIL = -12,
	BOOT_WRITE_DATA_FAIL = -13,
};

int verify_img_header(struct img_header *header);
int flash_boot(u32 flash_ops, u32 flash_bk_ops, long ram_ops);
u32 calc_word_chksum(int *data, int size);
int read_image_data(u32 flash_type, struct img_header *boot_header, struct boot_image_info *img_info);

#endif
