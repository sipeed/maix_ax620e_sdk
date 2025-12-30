#include "mmc.h"
#include "errno.h"
#include "cmn.h"
#include "secure.h"
#include "sdhci_cdns.h"
#include "timer.h"
#include "chip_reg.h"
#include "board.h"
#include "timer.h"
#include "boot.h"
#include "printf.h"

// #define WRITE_TEST
#define true 1
#define false 0

#define MMC_HS400_ES_SUPPORT
// #define MMC_HS400_SUPPORT
#define MMC_HS200_SUPPORT

static u8 high_capacity = 1;
static u8 sd_version = 1;
//mmc card set rca, but sd get rca
static u16 rca = 1;
static u8 sd_card_initialized = 0;
#if defined(AX620E_EMMC)
static u8 part_switch_time;
static u8 emmc_card_initialized = 0;
static u8 current_part = 0;   /* emmc default is UDA */
#endif
extern u32 current_bus_width;
extern u32 current_clock;
u32 emmc_phy_legacy_delayline = 0;//legacy mode
u32 emmc_phy_hs_delayline = 0;    // hs mode
u32 sd_phy_default_delayline = 0; //default mode

static u32 freqs[] = {400000, 300000, 200000, 100000};

static inline void mem_set(void *start, u8 value, int len)
{
	u8 *begin = (u8 *)start;
	while (len--) {
		*begin++ = value;
	}
}

int mmc_set_blocklen(void *host, int len)
{
	struct mmc_cmd cmd;
	int ret = 0;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;

	if (sdhci_send_cmd(host, &cmd, NULL))
		ret = -EMMC_SET_BLKLEN;

	return ret;
}

#ifdef WRITE_TEST
int mmc_write_blocks(void *host, u32 start, u32 blkcnt, const void *src)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	mem_set(&cmd, 0, mmc_cmd_len);
	if (blkcnt == 1)
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;

	//check high capacity
	if (1)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * MMC_MAX_BLOCK_LEN;

	cmd.resp_type = MMC_RSP_R1;

	data.src = src;
	data.blocks = blkcnt;
	data.blocksize = MMC_MAX_BLOCK_LEN;
	data.flags = MMC_DATA_WRITE;

	if (sdhci_send_cmd(host, &cmd, &data)) {
		return -1;
	}

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (sdhci_send_cmd(host, &cmd, NULL)) {
			return -EMMC_WSTOP_TRANS;
		}
	}
#if 0  //romcode doesn't need write
	/* Waiting for the ready status */
	if (mmc_poll_for_busy(mmc, timeout_ms))
		return 0;
#endif
	return 0;
}
#endif
#if defined(AX620E_EMMC)
int emmc_read(char *dest, u32 start_addr, int size)
{
	void *sdhci_host = (void *)(EMMC_BASE + SRS_BASE_OFFSET);
	int ret;
	u32 start_blk, blk_cnt;

	if (start_addr % MMC_MAX_BLOCK_LEN) {
		//printf("emmc_read input param is not aligned\n");
		return -1;
	}

	start_blk = start_addr / MMC_MAX_BLOCK_LEN;
	blk_cnt = (size + MMC_MAX_BLOCK_LEN - 1) / MMC_MAX_BLOCK_LEN;
	ret = mmc_read_blocks(sdhci_host, (void *)dest, start_blk, blk_cnt);

	return ret;
}
#endif

int sd_read(char *dest, u32 start_addr, int size)
{
	void *sdhci_host = (void *)(SD_MST1_BASE + SRS_BASE_OFFSET);
	int ret;
	u32 start_blk, blk_cnt;

	if (start_addr % MMC_MAX_BLOCK_LEN) {
		//printf("emmc_read input param is not aligned\n");
		return -1;
	}

	start_blk = start_addr / MMC_MAX_BLOCK_LEN;
	blk_cnt = (size + MMC_MAX_BLOCK_LEN - 1) / MMC_MAX_BLOCK_LEN;
	ret = mmc_read_blocks(sdhci_host, (void *)dest, start_blk, blk_cnt);

	return ret;
}

