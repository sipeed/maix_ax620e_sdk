#include "cmn.h"

extern u32 _bss_start;
extern u32 _bss_end;
extern u32 _size_bss;

void clear_bss_section(void)
{
volatile u32* start = (volatile u32*)&_bss_start;
volatile u32* end = (volatile u32*)&_bss_end;
	while(start < end)
		*start++ = 0;
}

