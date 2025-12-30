#include "cmn.h"
#include "chip_reg.h"
#include "timer.h"
#include "trace.h"
#include "boot.h"
#include "board.h"
#include "secure.h"
#include "efuse_drv.h"
#include "uart.h"
#include "pci.h"
#include "printf.h"
#include "atf.h"
#include "pwm.h"
#include "riscv.h"
#include "ax_timestamp.h"
#ifndef AX620E_SUPPORT_SD
#include "fw_env.h"
#endif

#define SPL_START_ADDR        (0x03000400)
#define COMM_SYS_DUMMY_SW0    (0x02340000 + 0xDC) //second core jump addr [31:0]
#define COMM_SYS_DUMMY_SW1    (0x02340000 + 0xE0) //second core jump addr [39:32]
#define SECONDARY_TIMEOUT     50
extern unsigned int pen_release[2];

#ifdef DEBUG_BOOT_TIME
	#define WRITE_DEBUG_BOOT_TIME(addr) writel(readl(TIMER_64_0_BASE), addr)
#else
	#define WRITE_DEBUG_BOOT_TIME(...)
#endif
#define DBG_READ_DDR_INIT_DATA_START (0x904)
#define DBG_READ_DDR_INIT_DATA_END   (0x908)
#define DBG_DDR_INIT_END             (0x90c)
#define DBG_READ_KERNEL_END          (0x910)
#define DBG_READ_DTB_END             (0x914)
#define DBG_READ_RAMDISK_END         (0x918)
#define DBG_READ_IMAGE_END           (0x91c)

#ifdef DDR_ENV_EDA
struct ddr_dfs_vref_t ddr_rom_param[DDR_DFS_MAX] = {0};

static void set_ddr_rom_param(void * rom_param)
{
	struct ddr_dfs_vref_t * dfs_info = (struct ddr_dfs_vref_t *)rom_param;

	dfs_info->freq = DDR_CLK_3200;
	dfs_info->dram_VREF_CA[0] = 0x10;
	dfs_info->dram_VREF_CA[1] = 0x20;
	dfs_info->dram_VREF_DQ[0] = 0x50;
	dfs_info->dram_VREF_DQ[1] = 0x70;
	dfs_info->rf_io_vrefi_adj_PHY_A = 0x60;
	dfs_info->rf_io_vrefi_adj_PHY_B = 0x90;
}
#endif


#if CONFIG_ARM64
void jump_to_execute(u32 start_addr)
{
	/* make sure wtd enabled before jump */
	wtd_enable(1);
	__asm__ __volatile__ (
		"mov x0,%0\n\t"
		"br x0\n\t"
		:
		: "r" (start_addr));
}
#else
#ifdef AX_BOOT_OPTIMIZATION_SUPPORT
typedef void (*arm32_jump_to_kernel)(u32 kernel_addr, u32 machine_type, u32 dtb_addr);
void jump_to_execute(u32 start_addr)
{
	writel(BOOT_KERNEL_FAIL, TOP_CHIPMODE_GLB_BACKUP0);
	/* make sure wtd enabled before jump */
	wtd_enable(1);
	arm32_jump_to_kernel func = (arm32_jump_to_kernel)start_addr;
	func(0, 0x8e0, DTB_IMG_HEADER_ADDR + 0x400);
}
#else
void jump_to_execute(u32 start_addr)
{
	/* make sure wtd enabled before jump */
	wtd_enable(1);
	__asm__ __volatile__ (
		"mov r0,%0\n\t"
		"bx r0\n\t"
		:
		: "r" (start_addr)
	);
}
#endif
#endif

#ifndef SUPPORT_ATF
#if 0
void prepare_boot(unsigned long boot_addr, unsigned long dtb_addr)
{
	__asm__ __volatile__("mov r6,r0\n\t");
	__asm__ __volatile__("mov r7,r1\n\t");
}
#endif

