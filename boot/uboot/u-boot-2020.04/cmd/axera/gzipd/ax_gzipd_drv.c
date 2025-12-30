/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <common.h>
#include <time.h>
#include <blk.h>
#include <asm/io.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include "crc32_mpeg2.h"
#include "ax_gzipd_reg.h"
#include "ax_gzipd_api.h"
#include "crc32_mpeg2.h"

#define CMDQ_DEPTH           (16)
#define GZIP_COMPLETE_INTR   (0x1 << 0)
#define ALMOST_EMPTY_INTR    (0x1 << 1)

#define ALMOST_EMPTY_LEVEL_SET	(0xA)

static void *gzipdRegBaseVirAddr = NULL;
static s32 gzipdTriggerFlag = FALSE;
static u64 gzipdTileSize = 0;

static u8  *gzipdInputDataAddr = NULL;
static u32  gzipdInputDataSize = 0;
static u32  gzipdInDataGoldenCRC = 0;

int gzipd_dev_show_regvalue(void);
int gzipd_dev_is_corrupt(void);

static u32 gzip_reg_read(u32 addr_offset)
{
	if (gzipdRegBaseVirAddr) {
		return __raw_readl(gzipdRegBaseVirAddr + addr_offset);
	} else {
		AX_GZIP_LOG_ERR("gzipd reg base addr wasn't mapped when read\n");
		return -1;
	}
}

static void gzip_reg_write(u32 addr_offset, u32 val)
{
	if (gzipdRegBaseVirAddr) {
		__raw_writel(val, gzipdRegBaseVirAddr + addr_offset);
	} else {
		AX_GZIP_LOG_ERR("gzipd reg base addr isn't mapped when write\n");
	}
}

static int gzipd_check_crc_is_valid(void)
{
	u32 crc;
	u32 offset;
	offset = sizeof(gzipd_header_info_t);
	crc = do_crc_table(gzipdInputDataAddr + offset, gzipdInputDataSize - offset);
	AX_GZIP_LOG_DBG("Computed CRC[0x%x], Golden CRC[0x%x]\n", crc, gzipdInDataGoldenCRC);
	return crc == gzipdInDataGoldenCRC;
}

static int wait_intr(u32 intr_mask)
{
	s32 ret = gzipd_ok;
	u32 loop = 5000;
	while ((gzip_reg_read(GZIPD_STATUS1_REG) & intr_mask) == 0 && --loop) {
		udelay(1*1000);
	}
	if (loop == 0) {
		AX_GZIP_LOG_ERR("gzipd wait complete interrupt timeout 5s\n");
		gzipd_dev_show_regvalue();
		if (!gzipd_check_crc_is_valid()) {
			AX_GZIP_LOG_ERR("gzipd input data's CRC error\n");
		}
		gzipd_dev_clk_auto_gate(0);
		return gzipd_error;
	}
	gzipd_dev_clk_auto_gate(1);
	return ret;
}

int gzipd_dev_init(void)
{
	int val;

	// SW_RST
	__raw_writel(GZIPD_SW_RST | GZIPD_CORE_SW_RST, FLASH_SYS_GLB_BASE_ADDR + FLASH_SW_RST0_SET_REG);
	udelay(10);
	__raw_writel(GZIPD_SW_RST | GZIPD_CORE_SW_RST, FLASH_SYS_GLB_BASE_ADDR + FLASH_SW_RST0_CLR_REG);

	gzipd_dev_clk_auto_gate(0);
	// select clk
	val = __raw_readl(FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_MUX0_REG);
	__raw_writel(val | NPLL_533M << 9 | 0x5 << 6, FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_MUX0_REG);

	gzipdRegBaseVirAddr = (void *)AX_GZIPD_BASE_PADDR;

#ifdef GZIPD_BYPASS_EN
	val = __raw_readl(FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_MUX0_REG);
	__raw_writel(val | NPLL_533M << 9 | 0x5 << 6, FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_MUX0_REG);
	gzip_reg_write(GZIPD_WDMA_CFG1_REG, osize-1);
#endif

	return gzipd_ok;
}

