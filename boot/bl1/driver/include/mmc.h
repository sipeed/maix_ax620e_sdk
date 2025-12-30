#ifndef MMC_H
#define MMC_H

#include "cmn.h"

#define DEFAULT_CMD6_TIMEOUT_MS  500

/* Maximum block size for MMC */
#define MMC_MAX_BLOCK_LEN	512

#define OCR_VOLTAGE_MASK	0x007FFF80
#define OCR_ACCESS_MODE		0x60000000
#define OCR_BUSY		    0x80000000
#define EMMC_IO_1V8         0x80   //accroding to ocr: 1.8V bit 7
#define EMMC_IO_3V3         0xFF8000   //accroding to ocr: 3.3V bit[23:15]
#define EMMC_HCS            0x40000000 //emmc high than 2GB


#define MMC_CMD_GO_IDLE_STATE		0
#define MMC_CMD_SEND_OP_COND		1
#define MMC_CMD_ALL_SEND_CID		2
#define MMC_CMD_SET_RELATIVE_ADDR	3
#define MMC_CMD_SWITCH			6
#define MMC_CMD_SELECT_CARD		7
#define MMC_CMD_SEND_EXT_CSD    8
#define MMC_CMD_SEND_CSD		9
#define MMC_CMD_STOP_TRANSMISSION	12
#define MMC_CMD_SEND_STATUS		13
#define MMC_CMD_SET_BLOCKLEN    16
#define MMC_CMD_READ_SINGLE_BLOCK	17
#define MMC_CMD_READ_MULTIPLE_BLOCK	18
#define MMC_CMD_WRITE_SINGLE_BLOCK	24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK	25
#define MMC_CMD_APP_CMD			55
#define MMC_CMD_SET_BLOCKLEN            16
#define MMC_CMD_SEND_TUNING_BLOCK_HS200	21

#define SD_CMD_APP_SET_BUS_WIDTH	6
#define SD_CMD_SWITCH_FUNC		6
#define SD_CMD_SEND_IF_COND		8
#define SD_CMD_APP_SEND_OP_COND		41

#define MMC_STATE_PRG               (7 << 9)

#define MMC_SWITCH_MODE_WRITE_BYTE	0x03 /* Set target byte to value */

#define EXT_CSD_ENH_START_ADDR		136	/* R/W */
#define EXT_CSD_ENH_SIZE_MULT		140	/* R/W */
#define EXT_CSD_GP_SIZE_MULT		143	/* R/W */
#define EXT_CSD_PARTITION_SETTING	155	/* R/W */
#define EXT_CSD_PARTITIONS_ATTRIBUTE	156	/* R/W */
#define EXT_CSD_MAX_ENH_SIZE_MULT	157	/* R */
#define EXT_CSD_PARTITIONING_SUPPORT	160	/* RO */
#define EXT_CSD_RST_N_FUNCTION		162	/* R/W */
#define EXT_CSD_BKOPS_EN		163	/* R/W & R/W/E */
#define EXT_CSD_WR_REL_PARAM		166	/* R */
#define EXT_CSD_WR_REL_SET		167	/* R/W */
#define EXT_CSD_RPMB_MULT		168	/* RO */
#define EXT_CSD_ERASE_GROUP_DEF		175	/* R/W */
#define EXT_CSD_BOOT_BUS_WIDTH		177
#define EXT_CSD_PART_CONF		179	/* R/W */
#define EXT_CSD_BUS_WIDTH		183	/* R/W */
#define EXT_CSD_STROBE_SUPPORT		184	/* R/W */
#define EXT_CSD_HS_TIMING		185	/* R/W */
#define EXT_CSD_REV			192	/* RO */
#define EXT_CSD_CARD_TYPE		196	/* RO */
#define EXT_CSD_PART_SWITCH_TIME	199	/* RO */
#define EXT_CSD_SEC_CNT			212	/* RO, 4 bytes */
#define EXT_CSD_HC_WP_GRP_SIZE		221	/* RO */
#define EXT_CSD_ERASE_TIMEOUT_MULT	223	/* RO */
#define EXT_CSD_HC_ERASE_GRP_SIZE	224	/* RO */
#define EXT_CSD_BOOT_MULT		226	/* RO */
#define EXT_CSD_SEC_TRIM_MULT		229	/* RO */
#define EXT_CSD_SEC_ERASE_MULT		230	/* RO */
#define EXT_CSD_SEC_FEATURE_SUPPORT	231	/* RO */
#define EXT_CSD_TRIM_MULT		232	/* RO */
#define EXT_CSD_GENERIC_CMD6_TIME       248     /* RO */
#define EXT_CSD_BKOPS_SUPPORT		502	/* RO */
#define EXT_CSD_CMD_SET_NORMAL		(1 << 0)