int mmc_read_blocks(void *host, void *dst, u32 start, u32 blkcnt)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	mem_set(&cmd, 0, mmc_cmd_len);
	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	//check high capacity
	if (high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * MMC_MAX_BLOCK_LEN;

	cmd.resp_type = MMC_RSP_R1;

	data.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = MMC_MAX_BLOCK_LEN;
	data.flags = MMC_DATA_READ;

	if (sdhci_send_cmd(host, &cmd, &data))
		return -EMMC_READ_BLK;

	if (blkcnt > 1) {
		mem_set(&cmd, 0, mmc_cmd_len);
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (sdhci_send_cmd(host, &cmd, NULL)) {
			return -EMMC_RSTOP_TRANS;
		}
	}

	return 0;
}
int mmc_send_status(void *host, unsigned int *status)
{
	struct mmc_cmd cmd;
	int err, retries = 5;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = rca << 16;

	while (retries--) {
		err = sdhci_send_cmd(host, &cmd, NULL);
		if (!err) {
			*status = cmd.response[0];
			return 0;
		}
	}
	return -EMMC_GET_STATUS;
}

#if defined(AX620E_EMMC)
static int mmc_switch(void *host, u8 set, u8 index, u8 value)
{
	unsigned int status;
	struct mmc_cmd cmd;
	int timeout_ms = DEFAULT_CMD6_TIMEOUT_MS;
	int is_part_switch = (set == EXT_CSD_CMD_SET_NORMAL) &&
				  (index == EXT_CSD_PART_CONF);
	int retries = 3;
	int ret;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	if (is_part_switch	&& part_switch_time)
		timeout_ms = part_switch_time * 10;

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
				 (index << 16) |
				 (value << 8);

	do {
		ret = sdhci_send_cmd(host, &cmd, NULL);
	} while (ret && retries-- > 0);

	if (ret)
		return -EEMMC_CMD_SWITCH;

	//calculate retries accroding to timeout_ms
	retries = timeout_ms * 1000 / 100;
	do {
		ret = mmc_send_status(host, &status);

		if (!ret && (status & MMC_STATUS_SWITCH_ERROR)) {
			return -EEMMC_CMD_SWITCH;
		}
		if (!ret && (status & MMC_STATUS_RDY_FOR_DATA))
			return 0;

		udelay(100);
	} while (retries--);

	return -EEMMC_CMD_SWITCH;
}
#endif

#if 0
int mmc_read_boot_blocks(void *host, void *dest, u8 partition,
		u32 start, u32 blkcnt)
{
	int ret;
	u8 part_conf;

	part_conf = (part_config & (~EXT_CSD_PARTITION_ACCESS_MASK)) |
			EXT_CSD_PARTITION_ACCESS(partition);
	ret = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONF,
			 part_conf);
	if (ret)
		return ret;

	ret = mmc_read_blocks(host, dest, start, blkcnt);

	return ret;
}

int mmc_write_boot_blocks(void *host, void *src, u8 partition,
		u32 start, u32 blkcnt)
{
	int ret;
	u8 part_conf;

	part_conf = EXT_CSD_BOOT_ACK(0) |
		    EXT_CSD_BOOT_PART_NUM(0) |
		    EXT_CSD_PARTITION_ACCESS(partition);

	ret = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONF,
			 part_conf);
	if (ret)
		return ret;

	ret = mmc_write_blocks(host, start, blkcnt, src);

	return ret;
}
#endif
#if defined(AX620E_EMMC)
#if defined MMC_HS200_SUPPORT || defined MMC_HS400_SUPPORT
static int mmc_send_ext_csd(void *host, char *ext_csd)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int ret;
	int mmc_cmd_len = sizeof(struct mmc_cmd);
	mem_set(&cmd, 0, mmc_cmd_len);
	/* Get the Card Status Register */
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;

	data.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = MMC_MAX_BLOCK_LEN;
	data.flags = MMC_DATA_READ;

	ret = sdhci_send_cmd(host, &cmd, &data);
	if (ret) {
		return -EEMMC_EXT_CSD;
	}

	return ret;
}

