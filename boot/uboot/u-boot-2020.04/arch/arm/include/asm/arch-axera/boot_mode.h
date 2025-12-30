/*
 * AXERA AX620E
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __BOOT_MODE_H__
#define __BOOT_MODE_H__

typedef enum boot_mode {
	SYSDUMP_MODE = 0x0,
	SD_UPDATE_MODE = 0x1,
	USB_UPDATE_MODE = 0x2,
	UART_UPDATE_MODE = 0x3,
	TFTP_UPDATE_MODE = 0x4,
	USB_STOR_MODE = 0x5,
	SD_BOOT_MODE = 0x6,
	NORMAL_BOOT_MODE = 0x7,
	CMD_UNDEFINED_MODE,
	BOOTMODE_FUN_NUM,
} boot_mode_t;

typedef boot_mode_t (*s_boot_func_array)(void);

typedef enum dl_channel {
	DL_CHAN_UNKNOWN = 0x0,
	DL_CHAN_SDIO = 0x1,
	DL_CHAN_SPI = 0x2,
	DL_CHAN_USB = 0x3,
	DL_CHAN_UART0 = 0x4,	//0x4 is uart0, 0x5 is uart1
	DL_CHAN_UART1 = 0x5,
	DL_CHAN_SD = 0x6,
} dl_channel_t;

typedef enum storage_type_sel {
	STORAGE_TYPE_EMMC = 0x0,
	STORAGE_TYPE_NAND = 0x1,
	STORAGE_TYPE_NOR = 0x2,
	STORAGE_TYPE_SD = 0x3,
	STORAGE_TYPE_PCIE = 0x4,
	STORAGE_TYPE_UNKNOWN = 0x5,
} storage_sel_t;

typedef enum chip_type {
	NONE_CHIP_TYPE = 0x0,
	AX620Q_CHIP = 0x1,
	AX620QX_CHIP = 0x2,
	AX630C_CHIP = 0x4,
	AX631_CHIP = 0x5,
	AX620QZ_CHIP = 0x6,
	AX620QP_CHIP = 0x7,
	AX620E_CHIP_MAX = 0x8,
} chip_type_e;

typedef enum ax630c_board_type {
	PHY_AX630C_EVB_V1_0 = 0,
	PHY_AX630C_DEMO_V1_0 = 1,
	PHY_AX630C_DEMO_DDR3_V1_0 = 3,
	PHY_AX630C_SLT_V1_0 = 8,
	PHY_AX630C_DEMO_V1_1 = 6,
	PHY_AX630C_DEMO_LP4_V1_0 = 12,
	// ### SIPEED EDIT ###
	PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G = 2,
	PHY_AX630C_AX631_MAIXCAM2_SOM_1G = 5,
	PHY_AX630C_AX631_MAIXCAM2_SOM_2G = 10,
	PHY_AX630C_AX631_MAIXCAM2_SOM_4G = 14,
	// ### SIPEED EDIT END ###
} ax630c_board_type_e;

typedef enum ax620q_board_type {
	PHY_AX620Q_LP4_EVB_V1_0 = 4,
	PHY_AX620Q_LP4_DEMO_V1_0 = 5,
	PHY_AX620Q_LP4_SLT_V1_0 = 10,
	PHY_AX620Q_LP4_DEMO_V1_1 = 11,
	PHY_AX620Q_LP4_38BOARD_V1_0 = 14,
	PHY_AX620Q_LP4_MINION_BOARD = 15,
} ax620q_board_type_e;

typedef enum board_type {
	AX630C_EVB_V1_0 = 0,
	AX630C_DEMO_V1_0,
	AX630C_SLT_V1_0,
	AX620Q_LP4_EVB_V1_0,
	AX620Q_LP4_DEMO_V1_0,
	AX620Q_LP4_SLT_V1_0,
	AX630C_DEMO_V1_1,
	AX620Q_LP4_DEMO_V1_1,
	AX630C_DEMO_LP4_V1_0,
	AX620Q_LP4_38BOARD_V1_0,
	AX620Q_LP4_MINION_BOARD,
	AX630C_DEMO_DDR3_V1_0,
	// ### SIPEED EDIT ###
	AX630C_AX631_MAIXCAM2_SOM_0_5G = PHY_AX630C_AX631_MAIXCAM2_SOM_0_5G,
	AX630C_AX631_MAIXCAM2_SOM_1G = PHY_AX630C_AX631_MAIXCAM2_SOM_1G,
	AX630C_AX631_MAIXCAM2_SOM_2G = PHY_AX630C_AX631_MAIXCAM2_SOM_2G,
	AX630C_AX631_MAIXCAM2_SOM_4G = PHY_AX630C_AX631_MAIXCAM2_SOM_4G,
	// ### SIPEED EDIT END ###
	AX620E_BOARD_MAX,
} board_type_e;

typedef enum boot_type_sel {
	BOOT_TYPE_UNKNOWN = 0x0,
	EMMC_BOOT_UDA = 0x1,
	EMMC_BOOT_8BIT_50M_768K = 0x2,
	EMMC_BOOT_4BIT_25M_768K = 0x3,
	EMMC_BOOT_4BIT_25M_128K = 0x4,
	NAND_2K = 0x5,
	NAND_4K = 0x6,
	NOR = 0x7,
} boot_type_t;

#define BOOT_MODE_ENV_MAGIC	0x12345678

#define SLOTA             BIT(2)
#define SLOTB             BIT(3)
#define BOOT_KERNEL_FAIL  BIT(7)
#define BOOT_DOWNLOAD     BIT(8)
#define BOOT_RECOVERY     BIT(11)

/* For boot mode or download in thm_reset/wdt_reset/soft_rst */
#define TOP_CHIPMODE_GLB                (0x2390000)
#define TOP_CHIPMODE_GLB_SW_PORT        (TOP_CHIPMODE_GLB + 0x0)
#define TOP_CHIPMODE_GLB_SW_PORT_SET    (TOP_CHIPMODE_GLB + 0x4)
#define TOP_CHIPMODE_GLB_SW_PORT_CLR    (TOP_CHIPMODE_GLB + 0x8)
#define TOP_CHIPMODE_GLB_SW             (TOP_CHIPMODE_GLB + 0xC)
#define TOP_CHIPMODE_GLB_SW_SET         (TOP_CHIPMODE_GLB + 0x10)
#define TOP_CHIPMODE_GLB_SW_CLR         (TOP_CHIPMODE_GLB + 0x14)
#define TOP_CHIPMODE_GLB_BACKUP0        (TOP_CHIPMODE_GLB + 0x24)
#define TOP_CHIPMODE_GLB_BACKUP0_SET    (TOP_CHIPMODE_GLB + 0x28)
#define TOP_CHIPMODE_GLB_BACKUP0_CLR    (TOP_CHIPMODE_GLB + 0x2C)

