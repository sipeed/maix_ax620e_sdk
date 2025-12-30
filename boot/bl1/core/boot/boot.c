#include "boot.h"
#include "dma.h"
#include "trace.h"
#if defined(AX620E_NAND)
#include "spinand.h"
#endif
#if defined(AX620E_NOR)
#include "spinor.h"
#endif
#include "secure.h"
#include "board.h"
#include "efuse_drv.h"
#include "mmc.h"
#include "cmn.h"
#include "sdhci_cdns.h"
#include "ff.h"
#include "ax_gzipd_api.h"
#include "printf.h"
#include "trace.h"
#include "libfdt.h"
#include "timer.h"

static int flash_clk_array[] = {200000000, 50000000, 25000000, 6000000};
#if defined(AX620E_SUPPORT_SD)
static FATFS fs;
static FIL fil;
static u8 fatfs_mounted = 0;
#endif
static char *boot_image_file;

#if defined(AX620E_EMMC)
static u8 emmc_part_boot = 0; /* defalut from UDA */
// static u8 emmc_part_boot = 1; /* test boot partition1 */
#endif

#if defined(AX_SUPPORT_AB_PART)
static u32 support_ab = 1;
#else
static u32 support_ab = 0;
#endif

extern u8 curr_cs;
u32 current_bus_width = 1;
u32 current_clock = 0;
extern struct pci_msg_reg *reg;
extern int axi_dma_word_checksum(u32 *out, u64 sar, int size);

#ifdef AX_SPL_SUPPORT_MODIFY_BOOTARGS
#define ABORT_WDT2_EN BIT(9)
#define ABORT_WDT0_EN BIT(7)
#define ABORT_THM_EN BIT(6)
#define ABORT_WDT2_CLR BIT(5)
#define ABORT_WDT0_CLR BIT(3)
#define ABORT_THM_CLR BIT(2)
#define ABORT_SWRST_CLR BIT(1)

static char *bootargs = KERNEL_BOOTARGS;

u32 axera_get_boot_reason(void)
{
	u32 abort_cfg;
	u32 abort_status;
	abort_status = readl(COMM_ABORT_STATUS);
	/*clear abort alarm status */
	abort_cfg =
	    ABORT_WDT0_CLR | ABORT_WDT2_CLR | ABORT_THM_CLR | ABORT_SWRST_CLR;
	writel(abort_cfg, COMM_ABORT_CFG);
	/*enable watchdog, thermal abort function */
	abort_cfg = ABORT_WDT2_EN | ABORT_WDT0_EN | ABORT_THM_EN;
	writel(abort_cfg, COMM_ABORT_CFG);
	return abort_status;
}

static char * strstr(const char * s1,const char * s2)
{
	int l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	l1 = strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1,s2,l2))
			return (char *) s1;
		s1++;
	}
	return NULL;
}

static void update_cmdline(char * bootargs)
{
	char *boot_reason_cmd = NULL;
	char *board_id_cmd = NULL;
	u32 boot_reason;
	u32 board_id;
	misc_info_t *misc_info = (misc_info_t *) MISC_INFO_ADDR;
	if(NULL == bootargs)
		return;
	boot_reason = axera_get_boot_reason();
	boot_reason_cmd = strstr(bootargs, "boot_reason");
	if(!boot_reason_cmd) {
		info("boot_reason not find bootargs\n");
		return;
	}
	if (boot_reason >= 0 && boot_reason <= 9) {
		boot_reason_cmd[15] = boot_reason + '0';
	}
	if (boot_reason == 0x10) {
		boot_reason_cmd[14] = 1 + '0';
		boot_reason_cmd[15] = '0';
	}

	board_id = misc_info->phy_board_id;

	board_id_cmd = strstr(bootargs, "board_id");
	if(!board_id_cmd) {
		info("board_id not find bootargs\n");
		return;
	}
	if (board_id >= 0 && board_id <= 9) {
		board_id_cmd[11] = board_id + '0';
	}
	if (board_id >= 10 && board_id <= 15) {
		board_id_cmd[11] = board_id + 'W';
	}
}
#endif

static char *sd_img_name[] = {
	[DDRINIT] = "0:ddrinit.img",
	[ATF] = "0:atf.img",
	[UBOOT] = "0:uboot.bin",
	[OPTEE] = "0:optee.img",
	[DTB] = "0:dtb.img",
	[KERNEL] = "0:kernel.img",
	[RISCV] = "0:rtthread.bin"
};

