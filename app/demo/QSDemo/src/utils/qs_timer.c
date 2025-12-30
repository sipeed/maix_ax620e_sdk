/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "qs_timer.h"
#include "qs_log.h"
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MMAP_FILE_NAME          "/dev/mem"
#define PAGE_SIZE               0x1000
#define TIMER_BASE              0x4820000
#define MAP_SIZE                0x100

#define TIME_DELTA              (30000)  // us

AX_S32 wakeup_timer_set(AX_U32 wait_time)
{
    /*
    AX_CHAR command[256] = {0};
    AX_S32 nSize = 0;
    nSize = sprintf(command, "echo %u > /proc/ax_proc/wake_timer/timers", wait_time);
    command[nSize] = 0;
    system(command);
    */

    AX_CHAR str[64] = {0};
    AX_S32 nSize = 0;
    nSize = sprintf(str, "%u", wait_time);
    str[nSize] = 0;

    FILE * fp = fopen("/proc/ax_proc/wake_timer/timers", "w+");
    if (fp) {
        fwrite(str, 1, strlen(str),fp);
        fflush(fp);
        fclose(fp);
    } else {
        ALOGE("open /proc/ax_proc/wake_timer/timers failed, error=%d", errno);
        return -1;
    }

    // AX_VOID *base;
    // AX_S32 fd = open(MMAP_FILE_NAME, O_RDWR | O_SYNC);
    // if (fd == -1) {
    //     ALOGE("wakeup_timer_set: open %s failed", MMAP_FILE_NAME);
    //     return -1;
    // }
    // base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x1000);

    // if (base == MAP_FAILED) {
    //     ALOGE("wakeup_timer_set: mmap failed");
    //     close(fd);
    //     return -1;
    // }

    // *(volatile unsigned int *)(base + 0xFC) = wait_time;

    // munmap(base, MAP_SIZE);
    // close(fd);

	return 0;
}

AX_U64 GetTickCountPts(AX_VOID) {
    AX_U64 val = 0;
    void* timer_vaddr = 0;
    int fd = 0;

    fd = open(MMAP_FILE_NAME, O_RDONLY | O_SYNC);
    if (-1 == fd) {
        return 0;
    }

    timer_vaddr = mmap(0, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, TIMER_BASE);
    if (0 == timer_vaddr) {
        close(fd);
        return 0;
    }

    val = *(volatile unsigned long long *)(timer_vaddr + 0x0);

    munmap((void*)timer_vaddr, PAGE_SIZE);
    close(fd);

    return (val/24 + TIME_DELTA);
}


AX_S32 ReadISPTimes(AX_U64 *u64Started, AX_U64 *u64Opended, AX_U64 *u64FrameReady, AX_U64 *u64OutToNext, AX_U64 *u64Fsof, AX_S32 nCamCnt) {
    long timer_vaddr = 0;
    int fd = 0;

    fd = open(MMAP_FILE_NAME, O_RDONLY | O_SYNC);
    if (-1 == fd) {
        return 0;
    }

    timer_vaddr = (long)mmap(0, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (0 == timer_vaddr) {
        close(fd);
        return 0;
    }

    u64Started[0] = (*(volatile unsigned int *)(timer_vaddr + 0x940))/24 + TIME_DELTA;
    u64Opended[0] = (*(volatile unsigned int *)(timer_vaddr + 0x944))/24 + TIME_DELTA;
    u64FrameReady[0] = (*(volatile unsigned int *)(timer_vaddr + 0x94c))/24 + TIME_DELTA;
    u64OutToNext[0] = (*(volatile unsigned int *)(timer_vaddr + 0x950))/24 + TIME_DELTA;
    u64Fsof[0] =  (*(volatile unsigned int *)(timer_vaddr + 0x988))/24 + TIME_DELTA;

    if (nCamCnt == 2) {
        u64Started[1] = (*(volatile unsigned int *)(timer_vaddr + 0x978))/24 + TIME_DELTA;
        u64Opended[1] = (*(volatile unsigned int *)(timer_vaddr + 0x97c))/24 + TIME_DELTA;
        u64FrameReady[1] = (*(volatile unsigned int *)(timer_vaddr + 0x980))/24 + TIME_DELTA;
        u64OutToNext[1] = (*(volatile unsigned int *)(timer_vaddr + 0x984))/24 + TIME_DELTA;
        u64Fsof[1] =  (*(volatile unsigned int *)(timer_vaddr + 0x98C))/24 + TIME_DELTA;
    }

    munmap((void*)timer_vaddr, PAGE_SIZE);
    close(fd);

    return 1;
}