static int mmc_set_card_speed(void *host, enum bus_mode mode, u8 hsdowngrade)
{
	int ret;
	int speed_bits;
	char ext_csd[EXT_CSD_LEN];

	switch (mode) {
	case MMC_HS:
	case MMC_HS_52:
	case MMC_DDR_52:
		speed_bits = EXT_CSD_TIMING_HS;
		break;
#ifdef MMC_HS200_SUPPORT
	case MMC_HS_200:
		speed_bits = EXT_CSD_TIMING_HS200;
		break;
#endif
#ifdef MMC_HS400_SUPPORT
	case MMC_HS_400:
		speed_bits = EXT_CSD_TIMING_HS400;
		break;
#endif
#ifdef MMC_HS400_ES_SUPPORT
	case MMC_HS_400_ES:
		speed_bits = EXT_CSD_TIMING_HS400;
		break;
#endif
	case MMC_LEGACY:
		speed_bits = EXT_CSD_TIMING_LEGACY;
		break;
	default:
		return -EINVAL;
	}
	ret = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, speed_bits);
	if (ret) {
		err("set EXT_CSD_HS_TIMING fail\n");
		return ret;
	}


#if defined MMC_HS200_SUPPORT || defined MMC_HS400_SUPPORT
	/*
	 * In case the eMMC is in HS200/HS400 mode and we are downgrading
	 * to HS mode, the card clock are still running much faster than
	 * the supported HS mode clock, so we can not reliably read out
	 * Extended CSD. Reconfigure the controller to run at HS mode.
	 */
	if (hsdowngrade) {
		mmc_select_mode(host, MMC_HS);
		ret = sdhci_set_clock(host, HS_EMMC_CLK, CLK_200M);
		if (ret) {
			err("set clock to %d\n fail\r\n", HS_EMMC_CLK);
			return ret;
		}
	}
#endif

	if ((mode == MMC_HS) || (mode == MMC_HS_52)) {
		/* Now check to see that it worked */
		ret = mmc_send_ext_csd(host, ext_csd);
		if (ret) {
			err("mmc_send_ext_csd fail\r\n");
			return ret;
		}

		/* No high-speed support */
		if (!ext_csd[EXT_CSD_HS_TIMING])
			return -ENOTSUPP;
	}
	return 0;
}

static const u8 tuning_blk_pattern_4bit[] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
	0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
	0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
	0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
	0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
	0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
	0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
	0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

static const u8 tuning_blk_pattern_8bit[] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

int mmc_send_tuning(void *host, u32 opcode, int *cmd_error)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	const u8 *tuning_block_pattern;
	int size, ret;
	u8 data_buf[sizeof(tuning_blk_pattern_8bit)];

	if (current_bus_width == 8) {
		tuning_block_pattern = tuning_blk_pattern_8bit;
		size = sizeof(tuning_blk_pattern_8bit);
	} else if (current_bus_width == 4) {
		tuning_block_pattern = tuning_blk_pattern_4bit;
		size = sizeof(tuning_blk_pattern_4bit);
	} else {

		return -EINVAL;
	}

	cmd.cmdidx = opcode;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = (void *)data_buf;
	data.blocks = 1;
	data.blocksize = size;
	data.flags = MMC_DATA_READ;
	ret = sdhci_send_cmd(host, &cmd, &data);
	if (ret) {
		err("tuning cmd send err: %d\r\n", ret);
		return ret;
	}

	if (memcmp(data_buf, tuning_block_pattern, size)) {
		err("tuning_block_pattern cmp not same\r\n");
		return -EIO;
	}
	return 0;
}

