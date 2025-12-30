/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_MAILBOX_H__
#define __DRV_MAILBOX_H__

#include <stdint.h>

#define MBOX_ID_RECV_SENSOR    0x00
#define MBOX_ID_SEND_SENSOR    0x05
#define MBOX_ID_RECV_VERIFY    0xff

typedef struct {
    uint8_t id;
    uint8_t data[31];
} mbox_msg_t; // mailbox message length is 32 bytes

typedef int(*mbox_callback_t)(mbox_msg_t *msg, void *data);

int mbox_auto_send_message(mbox_msg_t *msg);
int mbox_register_callback(mbox_callback_t callback, void *data, uint8_t id);
int mbox_unregister_callback(uint8_t id);

#endif //__DRV_MAILBOX_H__

