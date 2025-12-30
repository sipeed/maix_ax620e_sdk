#ifndef __BOOT_H__
#define __BOOT_H__

#include "cmn.h"
#include "chip_reg.h"
#include "fdl_channel.h"
#include "ddr_init_config.h"


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

#define BOOT_MODE_ENV_MAGIC	0x12345678
#define BOOT_MODE_INFO_ADDR 0x700 //iram0 addr
#define MISC_INFO_ADDR 0x740 //iram0 addr
#define DDR_INFO_ADDR 0x800 //iram0 addr

typedef struct boot_mode_info {
	u32 magic; //0x12345678
	boot_mode_e mode;
	dl_channel_e dl_channel;//usb,uart0,uart1,uart2...
	storage_sel_e storage_sel;
	boot_type_e boot_type;
	u8 is_sd_boot;
} boot_mode_info_t;

typedef struct misc_info {
	u32 pub_key_hash[8];
	u32 aes_key[8];
	u32 board_id;
	u32 chip_type;
	u32 uid_l;
	u32 uid_h;
	u32 thm_vref;
	u32 thm_temp;
	u16 bgs;
	u16 trim;
	u32 phy_board_id;
} misc_info_t;

typedef struct ddr_info {
	struct ddr_dfs_vref_t dfs_vref[DDR_DFS_MAX];
} ddr_info_t;

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
	RISCV = 0x7,
	RAMDISK = 0x8,
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

#define BOOT_INDEX        GENMASK(1, 0)
#define SLOTA             BIT(2)
#define SLOTB             BIT(3)
#define SLOTA_BOOTABLE    BIT(4)
#define SLOTB_BOOTABLE    BIT(5)
#define BOOT_SD           BIT(6)
#define BOOT_KERNEL_FAIL  BIT(7)
#define BOOT_DOWNLOAD     BIT(8)
#define BOOT_PANIC        BIT(9)
#define BOOT_WDT_TIMEOUT  BIT(10)
#define BOOT_RECOVERY     BIT(11)
#define OTA_STATUS        BIT(30)
#define OTA_SUPPORT       BIT(31)

int verify_img_header(struct img_header *header);
int flash_boot(u32 flash_type, boot_img_e boot_type, u32 flash_ops, u32 flash_bk_ops, long ram_ops);
u32 calc_word_chksum(int *data, int size);
#ifdef SECURE_BOOT_TEST
int ce_hw_init(u32 flash_type, struct img_header *header, struct img_header *boot_header);
#endif
int read_image_data(u32 flash_type, struct img_header *boot_header, struct boot_image_info *img_info);

#endif
