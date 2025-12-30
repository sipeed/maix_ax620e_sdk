#ifndef __WDT_H_
#define __WDT_H_
#include <lib/mmio.h>
#define WDT0_BASE	0x4840000

void wdt0_enable(int enable);
#endif