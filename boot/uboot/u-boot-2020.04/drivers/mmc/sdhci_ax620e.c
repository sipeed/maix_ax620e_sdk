// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Socionext Inc.
 *   Author: Masahiro Yamada <yamada.masahiro@socionext.com>
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <linux/bitfield.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/sizes.h>
#include <linux/libfdt.h>
#include <mmc.h>
#include <sdhci.h>
#include <linux/dma-mapping.h>
#include <asm/arch/ax620e.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch/ax620e.h>

/* HRS - Host Register Set (specific to Cadence) */
#define SDHCI_CDNS_HRS04		0x10		/* PHY access port */
#define   SDHCI_CDNS_HRS04_ACK			BIT(26)
#define   SDHCI_CDNS_HRS04_RD			BIT(25)
#define   SDHCI_CDNS_HRS04_WR			BIT(24)
#define   SDHCI_CDNS_HRS04_RDATA		GENMASK(23, 16)
#define   SDHCI_CDNS_HRS04_WDATA		GENMASK(15, 8)
#define   SDHCI_CDNS_HRS04_ADDR			GENMASK(5, 0)

#define SDHCI_CDNS_HRS06		0x18		/* eMMC control */
#define   SDHCI_CDNS_HRS06_TUNE_UP		BIT(15)
#define   SDHCI_CDNS_HRS06_TUNE			GENMASK(13, 8)
#define   SDHCI_CDNS_HRS06_MODE			GENMASK(2, 0)
#define   SDHCI_CDNS_HRS06_MODE_SD		0x0
#define   SDHCI_CDNS_HRS06_MODE_MMC_LEGACY	0x1
#define   SDHCI_CDNS_HRS06_MODE_MMC_SDR		0x2
#define   SDHCI_CDNS_HRS06_MODE_MMC_DDR		0x3
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS200	0x4
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS400	0x5
#define   SDHCI_CDNS_HRS06_MODE_MMC_HS400ES	0x6

/* SRS - Slot Register Set (SDHCI-compatible) */
#define SDHCI_CDNS_SRS_BASE		0x200

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
#define SDHCI_CDNS_PHY_DLY_SDCLK	0x0b
#define SDHCI_CDNS_PHY_DLY_HSMMC	0x0c
#define SDHCI_CDNS_PHY_DLY_STROBE	0x0d
#define SDHCI_CDNS_PHY_DLL_RESET       0x0f

/*
 * The tuned val register is 6 bit-wide, but not the whole of the range is
 * available.  The range 0-42 seems to be available (then 43 wraps around to 0)
 * but I am not quite sure if it is official.  Use only 0 to 39 for safety.
 */
#define SDHCI_CDNS_MAX_TUNING_LOOP	40

#define PINMUX_SD_FUNCTION_BASE  0x104F1000
#define SD_PIN_DATA0  (0xC)
#define SD_PIN_DATA1  (0x18)
#define SD_PIN_DATA2  (0x3C)
#define SD_PIN_DATA3  (0x48)
#define SD_PIN_CMD  (0x30)
#define SD_PIN_CLK  (0x24)


struct sdhci_cdns_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	void __iomem *hrs_addr;
};

struct sdhci_cdns_phy_cfg {
	const char *property;
	u8 addr;
};

/* mode = 1 pull up , mode = 0 pull down*/
static void set_sd_pad_state(int mode)
{
	u32 val;
	void *addr = NULL;
	u8 pad_state;

	if(mode) { /* pull up */
		pad_state = 0x2;
	} else { /* pull down */
		pad_state = 0x1;
	}

	addr = (void *)PINMUX_SD_FUNCTION_BASE;

	val = readl(addr + SD_PIN_CMD);
	val &= ~GENMASK(7,6);
	val |= (pad_state << 6);
	writel(val, addr + SD_PIN_CMD);

	val = readl(addr + SD_PIN_DATA0);
	val &= ~GENMASK(7,6);
	val |= (pad_state << 6);
	writel(val, addr + SD_PIN_DATA0);

	val = readl(addr + SD_PIN_DATA1);
	val &= ~GENMASK(7,6);
	val |= (pad_state << 6);
	writel(val, addr + SD_PIN_DATA1);

	val = readl(addr + SD_PIN_DATA2);
	val &= ~GENMASK(7,6);
	val |= (pad_state << 6);
	writel(val, addr + SD_PIN_DATA2);

	val = readl(addr + SD_PIN_DATA3);
	val &= ~GENMASK(7,6);
	val |= (pad_state << 6);
	writel(val, addr + SD_PIN_DATA3);
}

# if 0
static int sdhci_cdns_write_phy_reg_legacy(void *hrs_addr, u8 addr, u8 data)
{
	void __iomem *reg = hrs_addr + SDHCI_CDNS_HRS04;
	u32 tmp;
	int ret;
	tmp = FIELD_PREP(SDHCI_CDNS_HRS04_WDATA, data) |
	      FIELD_PREP(SDHCI_CDNS_HRS04_ADDR, addr);
	writel(tmp, reg);
	tmp |= SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, tmp & SDHCI_CDNS_HRS04_ACK, 10);
	if (ret)
		return ret;

	tmp &= ~SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	return 0;
}
#endif
int sdhci_cdns_read_phy_reg(void *hrs_addr, u8 addr)
{
	void __iomem *reg = (void __iomem *)(hrs_addr + SDHCI_CDNS_HRS04);
	u32 tmp;
	int ret;

	tmp = readl(reg);
	tmp = tmp & 0xffffff00;
	tmp = tmp | addr;
	/* set address */
	writel(tmp, reg);

	tmp |= SDHCI_CDNS_HRS04_RD;
	/* send read request */
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, tmp & SDHCI_CDNS_HRS04_ACK, 10);
	if (ret)
		return ret;

	tmp &= ~SDHCI_CDNS_HRS04_RD;
	/* clear read request */
	writel(0, reg);
	tmp = tmp >> 16;

	return tmp;
}

static void sdhci_reset(struct sdhci_host *host, u8 mask)
{
	unsigned long timeout;

	/* Wait max 100 ms */
	timeout = 100;
	sdhci_writeb(host, mask, SDHCI_SOFTWARE_RESET);
	while (sdhci_readb(host, SDHCI_SOFTWARE_RESET) & mask) {
		if (timeout == 0) {
			printf("%s: Reset 0x%x never completed.\n",
			       __func__, (int)mask);
			return;
		}
		timeout--;
		udelay(1000);
	}
}

static void sdhci_cmd_done(struct sdhci_host *host, struct mmc_cmd *cmd)
{
	int i;
	if (cmd->resp_type & MMC_RSP_136) {
		/* CRC is stripped so we need to do some shifting. */
		for (i = 0; i < 4; i++) {
			cmd->response[i] = sdhci_readl(host,
					SDHCI_RESPONSE + (3-i)*4) << 8;
			if (i != 3)
				cmd->response[i] |= sdhci_readb(host,
						SDHCI_RESPONSE + (3-i)*4-1);
		}
	} else {
		cmd->response[0] = sdhci_readl(host, SDHCI_RESPONSE);
	}
}

