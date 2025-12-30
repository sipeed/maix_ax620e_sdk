/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef CONFIG_BUILD_ENGINE_TINY
#include <rtthread.h>
#define algo_log_print(format, ...)  rt_kprintf(format, ##__VA_ARGS__)
#else
#include <stdio.h>
#define algo_log_print(format, ...)  printf(format, ##__VA_ARGS__)
#endif

#define ALOG_LOG(format, ...) \
    do { \
        algo_log_print(format, ##__VA_ARGS__); \
    } while (0)

#define ALOG_LOG_BASE(msg_type, format, ...) \
    do { \
        algo_log_print("[ALGO][%s][%s %d]: " format "\n", msg_type, __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define ALOG_LOG_ERR(format, ...)   ALOG_LOG_BASE("Error", format, ##__VA_ARGS__)
#define ALOG_LOG_WARN(format, ...)  ALOG_LOG_BASE("Warning", format, ##__VA_ARGS__)
#define ALOG_LOG_INFO(format, ...)  ALOG_LOG_BASE("Info", format, ##__VA_ARGS__)
#define ALOG_LOG_DBG(format, ...)   ALOG_LOG_BASE("Debug", format, ##__VA_ARGS__)

#define ITP_CMM_ALIGN_SIZE      128


/*!
 * @brief Allocates memory.
 * @param [in]  size: Size in byte.
 * @return Pointer of allocated memory.
 */
void* sys_malloc(size_t size);


/*!
 * @brief Deallocates previously allocated memory.
 * @param [in]  ptr: Pointer of previously allocated memory.
 */
void sys_free(void* ptr);


//!< interface of allocate this neural network device memory
int cmm_alloc(void** vir, uint64_t* phy, size_t size);


//!< interface of allocate this neural network device cached memory
int cmm_alloc_cache(void** vir, uint64_t* phy, size_t size);


//!< interface of release allocated resources.
int cmm_free(void* vir, uint64_t phy);


//!< interface of invalidate allocated resources cpu cache.
int cmm_invalidate(void* vir, uint64_t phy, size_t size);


//!< interface of flush allocated resources cpu cache.
int cmm_flush(void* vir, uint64_t phy, size_t size);