#define SDHCI_CDNS_MAX_TUNING_LOOP	40
static int mmc_execute_tuning(void *host, u32 opcode)
{
	int cur_streak = 0;
	int max_streak = 0;
	int end_of_streak = 0;
	int i;
	if (opcode != MMC_CMD_SEND_TUNING_BLOCK_HS200)
		return -EINVAL;

	for (i = 0; i < SDHCI_CDNS_MAX_TUNING_LOOP; i++) {
		if (sdhci_cdns_set_tune_val(host, i) ||
		    mmc_send_tuning(host, opcode, NULL)) { /* bad */
			cur_streak = 0;
		} else { /* good */
			cur_streak++;
			if (cur_streak > max_streak) {
				max_streak = cur_streak;
				end_of_streak = i;
			}
		}
	}

	if (!max_streak) {
		err("no tuning point found\r\n");
		return -EIO;
	}
	err("tuning successful, set value: %d\r\n", end_of_streak - max_streak / 2);
	return sdhci_cdns_set_tune_val(host, end_of_streak - max_streak / 2);
}
#endif

#ifdef MMC_HS400_SUPPORT
static int mmc_select_hs400(void *host)
{
	int ret;
	err("set to hs400\r\n");
	/* Set timing to HS200 for tuning */
	ret = mmc_set_card_speed(host, MMC_HS_200, false);
	if (ret) {
		err("mmc_set_card_speed MMC_HS_200 failed\r\n");
		return ret;
	}

	/* configure the bus mode (host) */
	mmc_select_mode(host, MMC_HS_200);
	ret = sdhci_set_clock(host, HS400_EMMC_CLK, CLK_200M);
	if (ret) {
		err("set clock to %d\n fail\n", HS400_EMMC_CLK);
		return ret;
	}

	/* execute tuning if needed */
	ret = mmc_execute_tuning(host, MMC_CMD_SEND_TUNING_BLOCK_HS200);
	if (ret) {
		err("tuning failed\n");
		return ret;
	}

	/* Set back to HS */
	mmc_set_card_speed(host, MMC_HS, true);

	ret = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH,
			 EXT_CSD_BUS_WIDTH_8 | EXT_CSD_DDR_FLAG);
	if (ret)
		return ret;

	ret = mmc_set_card_speed(host, MMC_HS_400, false);
	if (ret)
		return ret;

	mmc_select_mode(host, MMC_HS_400);
	ret = sdhci_set_clock(host, HS400_EMMC_CLK, CLK_200M);
	if (ret) {
		err("set clock to %d\n fail\n", HS400_EMMC_CLK);
		return ret;
	}

	return 0;
}
#endif

#ifdef MMC_HS200_SUPPORT
static int mmc_select_hs200(void *host)
{
	int ret;
	err("set to hs200\r\n");
	/* Set timing to HS200 for tuning */
	ret = mmc_set_card_speed(host, MMC_HS_200, false);
	if (ret) {
		err("mmc_set_card_speed MMC_HS_200 failed\r\n");
		return ret;
	}

	/* configure the bus mode (host) */
	mmc_select_mode(host, MMC_HS_200);
	ret = sdhci_set_clock(host, HS400_EMMC_CLK, CLK_200M);
	if (ret) {
		err("set clock to %d\n fail\n", HS400_EMMC_CLK);
		return ret;
	}

	/* execute tuning if needed */
	ret = mmc_execute_tuning(host, MMC_CMD_SEND_TUNING_BLOCK_HS200);
	if (ret) {
		err("tuning failed\n");
		return ret;
	}
	return 0;
}
#endif

