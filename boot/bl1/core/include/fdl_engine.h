/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __FDL_ENGINE_H
#define __FDL_ENGINE_H
#include "fdl_frame.h"

typedef int (*cmd_handler_t) (fdl_frame_t *pframe, void *arg);

struct fdl_cmdproc {
	//CMD_TYPE cmd;
	cmd_handler_t handler;
	void *arg;
};

struct fdl_file_info {
	u32 start_addr;
	u32 curr_addr;
	u64 target_len;
	u32 recv_len;
	u32 image_id;
	u32 secure_en;
};

void jump_to_execute(u32 start_addr);
void fw_load_and_exec();

#endif