static void sdhci_transfer_pio(struct sdhci_host *host, struct mmc_data *data)
{
	int i;
	char *offs;
	for (i = 0; i < data->blocksize; i += 4) {
		offs = data->dest + i;
		if (data->flags == MMC_DATA_READ)
			*(u32 *)offs = sdhci_readl(host, SDHCI_BUFFER);
		else
			sdhci_writel(host, *(u32 *)offs, SDHCI_BUFFER);
	}
}

#if CONFIG_IS_ENABLED(MMC_SDHCI_ADMA)
static void sdhci_adma_desc(struct sdhci_host *host, dma_addr_t dma_addr,
			    u16 len, bool end)
{
	struct sdhci_adma_desc *desc;
	u8 attr;

	desc = &host->adma_desc_table[host->desc_slot];

	attr = ADMA_DESC_ATTR_VALID | ADMA_DESC_TRANSFER_DATA;
	if (!end)
		host->desc_slot++;
	else
		attr |= ADMA_DESC_ATTR_END;

	desc->attr = attr;
	desc->len = len;
	desc->reserved = 0;
	desc->addr_lo = lower_32_bits(dma_addr);
#ifdef CONFIG_DMA_ADDR_T_64BIT
	desc->addr_hi = upper_32_bits(dma_addr);
#endif
}

static void sdhci_prepare_adma_table(struct sdhci_host *host,
				     struct mmc_data *data)
{
	uint trans_bytes = data->blocksize * data->blocks;
	uint desc_count = DIV_ROUND_UP(trans_bytes, ADMA_MAX_LEN);
	int i = desc_count;
	dma_addr_t dma_addr = host->start_addr;

	host->desc_slot = 0;

	while (--i) {
		sdhci_adma_desc(host, dma_addr, ADMA_MAX_LEN, false);
		dma_addr += ADMA_MAX_LEN;
		trans_bytes -= ADMA_MAX_LEN;
	}

	sdhci_adma_desc(host, dma_addr, trans_bytes, true);

	flush_cache((dma_addr_t)host->adma_desc_table,
		    ROUND(desc_count * sizeof(struct sdhci_adma_desc),
			  ARCH_DMA_MINALIGN));
}
#elif defined(CONFIG_MMC_SDHCI_SDMA)
static void sdhci_prepare_adma_table(struct sdhci_host *host,
				     struct mmc_data *data)
{}
#endif
#if (defined(CONFIG_MMC_SDHCI_SDMA) || CONFIG_IS_ENABLED(MMC_SDHCI_ADMA))
static void sdhci_prepare_dma(struct sdhci_host *host, struct mmc_data *data,
			      int *is_aligned, int trans_bytes)
{
	unsigned char ctrl;
	void *buf;

	if (data->flags == MMC_DATA_READ)
		buf = data->dest;
	else
		buf = (void *)data->src;

	ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);
	ctrl &= ~SDHCI_CTRL_DMA_MASK;
	if (host->flags & USE_ADMA64)
		ctrl |= SDHCI_CTRL_ADMA64;
	else if (host->flags & USE_ADMA)
		ctrl |= SDHCI_CTRL_ADMA32;
	sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);

	if (host->flags & USE_SDMA &&
	    (host->force_align_buffer ||
	     (host->quirks & SDHCI_QUIRK_32BIT_DMA_ADDR &&
	      ((unsigned long)buf & 0x7) != 0x0))) {
		*is_aligned = 0;
		if (data->flags != MMC_DATA_READ)
			memcpy(host->align_buffer, buf, trans_bytes);
		buf = host->align_buffer;
	}

	host->start_addr = dma_map_single(buf, trans_bytes,
					  mmc_get_dma_dir(data));

	if (host->flags & USE_SDMA) {
		sdhci_writel(host, host->start_addr, SDHCI_DMA_ADDRESS);
	} else if (host->flags & (USE_ADMA | USE_ADMA64)) {
		sdhci_prepare_adma_table(host, data);

		sdhci_writel(host, lower_32_bits(host->adma_addr),
			     SDHCI_ADMA_ADDRESS);
		if (host->flags & USE_ADMA64)
			sdhci_writel(host, upper_32_bits(host->adma_addr),
				     SDHCI_ADMA_ADDRESS_HI);
	}
}
#else
static void sdhci_prepare_dma(struct sdhci_host *host, struct mmc_data *data,
			      int *is_aligned, int trans_bytes)
{}
#endif
static int sdhci_transfer_data(struct sdhci_host *host, struct mmc_data *data)
{
	dma_addr_t start_addr = host->start_addr;
	unsigned int stat, rdy, mask, timeout, block = 0;
	bool transfer_done = false;

	timeout = 1000000;
	rdy = SDHCI_INT_SPACE_AVAIL | SDHCI_INT_DATA_AVAIL;
	mask = SDHCI_DATA_AVAILABLE | SDHCI_SPACE_AVAILABLE;
	do {
		stat = sdhci_readl(host, SDHCI_INT_STATUS);
		if (stat & SDHCI_INT_ERROR) {
			pr_debug("%s: Error detected in status(0x%X)!\n",
				 __func__, stat);
			return -EIO;
		}
		if (!transfer_done && (stat & rdy)) {
			if (!(sdhci_readl(host, SDHCI_PRESENT_STATE) & mask))
				continue;
			sdhci_writel(host, rdy, SDHCI_INT_STATUS);
			sdhci_transfer_pio(host, data);
			data->dest += data->blocksize;
			if (++block >= data->blocks) {
				/* Keep looping until the SDHCI_INT_DATA_END is
				 * cleared, even if we finished sending all the
				 * blocks.
				 */
				transfer_done = true;
				continue;
			}
		}
		if ((host->flags & USE_DMA) && !transfer_done &&
		    (stat & SDHCI_INT_DMA_END)) {
			sdhci_writel(host, SDHCI_INT_DMA_END, SDHCI_INT_STATUS);
			if (host->flags & USE_SDMA) {
				start_addr &=
				~(SDHCI_DEFAULT_BOUNDARY_SIZE - 1);
				start_addr += SDHCI_DEFAULT_BOUNDARY_SIZE;
				sdhci_writel(host, start_addr,
					     SDHCI_DMA_ADDRESS);
			}
		}
		if (timeout-- > 0)
			udelay(10);
		else {
			printf("%s: Transfer data timeout\n", __func__);
			return -ETIMEDOUT;
		}
	} while (!(stat & SDHCI_INT_DATA_END));

	dma_unmap_single(host->start_addr, data->blocks * data->blocksize,
			 mmc_get_dma_dir(data));

	return 0;
}

/*
 * No command will be sent by driver if card is busy, so driver must wait
 * for card ready state.
 * Every time when card is busy after timeout then (last) timeout value will be
 * increased twice but only if it doesn't exceed global defined maximum.
 * Each function call will use last timeout value.
 */
#define SDHCI_CMD_MAX_TIMEOUT			3200
#define SDHCI_CMD_DEFAULT_TIMEOUT		100
#define SDHCI_READ_STATUS_TIMEOUT		1000

#ifdef CONFIG_DM_MMC
static int sdhci_send_command(struct udevice *dev, struct mmc_cmd *cmd,
			      struct mmc_data *data)
{
	struct mmc *mmc = mmc_get_mmc_dev(dev);

#else
static int sdhci_send_command(struct mmc *mmc, struct mmc_cmd *cmd,
			      struct mmc_data *data)
{
#endif
	struct sdhci_host *host = mmc->priv;
	unsigned int stat = 0;
	int ret = 0;
	int trans_bytes = 0, is_aligned = 1;
	u32 mask, flags, mode;
	unsigned int time = 0;
	int mmc_dev = mmc_get_blk_desc(mmc)->devnum;
	ulong start = get_timer(0);