#ifdef MMC_HS400_ES_SUPPORT
static int mmc_select_hs400es(void *host)
{
	int ret;

	err("set to hs400es\r\n");
	ret = mmc_set_card_speed(host, MMC_HS, true);
	if (ret) {
		err("mmc_set_card_speed MMC_HS failed\r\n");
		return ret;
	}

	ret = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH,
			 EXT_CSD_BUS_WIDTH_8 | EXT_CSD_DDR_FLAG |
			 EXT_CSD_BUS_WIDTH_STROBE);
	if (ret) {
		err("switch to bus width for hs400 failed\n");
		return ret;
	}
	/* TODO: driver strength */
	ret = mmc_set_card_speed(host, MMC_HS_400_ES, false);
	if (ret) {
		err("mmc_set_card_speed MMC_HS_400_ES failed\n");
		return ret;
	}

	mmc_select_mode(host, MMC_HS_400_ES);
	ret = sdhci_set_clock(host, HS400_ES_EMMC_CLK, CLK_200M);
	if (ret) {
		err("set clock to %d\n fail\n", HS400_EMMC_CLK);
		return ret;
	}
	// return mmc_set_enhanced_strobe(host);
	return 0;
}
#endif
#endif
static int sd_select_bus_width(void *host, int w)
{
	int err;
	struct mmc_cmd cmd;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	if ((w != 4) && (w != 1))
		return -1;

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = rca << 16;

	err = sdhci_send_cmd(host, &cmd, NULL);
	if (err)
		return err;

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
	cmd.resp_type = MMC_RSP_R1;
	if (w == 4)
		cmd.cmdarg = 2;
	else if (w == 1)
		cmd.cmdarg = 0;
	err = sdhci_send_cmd(host, &cmd, NULL);
	if (err)
		return err;

	return 0;
}
#if defined(AX620E_EMMC)
static int emmc_select_bus_width(void *host, int w)
{
	int err;
	u8 ext_bus_width = 0;

	/* emmc default 1 bit */
	if ((w != 1) && (w != 4) && (w != 8))
		return -1;
	if (1 == w) {
		ext_bus_width = 0;
	} else if (4 == w) {
		ext_bus_width = 1;
	} else if (8 == w) {
		ext_bus_width = 2;
	} else {
		//printf("emmc set bus width param error");
		return -1;
	}

	/* configure the bus width (card + host) */
	err = mmc_switch(host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH,
						ext_bus_width);
	return err;
}
#endif
static int mmc_startup(void *host, CARD_TYPE card_type)
{
	int ret = 0;
	u8 mmc_version = 0;
	struct mmc_cmd cmd;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	/* Put the Card in Identify Mode */
	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = MMC_CMD_ALL_SEND_CID;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;
	ret = sdhci_send_cmd(host, &cmd, NULL);
	if (ret) {
		return -EEMMC_CID;
	}

	/* set rsa emmc set rca, but sd get rca */
	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = MMC_CMD_SET_RELATIVE_ADDR;
	if (card_type == CARD_EMMC) {
		cmd.cmdarg = rca << 16;
	} else {
		cmd.cmdarg = 0;
	}
	cmd.resp_type = MMC_RSP_R1;
	ret = sdhci_send_cmd(host, &cmd, NULL);
	if (ret) {
		return -EMMC_SET_RCA;
	}
	if (card_type == CARD_SD)
		rca = (cmd.response[0] >> 16) & 0xFFFF;

	/* Get the Card-Specific Data */
	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = rca << 16;
	ret = sdhci_send_cmd(host, &cmd, NULL);
	if (ret) {
		return -EMMC_CSD;
	}
	mmc_version = (cmd.response[0] >> 26) & 0xF;
	/* mmc version low than 4 don't support EXT_CSD*/
	if (card_type == CARD_EMMC && mmc_version < 4)
		return -EEVERSION_L4;

	/* Select the card, and put it into Transfer Mode */
	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = MMC_CMD_SELECT_CARD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = rca << 16;
	ret = sdhci_send_cmd(host, &cmd, NULL);
	if (ret) {
		return -EMMC_SELECT_CARD;
	}

	return 0;
}

static int mmc_send_if_cond(void *host)
{
	struct mmc_cmd cmd;
	int err;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = 0x1aa;
	cmd.resp_type = MMC_RSP_R7;

	err = sdhci_send_cmd(host, &cmd, NULL);

	if (err)
		return err;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return -1;
	else
		sd_version = 2;

	return 0;
}


