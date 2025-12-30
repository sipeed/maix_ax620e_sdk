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
#include <sys/prctl.h>

typedef enum {
    OPAL_THREAD_STATE_IDLE,
    OPAL_THREAD_STATE_INIT,
    OPAL_THREAD_STATE_ERR,
    OPAL_THREAD_STATE_RUNNING,
    OPAL_THREAD_STATE_PAUSED,
    OPAL_THREAD_STATE_STOPPED,
    OPAL_THREAD_STATE_FINISH,
} OPAL_THREAD_STATET_E;

typedef struct _OPAL_COM_THREAD_T {
    OPAL_THREAD_STATET_E eState;
    pthread_t thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int is_running;
    void (*run_func)(void *arg);
    void *arg;
} OPAL_THREAD_T;

OPAL_THREAD_T* OPAL_CreateThread(void (*run_func)(void *arg), void *arg);
void OPAL_DestroyThread(OPAL_THREAD_T *thread);
void OPAL_StartThread(OPAL_THREAD_T *thread);
void*OPAL_RunThread(void *arg);
void OPAL_StopThread(OPAL_THREAD_T *thread);
void OPAL_PauseThread(OPAL_THREAD_T *thread);
void OPAL_ResumeThread(OPAL_THREAD_T *thread);

#endif // AXOP_COM_THREAD_H