	host->start_addr = 0;
	/* Timeout unit - ms */
	static unsigned int cmd_timeout = SDHCI_CMD_DEFAULT_TIMEOUT;

	mask = SDHCI_CMD_INHIBIT | SDHCI_DATA_INHIBIT;

	/* We shouldn't wait for data inihibit for stop commands, even
	   though they might use busy signaling */
	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION ||
	    ((cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK ||
	      cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200) && !data))
		mask &= ~SDHCI_DATA_INHIBIT;

	while (sdhci_readl(host, SDHCI_PRESENT_STATE) & mask) {
		if (time >= cmd_timeout) {
			printf("%s: MMC: %d busy ", __func__, mmc_dev);
			if (2 * cmd_timeout <= SDHCI_CMD_MAX_TIMEOUT) {
				cmd_timeout += cmd_timeout;
				printf("timeout increasing to: %u ms.\n",
				       cmd_timeout);
			} else {
				puts("timeout.\n");
				return -ECOMM;
			}
		}
		time++;
		udelay(1000);
	}

	sdhci_writel(host, SDHCI_INT_ALL_MASK, SDHCI_INT_STATUS);

	mask = SDHCI_INT_RESPONSE;
	if ((cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK ||
	     cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200) && !data)
		mask = SDHCI_INT_DATA_AVAIL;

	if (!(cmd->resp_type & MMC_RSP_PRESENT))
		flags = SDHCI_CMD_RESP_NONE;
	else if (cmd->resp_type & MMC_RSP_136)
		flags = SDHCI_CMD_RESP_LONG;
	else if (cmd->resp_type & MMC_RSP_BUSY) {
		flags = SDHCI_CMD_RESP_SHORT_BUSY;
		if (data)
			mask |= SDHCI_INT_DATA_END;
	} else
		flags = SDHCI_CMD_RESP_SHORT;

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= SDHCI_CMD_CRC;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		flags |= SDHCI_CMD_INDEX;
	if (data || cmd->cmdidx ==  MMC_CMD_SEND_TUNING_BLOCK ||
	    cmd->cmdidx == MMC_CMD_SEND_TUNING_BLOCK_HS200)
		flags |= SDHCI_CMD_DATA;

	/* Set Transfer mode regarding to data flag */
	if (data) {
		sdhci_writeb(host, 0xe, SDHCI_TIMEOUT_CONTROL);
		mode = SDHCI_TRNS_BLK_CNT_EN;
		trans_bytes = data->blocks * data->blocksize;
		if (data->blocks > 1)
			mode |= SDHCI_TRNS_MULTI;

		if (data->flags == MMC_DATA_READ)
			mode |= SDHCI_TRNS_READ;

		if (host->flags & USE_DMA) {
			mode |= SDHCI_TRNS_DMA;
			sdhci_prepare_dma(host, data, &is_aligned, trans_bytes);
		}

		sdhci_writew(host, SDHCI_MAKE_BLKSZ(SDHCI_DEFAULT_BOUNDARY_ARG,
				data->blocksize),
				SDHCI_BLOCK_SIZE);

		sdhci_writew(host, data->blocks, SDHCI_BLOCK_COUNT);
		sdhci_writew(host, mode, SDHCI_TRANSFER_MODE);
	} else if (cmd->resp_type & MMC_RSP_BUSY) {
		sdhci_writeb(host, 0xe, SDHCI_TIMEOUT_CONTROL);
	}
	sdhci_writel(host, cmd->cmdarg, SDHCI_ARGUMENT);
	sdhci_writew(host, SDHCI_MAKE_CMD(cmd->cmdidx, flags), SDHCI_COMMAND);
	start = get_timer(0);
	do {
		stat = sdhci_readl(host, SDHCI_INT_STATUS);
		if (stat & SDHCI_INT_ERROR)
			break;

		if (get_timer(start) >= SDHCI_READ_STATUS_TIMEOUT) {
			if (host->quirks & SDHCI_QUIRK_BROKEN_R1B) {
				return 0;
			} else {
				printf("%s: Timeout for status update!\n",
				       __func__);
				return -ETIMEDOUT;
			}
		}
	} while ((stat & mask) != mask);

	if ((stat & (SDHCI_INT_ERROR | mask)) == mask) {
		sdhci_cmd_done(host, cmd);
		sdhci_writel(host, mask, SDHCI_INT_STATUS);
	} else
		ret = -1;

	if (!ret && data)
		ret = sdhci_transfer_data(host, data);

	if (host->quirks & SDHCI_QUIRK_WAIT_SEND_CMD)
		udelay(1000);

	stat = sdhci_readl(host, SDHCI_INT_STATUS);
	sdhci_writel(host, SDHCI_INT_ALL_MASK, SDHCI_INT_STATUS);
	if (!ret) {
		if (data && (host->quirks & SDHCI_QUIRK_32BIT_DMA_ADDR) &&
				!is_aligned && (data->flags == MMC_DATA_READ))
			memcpy(data->dest, host->align_buffer, trans_bytes);
		return 0;
	}

	sdhci_reset(host, SDHCI_RESET_CMD);
	sdhci_reset(host, SDHCI_RESET_DATA);
	if (stat & SDHCI_INT_TIMEOUT)
		return -ETIMEDOUT;
	else
		return -ECOMM;
}

#if defined(CONFIG_DM_MMC) && defined(MMC_SUPPORTS_TUNING)
static int sdhci_execute_tuning(struct udevice *dev, uint opcode)
{
	int err;
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	struct sdhci_host *host = mmc->priv;

	debug("%s\n", __func__);

	if (host->ops && host->ops->platform_execute_tuning) {
		err = host->ops->platform_execute_tuning(mmc, opcode);
		if (err)
			return err;
		return 0;
	}
	return 0;
}
#endif

#define CLK_24M   (24000000)
#define CLK_200M  (200000000)

/*
 *     Function Description: select emmc/sd clk source, set div
 *     Parameter:
 *     Return:
 */
#define CLK_EMMC_CLK_CARD_SEL(x)    (((x) & 0x3) << 5)
#define CLK_EMMC_CLK_CARD_DIV(x)    (((x) & 0x3F) << 0)
#define EMMC_CLK_DIV_UPDATE         (0x1 << 6)
#define CLK_SD_CLK_CARD_SEL(x)    (((x) & 0x3) << 16)
#define CLK_SD_CLK_CARD_DIV(x)    (((x) & 0x3F) << 20)
#define SD_CLK_DIV_UPDATE         (0x1 << 26)

#define MMC_BASE_ADDR 0x1b40000
#define SD_BASE_ADDR  0x104e0000