static int sd_send_op_cond(void *host)
{
	int timeout = 10000;
	int err;
	struct mmc_cmd cmd;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	while (1) {
		mem_set(&cmd, 0, mmc_cmd_len);
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;

		err = sdhci_send_cmd(host, &cmd, NULL);

		if (err)
			return err;

		mem_set(&cmd, 0, mmc_cmd_len);
		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * bit15~23 is voltage window
		 * bit15  2.7-2.8V
		 * bit16  2.8-2.9V
		 * ...............
		 * bit20  3.2-3.3V
		 * bit21  3.3-3.4V
		 * bit23  3.5-3.6V
		 */
		//use 3.3V
		cmd.cmdarg = (0x300000 & 0xff8000);

		if (sd_version == 2)
			cmd.cmdarg |= OCR_HCS;

		err = sdhci_send_cmd(host, &cmd, NULL);

		if (err)
			return err;

		if (cmd.response[0] & OCR_BUSY)
			break;

		if (timeout-- <= 0)
			return -ESD_OCR_TIMEOUT;

		udelay(100);
	}

	high_capacity = ((cmd.response[0] & OCR_HCS) == OCR_HCS);

	return 0;
}

int sd_init_card(void *host, CARD_TYPE card_type)
{
	int ret;
	struct mmc_cmd cmd;
	int mmc_cmd_len = sizeof(struct mmc_cmd);

	/* set default */
	high_capacity = 0;
	rca = 0;
	udelay(1000); //mc20e after power up need wait stable of power, TBC need delay

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdarg = 0x0;
	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.resp_type = MMC_RSP_NONE;
	ret = sdhci_send_cmd(host, &cmd, NULL);

	udelay(2000);

	/* Test for SD version 2 */
	ret = mmc_send_if_cond(host);

	/* Now try to get the SD card's operating condition */
	ret = sd_send_op_cond(host);
	if (ret)
		return -EMMC_OCR;

	ret = mmc_startup(host, CARD_SD);

	return ret;
}

#ifdef WRITE_TEST
void test_mmc_rw(void *host)
{
	void *write_addr = (void *)0;
	void *read_addr = (void *)0x8000;
	int i, ret;

	for (i = 0; i < 2048; i++) {
		*(u32 *)(write_addr + i * 4) = i;
	}
	ret = mmc_write_blocks(host, 0, 16, write_addr);

	ret = mmc_read_blocks(host, read_addr, 0, 16);
}

void test_mmc_wrtie_bin(void *host)
{
	void *write_addr = (void *)0x40000000; //ddr
	// void *write_addr = (void *)0x3000000; //ocm
	int ret;

	ret = mmc_write_blocks(host, 0, 0x600, write_addr);

	//ret = mmc_read_blocks(host, read_addr, 0, 16);
}

void test_mmc_read_bin(void *host)
{
	void *read_addr = (void *)0x41000000; //ddr
	// void *read_addr = (void *)0x3100000; //ocm
	int ret;

	ret = mmc_read_blocks(host, read_addr, 0, 0x600);

	//ret = mmc_read_blocks(host, read_addr, 0, 16);
}
#endif
#if defined(AX620E_EMMC)
int emmc_init_card (void *host, CARD_TYPE card_type)
{
	int ret;
	struct mmc_cmd cmd;
	int mmc_cmd_len = sizeof(struct mmc_cmd);
	int i = 100000;
	u32 ocr = 0;
	/* set default */
	high_capacity = 0;
	rca = 1;

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdarg = 0x0;
	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.resp_type = MMC_RSP_NONE;
	ret = sdhci_send_cmd(host, &cmd, NULL);
	udelay(1000);//in kernel delay 2ms

	mem_set(&cmd, 0, mmc_cmd_len);
	cmd.cmdarg = 0x0;
	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
	cmd.resp_type = MMC_RSP_R3;
	ret = sdhci_send_cmd(host, &cmd, NULL);
	if (!ret) {
		ocr = cmd.response[0];
	} else {
		return -EMMC_OCR;
	}

	/* emmc not support 1.8V IO */
	if (ocr != 0 && !(ocr & EMMC_IO_1V8)) {
		//printf("warning: ocr not suppoer 1.8V\n");
	}

	while (i-- > 0) {
		mem_set(&cmd, 0, mmc_cmd_len);
		if(get_emmc_voltage()) {
			cmd.cmdarg = EMMC_HCS | EMMC_IO_1V8 ;
		} else {
			cmd.cmdarg = EMMC_HCS | EMMC_IO_3V3 ;
		}
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;
		ret = sdhci_send_cmd(host, &cmd, NULL);
		if (cmd.response[0] & OCR_BUSY)
			break;
		udelay(10);
	}
	if (!(cmd.response[0] & OCR_BUSY)) {
		return -EMMC_OCR;
	}

	if (cmd.response[0] & OCR_HCS)
		high_capacity = 1;

	//mmc send cmd1... to transfer mode
	ret = mmc_startup(host, CARD_EMMC);

	return ret;
}