void wakeup_slave(void) {

	unsigned int last_time;
	int wake_flag = 0;

	*((unsigned int*)(COMM_SYS_DUMMY_SW1)) = 0x0;
	*((unsigned int*)(COMM_SYS_DUMMY_SW0)) = SPL_START_ADDR;
	__asm__ __volatile__("sev\n\t");

	/* waiting secondary cores to finish, the getCurrTime is decrease */
	last_time = getCurrTime(MSEC);
	do {
		if (pen_release[1] == -1) {
			wake_flag = 1;
			break;
		}
	} while ((last_time - getCurrTime(MSEC)) < SECONDARY_TIMEOUT);

	if (!wake_flag) {
		info("wake secondary cpus fail state:%d\r\n", pen_release[1]);
		while (1);
	}

	/* reset dummy to 0, secondary cores holding */
	writel(0, COMM_SYS_DUMMY_SW0);

}
#endif

static void setup_boot_mode(u32 flash_type)
{
	boot_mode_info_t *boot_mode = (boot_mode_info_t *) BOOT_MODE_INFO_ADDR;
	ax_memset((void *)boot_mode, 0, sizeof(boot_mode_info_t));
	boot_mode->magic = BOOT_MODE_ENV_MAGIC;
	boot_mode->mode = BOOT_MODE_NORMALBOOT;
	boot_mode->is_sd_boot = false;

	if (DL_CHAN_SD == readl(COMM_SYS_DUMMY_SW5)) {
		boot_mode->is_sd_boot = true;
		info("The system boot from SD\r\n");
	}
	switch (flash_type) {
	case FLASH_EMMC:
		boot_mode->storage_sel = STORAGE_TYPE_EMMC;
		boot_mode->boot_type = EMMC_BOOT_UDA;
		if (boot_mode->is_sd_boot == true)
			info("storage is eMMC\r\n");
		else
		info("The system boot from eMMC\r\n");
		break;
	case FLASH_EMMC_BOOT_8BIT_50M_768K:
		boot_mode->storage_sel = STORAGE_TYPE_EMMC;
		boot_mode->boot_type = EMMC_BOOT_8BIT_50M_768K;
		if (boot_mode->is_sd_boot == true)
			info("storage is EMMC_BOOT_8BIT_50M_768K\r\n");
		else
		info("The system boot from EMMC_BOOT_8BIT_50M_768K\r\n");
		break;
	case FLASH_EMMC_BOOT_4BIT_25M_768K:
		boot_mode->storage_sel = STORAGE_TYPE_EMMC;
		boot_mode->boot_type = EMMC_BOOT_4BIT_25M_768K;
		if (boot_mode->is_sd_boot == true)
			info("storage is EMMC_BOOT_4BIT_25M_768K\r\n");
		else
		info("The system boot from EMMC_BOOT_4BIT_25M_768K\r\n");
		break;
	case FLASH_EMMC_BOOT_4BIT_25M_128K:
		boot_mode->storage_sel = STORAGE_TYPE_EMMC;
		boot_mode->boot_type = EMMC_BOOT_4BIT_25M_128K;
		if (boot_mode->is_sd_boot == true)
			info("storage is EMMC_BOOT_4BIT_25M_128K\r\n");
		else
		info("The system boot from EMMC_BOOT_4BIT_25M_128K\r\n");
		break;
	case FLASH_NOR:
		boot_mode->storage_sel = STORAGE_TYPE_NOR;
		boot_mode->boot_type = NOR;
		if (boot_mode->is_sd_boot == true)
			info("storage is NOR\r\n");
		else
		info("The system boot from NOR\r\n");
		break;
	case FLASH_NAND_2K:
		boot_mode->storage_sel = STORAGE_TYPE_NAND;
		boot_mode->boot_type = NAND_2K;
		if (boot_mode->is_sd_boot == true)
			info("storage is NAND_2K\r\n");
		else
		info("The system boot from NAND_2K\r\n");
		break;
	case FLASH_NAND_4K:
		boot_mode->storage_sel = STORAGE_TYPE_NAND;
		boot_mode->boot_type = NAND_4K;
		if (boot_mode->is_sd_boot == true)
			info("storage is NAND_4K\r\n");
		else
		info("The system boot from NAND_4K\r\n");
		break;
	default:
		return;
	}

	info("enter boot normal mode\r\n");
}