int gzipd_dev_clk_auto_gate(bool close)
{
	if (close) {
		__raw_writel(CLK_GZIPD_CORE_EB_BIT, FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_EN_0_CLR_REG);
		__raw_writel(CLK_GZIPD_EB_BIT, FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_EN_1_CLR_REG);
	} else {
		__raw_writel(CLK_GZIPD_CORE_EB_BIT, FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_EN_0_SET_REG);
		__raw_writel(CLK_GZIPD_EB_BIT, FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_EN_1_SET_REG);
	}
	return gzipd_ok;
}

int gzipd_dev_get_header_info(void *pData, gzipd_header_info_t *pInfo)
{
	u8 type[2];
	if (pData == NULL || pInfo == NULL) {
		AX_GZIP_LOG_ERR(" input param is NULL, pData = 0x%p, pInfo = 0x%p", pData, pInfo);
		return gzipd_error;
	}

	memcpy(&type[0], pData, 2);
	if (memcmp(&type[0], "20", 2)) {
		AX_GZIP_LOG_ERR("This data is without gzipd header info [%x, %x]", *(char *)pData, *((char *)pData + 1));
		return gzipd_error;
	}
	memcpy(pInfo, pData, sizeof(gzipd_header_info_t));
	AX_GZIP_LOG_DBG(" ZIP data info .blk_num = %d, .isize = %d, osize = %d, icrc = %d",
					pInfo->blk_num, pInfo->isize, pInfo->osize, pInfo->icrc32);

	gzipdInputDataAddr = (u8 *)pData;
	gzipdInputDataSize = pInfo->isize;
	gzipdInDataGoldenCRC = pInfo->icrc32;

	return gzipd_ok;
}

int gzipd_dev_cfg(u64 tileSize, void *out_addr, u64 isize, u64 osize, u32 blk_num, u32 *cnt, u32 *last_size)
{
	u32 tile_cnt;
	const u64 MinTileSz = 8 * 1024;
	u64 validDataLen;
#ifdef AX_GZIPD_BYPASS_EN
	u32 bypass = 1;
#else
	u32 bypass = 0;
#endif

	if (tileSize % MinTileSz && !(tileSize / MinTileSz)) {
		AX_GZIP_LOG_ERR("tile size MUST more than 8KB and a intergral multiple of it \n");
		return gzipd_error;
	}

	if (out_addr == NULL) {
		AX_GZIP_LOG_ERR("output address is NULL");
		return gzipd_error;
	}

	if (isize == 0 || osize == 0 || blk_num == 0) {
		AX_GZIP_LOG_ERR("isize , osize or blk_num is 0");
		return gzipd_error;
	}

	validDataLen = isize - sizeof(gzipd_header_info_t);
	gzipdTileSize = tileSize;
	if (validDataLen % gzipdTileSize < MinTileSz) {
		tile_cnt = validDataLen / gzipdTileSize;
	} else {
		tile_cnt = validDataLen / gzipdTileSize + 1;
	}

	gzip_reg_write(GZIPD_WDMA_CFG1_REG, osize-1);
	gzip_reg_write(GZIPD_WDMA_CFG0_REG, (long)out_addr >> 3);
	if (tile_cnt == 0) {
		gzip_reg_write(GZIPD_RDMA_CFG0_REG, (ALMOST_EMPTY_LEVEL_SET << 11) | (blk_num << 16) | bypass << 31);
	} else {
		gzip_reg_write(GZIPD_RDMA_CFG0_REG, (tile_cnt - 1) | (ALMOST_EMPTY_LEVEL_SET << 11) | (blk_num << 16) | bypass << 31);
	}

	*cnt = tile_cnt;
	*last_size = validDataLen - (tile_cnt - 1) * gzipdTileSize;
	return gzipd_ok;
}

