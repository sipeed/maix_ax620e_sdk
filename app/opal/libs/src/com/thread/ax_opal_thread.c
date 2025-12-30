/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

AX_OPAL_THREAD_T* AX_OPAL_CreateThread(void (*run_func)(void *arg), void *arg) {

    AX_OPAL_THREAD_T* thread = (AX_OPAL_THREAD_T*)malloc(sizeof(AX_OPAL_THREAD_T));
    memset(thread, 0x0, sizeof(AX_OPAL_THREAD_T));
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    thread->run_func = run_func;
    thread->arg = arg;
    thread->eState = AX_OPAL_THREAD_STATE_INIT;
    thread->is_running = false;
    return thread;
}

void AX_OPAL_DestroyThread(AX_OPAL_THREAD_T *thread) {
    if (thread->is_running) {
        AX_OPAL_StopThread(thread);
    }
    pthread_mutex_destroy(&thread->mutex);
    pthread_cond_destroy(&thread->cond);
    thread->eState = AX_OPAL_THREAD_STATE_IDLE;
    free(thread);
}

void AX_OPAL_StartThread(AX_OPAL_THREAD_T *thread) {
    pthread_mutex_lock(&thread->mutex);
    if (thread->eState == AX_OPAL_THREAD_STATE_INIT) {
        thread->eState = AX_OPAL_THREAD_STATE_RUNNING;
        thread->is_running = true;
        pthread_create(&thread->thread_id, NULL, AX_OPAL_RunThread, thread);
    }
    pthread_mutex_unlock(&thread->mutex);
}

void *AX_OPAL_RunThread(void *arg) {
    // printf("RunThread ++++\n");
    AX_OPAL_THREAD_T *thread = (AX_OPAL_THREAD_T *)arg;
    pthread_mutex_lock(&thread->mutex);
    while (thread->is_running) {
        if (thread->eState == AX_OPAL_THREAD_STATE_RUNNING) {
            pthread_mutex_unlock(&thread->mutex);
            /* while true */
            thread->run_func(thread);
            thread->eState = AX_OPAL_THREAD_STATE_FINISH;
            pthread_mutex_lock(&thread->mutex);
        } else {
            pthread_cond_wait(&thread->cond, &thread->mutex);
        }
    }
    // if (thread->eState != AX_OPAL_THREAD_STATE_STOPPED || thread->eState != AX_OPAL_THREAD_STATE_ERR) {
    //     thread->eState != AX_OPAL_THREAD_STATE_FINISH;
    // }
    pthread_mutex_unlock(&thread->mutex);
    pthread_exit(NULL);
    return NULL;
}

void AX_OPAL_StopThread(AX_OPAL_THREAD_T *thread) {
    pthread_mutex_lock(&thread->mutex);
    if (thread->eState != AX_OPAL_THREAD_STATE_STOPPED) {
        thread->is_running = false;
        pthread_cond_signal(&thread->cond);
        thread->eState = AX_OPAL_THREAD_STATE_STOPPED;
        pthread_mutex_unlock(&thread->mutex);
        pthread_join(thread->thread_id, NULL);
    } else {
        pthread_mutex_unlock(&thread->mutex);
    }
}

void AX_OPAL_PauseThread(AX_OPAL_THREAD_T *thread) {
    pthread_mutex_lock(&thread->mutex);
    if (thread->eState == AX_OPAL_THREAD_STATE_RUNNING) {
        thread->eState = AX_OPAL_THREAD_STATE_PAUSED;
    }
    pthread_mutex_unlock(&thread->mutex);
}

void AX_OPAL_ResumeThread(AX_OPAL_THREAD_T *thread) {
    pthread_mutex_lock(&thread->mutex);
    if (thread->eState == AX_OPAL_THREAD_STATE_PAUSED) {
        thread->eState = AX_OPAL_THREAD_STATE_RUNNING;
        pthread_cond_signal(&thread->cond);
    }
    pthread_mutex_unlock(&thread->mutex);
}