extern void    mc20e_ddr_init(void * rom_param);
#ifndef AX620E_SUPPORT_SD
#define SPL_BOOT_STR  "\r\nspl boot @ ocm: 0x"
#define SPL_BOOT_ADDR (0x3000400)
#endif

#ifdef SAMPLE_EFUSE_WRITE
#define PUBKEY_HASH_SIZE_U32	8
void sample_efuse_hash()
{
	int i, ret;
	/* replace it */
	u32 pub_key_hash[PUBKEY_HASH_SIZE_U32] = {
		0x78DDFE9E, 0x698A9288, 0xCCAD1548, 0x5CA04574,
		0x9F1AA945, 0xF57EB822, 0xB807DB67, 0x29140398,
	};

	ret = efuse_init();
	if (ret != EFUSE_SUCCESS) {
		ax_print_str("sample_efuse_hash efuse_init fail\r\n");
		goto loop_while;
	}

	ax_print_str("write pub_key_hash to efuse begin\r\n");
	for (i = 0; i < PUBKEY_HASH_SIZE_U32; i++) {
		ret = efuse_write(PUBKEY_HASH_BLK_START + i, pub_key_hash[i]);
		if (ret != EFUSE_SUCCESS)
			break;
	}
	if (ret != EFUSE_SUCCESS)
		ax_print_str("write pub_key_hash end with error\r\n");
	else
		ax_print_str("write pub_key_hash end successfully\r\n");

loop_while:
	ax_print_str("sample_efuse_hash loop while\r\n");
	while (1);
}
#endif

int flash_uboot_process(char flash_type)
{
	int boot_addr;
	boot_addr = flash_boot(flash_type, UBOOT, UBOOT_HEADER_FLASH_BASE, UBOOT_HEADER_BAK_FLASH_BASE, UBOOT_IMG_HEADER_BASE);
	info("get uboot entry is 0x%x\r\n", boot_addr);
	if (boot_addr <= 0) {
		goto failed;
	}
	return boot_addr;
failed:
	AX_LOG_STR_ERROR("uboot flash boot failed\r\n");
	while(1);
}

static void is_boot_uboot(char flash_type)
{
	int boot_addr = 0;
	u32 boot_reg = readl(TOP_CHIPMODE_GLB_BACKUP0);
	if ((boot_reg & BOOT_DOWNLOAD)
	|| (boot_reg & BOOT_RECOVERY)
	|| (boot_reg & BOOT_KERNEL_FAIL)) {
		boot_addr = flash_uboot_process(flash_type);
		jump_to_execute(boot_addr);
	}
}

#ifdef AX_BOOT_OPTIMIZATION_SUPPORT
static int fast_boot_process(char flash_type, int *dtb_addr)
{
	int boot_addr = 0;

	boot_addr = flash_boot(flash_type, KERNEL, KERNEL_HEADER_FLASH_BASE, KERNEL_HEADER_BAK_FLASH_BASE, KERNEL_IMG_HEADER_ADDR);
	WRITE_DEBUG_BOOT_TIME(DBG_READ_KERNEL_END);//read kernel end
	*dtb_addr = flash_boot(flash_type, DTB, DTB_HEADER_FLASH_BASE, DTB_HEADER_BAK_FLASH_BASE, DTB_IMG_HEADER_ADDR);
	WRITE_DEBUG_BOOT_TIME(DBG_READ_DTB_END); //read dtb end

	#ifdef SUPPORT_RAMDISK
#ifndef AX_RISCV_LOAD_ROOTFS_SUPPORT
		writel(SW4_LOAD_ROOTFS_DONE, COMM_SYS_DUMMY_SW4);
		int ramdisk_addr = flash_boot(flash_type, RAMDISK, RAMDISK_FLASH_POS, 0, RAMDISK_START_ADDR);
		info("get kernel entry is 0x%x, dtb_addr:0x%x, ramdisk_addr:0x%x\r\n", boot_addr, *dtb_addr, ramdisk_addr);
		if (*dtb_addr <= 0 || boot_addr <= 0 || ramdisk_addr <= 0) {
			goto failed;
		}
#else
		if (*dtb_addr <= 0 || boot_addr <= 0) {
			goto failed;
		}
#endif
	#else
		info("get kernel entry is 0x%x, dtb_addr:0x%x\r\n", boot_addr, *dtb_addr);
		if (*dtb_addr <= 0 || boot_addr <= 0) {
			goto failed;
		}
	#endif
	WRITE_DEBUG_BOOT_TIME(DBG_READ_RAMDISK_END);  //read ramdisk end

	return boot_addr;

failed:
	AX_LOG_STR_ERROR("fast boot failed\r\n");
	while(1);
}
#endif