u32 calc_word_chksum(int *data, int size)
{
	int count = size / 4;
	int i;
	u32 sum = 0;
	if(axi_dma_word_checksum(&sum, (unsigned long)data, size))
	{
		/* if dma calulate failed, use cpu calu. */
		sum = 0;
		for(i = 0; i < count; i++) {
			sum += data[i];
		}
	}
	return sum;
}
#if defined(AX620E_SUPPORT_SD)
int sdcard_read(u32 offset, u32 size, char *buf)
{
	int ret;
	UINT bw;

	info("get %s from sd, offset:%x, size:%d \r\n", boot_image_file, offset, size);

	ret = f_open(&fil, boot_image_file, FA_OPEN_EXISTING | FA_READ);
	if(ret != FR_OK)
		return -1;
	ret = f_lseek(&fil, offset);
	if(ret != FR_OK) {
		f_close(&fil);
		return -1;
	}
	ret = f_read(&fil, buf, size, &bw);
	if(ret != FR_OK) {
		f_close(&fil);
		return -1;
	}
	f_close(&fil);

	return 0;
}
#endif
static int flash_init(u32 flash_type, u32 clk, u32 bus_width)
{
	int ret;

	switch (flash_type) {
#if defined(AX620E_SUPPORT_SD)
	case FLASH_SD:
		ret = sd_init(clk, bus_width);
		if (ret < 0) {
			err("sd init failed\r\n");
			return ret;
		}
		/* MUST mount fatfs after sd_init success */
		if (!fatfs_mounted) {
			ret = f_mount(&fs, "0:", 1);
			if(ret == FR_NO_FILESYSTEM) {
				err("mount fat32 failed\r\n");
				return -1;
			}
			fatfs_mounted = 1;
			info("mount fat32 success\r\n");
		}
		break;
#endif
#if defined(AX620E_NOR)
	case FLASH_NOR:
		if (0 == curr_cs)
			info("nor CS0 BOOT");
		else
			info("nor CS1 BOOT");
		ret = spinor_init(clk, bus_width);
		if (ret < 0)
			return ret;
		break;
#endif
#if defined(AX620E_NAND)
	case FLASH_NAND_4K:
		if (0 == curr_cs)
			info("nand 4k CS0 BOOT");
		else
			info("nand 4k CS1 BOOT");
		ret = spinand_init(clk, bus_width);
		if (ret < 0)
			return ret;
		break;
	case FLASH_NAND_2K:
		if (0 == curr_cs)
			info("nand 2k CS0 BOOT");
		else
			info("nand 2k CS0 BOOT");
		ret = spinand_init(clk, bus_width);
		if (ret < 0)
			return ret;
		break;
#endif
#if defined(AX620E_EMMC)
	case FLASH_EMMC:
	case FLASH_EMMC_BOOT_8BIT_50M_768K:
	case FLASH_EMMC_BOOT_4BIT_25M_768K:
	case FLASH_EMMC_BOOT_4BIT_25M_128K:
		/* first version default user partition */
		ret = emmc_init(flash_type, clk, bus_width, emmc_part_boot);
		if (ret < 0)
			return ret;
		break;
#endif
	default:
		return -1;
	}

	return ret;
}

int flash_read(u32 flash_type, char *buf, u32 flash_addr, int size)
{
	int ret;
	switch (flash_type) {
#if defined(AX620E_SUPPORT_SD)
	case FLASH_SD:
		ret = sdcard_read(flash_addr, size, buf);
		if (ret < 0)
			return ret;
		break;
#endif
#if defined(AX620E_NOR)
	case FLASH_NOR:
		ret = spinor_read(flash_addr, size, (u8 *)buf);
		if (ret < 0)
			return ret;
		break;
#endif
#if defined(AX620E_NAND)
	case FLASH_NAND_2K:
	case FLASH_NAND_4K:
		ret = spinand_read(flash_addr, size, buf);
		if (ret < 0)
			return ret;
		break;
#endif
#if defined(AX620E_EMMC)
	case FLASH_EMMC:
	case FLASH_EMMC_BOOT_8BIT_50M_768K:
	case FLASH_EMMC_BOOT_4BIT_25M_768K:
	case FLASH_EMMC_BOOT_4BIT_25M_128K:
		ret = emmc_read(buf, flash_addr, size);
		if (ret < 0)
			return ret;
		break;
#endif
	default:
		return -1;
	}

	return size;
}

#ifdef SUPPPORT_GZIPD
#if defined(AX620E_EMMC)
#define IMAGE_COMPRESSED_PADDR 		0x58000000
#define SAMPLE_TILE_SIZE    (64 * 1024 * 1024)
#endif
#if defined(AX620E_NAND)
#define IMAGE_COMPRESSED_PADDR 		0x5e000000
#define SAMPLE_TILE_SIZE    (64 * 1024 * 1024)
#endif
#if defined(AX620E_NOR)
#define IMAGE_COMPRESSED_PADDR 		0x5e000000
#define SAMPLE_TILE_SIZE    (4 * 16 * 1024)
#endif
#if defined(AX620E_SUPPORT_SD)
#define IMAGE_COMPRESSED_PADDR 		0x58000000
#define SAMPLE_TILE_SIZE    (4 * 16 * 1024)
#endif
#define AX_GZIPD_FALSE 0
#define AX_GZIPD_TRUE  1
#define FIFO_DEPTH  16

