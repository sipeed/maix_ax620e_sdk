#ifndef __TIMESTAMP_H_
#define __TIMESTAMP_H_
#include "wakeup_source.h"
typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned char u8;
#define MAX_SUBID 10
#define AX_ID_ATF 46
#define AX_SUB_ID_SUSPEND_END 3
#define AX_SUB_ID_SUSPEND_START 2
#define AX_SUB_ID_RESUME_START 0
#define AX_SUB_ID_RESUME_END 1
typedef struct {
	u64 wakeup_tmr64_cnt;
	u32 store_index;
} timestamp_header_t;
typedef struct {
	u32 data[MAX_SUBID];
}timestamp_data_t;
unsigned int ax_sys_sleeptimestamp(int modid, unsigned int subid);
#endif