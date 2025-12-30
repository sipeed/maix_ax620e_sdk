#include "cmn.h"
#include "board.h"
#include "chip_reg.h"
#include "timer.h"
#include "fdl_channel.h"
#include "fdl_engine.h"
//#include "lpddr4x.h"
#include "uart.h"
#include "boot.h"
#include "printf.h"
#include "trace.h"
#include "pwm.h"

/* chip_mode[0] */
#define USB_DL_MASK     (1 << 0)
/* chip_mode[2:1] */
#define FLASH_BOOT_MASK (3 << 1)
/* chip_mode[3] */
#define SD_BOOT_MASK    (1 << 3)

extern FDL_ChannelHandler_T *g_CurrChannel;
extern struct FDL_ChannelHandler gUartChannel;

void setup_download_mode(u32 flash_type, u32 dl_channel)
{
	boot_mode_info_t *boot_mode = (boot_mode_info_t *) BOOT_MODE_INFO_ADDR;
	ax_memset((void *)boot_mode, 0, sizeof(boot_mode_e));
	boot_mode->magic = BOOT_MODE_ENV_MAGIC;
	boot_mode->mode = BOOT_MODE_NORMALBOOT;
	boot_mode->dl_channel = dl_channel;
	boot_mode->is_sd_boot = 0;
	if (boot_mode->dl_channel == DL_CHAN_UART1) {
		info("The download form uart1\r\n");
	}
	if (boot_mode->dl_channel == DL_CHAN_UART) {
		info("The download form uart0\r\n");
	}
	if (boot_mode->dl_channel == DL_CHAN_USB) {
		info("The download form usb\r\n");
	}
	if (boot_mode->dl_channel == DL_CHAN_SPI) {
		info("The download form spi slv\r\n");
	}
	switch (flash_type) {
	case FLASH_EMMC:
		boot_mode->storage_sel = STORAGE_TYPE_EMMC;
		boot_mode->boot_type = EMMC_BOOT_UDA;
		info("download to eMMC\r\n");
		break;
	case FLASH_EMMC_BOOT_8BIT_50M_768K:
		boot_mode->storage_sel = STORAGE_TYPE_EMMC;
		boot_mode->boot_type = EMMC_BOOT_8BIT_50M_768K;
		info("download to EMMC_BOOT_8BIT_50M_768K\r\n");
		break;
	case FLASH_EMMC_BOOT_4BIT_25M_768K:
		boot_mode->storage_sel = STORAGE_TYPE_EMMC;
		boot_mode->boot_type = EMMC_BOOT_4BIT_25M_768K;
		info("download to EMMC_BOOT_4BIT_25M_768K\r\n");
		break;
	case FLASH_EMMC_BOOT_4BIT_25M_128K:
		boot_mode->storage_sel = STORAGE_TYPE_EMMC;
		boot_mode->boot_type = EMMC_BOOT_4BIT_25M_128K;
		info("download to EMMC_BOOT_4BIT_25M_128K\r\n");
		break;
	case FLASH_NOR:
		boot_mode->storage_sel = STORAGE_TYPE_NOR;
		boot_mode->boot_type = NOR;
		info("download to NOR\r\n");
		break;
	case FLASH_NAND_2K:
		boot_mode->storage_sel = STORAGE_TYPE_NAND;
		boot_mode->boot_type = NAND_2K;
		info("download to NAND_2K\r\n");
		break;
	case FLASH_NAND_4K:
		boot_mode->storage_sel = STORAGE_TYPE_NAND;
		boot_mode->boot_type = NAND_4K;
		info("download to NAND_4K\r\n");
		break;
	default:
		return;
	}

	info("enter download mode\r\n");
}

extern int uart_write(int base, u8 * buf, int len);
extern void mc20e_ddr_init(void * rom_param);
// extern int printf_tb(const char *fmt, ...);
int main(void)
{
	int ret = 0;
	char flash_type, chip_mode;
	u32 dl_channel;
	ax_print_str("hello fdl\r\n");
#if !defined (DDR_ENV_EDA) && !defined (DDR_ENV_ATE)
	timer_init();
	get_misc_info();
	chip_init();
	wtd_enable(0);
	uart_init(USE_UART);
	uart_init(UART1_BASE);
	set_pwm_volt();

	chip_mode = readl(TOP_CHIPMODE_GLB_SW);
	flash_type = get_boot_mode(chip_mode);
	dl_channel = readl(COMM_SYS_DUMMY_SW5);
	setup_download_mode(flash_type, dl_channel);
#endif
	ax_memset((void *)DDR_INFO_ADDR, 0, sizeof(ddr_info_t));
	mc20e_ddr_init(NULL);

	if (dl_channel == DL_CHAN_UART1)
		dl_channel = DL_CHAN_UART;
	ret = fdl_channel_init(dl_channel, 50000);
	if (ret < 0)
		goto boot_trap;

	fw_load_and_exec();

boot_trap:
	while (1) ;

	return 0;
}