static int gzip_pipeline_flash_read(u32 flash_type, char *buf, u32 flash_addr, int size)
{
	int ret;
	gzipd_header_info_t header_info;
	void *img_compressed_addr;
	u32 tile_cnt, idx;
	u32 last_tile_size = 0;
	unsigned long tile_addr;
	u32 read_emmc_data_pos = 0;
	u32 start_pos;
	u32 rd_len;
	void *img_buf_pos;
	void *tiles_addr_start;
	void *tiles_addr_end;
	int need_read_data;
	u32 wait_loop = 50;

	start_pos = flash_addr;
	#if defined(AX620E_NAND)
	img_buf_pos = (void *)(IMAGE_COMPRESSED_PADDR + sizeof(struct img_header));
	#else
	img_buf_pos = (void *)IMAGE_COMPRESSED_PADDR;
	#endif
	if (size < SAMPLE_TILE_SIZE)
		rd_len = size;
	else
		rd_len = SAMPLE_TILE_SIZE;

	#if defined(AX620E_NAND)
	ret = 0;
	#else
	ret = flash_read(flash_type, img_buf_pos, start_pos, rd_len);
	#endif
	if (ret < 0) {
		info("read data from emmc fail\r\n");
		return -1;
	}
	read_emmc_data_pos += rd_len;

	img_compressed_addr = img_buf_pos;
	start_pos += rd_len;
	img_buf_pos += rd_len;
	ret = gzipd_dev_get_header_info(img_compressed_addr, &header_info);
	if (ret) {
		info("get header info error\n");
		return -1;
	}

	debug("header_info.blk_num = %d, isize = %d, osize = %d \r\n",
			header_info.blk_num, header_info.isize, header_info.osize);

	gzipd_dev_cfg(SAMPLE_TILE_SIZE, (void *)buf,
					header_info.isize, header_info.osize, header_info.blk_num, &tile_cnt, &last_tile_size);

	tile_addr = (unsigned long)img_compressed_addr + sizeof(header_info);

	debug("tile_cnt = %d, last_tile_size = %d\r\n", tile_cnt, last_tile_size);
	if (tile_cnt == 0) {
		if (gzipd_dev_run_last_tile((void *)tile_addr, last_tile_size)) {
			info(" run last tile error\r\n");
			return -1;
		}
		goto complete_finish;
	}

	tiles_addr_start = (void *)tile_addr;
	tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * ((tile_cnt < 16 ? tile_cnt : 16) - 1) ;
	need_read_data = AX_GZIPD_FALSE;
	idx = 0;
	while (idx < tile_cnt - 1) {
		u32 run_num = 0;
		if ((unsigned long)tiles_addr_end - tile_addr <= read_emmc_data_pos) {
			ret = gzipd_dev_run(tiles_addr_start, tiles_addr_end, &run_num);
			if (ret) {
				return -1;
			}
			idx += run_num;
			tiles_addr_start += run_num * SAMPLE_TILE_SIZE;
			if (tile_cnt - idx > FIFO_DEPTH ) {
				tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * (FIFO_DEPTH - 1);
			} else {
				debug("not full cmdq, tile_cnt = %d, idx = %d\r\n", tile_cnt, idx);
				tiles_addr_end = tiles_addr_start + SAMPLE_TILE_SIZE * (tile_cnt - idx - 1);
			}
		} else {
			debug("tiles_addr_end = 0x%x, tile_addr = 0x%x, diff = %p, read_emmc_data_pos = 0x%x \r\n",
					(u64)tiles_addr_end, tile_addr, tiles_addr_end - tile_addr, read_emmc_data_pos);
		}

		while (wait_loop--) {
			int level = gzipd_dev_get_fifo_level();
			if (level < 16 && need_read_data == AX_GZIPD_FALSE) {
				debug("here input data to gzipd ,level = %d, 0x%x\r\n", level, read_emmc_data_pos);
				if (read_emmc_data_pos < (idx + FIFO_DEPTH) * SAMPLE_TILE_SIZE) {
					need_read_data = AX_GZIPD_TRUE;
					continue;
				}
				break;
			} else if((level == 16) || need_read_data == AX_GZIPD_TRUE) {
				debug("read next data in emmc when gzipd cmdq full or need more data = %d, read_emmc_data_pos:0x%x\r\n", need_read_data, read_emmc_data_pos);
				if (read_emmc_data_pos >= header_info.isize) {
					debug(" read the end of the whole z20e file's data\r\n");
					break;
				}

				rd_len = SAMPLE_TILE_SIZE;
				if (header_info.isize - read_emmc_data_pos < rd_len) {
					rd_len = header_info.isize - read_emmc_data_pos;
				}
				debug("read emmc data, start = 0x%x size =0x%x \r\n", start_pos, rd_len);
				#if defined(AX620E_NAND)
				ret = 0;
				#else
				ret = flash_read(flash_type, img_buf_pos, start_pos, rd_len);
				#endif
				if (ret < 0) {
					info("read len[%d] data from emmc[0x%x] fail\r\n", rd_len, start_pos);
					return -1;
				}
				start_pos += rd_len;
				img_buf_pos += rd_len;
				read_emmc_data_pos += rd_len;
				need_read_data = AX_GZIPD_FALSE;
			}
		}

		debug(" sampled gzipd test : idx = %d\r\n", idx);
	}

	if (gzipd_dev_run_last_tile(tiles_addr_end, last_tile_size)) {
		info("lastly gzipd run last tile error\r\n");
		return -1;
	}

complete_finish:
	if (gzipd_dev_wait_complete_finish()) {
		debug("finish de-compress the whole gzip file fail\r\n");
		return -1;
	} else {
		debug("finish de-compress the whole gzip file success\r\n");
	}

	return 0;
}
#endif

