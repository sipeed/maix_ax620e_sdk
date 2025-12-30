/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AXFIFO_H__
#define _AXFIFO_H__
#include "ax_global_type.h"
#include <pthread.h>

typedef struct _axfifo {
    AX_U32    in;
    AX_U32    out;
    AX_U32    mask;
    AX_VOID  *data;
    //pthread_mutex_t mtx;
} axfifo_t, *axfifo_ptr;

typedef struct _axfifo_data {
    AX_VOID  *buf;
    AX_U32    len;
} axfifo_data_t, *axfifo_data_ptr;

typedef struct _axfifo_data_info {
    axfifo_data_t data[2];
} axfifo_data_info_t, *axfifo_data_info_ptr;


AX_S32 axfifo_create(axfifo_ptr fifo, AX_U32 size);  //size must be pow2
AX_S32 axfifo_destory(axfifo_ptr fifo);

AX_U32 axfifo_capacity(axfifo_ptr fifo);
AX_U32 axfifo_size(axfifo_ptr fifo);
AX_U32 axfifo_left(axfifo_ptr fifo);

AX_U32 axfifo_put(axfifo_ptr fifo, const AX_VOID *buf, AX_U32 len);
AX_U32 axfifo_pop(axfifo_ptr fifo, AX_VOID *buf, AX_U32 len);  // allow to set buf null, if buff is null, it will remove the data only
AX_U32 axfifo_clear(axfifo_ptr fifo);

AX_U32 axfifo_peek(axfifo_ptr fifo, AX_VOID *buf, AX_U32 len);
AX_U32 axfifo_peek_nocopy(axfifo_ptr fifo, axfifo_data_info_ptr di, AX_U32 len);

#endif // _AXFIFO_H__