void axera_sys_glb_clk_set(struct sdhci_host *host)
{
	int clk_sel = 0,div = 0;
	if((long)host->ioaddr == (MMC_BASE_ADDR + SDHCI_CDNS_SRS_BASE)) {
		clk_sel = 0x3; //source npll_400m
		div = 0x1; //200M supply card
		writel(BIT(2), CPU_SYS_GLB_CLK_EB0_CLR);//close clk_emmc_card_eb
		udelay(2);
		writel(CLK_EMMC_CLK_CARD_SEL(0x3), CPU_SYS_GLB_CLK_MUX0_CLR);//clr clk source bit[6:5]
		writel(CLK_EMMC_CLK_CARD_SEL(clk_sel), CPU_SYS_GLB_CLK_MUX0_SET);//Select clk source bit[6:5]
		writel(BIT(2), CPU_SYS_GLB_CLK_EB0_SET);//open clk_emmc_card_eb
		writel(GENMASK(5, 0), CPU_SYS_GLB_CLK_DIV0_CLR);//clear bit[5:0]
		writel(CLK_EMMC_CLK_CARD_DIV(div), CPU_SYS_GLB_CLK_DIV0_SET);//set div
		writel(EMMC_CLK_DIV_UPDATE, CPU_SYS_GLB_CLK_DIV0_SET);
		udelay(2);
		writel(EMMC_CLK_DIV_UPDATE, CPU_SYS_GLB_CLK_DIV0_CLR);
	}
	if((long)host->ioaddr == (SD_BASE_ADDR + SDHCI_CDNS_SRS_BASE)) {
		clk_sel = 0x3; //source npll_400m
		div = 0x1; //200M supply card
		writel(BIT(9), FLASH_SYS_GLB_CLK_EB0_CLR);//close clk_sd_card_eb
		udelay(2);
		writel(CLK_SD_CLK_CARD_SEL(0x3), FLASH_SYS_GLB_CLK_MUX0_CLR);//clr clk source bit[17:16]
		writel(CLK_SD_CLK_CARD_SEL(clk_sel), FLASH_SYS_GLB_CLK_MUX0_SET);//Select clk source bit[17:16]
		writel(BIT(9), FLASH_SYS_GLB_CLK_EB0_SET);//open clk_sd_card_eb
		writel(GENMASK(25, 20), FLASH_SYS_GLB_CLK_DIV0_CLR);//clear bit[25:20]
		writel(CLK_SD_CLK_CARD_DIV(div), FLASH_SYS_GLB_CLK_DIV0_SET);//set div
		writel(SD_CLK_DIV_UPDATE, FLASH_SYS_GLB_CLK_DIV0_SET);
		udelay(2);
		writel(SD_CLK_DIV_UPDATE, FLASH_SYS_GLB_CLK_DIV0_CLR);
	}
	return ;
}


int sdhci_set_clock(struct mmc *mmc, unsigned int clock)
{
	struct sdhci_host *host = mmc->priv;
	unsigned int div, clk = 0, timeout;

	/* Wait max 20 ms */
	timeout = 200;
	while (sdhci_readl(host, SDHCI_PRESENT_STATE) &
			   (SDHCI_CMD_INHIBIT | SDHCI_DATA_INHIBIT)) {
		if (timeout == 0) {
			printf("%s: Timeout to wait cmd & data inhibit\n",
			       __func__);
			return -EBUSY;
		}

		timeout--;
		udelay(100);
	}

	sdhci_writew(host, 0, SDHCI_CLOCK_CONTROL);

	if (clock == 0)
		return 0;

	if (host->ops && host->ops->set_delay)
		host->ops->set_delay(host);

	if (SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) {
		/*
		 * Check if the Host Controller supports Programmable Clock
		 * Mode.
		 */
		if (host->clk_mul) {
			for (div = 1; div <= 1024; div++) {
				if ((host->max_clk / div) <= clock)
					break;
			}

			/*
			 * Set Programmable Clock Mode in the Clock
			 * Control register.
			 */
			clk = SDHCI_PROG_CLOCK_MODE;
			div--;
		} else {
			/* Version 3.00 divisors must be a multiple of 2. */
			if (host->max_clk <= clock) {
				div = 1;
			} else {
				for (div = 2;
				     div < SDHCI_MAX_DIV_SPEC_300;
				     div += 2) {
					if ((host->max_clk / div) <= clock)
						break;
				}
			}
			div >>= 1;
		}
	} else {
		/* Version 2.00 divisors must be a power of 2. */
		for (div = 1; div < SDHCI_MAX_DIV_SPEC_200; div *= 2) {
			if ((host->max_clk / div) <= clock)
				break;
		}
		div >>= 1;
	}

	if (host->ops && host->ops->set_clock)
		host->ops->set_clock(host, div);

	clk |= (div & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
	clk |= ((div & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN)
		<< SDHCI_DIVIDER_HI_SHIFT;
	clk |= SDHCI_CLOCK_INT_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);

	/* Wait max 20 ms */
	timeout = 20;
	while (!((clk = sdhci_readw(host, SDHCI_CLOCK_CONTROL))
		& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			printf("%s: Internal clock never stabilised.\n",
			       __func__);
			return -EBUSY;
		}
		timeout--;
		udelay(1000);
	}

	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, clk, SDHCI_CLOCK_CONTROL);
	return 0;
}

static void sdhci_set_power(struct sdhci_host *host, unsigned short power)
{
	u8 pwr = 0;

	if (power != (unsigned short)-1) {
		switch (1 << power) {
		case MMC_VDD_165_195:
			pwr = SDHCI_POWER_180;
			break;
		case MMC_VDD_29_30:
		case MMC_VDD_30_31:
			pwr = SDHCI_POWER_300;
			break;
		case MMC_VDD_32_33:
		case MMC_VDD_33_34:
			pwr = SDHCI_POWER_330;
			break;
		}
	}

	if (pwr == 0) {
		sdhci_writeb(host, 0, SDHCI_POWER_CONTROL);
		return;
	}

	pwr |= SDHCI_POWER_ON;

	sdhci_writeb(host, pwr, SDHCI_POWER_CONTROL);
}

void sdhci_set_uhs_timing(struct sdhci_host *host)
{
	struct mmc *mmc = host->mmc;
	u32 reg;

	reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	reg &= ~SDHCI_CTRL_UHS_MASK;

	switch (mmc->selected_mode) {
	case UHS_SDR50:
	case MMC_HS_52:
		reg |= SDHCI_CTRL_UHS_SDR50;
		break;
	case UHS_DDR50:
	case MMC_DDR_52:
		reg |= SDHCI_CTRL_UHS_DDR50;
		break;
	case UHS_SDR104:
	case MMC_HS_200:
		reg |= SDHCI_CTRL_UHS_SDR104;
		break;
	default:
		reg |= SDHCI_CTRL_UHS_SDR12;
	}

	sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);
}

#define PIN_MUX_G9_BASE			0x104F1000
#define PIN_MUX_G9_VDET_RO0		(PIN_MUX_G9_BASE + 0x58)	//bit0
#define PIN_MUX_G9_PINCTRL_SET	        (PIN_MUX_G9_BASE + 0x4)
#define PIN_MUX_G9_PINCTRL_CLR	        (PIN_MUX_G9_BASE + 0x8)

static void sdhci_axera_voltage_switch(struct sdhci_host *host)
{
	u32 val;

	mdelay(10);

	/* CMD & DATA pad switch to 1.8V */
	val = readl(PIN_MUX_G9_VDET_RO0);
	debug("vdet val:%x\n", val);

	if (((val >> 0) & BIT(0))) { //3.3v
		writel(GENMASK(8, 7), PIN_MUX_G9_PINCTRL_CLR);
		debug("sd io voltage switch to 3.3V\n");
	} else { //1.8v
		writel(GENMASK(8, 7), PIN_MUX_G9_PINCTRL_SET);
		debug("sd io voltage switch to 1.8V\n");
	}
}