#ifdef OLD_INTERFACE
static int flash_get_bus_width(u32 flash_type, struct img_header *header)
{
	int bus_width = BUS_WIDTH_1;

	switch (flash_type) {
	case FLASH_SD:
		if (SDCARD_BUSWIDTH_4 & header->capability)
			bus_width = BUS_WIDTH_4;
		break;
	case FLASH_NOR:
		if (SPI_NOR_BUSWIDTH_4 & header->capability)
			bus_width = BUS_WIDTH_4;
		break;
	case FLASH_NAND_2K:
	case FLASH_NAND_4K:
		if (SPI_NAND_BUSWIDTH_4 & header->capability)
			bus_width = BUS_WIDTH_4;
		break;
	case FLASH_EMMC:
		if (MMC_BUSWIDTH_4 & header->capability)
			bus_width = BUS_WIDTH_4;
		if (MMC_BUSWIDTH_8 & header->capability)
			bus_width = BUS_WIDTH_8;
		break;
	}

	return bus_width;
}

static int flash_get_bus_clk(u32 flash_type, struct img_header *header)
{
	int bus_clk = DEFAULT_EMMC_CLK;

	switch (flash_type) {
	case FLASH_SD:
		//TBD:
		bus_clk = DEFAULT_SD_CLK;
		break;

	case FLASH_EMMC:
		if (MMC_BUSCLK_25M & header->capability)
			bus_clk = LEGACY_EMMC_CLK;
		if (MMC_BUSCLK_50M & header->capability)
			bus_clk = HS_EMMC_CLK;
		break;

	case FLASH_NAND_2K:
	case FLASH_NAND_4K:
		bus_clk = SPINAND_DEFAULT_CLK;
		if (SPI_NAND_BUSCLK_25M & header->capability)
			bus_clk = SPINAND_25M_CLK;
		if (SPI_NAND_BUSCLK_50M & header->capability)
			bus_clk = SPINAND_50M_CLK;
		break;

	case FLASH_NOR:
		bus_clk = SPINOR_DEFAULT_CLK;
		if (SPI_NOR_BUSCLK_25M & header->capability)
			bus_clk = SPINOR_25M_CLK;
		if (SPI_NOR_BUSCLK_50M & header->capability)
			bus_clk = SPINOR_50M_CLK;
		break;
	}

	return bus_clk;
}
#endif

int verify_img_header(struct img_header *header)
{
	u32 cksum = 0;

	if(header->magic_data != IMG_HEADER_MAGIC_DATA)
	{
		AX_LOG_STR_ERROR("\r\nverify_img_header: magic ");
		AX_LOG_HEX_ERROR(IMG_HEADER_MAGIC_DATA);
		AX_LOG_STR_ERROR(" != ");
		AX_LOG_HEX_ERROR(header->magic_data);
		return -1;
	}
	cksum = calc_word_chksum((int *)&header->capability, sizeof(struct img_header) - 8);
	if(cksum != header->check_sum)
	{
		AX_LOG_STR_ERROR("\r\nverify_img_header: cksum ");
		AX_LOG_HEX_ERROR(cksum);
		AX_LOG_STR_ERROR(" != ");
		AX_LOG_HEX_ERROR(header->check_sum);
		return -1;
	}

	return 0;
}

