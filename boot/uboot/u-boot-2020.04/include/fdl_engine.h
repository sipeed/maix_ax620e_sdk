/*
 * (C) Copyright 2020 Aixin-Chip
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __FDL_ENGINE_H__
#define __FDL_ENGINE_H__

#include <linux/sizes.h>

#define CMD_PART_ID_SIZE         36

struct fdl_file_info {
	u_long start_addr;	//ddr_base - text_base
	u_long curr_addr;
	u64 target_len;		//bin file size
	u64 recv_len;
	u64 unsave_recv_len;	//data in buffer waitting for write to medium, 2M
	char part_name[32];	//same size as disk_partition.name
	u8 chksum_flag;
	u32 target_chksum;
	u32 recv_chksum;
	u64 full_img_size;
};

struct fdl_read_info {
	u_long buf_addr;	//ddr_base - text_base
	u_long buf_point;
	u_long buf_len;		//bin file size
	u_long buf_send_len;
	u64 request_len;
	u64 total_send_len;
	u64 part_base_addr;
	u64 part_size;
	u64 part_read_size;
	char part_name[32];	//same size as disk_partition.name
	u8 chksum_flag;
	u32 send_chksum;
};

#define PARTITION_HEADER_MAGIC            0x3A726170	//"par:"
typedef struct fdl_partition_header {
	u32 dwMagic;		// fix to PARTITION_HEADER_MAGIC
	u8 byte_version;	// fix to 1
	u8 size_unit;		// 0: 1MB; 1: 512KB; 2: 1KB; 3: 1B
	u16 part_count;		// Count of PARTITION_BODY_T
} fdl_partition_header_t;

typedef struct fdl_partition {
	u16 part_id[CMD_PART_ID_SIZE];	// Specifies the partition name, eg:’system’
	u64 part_sizeof_unit;	// Specifies the partition size in unit
	u64 part_gapof_unit;	// gap size between two size, most times is 0
	u64 sizeof_bytes;
	u64 gapof_bytes;
} fdl_partition_t;

void fdl_dl_init(void);
void fdl_dl_entry(void);
#endif
