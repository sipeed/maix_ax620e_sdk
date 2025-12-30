#include "sdhci_cdns.h"
#include "cmn.h"
#include "chip_reg.h"
#include "timer.h"
#include "errno.h"
#include "trace.h"
#include "board.h"
#include "mmc.h"

#define TRANSFER_WITH_SDMA

#define CARD_DETECT_TIMEOUT    5

#define SDHCI_CDNS_HRS04			0x10		/* PHY access port */
#define SDHCI_CDNS_HRS04_ACK		BIT(26)
#define SDHCI_CDNS_HRS04_RD			BIT(25)
#define SDHCI_CDNS_HRS04_WR			BIT(24)
#define SDHCI_CDNS_HRS04_RDATA		GENMASK(23, 16)
#define SDHCI_CDNS_HRS04_WDATA		GENMASK(15, 8)
#define SDHCI_CDNS_HRS04_ADDR		GENMASK(5, 0)
#define SDHCI_CDNS_HRS04_WDATA_SHIFT	8
#define SDHCI_CDNS_HRS04_ADDR_SHIFT	0

#define SDHCI_CDNS_HRS06		0x18		/* eMMC control */
#define   SDHCI_CDNS_HRS06_TUNE_UP		BIT(15)
#define   SDHCI_CDNS_HRS06_TUNE			GENMASK(13, 8)
#define   SDHCI_CDNS_HRS06_MODE			GENMASK(2, 0)
#define   SDHCI_CDNS_HRS06_MODE_SD		0x0
#define   SDHCI_CDNS_HRS06_MODE_MMC_SDR		0x2
#define   SDHCI_CDNS_HRS06_MODE_MMC_DDR		0x3
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS200	0x4
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS400	0x5
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS400ES	0x6
#define   SDHCI_CDNS_HRS06_MODE_MMC_LEGACY	0x1  /* legacy mode */

/* PHY */
#define SDHCI_CDNS_PHY_DLY_SD_HS	0x00
#define SDHCI_CDNS_PHY_DLY_SD_DEFAULT	0x01
#define SDHCI_CDNS_PHY_DLY_UHS_SDR12	0x02
#define SDHCI_CDNS_PHY_DLY_UHS_SDR25	0x03
#define SDHCI_CDNS_PHY_DLY_UHS_SDR50	0x04
#define SDHCI_CDNS_PHY_DLY_UHS_DDR50	0x05
#define SDHCI_CDNS_PHY_DLY_EMMC_LEGACY	0x06
#define SDHCI_CDNS_PHY_DLY_EMMC_SDR	0x07
#define SDHCI_CDNS_PHY_DLY_EMMC_DDR	0x08
#define SDHCI_CDNS_PHY_DLY_SDCLK	0x0b  /* output delay in default mode */
#define SDHCI_CDNS_PHY_DLY_HSMMC	0x0c  /* output delay in HS200&400 mode */
#define SDHCI_CDNS_PHY_DLY_STROBE	0x0d

#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))
#define lower_32_bits(n) ((u32)(n))

static void sdhci_cdns_writel(void *host, u32 val, int reg)
{
	writel(val, (unsigned long)host + reg);
}

static void sdhci_cdns_writew(void *host, u16 val_w, int reg)
{
	writew(val_w, (unsigned long)host + reg);
}

static void sdhci_cdns_writeb(void *host, u8 val_b, int reg)
{
	writeb(val_b, (unsigned long)host + reg);
}

static u32 sdhci_cdns_readl(void *host, int reg)
{
	return readl((unsigned long)host + reg);
}

static u16 sdhci_cdns_readw(void *host, int reg)
{
	u16 val;
	val = readw((unsigned long)host + reg);
	return val;
}

static u8 sdhci_cdns_readb(void *host, int reg)
{
	u8 val;
	val = readb((unsigned long)host + reg);
	return val;
}

/*
 *	Function Description:
 *	Parameter:
 *	Return:
 */
static void sdhci_host_int_stat_config(void *host)
{
	/* int stat enable */
	sdhci_cdns_writel(host, 0x27F003B, SDHCI_NORMAL_INT_STAT_EN_R);
	/* not signal */
	sdhci_cdns_writel(host, 0, SDHCI_NORMAL_INT_SIGNAL_EN_R);
}