int read_img_header(u32 flash_type, struct img_header *header, struct img_header *boot_header, struct boot_image_info *img_info)
{
	int sel_clk;
	u32 flash_addr, flash_addr_bk;
	u32 bus_width = BUS_WIDTH_8;
	/* read image header, try different bus frequency if failed
	 * emmc/sd use 12Mhz
	 */
	if (!boot_header || !header || !img_info)
		return -1;
	flash_addr = img_info->boot_header_flash_addr;
	flash_addr_bk = img_info->boot_header_flash_bk_addr;

	switch(flash_type) {
		#if defined(AX620E_EMMC)
		case FLASH_EMMC_BOOT_4BIT_25M_768K:
		case FLASH_EMMC_BOOT_4BIT_25M_128K:
			bus_width = BUS_WIDTH_4; // use 4bit
		case FLASH_EMMC_BOOT_8BIT_50M_768K:
		case FLASH_EMMC:
			/* if is emmc, we use 50Mhz freq, do not reduce frequency processing. */
			sel_clk = 0;
			break;
		#endif
		case FLASH_SD:
			/* if is sd, we use 25Mhz freq, do not reduce frequency processing. */
			bus_width = BUS_WIDTH_4;
			sel_clk = 2;
			break;
		#if defined(AX620E_NAND) || defined(AX620E_NOR)
		case FLASH_NAND_4K:
		case FLASH_NOR:
		case FLASH_NAND_2K:
			if (img_info->boot_index >= 2)
				curr_cs = 1;
			else
				curr_cs = 0;
		#endif
		default:
			bus_width = BUS_WIDTH_4; // use 4bit
			sel_clk = 1;
			break;
	}
	#if defined(AX620E_NAND)
	/* decode nand org info */
	if (flash_type  == FLASH_NAND_2K || flash_type  == FLASH_NAND_4K) {
		spinand_page_size_shift = (header->nand_nor_cfg & GENMASK(1, 0)) + 11;
		spinand_blk_size_shift =  ((header->nand_nor_cfg & GENMASK(4, 2)) >> 2) + 16;
		spinand_oob_size_shift = ((header->nand_nor_cfg & GENMASK(6, 5)) >> 5) + 6;
		spinand_oob_len = (header->nand_nor_cfg & BIT(7)) ? 2 : 1;
		spinand_planes = (header->nand_ext_cfg & GENMASK(1, 0));
		spinand_rx_sample_delay[0] = header->nand_rx_sample_delay[0];
		spinand_rx_sample_delay[1] = header->nand_rx_sample_delay[1];
		spinand_rx_sample_delay[2] = header->nand_rx_sample_delay[2];
		spinand_phy_setting[0] = header->nand_phy_setting[0];
		spinand_phy_setting[1] = header->nand_phy_setting[1];
		spinand_phy_setting[2] = header->nand_phy_setting[2];
	}
	#endif
	#if defined(AX620E_NOR)
	if (flash_type == FLASH_NOR) {
		spinor_qe_rdsr = (header->nand_nor_cfg & GENMASK(15, 8)) >> 8;
		spinor_qe_wrsr = (header->nand_nor_cfg & GENMASK(23, 16)) >> 16;
		spinor_qe_bit = (header->nand_nor_cfg & GENMASK(31, 24)) >> 24;
		spinor_qe_cfg_parsred = 1;
		spinor_rx_sample_delay[0] = header->nor_rx_sample_delay[0];
		spinor_rx_sample_delay[1] = header->nor_rx_sample_delay[1];
		spinor_rx_sample_delay[2] = header->nor_rx_sample_delay[2];
		spinor_phy_setting[0] = header->nor_phy_setting[0];
		spinor_phy_setting[1] = header->nor_phy_setting[1];
		spinor_phy_setting[2] = header->nor_phy_setting[2];
	}
	#endif

	if (support_ab)
		flash_addr = ((img_info->boot_index == 0) || (img_info->boot_index == 2)) ? flash_addr : flash_addr_bk;

retry_read_bk:
	if(flash_init(flash_type, flash_clk_array[sel_clk], bus_width) < 0) {
		AX_LOG_STR_ERROR("spl flash init failed\r\n");
		return BOOT_FLASH_INIT_FAIL;
	}
	if(flash_read(flash_type, (char *)boot_header, flash_addr, sizeof(struct img_header)) < 0) {
		AX_LOG_STR_ERROR("spl flash read failed\r\n");
		return BOOT_FLASH_READ_FAIL;
	}

	if(verify_img_header((struct img_header *)boot_header) < 0) {
		AX_LOG_STR_ERROR("spl verify header failed\r\n");
		if(support_ab)
			return BOOT_READ_HEADER_FAIL;

		if(flash_addr == flash_addr_bk || flash_addr_bk == 0) {
			return BOOT_READ_HEADER_FAIL;
		} else {
			flash_addr = flash_addr_bk;
			goto retry_read_bk;
		}
	}

	return 0;
}