#ifdef SUPPORT_ATF
static int atf_process(char flash_type, int boot_addr, int dtb_addr)
{
	int atf_addr = 0;
	atf_boot_fn atf_boot;
	int optee_addr = 0;
#ifdef SUPPORT_RECOVERY
	u32 boot_reg = readl(TOP_CHIPMODE_GLB_BACKUP0);
#endif
#ifdef OPTEE_BOOT
	optee_addr = flash_boot(flash_type, OPTEE, OPTEE_HEADER_FLASH_BASE, OPTEE_HEADER_BAK_FLASH_BASE, OPTEE_IMAGE_ADDR - sizeof(struct img_header));
	info("get optee entry is 0x%x\r\n", optee_addr);
	if (optee_addr <= 0) {
		goto failed;
	}
#endif
	if (boot_addr > 0) {
		if ((flash_type == FLASH_NOR)
		|| (flash_type == FLASH_NAND_2K)
		|| (flash_type == FLASH_NAND_4K)
		|| (flash_type == FLASH_EMMC)
		|| (flash_type == FLASH_EMMC_BOOT_8BIT_50M_768K)
		|| (flash_type == FLASH_EMMC_BOOT_4BIT_25M_128K)
		|| (flash_type == FLASH_EMMC_BOOT_4BIT_25M_768K)
		|| (flash_type == FLASH_SD)) {
			atf_addr = flash_boot(flash_type, ATF, ATF_HEADER_FLASH_BASE, ATF_HEADER_BAK_FLASH_BASE, ATF_IMG_HEADER_BASE);
			if (atf_addr > 0) {
				atf_boot_prepare(boot_addr, optee_addr);
				atf_boot = (atf_boot_fn)(unsigned long)atf_addr;
		#ifdef AX_RISCV_LOAD_ROOTFS_SUPPORT
		#ifdef SUPPORT_RECOVERY
			if (!(boot_reg & BOOT_RECOVERY))
				riscv_start_load_rootfs();
		#else
			riscv_start_load_rootfs();
		#endif
		#endif
				info("enter atf, then start uboot..., atf entry is 0x%x\r\n", atf_addr);
				atf_boot((unsigned long)&atf_bl_params, 0, dtb_addr, ARM_BL31_PLAT_PARAM_VAL);
	}
		} else {
			info("flash_type error\r\n");
			goto failed;
		}
	}
	return 0;
failed:
	AX_LOG_STR_ERROR("atf boot failed\r\n");
	while(1);
}
#endif

#ifdef AX_RISCV_SUPPORT
static int riscv_boot_process(char flash_type)
{
	ax_timestamp(STAMP_SPL_START_LOAD_RISCV);
#ifdef AX_SUPPORT_AB_PART
	int riscv_addr = flash_boot(flash_type, RISCV, RISCV_HEADER_FLASH_BASE, RISCV_B_HEADER_FLASH_BASE, RISCV_HEADER_DDR_START);
#else
	int riscv_addr = flash_boot(flash_type, RISCV, RISCV_HEADER_FLASH_BASE, 0, RISCV_HEADER_DDR_START);
#endif
	ax_timestamp(STAMP_SPL_END_LOAD_RISCV);
	if (riscv_addr != RISCV_BIN_DDR_START) {
		goto failed;
	}

#ifndef AX620E_SUPPORT_SD
	if(fw_env_load(flash_type))
		ax_print_str("load ax env failed\r\n");
#endif
	riscv_boot_up();
	return 0;
failed:
	AX_LOG_STR_ERROR("riscv flash boot failed\r\n");
	while(1);
}
#endif

static void clk_aux_config(void)
{
	int val = 0;

	/* enable clk_aux1_1 function for pll work around low power way */
	val = readl(0x2300030);
	val &= ~(7 << 16);
	val |= (3 << 16);
	writel(val, 0x2300030);
	writel(0x110, 0x23401b8);
}