#ifdef CONFIG_DM_MMC
static int sdhci_set_ios(struct udevice *dev)
{
	struct mmc *mmc = mmc_get_mmc_dev(dev);
#else
static int sdhci_set_ios(struct mmc *mmc)
{
#endif
	u32 ctrl;
	struct sdhci_host *host = mmc->priv;

	if (host->ops && host->ops->set_control_reg)
		host->ops->set_control_reg(host);

	if (mmc->clock != host->clock)
		sdhci_set_clock(mmc, mmc->clock);

	if (mmc->clk_disable)
		sdhci_set_clock(mmc, 0);

	/* Set bus width */
	ctrl = sdhci_readb(host, SDHCI_HOST_CONTROL);
	debug("sdhci_set_ios: mmc->bus_width = %d, host->clock:%d, mmc->clock:%d\n",mmc->bus_width, host->clock, mmc->clock);
	if (mmc->bus_width == 8) {
		ctrl &= ~SDHCI_CTRL_4BITBUS;
		if ((SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) ||
				(host->quirks & SDHCI_QUIRK_USE_WIDE8))
			ctrl |= SDHCI_CTRL_8BITBUS;
	} else {
		if ((SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) ||
				(host->quirks & SDHCI_QUIRK_USE_WIDE8))
			ctrl &= ~SDHCI_CTRL_8BITBUS;
		if (mmc->bus_width == 4)
			ctrl |= SDHCI_CTRL_4BITBUS;
		else
			ctrl &= ~SDHCI_CTRL_4BITBUS;
	}

	if (mmc->clock > 26000000)
		ctrl |= SDHCI_CTRL_HISPD;
	else
		ctrl &= ~SDHCI_CTRL_HISPD;

	if ((host->quirks & SDHCI_QUIRK_NO_HISPD_BIT) ||
	    (host->quirks & SDHCI_QUIRK_BROKEN_HISPD_MODE))
		ctrl &= ~SDHCI_CTRL_HISPD;

	sdhci_writeb(host, ctrl, SDHCI_HOST_CONTROL);

	if(IS_SD(host->mmc))
		sdhci_axera_voltage_switch(host);

	/* If available, call the driver specific "post" set_ios() function */
	if (host->ops && host->ops->set_ios_post)
		return host->ops->set_ios_post(host);

	return 0;
}

static int sdhci_init(struct mmc *mmc)
{
	struct sdhci_host *host = mmc->priv;
#if CONFIG_IS_ENABLED(DM_MMC) && CONFIG_IS_ENABLED(DM_GPIO)
	struct udevice *dev = mmc->dev;

	gpio_request_by_name(dev, "cd-gpios", 0,
			     &host->cd_gpio, GPIOD_IS_IN);
#endif

	if((long)host->ioaddr == (SD_BASE_ADDR + SDHCI_CDNS_SRS_BASE)) {
		printf("pull up sd cmd&data\n");
		set_sd_pad_state(1);
	}

	sdhci_reset(host, SDHCI_RESET_ALL);

#if defined(CONFIG_FIXED_SDHCI_ALIGNED_BUFFER)
	host->align_buffer = (void *)CONFIG_FIXED_SDHCI_ALIGNED_BUFFER;
	/*
	 * Always use this bounce-buffer when CONFIG_FIXED_SDHCI_ALIGNED_BUFFER
	 * is defined.
	 */
	host->force_align_buffer = true;
#else
	if (host->quirks & SDHCI_QUIRK_32BIT_DMA_ADDR) {
		host->align_buffer = memalign(8, 512 * 1024);
		if (!host->align_buffer) {
			printf("%s: Aligned buffer alloc failed!!!\n",
			       __func__);
			return -ENOMEM;
		}
	}
#endif

	sdhci_set_power(host, fls(mmc->cfg->voltages) - 1);

	if (host->ops && host->ops->get_cd)
		host->ops->get_cd(host);

	/* Enable only interrupts served by the SD controller */
	sdhci_writel(host, SDHCI_INT_DATA_MASK | SDHCI_INT_CMD_MASK,
		     SDHCI_INT_ENABLE);
	/* Mask all sdhci interrupt sources */
	sdhci_writel(host, 0x0, SDHCI_SIGNAL_ENABLE);


	return 0;
}

#ifdef CONFIG_DM_MMC
int sdhci_probe(struct udevice *dev)
{
	struct mmc *mmc = mmc_get_mmc_dev(dev);

	return sdhci_init(mmc);
}

static int sdhci_deferred_probe(struct udevice *dev)
{
	int err;
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	struct sdhci_host *host = mmc->priv;

	if (host->ops && host->ops->deferred_probe) {
		err = host->ops->deferred_probe(host);
		if (err)
			return err;
	}
	return 0;
}

static int sdhci_get_cd(struct udevice *dev)
{
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	struct sdhci_host *host = mmc->priv;
	int value;

	/* If nonremovable, assume that the card is always present. */
	if (mmc->cfg->host_caps & MMC_CAP_NONREMOVABLE)
		return 1;
	/* If polling, assume that the card is always present. */
	if (mmc->cfg->host_caps & MMC_CAP_NEEDS_POLL)
		return 1;

#if CONFIG_IS_ENABLED(DM_GPIO)
	value = dm_gpio_get_value(&host->cd_gpio);
	if (value >= 0) {
		if (mmc->cfg->host_caps & MMC_CAP_CD_ACTIVE_HIGH)
			return !value;
		else
			return value;
	}
#endif
	value = !!(sdhci_readl(host, SDHCI_PRESENT_STATE) &
		   SDHCI_CARD_PRESENT);
	if (mmc->cfg->host_caps & MMC_CAP_CD_ACTIVE_HIGH)
		return !value;
	else
		return value;
}

const struct dm_mmc_ops sdhci_ops = {
	.send_cmd	= sdhci_send_command,
	.set_ios	= sdhci_set_ios,
	.get_cd		= sdhci_get_cd,
	.deferred_probe	= sdhci_deferred_probe,
#ifdef MMC_SUPPORTS_TUNING
	.execute_tuning	= sdhci_execute_tuning,
#endif
};
#else
static const struct mmc_ops sdhci_ops = {
	.send_cmd	= sdhci_send_command,
	.set_ios	= sdhci_set_ios,
	.init		= sdhci_init,
};
#endif

static void emmc_is_set_4bit(struct sdhci_host *host, u32 *caps)
{
	if(host->ioaddr == (void *)0x1B40200) {
		boot_mode_info_t *boot_mode = (boot_mode_info_t *) BOOT_MODE_INFO_ADDR;
		if ((boot_mode->boot_type == EMMC_BOOT_4BIT_25M_768K) || (boot_mode->boot_type == EMMC_BOOT_4BIT_25M_128K))
			*caps &= ~SDHCI_CAN_DO_8BIT;
	}
}

int sdhci_setup_cfg(struct mmc_config *cfg, struct sdhci_host *host,
		u32 f_max, u32 f_min)
{
	u32 caps, caps_1 = 0;
#if CONFIG_IS_ENABLED(DM_MMC)
	u64 dt_caps, dt_caps_mask;

	dt_caps_mask = dev_read_u64_default(host->mmc->dev,
					    "sdhci-caps-mask", 0);
	dt_caps = dev_read_u64_default(host->mmc->dev,
				       "sdhci-caps", 0);
	caps = ~(u32)dt_caps_mask &
	       sdhci_readl(host, SDHCI_CAPABILITIES);
	caps |= (u32)dt_caps;
#else
	caps = sdhci_readl(host, SDHCI_CAPABILITIES);
#endif
	debug("%s, caps: 0x%x\n", __func__, caps);

	emmc_is_set_4bit(host, &caps);

#ifdef CONFIG_MMC_SDHCI_SDMA
	if (!(caps & SDHCI_CAN_DO_SDMA)) {
		printf("%s: Your controller doesn't support SDMA!!\n",
		       __func__);
		return -EINVAL;
	}

	host->flags |= USE_SDMA;
#endif
#if CONFIG_IS_ENABLED(MMC_SDHCI_ADMA)
	if (!(caps & SDHCI_CAN_DO_ADMA2)) {
		printf("%s: Your controller doesn't support SDMA!!\n",
		       __func__);
		return -EINVAL;
	}
	host->adma_desc_table = memalign(ARCH_DMA_MINALIGN, ADMA_TABLE_SZ);

	host->adma_addr = (dma_addr_t)host->adma_desc_table;
#ifdef CONFIG_DMA_ADDR_T_64BIT
	host->flags |= USE_ADMA64;
#else
	host->flags |= USE_ADMA;
#endif
#endif
	if (host->quirks & SDHCI_QUIRK_REG32_RW)
		host->version =
			sdhci_readl(host, SDHCI_HOST_VERSION - 2) >> 16;
	else
		host->version = sdhci_readw(host, SDHCI_HOST_VERSION);

	cfg->name = host->name;
#ifndef CONFIG_DM_MMC
	cfg->ops = &sdhci_ops;
#endif

	/* Check whether the clock multiplier is supported or not */
	if (SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) {
#if CONFIG_IS_ENABLED(DM_MMC)
		caps_1 = ~(u32)(dt_caps_mask >> 32) &
			 sdhci_readl(host, SDHCI_CAPABILITIES_1);
		caps_1 |= (u32)(dt_caps >> 32);
#else
		caps_1 = sdhci_readl(host, SDHCI_CAPABILITIES_1);
#endif
		debug("%s, caps_1: 0x%x\n", __func__, caps_1);
		host->clk_mul = (caps_1 & SDHCI_CLOCK_MUL_MASK) >>
				SDHCI_CLOCK_MUL_SHIFT;
	}

	if (host->max_clk == 0) {
		if (SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300)
			host->max_clk = (caps & SDHCI_CLOCK_V3_BASE_MASK) >>
				SDHCI_CLOCK_BASE_SHIFT;
		else
			host->max_clk = (caps & SDHCI_CLOCK_BASE_MASK) >>
				SDHCI_CLOCK_BASE_SHIFT;
		host->max_clk *= 1000000;
		if (host->clk_mul)
			host->max_clk *= host->clk_mul;
	}
	if (host->max_clk == 0) {
		printf("%s: Hardware doesn't specify base clock frequency\n",
		       __func__);
		return -EINVAL;
	}
	if (f_max && (f_max < host->max_clk))
		cfg->f_max = f_max;
	else
		cfg->f_max = host->max_clk;
	if (f_min)
		cfg->f_min = f_min;
	else {
		if (SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300)
			cfg->f_min = cfg->f_max / SDHCI_MAX_DIV_SPEC_300;
		else
			cfg->f_min = cfg->f_max / SDHCI_MAX_DIV_SPEC_200;
	}
	cfg->voltages = 0;
	if (caps & SDHCI_CAN_VDD_330)
		cfg->voltages |= MMC_VDD_32_33 | MMC_VDD_33_34;
	if (caps & SDHCI_CAN_VDD_300)
		cfg->voltages |= MMC_VDD_29_30 | MMC_VDD_30_31;
	if (caps & SDHCI_CAN_VDD_180)
		cfg->voltages |= MMC_VDD_165_195;

	if (host->quirks & SDHCI_QUIRK_BROKEN_VOLTAGE)
		cfg->voltages |= host->voltages;

	cfg->host_caps |= MMC_CAP(MMC_HS) | MMC_MODE_HS_52MHz | MMC_MODE_8BIT | MMC_MODE_4BIT;

	/* Since Host Controller Version3.0 */
	if (SDHCI_GET_VERSION(host) >= SDHCI_SPEC_300) {
		if (!(caps & SDHCI_CAN_DO_8BIT))
			cfg->host_caps &= ~MMC_MODE_8BIT;
	}

	if (host->quirks & SDHCI_QUIRK_BROKEN_HISPD_MODE) {
		cfg->host_caps &= ~MMC_MODE_HS;
		cfg->host_caps &= ~MMC_MODE_HS_52MHz;
	}

	if (!(cfg->voltages & MMC_VDD_165_195) ||
	    (host->quirks & SDHCI_QUIRK_NO_1_8_V))
		caps_1 &= ~(SDHCI_SUPPORT_SDR104 | SDHCI_SUPPORT_SDR50 |
			    SDHCI_SUPPORT_DDR50);

#if 0
	if (caps_1 & (SDHCI_SUPPORT_SDR104 | SDHCI_SUPPORT_SDR50 |
		      SDHCI_SUPPORT_DDR50))
		cfg->host_caps |= MMC_CAP(UHS_SDR12) | MMC_CAP(UHS_SDR25);

	if (caps_1 & SDHCI_SUPPORT_SDR104) {
		cfg->host_caps |= MMC_CAP(UHS_SDR104) | MMC_CAP(UHS_SDR50);
		/*
		 * SD3.0: SDR104 is supported so (for eMMC) the caps2
		 * field can be promoted to support HS200.
		 */
		cfg->host_caps |= MMC_CAP(MMC_HS_200);
	} else if (caps_1 & SDHCI_SUPPORT_SDR50) {
		cfg->host_caps |= MMC_CAP(UHS_SDR50);
	}

	if (caps_1 & SDHCI_SUPPORT_DDR50)
		cfg->host_caps |= MMC_CAP(UHS_DDR50);
#else
	if (caps_1 & SDHCI_SUPPORT_SDR104) {
		/*
		 * SD3.0: SDR104 is supported so (for eMMC) the caps2
		 * field can be promoted to support HS200.
		 */
		cfg->host_caps |= MMC_CAP(MMC_HS_200);
	}
#endif
	if (host->host_caps)
		cfg->host_caps |= host->host_caps;

	cfg->b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

	return 0;
}

#ifdef CONFIG_BLK
int sdhci_bind(struct udevice *dev, struct mmc *mmc, struct mmc_config *cfg)
{
	return mmc_bind(dev, mmc, cfg);
}
#else
int add_sdhci(struct sdhci_host *host, u32 f_max, u32 f_min)
{
	int ret;

	ret = sdhci_setup_cfg(&host->cfg, host, f_max, f_min);
	if (ret)
		return ret;

	host->mmc = mmc_create(&host->cfg, host);
	if (host->mmc == NULL) {
		printf("%s: mmc create fail!\n", __func__);
		return -ENOMEM;
	}

	return 0;
}
#endif

#define SDCTRL_REG_NUM 64
static u32 emmc_ctrl_reg_buf[SDCTRL_REG_NUM] = {0xff};
void sdhci_dumpregs(struct sdhci_host *host)
{
	u32 i;

	printf("==================== REG PHY DUMP ==================\n");
	for (i = 0; i < 0x17; i++) {
		printf("PHY reg: %d:  0x%x\n", i, sdhci_cdns_read_phy_reg((ulong *)(host->ioaddr - 0x200), i));
	}
	printf("==================== REG DUMP ==================\n");
	for (i = 0; i < 64 / 4; i++) {
		printf("%lX ++ %04X: %08X %08X %08X %08X\n",
		       (ulong)(host->ioaddr - 0x200), i * 0x10,
		       readl(host->ioaddr - 0x200 + i * 0x10), readl(host->ioaddr - 0x200 + i * 0x10 + 0x4),
		       readl(host->ioaddr - 0x200 + i * 0x10 + 0x8), readl(host->ioaddr - 0x200 + i * 0x10 + 0xC));
	}

	for (i = 0; i < SDCTRL_REG_NUM; i++) {
		emmc_ctrl_reg_buf[i] = readl(host->ioaddr + i * 4);
	}

	printf("==================== REG BUF DUMP ===================\n");
	for (i = 0; i < SDCTRL_REG_NUM / 4; i++) {
		printf("%lX ++ %04X: %08X %08X %08X %08X\n", (ulong)host->ioaddr, i * 0x10,
		       emmc_ctrl_reg_buf[i * 4], emmc_ctrl_reg_buf[i * 4 + 1],
		       emmc_ctrl_reg_buf[i * 4 + 2], emmc_ctrl_reg_buf[i * 4 + 3]);
	}

	printf("==================== REG UART DUMP ==================\n");
	for (i = 0; i < SDCTRL_REG_NUM / 4; i++) {
		printf("%lX ++ %04X: %08X %08X %08X %08X\n",
		       (ulong)host->ioaddr, i * 0x10,
		       readl(host->ioaddr + i * 0x10), readl(host->ioaddr + i * 0x10 + 0x4),
		       readl(host->ioaddr + i * 0x10 + 0x8), readl(host->ioaddr + i * 0x10 + 0xC));
	}

	printf("=====================================================\n");
}

void sdhci_cdns_set_uhs_timing(struct sdhci_host *host)
{
	struct mmc *mmc = host->mmc;
	u32 reg;

	reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	reg &= ~(SDHCI_CTRL_UHS_MASK | SDHCI_CTRL_VDD_180);

	switch (mmc->selected_mode) {
	case UHS_SDR12:
		reg |= SDHCI_CTRL_UHS_SDR12 | SDHCI_CTRL_VDD_180;
		break;
	case UHS_SDR25:
		reg |= SDHCI_CTRL_UHS_SDR25 | SDHCI_CTRL_VDD_180;
		break;
	case UHS_SDR50:
		reg |= SDHCI_CTRL_UHS_SDR50 | SDHCI_CTRL_VDD_180;
		break;
	case UHS_DDR50:
		reg |= SDHCI_CTRL_UHS_DDR50 | SDHCI_CTRL_VDD_180;
		break;
	case UHS_SDR104:
		reg |= SDHCI_CTRL_UHS_SDR104 | SDHCI_CTRL_VDD_180;
		break;
	default:
		break; //SD default and hs mode, set 0
	}

	sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);
}
static void sdhci_cdns_set_emmc_mode(struct sdhci_cdns_plat *plat, u32 mode)
{
	u32 tmp;

	/* The speed mode for eMMC is selected by HRS06 register */
	tmp = readl(plat->hrs_addr + SDHCI_CDNS_HRS06);
	tmp &= ~SDHCI_CDNS_HRS06_MODE;
	tmp |= FIELD_PREP(SDHCI_CDNS_HRS06_MODE, mode);
	writel(tmp, plat->hrs_addr + SDHCI_CDNS_HRS06);
}