#ifdef SECURE_BOOT_TEST
static int ce_init_configed = 0;
/* secure boot or encryption is not enabled in efuse, but the subsequent mirror of spl is enabled. You can enable ce by using this macro. */
int ce_hw_init(u32 flash_type, struct img_header *header, struct img_header *boot_header)
{
	if (ce_init_configed)
		return 0;
	if (!boot_header || !header)
		return -1;
	char *img_addr = (char *)((u64)boot_header + sizeof(struct img_header));
	int use_dma_copy_fw = 0;
	int bak_enable = (header->capability & FW_BAK_ENABLE) ? 1 : 0;
	int times = 1;
	int i;
	u32 flash_addr = header->fw_flash_addr;
	/* if secure_en enabled, bootrom already init ce, just return */

	if((bak_enable == 1)) {
		times = 2;
	} else {
		times = 1;
	}

	/* read ce fw */
	for(i = 0; i < times; i++) {
		if(i == 1) {
			flash_addr = header->fw_bak_flash_addr;
			img_addr = (char *)(u64)(SYS_OCM_BASE + header->fw_bak_flash_addr);
		}

		if(flash_read(flash_type, img_addr, flash_addr, (header->fw_size + sizeof(u32) - 1) / sizeof(u32) * sizeof(u32)) < 0)
		{
			info("read ce fw data faield\r\n");
			continue;
		}
		/* verify checksum if needed */
		if(boot_header->capability & FW_CHECK_ENABLE) {
			/* read the bankup ce fw if checksum failed */
			if (calc_word_chksum((int *)img_addr, header->fw_size) != header->fw_check_sum) {
				info("ce fw checksum faield\r\n");
				continue;
			}
		}
		break;
	}
	if(i == times)
		return -1;

	/* run fw to initial ce hw */
	if (secure_init((u64)(img_addr), header->fw_size, use_dma_copy_fw) < 0)
	{
		info("ce fw initial faield\r\n");
		return BOOT_EIP_INIT_FAIL;
	}
	info("ce fw initial pass\r\n");
	ce_init_configed = 1;
	return 0;
}
#endif

#define PUB_KEY_ARRAY_MAX_SZ  (396)
int read_image_data(u32 flash_type, struct img_header *boot_header, struct boot_image_info *img_info)
{
	if (!boot_header || !img_info)
		return -1;
	int ret = 0;
	u32 cksum = 0;
	u32 flash_addr = img_info->boot_flash_addr;
	char public_key[PUB_KEY_ARRAY_MAX_SZ];
	char *img_addr = (char *)((unsigned long)boot_header + sizeof(struct img_header));
	#ifdef SECURE_BOOT_TEST
	int cipher_en = (boot_header->capability & IMG_CIPHER_ENABLE) ? 1 : 0;
	int secure_en = 1;
	#else
	int cipher_en = (boot_header->capability & IMG_CIPHER_ENABLE) ? 1 : 0;
	int secure_en = is_secure_enable();
	#endif
	int check_enable = (boot_header->capability & IMG_CHECK_ENABLE) ? 1 : 0;
	int bak_enable = (boot_header->capability & IMG_BAK_ENABLE) ? 1 : 0;
	int key_len = (boot_header->capability & RSA_3072_MODE) ? 3072 : 2048;

	/* The ddrinit partition temporarily skips the security check. */
	if (img_info->img_type == DDRINIT) {
		secure_en = 0;
		cipher_en = 0;
	}

	if (secure_en) {
		ax_memset(public_key, 0, PUB_KEY_ARRAY_MAX_SZ);
		ax_memcpy((void *)(public_key), (void *)&boot_header->key_n_header, (4 + key_len / 8));
		ax_memcpy((void *)(public_key + (4 + key_len / 8)), (void *)&boot_header->key_e_header, 8);
		if (public_key_verify((unsigned long)(public_key), sizeof(public_key)) < 0)
		{
			info("read img data public_key_verify faield\r\n");
			return SECURE_PUBLIC_KEY_CHECK_FAIL;
		}
	}

	if(support_ab) {
		flash_addr = (img_info->boot_index == 0) || (img_info->boot_index == 2) ? img_info->boot_flash_addr : img_info->boot_flash_bk_addr;
	}

retry_read_bk:
#ifdef SUPPPORT_GZIPD
	if(img_info->img_type != DDRINIT) {
		if(gzip_pipeline_flash_read(flash_type, img_addr, flash_addr, (boot_header->img_size + sizeof(u32) - 1) / sizeof(u32) * sizeof(u32)) < 0) {
			AX_LOG_STR_ERROR("spl flash read failed\r\n");
			if (check_enable) {
				ret = BOOT_FLASH_READ_FAIL;
			}
			else {
				return BOOT_FLASH_READ_FAIL;
			}
		}
	}
	else {
#endif
		/* read image data */
		if (flash_read(flash_type, img_addr, flash_addr, (boot_header->img_size + sizeof(u32) - 1) / sizeof(u32) * sizeof(u32)) < 0) {
			AX_LOG_STR_ERROR("read img data flash_read faield\r\n");
			return BOOT_FLASH_READ_FAIL;
		}
#ifdef SUPPPORT_GZIPD
	}
#endif

#ifdef SUPPPORT_GZIPD
	if(img_info->img_type != DDRINIT)
		#if defined(AX620E_NAND)
		img_addr = (void *)(IMAGE_COMPRESSED_PADDR + sizeof(struct img_header));
		#else
		img_addr = (char *)IMAGE_COMPRESSED_PADDR;
		#endif
#endif
	if (check_enable) {
	        /* read backup if checksum failed */
	        cksum = calc_word_chksum((int *)(img_addr), boot_header->img_size);
	        if (cksum != boot_header->img_check_sum) {
	                AX_LOG_STR_ERROR("\r\nread_image_data: cksum ");
	                AX_LOG_HEX_ERROR(cksum);
	                AX_LOG_STR_ERROR(" != ");
	                AX_LOG_HEX_ERROR(boot_header->img_check_sum);
	                if(support_ab || !bak_enable)
	                        return BOOT_IMG_CHECK_FAIL;

	                if(flash_addr == img_info->boot_flash_bk_addr || img_info->boot_flash_bk_addr == 0x400) {
	                        return BOOT_IMG_CHECK_FAIL;
	                } else {
	                        flash_addr = img_info->boot_flash_bk_addr;
	                        goto retry_read_bk;
	                }
	        }
	        else if (ret < 0) {
	                return ret;
	        }
	}

	if (secure_en) {
		if (cipher_sha256(img_addr, (char *)hash_digest, boot_header->img_size) < 0)
		{
			info("read img data cipher_sha256 faield\r\n");
			return BOOT_SHA_FAIL;
		}
		if (cipher_rsa_verify(public_key, (char *)&boot_header->sig_header, hash_digest, key_len) < 0) {
			info("read img data cipher_rsa_verify faield\r\n");
			return BOOT_RSA_FAIL;
		}
	}
	/* decrypto if crypto enabled */
	if (cipher_en) {
		get_aes_key((int *)aes_value);
		if (cipher_aes_ecb_decrypto(aes_value, (unsigned long)boot_header->aes_key, (unsigned long)boot_header->aes_key, sizeof(boot_header->aes_key)) < 0)
		{
			info("read header aes key cipher_aes_ecb_decrypto faield\r\n");
			return BOOT_AES_FAIL;
		}
		if (cipher_aes_ecb_decrypto((int *)boot_header->aes_key, (unsigned long)img_addr, (unsigned long)img_addr, boot_header->img_size) < 0)
		{
			info("read img data cipher_aes_ecb_decrypto faield\r\n");
			return BOOT_AES_FAIL;
		}
	}
#ifdef SUPPPORT_GZIPD
	if(img_info->img_type != DDRINIT)
		img_addr =  (char *)((unsigned long)boot_header + sizeof(struct img_header));
#endif
#ifdef AX_SPL_SUPPORT_MODIFY_BOOTARGS
	if(img_info->img_type == DTB) {
		update_cmdline(bootargs);
		update_fdt_bootargs((void *)img_addr, bootargs);
	}
#endif
	return (int)(unsigned long)img_addr;
}

