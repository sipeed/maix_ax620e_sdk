#ifndef __WAKEUP_SOURCE_H_
#define __WAKEUP_SOURCE_H_
#include <deb_gpio.h>
#include <sys_all_have.h>

#define TIMER32_COUNT_PER_SEC	32768

#define TIMER32_BASE_ADDR		0x2240000
#define TIMER32_CNT_CCVR		(TIMER32_BASE_ADDR + 0x0)
#define TIMER32_CMR			(TIMER32_BASE_ADDR + 0x4)
#define TIMER32_CMR_START		(TIMER32_BASE_ADDR + 0x10)
#define TIMER32_INTR_STATUS		(TIMER32_BASE_ADDR + 0x28)
#define BIT_TIMER32_CMR_START		BIT(0)
#define TIMER32_INTR_CTRL		(TIMER32_BASE_ADDR + 0x1C)
#define BIT_TIMER32_INTR_EOI		BIT(2)
#define BIT_TIMER32_INTR_MASK		BIT(1)
#define BIT_TIMER32_INTR_EN		BIT(0)

/* TIMER64 REG */
#define TIMER64_CNT_LOW_ADDR		0x2250000

extern void timer32_wakeup_config(unsigned int wait_count);
extern void timer32_init(void);
extern void delayus(unsigned int us);
extern void delayms(unsigned int ms);
extern void delays(unsigned int s);
#endif
