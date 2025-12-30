#ifndef __TIMER_H
#define __TIMER_H

#include "cmn.h"

#define TIMER_BASE				0x4830000
#define TIMER_LOADCOUNT			(TIMER_BASE + 0x0)
#define TIMER_CURRENT_VALUE		(TIMER_BASE + 0x4)
#define TIMER_CONTROL_REG		(TIMER_BASE + 0x8)

/* control reg bit field */
#define TIMER_INT_MASK			0x4
#define USER_MODE				0x2
#define TIMER_ENABLE			0x1

#define TICK2SEC				24000000
#define TICK2MSEC				24000
#define TICK2USEC				24

typedef enum {
	SEC = 1,
	USEC = 2,
	MSEC = 3
} UNIT;

u32 getCurrTime(UNIT time_unit);
void delay(u32 s);
void mdelay(u32 ms);
void udelay(u32 us);
void timer_init(void);

#endif