static void get_image_info(u32 flash_type, int boot_index, boot_img_e img_type, boot_image_info_t *image_info, u32 flash_ops, u32 flash_bk_ops)
{
	if(flash_type == FLASH_SD) {
		boot_image_file = sd_img_name[img_type];
		image_info->boot_header_flash_addr = 0;
		image_info->boot_header_flash_bk_addr = 0;
		image_info->boot_flash_addr = sizeof(struct img_header);
		image_info->boot_flash_bk_addr = sizeof(struct img_header);
	} else {
		image_info->img_type = img_type;
		image_info->boot_index = boot_index;
		image_info->boot_header_flash_addr = flash_ops;
		image_info->boot_header_flash_bk_addr = flash_bk_ops;
		image_info->boot_flash_addr = flash_ops + sizeof(struct img_header);
		image_info->boot_flash_bk_addr = flash_bk_ops + sizeof(struct img_header);
	}
}

int boot_process(u32 flash_type, int boot_index, boot_img_e img_type, u32 flash_ops, u32 flash_bk_ops, long ram_ops)
{
	int ret;
#ifdef SUPPORT_RISCV
	int riscv_addr;
	u32 with_riscv = 0, riscv_chksum_result;
#endif
	struct img_header *header = (struct img_header *)SPL_IMG_HEADER_BASE;
	boot_image_info_t image_info;
	struct img_header *boot_header = (struct img_header *)ram_ops;

	get_image_info(flash_type, boot_index, img_type, &image_info, flash_ops, flash_bk_ops);

#ifdef SUPPORT_RISCV
	/* if with riscv, jump riscv to run */
	with_riscv = (header->capability & RISCV_EXISTS) ? 1 : 0;

	if (with_riscv) {
		riscv_addr = (u64)(SYS_OCM_BASE + header->riscv_flash_addr);
		riscv_chksum_result = (readl(TRACE_MEM_BASE + 0x24) & BOOT_RISCV_CHECKSUM_PASS) >> 28;
		info("boot_process riscv_chksum_result = %d\r\n", riscv_chksum_result);
		/* return riscv_addr; */
	}
#endif
	ret = read_img_header(flash_type, header, boot_header, &image_info);
	if(ret < 0) {
		err("read img header failed\r\n");
		return ret;
	}

#if defined(AX620E_NAND) && defined (SUPPPORT_GZIPD)
	if ((img_type != DDRINIT) &&
		((FLASH_NAND_2K == flash_type) || (FLASH_NAND_4K == flash_type))) {
		ret = spinand_read(flash_ops, (boot_header->img_size + sizeof(struct img_header) + sizeof(u32) - 1) / sizeof(u32) * sizeof(u32),
			(char *)IMAGE_COMPRESSED_PADDR);
		if (ret < 0) {
			err("spinand read img data failed\r\n");
			return ret;
		}
	}
#endif

	#ifdef SECURE_BOOT_TEST
	ret = ce_hw_init(flash_type, header, boot_header);
	if(ret < 0) {
		return ret;
	}
	#endif
	ret = read_image_data(flash_type, boot_header, &image_info);
	if(ret < 0) {
		err("read img data failed\r\n");
		return ret;
	}
	return ret;
}

