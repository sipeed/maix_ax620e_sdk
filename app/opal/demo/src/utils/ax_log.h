/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_LOG_H_
#define _AX_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>


typedef enum {
    AXOP_LOG_MIN         = -1,
    AXOP_LOG_EMERGENCY   = 0,
    AXOP_LOG_ALERT       = 1,
    AXOP_LOG_CRITICAL    = 2,
    AXOP_LOG_ERROR       = 3,
    AXOP_LOG_WARN        = 4,
    AXOP_LOG_NOTICE      = 5,
    AXOP_LOG_INFO        = 6,
    AXOP_LOG_DEBUG       = 7,
    AXOP_LOG_MAX
} AXOP_LOG_LEVEL_E;

extern AXOP_LOG_LEVEL_E g_opalapp_log_level;

#if 1
#define MACRO_BLACK "\033[1;30;30m"
#define MACRO_RED "\033[1;30;31m"
#define MACRO_GREEN "\033[1;30;32m"
#define MACRO_YELLOW "\033[1;30;33m"
#define MACRO_BLUE "\033[1;30;34m"
#define MACRO_PURPLE "\033[1;30;35m"
#define MACRO_WHITE "\033[1;30;37m"
#define MACRO_END "\033[0m"
#else
#define MACRO_BLACK
#define MACRO_RED
#define MACRO_GREEN
#define MACRO_YELLOW
#define MACRO_BLUE
#define MACRO_PURPLE
#define MACRO_WHITE
#define MACRO_END
#endif


#define LOG_M_A(tag, fmt, ...)  do { if (g_opalapp_log_level >= AXOP_LOG_ALERT) {\
    struct timeval tv; struct tm t; gettimeofday(&tv, NULL);  time_t now = time(NULL); localtime_r(&now, &t);\
    printf(MACRO_YELLOW "[%02u-%02u %02u:%02u:%02u:%03u][E][%s][%s][%4d]: " fmt MACRO_END "\n", t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (AX_U32)(tv.tv_usec / 1000), tag, __FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)
#define LOG_M_C(tag, fmt, ...) do { if (g_opalapp_log_level >= AXOP_LOG_CRITICAL) {\
    struct timeval tv; struct tm t; gettimeofday(&tv, NULL);  time_t now = time(NULL); localtime_r(&now, &t);\
    printf(MACRO_YELLOW "[%02u-%02u %02u:%02u:%02u:%03u][C][%s][%s][%4d]: " fmt MACRO_END "\n", t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (AX_U32)(tv.tv_usec / 1000), tag, __FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)
#define LOG_M_E(tag, fmt, ...)  do { if (g_opalapp_log_level >= AXOP_LOG_ERROR) {\
    struct timeval tv; struct tm t; gettimeofday(&tv, NULL);  time_t now = time(NULL); localtime_r(&now, &t);\
    printf(MACRO_RED "[%02u-%02u %02u:%02u:%02u:%03u][E][%s][%s][%4d]: " fmt MACRO_END "\n", t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (AX_U32)(tv.tv_usec / 1000), tag, __FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)
#define LOG_M_W(tag, fmt, ...) do { if (g_opalapp_log_level >= AXOP_LOG_WARN) {\
    struct timeval tv; struct tm t; gettimeofday(&tv, NULL);  time_t now = time(NULL); localtime_r(&now, &t);\
    printf(MACRO_YELLOW "[%02u-%02u %02u:%02u:%02u:%03u][W][%s][%s][%4d]: " fmt MACRO_END "\n", t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (AX_U32)(tv.tv_usec / 1000), tag, __FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)
#define LOG_M_I(tag, fmt, ...) do { if (g_opalapp_log_level >= AXOP_LOG_INFO) {\
    struct timeval tv; struct tm t; gettimeofday(&tv, NULL);  time_t now = time(NULL); localtime_r(&now, &t);\
    printf(MACRO_GREEN "[%02u-%02u %02u:%02u:%02u:%03u][I][%s][%s][%4d]: " fmt MACRO_END "\n", t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (AX_U32)(tv.tv_usec / 1000), tag, __FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)
#define LOG_M_D(tag, fmt, ...) do { if (g_opalapp_log_level >= AXOP_LOG_DEBUG) {\
    struct timeval tv; struct tm t; gettimeofday(&tv, NULL);  time_t now = time(NULL); localtime_r(&now, &t);\
    printf(MACRO_WHITE "[%02u-%02u %02u:%02u:%02u:%03u][D][%s][%s][%4d]: " fmt MACRO_END "\n", t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (AX_U32)(tv.tv_usec / 1000), tag, __FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)
#define LOG_M_N(tag, fmt, ...) do { if (g_opalapp_log_level >= AXOP_LOG_NOTICE) {\
    struct timeval tv; struct tm t; gettimeofday(&tv, NULL);  time_t now = time(NULL); localtime_r(&now, &t);\
    printf(MACRO_PURPLE "[%02u-%02u %02u:%02u:%02u:%03u][N][%s][%s][%4d]: " fmt MACRO_END "\n", t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (AX_U32)(tv.tv_usec / 1000), tag, __FUNCTION__, __LINE__, ##__VA_ARGS__); }}while(0)

#endif /* _AX_LOG_H_ */