#define COMM_SYS_DUMMY_SW5	(COMM_SYS_GLB + 0x204)
#define COMM_SYS_DUMMY_SW9	(COMM_SYS_GLB + 0x220)
#define COMM_SYS_DUMMY_SW1	(COMM_SYS_GLB + 0xE0)
#define COMM_SYS_DUMMY_SW2	(COMM_SYS_GLB + 0xE4)
typedef struct boot_mode_info {
	u32 magic;		//0x12345678
	boot_mode_t mode;
	dl_channel_t dl_channel;	//usb,uart0,uart1,uart2...
	storage_sel_t storage_sel;
	boot_type_t boot_type;
	u8 is_sd_boot;
} boot_mode_info_t;

#define MISC_INFO_ADDR 0x740 //iram0 addr
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

#define DDR_INFO_ADDR	0x800 //iram0 addr
#define DDR_DFS_MAX	2
#define RANK_MAX	2
typedef struct ddr_dfs_vref {
	u16 freq;
	u8 dram_VREF_CA[RANK_MAX];
	u8 dram_VREF_DQ[RANK_MAX];
	u8 rf_io_vrefi_adj_PHY_A;
	u8 rf_io_vrefi_adj_PHY_B;
} ddr_dfs_vref_t;

typedef struct ddr_info {
	struct ddr_dfs_vref dfs_vref[DDR_DFS_MAX];
} ddr_info_t;

#endif