int emmc_init(u32 flash_type, u32 clk, u32 bus_width, u8 part)
{
	int ret, i;
	int clk_cnt = sizeof(freqs) / sizeof(freqs[0]);
	u8 part_conf;
	void *sdhci_host = (void *)(EMMC_BASE + SRS_BASE_OFFSET);

	if(flash_type == FLASH_EMMC) {
		emmc_card_initialized = 1;
	}
	if (!emmc_card_initialized) {
		/* set clk source to 200M*/
		// axera_sys_glb_clk_set(CARD_EMMC);

		for (i = 0; i < clk_cnt; i++) {
			ret  = sdhci_host_init(sdhci_host, CARD_EMMC);
			if (ret) {
				//printf("host init fail\n");
				continue;
			}

			ret = sdhci_host_start(sdhci_host, CARD_EMMC);
			if (ret) {
				//printf("emmc host start fail\n");
				continue;
			}

			/* enable low than 400Khz to initialize emmc card */
			ret = sdhci_set_clock(sdhci_host, freqs[i], CLK_200M);
			if (ret) {
				//printf("set clock to %d\n fail\n", clk);
				continue;
			}
			emmc_reset(); //rst

			ret = emmc_init_card(sdhci_host, CARD_EMMC);
			if (ret) {
				//printf("emmc init card fail\n");
				continue;
			}

			/* set block len */
			ret = mmc_set_blocklen(sdhci_host, MMC_MAX_BLOCK_LEN);
			if (ret) {
				//printf("set blocklen fail\n");
				continue;
			}

			break;
		}

		if (i < clk_cnt)
			emmc_card_initialized = 1;
		else
			return -1;

	}

	/* set bus_width both card & host */
	if (current_bus_width != bus_width) {
		ret = emmc_select_bus_width(sdhci_host, bus_width);
		if (ret) {
			//printf("set card bus_width to %d\n fail\n", bus_width);
			goto dummy;
		}
		sdhci_set_bus_width(sdhci_host, bus_width);
		current_bus_width = bus_width;
	}

	/* set clock to target freq */
	if (current_clock != clk) {
		if (clk == HS_EMMC_CLK) {
			mmc_select_mode(sdhci_host, MMC_HS);
		}
		if (clk == HS400_EMMC_CLK) {
			if(current_bus_width == 8) {
				#ifdef MMC_HS400_ES_SUPPORT
				mmc_select_hs400es(sdhci_host);
				#endif
			}
			if(current_bus_width == 4) {
				#ifdef MMC_HS200_SUPPORT
				mmc_select_hs200(sdhci_host);
				#endif
			}
		} else {
			ret = sdhci_set_clock(sdhci_host, clk, CLK_200M);
			if (ret) {
				//printf("set clock to %d\n fail\n", clk);
				goto dummy;
			}
		}
		current_clock = clk;
	}

#ifdef WRITE_TEST
	// //test_mmc_rw(sdhci_host);
	// test_mmc_wrtie_bin(sdhci_host);
	// test_mmc_read_bin(sdhci_host);
#endif
	//switch partition: default part is 0, only 0->1 may happen
	if (current_part != part) {
		struct mmc_cmd cmd;
		struct mmc_data data;
		int mmc_cmd_len = sizeof(struct mmc_cmd);
		char ext_csd[EXT_CSD_LEN];
		u8 part_config;

		/* Get the Card Status Register */
		mem_set(&cmd, 0, mmc_cmd_len);
		cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;
		data.dest = ext_csd;
		data.blocks = 1;
		data.blocksize = MMC_MAX_BLOCK_LEN;
		data.flags = MMC_DATA_READ;
		ret = sdhci_send_cmd(sdhci_host, &cmd, &data);
		if (ret) {
			return -EEMMC_EXT_CSD;
		}

		part_switch_time = ext_csd[EXT_CSD_PART_SWITCH_TIME];
		part_config = ext_csd[EXT_CSD_PART_CONF];

		part_conf = (part_config & (~EXT_CSD_PARTITION_ACCESS_MASK)) |
				EXT_CSD_PARTITION_ACCESS(part);
		ret = mmc_switch(sdhci_host, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONF,
				 part_conf);
		if (ret) {
			//printf("switch boot partition %d fail\n", part);
			goto dummy;
		}
		current_part = part;
	}

dummy:
	return ret;
}
#endif