int gzipd_dev_run(void *tiles_start_addr,  void *tiles_end_addr, u32 *queued_num)
{
	s32 idx;
	u64 start;
	u64 end;
	u64 tile_addr;
	u32 tiles_cnt;
	u32 reg_cmd_fifo_val;
	u32 cur_cmdq_available_num;

#ifdef GZIPD_CHECK_TIME_COMSUM
	u64 start_ts;
#endif

	start = (long)tiles_start_addr;
	end = (long)tiles_end_addr;
	AX_GZIP_LOG_DBG("tile data range [0x%8llx, 0x%8llx]", start, end);
	if ((end - start) % gzipdTileSize != 0) {
		AX_GZIP_LOG_ERR("data length is not an integer multiple of TILE_SIZE");
		return gzipd_error;
	}

	if (gzipd_dev_is_corrupt()) {
		return gzipd_error;
	}
#ifdef GZIPD_CHECK_TIME_COMSUM
	start_ts = get_timer_us(0);
	AX_GZIP_LOG_DBG("[%lld us ]config done, begin to run", start);

#endif
	tiles_cnt = (end - start) / gzipdTileSize;
	tile_addr = start;
	reg_cmd_fifo_val = (gzip_reg_read(GZIPD_STATUS0_REG) >> 16) & 0x1F;
	cur_cmdq_available_num = CMDQ_DEPTH - reg_cmd_fifo_val;
	AX_GZIP_LOG_DBG("tiles_cnt = %d, current cmdq empty num = %d", tiles_cnt, cur_cmdq_available_num);
	if (tiles_cnt > cur_cmdq_available_num) {
		tiles_cnt = cur_cmdq_available_num;
	}

	for (idx = 0; idx < tiles_cnt;) {
		gzip_reg_write(GZIPD_RDMA_CFG1_REG, tile_addr >> 3);
		gzip_reg_write(GZIPD_RDMA_CFG2_REG, gzipdTileSize - 1);
		tile_addr += gzipdTileSize;
		idx++;
		AX_GZIP_LOG_DBG("tile_addr[%3d] = 0x%llx", idx, tile_addr);
	}
	// gzip_reg_write(GZIPD_RDMA_CFG1_REG, tile_addr >> 3);
	// gzip_reg_write(GZIPD_RDMA_CFG2_REG, last_tile_size - 1);
	// trigger gzipd to work
	if (gzipdTriggerFlag == FALSE) {
		gzipdTriggerFlag = TRUE;
		gzip_reg_write(GZIPD_CTRL_CFG_REG,0x1f01u);
		AX_GZIP_LOG_DBG(" start to run gzipd.......");
	}

	*queued_num = idx;
	return gzipd_ok;
}

int gzipd_dev_run_last_tile(void *last_tile_start_addr,  u64 last_tile_size)
{
	if (last_tile_start_addr == NULL) {
		AX_GZIP_LOG_ERR(" tile addr is NULL");
		return gzipd_error;
	}

	if (gzipd_dev_is_corrupt()) {
		return gzipd_error;
	}

	if (gzipdTriggerFlag == FALSE) {
		gzipdTriggerFlag = TRUE;
		gzip_reg_write(GZIPD_CTRL_CFG_REG,0x1f01u);
		AX_GZIP_LOG_DBG(" start to run gzipd.......");
	}

	gzip_reg_write(GZIPD_RDMA_CFG1_REG, (long)last_tile_start_addr >> 3);
	gzip_reg_write(GZIPD_RDMA_CFG2_REG, last_tile_size - 1);

	gzipdTriggerFlag = FALSE;
	return gzipd_ok;
}

int gzipd_dev_get_fifo_level(void)
{
	return (gzip_reg_read(GZIPD_STATUS0_REG) >> 16) & 0x1F;
}