static const struct sdhci_cdns_phy_cfg sdhci_cdns_phy_cfgs[] = {
	{ "cdns,phy-input-delay-sd-highspeed", SDHCI_CDNS_PHY_DLY_SD_HS, },
	{ "cdns,phy-input-delay-legacy", SDHCI_CDNS_PHY_DLY_SD_DEFAULT, },
	{ "cdns,phy-input-delay-sd-uhs-sdr12", SDHCI_CDNS_PHY_DLY_UHS_SDR12, },
	{ "cdns,phy-input-delay-sd-uhs-sdr25", SDHCI_CDNS_PHY_DLY_UHS_SDR25, },
	{ "cdns,phy-input-delay-sd-uhs-sdr50", SDHCI_CDNS_PHY_DLY_UHS_SDR50, },
	{ "cdns,phy-input-delay-sd-uhs-ddr50", SDHCI_CDNS_PHY_DLY_UHS_DDR50, },
	{ "cdns,phy-input-delay-mmc-legacy", SDHCI_CDNS_PHY_DLY_EMMC_LEGACY, },
	{ "cdns,phy-input-delay-mmc-highspeed", SDHCI_CDNS_PHY_DLY_EMMC_SDR, },
	{ "cdns,phy-input-delay-mmc-ddr", SDHCI_CDNS_PHY_DLY_EMMC_DDR, },
	{ "cdns,phy-dll-delay-sdclk", SDHCI_CDNS_PHY_DLY_SDCLK, },
	{ "cdns,phy-dll-delay-sdclk-hsmmc", SDHCI_CDNS_PHY_DLY_HSMMC, },
	{ "cdns,phy-dll-delay-strobe", SDHCI_CDNS_PHY_DLY_STROBE, },
};

