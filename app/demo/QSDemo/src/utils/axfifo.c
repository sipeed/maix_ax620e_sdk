/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "axfifo.h"

#define min(x, y)    ((x) < (y) ? (x) : (y))
#define is_pow_of_two(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define size_of_fifo(fifo)  (fifo->in - fifo->out)
#define capacity_of_fifo(fifo)  (fifo->mask + 1)
#define left_of_fifo(fifo)  ((fifo->mask + 1) - (fifo->in - fifo->out))

static AX_VOID mem_barrier()
{
    //__asm__ __volatile__("": : :"memory");
    //asm volatile ("isync" : : : "memory");
    __sync_synchronize();
}

static AX_U32 roundup_pow_of_two(AX_U32 x)
{
    AX_S32 position = 0;
    AX_S32 i;

    for (i = (x-1); i != 0; ++position)
        i >>= 1;

    return 1UL << position;
}

AX_S32 axfifo_create(axfifo_ptr fifo, AX_U32 size) {
    if (!fifo) {
        return -EINVAL;
    }

    fifo->in = 0;
    fifo->out = 0;

    if(size < 2) {
        fifo->data = NULL;
        fifo->mask = 0;
        return -EINVAL;
    }

    if(!is_pow_of_two(size))
        size = roundup_pow_of_two(size);

    fifo->data = malloc(size);

    if (!fifo->data) {
        fifo->mask = 0;
        return -ENOMEM;
    }
    fifo->mask = size - 1;

    return 0;
}

AX_S32 axfifo_destory(axfifo_ptr fifo) {
    if (!fifo) {
        return -EINVAL;
    }
    if (fifo->data) {
        free(fifo->data);
        fifo->data = NULL;
    }
    fifo->in = 0;
    fifo->out = 0;
    fifo->mask = 0;
    return 0;
}

AX_U32 axfifo_capacity(axfifo_ptr fifo) {
    if (!fifo) {
        return 0;
    }

    return (fifo->mask + 1);
}

AX_U32 axfifo_size(axfifo_ptr fifo) {
    if (!fifo) {
        return 0;
    }
    return (fifo->in - fifo->out);
}

AX_U32 axfifo_left(axfifo_ptr fifo) {
    if (!fifo) {
        return 0;
    }

    return (fifo->mask + 1) - (fifo->in - fifo->out);
}

AX_U32 axfifo_put(axfifo_ptr fifo, const AX_VOID *buf, AX_U32 len) {
    if (!fifo || !buf) {
        return 0;
    }

    AX_U32 off = 0;
    AX_U32 size = left_of_fifo(fifo);  // left capacity

    /* ensure left enough space to store the buf*/
    if (len > size) {
        return 0;
    }

    /* that is why request size is power of two, instead of off %= fifo->size */
    off = fifo->in & fifo->mask;
    size = min(len, capacity_of_fifo(fifo) - off);  // first part from in
    memcpy(fifo->data + off, buf, size);  // copy first part
    memcpy(fifo->data, buf + size, len - size); // copy second part

    /* make sure that the data in the fifo is up to date before
     * incrementing the fifo->in index counter
     */
    mem_barrier();

    fifo->in += len;
    return len;
}

AX_U32 axfifo_pop(axfifo_ptr fifo, AX_VOID *buf, AX_U32 len) {

    AX_U32 off = 0;
    AX_U32 size = size_of_fifo(fifo);
    if (len > size) {
        len = size;
    }

    if (buf) {
        off = fifo->out & fifo->mask;
        size = min(len, capacity_of_fifo(fifo) - off);
        memcpy(buf, fifo->data + off, size);
        memcpy(buf + size, fifo->data, len - size);
    }

    /* make sure that the data is copied before
     * incrementing the fifo->out index counter
     */
    mem_barrier();

    fifo->out += len;
    return len;
}

AX_U32 axfifo_clear(axfifo_ptr fifo) {
    mem_barrier();
    fifo->in = 0;
    fifo->out = 0;
    return 0;
}

AX_U32 axfifo_peek(axfifo_ptr fifo, AX_VOID *buf, AX_U32 len) {
    AX_U32 off = 0;
    AX_U32 size = size_of_fifo(fifo);
    if (len > size) {
        len = size;
    }

    off = fifo->out & fifo->mask;
    size = min(len, capacity_of_fifo(fifo) - off);
    memcpy(buf, fifo->data + off, size);
    memcpy(buf + size, fifo->data, len - size);

    /* make sure that the data is copied before
     * incrementing the fifo->out index counter
     */
    mem_barrier();

    return len;
}

AX_U32 axfifo_peek_nocopy(axfifo_ptr fifo, axfifo_data_info_ptr di,  AX_U32 len) {
    AX_U32 off = 0;
    AX_U32 size = size_of_fifo(fifo);
    if (len > size) {
        len = size;
    }

    off = fifo->out & fifo->mask;
    size = min(len, capacity_of_fifo(fifo) - off);
    di->data[0].buf = fifo->data + off;
    di->data[0].len = size;

    di->data[1].len = len - size;
    if (di->data[1].len > 0) {
        di->data[1].buf = fifo->data;
    } else {
        di->data[1].buf = NULL;
    }

    return len;
}