int gzipd_dev_wait_complete_finish(void)
{
	return wait_intr(GZIP_COMPLETE_INTR);
}

int gzipd_dev_get_status(u32 *stat_1, u32 *stat_2)
{
	u32 status1;
	u32 status2;
	u32 idx;
	#define STATUS1_MASK_BITMAP	 (0x1FF)
	#define STATUS2_MASK_BITMAP	 (0x3F)
	status1 = gzip_reg_read(GZIPD_STATUS1_REG) & STATUS1_MASK_BITMAP;
	*stat_1 = status1;
	status2 = gzip_reg_read(GZIPD_STATUS2_REG) & STATUS2_MASK_BITMAP;
	*stat_2 = status1;

	for (idx = 0; idx < 9; idx++) {
		u32 bit = (status1 >> idx ) & 0x1;
		if (!bit) {
			continue;
		}
		switch (idx) {
			case 0: {
				AX_GZIP_LOG_DBG("gzip complete ok");
				break;
			}
			case 1: {
				AX_GZIP_LOG_DBG("gzip almost complete ok");
				break;
			}
			case 2: {
				AX_GZIP_LOG_ERR("gzip iblock crc error");
				break;
			}
			case 3: {
				AX_GZIP_LOG_ERR("gzip oblock crc error");
				break;
			}
			case 4: {
				AX_GZIP_LOG_ERR("gzip uncompressed size is larger than osize_m1 error");
				break;
			}
			default: {
				AX_GZIP_LOG_ERR("gzip rresp or bresp error");
				break;
			}
		}
	}

	for (idx = 0; idx < 6; idx++) {
		u32 bit = (status2 >> idx ) & 0x1;
		if (!bit) {
			continue;
		}
		switch (idx) {
			case 0: {
				AX_GZIP_LOG_ERR("gzip rdma cmd busy");
				break;
			}
			case 1: {
				AX_GZIP_LOG_ERR("gzip rdma axi ost busy");
				break;
			}
			case 2: {
				AX_GZIP_LOG_ERR("gzip rdma data busty");
				break;
			}
			case 3: {
				AX_GZIP_LOG_ERR("gzip wdma cmd busy");
				break;
			}
			case 4: {
				AX_GZIP_LOG_ERR("gzip wdma axi ost busy");
				break;
			}
			case 5: {
				AX_GZIP_LOG_ERR("gzip wdma data busy");
				break;
			}
		}
	}
	return gzipd_ok;
}

int gzipd_dev_show_regvalue(void)
{
	int row;
	u32 val0; u32 val1;
	u32 val2; u32 val3;
	AX_GZIP_LOG_ERR(" >>>>>>>>>>>>>>>>>>>>>>>> gzip reg value >>>>>>>>>>>>>>>>>>>>>>>>");
	for (row = 0; row < 3; row++) {
		val0 = gzip_reg_read(row * 0x10 + 0 * 4);
		val1 = gzip_reg_read(row * 0x10 + 1 * 4);
		val2 = gzip_reg_read(row * 0x10 + 2 * 4);
		val3 = gzip_reg_read(row * 0x10 + 3 * 4);
		AX_GZIP_LOG_ERR("    offset-%d0 : [0x%08x, 0x%08x, 0x%08x, 0x%08x]  ", row, val0, val1, val2, val3);
	}
	AX_GZIP_LOG_ERR(" <<<<<<<<<<<<<<<<<<<<<<<< gzip reg value <<<<<<<<<<<<<<<<<<<<<<<<");
	return gzipd_ok;
}
int gzipd_dev_is_corrupt(void)
{
	u32 statusReg1;
	statusReg1 = gzip_reg_read(GZIPD_STATUS1_REG);
	if (statusReg1 & (0x7 << 3)) {
		AX_GZIP_LOG_ERR("gzipd GZIPD_STATUS1_REG = 0x%x", statusReg1);
		return gzipd_error;
	}
	return gzipd_ok;
}
