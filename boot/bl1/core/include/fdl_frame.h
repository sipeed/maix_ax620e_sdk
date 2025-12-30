/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __FDL_FRAME_H
#define __FDL_FRAME_H
#include "cmn.h"

#define FDL_MAGIC_SIZE            4
#define FDL_DATALEN_SIZE          2
#define FDL_CMDIDX_SIZE           2
#define FDL_CHECKSUM_SIZE         2
#define FRAME_EXCEPT_DATA_SIZE    10
#define FRAME_DATA_MAXLEN         (FDL_FRAME_MAX_SIZE - FRAME_EXCEPT_DATA_SIZE)
#define FDL_FRAME_MAX_SIZE        64


typedef enum {
	FDL_CMD_TYPE_MIN                = 0,
	FDL_CMD_CONNECT                 = FDL_CMD_TYPE_MIN,
	FDL_CMD_START_TRANSFER          = 1,
	FDL_CMD_START_TRANSFER_DATA     = 2,
	FDL_CMD_START_TRANSFER_END      = 3,
	FDL_CMD_EXECUTE                 = 4,
	FDL_CMD_HANDSHAKE               = 5,
	FDL_CMD_ENABLE_QUAD             = 7,
	FDL_CMD_UPDATE_FREQ             = 8,
	FDL_CMD_CHANGE_BAUD             = 9,
	FDL_CMD_SLV_FREQ_BOOST          = 10,
	FDL_CMD_TYPE_MAX,

	FDL_RESP_TYPE_MIN               = 0x80,
	FDL_RESP_ACK                    = FDL_RESP_TYPE_MIN,
	FDL_RESP_VERSION                = 0x81,
	FDL_RESP_INVLID_CMD             = 0x82,
	FDL_RESP_UNKNOWN_CMD            = 0x83,
	FDL_RESP_OPERATION_FAIL         = 0x84,
	FDL_EIPFW_INVALID               = 0x85,
	FDL_EIPFW_DATA_ERR              = 0x86,
	FDL_EIPFW_HASH_ERR              = 0x87,
	FDL_RESP_SECURE_SIGNATURE_ERR   = 0x88,
	FDL_RESP_DEST_ERR               = 0x89,
	FDL_RESP_SIZE_ERR               = 0x8A,
	FDL_RESP_VERIFY_ERROR           = 0x8B,
	FDL_RESP_IMG_HEADER_ERROR       = 0x8C,
	FDL_RESP_TYPE_MAX
} CMD_TYPE;

typedef struct fdl_frame{
	u32 magic_num;
	u16 data_len;
	u16 cmd_index;
	u8 data[FRAME_DATA_MAXLEN];  //checksum add to the end of data
} fdl_frame_t;

int frame_send_version();
int frame_send_respone(CMD_TYPE resp);
int frame_send(CMD_TYPE cmd, u8 * txbuf, u32 txlen, u32 timeout_ms);
fdl_frame_t *frame_get(u32 timeout_ms);

#endif
