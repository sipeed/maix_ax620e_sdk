/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rtdevice.h>
#include "drv_mailbox.h"

void mailbox_tool(void)
{
    mbox_msg_t msg = {
        .id = MBOX_ID_RECV_VERIFY,
        .data = {0xc3, 0x55, 0xaa}
    };
    mbox_auto_send_message(&msg);
}

MSH_CMD_EXPORT(mailbox_tool, mailbox test);