static int select_slot_ab(void)
{
	u32 slottype = 0;
	u32 slota, slotb, slota_bootable, slotb_bootable;

	slottype = readl(TOP_CHIPMODE_GLB_BACKUP0);
	slota = slottype & SLOTA;
	slotb = slottype & SLOTB;
	slota_bootable = slottype & SLOTA_BOOTABLE;
	slotb_bootable = slottype & SLOTB_BOOTABLE;

	if(slota == 0 && slotb == 0) {
		/* set to slota */
		slottype &= ~(SLOTA_BOOTABLE | SLOTB_BOOTABLE | SLOTB);
		slottype |=  SLOTA;
		writel(slottype, TOP_CHIPMODE_GLB_BACKUP0);
		warn("power off boot, slot A\r\n");
		/* slota */
		return 0;
	}
	if (slota != 0) {
		if (slota_bootable != 0) {
			slottype &= ~(SLOTA_BOOTABLE | SLOTB);
			slottype |=  SLOTA;
			writel(slottype, TOP_CHIPMODE_GLB_BACKUP0);
			warn("slot A\r\n");
			/* slota */
			return 0;
		} else {
			slottype &= ~(SLOTA_BOOTABLE | SLOTA);
			slottype |=  SLOTB;
#ifdef AX_BOOT_OPTIMIZATION_SUPPORT
			slottype |=  BOOT_KERNEL_FAIL;
#endif
			writel(slottype, TOP_CHIPMODE_GLB_BACKUP0);
			warn("try slot B\r\n");
			/* slotb */
			return 1;
		}
	}
	if (slotb != 0) {
		if (slotb_bootable != 0) {
			slottype &= ~(SLOTB_BOOTABLE | SLOTA);
			slottype |=  SLOTB;
			writel(slottype, TOP_CHIPMODE_GLB_BACKUP0);
			warn("slot B\r\n");
			/* slotb */
			return 1;
		} else {
			slottype &= ~(SLOTB_BOOTABLE | SLOTB);
			slottype |=  SLOTA;
#ifdef AX_BOOT_OPTIMIZATION_SUPPORT
			slottype |=  BOOT_KERNEL_FAIL;
#endif
			writel(slottype, TOP_CHIPMODE_GLB_BACKUP0);
			warn("try slot A\r\n");
			/* slota */
			return 0;
		}
	}
	return slottype;
}
static int boot_index;
static int select_slot_configed;
int flash_boot(u32 flash_type, boot_img_e boot_type, u32 flash_ops, u32 flash_bk_ops, long ram_ops)
{
	int ret = 0;

	/* enable wtd during boot */
	wtd_enable(1);
#ifndef AX620E_EMMC
	axi_dma_hw_init();
#endif
#ifdef SUPPPORT_GZIPD
	gzipd_dev_init();
#endif

	if (!select_slot_configed && support_ab && (boot_type != DDRINIT)) {
		boot_index = select_slot_ab();
		select_slot_configed = 1;
	}
	warn("%s: boot_index:0x%x\r\n", __func__, boot_index);
	ret = boot_process(flash_type, boot_index, boot_type, flash_ops, flash_bk_ops, ram_ops);
	if(ret < 0) {
#ifdef AX_BOOT_OPTIMIZATION_SUPPORT
		/* spl boot failed, watchdog restarts after a 2S*2 timeout and enters uboot. */
		if (!support_ab)
			writel(BOOT_KERNEL_FAIL, TOP_CHIPMODE_GLB_BACKUP0);
		writel(0, 0x484000C);
		writel(1, 0x4840018);
		udelay(50);
		writel(0, 0x4840018);
		AX_LOG_STR_ERROR("boot process failed\r\n");
#endif
		info("boot_process %d failed\r\n", boot_type);
		return -1;
	}

	return ret;
}
