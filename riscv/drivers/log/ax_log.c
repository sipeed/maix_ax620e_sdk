/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rtdevice.h>
#include <stdint.h>
#include "drv_uart.h"
#include "ax_log.h"
#include "ax_env.h"

#define FW_ENV_UART_LOG_TOGGLE  "riscv_uart_log_toggle"
#define FW_ENV_LOG_LEVEL        "riscv_log_level"
#define FW_ENV_UART_CHANNEL     "riscv_uart_channel"
#define FW_ENV_UART_BAUDRATE    "riscv_uart_baudrate"

#define LOG_MAGIC           0x55aa55aa
#define LOG_VERSION_LEN     8
#define LOG_VERSION         "1.0.0"

typedef struct {
    uint32_t magic;
    char version[LOG_VERSION_LEN];
    uint32_t mem_len;
    uint32_t header_addr;
    uint32_t header_len;
    uint32_t log_addr;
    uint32_t log_total_len;
    uint32_t log_mem_write;
    uint32_t log_mem_cnt;
} log_2_mem_header_t;

static log_2_mem_header_t *log_2_mem_header;
static int log_2_mem_ready = 0;

uart_log_toggle_e uart_log_toggle = uart_log_on;
enum AX_LOG_LEVEL ax_log_level = AX_LOG_LEVEL_ERROR;
uart_e uart_channel = uart_1;
char *uart_console_name = "uart1";
uart_baudrate_e uart_baudrate = uart_baudrate_921600;

void log_2_mem_store(uint8_t *log_buff, rt_size_t log_len)
{
    if (log_2_mem_ready == 0) {
        return;
    }

    rt_spin_lock("lock");

    if ((log_2_mem_header->log_mem_cnt + log_len) <= log_2_mem_header->log_total_len) {
        rt_memcpy((void *)log_2_mem_header->log_mem_write, log_buff, log_len);
        log_2_mem_header->log_mem_write += log_len;
    } else {
        uint32_t offset = log_2_mem_header->log_mem_cnt % log_2_mem_header->log_total_len;
        uint32_t left = log_2_mem_header->log_total_len - offset;
        if (left >= log_len) {
            rt_memcpy((void *)log_2_mem_header->log_mem_write, log_buff, log_len);
            log_2_mem_header->log_mem_write += log_len;
        } else {
            rt_memcpy((void *)log_2_mem_header->log_mem_write, log_buff, left);
            rt_memcpy((void *)log_2_mem_header->log_addr, log_buff + left, log_len - left);
            log_2_mem_header->log_mem_write = log_2_mem_header->log_addr + (log_len - left);
        }
    }
    if (log_2_mem_header->log_mem_write == (log_2_mem_header->log_addr + log_2_mem_header->log_total_len)) {
        log_2_mem_header->log_mem_write = log_2_mem_header->log_addr;
    }

    log_2_mem_header->log_mem_cnt += log_len;
    rt_spin_unlock("lock");
}

static void log_fw_env_init(void)
{
    char *uart_log_str = fw_getenv(FW_ENV_UART_LOG_TOGGLE);
    char *uart_chanel_str = fw_getenv(FW_ENV_UART_CHANNEL);
    char *uart_baudrate_str = fw_getenv(FW_ENV_UART_BAUDRATE);
    char *log_level_str = fw_getenv(FW_ENV_LOG_LEVEL);

    if (rt_strcmp(uart_log_str, "on") == 0) {
        uart_log_toggle = uart_log_on;
    } else if (rt_strcmp(uart_log_str, "off") == 0) {
        uart_log_toggle = uart_log_off;
    } else {
        /* do nothing */
    }

    if (rt_strcmp(uart_chanel_str, "uart_0") == 0) {
        uart_channel = uart_0;
        uart_console_name = "uart0";
    } else if (rt_strcmp(uart_chanel_str, "uart_1") == 0) {
        uart_channel = uart_1;
        uart_console_name = "uart1";
    } else if (rt_strcmp(uart_chanel_str, "uart_2") == 0) {
        uart_channel = uart_2;
        uart_console_name = "uart2";
    } else if (rt_strcmp(uart_chanel_str, "uart_3") == 0) {
        uart_channel = uart_3;
        uart_console_name = "uart3";
    } else if (rt_strcmp(uart_chanel_str, "uart_4") == 0) {
        uart_channel = uart_4;
        uart_console_name = "uart4";
    } else if (rt_strcmp(uart_chanel_str, "uart_5") == 0) {
        uart_channel = uart_5;
        uart_console_name = "uart5";
    } else {
        /* do nothing */
    }

    if (rt_strcmp(uart_baudrate_str, "9600") == 0) {
        uart_baudrate = uart_baudrate_9600;
    } else if (rt_strcmp(uart_baudrate_str, "19200") == 0) {
        uart_baudrate = uart_baudrate_19200;
    } else if (rt_strcmp(uart_baudrate_str, "38400") == 0) {
        uart_baudrate = uart_baudrate_38400;
    } else if (rt_strcmp(uart_baudrate_str, "57600") == 0) {
        uart_baudrate = uart_baudrate_57600;
    } else if (rt_strcmp(uart_baudrate_str, "115200") == 0) {
        uart_baudrate = uart_baudrate_115200;
    } else if (rt_strcmp(uart_baudrate_str, "921600") == 0) {
        uart_baudrate = uart_baudrate_921600;
    } else {
        /* do nothing */
    }

    if (rt_strcmp(log_level_str, "debug") == 0) {
        ax_log_level = AX_LOG_LEVEL_DEBUG;
    } else if (rt_strcmp(log_level_str, "info") == 0) {
        ax_log_level = AX_LOG_LEVEL_INFO;
    } else if (rt_strcmp(log_level_str, "warn") == 0) {
        ax_log_level = AX_LOG_LEVEL_WARNING;
    } else if (rt_strcmp(log_level_str, "error") == 0) {
        ax_log_level = AX_LOG_LEVEL_ERROR;
    } else if (rt_strcmp(log_level_str, "critical") == 0) {
        ax_log_level = AX_LOG_LEVEL_CRITICAL;
    } else if (rt_strcmp(log_level_str, "notice") == 0) {
        ax_log_level = AX_LOG_LEVEL_NOTICE;
    } else {
        /* do nothing */
    }
}

static void log_2_mem_init(void)
{
#ifdef LOG_MEM_DDR_START
    log_2_mem_header = (log_2_mem_header_t *)LOG_MEM_DDR_START;
    log_2_mem_header->magic = LOG_MAGIC;
    rt_strcpy(log_2_mem_header->version, LOG_VERSION);
    log_2_mem_header->mem_len = LOG_MEM_DDR_LEN;
    log_2_mem_header->header_addr = LOG_MEM_DDR_START;
    log_2_mem_header->header_len = sizeof(log_2_mem_header_t);
    log_2_mem_header->log_addr = log_2_mem_header->header_addr + log_2_mem_header->header_len;
    log_2_mem_header->log_total_len = LOG_MEM_DDR_LEN - log_2_mem_header->header_len;
    log_2_mem_header->log_mem_write = log_2_mem_header->header_addr + log_2_mem_header->header_len;
    log_2_mem_header->log_mem_cnt = 0;

    log_2_mem_ready = 1;
#endif
}

int ax_log_init(void)
{
    log_fw_env_init();
    log_2_mem_init();
    return 0;
}