static int sdhci_cdns_write_phy_reg(struct sdhci_cdns_plat *plat,
				    u8 addr, u8 data)
{
	void __iomem *reg = plat->hrs_addr + SDHCI_CDNS_HRS04;
	u32 tmp;
	int ret;

	tmp = FIELD_PREP(SDHCI_CDNS_HRS04_WDATA, data) |
	      FIELD_PREP(SDHCI_CDNS_HRS04_ADDR, addr);
	writel(tmp, reg);

	tmp |= SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	ret = readl_poll_timeout(reg, tmp, tmp & SDHCI_CDNS_HRS04_ACK, 10);
	if (ret)
		return ret;

	tmp &= ~SDHCI_CDNS_HRS04_WR;
	writel(tmp, reg);

	return 0;
}

static int sdhci_cdns_phy_init(struct sdhci_cdns_plat *plat,
				const void *fdt, int nodeoffset)
{
	const fdt32_t *prop;
	int ret, i;

	sdhci_cdns_write_phy_reg(plat, SDHCI_CDNS_PHY_DLL_RESET, 0x0);
	sdhci_cdns_write_phy_reg(plat, SDHCI_CDNS_PHY_DLL_RESET, 0x1);

	for (i = 0; i < ARRAY_SIZE(sdhci_cdns_phy_cfgs); i++) {
		prop = fdt_getprop(fdt, nodeoffset,
				   sdhci_cdns_phy_cfgs[i].property, NULL);
		if (!prop)
			continue;

		ret = sdhci_cdns_write_phy_reg(plat,
					       sdhci_cdns_phy_cfgs[i].addr,
					       fdt32_to_cpu(*prop));
		if (ret)
			return ret;
	}

	return 0;
}

