#ifndef __SYS_ALL_HAVE_H_
#define __SYS_ALL_HAVE_H_
#include <lib/mmio.h>
#include <common/debug.h>


static inline void writel(uint32_t value, uintptr_t addr)
{
	*(volatile uint32_t*)addr = value;
}

static inline uint32_t readl(uintptr_t addr)
{
	return *(volatile uint32_t*)addr;
}
#endif