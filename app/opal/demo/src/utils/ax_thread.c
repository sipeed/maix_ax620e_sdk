/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

OPAL_THREAD_T* OPAL_CreateThread(void (*run_func)(void *arg), void *arg) {

    OPAL_THREAD_T* thread = (OPAL_THREAD_T*)malloc(sizeof(OPAL_THREAD_T));
    memset(thread, 0x0, sizeof(OPAL_THREAD_T));
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    thread->run_func = run_func;
    thread->arg = arg;
    thread->eState = OPAL_THREAD_STATE_INIT;
    thread->is_running = 0;
    return thread;
}

void OPAL_DestroyThread(OPAL_THREAD_T *thread) {
    if (thread->is_running) {
        OPAL_StopThread(thread);
    }
    pthread_mutex_destroy(&thread->mutex);
    pthread_cond_destroy(&thread->cond);
    thread->eState = OPAL_THREAD_STATE_IDLE;
    free(thread);
}

void OPAL_StartThread(OPAL_THREAD_T *thread) {
    pthread_mutex_lock(&thread->mutex);
    if (thread->eState == OPAL_THREAD_STATE_INIT) {
        thread->eState = OPAL_THREAD_STATE_RUNNING;
        thread->is_running = 1;
        pthread_create(&thread->thread_id, NULL, OPAL_RunThread, thread);
    }
    pthread_mutex_unlock(&thread->mutex);
}

void* OPAL_RunThread(void *arg) {
    // printf("RunThread ++++\n");
    OPAL_THREAD_T *thread = (OPAL_THREAD_T *)arg;
    pthread_mutex_lock(&thread->mutex);
    while (thread->is_running) {
        if (thread->eState == OPAL_THREAD_STATE_RUNNING) {
            pthread_mutex_unlock(&thread->mutex);
            /* while true */
            thread->run_func(thread);
            thread->eState = OPAL_THREAD_STATE_FINISH;
            pthread_mutex_lock(&thread->mutex);
        } else {
            pthread_cond_wait(&thread->cond, &thread->mutex);
        }
    }
    // if (thread->eState != OPAL_THREAD_STATE_STOPPED || thread->eState != OPAL_THREAD_STATE_ERR) {
    //     thread->eState != OPAL_THREAD_STATE_FINISH;
    // }
    pthread_mutex_unlock(&thread->mutex);
    pthread_exit(NULL);
    return NULL;
}

void OPAL_StopThread(OPAL_THREAD_T *thread) {
    pthread_mutex_lock(&thread->mutex);
    if (thread->eState != OPAL_THREAD_STATE_STOPPED) {
        thread->is_running = 0;
        pthread_cond_signal(&thread->cond);
        thread->eState = OPAL_THREAD_STATE_STOPPED;
        pthread_mutex_unlock(&thread->mutex);
        pthread_join(thread->thread_id, NULL);
    } else {
        pthread_mutex_unlock(&thread->mutex);
    }
}

void OPAL_PauseThread(OPAL_THREAD_T *thread) {
    pthread_mutex_lock(&thread->mutex);
    if (thread->eState == OPAL_THREAD_STATE_RUNNING) {
        thread->eState = OPAL_THREAD_STATE_PAUSED;
    }
    pthread_mutex_unlock(&thread->mutex);
}

void OPAL_ResumeThread(OPAL_THREAD_T *thread) {
    pthread_mutex_lock(&thread->mutex);
    if (thread->eState == OPAL_THREAD_STATE_PAUSED) {
        thread->eState = OPAL_THREAD_STATE_RUNNING;
        pthread_cond_signal(&thread->cond);
    }
    pthread_mutex_unlock(&thread->mutex);
}