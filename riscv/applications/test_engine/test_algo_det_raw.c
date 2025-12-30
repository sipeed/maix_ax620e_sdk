/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "image.h"

#include "algo_process.h"
#include "algo_utilities.h"

#include <stdint.h>
#include <stdlib.h>

#define ENGINE_THREAD_STACK_SIZE    2560
#define ENGINE_THREAD_PRIORITY      6
#define ENGINE_THREAD_TICK_SLICE    5

#define ALG_DET_CSS_NUM             1
#define ALG_DET_NMS_THR             0.6f
#define ALG_DET_MIN_CONF            0.5f
#define ALG_DET_FRAME_WIDTH         960
#define ALG_DET_FRAME_HEIGHT        544

#if 0
static void test_engine_thread(void* parameter)
{
    ALOG_LOG_DBG("Test app working...\n");
    alg_info_t info = {0};

    // config
    info.cfg.conf_threshold     = ALG_DET_MIN_CONF;         //!< notice: depends on training process of model
    info.cfg.nms_threshold      = ALG_DET_NMS_THR;          //!< notice: depends on training process of model
    info.cfg.frame.w            = ALG_DET_FRAME_WIDTH;      //!< notice: depends on uniform defined raw width
    info.cfg.frame.h            = ALG_DET_FRAME_HEIGHT;     //!< notice: depends on uniform defined raw height
    info.cfg.letterbox.w        = ALG_DET_FRAME_WIDTH;      //!< notice: depends on cropping & padding value of frame
    info.cfg.letterbox.h        = ALG_DET_FRAME_HEIGHT;     //!< notice: depends on cropping & padding value of frame
    info.cfg.class_count        = ALG_DET_CSS_NUM;          //!< notice: depends on training process of model

    int ret = alg_init(&info);
    if (0 != ret)
    {
        ALOG_LOG_ERR("Init failed.");
    }

    if (0 == ret)
    {
        ret = alg_det(&info, raw_image);
        if (0 != ret)
        {
            ALOG_LOG_ERR("Detection failed.");
        }
    }

    if (0 == ret)
    {
        ALOG_LOG_DBG("Detected {%d} objects:", info.objects.inserted_count);
        for (int m = 0; m < info.objects.inserted_count; m++)
        {
            det_bbox_t* box = &info.objects.boxes[m];

            // Notice: rt-thread cannot print float default
            ALOG_LOG_DBG("[DET][Object] {%2d.%1d%%} -> {%4d, %4d, %4d, %4d}.",
                (int)(box->prob * 100.f),
                (int)(box->prob * 1000.f) % 10,
                (int)(roundf(box->x)),
                (int)(roundf(box->y)),
                (int)(roundf(box->x + box->w)),
                (int)(roundf(box->y + box->h)));
        }

        ret = alg_release(&info);
        if (0 != ret)
        {
            ALOG_LOG_ERR("Release failed.");
        }
    }

#ifndef CONFIG_BUILD_ENGINE_TINY
    *((int*)(parameter)) = ret;
#endif
}


#ifndef CONFIG_BUILD_ENGINE_TINY
static void print_usage()
{
    ALOG_LOG("Usage: test_engine [STACK PRIORITY SLICE]\n");
    ALOG_LOG("       test_engine\n");
    ALOG_LOG("       test_engine %d %d %d\n", ENGINE_THREAD_STACK_SIZE, ENGINE_THREAD_PRIORITY, ENGINE_THREAD_TICK_SLICE);
}


int test_engine(int argc, char* argv[])
{
    if (!(4 == argc || 2 == argc || 1 == argc))
    {
        print_usage();
        return 0;
    }

    int stack_size = ENGINE_THREAD_STACK_SIZE;
    int priority = ENGINE_THREAD_PRIORITY;
    int time_slice = ENGINE_THREAD_TICK_SLICE;

    if (4 == argc)
    {
        stack_size = atoi(argv[1]);
        priority = atoi(argv[2]);
        time_slice = atoi(argv[3]);
    }

    ALOG_LOG_DBG("Test thread will start {stack: %d, priority: %d, slice: %d}...", stack_size, priority, time_slice);

    rt_thread_t tid = rt_thread_create("engine", test_engine_thread, RT_NULL, stack_size, priority, time_slice);
    if(tid == RT_NULL)
    {
        ALOG_LOG_ERR("Create thread failed.");
        return -1;
    }
    ALOG_LOG_DBG("Test thread tid: %p.\n", tid);
    
    rt_err_t ret = rt_thread_startup(tid);
    if (RT_EOK != ret)
    {
        ALOG_LOG_ERR("Startup thread failed.");
        return -1;
    }

    return 0;
}
#endif


#ifdef CONFIG_BUILD_ENGINE_TINY
int main(int argc, char* argv[])
{
    int ret = 0;
    test_engine_thread(&ret);

    return ret;
}
#endif
#endif