static void sdhci_reset(void *host, u8 mask)
{
	unsigned long timeout;

	/* Wait max 100 ms */
	timeout = 1000;

	sdhci_cdns_writeb(host, mask, SDHCI_SW_RST_R);

	/* after reset the bit is cleared by hw */
	while (sdhci_cdns_readb(host, SDHCI_SW_RST_R) & mask) {
		if (timeout == 0) {
			//printf warning
			return;
		}
		timeout--;

		udelay(10);
	}
}

/*
 *	Function Description:
 *	Parameter:
 *	Return:
 */
int sdhci_host_init_config(void *host, CARD_TYPE card_type)
{
	sdhci_reset(host, BIT_SDHCI_RESET_ALL);
	sdhci_host_int_stat_config(host);
	return 0;
}

/*
 *	Function Description:
 *	Parameter:
 *	Return:
 */
u16 sdhci_cmddata_set(struct mmc_cmd *cmd, struct mmc_data *data)
{
	u16 cmddata;

	cmddata = BITS_SDHCI_CMD_INDEX(cmd->cmdidx);

	if (!(cmd->resp_type & MMC_RSP_PRESENT)) {
		cmddata |= BITS_SDHCI_CMD_RESP_TYPE_SELECT(CMD_RESP_TYPE_NO_RESP);
	} else if (cmd->resp_type & MMC_RSP_136) {
		cmddata |= BITS_SDHCI_CMD_RESP_TYPE_SELECT(CMD_RESP_TYPE_RESP_LEN_136);
	} else if (cmd->resp_type & MMC_RSP_BUSY) {
		cmddata |= BITS_SDHCI_CMD_RESP_TYPE_SELECT(CMD_RESP_TYPE_RESP_LEN_48B);
	} else {
		cmddata |= BITS_SDHCI_CMD_RESP_TYPE_SELECT(CMD_RESP_TYPE_RESP_LEN_48);
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		cmddata |= BIT_SDHCI_CMD_CRC_CHK_ENABLE;

	if (cmd->resp_type & MMC_RSP_OPCODE)
		cmddata |= BIT_SDHCI_CMD_INDEX_CHK_ENABLE;

	if (data)
		cmddata |= BIT_SDHCI_CMD_DATA_PRESENT_SEL;

	return cmddata;
}

/*
 *	Function Description: data transefer per io
 *	Parameter:
 *	Return:
 */
static void sdhci_transfer_pio(void *host, struct mmc_data *data)
{
	int i;
	char *offs;
	for (i = 0; i < data->blocksize; i += 4) {
		offs = data->dest + i;
		if (data->flags == MMC_DATA_READ)
			*(u32 *)offs = sdhci_cdns_readl(host, SDHCI_BUF_DATA_R);
		else
			sdhci_cdns_writel(host, *(u32 *)offs, SDHCI_BUF_DATA_R);
	}
}

/*
 *	Function Description: data transefer
 *	Parameter:
 *	Return:
 */
#if 0 //debug later
static int dummy_cnt = 0;
#endif

int sdhci_transfer_data(void *host, struct mmc_data *data)
{
	unsigned int rdy, mask, block = 0;
	int timeout;
	int transfer_done = 0;
	u16 stat;
#ifdef TRANSFER_WITH_SDMA
	long start_addr;
	u32 temp_u32;
#endif

#ifdef TRANSFER_WITH_SDMA
	if (data->flags == MMC_DATA_READ)
		start_addr = (long)data->dest;
	else
		start_addr = (long)data->src;
#endif
	timeout = 1000000;
	rdy = BIT_SDHCI_NORMAL_INT_BUF_RD_READY |
			BIT_SDHCI_NORMAL_INT_BUF_WR_READY;
	mask = BIT_SDHCI_PSTATE_BUF_RD_ENABLE |
			BIT_SDHCI_PSTATE_BUF_WR_ENABLE;
	do {
		stat = sdhci_cdns_readw(host, SDHCI_NORMAL_INT_STAT_R);

		// printf("%s: stat:0x%x\n",__func__, stat);
		if (stat & BIT_SDHCI_NORMAL_INT_ERR_INTERRUPT) {
			return -ETRANS_ERRINT;
		}

		if (!transfer_done && (stat & rdy)) {
			if (!(sdhci_cdns_readl(host, SDHCI_PSTATE_REG) & mask))
				continue;
			sdhci_cdns_writew(host, rdy, SDHCI_NORMAL_INT_STAT_R);
			sdhci_transfer_pio(host, data);
			data->dest += data->blocksize;
#if 0    //debug
			//for debug
			if (data->blocks > 1)
				writel(dummy_cnt++, (void *)DUMMY2_ADDR);
#endif
			if (++block >= data->blocks) {
				/* Keep looping until the SDHCI_INT_DATA_END is
				 * cleared, even if we finished sending all the
				 * blocks.
				 */
				transfer_done = 1;
				continue;
			}
		}
#ifdef TRANSFER_WITH_SDMA
		if (stat & BIT_SDHCI_NORMAL_INT_DMA_INTERRUPT) {
			//clear interrupt
			sdhci_cdns_writel(host, BIT_SDHCI_NORMAL_INT_DMA_INTERRUPT,
								SDHCI_NORMAL_INT_STAT_R);

			//set next boundary addr
			start_addr &= ~(DEFAULT_BOUNDARY_SIZE - 1);
			start_addr += DEFAULT_BOUNDARY_SIZE;

			if (sdhci_cdns_readw(host, SDHCI_HOST_CTRL2_R) &
				BIT_SDHCI_HOST_CTRL2_HOST_VER4_ENABLE) {
				temp_u32 = lower_32_bits(start_addr) & 0xFFFFFFFF;
				sdhci_cdns_writel(host, temp_u32, ADMA_SA_LOW_R);
				temp_u32 = upper_32_bits(start_addr) & 0xFFFFFFFF;
				sdhci_cdns_writel(host, temp_u32, ADMA_SA_HIGH_R);
			} else {
				sdhci_cdns_writel(host, (u32)start_addr, SDHCI_SDMASA_R);
			}
		}
#endif

		if (timeout-- > 0) {
			udelay(10);
		} else {
			return -ETRANS_TIMEOUT;
		}
	} while (!(stat & BIT_SDHCI_NORMAL_INT_STAT_XFER_COMPLETE));

	return 0;
}

#ifdef TRANSFER_WITH_SDMA
/*
 *	Function Description: send cmd& data transefer
 *	Parameter:
 *	Return:
 */
void sdhci_prepare_sdma(void *host, struct mmc_data *data)
{
	u8 ctrl;
	long start_addr;

	if (data->flags == MMC_DATA_READ)
		start_addr = (long)data->dest;
	else
		start_addr = (long)data->src;

	//select SDMA:0 bit3-4
	ctrl = sdhci_cdns_readb(host, SDHCI_HOST_CTRL1_R);
	ctrl &= ~(BITS_SDHCI_HOST_CTRL1_DMA_SEL(3));
	sdhci_cdns_writeb(host, ctrl, SDHCI_HOST_CTRL1_R);

	//set SDMA addr
	sdhci_cdns_writel(host, (u32)start_addr, SDHCI_SDMASA_R);
}
#endif

/*
 *	Function Description: response
 *	Parameter:
 *			  resp67
 *	|0x00|127-120|119-112|111-104|
 *	Return:
 */
static void sdhci_cmd_done(void *host, struct mmc_cmd *cmd)
{
	int i;

	if (cmd->resp_type & MMC_RSP_136) {
		/* CRC is stripped so we need to do some shifting. */
		for (i = 0; i < 4; i++) {
			cmd->response[i] =
				sdhci_cdns_readl(host, (SDHCI_RESP01_R + (3 - i) * 4)) << 8;
			if (i != 3)
				cmd->response[i] |=
					sdhci_cdns_readb(host, SDHCI_RESP01_R + (3 - i) * 4 - 1);
		}
	} else {
		cmd->response[0] = sdhci_cdns_readl(host, SDHCI_RESP01_R);
	}
}

#define CMD_DEFAULT_TIMEOUT		100
#define CMD_MAX_TIMEOUT			32000
/*
 *	Function Description: send cmd& data transefer
 *	Parameter:
 *	Return:
 */
int sdhci_send_cmd(void *host, struct mmc_cmd *cmd, struct mmc_data *data)
{
	u16 cmddata;
	u16 temp_u16;
	u16 trans_mode = 0;
	int ret = 0;
	u32 time = 0;
	u32 mask, stat;

	/* timeout unit ms */
	static unsigned int cmd_timeout = CMD_DEFAULT_TIMEOUT;

	mask = SDHCI_DATA_INHIBIT | SDHCI_CMD_INHIBIT;
	/* We shouldn't wait for data inihibit for stop commands, even
	   though they might use busy signaling */
	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		mask &= ~SDHCI_DATA_INHIBIT;

	/* check data/cmd ready */
	while (sdhci_cdns_readl(host, SDHCI_PSTATE_REG) & mask) {
		if (time >= cmd_timeout) {
			if (2 * cmd_timeout <= CMD_MAX_TIMEOUT) {
				cmd_timeout += cmd_timeout;
			} else {
				return -EINHIBIT_TIMEOUT;
			}
		}
		time++;

		udelay(100);
	}

	/* clear all normal and error interrupt status */
	sdhci_cdns_writel(host, SDHCI_INT_ALL_MASK, SDHCI_NORMAL_INT_STAT_R);

	mask = BIT_SDHCI_NORMAL_INT_STAT_CMD_COMPLETE;
	if ((cmd->resp_type & MMC_RSP_BUSY) && data)
		mask |= BIT_SDHCI_NORMAL_INT_STAT_XFER_COMPLETE;

	//add data handle before send cmd
	if (data) {
		sdhci_cdns_writeb(host, BITS_SDHCI_TOUT_CTRL_TOUT_CNT(0xE),
							SDHCI_TOUT_CTRL_R);

		trans_mode = BIT_SDHCI_XFER_MODE_BLOCK_COUNT_ENABLE;
		if (data->blocks > 1)
			trans_mode |= BIT_SDHCI_XFER_MODE_MULTI_BLK_SEL;

		if (data->flags == MMC_DATA_READ)
			trans_mode |= BIT_SDHCI_XFER_MODE_DATA_XFER_DIR; //read set 1

		//if use dma
	#ifdef TRANSFER_WITH_SDMA
			trans_mode |= BIT_SDHCI_XFER_MODE_DMA_ENABLE;
			sdhci_prepare_sdma(host, data);
	#endif

		//set block_size
		temp_u16 = BITS_SDHCI_BLOCKSIZE_SDMA_BUF_BDARY(DEFAULT_BOUNDARY_ARG) |
					BITS_SDHCI_BLOCKSIZE_XFER_BLOCK_SIZE(data->blocksize);
		sdhci_cdns_writew(host, temp_u16, SDHCI_BLOCKSIZE_R);

		//set block count BLOCKCOUNT_R is only 16bits
		if (data->blocks & 0xFFFF0000) {
			return -EMMC_BLKCNT_OVER;
		}
		sdhci_cdns_writew(host, (u16)data->blocks, SDHCI_BLOCKCOUNT_R);

		//set transfer mode
		//sdhci_cdns_writew(host, trans_mode, SDHCI_XFER_MODE_R);
	} else if (cmd->resp_type & MMC_RSP_BUSY) {
		sdhci_cdns_writeb(host, BITS_SDHCI_TOUT_CTRL_TOUT_CNT(0xe),
							SDHCI_TOUT_CTRL_R);
	}

	//set ARGUMENT_R
	sdhci_cdns_writel(host, cmd->cmdarg, SDHCI_ARGUMENT_R);

	//set CMD_R & transfer mode together for cadence special 4B align
	cmddata = sdhci_cmddata_set(cmd, data);
	//sdhci_cdns_writew(host, cmddata, SDHCI_CMD_R);
	sdhci_cdns_writel(host, (u32)(trans_mode | (cmddata << 16)), SDHCI_XFER_MODE_R);

	//wait cmd complete status max 1sec
	time = 50000;
	do {
		stat = sdhci_cdns_readw(host, SDHCI_NORMAL_INT_STAT_R);
		if (stat & BIT_SDHCI_NORMAL_INT_ERR_INTERRUPT)
			break;

		if (0 == time) {
			return -ECMDCOMPLETE_TIMEOUT;
		}

		udelay(20);
		time--;
	} while ((stat & mask) != mask);

	if ((stat & (BIT_SDHCI_NORMAL_INT_ERR_INTERRUPT | mask)) == mask) {
		sdhci_cmd_done(host, cmd);
		sdhci_cdns_writel(host, mask, SDHCI_NORMAL_INT_STAT_R);
	} else
		ret = -ECMD_RESP;

	if (!ret && data)
		ret = sdhci_transfer_data(host, data);

	/* clear all normal and error interrupt status */
	sdhci_cdns_writel(host, SDHCI_INT_ALL_MASK, SDHCI_NORMAL_INT_STAT_R);
	if (!ret) {
		return 0;
	}

	sdhci_reset(host, BIT_SDHCI_SW_RST_CMD);
	sdhci_reset(host, BIT_SDHCI_SW_RST_DAT);

	return ret;
}

/*
 *	Function Description:
 *	Parameter:
 *	Return:
 */

int sdhci_set_clock(void *host, unsigned int clock, unsigned int base_clock)
{
	unsigned int div, clk = 0, timeout;

	/* Wait max 20 ms */
	timeout = 2000;
	while (sdhci_cdns_readl(host, SDHCI_PSTATE_REG) &
			   (SDHCI_CMD_INHIBIT | SDHCI_DATA_INHIBIT)) {
		if (timeout == 0) {
			//printf("%s: Timeout to wait cmd & data inhibit\n",__func__);
			return -1;
		}

		timeout--;
		udelay(10);
	}

	sdhci_cdns_writew(host, 0, SDHCI_CLK_CTRL_R);

	if (clock == 0)
		return 0;

	if (base_clock <= clock) {
		div = 1;
	} else {
		for (div = 2;
		     div < SDHCI_MAX_DIV_SPEC_300;
		     div += 2) {
			if ((base_clock / div) <= clock)
				break;
		}
	}

	div >>= 1;

	clk |= (div & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
	clk |= ((div & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN)
		<< SDHCI_DIVIDER_HI_SHIFT;
	#if (TEST_PLATFORM == HAPS)
	if (clock == 10000000)
		clk = 0;
	#endif
	clk |= SDHCI_CLOCK_INT_EN;
	sdhci_cdns_writew(host, (u16)clk, SDHCI_CLK_CTRL_R);

	/* Wait max 20 ms */
	timeout = 2000;
	while (!((clk = sdhci_cdns_readw(host, SDHCI_CLK_CTRL_R))
		& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			//printf("%s: Internal clock never stabilised.\n", __func__);
			return -1;
		}
		timeout--;
		udelay(10);
	}

	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_cdns_writew(host, clk, SDHCI_CLK_CTRL_R);
	return 0;
}

static int sdhci_cdns_write_phy_reg(void *hrs_addr, u8 addr, u8 data)
{
	void *reg = hrs_addr + SDHCI_CDNS_HRS04;
	u32 tmp;
	int i;
	int retry = 5;

	tmp = ((data << SDHCI_CDNS_HRS04_WDATA_SHIFT) & SDHCI_CDNS_HRS04_WDATA) |
			((addr << SDHCI_CDNS_HRS04_ADDR_SHIFT) & SDHCI_CDNS_HRS04_ADDR);
	writel(tmp, (unsigned long)reg);

	tmp |= SDHCI_CDNS_HRS04_WR;
	writel(tmp, (unsigned long)reg);

	for (i = 0; i < retry; i++) {
		tmp = readl((unsigned long)reg);
		if (tmp & SDHCI_CDNS_HRS04_ACK)
			break;

		udelay(10);
	}

	if (!(tmp & SDHCI_CDNS_HRS04_ACK))
		return -1;  //timeout and not recevie ack

	tmp &= ~SDHCI_CDNS_HRS04_WR;
	writel(tmp, (unsigned long)reg);

	return 0;
}

/*
 *	Function Description: phy init
 *	Parameter:
 *	Return:
 */

int sdhci_phy_init(void *hrs_addr, CARD_TYPE card_type)
{
	int ret = -1;
	/* hs input delay */
	ret = sdhci_cdns_write_phy_reg(hrs_addr, SDHCI_CDNS_PHY_DLY_SD_HS, 2);

	/* ds input delay */
	ret += sdhci_cdns_write_phy_reg(hrs_addr, SDHCI_CDNS_PHY_DLY_SD_DEFAULT, 18);

	/* mmc legacy mode input delay */
	ret += sdhci_cdns_write_phy_reg(hrs_addr, SDHCI_CDNS_PHY_DLY_EMMC_LEGACY, 10);

	/* mmc sdr input delay */
	ret += sdhci_cdns_write_phy_reg(hrs_addr, SDHCI_CDNS_PHY_DLY_EMMC_SDR, 2);

	ret += sdhci_cdns_write_phy_reg(hrs_addr, SDHCI_CDNS_PHY_DLY_SDCLK, 45);
	ret += sdhci_cdns_write_phy_reg(hrs_addr, SDHCI_CDNS_PHY_DLY_HSMMC, 23);
	ret += sdhci_cdns_write_phy_reg(hrs_addr, SDHCI_CDNS_PHY_DLY_STROBE, 18);

	return ret;
}

/*
 *	Function Description: host init
 *	Parameter:
 *	Return:
 */
int sdhci_host_init(void *host, CARD_TYPE card_type)
{
	/* host controller init */
	sdhci_host_init_config(host, card_type);

	udelay(1000);
	return 0;
}

void sdhci_set_bus_width(void *host, int width)
{
	u8 ctrl;

	ctrl = sdhci_cdns_readb(host, SDHCI_HOST_CTRL1_R);
	if (width == 8) {
		ctrl &= ~SDHCI_CTRL_4BITBUS;
		ctrl |= SDHCI_CTRL_8BITBUS;
	} else {
		ctrl &= ~SDHCI_CTRL_8BITBUS;

		if (width == 4)
			ctrl |= SDHCI_CTRL_4BITBUS;
		else
			ctrl &= ~SDHCI_CTRL_4BITBUS;
	}

	sdhci_cdns_writeb(host, ctrl, SDHCI_HOST_CTRL1_R);
}

/*
 *	Function Description: host signal voltage emmc 1.8V and sd 3.3v
 *	Parameter:
 *	Return:
 */
int sdhci_set_signal_voltage(void *host, CARD_TYPE card_type)
{
	u16 ctrl;

	ctrl = sdhci_cdns_readw(host, SDHCI_HOST_CTRL2_R);

	switch (card_type) {
	case CARD_SD:
		/* Set 1.8V Signal Enable in the Host Control2 register to 0 */
		ctrl &= ~SDHCI_CTRL_VDD_180;
		sdhci_cdns_writew(host, ctrl, SDHCI_HOST_CTRL2_R);

#if 0
		/* Wait for 5ms */
		udelay(5000);
#endif
		/* 3.3V regulator output should be stable within 5 ms */
		ctrl = sdhci_cdns_readw(host, SDHCI_HOST_CTRL2_R);
		if (!(ctrl & SDHCI_CTRL_VDD_180))
			return 0;

		//printf warning

		return -1;
#if 0
	case CARD_EMMC:
		/* */
		/* Enable 1.8V Signal Enable in the Host Control2 register */
		ctrl |= SDHCI_CTRL_VDD_180;
		sdhci_cdns_writew(host, ctrl, SDHCI_HOST_CTRL2_R);
#if 0
		/* Wait for 5ms */
		udelay(5000);
#endif
		ctrl = sdhci_cdns_readw(host, SDHCI_HOST_CTRL2_R);
		if (ctrl & SDHCI_CTRL_VDD_180)
			return 0;

		//printf warning

		return -1;
#endif
	default:
		/* No signal voltage switch required */
		return 0;
	}
}

void sdhci_set_power(void *host, CARD_TYPE card_type)
{
	u8 pwr = 0;

	/* this BVS - SD Bus Voltage Select is invalid, only BP - SD Bus Power for VDD1 is valid. 3.3V or 1.8V select depends on the external pinmux configuration */
	switch (card_type) {
	case CARD_SD:
		pwr = SDHCI_POWER_330;
		break;
	case CARD_EMMC:
		if(get_emmc_voltage()) {
			pwr = SDHCI_POWER_180;
		} else {
			pwr = SDHCI_POWER_330;
		}
		break;
	default:
		//printf("CARD type is not right, default signal voltage is used\n");
		break;
	}

	pwr |= SDHCI_POWER_ON;

	sdhci_cdns_writeb(host, pwr, SDHCI_PWR_CTRL_R);
}

/*
 *	Function Description: host start
 *	Parameter:
 *	Return:
 */
int sdhci_host_start(void *host, CARD_TYPE card_type)
{
	int ret = 0;
	u8 ctrl = 0;
	u32 temp, addr;

	/* power up vdd1 bus voltage */
	/* in our design in fact power on emmc/sd bus voltage to io voltage
	   and sdio_power_en/emmc_power_en is controlled by bit0 of 0x29*/
	sdhci_set_power(host, card_type);

	/* set initial state buswidth, legacy mode, SDMA */
	sdhci_set_bus_width(host, 1);
	ctrl = sdhci_cdns_readb(host, SDHCI_HOST_CTRL1_R);
	ctrl &= ~SDHCI_CTRL_HISPD;
	ctrl &= ~SDHCI_CTRL_DMA_MASK; //select SDMA
	sdhci_cdns_writeb(host, ctrl, SDHCI_HOST_CTRL1_R);

	/* set initial io voltage SD_POWER_SW, do need config */
	// ret = sdhci_set_signal_voltage(host, card_type);
	// udelay(5000);

	/* cdns special HRS EMM mode config
	 * HRS06 bit0~2:0 V18SE:0 HSE:0 SD default speed
	 * HRS06 bit0~2:1 V18SE:- HSE:- EMMC legacy speed
	 */
	if (CARD_EMMC == card_type) {
		addr = (unsigned long)host - SRS_BASE_OFFSET + SDHCI_CDNS_HRS06;
		temp = readl(addr);
		temp &= ~SDHCI_CDNS_HRS06_MODE;
		temp |= SDHCI_CDNS_HRS06_MODE_MMC_LEGACY;
		writel(temp, addr);
	} else if (CARD_SD == card_type) {
		addr = (unsigned long)host - SRS_BASE_OFFSET + SDHCI_CDNS_HRS06;
		temp = readl(addr);
		temp &= ~SDHCI_CDNS_HRS06_MODE;
		writel(temp, addr);
	}

	/* phy init after controller reset before clock enable */
	ret += sdhci_phy_init(host - SRS_BASE_OFFSET, card_type);

	return ret;
}
void mmc_select_mode(void *host, enum bus_mode mode)
{
	u16 reg;

	reg = sdhci_cdns_readw((host - SRS_BASE_OFFSET), SDHCI_CDNS_HRS06);
	reg &= ~SDHCI_CTRL_UHS_MASK;

	switch (mode) {
	case MMC_LEGACY:
		reg |= MMC_TIMING_MMC_LEGACY;
		break;
	case MMC_HS:
		reg |= MMC_TIMING_MMC_HS;
		break;
	case MMC_HS_200:
		reg |= MMC_TIMING_MMC_HS200;
		break;
	case MMC_HS_400:
		reg |= MMC_TIMING_MMC_HS400;
		break;
	case MMC_HS_400_ES:
		reg |= MMC_TIMING_MMC_HS400_ES;
		break;
	default:
		break;
	}
	sdhci_cdns_writew((host - SRS_BASE_OFFSET), reg, SDHCI_CDNS_HRS06);
}

int sdhci_cdns_set_tune_val(void *host, unsigned int val)
{
	void *reg = (host - SRS_BASE_OFFSET) + SDHCI_CDNS_HRS06;
	u32 tmp;
	int i;
	int timeout = 1000;

	tmp = readl((unsigned long)reg);
	tmp &= ~SDHCI_CDNS_HRS06_TUNE;
	tmp |= val << 8;
	/*
	 * Workaround for IP errata:
	 * The IP6116 SD/eMMC PHY design has a timing issue on receive data
	 * path. Send tune request twice.
	 */
	for (i = 0; i < 2; i++) {
		tmp |= SDHCI_CDNS_HRS06_TUNE_UP;
		writel(tmp, (unsigned long)reg);

		do {
			tmp = readl((unsigned long)reg);
			if (timeout == 0)
				return -1;

			timeout--;
			udelay(10);
		} while(tmp & SDHCI_CDNS_HRS06_TUNE_UP);
	}
	return 0;
}