static void sdhci_cdns_set_control_reg(struct sdhci_host *host)
{
	u32 mode;

#if CONFIG_IS_ENABLED(BLK)
	struct mmc *mmc = host->mmc;
	struct sdhci_cdns_plat *plat = dev_get_platdata(mmc->dev);
	unsigned int clock = mmc->clock;

	/*
	 * REVISIT:
	 * The mode should be decided by MMC_TIMING_* like Linux, but
	 * U-Boot does not support timing.  Use the clock frequency instead.
	 */
	if (IS_MMC(mmc)) {
		if (clock <= 26000000) {
			mode = SDHCI_CDNS_HRS06_MODE_MMC_LEGACY; /* use this for Legacy */
		} else if (clock <= 52000000) {
			if (mmc->ddr_mode)
				mode = SDHCI_CDNS_HRS06_MODE_MMC_DDR;
			else
				mode = SDHCI_CDNS_HRS06_MODE_MMC_SDR;
		} else {
			if (mmc->ddr_mode)
				mode = SDHCI_CDNS_HRS06_MODE_MMC_HS400;
			else
				mode = SDHCI_CDNS_HRS06_MODE_MMC_HS200;
		}
		sdhci_cdns_set_emmc_mode(plat, mode);
	} else {
		sdhci_cdns_set_uhs_timing(host);
	}
#else
	u32 tmp;
	if (host->ioaddr == 0x1B40200) {
		tmp = readl((host->ioaddr - 0x200) + SDHCI_CDNS_HRS06);
		tmp &= ~SDHCI_CDNS_HRS06_MODE;
		if (clock <= 26000000) {
			tmp |= FIELD_PREP(SDHCI_CDNS_HRS06_MODE, SDHCI_CDNS_HRS06_MODE_MMC_LEGACY);
			writel(tmp, (host->ioaddr - 0x200) + SDHCI_CDNS_HRS06);
		} else if (clock <= 52000000) {
			tmp |= FIELD_PREP(SDHCI_CDNS_HRS06_MODE, SDHCI_CDNS_HRS06_MODE_MMC_SDR);
			writel(tmp, (host->ioaddr - 0x200) + SDHCI_CDNS_HRS06);
		}
	} else {
		reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
		reg &= ~SDHCI_CTRL_UHS_MASK;//SD default and hs mode, set 0
		sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);
	}
#endif
}

static const struct sdhci_ops sdhci_cdns_ops = {
	.set_control_reg = sdhci_cdns_set_control_reg,
};

static int sdhci_cdns_set_tune_val(struct sdhci_cdns_plat *plat,
				   unsigned int val)
{
	void __iomem *reg = plat->hrs_addr + SDHCI_CDNS_HRS06;
	u32 tmp;
	int i, ret;

	if (WARN_ON(!FIELD_FIT(SDHCI_CDNS_HRS06_TUNE, val)))
		return -EINVAL;

	tmp = readl(reg);
	tmp &= ~SDHCI_CDNS_HRS06_TUNE;
	tmp |= FIELD_PREP(SDHCI_CDNS_HRS06_TUNE, val);

	/*
	 * Workaround for IP errata:
	 * The IP6116 SD/eMMC PHY design has a timing issue on receive data
	 * path. Send tune request twice.
	 */
	for (i = 0; i < 2; i++) {
		tmp |= SDHCI_CDNS_HRS06_TUNE_UP;
		writel(tmp, reg);

		ret = readl_poll_timeout(reg, tmp,
					 !(tmp & SDHCI_CDNS_HRS06_TUNE_UP), 1);
		if (ret)
			return ret;
	}

	return 0;
}

static int __maybe_unused sdhci_cdns_execute_tuning(struct udevice *dev,
						    unsigned int opcode)
{
	struct sdhci_cdns_plat *plat = dev_get_platdata(dev);
	struct mmc *mmc = &plat->mmc;
	int cur_streak = 0;
	int max_streak = 0;
	int end_of_streak = 0;
	int i;

	/*
	 * This handler only implements the eMMC tuning that is specific to
	 * this controller.  The tuning for SD timing should be handled by the
	 * SDHCI core.
	 */
	if (!IS_MMC(mmc))
		return -ENOTSUPP;

	if (WARN_ON(opcode != MMC_CMD_SEND_TUNING_BLOCK_HS200))
		return -EINVAL;

	for (i = 0; i < SDHCI_CDNS_MAX_TUNING_LOOP; i++) {
		if (sdhci_cdns_set_tune_val(plat, i) ||
		    mmc_send_tuning(mmc, opcode, NULL)) { /* bad */
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
		dev_err(dev, "no tuning point found\n");
		return -EIO;
	}

	return sdhci_cdns_set_tune_val(plat, end_of_streak - max_streak / 2);
}

static struct dm_mmc_ops sdhci_cdns_mmc_ops;

static int sdhci_cdns_bind(struct udevice *dev)
{
	struct sdhci_cdns_plat *plat = dev_get_platdata(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static int sdhci_cdns_probe(struct udevice *dev)
{
	DECLARE_GLOBAL_DATA_PTR;
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct sdhci_cdns_plat *plat = dev_get_platdata(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	fdt_addr_t base;
	int ret;


	base = devfdt_get_addr(dev);
	if (base == FDT_ADDR_T_NONE)
		return -EINVAL;

	plat->hrs_addr = devm_ioremap(dev, base, SZ_1K);
	if (!plat->hrs_addr)
		return -ENOMEM;

	host->name = dev->name;
	host->ioaddr = plat->hrs_addr + SDHCI_CDNS_SRS_BASE;
	host->ops = &sdhci_cdns_ops;
	host->quirks |= SDHCI_QUIRK_WAIT_SEND_CMD;
	sdhci_cdns_mmc_ops = sdhci_ops;
#ifdef MMC_SUPPORTS_TUNING
	sdhci_cdns_mmc_ops.execute_tuning = sdhci_cdns_execute_tuning;
#endif
	axera_sys_glb_clk_set(host);
	ret = mmc_of_parse(dev, &plat->cfg);
	if (ret)
		return ret;

	ret = sdhci_cdns_phy_init(plat, gd->fdt_blob, dev_of_offset(dev));
	if (ret)
		return ret;

#if 0
	sdhci_dumpregs(host);
#endif

	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	ret = sdhci_setup_cfg(&plat->cfg, host, 0, 400000);
	if (ret)
		return ret;
	upriv->mmc = &plat->mmc;
	host->mmc->priv = host;

	return sdhci_probe(dev);
}

static const struct udevice_id sdhci_cdns_match[] = {
	{ .compatible = "axera,ax620e-sdhc" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(sdhci_cdns) = {
	.name = "sdhci-cdns",
	.id = UCLASS_MMC,
	.of_match = sdhci_cdns_match,
	.bind = sdhci_cdns_bind,
	.probe = sdhci_cdns_probe,
	.priv_auto_alloc_size = sizeof(struct sdhci_host),
	.platdata_auto_alloc_size = sizeof(struct sdhci_cdns_plat),
	.ops = &sdhci_cdns_mmc_ops,
};
