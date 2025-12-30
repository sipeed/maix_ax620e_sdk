#include <wakeup.h>
#include <wakeup_source.h>
#include <sys_all_have.h>
#include "ax620e_common_sys_glb.h"
#include <sleep_stage.h>
#include <platform_def.h>
//static console_t console;
extern console_t console;

void wakeup_sys(void)
{
	static unsigned long times = 0;
	writel(AX_WAKEUP_STAGE_16, SLEEP_STAGE_STORE_ADDR);
	peri_sys_wakeup();
	writel(AX_WAKEUP_STAGE_17, SLEEP_STAGE_STORE_ADDR);
	console_16550_register(UART0_BASE, AX620E_UART_CLOCK, AX620E_BAUDRATE, &console);
	flash_sys_wakeup();
	writel(AX_WAKEUP_STAGE_18, SLEEP_STAGE_STORE_ADDR);
	isp_sys_wakeup();
	writel(AX_WAKEUP_STAGE_19, SLEEP_STAGE_STORE_ADDR);
	mm_sys_wakeup();
	writel(AX_WAKEUP_STAGE_1A, SLEEP_STAGE_STORE_ADDR);
	npu_sys_wakeup();
	writel(AX_WAKEUP_STAGE_1B, SLEEP_STAGE_STORE_ADDR);
	vpu_sys_wakeup();
	writel(AX_WAKEUP_STAGE_1C, SLEEP_STAGE_STORE_ADDR);

	NOTICE("wakeup,times:%ld\r\n", times++);

	/* gtimer init */
	writel(0x0, 0x1b30008);
	writel(0x0, 0x1b3000c);
	writel(0x16e3600, 0x1b30020);
	writel(0x11, 0x1b30000);
	writel(AX_WAKEUP_STAGE_1D, SLEEP_STAGE_STORE_ADDR);
}
