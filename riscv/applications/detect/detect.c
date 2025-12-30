#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include "ax_base_type.h"
#include "ax_log.h"
#include "ax_env.h"
#include <rthw.h>

#include "mc20e_e907_interrupt.h"
#include "soc.h"
#include "drv_sw_int.h"
#include "ax_vin_api.h"
#include "algo_process.h"
#include "algo_utilities.h"
#include "detect.h"
#include "ax_common.h"

static void do_reboot(void)
{
    ALOG_LOG_DBG("enter");
    ax_writel(0x80, 0x239002c);
    ax_writel(1, COMMON_SYS_GLB_BASE + CHIP_SW_RST_SET); //reset chip, entry reboot
}

static void entry_sleep(void)
{
    riscv_sw_int_trigger(sw_int_group_3, sw_int_channel_24); //entry sleep
}

static void entry_raw_detect(void* parameter)
{
    int ret, i;
    unsigned int detect_count = 1;
    alg_info_t info = {0};
    AX_VIN_SCALER_BUFFER_T data = {0};

    ALOG_LOG_DBG("Raw detect working...\n");

    char *detect_num_str = fw_getenv(FW_ENV_RAW_DET_COUNT);
    if (detect_num_str != NULL)
        detect_count = (unsigned int)atoi(detect_num_str);

    if ((detect_count == 0) || (detect_count > DETECT_MAX_COUNT))
        detect_count = DETECT_MAX_COUNT;

    for(i = 0; i < detect_count; i++) {
        ret = AX_VIN_GetExtRawFrame(0, 0, &data, 200);
        if (ret < 0) {
            ALOG_LOG_ERR("Get frame failed.");
            return;
        }

        AX_LOG_DGB("[Detect] : height = 0x%x\n", data.nHeight);
        AX_LOG_DGB("[Detect] : width = 0x%x\n", data.nWidth);
        AX_LOG_DGB("[Detect] : phyaddr = 0x%x\n", data.phy_addr);
        AX_LOG_DGB("[Detect] : size = 0x%x\n", data.size);

        if (i == 0) { //Only the first frame is initialized
            // config
            info.cfg.conf_threshold     = ALG_DET_MIN_CONF;         //!< notice: depends on training process of model
            info.cfg.nms_threshold      = ALG_DET_NMS_THR;          //!< notice: depends on training process of model
            info.cfg.frame.w            = ALG_DET_FRAME_WIDTH;      //!< notice: depends on uniform defined raw width
            info.cfg.frame.h            = ALG_DET_FRAME_HEIGHT;     //!< notice: depends on uniform defined raw height
            info.cfg.letterbox.w        = data.nWidth;              //!< notice: depends on cropping & padding value of frame
            info.cfg.letterbox.h        = data.nHeight;             //!< notice: depends on cropping & padding value of frame
            info.cfg.class_count        = ALG_DET_CSS_NUM;          //!< notice: depends on training process of model
            ret = alg_init(&info);
            if (0 != ret)
            {
                ALOG_LOG_ERR("Init failed.");
            }
        }

        if (0 == ret)
        {
            ret = alg_det(&info, (const void *)(uintptr_t)(data.phy_addr));
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
            if (info.objects.inserted_count) {
                break;
            }
        }
    }

    if (!info.objects.inserted_count)
        do_reboot(); //No object detected, do reboot

    ret = alg_release(&info);
    if (0 != ret)
    {
        ALOG_LOG_ERR("Release failed.");
    }

    entry_sleep();

#ifndef CONFIG_BUILD_ENGINE_TINY
    *((int*)(parameter)) = ret;
#endif
}

static void entry_md_detect(void* parameter)
{
    //to do
}

int do_raw_detct()
{
    AX_LOG_INFO("do_raw_detct");

    rt_thread_t tid = rt_thread_create("raw_det", entry_raw_detect, RT_NULL, ENGINE_THREAD_STACK_SIZE, ENGINE_THREAD_PRIORITY, ENGINE_THREAD_TICK_SLICE);
    if(tid == RT_NULL)
    {
        ALOG_LOG_ERR("Create thread failed.");
        return -1;
    }
    ALOG_LOG_DBG("Raw_det thread tid: %p.\n", tid);

    rt_err_t ret = rt_thread_startup(tid);
    if (RT_EOK != ret)
    {
        ALOG_LOG_ERR("Startup thread failed.");
        return -1;
    }
    return 0;
}

int do_md_detect()
{
    AX_LOG_INFO("do_md_detct");

    rt_thread_t tid = rt_thread_create("md_det", entry_md_detect, RT_NULL, ENGINE_THREAD_STACK_SIZE, ENGINE_THREAD_PRIORITY, ENGINE_THREAD_TICK_SLICE);
    if(tid == RT_NULL)
    {
        ALOG_LOG_ERR("Create thread failed.");
        return -1;
    }
    ALOG_LOG_DBG("Md_det thread tid: %p.\n", tid);

    rt_err_t ret = rt_thread_startup(tid);
    if (RT_EOK != ret)
    {
        ALOG_LOG_ERR("Startup thread failed.");
        return -1;
    }
    return 0;
}

int ax_detect_type(void)
{
#if defined(AX_FAST_RISCV_RAW_DET_SUPPORT)
    return AX_RAW_DETECT;
#elif defined(AX_FAST_RISCV_MD_DET_SUPPORT)
    return AX_MD_DETECT;
#endif
    return AX_YUV_DETECT;
}

int ax_detect(void)
{
#if defined(AX_FAST_RISCV_RAW_DET_SUPPORT)
    do_raw_detct();
#elif defined(AX_FAST_RISCV_MD_DET_SUPPORT)
    do_md_detect();
#endif
    return 0;
}