extern void sd_pins_config(void);

int sd_init(u32 clk, u32 bus_width)
{
	int ret, i;
	int clk_cnt = sizeof(freqs) / sizeof(freqs[0]);
	void *sdhci_host = (void *)(SD_MST1_BASE + SRS_BASE_OFFSET);

	if (!sd_card_initialized) {
		/* pin config */
		err("start init sd\r\n");
		/* set clk source to 200M*/
		// axera_sys_glb_clk_set(CARD_SD);

		for (i = 0; i < clk_cnt; i++) {
			ret  = sdhci_host_init(sdhci_host, CARD_SD);
			if (ret) {
				err("sd host init fail\r\n");
				continue;
			}

			ret = sdhci_host_start(sdhci_host, CARD_SD);
			if (ret) {
				err("sd host start fail\r\n");
				continue;
			}

			/* enable low than 400Khz to initialize sd card */
			ret = sdhci_set_clock(sdhci_host, freqs[i], CLK_200M);
			if (ret) {
				err("set clock to %d fail\r\n", freqs[i]);
				continue;
			}

			ret = sd_init_card(sdhci_host, CARD_SD);
			if (ret) {
				err("sd init card fail\r\n");
				continue;
			}

			ret = mmc_set_blocklen(sdhci_host, MMC_MAX_BLOCK_LEN);
			if (ret) {
				err("set blocklen fail\r\n");
				continue;
			}

			break;
		}

		if (i < clk_cnt)
			sd_card_initialized = 1;
		else
			return -1;

		err("init sd success\r\n");
	} else {
		err("sd has been init\r\n");
	}

	/* set bus_width both card & host */
	if (current_bus_width != bus_width) {
		ret = sd_select_bus_width(sdhci_host, bus_width);
		if (ret) {
			err("set sd card bus_width to %d fail\r\n", bus_width);
			goto dummy;
		}
		sdhci_set_bus_width(sdhci_host, bus_width);

		current_bus_width = bus_width;
	}

	/* set clock to target freq */
	if (current_clock != clk) {
		ret = sdhci_set_clock(sdhci_host, clk, CLK_200M);
		if (ret) {
			err("setting clock to %d fail\r\n", clk);
			goto dummy;
		}

		current_clock = clk;
	}

#ifdef WRITE_TEST
	err("mmc test\r\n");
	//test_mmc_rw(sdhci_host);
	//test_mmc_wrtie_bin(sdhci_host);
	test_mmc_read_bin(sdhci_host);
#endif

dummy:
	return ret;
}
