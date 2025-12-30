/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_gzipd_adapter.h"
#include "ax_gzipd_reg.h"
#include "ax_gzipd_api.h"
#ifdef AX_GZIPD_CRC_ENABLE
#include "crc32_mpeg2.h"
#endif

#define CMDQ_DEPTH           (16)
#define GZIP_COMPLETE_INTR   (0x1 << 0)
#define ALMOST_EMPTY_INTR    (0x1 << 1)

static void *gzipdRegBaseVirAddr = NULL;
static u32 gzipdTriggerFlag = FALSE;
static u64 gzipdTileSize = 0;

#ifdef AX_GZIPD_CRC_ENABLE
static u8  *gzipdInputDataAddr = NULL;
static u32  gzipdInputDataSize = 0;
static u32  gzipdInDataGoldenCRC = 0;
#endif

static u32 gzip_reg_read(u32 addr_offset)
{
	return __raw_readl(gzipdRegBaseVirAddr + addr_offset);
}

static void gzip_reg_write(u32 addr_offset, u32 val)
{
	__raw_writel(val, gzipdRegBaseVirAddr + addr_offset);
}

#ifdef AX_GZIP_DEBUG_LOG_EN
static int gzipd_dev_show_regvalue(void)
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
#endif

#ifdef AX_GZIPD_CRC_ENABLE
static int gzipd_check_crc_is_valid(void)
{
	u32 crc;
	u32 offset;
	offset = sizeof(gzipd_header_info_t);
	crc = do_crc_table(gzipdInputDataAddr + offset, gzipdInputDataSize - offset);
	AX_GZIP_LOG_ERR("Computed CRC[0x%x], Golden CRC[0x%x]\n", crc, gzipdInDataGoldenCRC);
	return crc == gzipdInDataGoldenCRC;
}
#endif

static void gzipd_dev_show_key_reg_value(void)
{
	struct regnameval{
		char  *reg_name;
		u32 reg_offset;
	} key_register[] = {
		{.reg_name = "ctrl", .reg_offset = GZIPD_CTRL_CFG_REG },
		{.reg_name = "wdma_cfg1", .reg_offset = GZIPD_WDMA_CFG1_REG },
		{.reg_name = "sts0", .reg_offset = GZIPD_STATUS0_REG },
		{.reg_name = "sts1", .reg_offset = GZIPD_STATUS1_REG },
		{.reg_name = "sts2", .reg_offset = GZIPD_STATUS2_REG }
	};
	for (int i = 0; i < sizeof(key_register)/sizeof(key_register[0]); i++) {
		AX_GZIP_LOG_ERR("gzip %s_reg.val = 0x%08x\r\n", key_register[i].reg_name, gzip_reg_read(key_register[i].reg_offset));
	}
}

static int wait_intr(u32 intr_mask)
{
	u32 loop = 5000;
	while ((gzip_reg_read(GZIPD_STATUS1_REG) & intr_mask) == 0 && --loop) {
		udelay(1*1000);
	}
	if (loop == 0 || (gzip_reg_read(GZIPD_STATUS1_REG) & 0x1C)) {
		AX_GZIP_LOG_ERR("gzipd wait complete interrupt timeout 5s, left loop = %d\n", loop);
		gzipd_dev_show_key_reg_value();
#ifdef AX_GZIP_DEBUG_LOG_EN
		gzipd_dev_show_regvalue();
#endif
#ifdef AX_GZIPD_CRC_ENABLE
		if (!gzipd_check_crc_is_valid()) {
			AX_GZIP_LOG_ERR("gzipd input data's CRC error\n");
		}
#endif
		return gzipd_error;
	}
	return gzipd_ok;
}

int gzipd_dev_init(void)
{
	// SW_RST
	__raw_writel(GZIPD_SW_RST | GZIPD_CORE_SW_RST, FLASH_SYS_GLB_BASE_ADDR + FLASH_SW_RST0_SET_REG);
	udelay(10);
	__raw_writel(GZIPD_SW_RST | GZIPD_CORE_SW_RST, FLASH_SYS_GLB_BASE_ADDR + FLASH_SW_RST0_CLR_REG);

	// select clk
	__raw_writel(NPLL_533M << 9 | 0x5 << 6, FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_MUX0_REG_SET);

	gzipdRegBaseVirAddr = (void *)AX_GZIPD_BASE_PADDR;

#ifdef GZIPD_BYPASS_EN
	__raw_writel(NPLL_533M << 9 | 0x5 << 6, FLASH_SYS_GLB_BASE_ADDR + FLASH_CLK_MUX0_REG_SET);
	gzip_reg_write(GZIPD_WDMA_CFG1_REG, osize-1);
#endif

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

#ifdef AX_GZIPD_CRC_ENABLE
	gzipdInputDataAddr = (u8 *)pData;
	gzipdInputDataSize = pInfo->isize;
	gzipdInDataGoldenCRC = pInfo->icrc32;
#endif
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
	gzip_reg_write(GZIPD_WDMA_CFG0_REG, (unsigned long)out_addr >> 3);
	if (tile_cnt == 0) {
		gzip_reg_write(GZIPD_RDMA_CFG0_REG, (0x10 << 11) | (blk_num << 16) | bypass << 31);
	} else {
		gzip_reg_write(GZIPD_RDMA_CFG0_REG, (tile_cnt - 1) | (0x10 << 11) | (blk_num << 16) | bypass << 31);
	}

	*cnt = tile_cnt;
	*last_size = validDataLen - (tile_cnt - 1) * gzipdTileSize;
	return gzipd_ok;
}

int gzipd_dev_run(void *tiles_start_addr,  void *tiles_end_addr, u32 *queued_num)
{
	u32 idx;
	unsigned long start;
	unsigned long end;
	unsigned long tile_addr;
	u32 tiles_cnt;
	u32 reg_cmd_fifo_val;
	u32 cur_cmdq_available_num;

#ifdef GZIPD_CHECK_TIME_COMSUM
	u64 start_ts;
#endif

	start = (unsigned long)tiles_start_addr;
	end = (unsigned long)tiles_end_addr;
	AX_GZIP_LOG_DBG("tile data range [0x%8llx, 0x%8llx]", start, end);
	if ((end - start) % gzipdTileSize != 0) {
		AX_GZIP_LOG_ERR("data length is not an integer multiple of TILE_SIZE");
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
	gzip_reg_write(GZIPD_RDMA_CFG1_REG, (unsigned long)last_tile_start_addr >> 3);
	gzip_reg_write(GZIPD_RDMA_CFG2_REG, last_tile_size - 1);
	if (gzipdTriggerFlag == FALSE) {
		gzipdTriggerFlag = TRUE;
		gzip_reg_write(GZIPD_CTRL_CFG_REG,0x1f01u);
		AX_GZIP_LOG_DBG(" start to run gzipd.......");
	}
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
