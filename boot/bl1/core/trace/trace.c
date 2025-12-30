/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "trace.h"
#include "cmn.h"
#include "uart.h"

#if defined (TRACE_ENV_EDA)
enum AX_LOG_LEVEL ax_log_threhold = AX_LOG_LEVEL_OFF;
#elif defined (DEBUG)
enum AX_LOG_LEVEL ax_log_threhold = AX_LOG_LEVEL_DEBUG;
#else
enum AX_LOG_LEVEL ax_log_threhold = AX_LOG_LEVEL_ERROR;
#endif
enum AX_LOG_LEVEL ax_log_level = AX_LOG_LEVEL_DEBUG;

void ax_print_str(char * string)
{
	char * ch = (char *)string;

	if (ax_log_level > ax_log_threhold)
		return;

	while('\0' != *ch) {
		uart_putc(UART0_BASE, *ch++, 100);
	}
}

void ax_print_num(unsigned long int unum, unsigned int radix)
{
	/* Just need enough space to store 64 bit decimal integer */
	char num_buf[20];
	int i = 0;
	unsigned int rem;

	if (ax_log_level > ax_log_threhold)
		return;

	do {
		rem = unum % radix;
		if (rem < 0xa)
			num_buf[i] = '0' + rem;
		else
			num_buf[i] = 'a' + (rem - 0xa);
		i++;
		unum /= radix;
	} while (unum > 0U);

	while (--i >= 0) {
		uart_putc(UART0_BASE, num_buf[i], 100);
	}
}

