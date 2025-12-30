/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef AXOP_COM_THREAD_H
#define AXOP_COM_THREAD_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/prctl.h>

typedef enum {
    AX_OPAL_THREAD_STATE_IDLE,
    AX_OPAL_THREAD_STATE_INIT,
    AX_OPAL_THREAD_STATE_ERR,
    AX_OPAL_THREAD_STATE_RUNNING,
    AX_OPAL_THREAD_STATE_PAUSED,
    AX_OPAL_THREAD_STATE_STOPPED,
    AX_OPAL_THREAD_STATE_FINISH,
} AX_OPAL_THREAD_STATET_E;

typedef struct _AXOP_COM_THREAD_T {
    AX_OPAL_THREAD_STATET_E eState;
    pthread_t thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool is_running;
    void (*run_func)(void *arg);
    void *arg;
} AX_OPAL_THREAD_T;

AX_OPAL_THREAD_T* AX_OPAL_CreateThread(void (*run_func)(void *arg), void *arg);
void AX_OPAL_DestroyThread(AX_OPAL_THREAD_T *thread);
void AX_OPAL_StartThread(AX_OPAL_THREAD_T *thread);
void*AX_OPAL_RunThread(void *arg);
void AX_OPAL_StopThread(AX_OPAL_THREAD_T *thread);
void AX_OPAL_PauseThread(AX_OPAL_THREAD_T *thread);
void AX_OPAL_ResumeThread(AX_OPAL_THREAD_T *thread);

#endif // AXOP_COM_THREAD_H