static void set_ddr_init(void)
{
	ax_memset((void *)DDR_INFO_ADDR, 0, sizeof(ddr_info_t));
#ifdef DDR_ENV_EDA
	set_ddr_rom_param((void *)&ddr_rom_param[0]);
	mc20e_ddr_init((void *)&ddr_rom_param[0]);
#elif defined (AX620E_SUPPORT_SD)
	mc20e_ddr_init(NULL);
#else
	mc20e_ddr_init((void *)DDRINIT_PARAM_HEADER_BASE + sizeof(struct img_header));
#endif
}

int main(void)
{
	int boot_addr = 0;
#if defined(AX_BOOT_OPTIMIZATION_SUPPORT) || defined(SUPPORT_ATF)
	int dtb_addr = 0;
#endif
	char flash_type, chip_mode;
	boot_mode_info_t *boot_mode = (boot_mode_info_t *) BOOT_MODE_INFO_ADDR;

	timer_init();
	get_misc_info();
	chip_init();
	generic_timer_init();

#ifdef SAMPLE_EFUSE_WRITE
	sample_efuse_hash();
#endif
#ifndef DDR_ENV_EDA
	uart_init(USE_UART);
	info("\r\nenter spl\r\n");

	set_pwm_volt();

#ifndef AX620E_SUPPORT_SD
	ax_print_str(SPL_BOOT_STR);
	ax_print_num(SPL_BOOT_ADDR, 16);
	ax_print_str("\r\n");
#endif
	chip_mode = readl(TOP_CHIPMODE_GLB_SW);
	flash_type = get_boot_mode(chip_mode);
	setup_boot_mode(flash_type);
	if (boot_mode->is_sd_boot)
		flash_type = FLASH_SD;
	WRITE_DEBUG_BOOT_TIME(DBG_READ_DDR_INIT_DATA_START); //read ddr init data start
#ifndef AX620E_SUPPORT_SD
	#ifdef SUPPORT_DDRINIT_PART
	if (flash_boot(flash_type, DDRINIT, DDRINIT_HEADER_FLASH_BASE, DDRINIT_HEADER_FLASH_BASE, DDRINIT_PARAM_HEADER_BASE) < 0) {
		ax_print_str("get ddrinit param in rom fail\r\n");
	}
	#endif
#endif
	WRITE_DEBUG_BOOT_TIME(DBG_READ_DDR_INIT_DATA_END); //read ddr init data end
#endif
	set_ddr_init();
	WRITE_DEBUG_BOOT_TIME(DBG_DDR_INIT_END); //ddr init end

	clk_aux_config();

	is_boot_uboot(flash_type);

/* fast boot use this. */
#ifdef AX_RISCV_SUPPORT
	riscv_boot_process(flash_type);
#endif

#ifdef AX_BOOT_OPTIMIZATION_SUPPORT
	boot_addr = fast_boot_process(flash_type, &dtb_addr);
#else
	boot_addr = flash_uboot_process(flash_type);
#endif

#ifdef SUPPORT_ATF
	atf_process(flash_type, boot_addr, dtb_addr);
	WRITE_DEBUG_BOOT_TIME(DBG_READ_IMAGE_END); //spl read img end
#else
	WRITE_DEBUG_BOOT_TIME(DBG_READ_IMAGE_END); //spl read img end
	/* using spintable, wakeup second core to jump to spl. */
	wakeup_slave();
	if (boot_addr > 0) {
/* fastnor use this marco, fastemmc not use. */
#ifdef AX_RISCV_LOAD_ROOTFS_SUPPORT
	riscv_start_load_rootfs();
#endif
		ax_print_str("jump to next boot\r\n");
		jump_to_execute(boot_addr);
		// prepare_boot(boot_addr, dtb_addr);
	} else {
#ifdef AX_BOOT_OPTIMIZATION_SUPPORT
		AX_LOG_STR_ERROR("fastboot fail\r\n");
#else
		AX_LOG_STR_ERROR("uboot fail\r\n");
#endif
		while(1);
	}
#endif
	return 0;
}