#define EXT_CSD_BOOT_ACK(x)		       (x << 6)
#define EXT_CSD_BOOT_PART_NUM(x)	   (x << 3)
#define EXT_CSD_PARTITION_ACCESS(x)	   (x << 0)
#define EXT_CSD_PARTITION_ACCESS_MASK  (0x7)
#define UDA_DEFAULT					0
#define BOOT_PART1					1
#define BOOT_PART2					2

#define MMC_STATUS_CURR_STATE       (0xf << 9)
#define MMC_STATUS_MASK             (~0x0206BF7F)

#define MMC_STATUS_SWITCH_ERROR	(1 << 7)
#define MMC_STATUS_RDY_FOR_DATA (1 << 8)

#define EXT_CSD_BUS_WIDTH_1	0	/* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4	1	/* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8	2	/* Card is in 8 bit mode */
#define EXT_CSD_DDR_BUS_WIDTH_4	5	/* Card is in 4 bit DDR mode */
#define EXT_CSD_DDR_BUS_WIDTH_8	6	/* Card is in 8 bit DDR mode */
#define EXT_CSD_DDR_FLAG	BIT(2)	/* Flag for DDR mode */
#define EXT_CSD_BUS_WIDTH_STROBE BIT(7)	/* Enhanced strobe mode */

#define MMC_DATA_READ		1
#define MMC_DATA_WRITE		2


#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136	    (1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC  	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)


#define OCR_HCS			0x40000000
#define EXT_CSD_LEN     512

struct mmc_cmd {
	u16 cmdidx;
	u32 resp_type;
	u32 cmdarg;
	u32 response[4];
};

struct mmc_data {
	union {
		char *dest;
		const char *src;
	};
	u32 flags;
	u32 blocks;
	u32 blocksize;
};

typedef enum {
	CARD_EMMC = 0,
	CARD_SD = 1,
	CARD_TYPE_MAX
} CARD_TYPE;

enum bus_mode {
	MMC_LEGACY,
	MMC_HS,
	SD_HS,
	MMC_HS_52,
	MMC_DDR_52,
	UHS_SDR12,
	UHS_SDR25,
	UHS_SDR50,
	UHS_DDR50,
	UHS_SDR104,
	MMC_HS_200,
	MMC_HS_400,
	MMC_HS_400_ES,
	MMC_MODES_END
};

#define EXT_CSD_TIMING_LEGACY	0	/* no high speed */
#define EXT_CSD_TIMING_HS	1	/* HS */
#define EXT_CSD_TIMING_HS200	2	/* HS200 */
#define EXT_CSD_TIMING_HS400	3	/* HS400 */
#define CLK_24M   (24000000)
#define CLK_200M  (200000000)

extern u32 emmc_phy_legacy_delayline;//legacy mode
extern u32 emmc_phy_hs_delayline;    // hs mode
extern u32 sd_phy_default_delayline; //default mode

int emmc_init(u32 flash_type, u32 clk, u32 bus_width, u8 part);
int sd_init(u32 clk, u32 bus_width);
int emmc_read(char *dest, u32 start_addr, int size);
int sd_read(char *dest, u32 start_addr, int size);
int mmc_read_blocks(void *host, void *dst, u32 start, u32 blkcnt);
void emmc_reset(void);
void emmc_pins_config(void);
int emmc_boot_init(u32 flash_type, u32 clk, u32 bus_width, u32 read_cnt);
void axera_sys_glb_clk_set(CARD_TYPE card_type);
#endif
