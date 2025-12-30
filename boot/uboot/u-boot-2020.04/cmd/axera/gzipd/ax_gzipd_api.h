/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_GZIPD_API_H_
#define _AX_GZIPD_API_H_

typedef struct {
	u8 type[2];
	u16 blk_num;
	u32 osize;
	u32 isize;
	u32 icrc32;
} gzipd_header_info_t;

typedef enum {
	gzipd_error = -1,
	gzipd_ok = 0,
} gzipd_ret_e;

int gzipd_dev_init(void);
int gzipd_dev_get_header_info(void *pdata, gzipd_header_info_t *pInfo);
int gzipd_dev_cfg(u64 tileSize, void *out_addr, u64 isize, u64 osize, u32 blk_num, u32 *cnt, u32 *last_size);
int gzipd_dev_run(void *tiles_start_addr, void *tiles_end_start, u32 *queued_num);
int gzipd_dev_run_last_tile(void *last_tile_start_addr, u64 last_tile_size);
int gzipd_dev_get_fifo_level(void);
int gzipd_dev_wait_complete_finish(void);
int gzipd_dev_get_status(u32 *stat_1, u32 *stat_2);
int gzipd_dev_clk_auto_gate(bool close);

#endif