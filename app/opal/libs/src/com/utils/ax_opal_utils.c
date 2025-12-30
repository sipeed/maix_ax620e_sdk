/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_opal_utils.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

AX_VOID ax_opal_us_sleep(AX_U32 microseconds)
{
    struct timespec ts = {
        (time_t)(microseconds / 1000000),
        (long)((microseconds % 1000000) * 1000)
        };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
}

AX_VOID ax_opal_ms_sleep(AX_U32 milliseconds)
{
    struct timespec ts = {
        (time_t)(milliseconds / 1000),
        (long)((milliseconds % 1000) * 1000000)
        };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
}

AX_U64 ax_opal_get_timestamp()
{
    struct timeval  tTime;
    gettimeofday(&tTime, NULL);
    return  ((AX_U64)tTime.tv_sec * 1000 + (AX_U64)tTime.tv_usec / 1000);
}

AX_U64 ax_opal_get_clocktime()
{
    struct timespec ts = {0};
    int nError = clock_gettime(CLOCK_MONOTONIC, &ts);

    if (0 == nError)
    {
        return  (AX_U64)ts.tv_sec * 1000 + (AX_U64)ts.tv_nsec / 1000000;
    }
    else
    {
        printf("Get clock time fail with code: %d.\n", errno);
        return 0;
    }
}

AX_U64  ax_opal_get_nanotime()
{
    struct timespec ts = {0};
    int nError = clock_gettime(CLOCK_MONOTONIC, &ts);

    if (0 == nError)
    {
        return  1000000LU * (AX_U64)ts.tv_sec + (AX_U64)ts.tv_nsec / 1000LU;
    }
    else
    {
        printf("Get nano time fail with code: %d.\n", errno);
        return 0;
    }
}