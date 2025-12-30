#ifndef __PRINT_H_
#define __PRINT_H_

#include "config.h"


#define UART0_BASE		0x4880000

void uart_init(int base);
void uart_putc(int base, const char ch, unsigned int timeout_us);


static inline void print_init(void)
{
    uart_init(UART0_BASE);
}

static inline void output_char(char c)
{
    uart_putc(UART0_BASE, c, 50);
}


int printf_tb(const char *fmt, ...);

#define NOOP(...)

#if (TEST_PLATFORM == HAPS)
#define print NOOP
#define debug NOOP
#define info  printf_tb
#define err   printf_tb
#define warn  printf_tb
#else
#define print NOOP
#define debug NOOP
#define info  NOOP
#define err   NOOP
#define warn  NOOP
#endif

#endif